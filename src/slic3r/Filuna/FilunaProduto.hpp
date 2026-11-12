#pragma once

#include <string>

namespace Slic3r {
class Model;
}

namespace Slic3r::Filuna {

// Assinatura do produto da mesa (guia-v1/02-DADOS §3): mesma geometria e escala → mesmo produto,
// qualquer que seja o nome do arquivo. Mesa vazia → texto vazio.
std::string assinatura_do_modelo(const Model& modelo);

} // namespace Slic3r::Filuna
