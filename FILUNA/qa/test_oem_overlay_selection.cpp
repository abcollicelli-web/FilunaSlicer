#include "../../src/slic3r/GUI/DeviceCore/FilunaOEMOverlay.hpp"
#include <cassert>
#include <string_view>
#include <iostream>
int main(){
    using namespace Slic3r::Filuna;
    for(const auto& b:kOEMBindings){
        const auto* device=resolve_oem_for_stage(OEMStage::Device,b.model_id,1);
        const auto* prepare=resolve_oem_for_stage(OEMStage::Prepare,b.model_id,1);
        assert(device && prepare);
        assert(device == prepare);
        assert(device->module_id==b.module_id);
        assert(device->manifest_relative_path.find("FILUNA/fabricantes/")==0);
        assert(!may_enable_hardware_operations(*select_candidate(b.model_id,1),true,true,true));
    }
    assert(!resolve_oem_overlay("UNKNOWN",1));
    assert(!resolve_oem_overlay("BBL-A1",2));
    std::cout << "OEM overlay selection: 4 device/prepare pairs PASS; hardware locked\n";
}
