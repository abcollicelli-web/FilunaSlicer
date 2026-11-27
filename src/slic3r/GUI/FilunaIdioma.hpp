#ifndef slic3r_GUI_FilunaIdioma_hpp_
#define slic3r_GUI_FilunaIdioma_hpp_

// Textos da Filuna em inglês (versão americana). O texto original é o português; quando o idioma do programa
// (app_config "language") começa com "en", Filuna::tr() devolve a tradução de resources/web/filuna/i18n/en.json,
// o mesmo dicionário das telas web. Sem tradução, devolve o português. Os textos do Orca continuam no gettext (_L).

#include <initializer_list>
#include <string>

#include <wx/string.h>

namespace Slic3r { namespace Filuna {

// Idioma do programa começa com "en". Pode ser chamado de qualquer linha; a da tela atualiza o valor.
bool ingles();

// Frase inteira em português -> inglês (quando ingles()). Texto com várias linhas é traduzido linha a linha.
std::string tr(const std::string& pt);
wxString    trw(const std::string& pt);
// Frase com valores: tr_valores("A impressora {0} parou na camada {1}", {nome, camada}). Os valores não são traduzidos.
std::string tr_valores(const std::string& modelo_pt, std::initializer_list<std::string> valores);
wxString    trw_valores(const std::string& modelo_pt, std::initializer_list<std::string> valores);

// en.json inteiro (texto JSON), para a ponte das telas ("app.dicionario").
const std::string& dicionario_json();
// Roteiro injetado no começo de cada tela Filuna: diz o idioma e, em inglês, já traz o dicionário e o tradutor.
std::string roteiro_telas();

// Primeira abertura do programa: "en" se o Windows (idioma da interface ou, sem ele, a região) for inglês; senão "pt_BR".
std::string idioma_primeira_abertura();
// A conta informou região e moeda (campo "regiao"/"moeda" de /conta ou /resumo). Guarda no app_config e, se o idioma
// foi escolhido sozinho na primeira abertura e não bate com a região (us -> inglês, br -> português), oferece trocar.
void regiao_da_conta(const std::string& regiao, const std::string& moeda);
// O idioma atual foi escolhido sozinho (primeira abertura) e a região da conta ainda pode decidir.
bool idioma_automatico();

}} // namespace Slic3r::Filuna

#endif
