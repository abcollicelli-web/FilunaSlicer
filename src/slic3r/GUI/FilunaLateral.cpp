#include "FilunaLateral.hpp"

#include "Widgets/TabCtrl.hpp"

#include <wx/control.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/image.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace Slic3r { namespace GUI { namespace FilunaLateral {

namespace pintura {

const wxColour MAGENTA(0xE0, 0x18, 0x6B), MAGENTA_ESCURO(0xC9, 0x14, 0x5E), MAGENTA_CLARO(0xFF, 0x6F, 0xA8), VINHO(0x8A, 0x3A, 0x5C);
const wxColour LILAS(0xF3, 0xE8, 0xFB), LILAS_BORDA(0xD9, 0xC8, 0xEC), ROSA(0xFB, 0xE1, 0xEC), ROSA_BORDA(0xF2, 0xC2, 0xD6);
const wxColour ROSA_CLARO(0xFF, 0xF4, 0xF8), AREIA(0xF1, 0xEC, 0xE5), AREIA_BORDA(0xE6, 0xE0, 0xD7), AREIA_ESCURA(0xB9, 0xAE, 0xA2);
const wxColour TINTA(0x1D, 0x1A, 0x17), CINZA(0x6E, 0x62, 0x59), BRANCO(0xFF, 0xFF, 0xFF);
const wxColour CARTAO(0x2A, 0x29, 0x2E), CARTAO_BORDA(0x3A, 0x39, 0x3F), CARTAO_SOBRE(0x3A, 0x26, 0x30);
const wxColour TEXTO_CLARO(0xF4, 0xF2, 0xF5), TEXTO_APAGADO(0xB4, 0xAF, 0xB9);

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

void seta(wxGraphicsContext& gc, double cx, double cy, double t, const wxColour& cor, double esp, bool para_cima = false)
{
    gc.SetPen(wxPen(cor, int(std::max(1., esp))));
    const double v = para_cima ? -1. : 1.;
    wxGraphicsPath p = gc.CreatePath();
    p.MoveToPoint(cx - t, cy - v * t * 0.5);
    p.AddLineToPoint(cx, cy + v * t * 0.5);
    p.AddLineToPoint(cx + t, cy - v * t * 0.5);
    gc.StrokePath(p);
}

// Texto que cabe na largura (corta com "…").
wxString caber(wxWindow* w, const wxString& t, const wxFont& f, int largura)
{
    wxClientDC dc(w);
    dc.SetFont(f);
    return wxControl::Ellipsize(t, dc, wxELLIPSIZE_END, std::max(10, largura));
}

double largura_texto(wxGraphicsContext& gc, const wxString& t)
{
    double tw = 0, th = 0;
    gc.GetTextExtent(t, &tw, &th);
    return tw;
}

void texto_centro(wxGraphicsContext& gc, wxWindow* w, const wxString& t, const wxFont& f, const wxColour& cor, double cx, double y, int largura)
{
    const wxString s = caber(w, t, f, largura);
    gc.SetFont(f, cor);
    gc.DrawText(s, cx - largura_texto(gc, s) / 2., y);
}

// Texto centrado na vertical do retângulo, a partir de x.
void texto_meio(wxGraphicsContext& gc, wxWindow* w, const wxString& t, const wxFont& f, const wxColour& cor, double x, const wxRect& r, int largura)
{
    const wxString s = caber(w, t, f, largura);
    gc.SetFont(f, cor);
    double tw = 0, th = 0;
    gc.GetTextExtent(s, &tw, &th);
    gc.DrawText(s, x, r.y + (r.height - th) / 2.);
}

// Seletor: rótulo em cima e caixa com o valor e a seta. `fundo` inválido = branco.
void seletor(wxGraphicsContext& gc, wxWindow* w, const wxRect& caixa, const wxString& rotulo, const wxString& valor, bool ativo, bool sobre,
             const wxColour& borda, const wxColour& fundo = wxColour(), const wxBitmap& imagem = wxBitmap())
{
    const int pad = w->FromDIP(7);
    gc.SetFont(fonte(w, 7., true), CINZA);
    gc.DrawText(rotulo.Upper(), caixa.x + 1, caixa.y - w->FromDIP(13));
    gc.SetPen(wxPen(sobre && ativo ? MAGENTA : borda, w->FromDIP(1)));
    gc.SetBrush(wxBrush(fundo.IsOk() ? fundo : BRANCO));
    gc.DrawRoundedRectangle(caixa.x, caixa.y, caixa.width, caixa.height, w->FromDIP(7));
    int x = caixa.x + pad;
    if (imagem.IsOk()) {
        const double lado = caixa.height - w->FromDIP(8);
        const double esc  = std::min(lado / imagem.GetWidth(), lado / imagem.GetHeight());
        gc.DrawBitmap(imagem, x - w->FromDIP(3), caixa.y + (caixa.height - imagem.GetHeight() * esc) / 2., imagem.GetWidth() * esc,
                      imagem.GetHeight() * esc);
        x += int(lado) + w->FromDIP(3);
    }
    const int seta_l = ativo ? w->FromDIP(14) : 0;
    texto_meio(gc, w, valor, fonte(w, 9.5, true), ativo ? TINTA : CINZA, x, caixa, caixa.GetRight() - x - pad - seta_l);
    if (ativo)
        seta(gc, caixa.GetRight() - pad - w->FromDIP(3), caixa.y + caixa.height / 2., w->FromDIP(4), CINZA, w->FromDIP(2));
}

void cursor_mao(wxWindow* w, bool mao) { w->SetCursor(mao ? wxCursor(wxCURSOR_HAND) : wxNullCursor); }

// Os ícones dos botões, em traço, no quadrado `r`.
void desenhar_icone(wxGraphicsContext& gc, wxWindow* w, Icone ic, const wxRect2DDouble& r, const wxColour& cor, bool ligado)
{
    const double x = r.m_x, y = r.m_y, l = r.m_width, cx = x + l / 2., cy = y + l / 2.;
    gc.SetPen(wxPen(cor, std::max(1, w->FromDIP(2)), wxPENSTYLE_SOLID));
    gc.SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath p = gc.CreatePath();
    switch (ic) {
    case Icone::mais:
        p.MoveToPoint(cx, y + l * 0.15); p.AddLineToPoint(cx, y + l * 0.85);
        p.MoveToPoint(x + l * 0.15, cy); p.AddLineToPoint(x + l * 0.85, cy);
        break;
    case Icone::sincronizar:
        p.AddArc(cx, cy, l * 0.34, -2.7, 0.2, true);
        p.MoveToPoint(x + l * 0.84, y + l * 0.30); p.AddLineToPoint(x + l * 0.86, cy + l * 0.04); p.AddLineToPoint(x + l * 0.60, cy - l * 0.02);
        p.AddArc(cx, cy, l * 0.34, 0.44, 3.34, true);
        break;
    case Icone::misto:
        p.AddCircle(cx - l * 0.14, cy, l * 0.26);
        p.AddCircle(cx + l * 0.14, cy, l * 0.26);
        break;
    case Icone::lapis:
        p.MoveToPoint(x + l * 0.18, y + l * 0.82); p.AddLineToPoint(x + l * 0.20, y + l * 0.62);
        p.AddLineToPoint(x + l * 0.66, y + l * 0.16); p.AddLineToPoint(x + l * 0.84, y + l * 0.34);
        p.AddLineToPoint(x + l * 0.38, y + l * 0.80); p.CloseSubpath();
        break;
    case Icone::lixeira:
        p.MoveToPoint(x + l * 0.16, y + l * 0.26); p.AddLineToPoint(x + l * 0.84, y + l * 0.26);
        p.MoveToPoint(x + l * 0.40, y + l * 0.26); p.AddLineToPoint(x + l * 0.40, y + l * 0.14);
        p.AddLineToPoint(x + l * 0.60, y + l * 0.14); p.AddLineToPoint(x + l * 0.60, y + l * 0.26);
        p.MoveToPoint(x + l * 0.26, y + l * 0.26); p.AddLineToPoint(x + l * 0.31, y + l * 0.86);
        p.AddLineToPoint(x + l * 0.69, y + l * 0.86); p.AddLineToPoint(x + l * 0.74, y + l * 0.26);
        break;
    case Icone::atomo:
        for (double a : {0., M_PI / 3., 2. * M_PI / 3.}) {
            wxGraphicsMatrix m = gc.CreateMatrix();
            m.Translate(cx, cy);
            m.Rotate(a);
            wxGraphicsPath e = gc.CreatePath();
            e.AddEllipse(-l * 0.42, -l * 0.16, l * 0.84, l * 0.32);
            e.Transform(m);
            gc.StrokePath(e);
        }
        gc.SetBrush(wxBrush(cor));
        gc.DrawEllipse(cx - l * 0.07, cy - l * 0.07, l * 0.14, l * 0.14);
        return;
    case Icone::chave: {
        const double cw = l * 1.5, ch = l * 0.78, cx0 = cx - cw / 2., cy0 = cy - ch / 2.;
        gc.SetPen(*wxTRANSPARENT_PEN);
        gc.SetBrush(wxBrush(ligado ? MAGENTA : AREIA_ESCURA));
        gc.DrawRoundedRectangle(cx0, cy0, cw, ch, ch / 2.);
        gc.SetBrush(wxBrush(BRANCO));
        const double d = ch - l * 0.2;
        gc.DrawEllipse(ligado ? cx0 + cw - d - l * 0.1 : cx0 + l * 0.1, cy0 + l * 0.1, d, d);
        return;
    }
    case Icone::tabela:
        p.AddRoundedRectangle(x + l * 0.16, y + l * 0.12, l * 0.68, l * 0.76, l * 0.08);
        for (double k : {0.36, 0.52, 0.68}) {
            p.MoveToPoint(x + l * 0.30, y + l * k); p.AddLineToPoint(x + l * (k > 0.6 ? 0.56 : 0.70), y + l * k);
        }
        break;
    case Icone::comparar:
        p.MoveToPoint(x + l * 0.08, y + l * 0.70); p.AddLineToPoint(x + l * 0.24, y + l * 0.22); p.AddLineToPoint(x + l * 0.40, y + l * 0.70);
        p.MoveToPoint(x + l * 0.14, y + l * 0.52); p.AddLineToPoint(x + l * 0.34, y + l * 0.52);
        p.MoveToPoint(x + l * 0.58, y + l * 0.22); p.AddLineToPoint(x + l * 0.58, y + l * 0.70);
        p.AddLineToPoint(x + l * 0.78, y + l * 0.70);
        p.AddArc(x + l * 0.78, y + l * 0.58, l * 0.12, M_PI / 2., -M_PI / 2., false);
        p.AddLineToPoint(x + l * 0.58, y + l * 0.46); p.AddLineToPoint(x + l * 0.76, y + l * 0.46);
        p.AddArc(x + l * 0.76, y + l * 0.34, l * 0.12, M_PI / 2., -M_PI / 2., false);
        p.AddLineToPoint(x + l * 0.58, y + l * 0.22);
        break;
    case Icone::seta_cima:
    case Icone::seta_baixo:
        seta(gc, cx, cy, l * 0.28, cor, w->FromDIP(2), ic == Icone::seta_cima);
        return;
    case Icone::nenhum: return;
    }
    gc.StrokePath(p);
}

// Um botão: fundo branco (ou magenta no destaque), borda areia (magenta sob o mouse), ícone e nome.
void desenhar_botao(wxGraphicsContext& gc, wxWindow* w, const wxRect& r, const Botao& b, bool sobre, bool coluna)
{
    const wxColour fundo = b.destaque ? (sobre ? MAGENTA_ESCURO : MAGENTA) : BRANCO;
    gc.SetPen(wxPen(sobre && b.ativo && !b.destaque ? MAGENTA : b.destaque ? fundo : AREIA_BORDA, w->FromDIP(1)));
    gc.SetBrush(wxBrush(fundo));
    gc.DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, w->FromDIP(8));
    const wxColour cor_txt = !b.ativo ? AREIA_ESCURA : b.destaque ? BRANCO : TINTA;
    const wxColour cor_ic  = !b.ativo ? AREIA_ESCURA : b.destaque ? BRANCO : b.icone == Icone::mais ? MAGENTA : CINZA;
    const double   li      = w->FromDIP(coluna ? 18 : 15);
    const wxFont   f       = fonte(w, coluna ? 7.5 : 8.5, true);
    gc.SetFont(f, cor_txt);
    const bool   tem_ic = b.icone != Icone::nenhum || b.cor.IsOk();
    const double tw     = b.texto.empty() ? 0. : largura_texto(gc, b.texto);
    auto icone = [&](double ix, double iy) {
        if (b.cor.IsOk()) {
            gc.SetPen(wxPen(TINTA, 1));
            gc.SetBrush(wxBrush(b.cor));
            gc.DrawEllipse(ix + li * 0.06, iy + li * 0.06, li * 0.88, li * 0.88);
        } else
            desenhar_icone(gc, w, b.icone, wxRect2DDouble(ix, iy, li, li), cor_ic, b.ligado);
    };
    if (coluna) {
        const double y0 = r.y + (r.height - li - w->FromDIP(16)) / 2.;
        if (tem_ic)
            icone(r.x + (r.width - li) / 2., y0);
        texto_centro(gc, w, b.texto, f, cor_txt, r.x + r.width / 2., y0 + li + w->FromDIP(3), r.width - w->FromDIP(4));
        return;
    }
    const double gap = tem_ic && tw > 0 ? w->FromDIP(5) : 0.;
    const double tot = std::min<double>((tem_ic ? li : 0.) + gap + tw, r.width - w->FromDIP(8));
    double       x   = r.x + (r.width - tot) / 2.;
    if (tem_ic) {
        icone(x, r.y + (r.height - li) / 2.);
        x += li + gap;
    }
    if (!b.texto.empty())
        texto_meio(gc, w, b.texto, f, cor_txt, x, r, int(r.GetRight() - x - w->FromDIP(3)));
}

// Largura natural de um botão de linha (ícone + nome + folga).
int largura_botao(wxWindow* w, const Botao& b)
{
    if (b.texto.empty())
        return w->FromDIP(30);
    wxClientDC dc(w);
    dc.SetFont(fonte(w, 8.5, true));
    const bool tem_ic = b.icone != Icone::nenhum || b.cor.IsOk();
    return dc.GetTextExtent(b.texto).x + (tem_ic ? w->FromDIP(20) : 0) + w->FromDIP(18);
}

} // namespace pintura

using namespace pintura;

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
    SetMinSize(wxSize(-1, FromDIP(36)));
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
    gc->SetFont(fonte(this, 10., true), BRANCO);
    double tw = 0, th = 0;
    gc->GetTextExtent(wxString::FromUTF8("SELECIONAR IMPRESSORA"), &tw, &th);
    gc->DrawText(wxString::FromUTF8("SELECIONAR IMPRESSORA"), FromDIP(14), (s.y - th) / 2.);
    seta(*gc, s.x - FromDIP(20), s.y / 2., FromDIP(5), BRANCO, FromDIP(2));
}

// ------------------------------------------------------------------------------------------------ cartão da impressora
CartaoImpressora::CartaoImpressora(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(FromDIP(220), FromDIP(116)));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    auto ativo  = [this](int i) { return i == 1 ? m_bico_sel : i == 3 ? m_mesa_sel : true; };
    auto mudar  = [this](int s) {
        if (s == m_sobre)
            return;
        if ((s == 3) != (m_sobre == 3) && ao_mesa_sobre)
            ao_mesa_sobre(s == 3, wxRect(ClientToScreen(area(3).GetTopLeft()), area(3).GetSize()));
        m_sobre = s;
        cursor_mao(this, s != 0);
        Refresh();
    };
    Bind(wxEVT_MOTION, [this, ativo, mudar](wxMouseEvent& e) {
        int s = 0;
        for (int i = 1; i <= 4; ++i)
            if (ativo(i) && area(i).Contains(e.GetPosition()))
                s = i;
        mudar(s);
    });
    Bind(wxEVT_LEAVE_WINDOW, [mudar](wxMouseEvent&) { mudar(0); });
    Bind(wxEVT_LEFT_UP, [this, ativo, mudar](wxMouseEvent& e) {
        if (area(4).Contains(e.GetPosition())) {
            if (ao_editar)
                ao_editar();
            return;
        }
        for (int i = 1; i <= 3; ++i) {
            if (!ativo(i) || !area(i).Contains(e.GetPosition()))
                continue;
            if (i == 3)
                mudar(0); // a lista da mesa abre: a imagem grande sai
            const wxRect tela(ClientToScreen(area(i).GetTopLeft()), area(i).GetSize());
            const auto&  cb = i == 1 ? ao_bico : i == 2 ? ao_tipo : ao_mesa;
            if (cb)
                cb(tela);
        }
    });
}

// Foto à esquerda (quadrada na altura do cartão); à direita, bico e tipo lado a lado e a mesa embaixo, ocupando os dois.
wxRect CartaoImpressora::area(int i) const
{
    const wxSize s    = GetClientSize();
    const int    pad  = FromDIP(8), foto = s.y - 2 * pad, x0 = pad + foto + FromDIP(10), w = s.x - x0 - pad, gap = FromDIP(6);
    const int    h    = FromDIP(28), y1 = FromDIP(22), y2 = y1 + h + FromDIP(20);
    const int    w_bico = (w - gap) * 2 / 5;
    switch (i) {
    case 1: return wxRect(x0, y1, w_bico, h);
    case 2: return wxRect(x0 + w_bico + gap, y1, w - w_bico - gap, h);
    case 3: return wxRect(x0, y2, w, h);
    default: return wxRect(pad + foto - FromDIP(26), pad + FromDIP(4), FromDIP(22), FromDIP(22)); // lápis no canto da foto
    }
}

void CartaoImpressora::definir_foto(const wxBitmap& foto)
{
    m_foto = foto;
    Refresh();
}

void CartaoImpressora::definir_foto_mesa(const wxBitmap& foto)
{
    m_foto_mesa = foto;
    Refresh();
}

void CartaoImpressora::definir(const wxString& bico, bool bico_selecionavel, const wxString& tipo_bico, const wxString& mesa, bool mesa_selecionavel)
{
    if (bico == m_bico && bico_selecionavel == m_bico_sel && tipo_bico == m_tipo && mesa == m_mesa && mesa_selecionavel == m_mesa_sel)
        return;
    m_bico     = bico;
    m_bico_sel = bico_selecionavel;
    m_tipo     = tipo_bico;
    m_mesa     = mesa;
    m_mesa_sel = mesa_selecionavel;
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
    const int pad = FromDIP(8), lado = s.y - 2 * pad;
    gc->SetBrush(wxBrush(BRANCO));
    gc->DrawRoundedRectangle(pad, pad, lado, lado, FromDIP(8));
    if (m_foto.IsOk()) {
        const double esc = std::min(double(lado - FromDIP(6)) / m_foto.GetWidth(), double(lado - FromDIP(6)) / m_foto.GetHeight());
        const double w = m_foto.GetWidth() * esc, h = m_foto.GetHeight() * esc;
        gc->DrawBitmap(m_foto, pad + (lado - w) / 2., pad + (lado - h) / 2., w, h);
    }
    seletor(*gc, this, area(1), wxString::FromUTF8("Bico"), m_bico, m_bico_sel, m_sobre == 1, LILAS_BORDA);
    seletor(*gc, this, area(2), wxString::FromUTF8("Tipo de bico"), m_tipo, true, m_sobre == 2, LILAS_BORDA);
    seletor(*gc, this, area(3), wxString::FromUTF8("Mesa"), m_mesa, m_mesa_sel, m_sobre == 3, ROSA_BORDA, ROSA, m_foto_mesa);
    const wxRect l = area(4);
    gc->SetPen(wxPen(m_sobre == 4 ? MAGENTA : LILAS_BORDA, FromDIP(1)));
    gc->SetBrush(wxBrush(BRANCO));
    gc->DrawEllipse(l.x, l.y, l.width, l.height);
    desenhar_icone(*gc, this, Icone::lapis, wxRect2DDouble(l.x + FromDIP(4), l.y + FromDIP(4), l.width - FromDIP(8), l.height - FromDIP(8)),
                   m_sobre == 4 ? MAGENTA : CINZA, false);
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
        if (i == int(m_bobinas.size()) && ao_adicionar)
            ao_adicionar();
        else if (i >= 0 && ao_clicar)
            ao_clicar(i);
    });
    Bind(wxEVT_RIGHT_UP, [this](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i >= 0 && i < int(m_bobinas.size()) && ao_menu)
            ao_menu(i, ClientToScreen(e.GetPosition()));
    });
}

void Bobinas::definir(const std::vector<InfoBobina>& bobinas, int selecionada)
{
    const bool igual = selecionada == m_sel && bobinas.size() == m_bobinas.size() &&
                       std::equal(bobinas.begin(), bobinas.end(), m_bobinas.begin(), [](const InfoBobina& a, const InfoBobina& b) {
                           return a.numero == b.numero && a.cor == b.cor && a.marca == b.marca && a.tipo == b.tipo && a.estoque == b.estoque;
                       });
    if (igual)
        return;
    if (bobinas.size() != m_bobinas.size() ||
        !std::equal(bobinas.begin(), bobinas.end(), m_bobinas.begin(), [](const InfoBobina& a, const InfoBobina& b) { return a.cor == b.cor; }))
        m_desenhos.clear();
    m_bobinas = bobinas;
    m_sel     = selecionada;
    recalcular_altura();
    Refresh();
}

int Bobinas::altura_linha() const { return FromDIP(104) + FromDIP(6); }

void Bobinas::recalcular_altura()
{
    const int linhas = int(m_bobinas.size() + 1 + 3) / 4; // + o cartão "Adicionar"
    SetMinSize(wxSize(-1, linhas * altura_linha() - FromDIP(6)));
    InvalidateBestSize();
}

wxRect Bobinas::cartao(int i) const
{
    const int gap = FromDIP(6), w = std::max(FromDIP(40), (GetClientSize().x - 3 * gap) / 4);
    return wxRect((i % 4) * (w + gap), (i / 4) * altura_linha(), w, FromDIP(104));
}

int Bobinas::sob(const wxPoint& p) const
{
    for (int i = 0; i <= int(m_bobinas.size()); ++i)
        if (cartao(i).Contains(p))
            return i;
    return -1;
}

void Bobinas::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    const int larg_bobina = std::min(cartao(0).width - FromDIP(16), FromDIP(58));
    if (m_desenhos.size() != m_bobinas.size()) {
        m_desenhos.clear();
        for (const InfoBobina& b : m_bobinas)
            m_desenhos.push_back(bobina(b.cor, larg_bobina));
    }
    const wxFont f_marca = fonte(this, 7.5, true), f_tipo = fonte(this, 9., true), f_estoque = fonte(this, 7.5, false), f_num = fonte(this, 7., true);
    for (int i = 0; i < int(m_bobinas.size()); ++i) {
        const InfoBobina& b   = m_bobinas[i];
        const wxRect      r   = cartao(i);
        const bool        sel = i == m_sel, s = i == m_sobre;
        gc->SetPen(wxPen(sel || s ? MAGENTA : CARTAO_BORDA, FromDIP(sel ? 2 : 1)));
        gc->SetBrush(wxBrush(sel || s ? CARTAO_SOBRE : CARTAO));
        gc->DrawRoundedRectangle(r.x + 1, r.y + 1, r.width - 2, r.height - 2, FromDIP(10));
        const wxBitmap& d  = m_desenhos[i];
        const double    bx = r.x + (r.width - d.GetWidth()) / 2., by = r.y + FromDIP(7);
        gc->DrawBitmap(d, bx, by, d.GetWidth(), d.GetHeight());
        // número da entrada no canto da foto
        const double nd = FromDIP(17);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(sel ? MAGENTA : TEXTO_CLARO));
        gc->DrawEllipse(r.x + FromDIP(5), r.y + FromDIP(5), nd, nd);
        gc->SetFont(f_num, sel ? BRANCO : TINTA);
        double tw = 0, th = 0;
        gc->GetTextExtent(b.numero, &tw, &th);
        gc->DrawText(b.numero, r.x + FromDIP(5) + (nd - tw) / 2., r.y + FromDIP(5) + (nd - th) / 2.);
        // marca, tipo e estoque embaixo
        const double cx = r.x + r.width / 2.;
        double       y  = by + d.GetHeight() + FromDIP(1);
        const int    lt = r.width - FromDIP(8);
        texto_centro(*gc, this, b.marca.empty() ? wxString::FromUTF8("Genérico") : b.marca, f_marca, TEXTO_CLARO, cx, y, lt);
        y += FromDIP(13);
        texto_centro(*gc, this, b.tipo, f_tipo, sel ? MAGENTA_CLARO : TEXTO_CLARO, cx, y, lt);
        y += FromDIP(15);
        texto_centro(*gc, this, b.estoque, f_estoque, TEXTO_APAGADO, cx, y, lt);
    }
    // cartão "Adicionar", tracejado
    const wxRect r = cartao(int(m_bobinas.size()));
    const bool   s = m_sobre == int(m_bobinas.size());
    gc->SetPen(wxPen(s ? MAGENTA : AREIA_ESCURA, FromDIP(1), wxPENSTYLE_SHORT_DASH));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRoundedRectangle(r.x + 1, r.y + 1, r.width - 2, r.height - 2, FromDIP(10));
    const double li = FromDIP(22);
    desenhar_icone(*gc, this, Icone::mais, wxRect2DDouble(r.x + (r.width - li) / 2., r.y + r.height / 2. - li, li, li), s ? MAGENTA : CINZA, false);
    texto_centro(*gc, this, wxString::FromUTF8("Adicionar"), fonte(this, 8., true), s ? MAGENTA : CINZA, r.x + r.width / 2., r.y + r.height / 2. + FromDIP(4),
                 r.width - FromDIP(6));
}

// ------------------------------------------------------------------------------------------------ botões
Botoes::Botoes(wxWindow* pai, bool coluna)
    : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE), m_coluna(coluna)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(coluna ? 50 : 32)));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
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
            ao_clicar(i, wxRect(ClientToScreen(area(i).GetTopLeft()), area(i).GetSize()));
    });
}

void Botoes::definir(const std::vector<Botao>& botoes)
{
    if (botoes == m_botoes)
        return;
    m_botoes = botoes;
    Refresh();
}

wxRect Botoes::area(int i) const
{
    const int n = std::max(1, int(m_botoes.size())), gap = FromDIP(6), w = (GetClientSize().x - (n - 1) * gap) / n;
    return wxRect(i * (w + gap), 0, w, GetClientSize().y);
}

int Botoes::sob(const wxPoint& p) const
{
    for (int i = 0; i < int(m_botoes.size()); ++i)
        if (m_botoes[i].ativo && area(i).Contains(p))
            return i;
    return -1;
}

void Botoes::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    for (int i = 0; i < int(m_botoes.size()); ++i)
        desenhar_botao(*gc, this, area(i), m_botoes[i], i == m_sobre, m_coluna);
}

// ------------------------------------------------------------------------------------------------ barra da bobina
BarraBobina::BarraBobina(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(44)));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
    auto sob = [this](const wxPoint& p) {
        const auto b = botoes();
        for (int i = 0; i < 4; ++i)
            if (b[i].ativo && area(i).Contains(p))
                return i;
        return -1;
    };
    Bind(wxEVT_MOTION, [this, sob](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i != m_sobre) {
            m_sobre = i;
            cursor_mao(this, i >= 0);
            Refresh();
        }
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = -1; cursor_mao(this, false); Refresh(); });
    Bind(wxEVT_LEFT_UP, [this, sob](wxMouseEvent& e) {
        switch (sob(e.GetPosition())) {
        case 0: if (ao_cor) ao_cor(wxRect(ClientToScreen(area(0).GetTopLeft()), area(0).GetSize())); break;
        case 1: if (ao_trocar) ao_trocar(); break;
        case 2: if (ao_ajustes) ao_ajustes(); break;
        case 3: if (ao_tirar) ao_tirar(); break;
        default: break;
        }
    });
}

std::vector<Botao> BarraBobina::botoes() const
{
    Botao cor{wxString::FromUTF8("Cor")};
    cor.cor = m_cor.IsOk() ? m_cor : *wxWHITE;
    Botao tirar{wxString(), Icone::lixeira};
    tirar.ativo = m_pode_tirar;
    return {cor, Botao{wxString::FromUTF8("Trocar")}, Botao{wxString(), Icone::lapis}, tirar};
}

wxRect BarraBobina::area(int i) const
{
    const auto   b   = botoes();
    const wxSize s   = GetClientSize();
    const int    gap = FromDIP(5), h = FromDIP(30), y = (s.y - h) / 2;
    int          x   = s.x - FromDIP(6);
    for (int k = int(b.size()) - 1; k >= 0; --k) {
        const int w = largura_botao(const_cast<BarraBobina*>(this), b[k]);
        x -= w;
        if (k == i)
            return wxRect(x, y, w, h);
        x -= gap;
    }
    return wxRect();
}

void BarraBobina::definir(const wxString& entrada, const wxString& nome, const wxColour& cor, bool pode_tirar)
{
    if (entrada == m_entrada && nome == m_nome && cor == m_cor && pode_tirar == m_pode_tirar)
        return;
    m_entrada    = entrada;
    m_nome       = nome;
    m_cor        = cor;
    m_pode_tirar = pode_tirar;
    Refresh();
}

void BarraBobina::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    const wxSize s = GetClientSize();
    gc->SetPen(wxPen(ROSA_BORDA, FromDIP(1)));
    gc->SetBrush(wxBrush(ROSA_CLARO));
    gc->DrawRoundedRectangle(0.5, 0.5, s.x - 1, s.y - 1, FromDIP(10));
    const auto b   = botoes();
    const int  max = area(0).x - FromDIP(16);
    gc->SetFont(fonte(this, 7., true), VINHO);
    gc->DrawText(m_entrada.Upper(), FromDIP(10), FromDIP(6));
    const wxString nome = caber(this, m_nome, fonte(this, 8.5, true), max);
    gc->SetFont(fonte(this, 8.5, true), TINTA);
    gc->DrawText(nome, FromDIP(10), FromDIP(20));
    for (int i = 0; i < 4; ++i)
        desenhar_botao(*gc, this, area(i), b[i], i == m_sobre, false);
}

// ------------------------------------------------------------------------------------------------ resumo
Resumo::Resumo(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(110)));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        const int s = area_abrir().Contains(e.GetPosition()) ? 1 : area_mais().Contains(e.GetPosition()) ? 2 : 0;
        if (s != m_sobre) {
            m_sobre = s;
            cursor_mao(this, s != 0);
            Refresh();
        }
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = 0; cursor_mao(this, false); Refresh(); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
        if (area_mais().Contains(e.GetPosition())) {
            if (ao_adicionar)
                ao_adicionar();
        } else if (ao_abrir) // qualquer outro ponto do resumo abre a parte de cima de novo
            ao_abrir();
    });
}

wxRect Resumo::area_abrir() const
{
    const wxSize s = GetClientSize();
    return wxRect(s.x - FromDIP(76), FromDIP(14), FromDIP(68), FromDIP(30));
}

wxRect Resumo::area_mais() const
{
    const wxSize s = GetClientSize();
    return wxRect(s.x - FromDIP(36), FromDIP(70), FromDIP(28), FromDIP(28));
}

void Resumo::definir_foto(const wxBitmap& foto)
{
    m_foto = foto;
    Refresh();
}

void Resumo::definir(const wxString& linha1, const wxString& linha2, const std::vector<InfoBobina>& bobinas)
{
    const bool igual = linha1 == m_linha1 && linha2 == m_linha2 && bobinas.size() == m_bobinas.size() &&
                       std::equal(bobinas.begin(), bobinas.end(), m_bobinas.begin(),
                                  [](const InfoBobina& a, const InfoBobina& b) { return a.cor == b.cor && a.tipo == b.tipo; });
    if (igual)
        return;
    m_linha1  = linha1;
    m_linha2  = linha2;
    m_bobinas = bobinas;
    Refresh();
}

void Resumo::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    const wxSize s = GetClientSize();
    // linha da impressora
    const wxRect a(0, 0, s.x, FromDIP(58));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(LILAS));
    gc->DrawRoundedRectangle(a.x, a.y, a.width, a.height, FromDIP(12));
    const int lado = FromDIP(46);
    gc->SetBrush(wxBrush(BRANCO));
    gc->DrawRoundedRectangle(FromDIP(6), FromDIP(6), lado, lado, FromDIP(6));
    if (m_foto.IsOk()) {
        const double esc = std::min(double(lado - FromDIP(4)) / m_foto.GetWidth(), double(lado - FromDIP(4)) / m_foto.GetHeight());
        gc->DrawBitmap(m_foto, FromDIP(6) + (lado - m_foto.GetWidth() * esc) / 2., FromDIP(6) + (lado - m_foto.GetHeight() * esc) / 2.,
                       m_foto.GetWidth() * esc, m_foto.GetHeight() * esc);
    }
    const int tx = FromDIP(60), tl = area_abrir().x - tx - FromDIP(6);
    gc->SetFont(fonte(this, 9., true), TINTA);
    gc->DrawText(caber(this, m_linha1, fonte(this, 9., true), tl), tx, FromDIP(11));
    gc->SetFont(fonte(this, 8.5, false), CINZA);
    gc->DrawText(caber(this, m_linha2, fonte(this, 8.5, false), tl), tx, FromDIP(30));
    Botao abrir{wxString::FromUTF8("Abrir"), Icone::seta_baixo};
    desenhar_botao(*gc, this, area_abrir(), abrir, m_sobre == 1, false);
    // linha dos filamentos
    const wxRect b(0, FromDIP(64), s.x, FromDIP(40));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(CARTAO));
    gc->DrawRoundedRectangle(b.x, b.y, b.width, b.height, FromDIP(12));
    gc->SetFont(fonte(this, 8.5, true), TEXTO_CLARO);
    gc->DrawText(wxString::FromUTF8("Filamentos"), FromDIP(10), b.y + (b.height - FromDIP(14)) / 2.);
    double x = FromDIP(84);
    const double ch = FromDIP(24), lim = area_mais().x - FromDIP(6);
    for (const InfoBobina& f : m_bobinas) {
        const wxString t  = f.numero + " " + f.tipo;
        gc->SetFont(fonte(this, 7.5, true), TEXTO_CLARO);
        const double   cw = FromDIP(26) + largura_texto(*gc, t);
        if (x + cw > lim)
            break;
        const double y = b.y + (b.height - ch) / 2.;
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(CARTAO_BORDA));
        gc->DrawRoundedRectangle(x, y, cw, ch, ch / 2.);
        gc->SetBrush(wxBrush(f.cor));
        gc->DrawEllipse(x + FromDIP(3), y + FromDIP(3), ch - FromDIP(6), ch - FromDIP(6));
        gc->SetBrush(wxBrush(wxColour(0x14, 0x14, 0x16)));
        gc->DrawEllipse(x + ch / 2. - FromDIP(3), y + ch / 2. - FromDIP(3), FromDIP(6), FromDIP(6));
        gc->DrawText(t, x + ch + FromDIP(1), y + (ch - FromDIP(13)) / 2.);
        x += cw + FromDIP(5);
    }
    const wxRect m = area_mais();
    gc->SetPen(wxPen(m_sobre == 2 ? MAGENTA : wxColour(0x8A, 0x85, 0x90), FromDIP(1), wxPENSTYLE_SHORT_DASH));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRoundedRectangle(m.x, m.y, m.width, m.height, FromDIP(8));
    desenhar_icone(*gc, this, Icone::mais, wxRect2DDouble(m.x + FromDIP(7), m.y + FromDIP(7), FromDIP(14), FromDIP(14)), TEXTO_CLARO, false);
}

// ------------------------------------------------------------------------------------------------ divisor
Divisor::Divisor(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(14)));
    SetCursor(wxCursor(wxCURSOR_SIZENS));
    SetToolTip(wxString::FromUTF8("Arraste para dar mais espaço ao Processo"));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
        m_inicio_y   = ClientToScreen(e.GetPosition()).y;
        m_arrastando = true;
        CaptureMouse();
        if (ao_comecar)
            ao_comecar();
    });
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        if (m_arrastando && ao_arrastar)
            ao_arrastar(ClientToScreen(e.GetPosition()).y - m_inicio_y);
    });
    auto soltar = [this]() {
        if (!m_arrastando)
            return;
        m_arrastando = false;
        if (HasCapture())
            ReleaseMouse();
        if (ao_soltar)
            ao_soltar();
    };
    Bind(wxEVT_LEFT_UP, [soltar](wxMouseEvent&) { soltar(); });
    Bind(wxEVT_MOUSE_CAPTURE_LOST, [soltar](wxMouseCaptureLostEvent&) { soltar(); });
}

void Divisor::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(AREIA));
    dc.Clear();
    const wxSize s = GetClientSize();
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(AREIA_ESCURA));
    const int w = FromDIP(36), x = (s.x - w) / 2, y = s.y / 2 - FromDIP(3);
    dc.DrawRoundedRectangle(x, y, w, FromDIP(2), 1);
    dc.DrawRoundedRectangle(x, y + FromDIP(4), w, FromDIP(2), 1);
}

// ------------------------------------------------------------------------------------------------ abas em grade
GradeAbas::GradeAbas(wxWindow* pai, TabCtrl* abas)
    : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE), m_abas(abas)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(36)));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
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
        if (i < 0 || !m_abas)
            return;
        m_abas->SelectItem(m_itens[i].indice);
        conferir();
    });
    // As abas mudam com o modo (simples, avançado) e com a seleção feita por outros caminhos (busca, Ctrl+F).
    m_timer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { conferir(); });
    m_timer.Start(400);
    conferir();
}

void GradeAbas::conferir()
{
    if (!m_abas || !IsShownOnScreen())
        return;
    std::vector<Item> itens;
    for (int i = m_abas->GetFirstVisibleItem(); i >= 0; i = m_abas->GetNextVisible(i))
        itens.push_back({i, m_abas->GetItemText(i), m_abas->GetItemTextColour(i)});
    const int sel = m_abas->GetSelection();
    if (itens == m_itens && sel == m_sel)
        return;
    const bool mudou_qtd = itens.size() != m_itens.size();
    m_itens              = std::move(itens);
    m_sel                = sel;
    if (mudou_qtd) {
        const int linhas = std::max(1, int(m_itens.size() + 2) / 3);
        SetMinSize(wxSize(-1, linhas * FromDIP(36)));
        if (GetParent())
            GetParent()->Layout();
    }
    Refresh();
}

wxRect GradeAbas::area(int i) const
{
    const int gap = FromDIP(4), w = (GetClientSize().x - 2 * gap) / 3;
    return wxRect((i % 3) * (w + gap), (i / 3) * FromDIP(36), w, FromDIP(32));
}

int GradeAbas::sob(const wxPoint& p) const
{
    for (int i = 0; i < int(m_itens.size()); ++i)
        if (area(i).Contains(p))
            return i;
    return -1;
}

void GradeAbas::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    for (int i = 0; i < int(m_itens.size()); ++i) {
        const wxRect r   = area(i);
        const Item&  it  = m_itens[i];
        const bool   sel = it.indice == m_sel, s = i == m_sobre;
        // o Orca pinta de cinza o nome da página sem mudança (fora de seleção); outra cor = página com ajuste mudado
        const bool mudada = it.cor.IsOk() && (std::abs(it.cor.Red() - 0x6B) > 8 || std::abs(it.cor.Green() - 0x6B) > 8 || std::abs(it.cor.Blue() - 0x6C) > 8);
        gc->SetPen(s && !sel ? wxPen(MAGENTA, FromDIP(1)) : *wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(sel ? MAGENTA : AREIA));
        gc->DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, FromDIP(8));
        texto_centro(*gc, this, it.nome, fonte(this, 9., true), sel ? BRANCO : mudada ? it.cor : TINTA, r.x + r.width / 2., r.y + (r.height - FromDIP(15)) / 2.,
                     r.width - FromDIP(12));
        if (mudada && sel) { // selecionada e mudada: o ponto na cor do Orca
            gc->SetPen(wxPen(BRANCO, 1));
            gc->SetBrush(wxBrush(it.cor));
            gc->DrawEllipse(r.x + FromDIP(6), r.y + r.height / 2. - FromDIP(4), FromDIP(8), FromDIP(8));
        }
    }
}

}}} // namespace Slic3r::GUI::FilunaLateral
