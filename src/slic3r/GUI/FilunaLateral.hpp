#pragma once

// Filuna — visual fixo do menu lateral (aprovado no canvas "Menu lateral Filuna Slicer", proposta 3).
//
// Igual para qualquer impressora e perfil: um cartão com a faixa "SELECIONAR IMPRESSORA" (com a foto) e, embaixo, bico,
// tipo de bico e mesa numa linha; as bobinas dos filamentos numa fileira que rola de lado (a clicada mostra embaixo Cor,
// Trocar, Ajustes e Tirar); botões com nome no lugar dos ícones pequenos; o modo foco, em que impressora e filamentos
// viram uma faixa de uma linha e o Processo fica com quase a altura toda; Simples/Avançado/Especialista; e as abas do
// Processo em grade, todas à vista.
//
// Os componentes só desenham e avisam o clique: quem faz o trabalho continua sendo o Orca (listas, abas e botões
// originais, escondidos ou não), chamado por quem monta o menu (Plater, ParamsPanel, Tab).

#include <wx/bitmap.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/panel.h>
#include <wx/timer.h>

#include <functional>
#include <memory>
#include <vector>

class TabCtrl;

namespace Slic3r { namespace GUI { namespace FilunaLateral {

// Bobina de filamento deitada, vista de frente e de lado (face com o furo à esquerda, corpo indo para trás), com luz
// de cima à esquerda: o desenho aprovado. `largura` em pixels físicos; altura = 0,84 da largura.
wxBitmap bobina(const wxColour& cor, int largura);
// Frasco de resina em pé (tampa, vidro e a resina dentro), do mesmo tamanho da bobina, para a fileira da resina.
wxBitmap frasco(const wxColour& cor, int largura);

enum class Icone { nenhum, mais, sincronizar, misto, lapis, lixeira, atomo, chave, tabela, comparar, seta_cima, seta_baixo };

struct Botao
{
    wxString texto;                // vazio: só o ícone (botão quadrado)
    Icone    icone{Icone::nenhum};
    wxColour cor;                  // válida: uma bolinha dessa cor no lugar do ícone (botão de cor)
    bool     ativo{true};
    bool     ligado{false};        // ícone `chave`: ligada ou desligada
    bool     destaque{false};      // fundo magenta
    bool operator==(const Botao& o) const
    {
        return texto == o.texto && icone == o.icone && cor == o.cor && ativo == o.ativo && ligado == o.ligado && destaque == o.destaque;
    }
};

// Cores e desenho comuns a todo o visual Filuna (menu lateral e janelas de filamento).
namespace pintura {
extern const wxColour MAGENTA, MAGENTA_ESCURO, MAGENTA_CLARO, VINHO, LILAS, LILAS_BORDA, ROSA, ROSA_BORDA, ROSA_CLARO, AREIA, AREIA_BORDA,
    AREIA_ESCURA, TINTA, CINZA, BRANCO, CARTAO, CARTAO_BORDA, CARTAO_SOBRE, TEXTO_CLARO, TEXTO_APAGADO;
wxFont   fonte(wxWindow* w, double pontos, bool negrito);
// Limpa com o fundo de quem contém o painel e devolve o contexto de desenho (nulo se não deu).
std::unique_ptr<wxGraphicsContext> comecar(wxWindow* w, wxAutoBufferedPaintDC& dc);
wxString caber(wxWindow* w, const wxString& t, const wxFont& f, int largura);
void     texto_centro(wxGraphicsContext& gc, wxWindow* w, const wxString& t, const wxFont& f, const wxColour& cor, double cx, double y, int largura);
void     texto_meio(wxGraphicsContext& gc, wxWindow* w, const wxString& t, const wxFont& f, const wxColour& cor, double x, const wxRect& r, int largura);
void     desenhar_botao(wxGraphicsContext& gc, wxWindow* w, const wxRect& r, const Botao& b, bool sobre, bool coluna);
void     desenhar_icone(wxGraphicsContext& gc, wxWindow* w, Icone ic, const wxRect2DDouble& r, const wxColour& cor, bool ligado);
void     cursor_mao(wxWindow* w, bool mao);
} // namespace pintura

// Cartão lilás: faixa magenta com a foto e o nome da impressora escolhida ("SELECIONAR IMPRESSORA" só sem nenhuma), o lápis
// de editar o perfil da impressora e, embaixo, os seletores de bico, tipo de bico e mesa. Impressora de 2 bicos: sem o
// seletor de bico (o Orca mostra os dois bicos logo abaixo, com o tipo de fluxo e o AMS de cada um).
class CartaoImpressora : public wxPanel
{
public:
    enum Area { NADA = 0, FAIXA, BICO, TIPO, MESA, LAPIS };
    struct Info
    {
        wxString impressora; // nome da impressora escolhida na lista (vazio: a faixa pede para selecionar)
        wxString bico, tipo, mesa;
        wxString rotulo_bico, rotulo_tipo, rotulo_mesa; // vazio: Bico, Tipo de bico, Mesa (resina: Tela, Arquivo, Envio)
        bool     mostrar_bico{true}, bico_selecionavel{true}, tipo_selecionavel{true}, mesa_selecionavel{true};
        bool     operator==(const Info& o) const
        {
            return impressora == o.impressora && bico == o.bico && tipo == o.tipo && mesa == o.mesa && rotulo_bico == o.rotulo_bico && rotulo_tipo == o.rotulo_tipo &&
                   rotulo_mesa == o.rotulo_mesa && mostrar_bico == o.mostrar_bico && bico_selecionavel == o.bico_selecionavel &&
                   tipo_selecionavel == o.tipo_selecionavel && mesa_selecionavel == o.mesa_selecionavel;
        }
    };
    explicit CartaoImpressora(wxWindow* pai);
    void definir_foto(const wxBitmap& foto);
    void definir_foto_mesa(const wxBitmap& foto);
    void definir(const Info& info);
    std::function<void(int area, const wxRect& tela)>   ao_area;       // clique numa área (FAIXA, BICO, TIPO, MESA, LAPIS)
    std::function<void(bool sobre, const wxRect& tela)> ao_mesa_sobre; // o mouse entrou ou saiu da mesa

private:
    void     pintar();
    wxRect   area(int i) const;
    wxBitmap m_foto, m_foto_mesa;
    Info     m_info;
    bool     m_mostrar_bico{true}, m_bico_sel{true}, m_mesa_sel{true};
    int      m_sobre{NADA};
};

struct InfoBobina
{
    wxString numero;  // "1", "2"… (número da entrada)
    wxColour cor;
    wxString marca;   // "eSun", "Polymaker"… (vazio: "Genérico")
    wxString tipo;    // "PLA+", "PETG"…
    wxString estoque; // "750 g", "sem estoque", "—"
    bool     frasco{false}; // resina: desenha o frasco no lugar da bobina
};

// Bobinas dos filamentos numa fileira que rola de lado (roda do mouse ou as setas das pontas), com um "+" no fim. A
// selecionada fica com a borda magenta; o clique seleciona.
class Bobinas : public wxPanel
{
public:
    enum { ALTURA = 92, LARGURA = 60, SETA_ESQ = -2, SETA_DIR = -3 };
    explicit Bobinas(wxWindow* pai);
    // Só redesenha quando algo mudou; a selecionada nova é trazida para a vista.
    void definir(const std::vector<InfoBobina>& bobinas, int selecionada);
    void mostrar_mais(bool sim); // o "+" do fim da fileira (a resina não tem)
    std::function<void(int indice)>                      ao_clicar;
    std::function<void()>                                ao_adicionar;
    std::function<void(int indice, const wxPoint& tela)> ao_menu; // botão direito: menu do filamento do Orca

private:
    void   pintar();
    void   rolar(int dx);
    int    passo() const;
    int    largura_total() const;
    wxRect cartao(int i) const;
    int    sob(const wxPoint& p) const; // -1: nada; size(): o "+"; SETA_ESQ / SETA_DIR
    std::vector<InfoBobina> m_bobinas;
    std::vector<wxBitmap>   m_desenhos;
    int                     m_sel{0}, m_sobre{-1}, m_desloc{0};
    bool                    m_mais{true};
};

// Fileira de botões com ícone e nome. `coluna`: ícone em cima do nome, todos da mesma largura (ferramentas do Processo);
// senão ícone ao lado do nome, dividindo a largura igualmente.
class Botoes : public wxPanel
{
public:
    Botoes(wxWindow* pai, bool coluna);
    void definir(const std::vector<Botao>& botoes);
    std::function<void(int indice, const wxRect& tela)> ao_clicar;

private:
    void   pintar();
    wxRect area(int i) const;
    int    sob(const wxPoint& p) const;
    std::vector<Botao> m_botoes;
    bool               m_coluna;
    int                m_sobre{-1};
};

// Linha rosa fina embaixo das bobinas: qual entrada está selecionada e Cor, Trocar, Ajustes e Tirar.
class BarraBobina : public wxPanel
{
public:
    explicit BarraBobina(wxWindow* pai);
    void definir(const wxString& entrada, const wxString& nome, const wxColour& cor, bool pode_tirar, bool pode_cor = true);
    std::function<void(const wxRect& tela)> ao_cor;
    std::function<void()>                   ao_trocar, ao_ajustes, ao_tirar;

private:
    void   pintar();
    wxRect area(int i) const; // 0 cor, 1 trocar, 2 ajustes, 3 tirar
    std::vector<Botao> botoes() const;
    wxString m_entrada, m_nome;
    wxColour m_cor;
    bool     m_pode_tirar{true}, m_pode_cor{true};
    int      m_sobre{-1};
};

// Modo foco: a parte de cima recolhida numa faixa de uma linha (foto, bico, mesa, as bolinhas dos filamentos e Abrir).
class Resumo : public wxPanel
{
public:
    explicit Resumo(wxWindow* pai);
    void definir_foto(const wxBitmap& foto);
    void definir(const wxString& linha, const std::vector<InfoBobina>& bobinas, int selecionada);
    std::function<void()> ao_abrir;

private:
    void                    pintar();
    wxBitmap                m_foto;
    wxString                m_linha;
    std::vector<InfoBobina> m_bobinas;
    int                     m_sel{0};
    bool                    m_sobre{false};
};

// Escolha de uma opção entre poucas, em pílula (Simples / Avançado / Especialista).
class Segmentos : public wxPanel
{
public:
    Segmentos(wxWindow* pai, const std::vector<wxString>& opcoes);
    void                       selecionar(int i, bool ativo);
    std::function<void(int i)> ao_escolher;

private:
    void                  pintar();
    wxRect                area(int i) const;
    int                   sob(const wxPoint& p) const;
    std::vector<wxString> m_opcoes;
    int                   m_sel{-1}, m_sobre{-1};
    bool                  m_ativo{true};
};

// As abas de uma aba de ajustes do Orca (Qualidade, Resistência…) em grade de 3 colunas, todas à vista. A lista
// original continua criada (escondida) e é ela que troca a página: o clique aqui só a seleciona.
class GradeAbas : public wxPanel
{
public:
    GradeAbas(wxWindow* pai, TabCtrl* abas);

private:
    void   pintar();
    void   conferir();
    wxRect area(int i) const;
    int    sob(const wxPoint& p) const;
    TabCtrl*                         m_abas;
    struct Item
    {
        int      indice;
        wxString nome;
        wxColour cor; // a cor do nome na lista original: diferente do cinza = página com ajuste mudado
        bool     operator==(const Item& o) const { return indice == o.indice && nome == o.nome && cor == o.cor; }
    };
    std::vector<Item>                m_itens;
    int                              m_sel{-1}, m_sobre{-1};
    wxTimer                          m_timer;
};

}}} // namespace Slic3r::GUI::FilunaLateral
