// Filuna OEM Bambu manufacturer network model registry.
// Source: official BambuStudio resources/printers/*.json, snapshot
// 2026-11-06. Model-ID and SN prefixes below are identification hints,
// not authentication or evidence that a printer's firmware is homologated.
// Do not route a different model to A1-specific slicing functionality.
#pragma once
#include <array>
#include <string_view>

namespace Slic3r::Filuna::Bambu {

struct ManufacturerModel {
    std::string_view model_id;
    std::string_view display_name;
    std::string_view serial_prefix;
};

// Official Bambu Studio v02.09.01.70 model-matrix snapshot.
// O1C/O1C2 intentionally share a serial prefix: both remain distinct models.
inline constexpr std::array<ManufacturerModel, 17> kManufacturerModels = {{
    {"BL-P001", "Bambu Lab X1 Carbon", "00M"},
    {"BL-P002", "Bambu Lab X1", "00W"},
    {"C11", "Bambu Lab P1P", "01S"},
    {"C12", "Bambu Lab P1S", "01P"},
    {"C13", "Bambu Lab X1E", "03W"},
    {"N1", "Bambu Lab A1 mini", "030"},
    {"N2S", "Bambu Lab A1", "039"},
    {"N6", "Bambu Lab X2D", "20P"},
    {"N7", "Bambu Lab P2S", "22E"},
    {"N8", "Bambu Lab N8", "35F"},
    {"N9", "Bambu Lab A2L", "26A"},
    {"N10", "Bambu Lab H3C", "32X"},
    {"O1C", "Bambu Lab H2C", "31B"},
    {"O1C2", "Bambu Lab H2C", "31B"},
    {"O1D", "Bambu Lab H2D", "094"},
    {"O1E", "Bambu Lab H2D Pro", "239"},
    {"O1S", "Bambu Lab H2S", "093"}
}};

inline const ManufacturerModel* lookup_bambu_model(std::string_view model_id) {
    for (const auto& model : kManufacturerModels)
        if (model.model_id == model_id)
            return &model;
    return nullptr;
}

inline bool is_valid_bambu_discovery_identity(
    std::string_view model_id, std::string_view serial) {
    const auto* model = lookup_bambu_model(model_id);
    return model != nullptr && serial.size() >= model->serial_prefix.size()
        && serial.substr(0, model->serial_prefix.size()) == model->serial_prefix;
}

} // namespace Slic3r::Filuna::Bambu
