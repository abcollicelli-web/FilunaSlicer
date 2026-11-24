#pragma once

// Impressão em curva (teste do dono, só na versão portátil que traz o motor em "curva/").
// Depois de fatiar, a Luna fatia a mesa de novo em plano e com o topo curvo no motor do PancakeSlicer,
// confere a colisão do bico com o modelo da cabeça da impressora e mede o erro do topo.
// Só quando é seguro e o topo melhora de verdade aparece o aviso com a demonstração (o G-code curvo
// abre no visualizador; nada vai para a impressora).

#include <optional>
#include <string>
#include <nlohmann/json.hpp>

namespace Slic3r { namespace GUI { namespace FilunaCurva {

void depois_de_fatiar(bool ok);

// Ação da tela curva.html: "curva.dados" devolve o endereço do arquivo com os trechos dos dois G-codes.
std::optional<nlohmann::json> acao(const std::string& acao, const nlohmann::json& dados);

}}} // namespace Slic3r::GUI::FilunaCurva
