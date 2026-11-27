#include "FilunaAutoajuda.hpp"
#include "FilunaIdioma.hpp"

#include "GUI_App.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"

#include <boost/filesystem.hpp>
#include <wx/display.h>
#include <wx/image.h>
#include <wx/popupwin.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/weakref.h>
#include <boost/nowide/fstream.hpp>
#include <nlohmann/json.hpp>

#include <map>
#include <mutex>

namespace Slic3r { namespace GUI { namespace FilunaAutoajuda {

namespace {

struct Partes
{
    std::string titulo, oque, efeito;
};
std::map<std::string, Partes>& partes_m()
{
    static std::map<std::string, Partes> m;
    return m;
}

const std::map<std::string, wxString>& cartoes()
{
    static std::map<std::string, wxString> m;
    static std::once_flag                  uma_vez;
    std::call_once(uma_vez, [] {
        try {
            // Programa em inglês: en_US.json (mesmas chaves); sem ele, o português.
            const boost::filesystem::path pasta = boost::filesystem::path(resources_dir()) / "filuna" / "autoajuda";
            const boost::filesystem::path p     = Filuna::ingles() && boost::filesystem::exists(pasta / "en_US.json") ? pasta / "en_US.json" :
                                                                                                                       pasta / "pt_BR.json";
            boost::nowide::ifstream       in(p.string());
            const nlohmann::json          j = nlohmann::json::parse(in, nullptr, false);
            if (!j.is_object())
                return;
            for (auto it = j.begin(); it != j.end(); ++it) {
                if (!it.value().is_object())
                    continue;
                const std::string titulo = it.value().value("titulo", std::string()), oque = it.value().value("oque", std::string()),
                                  efeito = it.value().value("efeito", std::string());
                if (titulo.empty() || oque.empty())
                    continue;
                std::string t = titulo + "\n\n" + oque;
                if (!efeito.empty())
                    t += "\n\n" + efeito;
                m[it.key()]         = wxString::FromUTF8(t);
                partes_m()[it.key()] = {titulo, oque, efeito};
            }
        } catch (const std::exception&) {}
    });
    return m;
}

// ---- cartão com imagem ----
std::string sem_indice(const std::string& k) { return k.substr(0, k.find('#')); }

boost::filesystem::path imagem_de(const std::string& chave)
{
    return boost::filesystem::path(resources_dir()) / "filuna" / "autoajuda" / "img" / (chave + ".png");
}

class Cartao : public wxPopupWindow
{
public:
    Cartao(wxWindow* pai, const Partes& p, const boost::filesystem::path& img) : wxPopupWindow(pai, wxBORDER_SIMPLE)
    {
        SetBackgroundColour(wxColour(255, 253, 249));
        const int larg = FromDIP(300), m = FromDIP(12);
        auto*     s    = new wxBoxSizer(wxVERTICAL);
        wxImage   im;
        if (im.LoadFile(wxString::FromUTF8(img.string()), wxBITMAP_TYPE_PNG) && im.IsOk()) {
            im.Rescale(larg, larg * im.GetHeight() / std::max(1, im.GetWidth()), wxIMAGE_QUALITY_HIGH);
            s->Add(new wxStaticBitmap(this, wxID_ANY, wxBitmap(im)), 0, wxLEFT | wxRIGHT | wxTOP, m);
        }
        auto* t = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(p.titulo));
        t->SetFont(t->GetFont().Bold().Scaled(1.15f));
        t->SetForegroundColour(wxColour(179, 18, 79));
        s->Add(t, 0, wxLEFT | wxRIGHT | wxTOP, m);
        std::string corpo = p.oque;
        if (!p.efeito.empty())
            corpo += "\n\n" + p.efeito;
        auto* c = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(corpo));
        c->SetForegroundColour(wxColour(60, 55, 50));
        c->Wrap(larg);
        s->Add(c, 0, wxALL, m);
        SetSizerAndFit(s);
    }
};

wxWeakRef<wxWindow> g_alvo;
std::string         g_chave;
Cartao*             g_cartao = nullptr;

void esconder()
{
    if (g_cartao != nullptr) {
        g_cartao->Destroy();
        g_cartao = nullptr;
    }
}

bool mouse_dentro(wxWindow* w)
{
    return w != nullptr && w->IsShownOnScreen() && w->GetScreenRect().Contains(wxGetMousePosition());
}

wxTimer& relogio()
{
    static wxTimer* t = [] {
        auto* r = new wxTimer();
        r->Bind(wxEVT_TIMER, [](wxTimerEvent&) {
            wxWindow* w = g_alvo.get();
            if (w == nullptr || !ligada() || !mouse_dentro(w))
                return;
            const auto it = partes_m().find(g_chave);
            if (it == partes_m().end())
                return;
            wxWindow* janela = wxGetTopLevelParent(w);
            if (janela == nullptr || !janela->IsShownOnScreen())
                return;
            esconder();
            g_cartao        = new Cartao(janela, it->second, imagem_de(g_chave));
            wxPoint       p = wxGetMousePosition() + wxPoint(w->FromDIP(14), w->FromDIP(18));
            const wxSize  t = g_cartao->GetSize();
            const int     d = wxDisplay::GetFromPoint(p);
            const wxRect  a = wxDisplay(d == wxNOT_FOUND ? 0 : d).GetClientArea();
            if (p.x + t.x > a.GetRight())
                p.x = std::max(a.GetLeft(), a.GetRight() - t.x);
            if (p.y + t.y > a.GetBottom())
                p.y = std::max(a.GetTop(), wxGetMousePosition().y - t.y - w->FromDIP(10));
            g_cartao->Position(p, wxSize(0, 0));
            g_cartao->Show();
        });
        return r;
    }();
    return *t;
}

void sem_dica(wxWindow* w)
{
    w->UnsetToolTip();
    for (wxWindow* f : w->GetChildren())
        sem_dica(f);
}

void prender(wxWindow* raiz, wxWindow* w, const std::string& chave)
{
    w->Bind(wxEVT_ENTER_WINDOW, [raiz, chave](wxMouseEvent& e) {
        e.Skip();
        if (!ligada())
            return;
        sem_dica(raiz); // o campo pode ter posto a dica de novo ao mudar o valor
        if (g_alvo.get() != raiz || g_cartao == nullptr) {
            esconder();
            g_alvo  = raiz;
            g_chave = chave;
            relogio().StartOnce(450);
        }
    });
    w->Bind(wxEVT_LEAVE_WINDOW, [raiz](wxMouseEvent& e) {
        e.Skip();
        if (mouse_dentro(raiz))
            return; // passou para uma parte de dentro do mesmo campo
        relogio().Stop();
        esconder();
        if (g_alvo.get() == raiz)
            g_alvo = nullptr;
    });
    w->Bind(wxEVT_LEFT_DOWN, [](wxMouseEvent& e) {
        e.Skip();
        relogio().Stop();
        esconder();
    });
    for (wxWindow* f : w->GetChildren())
        prender(raiz, f, chave);
}

} // namespace

void ligar_cartao(wxWindow* campo, const std::string& opt_key)
{
    if (campo == nullptr || !ligada())
        return;
    const std::string chave = sem_indice(opt_key);
    cartoes(); // carrega os textos
    boost::system::error_code ec;
    if (!partes_m().count(chave) || !boost::filesystem::exists(imagem_de(chave), ec))
        return;
    sem_dica(campo);
    prender(campo, campo, chave);
}

bool ligada()
{
    AppConfig* cfg = wxGetApp().app_config;
    return cfg == nullptr || cfg->get("filuna_pref_interface_autoajuda") != "false";
}

void ligar(bool sim)
{
    if (AppConfig* cfg = wxGetApp().app_config)
        cfg->set("filuna_pref_interface_autoajuda", sim ? "true" : "false");
}

wxString cartao(const std::string& opt_key)
{
    if (!ligada())
        return wxString();
    // "nozzle_temperature#0" -> "nozzle_temperature"
    const std::string chave = opt_key.substr(0, opt_key.find('#'));
    const auto&       m     = cartoes();
    const auto        it    = m.find(chave);
    return it == m.end() ? wxString() : it->second;
}

std::string cartao_ferramenta(const std::string& chave)
{
    if (!ligada())
        return {};
    std::string k = chave;
    for (const char* fim : {".svg", "_dark"}) {
        const std::string f(fim);
        if (k.size() > f.size() && k.compare(k.size() - f.size(), f.size(), f) == 0)
            k.resize(k.size() - f.size());
    }
    const auto& m  = cartoes();
    const auto  it = m.find(k);
    return it == m.end() ? std::string() : std::string(it->second.ToUTF8().data());
}

}}} // namespace Slic3r::GUI::FilunaAutoajuda
