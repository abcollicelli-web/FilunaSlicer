// Filuna — substituto para a régua de paridade (ver README.md). Sem comportamento de rede.
#pragma once
#include <string>
#include <vector>

namespace Slic3r { namespace GUI {

struct Box_info
{
    int                      box_count          = 0;
    int                      auto_reload_detect = 0;
    std::vector<int>         slot_state;
    std::vector<int>         slot_id;
    std::vector<int>         filament_vendor;
    std::vector<int>         filament_index;
    std::vector<std::string> filament_id;
    std::vector<std::string> filament_colors;
    std::vector<std::string> filament_type;
};

}} // namespace Slic3r::GUI
