// Filuna Bambu OEM delta: LAN SSDP identity parser, AGPL-3.0-or-later.
// Official source evidence: bambulab/BambuStudio
// resources/profiles/BBL/machine/Bambu Lab A1.json model_id=N2S.
// No manufacturer cloud secrets or proprietary network plugin used.
// Discovery is not authentication, pairing, online heartbeat, or print authorization.
#pragma once
#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <string_view>
namespace Slic3r::Filuna::Bambu {
struct LanIdentity {
    std::string device_id;
    std::string model_code;
    std::string model_name;
    std::string display_name;
    std::string host;
    bool valid_a1 = false;
};
inline std::string trim(std::string s) {
    const auto start=s.find_first_not_of(" \t\r\n");
    if(start==std::string::npos) return {};
    const auto end=s.find_last_not_of(" \t\r\n");
    return s.substr(start,end-start+1);
}
inline std::string lower(std::string s) {
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return s;
}
inline LanIdentity parse_ssdp(std::string_view packet) {
    LanIdentity id;
    if(packet.size()>8192 || packet.find("HTTP/1.1 200 OK")==std::string_view::npos) return id;
    std::map<std::string,std::string> fields;
    std::size_t pos=0;
    while(pos<packet.size()){
        const auto end=packet.find('\n',pos);
        const auto limit=(end==std::string_view::npos)?packet.size():end;
        const std::string row=trim(std::string(packet.substr(pos,limit-pos)));
        pos=(end==std::string_view::npos)?packet.size():end+1;
        const auto colon=row.find(':');
        if(colon!=std::string::npos) fields[lower(trim(row.substr(0,colon)))]=trim(row.substr(colon+1));
    }
    if(fields["st"]!="urn:bambulab-com:device:3dprinter:1") return id;
    id.device_id=fields["usn"];
    id.model_code=fields["devmodel.bambu.com"];
    id.display_name=fields["devname.bambu.com"];
    id.host=fields["location"];
    if(id.model_code=="N2S") id.model_name="Bambu Lab A1";
    // Do not infer hardware identity solely from the human-readable name.
    id.valid_a1=(id.model_code=="N2S" && !id.device_id.empty() && !id.host.empty());
    return id;
}
} // namespace Slic3r::Filuna::Bambu
