#include "FilunaFilamento.hpp"

#include "FilunaDados.hpp"
#include "FilunaLateral.hpp"
#include "GUI_App.hpp"
#include "PresetComboBoxes.hpp"
#include "Tab.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "slic3r/Filuna/FilunaDb.hpp"

#include <wx/checkbox.h>
#include <wx/display.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>

namespace Slic3r { namespace GUI { namespace FilunaFilamento {

using namespace FilunaLateral;
using namespace FilunaLateral::pintura;

namespace {

const wxColour FUNDO(0xFF, 0xFD, 0xF9), LARANJA_CLARO(0xFD, 0xE7, 0xD7), VERDE_CLARO(0xE5, 0xF2, 0xEA), BORDA_CLARA(0xEF, 0xE9, 0xE1);

wxString u8(const std::string& s) { return wxString::FromUTF8(s.c_str()); }
wxString u8(const char* s) { return wxString::FromUTF8(s); }

// Primeiro valor da opção (as do filamento são listas de um valor), como texto; vazio se não há.
std::string primeiro(const DynamicPrintConfig& c, const std::string& chave)
{
    const ConfigOption* o = c.option(chave);
    if (!o)
        return {};
    if (const auto* v = dynamic_cast<const ConfigOptionVectorBase*>(o)) {
        const std::vector<std::string> s = v->vserialize();
        return s.empty() ? std::string() : s.front();
    }
    return o->serialize();
}

double numero(const DynamicPrintConfig& c, const std::string& chave, double padrao = 0.)
{
    try {
        const std::string s = primeiro(c, chave);
        return s.empty() || s == "nil" ? padrao : std::stod(s);
    } catch (const std::exception&) {
        return padrao;
    }
}

// "0.98" → "0,98"; "nil" (sem valor: vale o da impressora) → vazio.
wxString para_tela(const std::string& s)
{
    if (s == "nil")
        return wxString();
    wxString t = u8(s);
    t.Replace(".", ",");
    return t;
}

BedType mesa_atual()
{
    if (const auto* o = wxGetApp().preset_bundle->project_config.option<ConfigOptionEnum<BedType>>("curr_bed_type"))
        return o->value;
    return btPEI;
}

wxString gramas(double g)
{
    if (g >= 1000.) {
        wxString kg = wxString::Format("%.1f kg", g / 1000.);
        kg.Replace(".", ",");
        return kg;
    }
    return wxString::Format("%d g", int(std::lround(g)));
}

struct Perfil
{
    std::string nome; // nome interno do preset
    wxString    rotulo, marca, tipo, faixa, mesa;
    int         bico{0}, mesa_c{0}, ventoinha{0};
    double      vol_max{0.}, estoque_g{0.};
};

struct ItemEstoque
{
    std::string preset; // perfil do Orca que corresponde (pode ficar vazio)
    wxString    titulo, detalhe;
    wxColour    cor;
};

// Os perfis de filamento que o Orca mostra nas listas (visíveis e compatíveis com a impressora), já carregados.
std::vector<Perfil> carregar_perfis(const std::string& atual)
{
    std::vector<Perfil> v;
    const BedType       mesa = mesa_atual();
    for (const Preset& p : wxGetApp().preset_bundle->filaments.get_presets()) {
        if (p.is_default || ((!p.is_visible || !p.is_compatible) && p.name != atual))
            continue;
        Perfil f;
        f.nome   = p.name;
        f.rotulo = u8(p.label(false));
        f.marca  = u8(primeiro(p.config, "filament_vendor"));
        if (f.marca.empty())
            f.marca = "Generic";
        f.tipo      = u8(primeiro(p.config, "filament_type"));
        f.bico      = int(numero(p.config, "nozzle_temperature"));
        f.mesa_c    = int(numero(p.config, get_bed_temp_key(mesa)));
        f.ventoinha = int(numero(p.config, "fan_max_speed"));
        f.vol_max   = numero(p.config, "filament_max_volumetric_speed");
        const int lo = int(numero(p.config, "nozzle_temperature_range_low")), hi = int(numero(p.config, "nozzle_temperature_range_high"));
        f.faixa      = lo > 0 && hi > 0 ? wxString::Format(wxString::FromUTF8("%d–%d °C"), lo, hi) : wxString::Format(wxString::FromUTF8("%d °C"), f.bico);
        f.mesa       = wxString::Format(wxString::FromUTF8("mesa %d °C"), f.mesa_c);
        v.push_back(std::move(f));
    }
    std::sort(v.begin(), v.end(), [](const Perfil& a, const Perfil& b) { return a.rotulo.CmpNoCase(b.rotulo) < 0; });
    return v;
}

// O estoque Filuna (bobinas ativas), por tipo cadastrado, ligado ao perfil do Orca que corresponde.
std::vector<ItemEstoque> carregar_estoque(std::vector<Perfil>& perfis)
{
    std::vector<ItemEstoque> v;
    try {
        Filuna::FilunaDb&                db = filuna_db();
        std::map<std::string, double>    por_tipo;
        for (const auto& b : db.bobinas())
            por_tipo[b.filamento_tipo_id] += b.restante_g;
        for (const auto& t : db.tipos()) {
            const double g = por_tipo[t.id];
            if (g <= 0.)
                continue;
            const std::string mat = boost::to_upper_copy(t.material), marca = boost::to_upper_copy(t.marca);
            auto achar = [&](const std::function<bool(const Perfil&)>& f) {
                return std::find_if(perfis.begin(), perfis.end(), f);
            };
            auto it = achar([&](const Perfil& p) { return !t.preset_orca.empty() && p.nome == t.preset_orca; });
            if (it == perfis.end())
                it = achar([&](const Perfil& p) {
                    return boost::to_upper_copy(std::string(p.tipo.ToUTF8())) == mat && boost::to_upper_copy(std::string(p.marca.ToUTF8())) == marca;
                });
            if (it == perfis.end())
                it = achar([&](const Perfil& p) { return boost::to_upper_copy(std::string(p.tipo.ToUTF8())) == mat && p.marca == "Generic"; });
            ItemEstoque e;
            e.preset  = it == perfis.end() ? std::string() : it->nome;
            e.titulo  = u8(t.marca + " " + t.material);
            e.detalhe = (t.cor_nome.empty() ? wxString() : u8(t.cor_nome) + wxString::FromUTF8(" · ")) + gramas(g);
            e.cor     = wxColour(u8(t.cor_hex));
            if (!e.cor.IsOk())
                e.cor = *wxWHITE;
            if (it != perfis.end())
                it->estoque_g += g;
            v.push_back(std::move(e));
        }
    } catch (const std::exception&) {
        v.clear(); // sem banco local: a janela funciona só com os perfis
    }
    return v;
}

wxStaticText* rotulo(wxWindow* pai, const wxString& t, double pontos, bool negrito, const wxColour& cor)
{
    auto* s = new wxStaticText(pai, wxID_ANY, t);
    s->SetFont(fonte(pai, pontos, negrito));
    s->SetForegroundColour(cor);
    return s;
}

// Painel desenhado: chama `pintar(gc, tamanho)`; `clique(ponto)` no botão esquerdo.
class Desenho : public wxPanel
{
public:
    Desenho(wxWindow* pai, int altura) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(wxSize(-1, altura));
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(this);
            auto                  gc = comecar(this, dc);
            if (gc && pintar)
                pintar(*gc, GetClientSize());
        });
        Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
        Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
            const bool mao = sobre && sobre(e.GetPosition());
            cursor_mao(this, mao);
        });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
            if (clique)
                clique(e.GetPosition());
        });
    }
    std::function<void(wxGraphicsContext&, const wxSize&)> pintar;
    std::function<void(const wxPoint&)>                     clique;
    std::function<bool(const wxPoint&)>                     sobre; // o ponto é clicável
};

// Cabeçalho das janelas: fundo de cor, um desenho à esquerda (número ou bobina), título, subtítulo e o X de fechar.
Desenho* cabecalho(wxDialog* janela, const wxColour& fundo, const wxColour& texto, std::function<void(wxGraphicsContext&, const wxRect&)> marca,
                   const wxString& titulo, const wxString& subtitulo)
{
    auto* d     = new Desenho(janela, janela->FromDIP(64));
    auto  fechar = [d](const wxSize& s) { return wxRect(s.x - d->FromDIP(52), d->FromDIP(14), d->FromDIP(36), d->FromDIP(36)); };
    d->pintar   = [=](wxGraphicsContext& gc, const wxSize& s) {
        gc.SetPen(*wxTRANSPARENT_PEN);
        gc.SetBrush(wxBrush(fundo));
        gc.DrawRectangle(0, 0, s.x, s.y);
        marca(gc, wxRect(d->FromDIP(18), d->FromDIP(14), d->FromDIP(40), d->FromDIP(36)));
        gc.SetFont(fonte(d, 13., true), texto);
        gc.DrawText(titulo, d->FromDIP(70), d->FromDIP(12));
        gc.SetFont(fonte(d, 8.5, false), texto);
        gc.DrawText(caber(d, subtitulo, fonte(d, 8.5, false), s.x - d->FromDIP(140)), d->FromDIP(70), d->FromDIP(36));
        const wxRect x = fechar(s);
        gc.SetBrush(wxBrush(wxColour(255, 255, 255, 40)));
        gc.DrawRoundedRectangle(x.x, x.y, x.width, x.height, d->FromDIP(10));
        gc.SetPen(wxPen(texto, d->FromDIP(2)));
        const double m = d->FromDIP(12);
        gc.StrokeLine(x.x + m, x.y + m, x.GetRight() - m, x.GetBottom() - m);
        gc.StrokeLine(x.GetRight() - m, x.y + m, x.x + m, x.GetBottom() - m);
    };
    d->sobre  = [d, fechar](const wxPoint& p) { return fechar(d->GetClientSize()).Contains(p); };
    d->clique = [d, janela, fechar](const wxPoint& p) {
        if (fechar(d->GetClientSize()).Contains(p))
            janela->EndModal(wxID_CANCEL);
    };
    return d;
}

void tamanho_da_janela(wxDialog* j, int largura, int altura)
{
    const wxRect tela = wxDisplay(wxDisplay::GetFromWindow(j->GetParent()) == wxNOT_FOUND ? 0 : wxDisplay::GetFromWindow(j->GetParent())).GetClientArea();
    j->SetSize(std::min(j->FromDIP(largura), tela.width - 40), std::min(j->FromDIP(altura), tela.height - 40));
    j->CentreOnParent();
}

// As 12 cores prontas da janela de filamento.
const std::vector<std::pair<const char*, const char*>> CORES{
    {"Branco", "#F2F0EC"}, {"Preto", "#1F1F22"}, {"Vermelho", "#C8102E"}, {"Laranja", "#FF6B1A"}, {"Amarelo", "#F2C230"}, {"Verde", "#3FA34D"},
    {"Azul", "#2F6FD6"},   {"Roxo", "#7B3FB8"},  {"Rosa", "#E86AA6"},     {"Cinza", "#8C8C90"},   {"Bege", "#E3CBA8"},    {"Marrom", "#7A4A2A"}};

// Estado da janela de filamento, compartilhado pelos painéis.
struct Estado
{
    int                      entrada{0};
    std::vector<Perfil>      perfis;
    std::vector<ItemEstoque> estoque;
    std::string              preset;
    wxColour                 cor;
    wxString                 marca, material, busca;
    bool                     so_estoque{false};

    const Perfil* escolhido() const
    {
        auto it = std::find_if(perfis.begin(), perfis.end(), [this](const Perfil& p) { return p.nome == preset; });
        return it == perfis.end() ? nullptr : &*it;
    }
    bool passa(const Perfil& p, bool por_marca) const
    {
        if (!material.empty() && p.tipo != material)
            return false;
        if (so_estoque && p.estoque_g <= 0.)
            return false;
        if (!busca.empty())
            return (p.rotulo + " " + p.marca + " " + p.tipo).Lower().Contains(busca.Lower());
        return !por_marca || p.marca == marca;
    }
};

// O que a lista desenha, com a posição: títulos, cartões do estoque, linhas de perfil e as marcas.
struct ItemLista
{
    enum Tipo { titulo, estoque, perfil, marca } tipo;
    wxRect   r;
    int      i{0};
    wxString texto, extra;
};

class Lista : public wxPanel
{
public:
    Lista(wxWindow* pai, Estado& e) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE), m_e(e)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
        Bind(wxEVT_SIZE, [this](wxSizeEvent& ev) {
            if (ev.GetSize().x != m_largura)
                montar();
            ev.Skip();
        });
        Bind(wxEVT_MOTION, [this](wxMouseEvent& ev) {
            const int i = sob(ev.GetPosition());
            if (i != m_sobre) {
                m_sobre = i;
                cursor_mao(this, i >= 0);
                Refresh();
            }
        });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = -1; cursor_mao(this, false); Refresh(); });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& ev) {
            const int i = sob(ev.GetPosition());
            if (i < 0)
                return;
            const ItemLista& it = m_itens[i];
            if (it.tipo == ItemLista::estoque) {
                const ItemEstoque& s = m_e.estoque[it.i];
                if (!s.preset.empty())
                    m_e.preset = s.preset;
                m_e.cor = s.cor;
            } else if (it.tipo == ItemLista::perfil)
                m_e.preset = m_e.perfis[it.i].nome;
            else if (it.tipo == ItemLista::marca) {
                m_e.marca = it.texto;
                m_e.busca.clear();
            }
            if (ao_mudar)
                ao_mudar();
        });
    }
    std::function<void()> ao_mudar;

    void montar()
    {
        m_largura = GetClientSize().x;
        m_itens.clear();
        const int w = std::max(FromDIP(200), m_largura), gap = FromDIP(8);
        int       y = 0;
        auto      titulo = [&](const wxString& t) {
            m_itens.push_back({ItemLista::titulo, wxRect(0, y, w, FromDIP(22)), 0, t});
            y += FromDIP(24);
        };
        // estoque
        std::vector<int> est;
        for (int i = 0; i < int(m_e.estoque.size()); ++i) {
            const ItemEstoque& s = m_e.estoque[i];
            if (!m_e.busca.empty() && !(s.titulo + " " + s.detalhe).Lower().Contains(m_e.busca.Lower()))
                continue;
            const Perfil* p = nullptr;
            for (const Perfil& f : m_e.perfis)
                if (f.nome == s.preset)
                    p = &f;
            if (!m_e.material.empty() && (!p || p->tipo != m_e.material))
                continue;
            est.push_back(i);
        }
        if (!est.empty()) {
            titulo(u8("No seu estoque Filuna"));
            const int cw = (w - 2 * gap) / 3, ch = FromDIP(56);
            for (size_t k = 0; k < est.size(); ++k) {
                m_itens.push_back({ItemLista::estoque, wxRect(int(k % 3) * (cw + gap), y + int(k / 3) * (ch + gap), cw, ch), est[k]});
            }
            y += int((est.size() + 2) / 3) * (ch + gap) + FromDIP(6);
        }
        // perfis: da marca em foco, ou os que a busca acha em todas
        const bool por_marca = m_e.busca.empty();
        titulo(por_marca ? (m_e.marca == "Generic" ? u8("Perfis genéricos") : u8("Perfis da ") + m_e.marca) : u8("Resultados"));
        int n = 0;
        for (int i = 0; i < int(m_e.perfis.size()) && n < 120; ++i) {
            if (!m_e.passa(m_e.perfis[i], por_marca))
                continue;
            m_itens.push_back({ItemLista::perfil, wxRect(0, y, w, FromDIP(42)), i});
            y += FromDIP(42);
            ++n;
        }
        if (n == 0) {
            m_itens.push_back({ItemLista::titulo, wxRect(0, y, w, FromDIP(30)), 1, u8("Nenhum perfil com esse filtro.")});
            y += FromDIP(32);
        }
        y += FromDIP(10);
        // outras marcas
        std::map<wxString, int> marcas;
        for (const Perfil& p : m_e.perfis)
            if (p.marca != m_e.marca && (m_e.material.empty() || p.tipo == m_e.material) && (!m_e.so_estoque || p.estoque_g > 0.))
                ++marcas[p.marca];
        if (!marcas.empty()) {
            titulo(u8("Outras marcas"));
            int x = 0;
            wxClientDC dc(this);
            dc.SetFont(fonte(this, 9., true));
            for (const auto& [nome, qtd] : marcas) {
                const wxString t  = nome == "Generic" ? u8("Genérico") : nome;
                const int      bw = dc.GetTextExtent(t).x + dc.GetTextExtent(wxString::Format("%d", qtd)).x + FromDIP(34);
                if (x > 0 && x + bw > w) {
                    x = 0;
                    y += FromDIP(38);
                }
                m_itens.push_back({ItemLista::marca, wxRect(x, y, bw, FromDIP(32)), 0, nome, wxString::Format("%d", qtd)});
                x += bw + FromDIP(6);
            }
            y += FromDIP(40);
        }
        m_desenhos.clear();
        SetMinSize(wxSize(-1, y));
        if (GetParent()) {
            GetParent()->Layout();
            static_cast<wxScrolledWindow*>(GetParent())->FitInside();
        }
        Refresh();
    }

private:
    int sob(const wxPoint& p) const
    {
        for (int i = 0; i < int(m_itens.size()); ++i)
            if (m_itens[i].tipo != ItemLista::titulo && m_itens[i].r.Contains(p))
                return i;
        return -1;
    }

    void pintar()
    {
        wxAutoBufferedPaintDC dc(this);
        auto                  gc = comecar(this, dc);
        if (!gc)
            return;
        for (int i = 0; i < int(m_itens.size()); ++i) {
            const ItemLista& it = m_itens[i];
            const wxRect&    r  = it.r;
            const bool       s  = i == m_sobre;
            switch (it.tipo) {
            case ItemLista::titulo:
                gc->SetFont(fonte(this, 9., it.i == 0), it.i == 0 ? wxColour(0x4A, 0x41, 0x3A) : CINZA);
                gc->DrawText(it.texto, r.x, r.y + FromDIP(3));
                break;
            case ItemLista::estoque: {
                const ItemEstoque& e   = m_e.estoque[it.i];
                const bool         sel = !e.preset.empty() && e.preset == m_e.preset && e.cor == m_e.cor;
                gc->SetPen(wxPen(sel || s ? MAGENTA : CARTAO_BORDA, FromDIP(sel ? 2 : 1)));
                gc->SetBrush(wxBrush(sel || s ? CARTAO_SOBRE : CARTAO));
                gc->DrawRoundedRectangle(r.x + 1, r.y + 1, r.width - 2, r.height - 2, FromDIP(10));
                const wxString chave = e.cor.GetAsString(wxC2S_HTML_SYNTAX);
                if (!m_desenhos.count(chave))
                    m_desenhos[chave] = bobina(e.cor, FromDIP(40));
                const wxBitmap& b = m_desenhos[chave];
                gc->DrawBitmap(b, r.x + FromDIP(6), r.y + (r.height - b.GetHeight()) / 2., b.GetWidth(), b.GetHeight());
                const int tx = r.x + FromDIP(52), tl = r.GetRight() - tx - FromDIP(4);
                gc->SetFont(fonte(this, 8.5, true), TEXTO_CLARO);
                gc->DrawText(caber(this, e.titulo, fonte(this, 8.5, true), tl), tx, r.y + FromDIP(10));
                gc->SetFont(fonte(this, 7.5, false), TEXTO_APAGADO);
                gc->DrawText(caber(this, e.detalhe, fonte(this, 7.5, false), tl), tx, r.y + FromDIP(29));
                break;
            }
            case ItemLista::perfil: {
                const Perfil& p   = m_e.perfis[it.i];
                const bool    sel = p.nome == m_e.preset;
                gc->SetPen(wxPen(BORDA_CLARA, 1));
                gc->SetBrush(wxBrush(sel ? ROSA_CLARO : s ? wxColour(0xFB, 0xF8, 0xF4) : BRANCO));
                gc->DrawRectangle(r.x, r.y, r.width, r.height);
                const double rd = FromDIP(8), cx = r.x + FromDIP(20), cy = r.y + r.height / 2.;
                gc->SetPen(wxPen(sel ? MAGENTA : AREIA_ESCURA, FromDIP(sel ? 5 : 2)));
                gc->SetBrush(wxBrush(BRANCO));
                gc->DrawEllipse(cx - rd + (sel ? FromDIP(2) : 0), cy - rd + (sel ? FromDIP(2) : 0), 2 * rd - (sel ? FromDIP(4) : 0),
                                2 * rd - (sel ? FromDIP(4) : 0));
                const wxString direita = p.faixa + "    " + p.mesa + (p.estoque_g > 0. ? wxString::FromUTF8("    estoque ") + gramas(p.estoque_g) : wxString());
                gc->SetFont(fonte(this, 8., false), CINZA);
                double tw = 0, th = 0;
                gc->GetTextExtent(direita, &tw, &th);
                gc->DrawText(direita, r.GetRight() - tw - FromDIP(14), cy - th / 2.);
                texto_meio(*gc, this, p.rotulo, fonte(this, 9.5, true), TINTA, r.x + FromDIP(38), r, int(r.GetRight() - tw - FromDIP(60) - r.x));
                break;
            }
            case ItemLista::marca: {
                Botao b{it.texto == "Generic" ? u8("Genérico") : it.texto};
                desenhar_botao(*gc, this, r, b, s, false);
                break;
            }
            }
        }
    }

    Estado&                       m_e;
    std::vector<ItemLista>        m_itens;
    std::map<wxString, wxBitmap>  m_desenhos;
    int                           m_sobre{-1}, m_largura{-1};
};

} // namespace

bool editar_entrada(int entrada)
{
    PresetBundle& pb  = *wxGetApp().preset_bundle;
    Tab*          tab = wxGetApp().get_tab(Preset::TYPE_FILAMENT);
    if (!tab || entrada < 0 || entrada >= int(pb.filament_presets.size()))
        return false;
    const std::string& nome = pb.filament_presets[entrada];
    if (nome != pb.filaments.get_selected_preset_name() && !tab->select_preset(nome))
        return false;
    if (tab->get_combo_box())
        tab->get_combo_box()->set_filament_idx(entrada);
    return true;
}

// ================================================================================================ escolher filamento
struct JanelaFilamento::Dados
{
    Estado      e;
    Lista*      lista{nullptr};
    Desenho*    amostras{nullptr};
    Desenho*    chips{nullptr};
    Desenho*    resumo{nullptr};
    wxTextCtrl* hex{nullptr};
    Botoes*     rodape{nullptr};
    std::vector<wxString> materiais;
};

JanelaFilamento::JanelaFilamento(wxWindow* pai, int entrada)
    : wxDialog(pai, wxID_ANY, wxString::Format(wxString::FromUTF8("Filamento da entrada %d"), entrada + 1), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m(std::make_unique<Dados>())
{
    SetBackgroundColour(FUNDO);
    PresetBundle& pb = *wxGetApp().preset_bundle;
    Estado&       e  = m->e;
    e.entrada        = entrada;
    e.preset         = entrada < int(pb.filament_presets.size()) ? pb.filament_presets[entrada] : std::string();
    if (const auto* cores = pb.project_config.option<ConfigOptionStrings>("filament_colour"); cores && entrada < int(cores->values.size()))
        e.cor = wxColour(u8(cores->values[entrada].substr(0, 7)));
    if (!e.cor.IsOk())
        e.cor = *wxWHITE;
    e.perfis  = carregar_perfis(e.preset);
    e.estoque = carregar_estoque(e.perfis);
    if (const Perfil* p = e.escolhido())
        e.marca = p->marca;
    else
        e.marca = "Generic";
    {
        std::map<wxString, int> conta;
        for (const Perfil& p : e.perfis)
            if (!p.tipo.empty())
                ++conta[p.tipo];
        std::vector<std::pair<int, wxString>> ordem;
        for (const auto& [t, n] : conta)
            ordem.emplace_back(-n, t);
        std::sort(ordem.begin(), ordem.end());
        m->materiais.push_back(u8("Todos"));
        for (size_t i = 0; i < ordem.size() && i < 9; ++i)
            m->materiais.push_back(ordem[i].second);
    }

    const wxString impressora = u8(pb.printers.get_edited_preset().label(false));
    auto*          topo       = cabecalho(
        this, MAGENTA, BRANCO,
        [this, entrada](wxGraphicsContext& gc, const wxRect& r) {
            gc.SetPen(*wxTRANSPARENT_PEN);
            gc.SetBrush(wxBrush(BRANCO));
            const double d = FromDIP(32);
            gc.DrawEllipse(r.x, r.y + (r.height - d) / 2., d, d);
            texto_centro(gc, this, wxString::Format("%d", entrada + 1), fonte(this, 11., true), MAGENTA, r.x + d / 2., r.y + (r.height - d) / 2. + FromDIP(6),
                         int(d));
        },
        wxString::Format(wxString::FromUTF8("Filamento da entrada %d"), entrada + 1), impressora);

    auto* corpo = new wxBoxSizer(wxVERTICAL);
    corpo->Add(topo, 0, wxEXPAND);

    // 1 · cor
    auto* sec1 = new wxBoxSizer(wxHORIZONTAL);
    sec1->Add(rotulo(this, u8("1 · COR"), 9., true, VINHO), 0, wxALIGN_BOTTOM);
    sec1->Add(rotulo(this, u8("a cor que aparece na peça e na bobina"), 8.5, false, CINZA), 0, wxALIGN_BOTTOM | wxLEFT, FromDIP(8));
    corpo->Add(sec1, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
    auto* linha_cor = new wxBoxSizer(wxHORIZONTAL);
    m->amostras     = new Desenho(this, FromDIP(40));
    auto amostra    = [this](int i) { return wxRect(i * FromDIP(38), FromDIP(4), FromDIP(32), FromDIP(32)); };
    m->amostras->SetMinSize(wxSize(FromDIP(38) * int(CORES.size()), FromDIP(40)));
    m->amostras->pintar = [this, amostra](wxGraphicsContext& gc, const wxSize&) {
        for (int i = 0; i < int(CORES.size()); ++i) {
            const wxColour c(u8(CORES[i].second));
            const wxRect   r   = amostra(i);
            const bool     sel = c == m->e.cor;
            if (sel) {
                gc.SetPen(wxPen(MAGENTA, FromDIP(2)));
                gc.SetBrush(*wxTRANSPARENT_BRUSH);
                gc.DrawEllipse(r.x - FromDIP(3), r.y - FromDIP(3), r.width + FromDIP(6), r.height + FromDIP(6));
            }
            gc.SetPen(wxPen(wxColour(0, 0, 0, 50), 1));
            gc.SetBrush(wxBrush(c));
            gc.DrawEllipse(r.x, r.y, r.width, r.height);
        }
    };
    m->amostras->sobre  = [amostra](const wxPoint& p) {
        for (int i = 0; i < int(CORES.size()); ++i)
            if (amostra(i).Contains(p))
                return true;
        return false;
    };
    m->amostras->clique = [this, amostra](const wxPoint& p) {
        for (int i = 0; i < int(CORES.size()); ++i)
            if (amostra(i).Contains(p)) {
                m->e.cor = wxColour(u8(CORES[i].second));
                m->hex->ChangeValue(m->e.cor.GetAsString(wxC2S_HTML_SYNTAX));
                m->amostras->Refresh();
                m->resumo->Refresh();
            }
    };
    m->amostras->SetToolTip(u8("Clique numa cor ou digite o código ao lado"));
    linha_cor->Add(m->amostras, 0, wxALIGN_CENTER_VERTICAL);
    linha_cor->Add(rotulo(this, u8("Outra:"), 9., true, CINZA), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    m->hex = new wxTextCtrl(this, wxID_ANY, e.cor.GetAsString(wxC2S_HTML_SYNTAX), wxDefaultPosition, wxSize(FromDIP(90), -1));
    m->hex->SetToolTip(u8("Código da cor, como #C8102E"));
    m->hex->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        const wxString t = m->hex->GetValue().Trim().Trim(false);
        const wxColour c(t.StartsWith("#") ? t : "#" + t);
        if (c.IsOk() && (t.length() == 7 || t.length() == 6)) {
            m->e.cor = c;
            m->amostras->Refresh();
            m->resumo->Refresh();
        }
    });
    linha_cor->Add(m->hex, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(6));
    corpo->Add(linha_cor, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(10));
    auto* div = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    div->SetBackgroundColour(BORDA_CLARA);
    corpo->Add(div, 0, wxEXPAND | wxTOP, FromDIP(14));

    // 2 · filamento
    auto* sec2 = new wxBoxSizer(wxHORIZONTAL);
    sec2->Add(rotulo(this, u8("2 · FILAMENTO"), 9., true, VINHO), 0, wxALIGN_BOTTOM);
    sec2->Add(rotulo(this, u8("perfis do Orca já trazidos para a Filuna"), 8.5, false, CINZA), 0, wxALIGN_BOTTOM | wxLEFT, FromDIP(8));
    corpo->Add(sec2, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(14));
    auto* linha_busca = new wxBoxSizer(wxHORIZONTAL);
    auto* busca       = new wxTextCtrl(this, wxID_ANY, wxString(), wxDefaultPosition, wxSize(-1, FromDIP(30)));
    busca->SetHint(u8("Buscar marca, material ou nome"));
    linha_busca->Add(busca, 1, wxALIGN_CENTER_VERTICAL);
    auto* so = new wxCheckBox(this, wxID_ANY, u8("Só com estoque"));
    linha_busca->Add(so, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
    corpo->Add(linha_busca, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(10));

    m->chips      = new Desenho(this, FromDIP(34));
    auto chip     = [this](int i) {
        wxClientDC dc(m->chips);
        dc.SetFont(fonte(m->chips, 8.5, true));
        int x = 0;
        for (int k = 0; k < i; ++k)
            x += dc.GetTextExtent(m->materiais[k]).x + FromDIP(30);
        return wxRect(x, FromDIP(2), dc.GetTextExtent(m->materiais[i]).x + FromDIP(24), FromDIP(28));
    };
    m->chips->pintar = [this, chip](wxGraphicsContext& gc, const wxSize&) {
        for (int i = 0; i < int(m->materiais.size()); ++i) {
            Botao b{m->materiais[i]};
            b.destaque = (i == 0 && m->e.material.empty()) || (i > 0 && m->materiais[i] == m->e.material);
            desenhar_botao(gc, m->chips, chip(i), b, false, false);
        }
    };
    m->chips->sobre  = [this, chip](const wxPoint& p) {
        for (int i = 0; i < int(m->materiais.size()); ++i)
            if (chip(i).Contains(p))
                return true;
        return false;
    };
    m->chips->clique = [this, chip](const wxPoint& p) {
        for (int i = 0; i < int(m->materiais.size()); ++i)
            if (chip(i).Contains(p)) {
                m->e.material = i == 0 ? wxString() : m->materiais[i];
                m->chips->Refresh();
                m->lista->montar();
            }
    };
    corpo->Add(m->chips, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(10));

    auto* rolagem = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    rolagem->SetBackgroundColour(FUNDO);
    rolagem->SetScrollRate(0, FromDIP(20));
    m->lista     = new Lista(rolagem, e);
    auto* sl     = new wxBoxSizer(wxVERTICAL);
    sl->Add(m->lista, 0, wxEXPAND);
    rolagem->SetSizer(sl);
    corpo->Add(rolagem, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(10));

    // resumo do escolhido
    m->resumo       = new Desenho(this, FromDIP(60));
    auto botao_ajuste = [this](const wxSize& s) { return wxRect(s.x - FromDIP(96), FromDIP(13), FromDIP(78), FromDIP(34)); };
    m->resumo->pintar = [this, botao_ajuste](wxGraphicsContext& gc, const wxSize& s) {
        gc.SetPen(*wxTRANSPARENT_PEN);
        gc.SetBrush(wxBrush(ROSA_CLARO));
        gc.DrawRectangle(0, 0, s.x, s.y);
        gc.SetPen(wxPen(BORDA_CLARA, 1));
        gc.StrokeLine(0, 0, s.x, 0);
        const wxBitmap b = bobina(m->e.cor, FromDIP(42));
        gc.DrawBitmap(b, FromDIP(18), (s.y - b.GetHeight()) / 2., b.GetWidth(), b.GetHeight());
        const Perfil*  p    = m->e.escolhido();
        const int      tl   = botao_ajuste(s).x - FromDIP(76);
        gc.SetFont(fonte(m->resumo, 10., true), TINTA);
        gc.DrawText(caber(m->resumo, p ? p->rotulo : u8("Escolha um perfil"), fonte(m->resumo, 10., true), tl), FromDIP(70), FromDIP(12));
        if (p) {
            const wxString d = wxString::Format(wxString::FromUTF8("Bico %d °C · Mesa %d °C · Ventoinha %d%% · até %s mm³/s"), p->bico, p->mesa_c, p->ventoinha,
                                                wxString::Format("%g", p->vol_max));
            gc.SetFont(fonte(m->resumo, 8.5, false), wxColour(0x4A, 0x41, 0x3A));
            gc.DrawText(caber(m->resumo, d, fonte(m->resumo, 8.5, false), tl), FromDIP(70), FromDIP(33));
        }
        desenhar_botao(gc, m->resumo, botao_ajuste(s), Botao{u8("Ajustes"), Icone::lapis}, false, false);
    };
    m->resumo->sobre  = [this, botao_ajuste](const wxPoint& p) { return botao_ajuste(m->resumo->GetClientSize()).Contains(p); };
    m->resumo->clique = [this, botao_ajuste](const wxPoint& p) {
        if (botao_ajuste(m->resumo->GetClientSize()).Contains(p) && m->e.escolhido())
            EndModal(ABRIR_AJUSTES);
    };
    corpo->Add(m->resumo, 0, wxEXPAND | wxTOP, FromDIP(8));

    m->rodape = new Botoes(this, false);
    m->rodape->SetMinSize(wxSize(-1, FromDIP(40)));
    Botao usar{wxString::Format(wxString::FromUTF8("Usar na entrada %d"), entrada + 1)};
    usar.destaque = true;
    m->rodape->definir({Botao{u8("Cadastrar filamento novo"), Icone::mais}, Botao{u8("Cancelar")}, usar});
    m->rodape->ao_clicar = [this](int i, const wxRect&) {
        if (i == 0)
            EndModal(CADASTRAR_NOVO);
        else if (i == 1)
            EndModal(wxID_CANCEL);
        else if (m->e.escolhido())
            EndModal(wxID_OK);
    };
    corpo->Add(m->rodape, 0, wxEXPAND | wxALL, FromDIP(14));

    m->lista->ao_mudar = [this]() {
        m->hex->ChangeValue(m->e.cor.GetAsString(wxC2S_HTML_SYNTAX));
        m->amostras->Refresh();
        m->resumo->Refresh();
        m->lista->montar();
    };
    busca->Bind(wxEVT_TEXT, [this, busca](wxCommandEvent&) {
        m->e.busca = busca->GetValue().Trim().Trim(false);
        m->lista->montar();
    });
    so->Bind(wxEVT_CHECKBOX, [this, so](wxCommandEvent&) {
        m->e.so_estoque = so->GetValue();
        m->lista->montar();
    });

    SetSizer(corpo);
    tamanho_da_janela(this, 700, 860);
    Layout();
    m->lista->montar();
}

JanelaFilamento::~JanelaFilamento() = default;

const std::string& JanelaFilamento::preset() const { return m->e.preset; }
wxColour           JanelaFilamento::cor() const { return m->e.cor; }

// ================================================================================================ ajustes do filamento
namespace {
struct Campo
{
    std::string chave;
    const char* nome;
    const char* unidade;
    int         grupo;
    wxTextCtrl* caixa{nullptr};
    wxString    inicial;
};
} // namespace

struct JanelaAjustesFilamento::Dados
{
    int                entrada{0};
    std::string        nome;
    std::vector<Campo> campos;
};

JanelaAjustesFilamento::JanelaAjustesFilamento(wxWindow* pai, int entrada)
    : wxDialog(pai, wxID_ANY, wxString::FromUTF8("Ajustes do filamento"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m(std::make_unique<Dados>())
{
    SetBackgroundColour(FUNDO);
    PresetBundle& pb = *wxGetApp().preset_bundle;
    m->entrada       = entrada;
    m->nome          = entrada < int(pb.filament_presets.size()) ? pb.filament_presets[entrada] : std::string();
    const Preset* salvo = pb.filaments.find_preset(m->nome, false);
    // Valores atuais: os da aba (com o que já foi mudado e não salvo) se ela está neste perfil; senão os do perfil salvo.
    const DynamicPrintConfig* atual = pb.filaments.get_selected_preset_name() == m->nome ? &pb.filaments.get_edited_preset().config :
                                      salvo                                              ? &salvo->config :
                                                                                           nullptr;
    wxColour cor = *wxWHITE;
    if (const auto* cores = pb.project_config.option<ConfigOptionStrings>("filament_colour"); cores && entrada < int(cores->values.size()))
        cor = wxColour(u8(cores->values[entrada].substr(0, 7)));
    if (!cor.IsOk())
        cor = *wxWHITE;

    const BedType mesa = mesa_atual();
    m->campos          = {
        {"nozzle_temperature_initial_layer", "Bico, primeira camada", "°C", 0},
        {"nozzle_temperature", "Bico, outras camadas", "°C", 0},
        {get_bed_temp_1st_layer_key(mesa), "Mesa, primeira camada", "°C", 0},
        {get_bed_temp_key(mesa), "Mesa, outras camadas", "°C", 0},
        {"fan_max_speed", "Ventoinha da peça (máxima)", "%", 1},
        {"close_fan_the_first_x_layers", "Sem ventoinha nas primeiras", "camadas", 1},
        {"filament_flow_ratio", "Vazão (fluxo)", "", 2},
        {"filament_max_volumetric_speed", "Velocidade volumétrica máxima", "mm³/s", 2},
        {"pressure_advance", "Pressure advance", "", 2},
        {"filament_retraction_length", "Comprimento da retração", "mm", 3},
        {"filament_retraction_speed", "Velocidade da retração", "mm/s", 3},
    };

    auto* corpo = new wxBoxSizer(wxVERTICAL);
    corpo->Add(cabecalho(
                   this, CARTAO, TEXTO_CLARO,
                   [cor, this](wxGraphicsContext& gc, const wxRect& r) {
                       const wxBitmap b = bobina(cor, FromDIP(42));
                       gc.DrawBitmap(b, r.x - FromDIP(4), r.y + (r.height - b.GetHeight()) / 2., b.GetWidth(), b.GetHeight());
                   },
                   salvo ? u8(salvo->label(false)) : u8(m->nome), wxString::Format(wxString::FromUTF8("Ajustes do filamento · entrada %d"), entrada + 1)),
               0, wxEXPAND);

    const int lo = atual ? int(numero(*atual, "nozzle_temperature_range_low")) : 0, hi = atual ? int(numero(*atual, "nozzle_temperature_range_high")) : 0;
    const std::vector<std::tuple<wxString, wxString, wxColour>> grupos{
        {u8("TEMPERATURA"), lo > 0 && hi > 0 ? wxString::Format(wxString::FromUTF8("faixa do fabricante: %d–%d °C"), lo, hi) : wxString(), ROSA},
        {u8("VENTOINHA"), wxString(), LILAS},
        {u8("FLUXO E VELOCIDADE"), wxString(), LARANJA_CLARO},
        {u8("RETRAÇÃO"), u8("vazio: vale o da impressora"), VERDE_CLARO}};
    for (int g = 0; g < int(grupos.size()); ++g) {
        auto* caixa = new wxPanel(this);
        caixa->SetBackgroundColour(BRANCO);
        auto* cs    = new wxBoxSizer(wxVERTICAL);
        auto* topo  = new Desenho(caixa, FromDIP(34));
        const auto [titulo, dica, fundo] = grupos[g];
        topo->pintar = [topo, titulo = titulo, dica = dica, fundo = fundo](wxGraphicsContext& gc, const wxSize& s) {
            gc.SetPen(*wxTRANSPARENT_PEN);
            gc.SetBrush(wxBrush(fundo));
            gc.DrawRectangle(0, 0, s.x, s.y);
            texto_meio(gc, topo, titulo, fonte(topo, 8.5, true), VINHO, topo->FromDIP(14), wxRect(0, 0, s.x, s.y), s.x / 2);
            gc.SetFont(fonte(topo, 8., false), CINZA);
            double tw = 0, th = 0;
            gc.GetTextExtent(dica, &tw, &th);
            gc.DrawText(dica, s.x - tw - topo->FromDIP(14), (s.y - th) / 2.);
        };
        cs->Add(topo, 0, wxEXPAND);
        for (Campo& c : m->campos) {
            if (c.grupo != g)
                continue;
            auto* linha = new wxBoxSizer(wxHORIZONTAL);
            linha->Add(rotulo(caixa, u8(c.nome), 9.5, false, TINTA), 1, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(14));
            c.inicial = atual ? para_tela(primeiro(*atual, c.chave)) : wxString();
            c.caixa   = new wxTextCtrl(caixa, wxID_ANY, c.inicial, wxDefaultPosition, wxSize(FromDIP(80), -1), wxTE_RIGHT);
            linha->Add(c.caixa, 0, wxALIGN_CENTER_VERTICAL);
            linha->Add(rotulo(caixa, u8(c.unidade), 8.5, false, CINZA), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(8));
            linha->SetMinSize(wxSize(-1, FromDIP(42)));
            cs->Add(linha, 0, wxEXPAND);
            auto* risco = new wxPanel(caixa, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
            risco->SetBackgroundColour(BORDA_CLARA);
            cs->Add(risco, 0, wxEXPAND);
        }
        caixa->SetSizer(cs);
        corpo->Add(caixa, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(14));
    }

    auto* avancado = new Botoes(this, false);
    avancado->definir({Botao{u8("Ajustes avançados (todos os campos do Orca)"), Icone::tabela}});
    avancado->ao_clicar = [this](int, const wxRect&) { EndModal(AJUSTES_AVANCADOS); };
    corpo->Add(avancado, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(14));
    corpo->AddStretchSpacer(1);

    // Grava os campos mudados pela aba de filamento do Orca, como se o cliente tivesse mudado lá.
    auto aplicar = [this]() {
        PresetBundle&       pb  = *wxGetApp().preset_bundle;
        Tab*                tab = wxGetApp().get_tab(Preset::TYPE_FILAMENT);
        if (!tab || !editar_entrada(m->entrada))
            return false;
        const DynamicPrintConfig& base = pb.filaments.get_edited_preset().config;
        DynamicPrintConfig        novo;
        for (const Campo& c : m->campos) {
            const wxString v = c.caixa->GetValue().Trim().Trim(false);
            if (v == c.inicial || !base.has(c.chave))
                continue;
            std::string s = v.empty() ? std::string("nil") : std::string(v.ToUTF8());
            std::replace(s.begin(), s.end(), ',', '.');
            std::unique_ptr<ConfigOption> o(base.option(c.chave)->clone());
            try {
                o->deserialize(s);
            } catch (const std::exception&) {
                continue; // valor que não é número: fica o anterior
            }
            novo.set_key_value(c.chave, o.release());
        }
        if (!novo.empty())
            tab->load_config(novo);
        return true;
    };
    auto* rodape = new Botoes(this, false);
    rodape->SetMinSize(wxSize(-1, FromDIP(40)));
    Botao ok{u8("Aplicar")};
    ok.destaque = true;
    rodape->definir({Botao{u8("Voltar ao padrão")}, Botao{u8("Salvar como meu perfil")}, ok});
    rodape->ao_clicar = [this, aplicar, salvo](int i, const wxRect&) {
        if (i == 0) { // os valores do perfil salvo voltam para as caixas; só gravam no Aplicar
            for (Campo& c : m->campos)
                c.caixa->ChangeValue(salvo ? para_tela(primeiro(salvo->config, c.chave)) : c.inicial);
            return;
        }
        if (!aplicar())
            return;
        if (i == 1)
            if (Tab* tab = wxGetApp().get_tab(Preset::TYPE_FILAMENT))
                tab->save_preset();
        EndModal(wxID_OK);
    };
    corpo->Add(rodape, 0, wxEXPAND | wxALL, FromDIP(14));

    SetSizer(corpo);
    tamanho_da_janela(this, 560, 880);
    Layout();
}

JanelaAjustesFilamento::~JanelaAjustesFilamento() = default;

}}} // namespace Slic3r::GUI::FilunaFilamento
