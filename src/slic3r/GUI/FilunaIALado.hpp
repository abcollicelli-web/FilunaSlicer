#pragma once

// IA ao lado: uma janela com o site da IA do cliente (Gemini, ChatGPT…), entrando com a conta dele, ao lado do Slicer.
// Telas do Filuna e paramétricos do servidor usam para mandar um pedido (anexar foto, escrever, enviar) e pegar a
// resposta (a imagem gerada) por roteiros que rodam na página da IA. A geração sai na conta do cliente, sem custo
// para a Filuna. Ações: ia.lado.abrir {url}, ia.lado.script {id, js} (resposta pelo aviso "ia.lado.script"
// {id, ok, valor}), ia.lado.fechar.

#include <optional>
#include <string>
#include <nlohmann/json.hpp>

namespace Slic3r { namespace GUI {

std::optional<nlohmann::json> filuna_ia_lado_acao(const std::string& acao, const nlohmann::json& dados);

}} // namespace Slic3r::GUI
