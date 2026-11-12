// Filuna — substituto para a régua de paridade (ver README.md). Toda chamada responde "sem conexão".
#pragma once
#include <string>
#include <wx/string.h>
#include "QIDINetworkTypes.hpp"

namespace Slic3r { namespace GUI {

class QIDIDeviceApi
{
public:
    static bool get_file_list(const std::string&, std::string&) { return false; }
    static bool get_timelapse_directory(const std::string&, std::string&) { return false; }
    static bool get_system_info(const std::string&, std::string&) { return false; }
    static bool get_client_info(const std::string&, std::string&) { return false; }
    static bool fetch_url_to_buffer(const std::string&, std::string&, int) { return false; }

    bool     get_box_state(wxString&, const std::string&, const std::string&) { return false; }
    Box_info get_box_info(wxString&, const std::string&, const std::string&) { return {}; }
    bool     get_color_filament_str(wxString&, Box_info&, const std::string&, const std::string&) { return false; }
};

}} // namespace Slic3r::GUI
