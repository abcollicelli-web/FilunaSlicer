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
    // aceita qualquer assinatura de retorno (sucesso, erro): nunca chama nenhum dos dois
    template<class... A> void get_qidi_model_list(A&&...) {}
};

}} // namespace Slic3r::GUI
