// Filuna — substituto para a régua de paridade (ver README.md). Sem nuvem QIDI Maker.
#pragma once
#include <functional>
#include <string>
#include "QIDINetworkTypes.hpp"

namespace Slic3r { namespace GUI {

class MakerHttpHandle
{
public:
    static MakerHttpHandle& getInstance() { static MakerHttpHandle h; return h; }
    void get_qidi_model_list(std::function<void(std::string, unsigned)>) {}
};

}} // namespace Slic3r::GUI
