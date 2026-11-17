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
    const bool     baixo   = !coluna && r.height <= w->FromDIP(28); // botões baixos: letra e ícone menores, para o nome caber
    const double   li      = w->FromDIP(coluna ? 18 : baixo ? 13 : 15);
    const wxFont   f       = fonte(w, coluna || baixo ? 7.5 : 8.5, true);
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
    const double gap = tem_ic && tw > 0 ? w->FromDIP(baixo ? 4 : 5) : 0.;
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

// ------------------------------------------------------------------------------------------------ cartão da impressora
CartaoImpressora::CartaoImpressora(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(FromDIP(220), FromDIP(98)));
    SetToolTip(wxString::FromUTF8("Selecionar impressora"));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
    auto ativo = [this](int i) { return i == BICO ? m_bico_sel && m_mostrar_bico : i == MESA ? m_mesa_sel : true; };
    auto mudar = [this](int s) {
        if (s == m_sobre)
            return;
        if ((s == MESA) != (m_sobre == MESA) && ao_mesa_sobre)
            ao_mesa_sobre(s == MESA, wxRect(ClientToScreen(area(MESA).GetTopLeft()), area(MESA).GetSize()));
        m_sobre = s;
        cursor_mao(this, s != NADA);
        Refresh();
    };
    // o lápis fica por cima da faixa: é procurado primeiro
    auto sob = [this, ativo](const wxPoint& p) {
        for (int i : {LAPIS, FAIXA, BICO, TIPO, MESA})
            if (ativo(i) && area(i).Contains(p))
                return i;
        return int(NADA);
    };
    Bind(wxEVT_MOTION, [sob, mudar](wxMouseEvent& e) { mudar(sob(e.GetPosition())); });
    Bind(wxEVT_LEAVE_WINDOW, [mudar](wxMouseEvent&) { mudar(NADA); });
    Bind(wxEVT_LEFT_UP, [this, sob, mudar](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i == NADA || !ao_area)
            return;
        if (i == MESA)
            mudar(NADA); // a lista da mesa abre: a imagem grande sai
        ao_area(i, wxRect(ClientToScreen(area(i).GetTopLeft()), area(i).GetSize()));
    });
}

// Faixa magenta em cima (com a foto e o lápis) e, embaixo, bico, tipo de bico e mesa numa linha.
wxRect CartaoImpressora::area(int i) const
{
    const wxSize s = GetClientSize();
    const int    pad = FromDIP(6), gap = FromDIP(5), faixa_h = FromDIP(44), linha_y = pad + faixa_h + FromDIP(6), linha_h = FromDIP(36);
    const int    w = s.x - 2 * pad;
    switch (i) {
    case FAIXA: return wxRect(pad, pad, w, faixa_h);
    case LAPIS: return wxRect(pad + FromDIP(30), pad + FromDIP(26), FromDIP(16), FromDIP(16));
    default: break;
    }
    // larguras na proporção do canvas: bico 0,85, tipo 1,1, mesa 1,25 (sem bico: tipo 1, mesa 1,3)
    const double pb = m_mostrar_bico ? 0.85 : 0., pt = m_mostrar_bico ? 1.1 : 1., pm = m_mostrar_bico ? 1.25 : 1.3;
    const int    n = m_mostrar_bico ? 3 : 2, livre = w - (n - 1) * gap;
    const int    wb = int(livre * pb / (pb + pt + pm)), wt = int(livre * pt / (pb + pt + pm)), wm = livre - wb - wt;
    int          x = pad;
    if (m_mostrar_bico) {
        if (i == BICO)
            return wxRect(x, linha_y, wb, linha_h);
        x += wb + gap;
    }
    if (i == TIPO)
        return wxRect(x, linha_y, wt, linha_h);
    x += wt + gap;
    return wxRect(x, linha_y, wm, linha_h);
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

void CartaoImpressora::definir(const Info& info)
{
    if (info == m_info)
        return;
    m_info         = info;
    m_mostrar_bico = info.mostrar_bico;
    m_bico_sel     = info.bico_selecionavel;
    m_mesa_sel     = info.mesa_selecionavel;
    Refresh();
}

// Seletor em pílula: rótulo pequeno em cima e o valor embaixo, dentro da caixa.
static void pilula(wxGraphicsContext& gc, wxWindow* w, const wxRect& r, const wxString& rotulo, const wxString& valor, bool ativo, bool sobre,
                   const wxColour& borda, const wxColour& fundo, const wxBitmap& imagem)
{
    gc.SetPen(wxPen(sobre && ativo ? MAGENTA : borda, w->FromDIP(1)));
    gc.SetBrush(wxBrush(fundo));
    gc.DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, w->FromDIP(8));
    int x = r.x + w->FromDIP(7);
    if (imagem.IsOk()) {
        const double lado = w->FromDIP(22);
        const double esc  = std::min(lado / imagem.GetWidth(), lado / imagem.GetHeight());
        gc.DrawBitmap(imagem, x - w->FromDIP(2), r.y + (r.height - imagem.GetHeight() * esc) / 2., imagem.GetWidth() * esc, imagem.GetHeight() * esc);
        x += int(lado) + w->FromDIP(3);
    }
    const int larg = r.GetRight() - x - w->FromDIP(4);
    gc.SetFont(fonte(w, 6.5, true), CINZA);
    gc.DrawText(caber(w, rotulo.Upper(), fonte(w, 6.5, true), larg), x, r.y + w->FromDIP(4));
    const wxString v = ativo ? valor + wxString::FromUTF8(" ▾") : valor;
    gc.SetFont(fonte(w, 8.5, true), ativo ? TINTA : CINZA);
    gc.DrawText(caber(w, v, fonte(w, 8.5, true), larg), x, r.y + w->FromDIP(16));
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
    gc->DrawRoundedRectangle(0, 0, s.x, s.y, FromDIP(14));
    // faixa magenta com a foto
    const wxRect f = area(FAIXA);
    gc->SetBrush(wxBrush(m_sobre == FAIXA ? MAGENTA_ESCURO : MAGENTA));
    gc->DrawRoundedRectangle(f.x, f.y, f.width, f.height, FromDIP(10));
    const int lado = FromDIP(36), fx = f.x + FromDIP(4), fy = f.y + FromDIP(4);
    gc->SetBrush(wxBrush(BRANCO));
    gc->DrawRoundedRectangle(fx, fy, lado, lado, FromDIP(8));
    if (m_foto.IsOk()) {
        const double esc = std::min(double(lado - FromDIP(2)) / m_foto.GetWidth(), double(lado - FromDIP(2)) / m_foto.GetHeight());
        gc->DrawBitmap(m_foto, fx + (lado - m_foto.GetWidth() * esc) / 2., fy + (lado - m_foto.GetHeight() * esc) / 2., m_foto.GetWidth() * esc,
                       m_foto.GetHeight() * esc);
    }
    gc->SetFont(fonte(this, 9.5, true), BRANCO);
    double tw = 0, th = 0;
    gc->GetTextExtent(wxString::FromUTF8("SELECIONAR IMPRESSORA"), &tw, &th);
    gc->DrawText(wxString::FromUTF8("SELECIONAR IMPRESSORA"), fx + lado + FromDIP(10), f.y + (f.height - th) / 2.);
    seta(*gc, f.GetRight() - FromDIP(16), f.y + f.height / 2., (FromDIP(9) / 2.), BRANCO, FromDIP(2));
    // lápis no canto da foto: editar o perfil da impressora
    const wxRect l = area(LAPIS);
    gc->SetPen(wxPen(m_sobre == LAPIS ? MAGENTA : LILAS_BORDA, FromDIP(1)));
    gc->SetBrush(wxBrush(BRANCO));
    gc->DrawEllipse(l.x, l.y, l.width, l.height);
    desenhar_icone(*gc, this, Icone::lapis, wxRect2DDouble(l.x + FromDIP(3), l.y + FromDIP(3), l.width - FromDIP(6), l.height - FromDIP(6)),
                   m_sobre == LAPIS ? MAGENTA : CINZA, false);
    // bico, tipo de bico e mesa
    if (m_mostrar_bico)
        pilula(*gc, this, area(BICO), wxString::FromUTF8("Bico"), m_info.bico, m_bico_sel, m_sobre == BICO, LILAS_BORDA, BRANCO, wxBitmap());
    pilula(*gc, this, area(TIPO), wxString::FromUTF8("Tipo de bico"), m_info.tipo, true, m_sobre == TIPO, LILAS_BORDA, BRANCO, wxBitmap());
    pilula(*gc, this, area(MESA), wxString::FromUTF8("Mesa"), m_info.mesa, m_mesa_sel, m_sobre == MESA, ROSA_BORDA, ROSA, m_foto_mesa);
}

// ------------------------------------------------------------------------------------------------ bobinas
Bobinas::Bobinas(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(ALTURA)));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
        rolar(0);
        e.Skip();
    });
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i != m_sobre) {
            m_sobre = i;
            cursor_mao(this, i != -1);
            Refresh();
        }
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = -1; cursor_mao(this, false); Refresh(); });
    // a fileira rola de lado: roda do mouse, ou as setas nas pontas quando não cabe tudo
    Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent& e) { rolar(e.GetWheelRotation() > 0 ? -passo() : passo()); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i == SETA_ESQ)
            rolar(-passo());
        else if (i == SETA_DIR)
            rolar(passo());
        else if (i == int(m_bobinas.size()) && ao_adicionar)
            ao_adicionar();
        else if (i >= 0 && i < int(m_bobinas.size()) && ao_clicar)
            ao_clicar(i);
    });
    Bind(wxEVT_RIGHT_UP, [this](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i >= 0 && i < int(m_bobinas.size()) && ao_menu)
            ao_menu(i, ClientToScreen(e.GetPosition()));
    });
}

int Bobinas::passo() const { return FromDIP(LARGURA) + FromDIP(6); }

int Bobinas::largura_total() const { return int(m_bobinas.size()) * passo() + FromDIP(34); }

void Bobinas::rolar(int dx)
{
    const int max = std::max(0, largura_total() - GetClientSize().x);
    const int novo = std::clamp(m_desloc + dx, 0, max);
    if (novo == m_desloc && dx != 0)
        return;
    m_desloc = novo;
    Refresh();
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
    const bool nova_sel = selecionada != m_sel;
    m_bobinas           = bobinas;
    m_sel               = selecionada;
    if (nova_sel && m_sel >= 0) { // a selecionada fica à vista
        const wxRect r = cartao(m_sel);
        if (r.x < 0)
            m_desloc += r.x;
        else if (r.GetRight() > GetClientSize().x)
            m_desloc += r.GetRight() - GetClientSize().x + FromDIP(2);
    }
    rolar(0);
    Refresh();
}

wxRect Bobinas::cartao(int i) const
{
    const int w = i == int(m_bobinas.size()) ? FromDIP(34) : FromDIP(LARGURA);
    return wxRect(i * passo() - m_desloc, 0, w, FromDIP(ALTURA));
}

int Bobinas::sob(const wxPoint& p) const
{
    const bool cabe = largura_total() <= GetClientSize().x;
    if (!cabe && m_desloc > 0 && p.x < FromDIP(18))
        return SETA_ESQ;
    if (!cabe && m_desloc < largura_total() - GetClientSize().x && p.x > GetClientSize().x - FromDIP(18))
        return SETA_DIR;
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
    const int larg_bobina = FromDIP(42);
    if (m_desenhos.size() != m_bobinas.size()) {
        m_desenhos.clear();
        for (const InfoBobina& b : m_bobinas)
            m_desenhos.push_back(bobina(b.cor, larg_bobina));
    }
    const wxFont f_marca = fonte(this, 6.5, true), f_tipo = fonte(this, 8., true), f_estoque = fonte(this, 6.5, false), f_num = fonte(this, 6.5, true);
    for (int i = 0; i < int(m_bobinas.size()); ++i) {
        const wxRect r = cartao(i);
        if (r.GetRight() < 0 || r.x > GetClientSize().x)
            continue;
        const InfoBobina& b   = m_bobinas[i];
        const bool        sel = i == m_sel, s = i == m_sobre;
        gc->SetPen(wxPen(sel || s ? MAGENTA : CARTAO_BORDA, FromDIP(sel ? 2 : 1)));
        gc->SetBrush(wxBrush(sel || s ? CARTAO_SOBRE : CARTAO));
        gc->DrawRoundedRectangle(r.x + 1, r.y + 1, r.width - 2, r.height - 2, FromDIP(10));
        const wxBitmap& d  = m_desenhos[i];
        const double    bx = r.x + (r.width - d.GetWidth()) / 2., by = r.y + FromDIP(5);
        gc->DrawBitmap(d, bx, by, d.GetWidth(), d.GetHeight());
        const double nd = FromDIP(16);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(sel ? MAGENTA : TEXTO_CLARO));
        gc->DrawEllipse(r.x + FromDIP(3), r.y + FromDIP(3), nd, nd);
        gc->SetFont(f_num, sel ? BRANCO : TINTA);
        double tw = 0, th = 0;
        gc->GetTextExtent(b.numero, &tw, &th);
        gc->DrawText(b.numero, r.x + FromDIP(3) + (nd - tw) / 2., r.y + FromDIP(3) + (nd - th) / 2.);
        const double cx = r.x + r.width / 2.;
        double       y  = by + d.GetHeight();
        const int    lt = r.width - FromDIP(6);
        texto_centro(*gc, this, b.marca.empty() ? wxString::FromUTF8("Genérico") : b.marca, f_marca, TEXTO_CLARO, cx, y, lt);
        y += FromDIP(11);
        texto_centro(*gc, this, b.tipo, f_tipo, sel ? MAGENTA_CLARO : TEXTO_CLARO, cx, y, lt);
        y += FromDIP(13);
        texto_centro(*gc, this, b.estoque, f_estoque, TEXTO_APAGADO, cx, y, lt);
    }
    // "+" tracejado no fim da fileira
    const wxRect r = cartao(int(m_bobinas.size()));
    const bool   s = m_sobre == int(m_bobinas.size());
    gc->SetPen(wxPen(s ? MAGENTA : AREIA_ESCURA, FromDIP(1), wxPENSTYLE_SHORT_DASH));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRoundedRectangle(r.x + 1, r.y + 1, r.width - 2, r.height - 2, FromDIP(10));
    const double li = FromDIP(18);
    desenhar_icone(*gc, this, Icone::mais, wxRect2DDouble(r.x + (r.width - li) / 2., r.y + (r.height - li) / 2., li, li), MAGENTA, false);
    // setas nas pontas quando a fileira não cabe
    auto ponta = [&](bool esquerda) {
        const double cx = esquerda ? FromDIP(10) : GetClientSize().x - FromDIP(10), cy = FromDIP(ALTURA) / 2.;
        gc->SetPen(wxPen(AREIA_BORDA, 1));
        gc->SetBrush(wxBrush(BRANCO));
        gc->DrawEllipse(cx - FromDIP(9), cy - FromDIP(9), FromDIP(18), FromDIP(18));
        gc->SetPen(wxPen(TINTA, FromDIP(2)));
        const double d = esquerda ? 1. : -1., t = (FromDIP(7) / 2.);
        wxGraphicsPath p = gc->CreatePath();
        p.MoveToPoint(cx + d * t * 0.5, cy - t);
        p.AddLineToPoint(cx - d * t * 0.5, cy);
        p.AddLineToPoint(cx + d * t * 0.5, cy + t);
        gc->StrokePath(p);
    };
    if (largura_total() > GetClientSize().x) {
        if (m_desloc > 0)
            ponta(true);
        if (m_desloc < largura_total() - GetClientSize().x)
            ponta(false);
    }
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
    SetMinSize(wxSize(-1, FromDIP(32)));
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
    const int    gap = FromDIP(4), h = FromDIP(24), y = (s.y - h) / 2;
    int          x   = s.x - FromDIP(4);
    for (int k = int(b.size()) - 1; k >= 0; --k) {
        const int w = b[k].texto.empty() ? FromDIP(24) : largura_botao(const_cast<BarraBobina*>(this), b[k]) - FromDIP(4);
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
    const auto b = botoes();
    const wxRect tudo(0, 0, s.x, s.y);
    gc->SetFont(fonte(this, 8., true), VINHO);
    const double num_l = largura_texto(*gc, m_entrada);
    gc->DrawText(m_entrada, FromDIP(10), (s.y - FromDIP(13)) / 2.);
    const int x = FromDIP(10) + int(num_l) + FromDIP(6);
    texto_meio(*gc, this, m_nome, fonte(this, 8.5, true), TINTA, x, tudo, area(0).x - x - FromDIP(6));
    for (int i = 0; i < 4; ++i)
        desenhar_botao(*gc, this, area(i), b[i], i == m_sobre, false);
}

// ------------------------------------------------------------------------------------------------ resumo
Resumo::Resumo(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(48)));
    SetToolTip(wxString::FromUTF8("Abrir impressora e filamentos"));
    SetCursor(wxCursor(wxCURSOR_HAND));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { m_sobre = true; Refresh(); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = false; Refresh(); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) { if (ao_abrir) ao_abrir(); });
}

void Resumo::definir_foto(const wxBitmap& foto)
{
    m_foto = foto;
    Refresh();
}

void Resumo::definir(const wxString& linha, const std::vector<InfoBobina>& bobinas, int selecionada)
{
    const bool igual = linha == m_linha && selecionada == m_sel && bobinas.size() == m_bobinas.size() &&
                       std::equal(bobinas.begin(), bobinas.end(), m_bobinas.begin(), [](const InfoBobina& a, const InfoBobina& b) { return a.cor == b.cor; });
    if (igual)
        return;
    m_linha   = linha;
    m_bobinas = bobinas;
    m_sel     = selecionada;
    Refresh();
}

void Resumo::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    const wxSize s = GetClientSize();
    gc->SetPen(m_sobre ? wxPen(MAGENTA, FromDIP(1)) : *wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(LILAS));
    gc->DrawRoundedRectangle(0.5, 0.5, s.x - 1, s.y - 1, FromDIP(12));
    const int lado = FromDIP(40), fx = FromDIP(4), fy = (s.y - lado) / 2;
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(BRANCO));
    gc->DrawRoundedRectangle(fx, fy, lado, lado, FromDIP(9));
    if (m_foto.IsOk()) {
        const double esc = std::min(double(lado - FromDIP(2)) / m_foto.GetWidth(), double(lado - FromDIP(2)) / m_foto.GetHeight());
        gc->DrawBitmap(m_foto, fx + (lado - m_foto.GetWidth() * esc) / 2., fy + (lado - m_foto.GetHeight() * esc) / 2., m_foto.GetWidth() * esc,
                       m_foto.GetHeight() * esc);
    }
    // "Abrir" à direita
    gc->SetFont(fonte(this, 8., true), BRANCO);
    const wxString abrir = wxString::FromUTF8("Abrir");
    const double   aw    = largura_texto(*gc, abrir) + FromDIP(30);
    const wxRect   a(int(s.x - aw - FromDIP(8)), (s.y - FromDIP(26)) / 2, int(aw), FromDIP(26));
    gc->SetBrush(wxBrush(m_sobre ? MAGENTA_ESCURO : MAGENTA));
    gc->DrawRoundedRectangle(a.x, a.y, a.width, a.height, a.height / 2.);
    seta(*gc, a.x + FromDIP(12), a.y + a.height / 2., (FromDIP(7) / 2.), BRANCO, FromDIP(2));
    gc->DrawText(abrir, a.x + FromDIP(20), a.y + (a.height - FromDIP(13)) / 2.);
    // linha do bico e da mesa; embaixo, as bolinhas dos filamentos
    const int tx = fx + lado + FromDIP(8), tl = a.x - tx - FromDIP(6);
    gc->SetFont(fonte(this, 8.5, true), TINTA);
    gc->DrawText(caber(this, m_linha, fonte(this, 8.5, true), tl), tx, FromDIP(6));
    double x = tx;
    const double ch = FromDIP(16), cy = FromDIP(26);
    for (int i = 0; i < int(m_bobinas.size()); ++i) {
        gc->SetFont(fonte(this, 6.5, true), TEXTO_CLARO);
        const double cw = ch + FromDIP(4) + largura_texto(*gc, m_bobinas[i].numero);
        if (x + cw > tx + tl)
            break;
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(i == m_sel ? VINHO : CARTAO));
        gc->DrawRoundedRectangle(x, cy, cw, ch, ch / 2.);
        gc->SetBrush(wxBrush(m_bobinas[i].cor));
        gc->DrawEllipse(x + FromDIP(2), cy + FromDIP(2), ch - FromDIP(4), ch - FromDIP(4));
        gc->SetBrush(wxBrush(wxColour(0x14, 0x14, 0x16)));
        gc->DrawEllipse(x + ch / 2. - FromDIP(2), cy + ch / 2. - FromDIP(2), FromDIP(4), FromDIP(4));
        gc->DrawText(m_bobinas[i].numero, x + ch, cy + (ch - FromDIP(10)) / 2.);
        x += cw + FromDIP(4);
    }
}

// ------------------------------------------------------------------------------------------------ abas em grade
GradeAbas::GradeAbas(wxWindow* pai, TabCtrl* abas)
    : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE), m_abas(abas)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(28)));
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
        SetMinSize(wxSize(-1, linhas * FromDIP(28)));
        if (GetParent())
            GetParent()->Layout();
    }
    Refresh();
}

wxRect GradeAbas::area(int i) const
{
    const int gap = FromDIP(4), w = (GetClientSize().x - 2 * gap) / 3;
    return wxRect((i % 3) * (w + gap), (i / 3) * FromDIP(28), w, FromDIP(25));
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
        texto_centro(*gc, this, it.nome, fonte(this, 8., true), sel ? BRANCO : mudada ? it.cor : TINTA, r.x + r.width / 2., r.y + (r.height - FromDIP(13)) / 2.,
                     r.width - FromDIP(12));
        if (mudada && sel) { // selecionada e mudada: o ponto na cor do Orca
            gc->SetPen(wxPen(BRANCO, 1));
            gc->SetBrush(wxBrush(it.cor));
            gc->DrawEllipse(r.x + FromDIP(6), r.y + r.height / 2. - FromDIP(4), FromDIP(8), FromDIP(8));
        }
    }
}

// ------------------------------------------------------------------------------------------------ segmentos
Segmentos::Segmentos(wxWindow* pai, const std::vector<wxString>& opcoes)
    : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE), m_opcoes(opcoes)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    wxClientDC dc(this);
    dc.SetFont(fonte(this, 7.5, true));
    int w = FromDIP(4);
    for (const wxString& o : m_opcoes)
        w += dc.GetTextExtent(o).x + FromDIP(12);
    SetMinSize(wxSize(w, FromDIP(24)));
    Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i != m_sobre) {
            m_sobre = i;
            cursor_mao(this, i >= 0 && m_ativo);
            Refresh();
        }
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = -1; cursor_mao(this, false); Refresh(); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
        const int i = sob(e.GetPosition());
        if (i >= 0 && m_ativo && i != m_sel && ao_escolher)
            ao_escolher(i);
    });
}

void Segmentos::selecionar(int i, bool ativo)
{
    if (i == m_sel && ativo == m_ativo)
        return;
    m_sel   = i;
    m_ativo = ativo;
    Refresh();
}

wxRect Segmentos::area(int i) const
{
    wxClientDC dc(const_cast<Segmentos*>(this));
    dc.SetFont(fonte(const_cast<Segmentos*>(this), 7.5, true));
    int x = FromDIP(2);
    for (int k = 0; k < i; ++k)
        x += dc.GetTextExtent(m_opcoes[k]).x + FromDIP(12);
    return wxRect(x, FromDIP(2), dc.GetTextExtent(m_opcoes[i]).x + FromDIP(12), GetClientSize().y - FromDIP(4));
}

int Segmentos::sob(const wxPoint& p) const
{
    for (int i = 0; i < int(m_opcoes.size()); ++i)
        if (area(i).Contains(p))
            return i;
    return -1;
}

void Segmentos::pintar()
{
    wxAutoBufferedPaintDC dc(this);
    auto gc = comecar(this, dc);
    if (!gc)
        return;
    const wxSize s = GetClientSize();
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(wxColour(0xEF, 0xE9, 0xE1)));
    gc->DrawRoundedRectangle(0, 0, s.x, s.y, s.y / 2.);
    for (int i = 0; i < int(m_opcoes.size()); ++i) {
        const wxRect r   = area(i);
        const bool   sel = i == m_sel;
        if (sel || i == m_sobre) {
            gc->SetBrush(wxBrush(sel ? (m_ativo ? MAGENTA : AREIA_ESCURA) : BRANCO));
            gc->DrawRoundedRectangle(r.x, r.y, r.width, r.height, r.height / 2.);
        }
        texto_centro(*gc, this, m_opcoes[i], fonte(this, 7.5, true), sel ? BRANCO : wxColour(0x4A, 0x41, 0x3A), r.x + r.width / 2.,
                     r.y + (r.height - FromDIP(12)) / 2., r.width);
    }
}

}}} // namespace Slic3r::GUI::FilunaLateral
