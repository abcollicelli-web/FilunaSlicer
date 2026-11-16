#include "FilunaLateral.hpp"

#include <wx/control.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/image.h>

#include <algorithm>
#include <memory>

namespace Slic3r { namespace GUI { namespace FilunaLateral {

namespace {

const wxColour MAGENTA(0xE0, 0x18, 0x6B), MAGENTA_ESCURO(0xC9, 0x14, 0x5E), MAGENTA_CLARO(0xFF, 0x6F, 0xA8);
const wxColour LILAS(0xF3, 0xE8, 0xFB), LILAS_BORDA(0xD9, 0xC8, 0xEC), ROSA(0xFB, 0xE1, 0xEC), ROSA_BORDA(0xF2, 0xC2, 0xD6);
const wxColour TINTA(0x1D, 0x1A, 0x17), CINZA(0x6E, 0x62, 0x59), BRANCO(0xFF, 0xFF, 0xFF);
const wxColour CARTAO(0x2A, 0x29, 0x2E), CARTAO_BORDA(0x3A, 0x39, 0x3F), CARTAO_SOBRE(0x3A, 0x26, 0x30);
const wxColour TEXTO_CLARO(0xF4, 0xF2, 0xF5), TEXTO_APAGADO(0xA7, 0xA2, 0xAC);

// Clareia (f > 0) ou escurece (f < 0) a cor.
wxColour tom(const wxColour& c, double f)
{
    auto canal = [f](int v) { return f >= 0 ? int(v + (255 - v) * f) : int(v * (1. + f)); };
    return wxColour(canal(c.Red()), canal(c.Green()), canal(c.Blue()));
}

wxFont fonte(wxWindow* w, double pontos, bool negrito)
{
    wxFont f = w->GetFont();
    f.SetFractionalPointSize(pontos);
    f.SetWeight(negrito ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
    return f;
}

// Fundo do painel = fundo de quem o contém (o menu lateral muda com o tema claro/escuro).
std::unique_ptr<wxGraphicsContext> comecar(wxWindow* w, wxAutoBufferedPaintDC& dc)
{
    dc.SetBackground(wxBrush(w->GetParent()->GetBackgroundColour()));
    dc.Clear();
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (gc)
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
    return gc;
}

void seta(wxGraphicsContext& gc, double cx, double cy, double t, const wxColour& cor, double esp)
{
    gc.SetPen(wxPen(cor, int(std::max(1., esp))));
    wxGraphicsPath p = gc.CreatePath();
    p.MoveToPoint(cx - t, cy - t * 0.5);
    p.AddLineToPoint(cx, cy + t * 0.5);
    p.AddLineToPoint(cx + t, cy - t * 0.5);
    gc.StrokePath(p);
}

// Texto que cabe na largura (corta com "…").
wxString caber(wxWindow* w, const wxString& t, const wxFont& f, int largura)
{
    wxClientDC dc(w);
    dc.SetFont(f);
    return wxControl::Ellipsize(t, dc, wxELLIPSIZE_END, std::max(10, largura));
}

void texto_centro(wxGraphicsContext& gc, wxWindow* w, const wxString& t, const wxFont& f, const wxColour& cor, double cx, double y, int largura)
{
    const wxString s = caber(w, t, f, largura);
    gc.SetFont(f, cor);
    double tw = 0, th = 0;
    gc.GetTextExtent(s, &tw, &th);
    gc.DrawText(s, cx - tw / 2., y);
}

// Seletor: rótulo em cima e caixa branca com o valor e a seta.
void seletor(wxGraphicsContext& gc, wxWindow* w, const wxRect& caixa, const wxString& rotulo, const wxString& valor, bool ativo, bool sobre,
             const wxColour& borda, double pt_valor)
{
    const int pad = w->FromDIP(8);
    gc.SetFont(fonte(w, 7.5, true), CINZA);
    gc.DrawText(rotulo.Upper(), caixa.x + 1, caixa.y - w->FromDIP(15));
    gc.SetPen(wxPen(sobre && ativo ? MAGENTA : borda, w->FromDIP(1)));
    gc.SetBrush(wxBrush(BRANCO));
    gc.DrawRoundedRectangle(caixa.x, caixa.y, caixa.width, caixa.height, w->FromDIP(7));
    const wxFont fv  = fonte(w, pt_valor, true);
    const int    seta_l = ativo ? w->FromDIP(14) : 0;
    const wxString s = caber(w, valor, fv, caixa.width - 2 * pad - seta_l);
    gc.SetFont(fv, ativo ? TINTA : CINZA);
    double tw = 0, th = 0;
    gc.GetTextExtent(s, &tw, &th);
    gc.DrawText(s, caixa.x + pad, caixa.y + (caixa.height - th) / 2.);
    if (ativo)
        seta(gc, caixa.GetRight() - pad - w->FromDIP(3), caixa.y + caixa.height / 2., w->FromDIP(4), CINZA, w->FromDIP(2));
}

void cursor_mao(wxWindow* w, bool mao) { w->SetCursor(mao ? wxCursor(wxCURSOR_HAND) : wxNullCursor); }

} // namespace

// ------------------------------------------------------------------------------------------------ bobina
wxBitmap bobina(const wxColour& cor, int largura)
{
    const int W = std::max(16, largura), H = int(W * 0.84);
    wxImage   img(W, H);
    img.InitAlpha();
    std::fill(img.GetAlpha(), img.GetAlpha() + W * H, 0);
    {
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(img));
        if (!gc)
            return wxBitmap(img);
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        const double rx = W * 0.27, ry = H * 0.40, cy = H * 0.47, cxf = W * 0.34, cxb = W * 0.63;
        // sombra no chão
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(gc->CreateRadialGradientBrush(W * 0.5, cy + ry, W * 0.5, cy + ry, W * 0.42, wxColour(0, 0, 0, 70), wxColour(0, 0, 0, 0)));
        gc->DrawEllipse(W * 0.08, cy + ry - H * 0.07, W * 0.84, H * 0.14);
        // corpo: o cilindro indo para trás, mais escuro embaixo
        gc->SetBrush(gc->CreateLinearGradientBrush(0, cy - ry, 0, cy + ry, tom(cor, 0.16), tom(cor, -0.32)));
        gc->DrawEllipse(cxb - rx, cy - ry, 2 * rx, 2 * ry);
        gc->DrawRectangle(cxf, cy - ry, cxb - cxf, 2 * ry);
        // face da frente, com a luz de cima à esquerda
        gc->SetBrush(gc->CreateRadialGradientBrush(cxf - rx * 0.35, cy - ry * 0.45, cxf - rx * 0.35, cy - ry * 0.45, rx * 1.75, tom(cor, 0.24),
                                                   tom(cor, -0.12)));
        gc->SetPen(wxPen(wxColour(0, 0, 0, 38), 1));
        gc->DrawEllipse(cxf - rx, cy - ry, 2 * rx, 2 * ry);
        // voltas do filamento, bem leves
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        for (double k : {0.84, 0.68, 0.52}) {
            gc->SetPen(wxPen(wxColour(0, 0, 0, 16), 1));
            gc->DrawEllipse(cxf - rx * k, cy - ry * k, 2 * rx * k, 2 * ry * k);
        }
        // furo: parede iluminada à direita e o fundo escuro à esquerda
        const double hx = rx * 0.30, hy = ry * 0.31;
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(tom(cor, -0.42)));
        gc->DrawEllipse(cxf - hx, cy - hy, 2 * hx, 2 * hy);
        gc->SetBrush(wxBrush(tom(cor, -0.66)));
        gc->DrawEllipse(cxf - hx, cy - hy * 0.92, hx * 1.25, hy * 1.84);
    }
    return wxBitmap(img);
}

// ------------------------------------------------------------------------------------------------ faixa
Faixa::Faixa(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(40)));
    SetToolTip(wxString::FromUTF8("Selecionar impressora"));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { m_sobre = true; cursor_mao(this, true); Refresh(); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = false; cursor_mao(this, false); Refresh(); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) { if (ao_clicar) ao_clicar(); });
}

void Faixa::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    const wxSize s = GetClientSize();
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(m_sobre ? MAGENTA_ESCURO : MAGENTA));
    gc->DrawRoundedRectangle(0, 0, s.x, s.y, FromDIP(10));
    gc->SetFont(fonte(this, 10.5, true), BRANCO);
    double tw = 0, th = 0;
    gc->GetTextExtent(wxString::FromUTF8("SELECIONAR IMPRESSORA"), &tw, &th);
    gc->DrawText(wxString::FromUTF8("SELECIONAR IMPRESSORA"), FromDIP(14), (s.y - th) / 2.);
    seta(*gc, s.x - FromDIP(20), s.y / 2., FromDIP(5), BRANCO, FromDIP(2));
}

// ------------------------------------------------------------------------------------------------ cartão da impressora
CartaoImpressora::CartaoImpressora(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(FromDIP(200), FromDIP(124)));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        const int s = area_bico().Contains(e.GetPosition()) && m_bico_sel ? 1 : area_tipo().Contains(e.GetPosition()) ? 2 : 0;
        if (s != m_sobre) {
            m_sobre = s;
            cursor_mao(this, s != 0);
            Refresh();
        }
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = 0; cursor_mao(this, false); Refresh(); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
        if (area_bico().Contains(e.GetPosition()) && m_bico_sel && ao_bico)
            ao_bico(wxRect(ClientToScreen(area_bico().GetTopLeft()), area_bico().GetSize()));
        else if (area_tipo().Contains(e.GetPosition()) && ao_tipo)
            ao_tipo(wxRect(ClientToScreen(area_tipo().GetTopLeft()), area_tipo().GetSize()));
    });
}

wxRect CartaoImpressora::area_bico() const
{
    const wxSize s = GetClientSize();
    const int    x = FromDIP(10) + (s.y - FromDIP(20)) + FromDIP(10);
    return wxRect(x, FromDIP(28), s.x - x - FromDIP(10), FromDIP(30));
}

wxRect CartaoImpressora::area_tipo() const
{
    wxRect r = area_bico();
    r.y      = FromDIP(84);
    return r;
}

void CartaoImpressora::definir_foto(const wxBitmap& foto)
{
    m_foto = foto;
    Refresh();
}

void CartaoImpressora::definir(const wxString& bico, bool bico_selecionavel, const wxString& tipo_bico)
{
    if (bico == m_bico && bico_selecionavel == m_bico_sel && tipo_bico == m_tipo)
        return;
    m_bico     = bico;
    m_bico_sel = bico_selecionavel;
    m_tipo     = tipo_bico;
    Refresh();
}

void CartaoImpressora::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    const wxSize s = GetClientSize();
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(LILAS));
    gc->DrawRoundedRectangle(0, 0, s.x, s.y, FromDIP(12));
    const int lado = s.y - FromDIP(20);
    gc->SetBrush(wxBrush(BRANCO));
    gc->DrawRoundedRectangle(FromDIP(10), FromDIP(10), lado, lado, FromDIP(8));
    if (m_foto.IsOk()) {
        const double esc = std::min(double(lado - FromDIP(6)) / m_foto.GetWidth(), double(lado - FromDIP(6)) / m_foto.GetHeight());
        const double w = m_foto.GetWidth() * esc, h = m_foto.GetHeight() * esc;
        gc->DrawBitmap(m_foto, FromDIP(10) + (lado - w) / 2., FromDIP(10) + (lado - h) / 2., w, h);
    }
    seletor(*gc, this, area_bico(), wxString::FromUTF8("Bico"), m_bico, m_bico_sel, m_sobre == 1, LILAS_BORDA, 10.5);
    seletor(*gc, this, area_tipo(), wxString::FromUTF8("Tipo de bico"), m_tipo, true, m_sobre == 2, LILAS_BORDA, 10.5);
}

// ------------------------------------------------------------------------------------------------ cartão da mesa
CartaoMesa::CartaoMesa(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(FromDIP(108), FromDIP(124)));
    SetMaxSize(wxSize(FromDIP(108), -1));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        const bool s = m_selecionavel && area_seletor().Contains(e.GetPosition());
        if (s != m_sobre) {
            m_sobre = s;
            cursor_mao(this, s);
            Refresh();
        }
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = false; cursor_mao(this, false); Refresh(); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
        if (m_selecionavel && area_seletor().Contains(e.GetPosition()) && ao_clicar)
            ao_clicar(wxRect(ClientToScreen(area_seletor().GetTopLeft()), area_seletor().GetSize()));
    });
}

wxRect CartaoMesa::area_seletor() const
{
    const wxSize s = GetClientSize();
    return wxRect(FromDIP(6), FromDIP(88), s.x - FromDIP(12), FromDIP(26));
}

void CartaoMesa::definir_foto(const wxBitmap& foto)
{
    m_foto = foto;
    Refresh();
}

void CartaoMesa::definir(const wxString& mesa, bool selecionavel)
{
    if (mesa == m_mesa && selecionavel == m_selecionavel)
        return;
    m_mesa         = mesa;
    m_selecionavel = selecionavel;
    Refresh();
}

void CartaoMesa::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    const wxSize s = GetClientSize();
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(ROSA));
    gc->DrawRoundedRectangle(0, 0, s.x, s.y, FromDIP(12));
    const int lado = FromDIP(52);
    if (m_foto.IsOk()) {
        const double esc = std::min(double(lado) / m_foto.GetWidth(), double(lado) / m_foto.GetHeight());
        const double w = m_foto.GetWidth() * esc, h = m_foto.GetHeight() * esc;
        gc->DrawBitmap(m_foto, (s.x - w) / 2., FromDIP(8) + (lado - h) / 2., w, h);
    }
    seletor(*gc, this, area_seletor(), wxString::FromUTF8("Mesa"), m_mesa, m_selecionavel, m_sobre, ROSA_BORDA, 8.5);
}

// ------------------------------------------------------------------------------------------------ bobinas
Bobinas::Bobinas(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
        m_desenhos.clear(); // a largura do cartão mudou: redesenha as bobinas no tamanho novo
        Refresh();
        e.Skip();
    });
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i != m_sobre) {
            m_sobre = i;
            cursor_mao(this, i >= 0);
            Refresh();
        }
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = -1; cursor_mao(this, false); Refresh(); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i >= 0 && ao_clicar)
            ao_clicar(i, wxRect(ClientToScreen(cartao(i).GetTopLeft()), cartao(i).GetSize()));
    });
    Bind(wxEVT_RIGHT_UP, [this](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i >= 0 && ao_menu)
            ao_menu(i, ClientToScreen(e.GetPosition()));
    });
}

void Bobinas::definir(const std::vector<InfoBobina>& bobinas)
{
    const bool igual = bobinas.size() == m_bobinas.size() &&
                       std::equal(bobinas.begin(), bobinas.end(), m_bobinas.begin(), [](const InfoBobina& a, const InfoBobina& b) {
                           return a.numero == b.numero && a.cor == b.cor && a.marca == b.marca && a.tipo == b.tipo && a.estoque == b.estoque;
                       });
    if (igual)
        return;
    m_bobinas = bobinas;
    m_desenhos.clear();
    recalcular_altura();
    Refresh();
}

void Bobinas::recalcular_altura()
{
    const int linhas = std::max(1, int((m_bobinas.size() + 3) / 4));
    const int h      = m_bobinas.empty() ? 0 : linhas * FromDIP(126) + (linhas - 1) * FromDIP(6);
    SetMinSize(wxSize(-1, h));
    InvalidateBestSize();
}

wxRect Bobinas::cartao(int i) const
{
    const int gap = FromDIP(6), w = std::max(FromDIP(40), (GetClientSize().x - 3 * gap) / 4);
    return wxRect((i % 4) * (w + gap), (i / 4) * (FromDIP(126) + gap), w, FromDIP(126));
}

int Bobinas::sob(const wxPoint& p) const
{
    for (int i = 0; i < int(m_bobinas.size()); ++i)
        if (cartao(i).Contains(p))
            return i;
    return -1;
}

void Bobinas::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc || m_bobinas.empty())
        return;
    const int larg_bobina = std::min(cartao(0).width - FromDIP(14), FromDIP(70));
    if (m_desenhos.size() != m_bobinas.size()) {
        m_desenhos.clear();
        for (const InfoBobina& b : m_bobinas)
            m_desenhos.push_back(bobina(b.cor, larg_bobina));
    }
    const wxFont f_marca = fonte(this, 8., false), f_tipo = fonte(this, 9., true), f_estoque = fonte(this, 8., false), f_num = fonte(this, 7.5, true);
    for (int i = 0; i < int(m_bobinas.size()); ++i) {
        const InfoBobina& b = m_bobinas[i];
        const wxRect      r = cartao(i);
        const bool        s = i == m_sobre;
        gc->SetPen(wxPen(s ? MAGENTA : CARTAO_BORDA, FromDIP(1)));
        gc->SetBrush(wxBrush(s ? CARTAO_SOBRE : CARTAO));
        gc->DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, FromDIP(12));
        const wxBitmap& d  = m_desenhos[i];
        const double    bx = r.x + (r.width - d.GetWidth()) / 2., by = r.y + FromDIP(10);
        gc->DrawBitmap(d, bx, by, d.GetWidth(), d.GetHeight());
        // número da entrada no canto da foto
        const double nd = FromDIP(18);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(s ? MAGENTA : TEXTO_CLARO));
        gc->DrawEllipse(r.x + FromDIP(5), r.y + FromDIP(5), nd, nd);
        gc->SetFont(f_num, s ? BRANCO : TINTA);
        double tw = 0, th = 0;
        gc->GetTextExtent(b.numero, &tw, &th);
        gc->DrawText(b.numero, r.x + FromDIP(5) + (nd - tw) / 2., r.y + FromDIP(5) + (nd - th) / 2.);
        // marca, tipo e estoque embaixo
        const double cx = r.x + r.width / 2.;
        double       y  = by + d.GetHeight() + FromDIP(3);
        const int    lt = r.width - FromDIP(8);
        texto_centro(*gc, this, b.marca.empty() ? wxString::FromUTF8("Genérico") : b.marca, f_marca, TEXTO_CLARO, cx, y, lt);
        y += FromDIP(15);
        texto_centro(*gc, this, b.tipo, f_tipo, s ? MAGENTA_CLARO : TEXTO_CLARO, cx, y, lt);
        y += FromDIP(16);
        texto_centro(*gc, this, b.estoque, f_estoque, TEXTO_APAGADO, cx, y, lt);
    }
}

}}} // namespace Slic3r::GUI::FilunaLateral
