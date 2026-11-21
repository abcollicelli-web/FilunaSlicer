#pragma once
// C++17 admission policy. Evidence fields are inputs from trusted LOCAL verifiers,
// never values copied from an update manifest. This module never mutates files.
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
namespace Filuna { namespace Update {
enum class Class { A, B, C, D };
enum class Outcome { NoUpdate, Block, SyncData, StageModule, ConfirmRestart, ConfirmInstaller };
struct Version {
    uint32_t major=0,minor=0,patch=0;
    bool operator==(const Version& b) const { return major==b.major&&minor==b.minor&&patch==b.patch; }
    bool operator<(const Version& b) const {
        return major!=b.major?major<b.major:minor!=b.minor?minor<b.minor:patch<b.patch;
    }
    bool operator<=(const Version& b) const { return *this<b || *this==b; }
};
inline bool parse_version(std::string_view s,Version& out) {
    Version v; uint32_t* fields[]={&v.major,&v.minor,&v.patch};size_t p=0;
    for(int n=0;n<3;++n){
        size_t start=p;uint64_t num=0;
        while(p<s.size() && s[p]>='0' && s[p]<='9'){
            num=num*10+unsigned(s[p++]-'0');
            if(num>std::numeric_limits<uint32_t>::max())return false;
        }
        if(p==start||(p-start>1&&s[start]=='0'))return false;
        *fields[n]=uint32_t(num);
        if(n<2){if(p>=s.size()||s[p]!='.')return false;++p;}
        else if(p!=s.size())return false;
    }
    out=v;return true;
}
struct Installed {
    Version version;
    std::string channel,os,arch,update_base;
    bool unsaved=false,running_task=false;
};
struct Release {
    Version version,min_client,max_client;
    Class kind=Class::A;
    std::string channel,os,arch,update_base,notes,installer_reason;
    bool restart=false,installer_necessary=false;
};
struct TrustEvidence {
    // These MUST be results of server membership + independent crypto/QA checks.
    bool account_grant=false,authenticated_https=false,ecdsa_manifest=false;
    bool payload_sha256=false,github_attestation=false,owner_qa=false,authenticode=false;
};
struct Decision {Outcome outcome; const char* code;};
inline Decision decide(const Installed& i,const Release& r,const TrustEvidence& t){
    if(r.version<=i.version)return{Outcome::NoUpdate,"NO_NEW_VERSION"};
    if(r.max_client<r.min_client||i.version<r.min_client||r.max_client<i.version)
        return{Outcome::Block,"INCOMPATIBLE_VERSION"};
    if(i.channel.empty()||i.channel!=r.channel||i.os!="windows"||i.os!=r.os||
       (i.arch!="x64"&&i.arch!="arm64")||i.arch!=r.arch)
        return{Outcome::Block,"CHANNEL_PLATFORM_MISMATCH"};
    if(!t.account_grant||!t.authenticated_https||!t.ecdsa_manifest||
       !t.payload_sha256||!t.github_attestation||!t.owner_qa)
        return{Outcome::Block,"TRUST_MISSING"};
    if(r.notes.empty())return{Outcome::Block,"NOTES_MISSING"};
    switch(r.kind){
    case Class::A:
        if(r.restart||r.installer_necessary)return{Outcome::Block,"DATA_CANNOT_RESTART"};
        return{Outcome::SyncData,"SIGNED_DATA"};
    case Class::B:
        if(r.installer_necessary)return{Outcome::Block,"MODULE_CANNOT_INSTALL"};
        return{Outcome::StageModule,"SIGNED_COMPATIBLE_MODULE"};
    case Class::C:
        if(!t.authenticode||!r.restart||r.installer_necessary||r.update_base.empty()||
           r.update_base!=i.update_base)return{Outcome::Block,"CODE_SIGN_OR_CUMULATIVE_BASE_MISSING"};
        if(i.unsaved||i.running_task)return{Outcome::Block,"SAVE_OR_TASK_PENDING"};
        return{Outcome::ConfirmRestart,"USER_CONFIRMATION_REQUIRED"};
    case Class::D:
        if(!t.authenticode||!r.restart||!r.installer_necessary||r.installer_reason.empty())
            return{Outcome::Block,"INSTALLER_PROOF_MISSING"};
        if(i.unsaved||i.running_task)return{Outcome::Block,"SAVE_OR_TASK_PENDING"};
        return{Outcome::ConfirmInstaller,"USER_INSTALLER_CONSENT_REQUIRED"};
    }
    return{Outcome::Block,"UNKNOWN_CLASS"};
}
constexpr int check_interval_minutes=30;
constexpr int max_server_retained_version_sets=5;
}} // Filuna::Update
