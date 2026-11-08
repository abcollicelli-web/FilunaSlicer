#include "NativeWinHttpReleaseClient.hpp"
#include <cassert>
#include <iostream>
using namespace Filuna::Update;
int main(){
    assert(valid_tenant("owner_beta-1"));
    for(auto bad:{"","../x","a/b","name?x","sp ace","x%2F","a.b","\n"})
        assert(!valid_tenant(bad));
    assert(release_catalog_path("a9")==L"/api/v1/tenants/a9/releases");
    assert(release_catalog_path("../evil").empty());
    auto blocked=private_release_catalog("a9",L"");
    assert(!blocked.ok&&blocked.status==0);
    blocked=private_release_catalog("a9",L"x",L"other.example");
    assert(!blocked.ok&&blocked.status==0);
    blocked=private_release_catalog("a9",L"x\r\nX-Test: yes");
    assert(!blocked.ok&&blocked.status==0);
    std::cout<<"FILUNA_RELEASE_TRANSPORT_GUARDS_PASS\n";
}
