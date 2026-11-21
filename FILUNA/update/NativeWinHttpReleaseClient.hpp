#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <string>
#pragma comment(lib,"winhttp.lib")
namespace Filuna { namespace Update {
struct FeedResult { bool ok=false; unsigned status=0; std::string bytes; const char* error="NOT_REQUESTED"; };
inline bool valid_tenant(const std::string& id) {
    if(id.empty()||id.size()>80)return false;
    for(unsigned char c:id)
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return false;
    return true;
}
inline std::wstring release_catalog_path(const std::string& id) {
    if(!valid_tenant(id))return {};
    return L"/api/v1/tenants/"+std::wstring(id.begin(),id.end())+L"/releases";
}
struct HttpHandle {
    HINTERNET h;
    explicit HttpHandle(HINTERNET v):h(v){}
    ~HttpHandle(){if(h)WinHttpCloseHandle(h);}
    operator HINTERNET() const{return h;}
    HttpHandle(const HttpHandle&)=delete;
    HttpHandle& operator=(const HttpHandle&)=delete;
};
// Host is pinned to the configured Filuna service. No redirects, TLS bypass,
// automatic retry, credentials in URLs or API-provided command execution.
// HTTP 200 bytes remain UNTRUSTED until independently verified.
inline FeedResult private_release_catalog(const std::string& tenant,
    const std::wstring& access_token,const std::wstring& host=L"filuna3d.com") {
    FeedResult out;
    if(!valid_tenant(tenant)||access_token.empty()||host!=L"filuna3d.com"||
       access_token.find(L'\r')!=std::wstring::npos||
       access_token.find(L'\n')!=std::wstring::npos){
        out.error="AUTH_OR_HOST_INVALID";return out;
    }
    HttpHandle session(WinHttpOpen(L"FilunaNativeUpdate/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                   WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
    if(!session.h){out.error="SESSION_FAILED";return out;}
    if(!WinHttpSetTimeouts(session,5000,5000,10000,10000)){
        out.error="TIMEOUT_POLICY_FAILED";return out;
    }
    HttpHandle connection(WinHttpConnect(session,host.c_str(),INTERNET_DEFAULT_HTTPS_PORT,0));
    if(!connection.h){out.error="CONNECT_FAILED";return out;}
    const std::wstring path=release_catalog_path(tenant);
    HttpHandle request(WinHttpOpenRequest(connection,L"GET",path.c_str(),
                       nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!request.h){out.error="REQUEST_FAILED";return out;}
    DWORD redirects=WINHTTP_DISABLE_REDIRECTS;
    if(!WinHttpSetOption(request,WINHTTP_OPTION_DISABLE_FEATURE,&redirects,sizeof(redirects))){
        out.error="REDIRECT_POLICY_FAILED";return out;
    }
    std::wstring header=L"Authorization: Bearer "+access_token;
    BOOL h_ok=WinHttpAddRequestHeaders(request,header.c_str(),DWORD(header.size()),WINHTTP_ADDREQ_FLAG_ADD);
    SecureZeroMemory(&header[0],header.size()*sizeof(wchar_t));
    if(!h_ok){out.error="AUTH_HEADER_FAILED";return out;}
    if(!WinHttpSendRequest(request,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||
       !WinHttpReceiveResponse(request,nullptr)){
        out.error="RESPONSE_FAILED";return out;
    }
    DWORD status=0,sz=sizeof(status);
    if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,&status,&sz,WINHTTP_NO_HEADER_INDEX)){
        out.error="STATUS_FAILED";return out;
    }
    out.status=status;
    if(status!=200){out.error="SERVER_DENIED";return out;}
    char chunk[8192];
    for(;;){
        DWORD n=0;
        if(!WinHttpReadData(request,chunk,sizeof(chunk),&n)){
            out.bytes.clear();out.error="READ_FAILED";return out;
        }
        if(n==0)break;
        if(out.bytes.size()+n>128*1024){
            out.bytes.clear();out.error="RESPONSE_TOO_LARGE";return out;
        }
        out.bytes.append(chunk,n);
    }
    out.ok=true;out.error="BYTES_NOT_YET_VERIFIED";return out;
}
}}
#endif
