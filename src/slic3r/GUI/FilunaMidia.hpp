#pragma once

// Filuna — Post nas redes sociais (telas aprovadas pelo dono em 22/11/2026; desligado até ele configurar a Meta).
//
// O cliente liga "Post nas redes sociais" na tela de envio. Quando a impressora está imprimindo sem erro, abre a janela
// do post (midia.html) com o ChatGPT do próprio cliente ao lado: a tela manda a foto da peça, a marca e um cenário novo,
// recolhe a foto gerada e a legenda, e publica nas redes com o toque do cliente (servidor: site-v2/app/midia.py).
// A IA é a assinatura do cliente, logada aqui dentro: nada de chave de API nem crédito.
//
// A liberação vem do servidor (/api/slicer/v1/midia/estado): as telas guardam o resultado em "filuna_midia_disponivel"
// e, sem ele, nada desta ferramenta aparece.

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

class wxSizer;
class wxWindow;

namespace Slic3r { namespace GUI {

// Caixinha "Post nas redes sociais" nas telas de envio (só com a ferramenta liberada para a conta).
void filuna_midia_caixa_no_envio(wxWindow* pai, wxSizer* sizer);

// Envio confirmado: com a caixinha ligada, acompanha a impressora (dev_id; vazio = a selecionada agora) e abre o post
// quando ela estiver imprimindo sem erro.
void filuna_midia_envio_confirmado(const std::string& dev_id = {});

// Ações "midia.*" da ponte das telas Filuna. Devolve nada quando a ação não é desta ferramenta.
std::optional<nlohmann::json> filuna_midia_acao(const std::string& acao, const nlohmann::json& dados);

}} // namespace Slic3r::GUI
