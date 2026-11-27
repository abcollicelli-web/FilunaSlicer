#ifndef slic3r_GUI_FilunaLoja_hpp_
#define slic3r_GUI_FilunaLoja_hpp_

#include <functional>
#include <string>

// Atualização do Filuna instalado pela Microsoft Store, pedida de dentro do programa (Windows.Services.Store).
// A Store sozinha só instala com o programa fechado e no horário dela; assim o cliente atualiza na hora, com um clique.
// Sem dependência do wx: as chamadas bloqueiam e devem rodar numa thread própria.
namespace Slic3r { namespace GUI { namespace FilunaLoja {

// Pergunta à Store, com a conta Microsoft logada no Windows, se há versão nova deste pacote.
// 1 = há, 0 = não há, -1 = erro (motivo em `erro`).
int procurar(std::string& erro);

// Baixa e instala a versão nova pela Store. `janela` é a janela principal (HWND), dona da confirmação da Store.
// `progresso` recebe 0..100 do download. Ao terminar, o Windows fecha o programa para trocar os arquivos; antes disso
// o programa se registra para abrir de novo sozinho. Devolve false com o motivo se não instalou.
bool instalar(void* janela, const std::function<void(int)>& progresso, std::string& erro);

}}} // namespace Slic3r::GUI::FilunaLoja

#endif
