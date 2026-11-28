#pragma once

// Arquivos abertos: vários projetos no mesmo Filuna, trocados por uma coluna na borda esquerda da vista 3D.
//
// O Plater do Orca guarda um projeto por vez. Os outros ficam guardados num 3MF temporário em
// data_dir/filuna/abertos/ (modelo, mesas, configurações e as mudanças ainda não salvas no original); trocar
// guarda o atual e carrega o escolhido. O arquivo original só muda quando o cliente salva.
//
// O arquivo que o Windows manda abrir (dois cliques, "Abrir com", arrastar para o ícone) chega pela instância
// única e entra como arquivo novo na lista, sem substituir o projeto aberto.

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <wx/arrstr.h>

namespace Slic3r {
struct ThumbnailData;
namespace GUI {
class IMToolbarItem;
namespace FilunaAbertos {

// Quantos arquivos podem ficar abertos ao mesmo tempo.
constexpr size_t LIMITE = 8;

struct Item
{
    unsigned                             id{ 0 };
    std::string                          nome;     // nome do projeto (UTF-8)
    std::string                          caminho;  // projeto no disco; vazio = ainda sem arquivo salvo
    bool                                 ativo{ false };
    bool                                 modificado{ false };
    std::shared_ptr<const ThumbnailData> miniatura;  // pode faltar
    unsigned                             versao_miniatura{ 0 };
};

std::vector<Item> itens();

void ativar(unsigned id);
// Pergunta "Salvar / Descartar / Cancelar" quando há mudança não salva. Fechar o último deixa um projeto vazio.
void fechar(unsigned id);
// O "+" da coluna: escolhe arquivos e abre cada projeto como arquivo novo.
void abrir_outro();
// Arquivos vindos de outra instância (o Windows abrindo um arquivo com o Filuna já aberto). Espera o programa
// ficar livre (janela modal, carregamento, ferramenta em uso) e abre cada projeto como arquivo novo.
void abrir_de_fora(const wxArrayString& arquivos);

// Fechando o programa: pergunta por cada arquivo aberto (fora o atual) com mudança não salva. false = cancelar.
// O atual continua com a pergunta de sempre do Orca.
bool confirmar_saida();
// Programa fechando de vez: apaga os temporários desta execução.
void encerrar();

// Estado de desenho de uma vista 3D (texturas das miniaturas).
struct Coluna
{
    std::map<unsigned, std::pair<unsigned, std::unique_ptr<IMToolbarItem>>> texturas;
    Coluna();
    ~Coluna();
};

// Largura que a coluna ocupa na borda esquerda (com o respiro até o que vem ao lado), na escala da vista.
float largura(float escala);
// Desenha a coluna dentro do quadro ImGui da vista. x/topo/base em pixels da vista.
void desenhar(Coluna& coluna, float x, float topo, float base, float escala, bool escuro);

} // namespace FilunaAbertos
} // namespace GUI
} // namespace Slic3r
