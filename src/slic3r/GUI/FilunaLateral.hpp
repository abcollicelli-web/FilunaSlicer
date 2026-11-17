#pragma once

// Filuna — visual fixo do menu lateral (aprovado no canvas "Menu lateral Filuna Slicer", proposta 2).
//
// Igual para qualquer impressora e perfil: faixa "SELECIONAR IMPRESSORA", um cartão com a foto da impressora, o bico, o
// tipo de bico e a mesa, as bobinas dos filamentos 4 por linha (a clicada mostra embaixo Cor, Trocar, Ajustes e Tirar),
// botões com nome no lugar dos ícones pequenos, um resumo de duas linhas quando a parte de cima é recolhida, a faixa de
// arrastar que dá mais altura ao Processo e as abas do Processo em grade, todas à vista.
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

// Faixa magenta "SELECIONAR IMPRESSORA" com a seta.
class Faixa : public wxPanel
{
public:
    explicit Faixa(wxWindow* pai);
    std::function<void()> ao_clicar;

private:
    void pintar();
    bool m_sobre{false};
};

// Cartão lilás: foto da impressora, os seletores "Bico" e "Tipo de bico" e, embaixo, a linha rosa da mesa.
class CartaoImpressora : public wxPanel
{
public:
    explicit CartaoImpressora(wxWindow* pai);
    void definir_foto(const wxBitmap& foto);
    void definir_foto_mesa(const wxBitmap& foto);
    void definir(const wxString& bico, bool bico_selecionavel, const wxString& tipo_bico, const wxString& mesa, bool mesa_selecionavel);
    std::function<void(const wxRect& tela)> ao_bico, ao_tipo, ao_mesa; // área do seletor clicado, na tela

private:
    void   pintar();
    wxRect area(int i) const; // 1 = bico, 2 = tipo, 3 = mesa
    wxBitmap m_foto, m_foto_mesa;
    wxString m_bico, m_tipo, m_mesa;
    bool     m_bico_sel{true}, m_mesa_sel{true};
    int      m_sobre{0};
};

struct InfoBobina
{
    wxString numero;  // "1", "2"… (número da entrada)
    wxColour cor;
    wxString marca;   // "eSun", "Polymaker"… (vazio: "Genérico")
    wxString tipo;    // "PLA+", "PETG"…
    wxString estoque; // "750 g", "sem estoque", "—"
};

// Bobinas dos filamentos: 4 por linha, cartão escuro, e um cartão "Adicionar" no fim. A selecionada fica com a borda
// magenta; o clique seleciona.
class Bobinas : public wxPanel
{
public:
    explicit Bobinas(wxWindow* pai);
    // Só redesenha (e recalcula a altura) quando algo mudou.
    void definir(const std::vector<InfoBobina>& bobinas, int selecionada);
    int  altura_linha() const; // altura de uma linha de cartões, com o espaço entre linhas
    std::function<void(int indice)>                      ao_clicar;
    std::function<void()>                                ao_adicionar;
    std::function<void(int indice, const wxPoint& tela)> ao_menu; // botão direito: menu do filamento do Orca

private:
    void   pintar();
    void   recalcular_altura();
    wxRect cartao(int i) const;
    int    sob(const wxPoint& p) const; // -1: nada; size(): o cartão "Adicionar"
    std::vector<InfoBobina> m_bobinas;
    std::vector<wxBitmap>   m_desenhos;
    int                     m_sel{0}, m_sobre{-1};
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

// Faixa rosa embaixo das bobinas: qual entrada está selecionada e Cor, Trocar, Ajustes e Tirar.
class BarraBobina : public wxPanel
{
public:
    explicit BarraBobina(wxWindow* pai);
    void definir(const wxString& entrada, const wxString& nome, const wxColour& cor, bool pode_tirar);
    std::function<void(const wxRect& tela)> ao_cor;
    std::function<void()>                   ao_trocar, ao_ajustes, ao_tirar;

private:
    void   pintar();
    wxRect area(int i) const; // 0 cor, 1 trocar, 2 ajustes, 3 tirar
    std::vector<Botao> botoes() const;
    wxString m_entrada, m_nome;
    wxColour m_cor;
    bool     m_pode_tirar{true};
    int      m_sobre{-1};
};

// A parte de cima recolhida: uma linha com a foto, o bico e a mesa, e uma com as bolinhas dos filamentos.
class Resumo : public wxPanel
{
public:
    explicit Resumo(wxWindow* pai);
    void definir_foto(const wxBitmap& foto);
    void definir(const wxString& linha1, const wxString& linha2, const std::vector<InfoBobina>& bobinas);
    std::function<void()> ao_abrir, ao_adicionar;

private:
    void   pintar();
    wxRect area_abrir() const;
    wxRect area_mais() const;
    wxBitmap                m_foto;
    wxString                m_linha1, m_linha2;
    std::vector<InfoBobina> m_bobinas;
    int                     m_sobre{0}; // 1 = abrir, 2 = mais
};

// Faixa de arrastar acima do Processo: puxar para cima dá mais altura ao Processo.
class Divisor : public wxPanel
{
public:
    explicit Divisor(wxWindow* pai);
    std::function<void()>       ao_comecar;
    std::function<void(int dy)> ao_arrastar; // deslocamento desde o começo do arraste
    std::function<void()>       ao_soltar;

private:
    void pintar();
    int  m_inicio_y{0};
    bool m_arrastando{false};
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
    std::vector<std::pair<int, wxString>> m_itens; // índice na lista original e o nome
    int                              m_sel{-1}, m_sobre{-1};
    wxTimer                          m_timer;
};

}}} // namespace Slic3r::GUI::FilunaLateral
