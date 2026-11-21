#include "NativeUpdatePolicy.hpp"
#include <cassert>
#include <iostream>
using namespace Filuna::Update;
int main(){
    Version a,b;
    assert(parse_version("1.0.0",a)&&parse_version("1.0.1",b)&&a<b);
    for(auto s:{"","1","1.2","1.2.3.4","01.2.3","1.-2.3",
                "1.2.3-beta","4294967296.1.1","1.2.3\n","1.2."})
        assert(!parse_version(s,a));
    Installed i{{1,0,0},"owner-beta","windows","x64","base1",false,false};
    Release r{{1,0,1},{1,0,0},{1,0,1},Class::A,"owner-beta","windows",
              "x64","base1","Correções","",false,false};
    TrustEvidence t{true,true,true,true,true,true,false};
    assert(decide(i,r,t).outcome==Outcome::SyncData);
    t.ecdsa_manifest=false;assert(decide(i,r,t).outcome==Outcome::Block);
    t.ecdsa_manifest=true;
    r.notes.clear();assert(decide(i,r,t).outcome==Outcome::Block);
    r.notes="Mudanças";r.channel="stable";
    assert(decide(i,r,t).outcome==Outcome::Block);r.channel="owner-beta";
    r.kind=Class::B;assert(decide(i,r,t).outcome==Outcome::StageModule);
    r.kind=Class::C;r.restart=true;
    assert(decide(i,r,t).outcome==Outcome::Block);
    t.authenticode=true;
    assert(decide(i,r,t).outcome==Outcome::ConfirmRestart);
    i.unsaved=true;assert(decide(i,r,t).outcome==Outcome::Block);
    i.unsaved=false;i.running_task=true;assert(decide(i,r,t).outcome==Outcome::Block);
    i.running_task=false;r.update_base="other";
    assert(decide(i,r,t).outcome==Outcome::Block);
    r.update_base="base1";r.kind=Class::D;
    assert(decide(i,r,t).outcome==Outcome::Block);
    r.installer_necessary=true;r.installer_reason="Novo runtime";
    assert(decide(i,r,t).outcome==Outcome::ConfirmInstaller);
    r.version=i.version;assert(decide(i,r,t).outcome==Outcome::NoUpdate);
    std::cout<<"NATIVE_UPDATE_POLICY_PASS\n";
}
