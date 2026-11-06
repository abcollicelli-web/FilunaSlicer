#include "../fabricantes/bambu/transport/BambuLanDiscovery.hpp"
#include <cassert>
#include <iostream>
#include <string>
int main() {
    using namespace Slic3r::Filuna::Bambu;
    int matched=0;
    for (const auto& m : kManufacturerModels) {
        const std::string serial=std::string(m.serial_prefix)+"TEST000";
        const std::string packet=
            "HTTP/1.1 200 OK\r\n"
            "ST: urn:bambulab-com:device:3dprinter:1\r\n"
            "USN: "+serial+"\r\n"+
            "DevModel.bambu.com: "+std::string(m.model_id)+"\r\n"
            "DevName.bambu.com: Simulated Printer\r\n"
            "Location: 10.77.0.12\r\n";
        const auto id=parse_ssdp(packet);
        assert(id.valid_manufacturer_model);
        assert(id.model_code==m.model_id);
        assert(id.model_name==m.display_name);
        assert(id.valid_a1==(m.model_id=="N2S"));
        const std::string wrong_serial="ZZZTEST000";
        assert(!is_valid_bambu_discovery_identity(m.model_id,wrong_serial));
        ++matched;
    }
    assert(!lookup_bambu_model("UNKNOWN"));
    assert(!parse_ssdp("HTTP/1.1 200 OK\r\nST: urn:bambulab-com:device:3dprinter:1\r\nUSN: ZZ999\r\nDevModel.bambu.com: N2S\r\nLocation: 10.77.0.12\r\n").valid_manufacturer_model);
    std::cout << "Bambu OEM discovery: " << matched
              << " official model IDs + negative identity cases PASS; authentication NOT TESTED\n";
    return 0;
}
