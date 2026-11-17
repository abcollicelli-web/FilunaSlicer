#include "FilunaEnvio.hpp"

#include "FilunaLateral.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/StateColor.hpp"
#include "FilunaDispositivos.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "Plater.hpp"
#include "I18N.hpp"
#include "libslic3r/AppConfig.hpp"

#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/weakref.h>

#include <algorithm>
#include <vector>
#include <cmath>

namespace Slic3r { namespace GUI {

using namespace FilunaLateral::pintura;

namespace {

constexpr int LARGURA = 680, ALTURA = 400, FAIXA = 56, RODAPE = 64;
constexpr int COLUNA_L = 300; // a coluna dos passos, à esquerda

// A cor de uma peça que esquenta: cinza parado, depois laranja, depois vermelho. `t` vai de 0 a 1.
wxColour cor_quente(double t)
{
    t = std::max(0., std::min(1., t));
    const wxColour frio(0x9E, 0x9A, 0xA2), morno(0xE8, 0x8C, 0x30), quente(0xD8, 0x27, 0x27);
    const wxColour a = t < 0.5 ? frio : morno, b = t < 0.5 ? morno : quente;
    const double   u = t < 0.5 ? t / 0.5 : (t - 0.5) / 0.5;
    return wxColour(uint8_t(a.Red() + u * (b.Red() - a.Red())), uint8_t(a.Green() + u * (b.Green() - a.Green())),
                    uint8_t(a.Blue() + u * (b.Blue() - a.Blue())));
}

wxString graus(double v) { return wxString::Format("%d", int(std::lround(v))) + wxString::FromUTF8("\xC2\xB0"); }

} // namespace

// ================================================================================ o desenho da impressora

class JanelaEnvio::Desenho : public wxPanel
{
public:
    explicit Desenho(wxWindow* pai) : wxPanel(pai, wxID_ANY)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &Desenho::ao_pintar, this);
    }

    // temperaturas de verdade, lidas da impressora
    void temperaturas(double bico, double bico_alvo, double mesa, double mesa_alvo)
    {
        m_bico = bico, m_bico_alvo = bico_alvo, m_mesa = mesa, m_mesa_alvo = mesa_alvo;
        Refresh();
    }
    void animar() { m_fase += 0.08; Refresh(); }
    void imprimindo(bool v) { m_imprimindo = v; Refresh(); }

private:
    void ao_pintar(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        auto                  gcp = comecar(this, dc);
        if (!gcp) return;
        wxGraphicsContext& gc = *gcp;

        const wxSize s = GetClientSize();
        const double cx = s.x / 2.;

        // --- o corpo da impressora: um pórtico simples, no tom areia do Filuna
        const double larg = std::min<double>(s.x - FromDIP(40), FromDIP(210)), alt = std::min<double>(s.y - FromDIP(36), FromDIP(220));
        const double x0 = cx - larg / 2., y0 = (s.y - alt) / 2.;

        gc.SetBrush(wxBrush(AREIA));
        gc.SetPen(wxPen(AREIA_BORDA, 2));
        gc.DrawRoundedRectangle(x0, y0, larg, alt, FromDIP(14));

        // --- a barra de cima, por onde o bico corre
        const double barra_y = y0 + alt * 0.22;
        gc.SetBrush(wxBrush(AREIA_ESCURA));
        gc.SetPen(*wxTRANSPARENT_PEN);
        gc.DrawRoundedRectangle(x0 + FromDIP(14), barra_y, larg - FromDIP(28), FromDIP(8), FromDIP(4));

        // --- o bico, que anda devagar enquanto espera e esquenta de cor
        const double passeio = m_imprimindo ? std::sin(m_fase) * (larg / 2. - FromDIP(46)) : std::sin(m_fase * 0.4) * FromDIP(18);
        const double bx = cx + passeio, by = barra_y + FromDIP(8);
        const double t_bico = m_bico_alvo > 1. ? m_bico / m_bico_alvo : (m_bico > 35. ? std::min(1., (m_bico - 25.) / 180.) : 0.);
        const wxColour c_bico = cor_quente(t_bico);

        gc.SetBrush(wxBrush(wxColour(0x5E, 0x57, 0x63)));
        gc.DrawRoundedRectangle(bx - FromDIP(16), by, FromDIP(32), FromDIP(26), FromDIP(5));
        {   // a ponta do bico, em triângulo
            wxGraphicsPath p = gc.CreatePath();
            p.MoveToPoint(bx - FromDIP(9), by + FromDIP(26));
            p.AddLineToPoint(bx + FromDIP(9), by + FromDIP(26));
            p.AddLineToPoint(bx, by + FromDIP(40));
            p.CloseSubpath();
            gc.SetBrush(wxBrush(c_bico));
            gc.FillPath(p);
        }
        // o número da temperatura, logo acima do bico
        etiqueta(gc, graus(m_bico), c_bico, bx, by - FromDIP(22));

        // --- a mesa, que também esquenta
        const double mesa_y = y0 + alt * 0.74, mesa_l = larg - FromDIP(44);
        const double t_mesa = m_mesa_alvo > 1. ? m_mesa / m_mesa_alvo : (m_mesa > 30. ? std::min(1., (m_mesa - 25.) / 75.) : 0.);
        const wxColour c_mesa = cor_quente(t_mesa);
        gc.SetBrush(wxBrush(c_mesa));
        gc.SetPen(*wxTRANSPARENT_PEN);
        gc.DrawRoundedRectangle(cx - mesa_l / 2., mesa_y, mesa_l, FromDIP(10), FromDIP(5));
        gc.SetBrush(wxBrush(AREIA_ESCURA));
        gc.DrawRoundedRectangle(cx - mesa_l / 2. - FromDIP(6), mesa_y + FromDIP(10), mesa_l + FromDIP(12), FromDIP(6), FromDIP(3));
        etiqueta(gc, graus(m_mesa), c_mesa, cx, mesa_y + FromDIP(24));

        // --- enquanto imprime, um fiozinho saindo do bico
        if (m_imprimindo) {
            gc.SetPen(wxPen(MAGENTA, 3));
            gc.StrokeLine(bx, by + FromDIP(40), bx, mesa_y - FromDIP(2));
        }
    }

    void etiqueta(wxGraphicsContext& gc, const wxString& t, const wxColour& cor, double cx, double y)
    {
        const wxFont f = fonte(this, 8.5, true);
        gc.SetFont(f, cor);
        double w = 0, h = 0, d = 0, e = 0;
        gc.GetTextExtent(t, &w, &h, &d, &e);
        const double pad = FromDIP(6);
        gc.SetBrush(wxBrush(BRANCO));
        gc.SetPen(wxPen(AREIA_BORDA, 1));
        gc.DrawRoundedRectangle(cx - w / 2. - pad, y - h / 2. - FromDIP(2), w + pad * 2, h + FromDIP(4), (h + FromDIP(4)) / 2.);
        gc.SetFont(f, cor);
        gc.DrawText(t, cx - w / 2., y - h / 2.);
    }

    double m_bico{0}, m_bico_alvo{0}, m_mesa{0}, m_mesa_alvo{0}, m_fase{0};
    bool   m_imprimindo{false};
};

// ================================================================================ a coluna dos passos

class JanelaEnvio::Coluna : public wxPanel
{
public:
    explicit Coluna(wxWindow* pai) : wxPanel(pai, wxID_ANY)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &Coluna::ao_pintar, this);
    }

    void dizer(const wxString& titulo, const wxString& detalhe, int porcento)
    {
        m_titulo = titulo, m_detalhe = detalhe, m_porcento = porcento;
        Refresh();
    }
    void animar() { m_fase += 0.12; if (m_porcento < 0) Refresh(); }
    void pronto(bool v) { m_pronto = v; Refresh(); }

private:
    void ao_pintar(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        auto                  gcp = comecar(this, dc);
        if (!gcp) return;
        wxGraphicsContext& gc = *gcp;
        const wxSize       s = GetClientSize();
        const double       x = FromDIP(4);

        // a bolinha que gira enquanto trabalha (ou o certo, quando acaba)
        const double raio = FromDIP(9), cy = FromDIP(18);
        if (m_pronto) {
            gc.SetBrush(wxBrush(wxColour(0x2E, 0x9E, 0x5B)));
            gc.SetPen(*wxTRANSPARENT_PEN);
            gc.DrawEllipse(x, cy - raio, raio * 2, raio * 2);
            gc.SetPen(wxPen(BRANCO, 2.2));
            gc.StrokeLine(x + raio * 0.6, cy, x + raio * 0.95, cy + raio * 0.45);
            gc.StrokeLine(x + raio * 0.95, cy + raio * 0.45, x + raio * 1.45, cy - raio * 0.5);
        } else {
            gc.SetPen(wxPen(ROSA_BORDA, 3));
            gc.SetBrush(*wxTRANSPARENT_BRUSH);
            gc.DrawEllipse(x, cy - raio, raio * 2, raio * 2);
            wxGraphicsPath p = gc.CreatePath();
            p.AddArc(x + raio, cy, raio, m_fase, m_fase + 1.8, true);
            gc.SetPen(wxPen(MAGENTA, 3));
            gc.StrokePath(p);
        }

        const double tx = x + raio * 2 + FromDIP(12), largura = s.x - tx - FromDIP(4);
        const wxFont ft = fonte(this, 11, true);
        gc.SetFont(ft, TINTA);

        // O título quebra em até duas linhas; só a segunda é cortada se ainda não couber.
        std::vector<wxString> linhas;
        {
            wxString atual;
            for (const wxString& palavra : wxSplit(m_titulo, ' ')) {
                const wxString tenta = atual.empty() ? palavra : atual + " " + palavra;
                double w = 0, h = 0;
                gc.GetTextExtent(tenta, &w, &h);
                if (w <= largura || atual.empty())
                    atual = tenta;
                else {
                    linhas.push_back(atual);
                    atual = palavra;
                }
            }
            if (!atual.empty()) linhas.push_back(atual);
            if (linhas.size() > 2) {
                wxString resto;
                for (size_t i = 1; i < linhas.size(); ++i) resto += (i > 1 ? " " : "") + linhas[i];
                linhas.resize(2);
                linhas[1] = resto;
            }
            if (linhas.size() == 2) linhas[1] = caber(this, linhas[1], ft, int(largura));
        }
        const double alt_linha = FromDIP(20);
        for (size_t i = 0; i < linhas.size(); ++i)
            gc.DrawText(linhas[i], tx, cy - FromDIP(10) + alt_linha * double(i));
        const double extra = alt_linha * double(std::max<size_t>(1, linhas.size()) - 1);

        if (!m_detalhe.empty()) {
            gc.SetFont(fonte(this, 9, false), CINZA);
            gc.DrawText(caber(this, m_detalhe, fonte(this, 9, false), int(largura)), tx, cy + FromDIP(8) + extra);
        }

        // a barrinha do envio, só quando há porcentagem
        if (m_porcento >= 0) {
            const double by = cy + FromDIP(34) + extra, bl = largura;
            gc.SetBrush(wxBrush(AREIA));
            gc.SetPen(*wxTRANSPARENT_PEN);
            gc.DrawRoundedRectangle(tx, by, bl, FromDIP(6), FromDIP(3));
            gc.SetBrush(wxBrush(MAGENTA));
            gc.DrawRoundedRectangle(tx, by, bl * std::max(0, std::min(100, m_porcento)) / 100., FromDIP(6), FromDIP(3));
        }
    }

    wxString m_titulo{wxString::FromUTF8("Preparando o envio")}, m_detalhe;
    int      m_porcento{-1};
    double   m_fase{0};
    bool     m_pronto{false};
};

// ================================================================================ a janela

JanelaEnvio::JanelaEnvio(wxWindow* pai, const std::string& dev_id, const wxString& impressora, const wxString& arquivo)
    : wxDialog(pai, wxID_ANY, wxString::FromUTF8("Imprimir"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE & ~wxRESIZE_BORDER)
    , m_dev_id(dev_id)
    , m_impressora(impressora)
    , m_arquivo(arquivo)
{
    SetBackgroundColour(BRANCO);

    auto* tudo = new wxBoxSizer(wxVERTICAL);

    // --- faixa magenta do Filuna
    auto* faixa = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, FromDIP(FAIXA)));
    faixa->SetBackgroundColour(MAGENTA);
    {
        auto* fs = new wxBoxSizer(wxVERTICAL);
        auto* t1 = new wxStaticText(faixa, wxID_ANY, wxString::FromUTF8("Enviando para ") + m_impressora);
        t1->SetForegroundColour(BRANCO);
        t1->SetFont(fonte(faixa, 12, true));
        auto* t2 = new wxStaticText(faixa, wxID_ANY, m_arquivo);
        t2->SetForegroundColour(wxColour(0xFF, 0xE3, 0xEE));
        t2->SetFont(fonte(faixa, 9, false));
        fs->AddStretchSpacer();
        fs->Add(t1, 0, wxLEFT | wxRIGHT, FromDIP(18));
        fs->AddSpacer(FromDIP(2));
        fs->Add(t2, 0, wxLEFT | wxRIGHT, FromDIP(18));
        fs->AddStretchSpacer();
        faixa->SetSizer(fs);
    }
    tudo->Add(faixa, 0, wxEXPAND);

    // --- miolo: passos à esquerda, impressora à direita
    auto* miolo = new wxBoxSizer(wxHORIZONTAL);
    m_coluna = new Coluna(this);
    m_coluna->SetMinSize(wxSize(FromDIP(COLUNA_L), -1));
    m_desenho = new Desenho(this);
    miolo->Add(m_coluna, 0, wxEXPAND | wxALL, FromDIP(18));
    miolo->Add(m_desenho, 1, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, FromDIP(12));
    tudo->Add(miolo, 1, wxEXPAND);

    SetSizerAndFit(tudo);
    SetClientSize(FromDIP(LARGURA), FromDIP(ALTURA));
    CentreOnParent();

    m_relogio = new wxTimer(this);
    Bind(wxEVT_TIMER, &JanelaEnvio::ao_relogio, this);
    m_relogio->Start(120);

    // Fechar antes do fim é desistir do envio; depois do fim, é só fechar.
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& e) {
        if (!m_terminou) {
            m_cancelou = true; // ainda enviando: fechar é desistir
            e.Skip();
            return;
        }
        e.Skip();
        Destroy();
    });
}

JanelaEnvio::~JanelaEnvio()
{
    if (m_relogio != nullptr) m_relogio->Stop();
}

void JanelaEnvio::ao_relogio(wxTimerEvent&)
{
    if (m_coluna != nullptr) m_coluna->animar();
    if (m_desenho != nullptr) m_desenho->animar();
    static int conta = 0;
    if (++conta % 8 == 0) ler_temperaturas(); // a cada ~1 s
}

void JanelaEnvio::ler_temperaturas()
{
    if (m_dev_id.empty() || m_desenho == nullptr) return;
    double bico = 0, bico_alvo = 0, mesa = 0, mesa_alvo = 0;
    if (FilunaDispositivos::temperaturas(m_dev_id, bico, bico_alvo, mesa, mesa_alvo))
        m_desenho->temperaturas(bico, bico_alvo, mesa, mesa_alvo);
}

void JanelaEnvio::passo(Passo p, int porcento)
{
    if (m_coluna == nullptr) return;
    switch (p) {
    case Passo::PREPARANDO:
        m_coluna->dizer(wxString::FromUTF8("Preparando o envio"), wxString::FromUTF8("Separando o arquivo da mesa"), -1);
        break;
    case Passo::ENVIANDO:
        m_coluna->dizer(wxString::FromUTF8("Enviando o arquivo"), wxString::FromUTF8("Pela rede local, sem passar por nuvem"), porcento);
        break;
    case Passo::CONFERINDO:
        m_coluna->dizer(wxString::FromUTF8("Conferindo se o arquivo chegou"), wxString::FromUTF8("Comparando o que foi enviado"), -1);
        break;
    case Passo::FALANDO:
        m_coluna->dizer(wxString::FromUTF8("Falando com a impressora"), wxString::FromUTF8("Pedindo o início da impressão"), -1);
        break;
    case Passo::AQUECENDO:
        m_coluna->dizer(wxString::FromUTF8("A impressora está aquecendo"), wxString::FromUTF8("Bico e mesa chegando na temperatura"), -1);
        if (m_desenho != nullptr) m_desenho->imprimindo(true);
        break;
    case Passo::IMPRIMINDO:
        m_coluna->dizer(wxString::FromUTF8("Imprimindo"), wxString::FromUTF8("A impressora começou"), -1);
        if (m_desenho != nullptr) m_desenho->imprimindo(true);
        break;
    }
}

void JanelaEnvio::concluir(bool deu_certo, const wxString& recado)
{
    m_terminou = true;
    if (m_relogio != nullptr) m_relogio->Stop();
    if (m_coluna != nullptr) {
        m_coluna->pronto(deu_certo);
        if (deu_certo)
            m_coluna->dizer(wxString::FromUTF8("Sua impressão foi enviada e já está imprimindo"), m_impressora, -1);
        else
            m_coluna->dizer(wxString::FromUTF8("Não deu para enviar"), recado, -1);
    }
    if (m_desenho != nullptr) m_desenho->imprimindo(deu_certo);
    montar_fim(deu_certo, recado);
}

void JanelaEnvio::montar_fim(bool deu_certo, const wxString& recado)
{
    auto* tudo = GetSizer();
    if (tudo == nullptr) return;

    auto* rodape = new wxPanel(this, wxID_ANY);
    rodape->SetBackgroundColour(ROSA_CLARO);
    auto* rs = new wxBoxSizer(wxVERTICAL);

    if (deu_certo) {
        auto* a = new wxStaticText(rodape, wxID_ANY,
                                   wxString::FromUTF8("Acompanhe pela aba Dispositivo, de qualquer tela, ou deixe esta janela aberta."));
        a->SetFont(fonte(rodape, 9, false));
        a->SetForegroundColour(VINHO);
        rs->Add(a, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(16));

        auto* b = new wxStaticText(rodape, wxID_ANY,
                                   wxString::FromUTF8("Fechando esta tela nenhum arquivo é perdido, e você pode imprimir em outra impressora."));
        b->SetFont(fonte(rodape, 9, false));
        b->SetForegroundColour(CINZA);
        rs->Add(b, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(16));
    } else {
        auto* a = new wxStaticText(rodape, wxID_ANY, recado);
        a->SetFont(fonte(rodape, 9, false));
        a->SetForegroundColour(VINHO);
        rs->Add(a, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(16));
    }

    auto* linha = new wxBoxSizer(wxHORIZONTAL);
    linha->AddStretchSpacer();

    if (deu_certo) {
        auto* ver = new wxButton(rodape, wxID_ANY, wxString::FromUTF8("Acompanhar no Dispositivo"));
        ver->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (wxGetApp().mainframe != nullptr) wxGetApp().mainframe->select_filuna_tab(TAB_ID_MONITOR);
            Close(true);
        });
        linha->Add(ver, 0, wxRIGHT, FromDIP(8));

        auto* novo = new wxButton(rodape, wxID_ANY, wxString::FromUTF8("Abrir um projeto novo"));
        novo->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            m_projeto_novo = true;
            Close(true);
            if (Plater* p = wxGetApp().plater()) wxGetApp().CallAfter([p] { p->new_project(); });
        });
        linha->Add(novo, 0, wxRIGHT, FromDIP(8));
    }

    auto* fechar = new wxButton(rodape, wxID_ANY, wxString::FromUTF8(deu_certo ? "Ficar acompanhando" : "Fechar"));
    // Errado: fecha. Certo: "Ficar acompanhando" apenas some com o botão, e a janela segue mostrando a impressão.
    fechar->Bind(wxEVT_BUTTON, [this, deu_certo, fechar](wxCommandEvent&) {
        if (!deu_certo) {
            Close(true);
            return;
        }
        fechar->Hide();
        Layout();
    });
    linha->Add(fechar, 0);

    rs->Add(linha, 0, wxEXPAND | wxALL, FromDIP(14));
    rodape->SetSizer(rs);
    tudo->Add(rodape, 0, wxEXPAND);

    SetClientSize(FromDIP(LARGURA), FromDIP(ALTURA) + FromDIP(RODAPE) + (deu_certo ? FromDIP(22) : 0));
    Layout();
}

// ==============================================================================================
// A tela de conferir antes de imprimir.
// ==============================================================================================

namespace {

wxColour cor_de(const std::string& hex)
{
    if (hex.size() >= 7 && hex[0] == '#') {
        long r = 0, g = 0, b = 0;
        wxString(hex.substr(1, 2)).ToLong(&r, 16);
        wxString(hex.substr(3, 2)).ToLong(&g, 16);
        wxString(hex.substr(5, 2)).ToLong(&b, 16);
        return wxColour(uint8_t(r), uint8_t(g), uint8_t(b));
    }
    return AREIA_ESCURA;
}

wxString tempo_curto(long long s)
{
    if (s <= 0) return wxString::FromUTF8("—");
    const long long h = s / 3600, m = (s % 3600 + 30) / 60;
    return h > 0 ? wxString::Format("%lldh%02lld", h, m) : wxString::Format("%lldmin", m);
}

// Um quadradinho de cor, como o do estoque do Filuna.
wxPanel* amostra(wxWindow* pai, const wxColour& c)
{
    auto* p = new wxPanel(pai, wxID_ANY, wxDefaultPosition, pai->FromDIP(wxSize(16, 16)));
    p->SetBackgroundColour(c);
    return p;
}

} // namespace

// Os quatro números da mesa, desenhados no estilo Filuna.
class JanelaConfirmar::Resumo : public wxPanel
{
public:
    Resumo(wxWindow* pai, const ResumoMesa& r) : wxPanel(pai, wxID_ANY), m_r(r)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(-1, FromDIP(62)));
        Bind(wxEVT_PAINT, &Resumo::ao_pintar, this);
    }

private:
    void ao_pintar(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        auto                  gcp = comecar(this, dc);
        if (!gcp) return;
        wxGraphicsContext& gc = *gcp;
        const wxSize       s  = GetClientSize();

        struct { wxString titulo, valor; } campos[4] = {
            {wxString::FromUTF8("Tempo"), tempo_curto(m_r.tempo_s)},
            {wxString::FromUTF8("Peso"), m_r.gramas > 0. ? wxString::Format("%.1f g", m_r.gramas) : wxString::FromUTF8("—")},
            {wxString::FromUTF8("Custo"), m_r.custo > 0. ? wxString::FromUTF8(m_r.moeda + " ") + wxString::Format("%.2f", m_r.custo)
                                                         : wxString::FromUTF8("—")},
            {wxString::FromUTF8("Camada"), m_r.altura_camada > 0. ? wxString::Format("%.2f mm", m_r.altura_camada)
                                                                  : wxString::FromUTF8("—")},
        };

        const double vao = FromDIP(8), larg = (s.x - vao * 3) / 4.;
        for (int i = 0; i < 4; ++i) {
            const double x = i * (larg + vao);
            gc.SetBrush(wxBrush(AREIA));
            gc.SetPen(wxPen(AREIA_BORDA, 1));
            gc.DrawRoundedRectangle(x, 0, larg, s.y, FromDIP(10));
            texto_centro(gc, this, campos[i].titulo, fonte(this, 8, false), CINZA, x + larg / 2., FromDIP(10), int(larg) - FromDIP(8));
            texto_centro(gc, this, campos[i].valor, fonte(this, 12.5, true), TINTA, x + larg / 2., FromDIP(28), int(larg) - FromDIP(8));
        }
    }

    ResumoMesa m_r;
};

JanelaConfirmar::JanelaConfirmar(wxWindow* pai, const wxString& impressora, const ResumoMesa& resumo,
                                 const std::vector<ItemFilamento>& filamentos, const std::vector<ItemEntrada>& entradas,
                                 bool entrada_fixa)
    : wxDialog(pai, wxID_ANY, wxString::FromUTF8("Imprimir"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
    , m_entradas(entradas)
{
    SetBackgroundColour(BRANCO);
    auto* v = new wxBoxSizer(wxVERTICAL);

    // --- faixa magenta
    auto* faixa = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, FromDIP(56)));
    faixa->SetBackgroundColour(MAGENTA);
    {
        auto* fs = new wxBoxSizer(wxVERTICAL);
        auto* t1 = new wxStaticText(faixa, wxID_ANY, wxString::FromUTF8("Conferir antes de imprimir"));
        t1->SetForegroundColour(BRANCO);
        t1->SetFont(fonte(faixa, 12, true));
        auto* t2 = new wxStaticText(faixa, wxID_ANY, impressora);
        t2->SetForegroundColour(wxColour(0xFF, 0xE3, 0xEE));
        t2->SetFont(fonte(faixa, 9, false));
        fs->AddStretchSpacer();
        fs->Add(t1, 0, wxLEFT | wxRIGHT, FromDIP(18));
        fs->AddSpacer(FromDIP(2));
        fs->Add(t2, 0, wxLEFT | wxRIGHT, FromDIP(18));
        fs->AddStretchSpacer();
        faixa->SetSizer(fs);
    }
    v->Add(faixa, 0, wxEXPAND);

    auto titulo_secao = [this, v](const char* t) {
        auto* x = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(t));
        x->SetFont(fonte(this, 8.5, true));
        x->SetForegroundColour(CINZA);
        v->Add(x, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
    };

    // --- o arquivo
    titulo_secao("ARQUIVO");
    {
        auto* a = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(resumo.arquivo));
        a->SetFont(fonte(this, 10.5, false));
        v->Add(a, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
    }

    // --- os quatro números
    v->Add(new Resumo(this, resumo), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(18));

    // --- a mesa
    titulo_secao("MESA");
    {
        auto* l = new wxBoxSizer(wxHORIZONTAL);
        auto* m = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(resumo.mesa.empty() ? "A que está na impressora" : resumo.mesa));
        m->SetFont(fonte(this, 10, false));
        l->Add(m, 0, wxALIGN_CENTER_VERTICAL);
        l->AddStretchSpacer();
        m_nivelar = new wxCheckBox(this, wxID_ANY, wxString::FromUTF8("Nivelar a mesa antes"));
        m_nivelar->SetValue(wxGetApp().app_config->get("filuna_imprimir_nivelar") != "0");
        l->Add(m_nivelar, 0, wxALIGN_CENTER_VERTICAL);
        v->Add(l, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
    }

    // --- os filamentos
    if (!filamentos.empty() && !entradas.empty()) {
        titulo_secao("FILAMENTO");
        wxArrayString nomes;
        for (const ItemEntrada& e : entradas)
            nomes.Add(wxString::FromUTF8("Entrada " + std::to_string(e.global + 1) + (e.tipo.empty() ? "" : " · " + e.tipo)));

        auto* g = new wxFlexGridSizer(5, FromDIP(8), FromDIP(10));
        g->AddGrowableCol(2);
        for (const ItemFilamento& f : filamentos) {
            g->Add(amostra(this, cor_de(f.cor_hex)), 0, wxALIGN_CENTER_VERTICAL);

            wxString rotulo = wxString::FromUTF8("Filamento " + std::to_string(f.indice + 1) + (f.tipo.empty() ? "" : " · " + f.tipo));
            if (f.gramas > 0.) rotulo += wxString::Format("  (%.1f g)", f.gramas);
            auto* nome = new wxStaticText(this, wxID_ANY, rotulo);
            nome->SetFont(fonte(this, 9.5, false));
            g->Add(nome, 0, wxALIGN_CENTER_VERTICAL);

            auto* seta = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("→"));
            seta->SetForegroundColour(AREIA_ESCURA);
            g->Add(seta, 0, wxALIGN_CENTER_VERTICAL);

            int k = 0;
            for (std::size_t q = 0; q < entradas.size(); ++q)
                if (entradas[q].global == f.indice) k = int(q);
            auto* escolha = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, nomes);
            escolha->SetSelection(k);
            if (entrada_fixa) escolha->Disable();
            g->Add(escolha, 0, wxALIGN_CENTER_VERTICAL);
            m_escolhas.push_back(escolha);

            auto* destino = amostra(this, cor_de(entradas[k].cor_hex));
            escolha->Bind(wxEVT_CHOICE, [this, escolha, destino](wxCommandEvent&) {
                const int s = escolha->GetSelection();
                if (s >= 0 && s < int(m_entradas.size())) {
                    destino->SetBackgroundColour(cor_de(m_entradas[s].cor_hex));
                    destino->Refresh();
                }
            });
            g->Add(destino, 0, wxALIGN_CENTER_VERTICAL);
        }
        v->Add(g, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(18));

        if (entrada_fixa) {
            auto* n = new wxStaticText(this, wxID_ANY,
                                       wxString::FromUTF8("Nesta impressora o filamento 1 do projeto sai da Entrada 1, o 2 da Entrada 2, "
                                                          "e assim por diante."));
            n->SetFont(fonte(this, 8.5, false));
            n->SetForegroundColour(CINZA);
            v->Add(n, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
        }
    } else if (entradas.empty()) {
        titulo_secao("FILAMENTO");
        auto* n = new wxStaticText(this, wxID_ANY,
                                   wxString::FromUTF8("A impressora não informou um sistema de cores. Vai sair com o filamento que já "
                                                      "está carregado nela."));
        n->SetFont(fonte(this, 9.5, false));
        n->SetForegroundColour(CINZA);
        v->Add(n, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
    }

    // --- rodapé
    auto* rodape = new wxPanel(this, wxID_ANY);
    rodape->SetBackgroundColour(ROSA_CLARO);
    {
        auto* b = new wxBoxSizer(wxHORIZONTAL);
        b->AddStretchSpacer();
        auto* cancel = new wxButton(rodape, wxID_CANCEL, wxString::FromUTF8("Cancelar"));
        auto* so_env = new wxButton(rodape, wxID_APPLY, wxString::FromUTF8("Só enviar o arquivo"));
        // O botão principal no magenta do Filuna.
        auto* ok = new Button(rodape, wxString::FromUTF8("Imprimir"), "", 0, 0, wxID_OK);
        ok->SetBackgroundColor(StateColor(std::pair<wxColour, int>(MAGENTA_ESCURO, StateColor::Hovered),
                                          std::pair<wxColour, int>(MAGENTA, StateColor::Normal)));
        ok->SetBorderColor(StateColor(std::pair<wxColour, int>(MAGENTA_ESCURO, StateColor::Hovered),
                                      std::pair<wxColour, int>(MAGENTA, StateColor::Normal)));
        ok->SetTextColor(StateColor(std::pair<wxColour, int>(BRANCO, StateColor::Normal)));
        ok->SetFont(fonte(rodape, 9.5, true));
        ok->SetCornerRadius(ok->FromDIP(6));
        ok->SetMinSize(wxSize(ok->FromDIP(96), so_env->GetBestSize().y));
        ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_OK); });
        so_env->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_APPLY); });
        b->Add(cancel, 0, wxRIGHT, FromDIP(8));
        b->Add(so_env, 0, wxRIGHT, FromDIP(8));
        b->Add(ok, 0);
        rodape->SetSizer(b);
        b->SetMinSize(wxSize(-1, FromDIP(44)));
    }
    v->Add(rodape, 0, wxEXPAND | wxTOP, FromDIP(14));

    SetSizerAndFit(v);
    SetMinSize(wxSize(FromDIP(520), -1));
    Fit();
    CentreOnParent();
    wxGetApp().UpdateDlgDarkUI(this);
}

bool JanelaConfirmar::nivelar() const { return m_nivelar != nullptr && m_nivelar->GetValue(); }

std::vector<int> JanelaConfirmar::entradas_escolhidas() const
{
    std::vector<int> r;
    r.reserve(m_escolhas.size());
    for (wxChoice* c : m_escolhas) r.push_back(c->GetSelection());
    return r;
}


// ==============================================================================================
// Prévia das duas telas de impressão, para aprovar o visual sem impressora e sem enviar nada.
// Só roda com a variável de ambiente FILUNA_PREVIA_IMPRIMIR=1 (nenhum cliente tem).
// ==============================================================================================

void JanelaEnvio::mostrar_temperaturas(double bico, double bico_alvo, double mesa, double mesa_alvo)
{
    if (m_desenho != nullptr)
        m_desenho->temperaturas(bico, bico_alvo, mesa, mesa_alvo);
}

namespace {

// Relógio próprio da prévia, fora da janela, para não se misturar com a animação dela.
class RelogioPrevia : public wxEvtHandler
{
public:
    explicit RelogioPrevia(JanelaEnvio* j) : m_j(j), m_t(this)
    {
        Bind(wxEVT_TIMER, &RelogioPrevia::tique, this);
        m_t.Start(250);
    }

private:
    void tique(wxTimerEvent&)
    {
        JanelaEnvio* e = m_j.get();
        if (e == nullptr) { // fecharam a janela no meio
            m_t.Stop();
            wxTheApp->CallAfter([this] { delete this; });
            return;
        }
        const int t = ++m_n;
        if (t <= 20)
            e->passo(JanelaEnvio::Passo::ENVIANDO, t * 5);
        else if (t <= 26)
            e->passo(JanelaEnvio::Passo::CONFERINDO);
        else if (t <= 32)
            e->passo(JanelaEnvio::Passo::FALANDO);
        else if (t <= 80) {
            e->passo(JanelaEnvio::Passo::AQUECENDO);
            const double u = (t - 32) / 48.;
            e->mostrar_temperaturas(24 + u * (220 - 24), 220, 24 + u * (60 - 24), 60);
        } else {
            m_t.Stop();
            e->mostrar_temperaturas(220, 220, 60, 60);
            e->passo(JanelaEnvio::Passo::IMPRIMINDO);
            e->concluir(true);
            wxTheApp->CallAfter([this] { delete this; });
        }
    }

    wxWeakRef<JanelaEnvio> m_j;
    wxTimer                m_t;
    int                    m_n{0};
};

} // namespace

void previa_telas_de_impressao(wxWindow* pai)
{
    ResumoMesa r;
    r.arquivo       = "suporte_notebook_ondulado.gcode";
    r.mesa          = "Placa PEI texturizada";
    r.tempo_s       = 3780;
    r.gramas        = 35.9;
    r.custo         = 3.42;
    r.altura_camada = 0.20;
    const std::vector<ItemFilamento> fs = {{0, "PLA", "#C8102E", 22.4}, {1, "PETG", "#00C1AE", 13.5}};
    const std::vector<ItemEntrada>   es = {{0, "PLA", "#C8102E"}, {1, "PETG", "#00C1AE"}, {2, "PLA", "#F4F4F4"}, {3, "PLA", "#222222"}};

    JanelaConfirmar confirmar(pai, wxString::FromUTF8("A1-ASHLEY"), r, fs, es, false);
    const int resposta = confirmar.ShowModal();
    if (resposta != wxID_OK && resposta != wxID_APPLY)
        return;

    auto* envio = new JanelaEnvio(pai, std::string(), wxString::FromUTF8("A1-ASHLEY"), wxString::FromUTF8(r.arquivo));
    envio->Show();
    new RelogioPrevia(envio); // apaga-se sozinho no fim
}

}} // namespace Slic3r::GUI
