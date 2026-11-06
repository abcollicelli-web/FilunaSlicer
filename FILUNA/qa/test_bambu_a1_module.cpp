#include "../fabricantes/bambu/engine/BambuA1Module.hpp"
#include "../../src/slic3r/GUI/DeviceCore/FilunaOEMOverlay.hpp"
#include "../../src/slic3r/GUI/DeviceCore/FilunaVendorRegistry.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using namespace Slic3r::Filuna;
    assert(model_for_name("N2S") && model_for_name("N2S")->model_id=="BBL-A1");
    for (auto model: {"N2S","Bambu Lab A1","BBL-A1"}) {
        const auto* device=resolve_oem_for_stage(OEMStage::Device,model,1);
        const auto* prepare=resolve_oem_for_stage(OEMStage::Prepare,model,1);
        assert(device && prepare && device==prepare);
        assert(device->module_id=="filuna-bambu-a1");
        assert(Bambu::A1Module::activate_for_prepare(model)==Bambu::ActivationState::MathCandidateOnly);
    }
    assert(Bambu::A1Module::activate_for_prepare("X1C")==Bambu::ActivationState::WrongDevice);
    assert(Bambu::A1Module::bbs_mapping_speed(2.0,80.0)==320.0);
    assert(resolve_oem_overlay("N2S",2)==nullptr);
    assert(!may_enable_hardware_operations(*select_candidate("BBL-A1",1),true,true,true));
    std::cout<<"BAMBU A1 IN_PROCESS_MODULE_ENTRYPOINT PASS (not slicing equivalence or firmware)\n";
}
