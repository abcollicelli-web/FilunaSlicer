#pragma once

// Filuna — visual fixo da parte de cima do menu lateral (aprovado no canvas "Menu lateral Filuna Slicer").
//
// Igual para qualquer impressora e perfil: faixa "SELECIONAR IMPRESSORA", cartão da impressora (foto, bico e tipo de
// bico para selecionar), cartão da mesa (imagem e tipo para selecionar) e as bobinas dos filamentos, 4 por linha, cada
// uma desenhada na cor do filamento com o número da entrada no canto e, embaixo, marca, tipo e estoque.
//
// É só visual: as listas originais do Orca (impressora, variante, mesa, filamentos) continuam existindo e fazendo o
// trabalho; cada clique aqui abre a lista original correspondente. Nenhum perfil ou módulo muda por causa deste painel.

#include <wx/bitmap.h>
#include <wx/panel.h>

#include <functional>
#include <vector>

namespace Slic3r { namespace GUI { namespace FilunaLateral {

// Bobina de filamento deitada, vista de frente e de lado (face com o furo à esquerda, corpo indo para trás), com luz
// de cima à esquerda: o desenho aprovado. `largura` em pixels físicos; altura = 0,84 da largura.
wxBitmap bobina(const wxColour& cor, int largura);

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

// Cartão lilás: foto da impressora e os seletores "Bico" e "Tipo de bico".
class CartaoImpressora : public wxPanel
{
public:
    explicit CartaoImpressora(wxWindow* pai);
    void definir_foto(const wxBitmap& foto);
    void definir(const wxString& bico, bool bico_selecionavel, const wxString& tipo_bico);
    std::function<void(const wxRect& tela)> ao_bico, ao_tipo; // área do seletor clicado, na tela

private:
    void   pintar();
    wxRect area_bico() const;
    wxRect area_tipo() const;
    wxBitmap m_foto;
    wxString m_bico, m_tipo;
    bool     m_bico_sel{true};
    int      m_sobre{0}; // 1 = bico, 2 = tipo
};

// Cartão rosa: imagem da mesa e o seletor do tipo de mesa.
class CartaoMesa : public wxPanel
{
public:
    explicit CartaoMesa(wxWindow* pai);
    void definir_foto(const wxBitmap& foto);
    void definir(const wxString& mesa, bool selecionavel);
    std::function<void(const wxRect& tela)> ao_clicar;

private:
    void     pintar();
    wxRect   area_seletor() const;
    wxBitmap m_foto;
    wxString m_mesa;
    bool     m_selecionavel{true}, m_sobre{false};
};

struct InfoBobina
{
    wxString numero;  // "1", "2"… (número da entrada)
    wxColour cor;
    wxString marca;   // "eSun", "Polymaker"… (vazio: "Genérico")
    wxString tipo;    // "PLA+", "PETG"…
    wxString estoque; // "750 g", "sem estoque", "—"
};

// Bobinas dos filamentos: 4 por linha, cartão escuro; o cartão sob o mouse fica com a borda magenta.
class Bobinas : public wxPanel
{
public:
    explicit Bobinas(wxWindow* pai);
    // Só redesenha (e recalcula a altura) quando algo mudou.
    void definir(const std::vector<InfoBobina>& bobinas);
    std::function<void(int indice, const wxRect& tela)>  ao_clicar; // trocar o filamento (lista original)
    std::function<void(int indice, const wxPoint& tela)> ao_menu;   // botão direito: menu do filamento (cor, editar…)

private:
    void   pintar();
    void   recalcular_altura();
    wxRect cartao(int i) const;
    int    sob(const wxPoint& p) const;
    std::vector<InfoBobina> m_bobinas;
    std::vector<wxBitmap>   m_desenhos;
    int                     m_sobre{-1};
};

}}} // namespace Slic3r::GUI::FilunaLateral
