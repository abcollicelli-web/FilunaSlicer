#pragma once

// Autoajuda (FILUNA/guia-v1/01-TELAS.md §6): com ela ligada, a dica de cada parâmetro vira um cartão em
// português simples — título, o que faz e o efeito — vindo de resources/filuna/autoajuda/pt_BR.json.
// Desligada, vale a dica original do programa. Não muda valor nem função de parâmetro nenhum.
// Preferência: app_config "filuna_pref_interface_autoajuda" ("true"/"false", padrão ligada).

#include <string>
#include <wx/string.h>

namespace Slic3r { namespace GUI { namespace FilunaAutoajuda {

bool     ligada();
void     ligar(bool sim);
// Cartão do parâmetro; vazio quando não há texto para ele (ou a Autoajuda está desligada).
wxString cartao(const std::string& opt_key);

}}} // namespace Slic3r::GUI::FilunaAutoajuda
