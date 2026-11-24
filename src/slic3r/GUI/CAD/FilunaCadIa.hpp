#pragma once

// Filuna CAD + a IA do próprio cliente (Claude ou ChatGPT, pela assinatura dele, sem chave nem crédito): ações
// "cad.*" (estado do Design e execução dos passos que a IA pediu pelo conector Filuna) e "cadia.*" (o painel
// "Modelar com IA" e o navegador da IA dentro do Design). Devolve nada quando a ação não é daqui.

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace Slic3r { namespace GUI {

std::optional<nlohmann::json> filuna_cad_acao(const std::string& acao, const nlohmann::json& dados);

}} // namespace Slic3r::GUI
