#include "FilunaImpressao.hpp"
#include "libslic3r/miniz_extension.hpp"
#include "FilunaAoVivo.hpp"

#include "FilunaManutencao.hpp"
#include "slic3r/Filuna/FilunaAnalise.hpp"
#include "slic3r/Filuna/FilunaAjuste.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "MsgDialog.hpp"
#include "FilunaEnvio.hpp"
#include "PartPlate.hpp"
#include "Plater.hpp"
#include "libslic3r/Model.hpp"
#include "DeviceManager.hpp"
#include "DeviceCore/DevManager.h"
#include "DeviceCore/DevFilaSystem.h"
#include "DeviceCore/DevBed.h"
#include "DeviceCore/DevExtruderSystem.h"
#include "DeviceCore/DevFan.h"
#include "DeviceCore/DevPrintOptions.h"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "slic3r/Utils/NetworkAgent.hpp"
#include "slic3r/Utils/FilunaCameraUrls.hpp"
#include <ctime>
#include "slic3r/Utils/bambu_networking.hpp"
#include "slic3r/Utils/Http.hpp"
#include "NotificationManager.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r_version.h"
#include <boost/filesystem/fstream.hpp>
#include <boost/nowide/convert.hpp>
#include <cmath>
#include <map>
#include <wx/stdpaths.h>
#include <wx/mstream.h>
#include <wx/statbmp.h>
#include <wx/utils.h>
#include <wx/filedlg.h>
#include <chrono>
#include <future>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <nlohmann/json.hpp>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/utils.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace Slic3r { namespace GUI { namespace FilunaImpressao {

using json = nlohmann::json;

namespace {

std::string agente_ativo()
{
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return {};
    return wxGetApp().resolve_printer_agent_id(bundle->printers.get_edited_preset().config.opt_string("printer_agent"));
}

struct Entrada // uma entrada do sistema de cores da impressora
{
    int         global{0}; // unidade*4 + slot (o que os agentes esperam em ams_mapping)
    std::string tipo;      // PLA, PETG…
    std::string cor;       // RRGGBB
};

struct Filamento // um filamento do projeto usado nesta mesa
{
    int         indice{0}; // 0 = primeiro filamento do projeto
    std::string tipo;
    std::string cor; // RRGGBB
};

std::string so_rgb(std::string c)
{
    if (!c.empty() && c[0] == '#')
        c.erase(0, 1);
    if (c.size() > 6)
        c.resize(6);
    return boost::to_upper_copy(c);
}

wxColour cor_wx(const std::string& rgb)
{
    if (rgb.size() != 6)
        return wxColour(206, 206, 206);
    const long v = std::strtol(rgb.c_str(), nullptr, 16);
    return wxColour((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

int distancia(const std::string& a, const std::string& b)
{
    if (a.size() != 6 || b.size() != 6)
        return 3 * 255 * 255;
    const long x = std::strtol(a.c_str(), nullptr, 16), y = std::strtol(b.c_str(), nullptr, 16);
    const int  dr = int((x >> 16) & 0xFF) - int((y >> 16) & 0xFF), dg = int((x >> 8) & 0xFF) - int((y >> 8) & 0xFF),
               db = int(x & 0xFF) - int(y & 0xFF);
    return dr * dr + dg * dg + db * db;
}

std::vector<Entrada> entradas_de(MachineObject* o)
{
    std::vector<Entrada> es;
    auto                 fila = o->GetFilaSystem();
    if (!fila)
        return es;
    for (auto& [ams_id, ams] : fila->GetAmsList()) {
        if (ams == nullptr)
            continue;
        for (auto& [tray_id, tray] : ams->GetTrays()) {
            if (tray == nullptr || (tray->color.empty() && tray->m_fila_type.empty()))
                continue; // entrada vazia
            int a = 0, s = 0;
            try {
                a = std::stoi(ams_id), s = std::stoi(tray_id);
            } catch (...) {
                continue;
            }
            es.push_back({a * 4 + s, tray->m_fila_type, so_rgb(tray->color)});
        }
    }
    return es;
}

std::vector<Filamento> filamentos_da_mesa(PartPlate* plate)
{
    std::vector<Filamento> fs;
    PresetBundle*          bundle = wxGetApp().preset_bundle;
    const auto*            cores  = bundle->project_config.option<ConfigOptionStrings>("filament_colour");
    std::vector<int>       usados = plate->get_extruders(true);
    std::sort(usados.begin(), usados.end());
    usados.erase(std::unique(usados.begin(), usados.end()), usados.end());
    for (int e : usados) {
        const int i = e - 1;
        if (i < 0 || i >= int(bundle->filament_presets.size()))
            continue;
        Filamento f;
        f.indice = i;
        if (const Preset* p = bundle->filaments.find_preset(bundle->filament_presets[i]))
            f.tipo = p->config.opt_string("filament_type", 0u);
        if (cores != nullptr && i < int(cores->values.size()))
            f.cor = so_rgb(cores->values[i]);
        fs.push_back(f);
    }
    return fs;
}

// melhor entrada para um filamento: mesmo material primeiro, depois a cor mais próxima
int melhor_entrada(const Filamento& f, const std::vector<Entrada>& es)
{
    int melhor = -1, nota = 0;
    for (std::size_t k = 0; k < es.size(); ++k) {
        int n = distancia(f.cor, es[k].cor);
        if (!f.tipo.empty() && !boost::iequals(f.tipo, es[k].tipo))
            n += 4 * 255 * 255; // material diferente perde para qualquer cor do material certo
        if (melhor < 0 || n < nota)
            melhor = int(k), nota = n;
    }
    return melhor;
}

std::string nome_arquivo()
{
    std::string nome;
    if (Plater* pl = wxGetApp().plater())
        nome = std::string(pl->get_export_gcode_filename(".gcode", true).ToUTF8().data());
    for (const char* ext : {".gcode", ".3mf"})
        if (boost::iends_with(nome, ext))
            nome.resize(nome.size() - std::strlen(ext));
    std::string limpo;
    for (unsigned char c : nome)
        limpo += (std::isalnum(c) || c == '-' || c == '_' || c == '.') ? char(c) : '_';
    while (!limpo.empty() && (limpo.front() == '_' || limpo.front() == '.'))
        limpo.erase(0, 1);
    if (limpo.size() > 60)
        limpo.resize(60);
    if (limpo.empty())
        limpo = "filuna";
    return limpo + ".gcode";
}

void avisar(const wxString& texto, long estilo = wxOK | wxICON_INFORMATION)
{
    MessageDialog dlg(wxGetApp().mainframe, texto, wxString::FromUTF8("Imprimir"), estilo);
    dlg.ShowModal();
}

// Agentes cujo protocolo imprime o filamento N do projeto pela entrada N (sem remapeamento).
// Anycubic Modo LAN: a referência só registra mapeamento idêntico, e na Kobra 3 V2 a impressora usou a entrada
// do número do filamento mesmo com outra escolhida.
bool entrada_fixa(const std::string& agente) { return agente == "filuna_anycubic_lan"; }

struct Escolha
{
    bool             imprimir{true}; // false = só enviar
    bool             nivelar{true};
    std::vector<int> entrada;        // por filamento usado: índice em entradas, -1 = nenhuma
};

class Janela : public wxDialog
{
public:
    Janela(wxWindow* pai, const std::string& impressora, const std::string& arquivo, const std::vector<Filamento>& fs,
           const std::vector<Entrada>& es, bool fixa = false)
        : wxDialog(pai, wxID_ANY, wxString::FromUTF8("Imprimir"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE), m_es(es)
    {
        SetBackgroundColour(*wxWHITE);
        auto* v = new wxBoxSizer(wxVERTICAL);

        auto* titulo = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Imprimir em " + impressora));
        wxFont ft    = titulo->GetFont();
        ft.SetWeight(wxFONTWEIGHT_BOLD);
        ft.SetPointSize(ft.GetPointSize() + 2);
        titulo->SetFont(ft);
        v->Add(titulo, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
        v->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Arquivo: " + arquivo)), 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));

        if (!es.empty() && !fs.empty()) {
            v->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Entrada usada por cada filamento do projeto")), 0,
                   wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
            auto* g = new wxFlexGridSizer(4, FromDIP(8), FromDIP(10));
            wxArrayString nomes;
            for (const Entrada& e : es)
                nomes.Add(wxString::FromUTF8("Entrada " + std::to_string(e.global + 1) + (e.tipo.empty() ? "" : " · " + e.tipo)));
            for (const Filamento& f : fs) {
                auto* amostra = new wxPanel(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(18, 18)), wxBORDER_SIMPLE);
                amostra->SetBackgroundColour(cor_wx(f.cor));
                g->Add(amostra, 0, wxALIGN_CENTER_VERTICAL);
                g->Add(new wxStaticText(this, wxID_ANY,
                                        wxString::FromUTF8("Filamento " + std::to_string(f.indice + 1) + (f.tipo.empty() ? "" : " · " + f.tipo))),
                       0, wxALIGN_CENTER_VERTICAL);
                auto* escolha = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, nomes);
                int k = melhor_entrada(f, es);
                if (fixa) { // o protocolo imprime o filamento N pela entrada N: não há o que escolher
                    for (std::size_t q = 0; q < es.size(); ++q)
                        if (es[q].global == f.indice)
                            k = int(q);
                    escolha->Disable();
                }
                escolha->SetSelection(k < 0 ? 0 : k);
                auto* destino = new wxPanel(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(18, 18)), wxBORDER_SIMPLE);
                destino->SetBackgroundColour(cor_wx(es[k < 0 ? 0 : k].cor));
                escolha->Bind(wxEVT_CHOICE, [this, escolha, destino](wxCommandEvent&) {
                    const int s = escolha->GetSelection();
                    if (s >= 0 && s < int(m_es.size())) {
                        destino->SetBackgroundColour(cor_wx(m_es[s].cor));
                        destino->Refresh();
                    }
                });
                g->Add(escolha, 0, wxALIGN_CENTER_VERTICAL);
                g->Add(destino, 0, wxALIGN_CENTER_VERTICAL);
                m_escolhas.push_back(escolha);
            }
            v->Add(g, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
            if (fixa)
                v->Add(new wxStaticText(this, wxID_ANY,
                                        wxString::FromUTF8("Nesta impressora o filamento 1 do projeto sai da Entrada 1, o 2 da Entrada 2, e assim por diante.\n"
                                                           "Para mudar, use \"Usar no projeto\" no cartão da impressora.")),
                       0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
        } else if (es.empty()) {
            v->Add(new wxStaticText(this, wxID_ANY,
                                    wxString::FromUTF8("A impressora não informou um sistema de cores.\nSerá usado o filamento que já está carregado nela.")),
                   0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));
        }

        m_nivelar = new wxCheckBox(this, wxID_ANY, wxString::FromUTF8("Nivelar a mesa antes de imprimir"));
        m_nivelar->SetValue(wxGetApp().app_config->get("filuna_imprimir_nivelar") != "0");
        v->Add(m_nivelar, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(18));

        auto* b      = new wxBoxSizer(wxHORIZONTAL);
        auto* cancel = new wxButton(this, wxID_CANCEL, wxString::FromUTF8("Cancelar"));
        auto* enviar = new wxButton(this, wxID_APPLY, wxString::FromUTF8("Só enviar o arquivo"));
        auto* ok     = new wxButton(this, wxID_OK, wxString::FromUTF8("Imprimir"));
        ok->SetDefault();
        enviar->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_APPLY); });
        b->AddStretchSpacer(1);
        b->Add(cancel, 0, wxRIGHT, FromDIP(8));
        b->Add(enviar, 0, wxRIGHT, FromDIP(8));
        b->Add(ok, 0);
        v->Add(b, 0, wxEXPAND | wxALL, FromDIP(18));

        SetSizerAndFit(v);
        SetMinSize(wxSize(FromDIP(420), -1));
        Fit();
        CentreOnParent();
        wxGetApp().UpdateDlgDarkUI(this);
    }

    Escolha escolha() const
    {
        Escolha e;
        e.nivelar = m_nivelar->GetValue();
        for (wxChoice* c : m_escolhas)
            e.entrada.push_back(c->GetSelection());
        return e;
    }

private:
    std::vector<Entrada>   m_es;
    std::vector<wxChoice*> m_escolhas;
    wxCheckBox*            m_nivelar{nullptr};
};


// Tempo, peso, custo, altura da camada e nome da mesa, tirados do fatiamento que está na tela: são os mesmos
// números do rodapé da Preparação, para a tela de conferir não contar diferente do resto do programa.
ResumoMesa resumo_da_mesa(const std::string& arquivo)
{
    ResumoMesa r;
    r.arquivo = arquivo;

    Plater*       plater = wxGetApp().plater();
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (plater == nullptr || bundle == nullptr)
        return r;

    if (const auto* o = bundle->prints.get_edited_preset().config.option<ConfigOptionFloat>("layer_height"))
        r.altura_camada = o->value;
    if (const auto* o = bundle->project_config.option<ConfigOptionEnum<BedType>>("curr_bed_type"))
        r.mesa = bed_type_to_gcode_string(o->value);

    PartPlate* plate = plater->get_partplate_list().get_curr_plate();
    if (plate == nullptr || !plate->is_slice_result_valid() || plate->get_slice_result() == nullptr)
        return r;

    const auto& stats = plate->get_slice_result()->print_statistics;
    std::map<size_t, double> volumes;
    for (const auto* m : {&stats.model_volumes_per_extruder, &stats.support_volumes_per_extruder, &stats.wipe_tower_volumes_per_extruder})
        for (const auto& [id, v] : *m)
            volumes[id] += v;
    for (const auto& [id, v] : volumes) {
        if (v <= 0.)
            continue;
        double densidade = 1.24, preco_kg = 0.;
        if (id < bundle->filament_presets.size())
            if (const Preset* p = bundle->filaments.find_preset(bundle->filament_presets[id])) {
                if (const auto* o = p->config.option<ConfigOptionFloats>("filament_density"); o && !o->values.empty() && o->values.front() > 0.)
                    densidade = o->values.front();
                if (const auto* o = p->config.option<ConfigOptionFloats>("filament_cost"); o && !o->values.empty())
                    preco_kg = o->values.front();
            }
        const double g = v * densidade / 1000.;
        r.gramas += g;
        r.custo += g * preco_kg / 1000.;
    }
    const float t = stats.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)].time;
    if (t > 0.f)
        r.tempo_s = static_cast<long long>(std::llround(t));
    return r;
}

struct Andamento
{
    std::mutex        m;
    int               etapa{PrintingStageCreate};
    int               pct{0};
    std::string       msg;
    int               rc{-1};
    std::atomic<bool> fim{false};
    std::atomic<bool> cancelar{false};
};

} // namespace

bool agente_filuna()
{
    if (wxGetApp().preset_bundle == nullptr)
        return false;
    // Conectou pelo Filuna (agente da marca)? Então imprime pelas telas do Filuna, seja qual for a marca —
    // inclusive Bambu. Sem isso o cliente via a janela do Orca numa marca e a do Filuna na outra.
    return boost::starts_with(agente_ativo(), "filuna_");
}

bool agente_entrada_fixa() { return entrada_fixa(agente_ativo()); }

std::string filamento_id_do_tipo(const std::string& tipo, const std::string& marca)
{
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr || tipo.empty())
        return {};
    const Preset&     impressora = bundle->printers.get_edited_preset();
    const std::string da_impressora = impressora.vendor != nullptr ? boost::to_lower_copy(impressora.vendor->id) : std::string();
    // palavras da marca lida do carretel: a primeira é o fabricante ("elegoo"), as demais a linha ("pla+", "silk"…)
    std::vector<std::string> palavras;
    {
        std::string m = boost::to_lower_copy(marca);
        boost::split(palavras, m, boost::is_any_of(" \t_-/"), boost::token_compress_on);
        palavras.erase(std::remove_if(palavras.begin(), palavras.end(), [](const std::string& w) { return w.size() < 2; }), palavras.end());
    }
    std::string melhor;
    int         nota_melhor = -1;
    for (const Preset& f : bundle->filaments) {
        if (!f.is_compatible || f.filament_id.empty() || bundle->filaments.get_preset_base(f) != &f)
            continue;
        if (!boost::iequals(f.config.opt_string("filament_type", 0u), tipo))
            continue;
        const std::string nome = boost::to_lower_copy(f.name);
        int               nota = 0;
        if (!palavras.empty() && nome.find(palavras.front()) != std::string::npos) {
            nota += 100; // perfil da marca do filamento
            for (std::size_t k = 1; k < palavras.size(); ++k)
                if (nome.find(palavras[k]) != std::string::npos)
                    nota += 10; // e da mesma linha
        }
        if (!da_impressora.empty() && boost::starts_with(nome, da_impressora))
            nota += 5;
        if (boost::starts_with(nome, "generic"))
            nota += 2;
        if (nota > nota_melhor)
            nota_melhor = nota, melhor = f.filament_id;
    }
    return melhor;
}

void imprimir(int plate_idx)
{
    namespace fs = boost::filesystem;
    Plater*        plater = wxGetApp().plater();
    DeviceManager* dev    = wxGetApp().getDeviceManager();
    NetworkAgent*  net    = wxGetApp().getAgent();
    if (plater == nullptr || dev == nullptr || net == nullptr)
        return;

    const std::string agente = agente_ativo();
    MachineObject*    o      = dev->get_selected_machine();
    if (o == nullptr || o->printer_agent_id != agente) {
        avisar(wxString::FromUTF8("Nenhuma impressora deste modelo está conectada.\n\nAbra a aba Dispositivo, clique na impressora e volte para imprimir."),
               wxOK | wxICON_WARNING);
        return;
    }
    if (!o->is_online()) {
        avisar(wxString::FromUTF8("A impressora " + o->get_dev_name() + " está desligada ou fora da rede."), wxOK | wxICON_WARNING);
        return;
    }
    if (o->is_in_printing()) {
        avisar(wxString::FromUTF8("A impressora " + o->get_dev_name() + " já está imprimindo."), wxOK | wxICON_WARNING);
        return;
    }

    // Conferência da mesa pela câmera (IA): se a última, de até 15 minutos atrás, viu a mesa ocupada, pergunta
    // antes de enviar. Vale para qualquer marca com câmera.
    {
        const json v = json::parse(Filuna::mesa(o->get_dev_id()), nullptr, false);
        if (v.is_object() && !v.value("livre", true) && (long long) std::time(nullptr) - v.value("em", 0LL) < 15 * 60) {
            MessageDialog dlg(wxGetApp().mainframe,
                              wxString::FromUTF8("A câmera mostrou a mesa ocupada: " + v.value("motivo", std::string("há algo na mesa")) +
                                                 ".\n\nRetire as peças e limpe a mesa. Imprimir assim mesmo?"),
                              wxString::FromUTF8("Mesa ocupada"), wxYES_NO | wxICON_WARNING);
            if (dlg.ShowModal() != wxID_YES)
                return;
        }
    }

    PartPlateList& mesas = plater->get_partplate_list();
    PartPlate*     mesa  = plate_idx >= 0 ? mesas.get_plate(plate_idx) : mesas.get_curr_plate();
    if (mesa == nullptr)
        mesa = mesas.get_curr_plate();
    if (mesa == nullptr || !mesa->is_slice_result_valid()) {
        avisar(wxString::FromUTF8("Fatie a mesa antes de imprimir."), wxOK | wxICON_WARNING);
        return;
    }
    const fs::path origem(mesa->get_tmp_gcode_path());
    boost::system::error_code ec;
    if (!fs::exists(origem, ec)) {
        FilunaManutencao::diagnostico("impressao", "G-code da mesa não encontrado", json{{"caminho", origem.string()}, {"agente", agente}});
        avisar(wxString::FromUTF8("O G-code desta mesa não foi encontrado. Fatie de novo e tente outra vez."), wxOK | wxICON_ERROR);
        return;
    }

    const std::vector<Entrada>   es   = entradas_de(o);
    const std::vector<Filamento> fila = filamentos_da_mesa(mesa);
    const std::string            nome = nome_arquivo();

    // Protocolos que não remapeiam: o filamento N do projeto sai da entrada N do sistema de cores. A impressora
    // ignora outro mapeamento e imprimiria pela entrada errada (até vazia). Confere antes de enviar.
    const bool fixa = entrada_fixa(agente) && !es.empty();
    if (fixa) {
        for (const Filamento& f : fila) {
            const Entrada* e = nullptr;
            for (const Entrada& x : es)
                if (x.global == f.indice)
                    e = &x;
            const std::string n = std::to_string(f.indice + 1);
            if (e == nullptr) {
                FilunaManutencao::diagnostico("impressao", "entrada " + n + " vazia para o filamento " + n + " do projeto", json{{"agente", agente}});
                avisar(wxString::FromUTF8("O filamento " + n + " do projeto sai da Entrada " + n + " da impressora, e ela está vazia.\n\n"
                                          "Coloque o filamento na Entrada " + n + ", ou abra o cartão da impressora no Dispositivo e clique em "
                                          "\"Usar no projeto\" para alinhar os filamentos do projeto com as entradas."),
                       wxOK | wxICON_WARNING);
                return;
            }
            if (!f.tipo.empty() && !e->tipo.empty() && !boost::iequals(f.tipo, e->tipo)) {
                MessageDialog dlg(wxGetApp().mainframe,
                                  wxString::FromUTF8("O filamento " + n + " do projeto é " + f.tipo + ", mas a Entrada " + n + " da impressora tem " + e->tipo +
                                                     ".\n\nImprimir assim mesmo?"),
                                  wxString::FromUTF8("Material diferente"), wxYES_NO | wxICON_WARNING);
                if (dlg.ShowModal() != wxID_YES)
                    return;
            }
        }
    }

    // Primeira tela: conferir arquivo, tempo, peso, custo, mesa e para qual entrada vai cada filamento.
    std::vector<ItemFilamento> itens_fil;
    for (const Filamento& f : fila)
        itens_fil.push_back({f.indice, f.tipo, "#" + f.cor, 0.});
    std::vector<ItemEntrada> itens_ent;
    for (const Entrada& e : es)
        itens_ent.push_back({e.global, e.tipo, "#" + e.cor});

    JanelaConfirmar confirmar(wxGetApp().mainframe, wxString::FromUTF8(o->get_dev_name()), resumo_da_mesa(nome), itens_fil, itens_ent,
                              fixa);
    const int resposta = confirmar.ShowModal();
    if (resposta != wxID_OK && resposta != wxID_APPLY)
        return;
    Escolha esc;
    esc.nivelar           = confirmar.nivelar();
    esc.entrada           = confirmar.entradas_escolhidas();
    const bool so_envio   = resposta == wxID_APPLY;
    wxGetApp().app_config->set("filuna_imprimir_nivelar", esc.nivelar ? "1" : "0");

    // cópia com o nome que a impressora vai mostrar (o arquivo da mesa é temporário e tem nome interno)
    const fs::path pasta = fs::temp_directory_path(ec) / "filuna-envio";
    fs::create_directories(pasta, ec);
    const fs::path envio = pasta / nome;
    fs::copy_file(origem, envio, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        FilunaManutencao::diagnostico("impressao", "não copiou o G-code para envio: " + ec.message(),
                                      json{{"de", origem.string()}, {"para", envio.string()}});
        avisar(wxString::FromUTF8("Não foi possível preparar o arquivo para envio: " + ec.message()), wxOK | wxICON_ERROR);
        return;
    }

    // posição = filamento do projeto; valor = entrada global; -1 = não usado nesta mesa
    json mapa = json::array();
    if (!es.empty() && !fila.empty()) {
        const std::size_t total = wxGetApp().preset_bundle->filament_presets.size();
        for (std::size_t i = 0; i < total; ++i)
            mapa.push_back(-1);
        for (std::size_t k = 0; k < fila.size() && k < esc.entrada.size(); ++k) {
            const int s = esc.entrada[k];
            if (s >= 0 && s < int(es.size()) && fila[k].indice < int(mapa.size()))
                mapa[fila[k].indice] = es[s].global;
        }
    }

    PrintParams p;
    p.dev_id                      = o->get_dev_id();
    p.dev_ip                      = o->get_dev_ip();
    p.dev_name                    = o->get_dev_name();
    p.password                    = o->get_access_code();
    p.connection_type             = "lan";
    p.filename                    = envio.string();
    p.project_name                = envio.stem().string();
    p.task_name                   = p.project_name;
    p.plate_index                 = 1;
    p.ams_mapping                 = mapa.empty() ? std::string() : mapa.dump();
    p.use_ssl_for_ftp             = false;
    p.use_ssl_for_mqtt            = false;
    p.task_bed_leveling           = esc.nivelar;
    p.task_flow_cali              = false;
    p.task_vibration_cali         = false;
    p.task_layer_inspect          = false;
    p.task_record_timelapse       = false;
    p.task_timelapse_use_internal = false;
    p.task_use_ams                = !mapa.empty();
    p.task_ext_change_assist      = false;
    p.try_emmc_print              = false;

    auto and_ = std::make_shared<Andamento>();
    std::thread tarefa([and_, p, net, so_envio]() {
        auto atualiza = [and_](int etapa, int codigo, std::string msg) {
            std::lock_guard<std::mutex> l(and_->m);
            and_->etapa = etapa;
            if (etapa == PrintingStageUpload)
                and_->pct = codigo;
            if (etapa == PrintingStageERROR && !msg.empty())
                and_->msg = msg;
        };
        auto cancelou = [and_]() { return and_->cancelar.load(); };
        int  rc       = -1;
        try {
            rc = so_envio ? net->start_send_gcode_to_sdcard(p, atualiza, cancelou, nullptr) : net->start_local_print(p, atualiza, cancelou);
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> l(and_->m);
            and_->msg = e.what();
        } catch (...) {
            std::lock_guard<std::mutex> l(and_->m);
            and_->msg = "erro inesperado no envio";
        }
        {
            std::lock_guard<std::mutex> l(and_->m);
            and_->rc = rc;
        }
        and_->fim = true;
    });

    // A janela do Filuna: os passos à esquerda, a impressora desenhada à direita com o bico e a mesa esquentando.
    auto* jenvio = new JanelaEnvio(wxGetApp().mainframe, o->get_dev_id(), wxString::FromUTF8(o->get_dev_name()),
                                   wxString::FromUTF8(nome));
    jenvio->Show();
    jenvio->passo(JanelaEnvio::Passo::PREPARANDO);
    {
        int ultimo = -2;
        while (!and_->fim) {
            wxMilliSleep(60);
            wxYield(); // a janela precisa continuar desenhando e respondendo ao cliente
            int etapa, pct;
            {
                std::lock_guard<std::mutex> l(and_->m);
                etapa = and_->etapa, pct = and_->pct;
            }
            if (etapa == PrintingStageUpload)
                jenvio->passo(JanelaEnvio::Passo::ENVIANDO, std::max(0, std::min(pct, 99)));
            else if (etapa == PrintingStageSending && ultimo != etapa)
                jenvio->passo(JanelaEnvio::Passo::FALANDO);
            else if (ultimo != etapa)
                jenvio->passo(JanelaEnvio::Passo::PREPARANDO);
            ultimo = etapa;
            if (jenvio->cancelou())
                and_->cancelar = true;
        }
    }
    tarefa.join();
    jenvio->passo(JanelaEnvio::Passo::CONFERINDO);
    fs::remove(envio, ec);

    int         rc;
    std::string msg;
    {
        std::lock_guard<std::mutex> l(and_->m);
        rc = and_->rc, msg = and_->msg;
    }
    if (rc == BAMBU_NETWORK_SUCCESS) {
        if (!so_envio)
#ifdef _WIN32
            analise_comecar(o->get_dev_id(), boost::nowide::narrow(origem.wstring()), nome, o->get_dev_name());
#else
            analise_comecar(o->get_dev_id(), origem.string(), nome, o->get_dev_name());
#endif
        BOOST_LOG_TRIVIAL(info) << "FilunaImpressao: " << (so_envio ? "arquivo enviado" : "impressão pedida") << " agente=" << agente
                                << " arquivo=" << nome << " mapa=" << p.ams_mapping;
        jenvio->passo(so_envio ? JanelaEnvio::Passo::CONFERINDO : JanelaEnvio::Passo::AQUECENDO);
        jenvio->concluir(true, wxString::FromUTF8(so_envio ? "O arquivo está na impressora." : std::string()));
        return;
    }
    if (and_->cancelar) {
        jenvio->concluir(false, wxString::FromUTF8("Envio cancelado por você."));
        return;
    }
    FilunaManutencao::diagnostico("impressao", msg.empty() ? "falha ao enviar/imprimir (código " + std::to_string(rc) + ")" : msg,
                                  json{{"agente", agente}, {"codigo", rc}, {"so_envio", so_envio}, {"arquivo", nome},
                                       {"mapa", p.ams_mapping}, {"ip", p.dev_ip}});
    jenvio->concluir(false, wxString::FromUTF8("Não foi possível " + std::string(so_envio ? "enviar o arquivo" : "iniciar a impressão") +
                                               ". " + (msg.empty() ? "Código " + std::to_string(rc) : msg) +
                                               "\nO erro foi enviado ao suporte da Filuna."));
}

// ============================================================================================================
// Filuna Farm: o Slicer imprime as mesas da fila do site (docs/farm.md e docs/farm-wizard.md do site).
// A cada 20 s pede a fila; a primeira mesa liberada (pode_iniciar) da impressora conectada aqui é baixada,
// fatiada com os perfis da impressora (o próprio programa sem tela, como o site faz) e enviada para imprimir.
// O andamento volta para o site (fatiado, enviado, imprimindo, concluído ou falhou), que dá a baixa no estoque e
// pede a mesa limpa antes da próxima. Pausar, retomar e cancelar pedidos no site/app chegam por /comandos.
// ============================================================================================================
namespace {
namespace fsf = boost::filesystem;
const char* const FARM_API = "https://filuna3d.com/api/slicer/v1";

// Caminhos em UTF-8 nos argumentos (pasta do usuário com acento no Windows não pode quebrar o fatiamento)
std::string utf8_de(const fsf::path& p)
{
#ifdef _WIN32
    return boost::nowide::narrow(p.wstring());
#else
    return p.string();
#endif
}
fsf::path caminho_de(const std::string& utf8)
{
#ifdef _WIN32
    return fsf::path(boost::nowide::widen(utf8));
#else
    return fsf::path(utf8);
#endif
}

struct FarmResp
{
    unsigned    status{0};
    json        corpo;
    std::string erro;
};

int inteiro(const json& j, const char* k, int padrao)
{
    if (!j.is_object() || !j.contains(k))
        return padrao;
    const json& v = j[k];
    return v.is_number() ? int(v.get<double>()) : padrao;
}

std::string texto_de(const json& j, const char* k, const std::string& padrao = std::string())
{
    return j.is_object() && j.contains(k) && j[k].is_string() ? j[k].get<std::string>() : padrao;
}

FarmResp farm_http(const std::string& metodo, const std::string& rota, const json& corpo, const std::string& token,
                   const fsf::path& salvar = fsf::path())
{
    FarmResp    r;
    std::string texto;
    Http        http = metodo == "POST" ? Http::post(std::string(FARM_API) + rota) : Http::get(std::string(FARM_API) + rota);
    http.tls_verify(true);
    http.timeout_connect(15);
    http.timeout_max(salvar.empty() ? 60 : 1800);
    http.header("Authorization", "Bearer " + token);
    if (metodo == "POST") {
        http.header("Content-Type", "application/json");
        http.set_post_body(corpo.dump());
    }
    http.on_complete([&](std::string b, unsigned st) { texto = std::move(b), r.status = st; })
        .on_error([&](std::string b, std::string e, unsigned st) { texto = std::move(b), r.status = st, r.erro = e.empty() ? "HTTP " + std::to_string(st) : e; })
        .perform_sync();
    if (!salvar.empty() && r.status == 200) {
        fsf::ofstream o(salvar, std::ios::binary | std::ios::trunc);
        o.write(texto.data(), std::streamsize(texto.size()));
        if (!o.good())
            r.status = 0, r.erro = "não gravou o arquivo da mesa";
        return r;
    }
    r.corpo = json::parse(texto, nullptr, false);
    if (r.status != 200 && r.erro.empty())
        r.erro = r.corpo.is_object() ? r.corpo.value("erro", std::string("HTTP ") + std::to_string(r.status)) : "HTTP " + std::to_string(r.status);
    return r;
}

// Roda uma função na linha da janela e espera o resultado (com limite, para não travar ao fechar o programa).
template<class T> bool na_janela(std::function<T()> fn, T& saida, int segundos = 20)
{
    auto prom = std::make_shared<std::promise<T>>();
    auto fut  = prom->get_future();
    wxGetApp().CallAfter([fn, prom] {
        try {
            prom->set_value(fn());
        } catch (...) {
            try { prom->set_exception(std::current_exception()); } catch (...) {}
        }
    });
    if (fut.wait_for(std::chrono::seconds(segundos)) != std::future_status::ready)
        return false;
    try {
        saida = fut.get();
        return true;
    } catch (...) {
        return false;
    }
}

// Impressora que a Farm pode usar: a selecionada no programa ou uma das acompanhadas ao vivo (FilunaAoVivo), cada
// uma pela própria conexão.
struct FarmMaquina
{
    bool        ok{false};
    bool        ao_vivo{false}; // true = acompanhada em segundo plano (não é a selecionada no programa)
    std::string dev_id, ref, agente, status, erro;
    bool        online{false}, imprimindo{false}, pausada{false};
    int         progresso{0}, resta_s{0};
    json        cores = json::array();
};

FarmMaquina farm_maquina()
{
    FarmMaquina m;
    DeviceManager* dev = wxGetApp().getDeviceManager();
    MachineObject* o   = dev != nullptr ? dev->get_selected_machine() : nullptr;
    if (o == nullptr)
        return m;
    m.ok         = true;
    m.dev_id     = o->get_dev_id();
    m.ref        = wxGetApp().app_config->get("filuna_dispositivo_" + m.dev_id);
    m.agente     = o->printer_agent_id;
    m.status     = o->print_status;
    m.online     = o->is_online();
    m.imprimindo = o->is_in_printing();
    m.pausada    = o->is_in_printing_pause();
    m.progresso  = o->mc_print_percent;
    m.resta_s    = o->mc_left_time;
    for (const Entrada& e : entradas_de(o))
        m.cores.push_back({{"global", e.global}, {"tipo", e.tipo}, {"cor", e.cor}});
    return m;
}

// Impressoras ao vivo para a Farm: a selecionada (como antes) e as acompanhadas em segundo plano. Linha da tela.
std::vector<FarmMaquina> farm_maquinas()
{
    std::vector<FarmMaquina> v;
    FarmMaquina              sel = farm_maquina();
    if (sel.ok)
        v.push_back(sel);
    for (const FilunaAoVivo::Ligada& l : FilunaAoVivo::ligadas()) {
        if (sel.ok && l.dev_id == sel.dev_id)
            continue;
        FarmMaquina m;
        const json& p     = l.relatorio;
        const std::string gs = p.value("gcode_state", std::string());
        m.ok         = true;
        m.ao_vivo    = true;
        m.dev_id     = l.dev_id;
        m.ref        = wxGetApp().app_config->get("filuna_dispositivo_" + l.dev_id);
        m.agente     = l.agente;
        m.status     = gs;
        m.online     = true;
        m.imprimindo = gs == "RUNNING" || gs == "PREPARE" || gs == "SLICING" || gs == "PAUSE";
        m.pausada    = gs == "PAUSE";
        m.progresso  = p.contains("mc_percent") && p["mc_percent"].is_number() ? p["mc_percent"].get<int>() : 0;
        m.resta_s    = p.contains("mc_remaining_time") && p["mc_remaining_time"].is_number() ? p["mc_remaining_time"].get<int>() * 60 : 0;
        if (p.contains("ams") && p["ams"].is_object() && p["ams"].contains("ams") && p["ams"]["ams"].is_array())
            for (const auto& u : p["ams"]["ams"]) {
                if (!u.is_object() || !u.contains("tray") || !u["tray"].is_array())
                    continue;
                int a = 0;
                try {
                    a = u.contains("id") ? (u["id"].is_string() ? std::stoi(u["id"].get<std::string>()) : u["id"].get<int>()) : 0;
                } catch (...) {
                    continue;
                }
                for (const auto& t : u["tray"]) {
                    if (!t.is_object())
                        continue;
                    const std::string cor = t.value("tray_color", std::string()), tipo = t.value("tray_type", std::string());
                    if (cor.empty() && tipo.empty())
                        continue; // entrada vazia
                    int sl = 0;
                    try {
                        sl = t.contains("id") ? (t["id"].is_string() ? std::stoi(t["id"].get<std::string>()) : t["id"].get<int>()) : 0;
                    } catch (...) {
                        continue;
                    }
                    m.cores.push_back({{"global", a * 4 + sl}, {"tipo", tipo}, {"cor", so_rgb(cor)}});
                }
            }
        v.push_back(m);
    }
    return v;
}

// Perfis da impressora, do processo e do filamento para o fatiamento sem tela (arquivos JSON completos, sem herança).
struct FarmPerfis
{
    bool        ok{false};
    std::string erro, maquina, processo, filamento, agente;
    double      altura{0.2};
    PrintParams base;
};

bool gravar_perfil(const DynamicPrintConfig& cfg, const std::string& tipo, const std::string& nome, const fsf::path& arq)
{
    std::ostringstream os;
    cfg.save_to_json(os, nome, "User", SLIC3R_VERSION);
    json j = json::parse(os.str(), nullptr, false);
    if (!j.is_object())
        return false;
    j["type"] = tipo, j["from"] = "User", j["inherits"] = "", j["name"] = nome;
    fsf::ofstream o(arq, std::ios::binary | std::ios::trunc);
    o << j.dump();
    return o.good();
}

// Perfil de filamento do material para a impressora: entre os compatíveis, o do tipo pedido (PLA, PETG...),
// preferindo o mesmo filament_id que o programa escolheria, depois o de sistema, o "Basic" da marca e o genérico.
// Antes só valia o filament_id da impressora da Preparação, e a P2S ficava sem PETG quando a Preparação era outra.
const Preset* filamento_do_material(const std::string& material, const std::function<bool(const Preset&)>& compativel)
{
    PresetBundle* b = wxGetApp().preset_bundle;
    if (b == nullptr || material.empty())
        return nullptr;
    const std::string fid = filamento_id_do_tipo(material);
    const Preset*     melhor = nullptr;
    int               nota   = -1;
    for (const Preset& f : b->filaments) {
        if (!f.is_visible || !compativel(f))
            continue;
        const auto* tipo = f.config.option<ConfigOptionStrings>("filament_type");
        const bool  mesmo_id = !fid.empty() && f.filament_id == fid;
        if (!mesmo_id && (tipo == nullptr || tipo->values.empty() || !boost::iequals(tipo->values.front(), material)))
            continue;
        const int n = (mesmo_id ? 100 : 0) + (f.is_system ? 20 : 0) + (f.name.find("Basic") != std::string::npos ? 6 : 0) +
                      (boost::istarts_with(f.name, "Generic") ? 5 : 0) + (f.name.find("Support") != std::string::npos ? -30 : 0);
        if (n > nota)
            nota = n, melhor = &f;
    }
    return melhor;
}

// Quantos filamentos o projeto 3MF usa (Metadata/project_settings.config). A Farm imprime a mesa numa cor só: o
// filamento dela vai em todos, senão o fatiador sem tela mistura o da Farm (PETG) com os do projeto (PLA) e recusa
// pela diferença de temperatura (código -62).
int filamentos_do_3mf(const fsf::path& arquivo)
{
    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!open_zip_reader(&zip, utf8_de(arquivo)))
        return 0;
    int        n   = 0;
    const int  idx = mz_zip_reader_locate_file(&zip, "Metadata/project_settings.config", nullptr, 0);
    size_t     tam = 0;
    void*      p   = idx >= 0 ? mz_zip_reader_extract_to_heap(&zip, mz_uint(idx), &tam, 0) : nullptr;
    if (p != nullptr) {
        const json j = json::parse(std::string(static_cast<const char*>(p), tam), nullptr, false);
        for (const char* k : {"filament_settings_id", "filament_type", "filament_colour"})
            if (j.is_object() && j.contains(k) && j[k].is_array())
                n = std::max(n, int(j[k].size()));
        mz_free(p);
    }
    close_zip_reader(&zip);
    return std::min(n, 32);
}

FarmPerfis farm_perfis(const std::string& material, const fsf::path& pasta)
{
    FarmPerfis      r;
    PresetBundle*   b   = wxGetApp().preset_bundle;
    DeviceManager*  dev = wxGetApp().getDeviceManager();
    MachineObject*  o   = dev != nullptr ? dev->get_selected_machine() : nullptr;
    if (b == nullptr || o == nullptr) {
        r.erro = "sem impressora conectada";
        return r;
    }
    r.agente = agente_ativo();
    if (o->printer_agent_id != r.agente) {
        r.erro = "a impressora escolhida na Preparação não é a conectada";
        return r;
    }
    const Preset* fil = filamento_do_material(material, [](const Preset& f) { return f.is_compatible; });
    if (fil == nullptr) {
        r.erro = "não há perfil de " + material + " para esta impressora";
        return r;
    }
    r.maquina   = utf8_de(pasta / "maquina.json");
    r.processo  = utf8_de(pasta / "processo.json");
    r.filamento = utf8_de(pasta / "filamento.json");
    r.altura    = b->prints.get_edited_preset().config.opt_float("layer_height");
    if (!gravar_perfil(b->printers.get_edited_preset().config, "machine", "Filuna Farm maquina", pasta / "maquina.json") ||
        !gravar_perfil(b->prints.get_edited_preset().config, "process", "Filuna Farm processo", pasta / "processo.json") ||
        !gravar_perfil(fil->config, "filament", "Filuna Farm " + material, pasta / "filamento.json")) {
        r.erro = "não gravou os perfis para o fatiamento";
        return r;
    }
    r.base.dev_id          = o->get_dev_id();
    r.base.dev_ip          = o->get_dev_ip();
    r.base.dev_name        = o->get_dev_name();
    r.base.password        = o->get_access_code();
    r.base.connection_type = "lan";
    r.ok                   = true;
    return r;
}

// Perfis para uma impressora ao vivo que não é a da Preparação: o perfil de sistema do modelo dela, com processo e
// filamento compatíveis (sem trocar a impressora escolhida na Preparação). Linha da tela.
FarmPerfis farm_perfis_ao_vivo(const std::string& material, const fsf::path& pasta, const FarmMaquina& m)
{
    FarmPerfis     r;
    PresetBundle*  b   = wxGetApp().preset_bundle;
    DeviceManager* dev = wxGetApp().getDeviceManager();
    MachineObject* o   = dev != nullptr ? dev->get_local_machine(m.dev_id) : nullptr;
    if (b == nullptr || o == nullptr) {
        r.erro = "a impressora não está na lista do programa";
        return r;
    }
    const Preset* pr = get_printer_preset(o);
    if (pr == nullptr)
        for (const Preset& x : b->printers)
            if (x.is_system && x.config.has("printer_model") && x.config.opt_string("printer_model") == o->printer_type &&
                (pr == nullptr || x.name.find("0.4") != std::string::npos))
                pr = &x;
    if (pr == nullptr) {
        r.erro = "não há perfil instalado para o modelo desta impressora (" + o->printer_type + ")";
        return r;
    }
    const PresetWithVendorProfile pv = b->printers.get_preset_with_vendor_profile(*pr);
    const Preset*                 proc = nullptr;
    double                        dif  = 1e9;
    for (const Preset& x : b->prints) {
        if (!x.is_system || !is_compatible_with_printer(b->prints.get_preset_with_vendor_profile(x), pv))
            continue;
        const double d = std::abs(x.config.opt_float("layer_height") - 0.2) + (x.name.find("Standard") != std::string::npos ? 0 : 0.001);
        if (d < dif)
            dif = d, proc = &x;
    }
    const Preset* fil = filamento_do_material(material, [b, &pv](const Preset& f) {
        return f.is_system && is_compatible_with_printer(b->filaments.get_preset_with_vendor_profile(f), pv);
    });
    if (proc == nullptr || fil == nullptr) {
        r.erro = proc == nullptr ? "não há perfil de impressão para " + pr->name : "não há perfil de " + material + " para " + pr->name;
        return r;
    }
    r.agente    = m.agente;
    r.maquina   = utf8_de(pasta / "maquina.json");
    r.processo  = utf8_de(pasta / "processo.json");
    r.filamento = utf8_de(pasta / "filamento.json");
    r.altura    = proc->config.opt_float("layer_height");
    if (!gravar_perfil(pr->config, "machine", "Filuna Farm maquina", pasta / "maquina.json") ||
        !gravar_perfil(proc->config, "process", "Filuna Farm processo", pasta / "processo.json") ||
        !gravar_perfil(fil->config, "filament", "Filuna Farm " + material, pasta / "filamento.json")) {
        r.erro = "não gravou os perfis para o fatiamento";
        return r;
    }
    r.base.dev_id          = o->get_dev_id();
    r.base.dev_ip          = o->get_dev_ip();
    r.base.dev_name        = o->get_dev_name();
    r.base.password        = o->get_access_code();
    r.base.connection_type = "lan";
    r.ok                   = true;
    return r;
}

// Roda um programa e espera (sem janela). O fatiamento sem tela é o próprio Filuna Slicer; a análise do arquivo é o
// Python que vem com ele (python/python.exe), com o mesmo farm_arquivos.py do site.
int rodar_programa(const std::string& exe, const std::vector<std::string>& args, int limite_s)
{
#ifdef _WIN32
    std::wstring linha = L"\"" + boost::nowide::widen(exe) + L"\"";
    for (const std::string& a : args)
        linha += L" \"" + boost::nowide::widen(a) + L"\"";
    STARTUPINFOW        si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    std::vector<wchar_t> buf(linha.begin(), linha.end());
    buf.push_back(0);
    if (!::CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi))
        return -1;
    const DWORD w = ::WaitForSingleObject(pi.hProcess, DWORD(limite_s) * 1000u);
    DWORD       codigo = 1;
    if (w == WAIT_TIMEOUT)
        ::TerminateProcess(pi.hProcess, 2), codigo = 2;
    else
        ::GetExitCodeProcess(pi.hProcess, &codigo);
    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
    return int(codigo);
#else
    std::string linha = "\"" + exe + "\"";
    for (const std::string& a : args)
        linha += " \"" + a + "\"";
    (void) limite_s;
    return std::system(linha.c_str());
#endif
}

std::string exe_do_programa() { return std::string(wxStandardPaths::Get().GetExecutablePath().ToUTF8().data()); }

int rodar_fatiamento(const std::vector<std::string>& args, int limite_s) { return rodar_programa(exe_do_programa(), args, limite_s); }

std::string python_do_programa()
{
    const fsf::path pasta = fsf::path(exe_do_programa()).parent_path();
    boost::system::error_code ec;
    for (const fsf::path& p : {pasta / "python" / "python.exe", pasta / "python" / "bin" / "python3"})
        if (fsf::exists(p, ec))
            return p.string();
    return "python3";
}

fsf::path pasta_farm_py()
{
    // páginas atualizadas pelo servidor (recurso "telas") valem sobre as do instalador, como nas telas Filuna
    const fsf::path base = fsf::path(resources_dir()) / "web" / "filuna" / "farm-py";
    return base;
}

struct FarmAtivo // a mesa da Farm que esta impressora está fazendo (sobrevive a fechar e abrir o programa)
{
    std::string id, dev_id;
    long long   desde{0};
    double      gramas{0};
    int         posicao{1};
    bool        viu_imprimindo{false};
};

std::atomic<bool> g_farm_iniciada{false};
std::atomic<bool> g_farm_agora{false};
std::atomic<bool> g_farm_destravar{false}; // suporte remoto: esquecer as mesas em andamento   // projeto salvo agora: a volta não espera os 20 s
std::mutex        g_farm_indice_trava;

fsf::path farm_indice() { return fsf::path(data_dir()) / "filuna" / "farm" / "locais.json"; }

json farm_indice_ler()
{
    fsf::ifstream i(farm_indice(), std::ios::binary);
    const json    j = json::parse(std::string(std::istreambuf_iterator<char>(i), std::istreambuf_iterator<char>()), nullptr, false);
    return j.is_object() ? j : json::object();
}

std::string farm_arquivo_local(const std::string& tid)
{
    std::lock_guard<std::mutex> g(g_farm_indice_trava);
    const json j = farm_indice_ler();
    return j.contains(tid) && j[tid].is_string() ? j[tid].get<std::string>() : std::string();
}
std::map<std::string, long long> g_farm_recusadas; // mesa -> quando foi recusada (tenta de novo depois de 10 min)
std::string       g_farm_ultimo_aviso;

void farm_avisar(const std::string& texto)
{
    if (texto == g_farm_ultimo_aviso)
        return;
    g_farm_ultimo_aviso = texto;
    BOOST_LOG_TRIVIAL(warning) << "Filuna Farm: " << texto;
    wxGetApp().CallAfter([texto] {
        if (Plater* pl = wxGetApp().plater())
            if (auto* nm = pl->get_notification_manager())
                nm->push_notification("Farm: " + texto);
    });
}

using FarmAtivos = std::map<std::string, FarmAtivo>; // impressora (dev_id) -> mesa que ela está fazendo

json farm_ativo_json(const FarmAtivo& a)
{
    return {{"id", a.id}, {"dev_id", a.dev_id}, {"desde", a.desde}, {"gramas", a.gramas}, {"posicao", a.posicao}, {"viu", a.viu_imprimindo}};
}

FarmAtivo farm_ativo_de(const json& j)
{
    FarmAtivo a;
    if (j.is_object()) {
        a.id = j.value("id", std::string()), a.dev_id = j.value("dev_id", std::string()), a.desde = j.value("desde", 0LL);
        a.gramas = j.value("gramas", 0.0), a.posicao = j.value("posicao", 1), a.viu_imprimindo = j.value("viu", false);
    }
    return a;
}

void farm_gravar_ativos(const FarmAtivos& as)
{
    json j = json::object();
    for (const auto& [dev, a] : as)
        if (!a.id.empty())
            j[dev] = farm_ativo_json(a);
    wxGetApp().CallAfter([t = j.empty() ? std::string() : j.dump()] {
        wxGetApp().app_config->set("filuna_farm_ativos", t);
        wxGetApp().app_config->set("filuna_farm_ativo", ""); // formato antigo (uma impressora só)
    });
}

// Lê as mesas em andamento; aceita o formato antigo, de uma mesa só ("filuna_farm_ativo").
FarmAtivos farm_ler_ativos(const std::string& novo, const std::string& antigo)
{
    FarmAtivos as;
    const json j = json::parse(novo, nullptr, false);
    if (j.is_object())
        for (auto it = j.begin(); it != j.end(); ++it) {
            FarmAtivo a = farm_ativo_de(it.value());
            if (!a.id.empty())
                as[a.dev_id.empty() ? it.key() : a.dev_id] = a;
        }
    const FarmAtivo velho = farm_ativo_de(json::parse(antigo, nullptr, false));
    if (!velho.id.empty() && !velho.dev_id.empty() && !as.count(velho.dev_id))
        as[velho.dev_id] = velho;
    return as;
}

// Mesma regra do site (farm_plano.cor_perto): cores "iguais" a até 60 de distância RGB.
bool cor_perto(std::string a, std::string b)
{
    auto rgb = [](std::string h, int& r, int& g, int& bl) {
        if (!h.empty() && h[0] == '#')
            h = h.substr(1);
        if (h.size() < 6)
            return false;
        try {
            r = std::stoi(h.substr(0, 2), nullptr, 16), g = std::stoi(h.substr(2, 2), nullptr, 16), bl = std::stoi(h.substr(4, 2), nullptr, 16);
        } catch (...) {
            return false;
        }
        return true;
    };
    int r1, g1, b1, r2, g2, b2;
    if (!rgb(a, r1, g1, b1) || !rgb(b, r2, g2, b2))
        return false;
    return std::sqrt(double((r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2))) <= 60.0;
}

// A posição que vem do site conta todas as entradas do sistema de cores, as vazias também (a lista do Dispositivo:
// unidade × 4 + espaço, de 1 em diante). m.cores só tem as ocupadas: procura pelo número global, não pela ordem.
// Antes a A4 de uma impressora com A2 e A3 vazias virava "a mesa pede a entrada 4, e a impressora tem 2".
const json* entrada_da_posicao(const FarmMaquina& m, int posicao)
{
    for (const json& e : m.cores)
        if (e.value("global", -1) == posicao - 1)
            return &e;
    return nullptr;
}

// Conferência antes de imprimir uma mesa que veio do site/app: a impressora conectada tem, na entrada indicada, o
// material e a cor da mesa? Vazio = pode imprimir; senão, o motivo (a mesa não é enviada).
std::string farm_conferir(const json& t, const FarmMaquina& m)
{
    const std::string material = texto_de(t, "material"), cor = texto_de(t, "cor"), cor_nome = texto_de(t, "cor_nome");
    const int         posicao  = std::max(1, inteiro(t, "posicao", 1));
    if (m.cores.empty())
        return {}; // impressora sem sistema de cores: usa o filamento que está nela (o site já conferiu pela telemetria)
    const json* achada = entrada_da_posicao(m, posicao);
    if (achada == nullptr)
        return "a entrada " + std::to_string(posicao) + " está vazia; a mesa pede " + material + " " + cor_nome;
    const json&       e    = *achada;
    const std::string tipo = texto_de(e, "tipo"), tem = texto_de(e, "cor");
    if (tipo.empty())
        return "a entrada " + std::to_string(posicao) + " está vazia; a mesa pede " + material + " " + cor_nome;
    if (!material.empty() && !boost::iequals(tipo, material))
        return "a entrada " + std::to_string(posicao) + " tem " + tipo + " e a mesa pede " + material + " " + cor_nome;
    if (!cor.empty() && !tem.empty() && !cor_perto(cor, tem))
        return "a cor da entrada " + std::to_string(posicao) + " não é " + cor_nome + " (troque a bobina ou registre a troca)";
    return {};
}

// Uma mesa: baixa, fatia, envia. Devolve vazio se deu certo ou o motivo.
std::string farm_imprimir_mesa(const json& t, const std::string& token, const FarmMaquina& m, FarmAtivo& ativo)
{
    const std::string tid = texto_de(t, "id"), material = texto_de(t, "material", "PLA");
    if (tid.empty() || tid.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos)
        return "trabalho com identificador inválido";
    const int posicao = std::max(1, inteiro(t, "posicao", 1)), qtd = std::max(1, std::min(200, inteiro(t, "quantidade", 1)));
    {
        const std::string confere = farm_conferir(t, m);
        if (!confere.empty())
            return "mesa " + std::to_string(inteiro(t, "mesa", 1)) + " de " + texto_de(t, "descricao", "Farm") + " não foi enviada: " + confere;
    }
    const fsf::path pasta = fsf::path(data_dir()) / "filuna" / "farm" / tid;
    boost::system::error_code ec;
    fsf::remove_all(pasta, ec);
    fsf::create_directories(pasta / "saida", ec);
    const std::string arquivo = texto_de(t, "arquivo", "modelo.stl");
    const std::string ext     = boost::iends_with(arquivo, ".3mf") ? ".3mf" : boost::iends_with(arquivo, ".obj") ? ".obj" : ".stl";
    const fsf::path   modelo  = pasta / ("modelo" + ext);
    // projeto feito no Slicer: o modelo já está no computador (não vai ao servidor para voltar)
    bool local = false;
    {
        const std::string guardado = farm_arquivo_local(tid);
        if (!guardado.empty() && fsf::exists(caminho_de(guardado), ec)) {
            fsf::copy_file(caminho_de(guardado), modelo, fsf::copy_options::overwrite_existing, ec);
            local = !ec;
        }
    }
    if (!local) {
        FarmResp d = farm_http("GET", "/trabalhos/" + tid + "/arquivo", json(), token, modelo);
        if (d.status != 200)
            return "não baixou o modelo da mesa (" + d.erro + ")";
    }
    FarmPerfis perf;
    if (!na_janela<FarmPerfis>([material, pasta, m] { return m.ao_vivo ? farm_perfis_ao_vivo(material, pasta, m) : farm_perfis(material, pasta); }, perf) ||
        !perf.ok)
        return perf.erro.empty() ? "não leu os perfis da impressora" : perf.erro;
    const fsf::path saida = pasta / "saida";
    // Marcas que imprimem o filamento N pela entrada N (Anycubic: a impressora segue a ferramenta do G-code e ignora a
    // entrada escolhida): a peça é fatiada como filamento N, o da entrada onde está a cor (entrada global + 1).
    const json* entrada_cor = entrada_da_posicao(m, posicao);
    const int   global_cor  = entrada_cor != nullptr ? entrada_cor->value("global", 0) : 0;
    const int filamento_n = entrada_fixa(m.agente) ? std::max(1, std::min(16, global_cor + 1)) : 1;
    const bool projeto_3mf = ext == ".3mf"; // o fatiador sem tela não clona nem troca o filamento de um projeto 3MF
    if (projeto_3mf && filamento_n > 1)
        return "este projeto 3MF imprime pela Entrada 1 nesta impressora: coloque o " + material + " " + texto_de(t, "cor_nome") +
               " na Entrada 1 ou mande o modelo em STL";
    const int   copias     = projeto_3mf ? std::max(1, filamentos_do_3mf(modelo)) : filamento_n;
    std::string filamentos = perf.filamento;
    for (int k = 1; k < copias; ++k)
        filamentos += ";" + perf.filamento;
    std::vector<std::string> args{"--datadir", utf8_de(pasta / "dados"), "--load-settings", perf.maquina + ";" + perf.processo,
                                  "--load-filaments", filamentos};
    if (!projeto_3mf) {
        args.insert(args.end(), {"--clone-objects", std::to_string(qtd)});
        if (filamento_n > 1)
            args.insert(args.end(), {"--load-filament-ids", std::to_string(filamento_n)});
    }
    args.insert(args.end(), {"--arrange", "1", "--ensure-on-bed", "--allow-newer-file", "--slice", "0", "--export-statistics",
                             "--outputdir", utf8_de(saida), utf8_de(modelo)});
    const int rc = rodar_fatiamento(args, 30 * 60);
    const fsf::path gcode = saida / "plate_1.gcode";
    if (rc != 0 || !fsf::exists(gcode, ec)) {
        std::string motivo = "o fatiamento da mesa falhou (código " + std::to_string(rc) + ")";
        fsf::ifstream rj(saida / "result.json");
        const json    rr = json::parse(std::string(std::istreambuf_iterator<char>(rj), std::istreambuf_iterator<char>()), nullptr, false);
        if (rr.is_object() && !rr.value("error_string", std::string()).empty())
            motivo += ": " + rr.value("error_string", std::string());
        return motivo;
    }
    double tempo = 0, gramas = 0;
    {
        fsf::ifstream sj(saida / "plate_1.statistics.json");
        const json    st = json::parse(std::string(std::istreambuf_iterator<char>(sj), std::istreambuf_iterator<char>()), nullptr, false);
        if (st.is_object() && st.contains("summary")) {
            tempo = st["summary"].value("time", 0.0);
            for (const auto& f : st["summary"].value("filaments", json::array()))
                gramas += f.value("weight", 0.0);
        }
    }
    farm_http("POST", "/trabalhos/" + tid, {{"estado", "fatiado"}, {"tempo_s", int(tempo)}, {"gramas", gramas}}, token);
    // nome que a impressora mostra: pedido + mesa
    std::string nome = texto_de(t, "descricao", "Farm");
    for (char& c : nome)
        if (std::string("\\/:*?\"<>|").find(c) != std::string::npos)
            c = '-';
    nome = nome.substr(0, 60) + " - mesa " + std::to_string(inteiro(t, "mesa", 1)) + ".gcode";
    const fsf::path envio = pasta / nome;
    fsf::copy_file(gcode, envio, fsf::copy_options::overwrite_existing, ec);
    // posição da cor na impressora (1 = primeira entrada informada) -> entrada global do sistema de cores
    // (na Anycubic a cor está no filamento N: as posições antes dele ficam sem entrada)
    json mapa = json::array();
    if (entrada_cor != nullptr) {
        for (int k = 1; k < filamento_n; ++k)
            mapa.push_back(-1);
        mapa.push_back(global_cor);
        // projeto 3MF com vários filamentos, todos trocados pelo da Farm: todos saem da mesma entrada
        for (int k = 1; projeto_3mf && k < copias; ++k)
            mapa.push_back(global_cor);
    }
    PrintParams p                 = perf.base;
    p.filename                    = utf8_de(envio);
    p.project_name                = envio.stem().string();
    p.task_name                   = p.project_name;
    p.plate_index                 = 1;
    p.ams_mapping                 = mapa.empty() ? std::string() : mapa.dump();
    p.task_use_ams                = !mapa.empty();
    p.task_bed_leveling           = wxGetApp().app_config->get("filuna_imprimir_nivelar") != "0";
    p.task_flow_cali = p.task_vibration_cali = p.task_layer_inspect = p.task_record_timelapse = false;
    p.task_timelapse_use_internal = p.task_ext_change_assist = p.try_emmc_print = false;
    p.use_ssl_for_ftp = p.use_ssl_for_mqtt = false;
    std::string msg_envio;
    int         rc_envio = -1;
    if (m.ao_vivo)
        rc_envio = FilunaAoVivo::imprimir(m.dev_id, p, msg_envio);
    else {
        NetworkAgent* net = wxGetApp().getAgent();
        if (net == nullptr)
            return "sem conexão com a impressora";
        try {
            rc_envio = net->start_local_print(p, [&msg_envio](int etapa, int, std::string msg) { if (etapa == PrintingStageERROR) msg_envio = msg; },
                                              [] { return false; });
        } catch (const std::exception& e) {
            msg_envio = e.what();
        }
    }
    if (rc_envio != 0)
        return "a impressora recusou a mesa (" + (msg_envio.empty() ? "código " + std::to_string(rc_envio) : msg_envio) + ")";
    farm_http("POST", "/trabalhos/" + tid, {{"estado", "enviado"}}, token);
    analise_comecar(m.dev_id, utf8_de(envio), texto_de(t, "descricao").empty() ? envio.stem().string() : texto_de(t, "descricao"),
                    t.is_object() && t.contains("impressora") ? texto_de(t["impressora"], "nome") : std::string(), tid);
    ativo.id = tid, ativo.dev_id = m.dev_id, ativo.desde = (long long) std::time(nullptr), ativo.gramas = gramas, ativo.posicao = posicao;
    ativo.viu_imprimindo = false;
    fsf::remove(modelo, ec);
    return {};
}

// Pausar, retomar e cancelar pedidos no site/app: cada comando vai para a impressora dele (a selecionada ou uma ao vivo).
void farm_comandos(const std::string& token, const std::vector<FarmMaquina>& ms)
{
    FarmResp r = farm_http("GET", "/comandos", json(), token);
    if (r.status != 200 || !r.corpo.is_object())
        return;
    for (const auto& c : r.corpo.value("comandos", json::array())) {
        const std::string cid = texto_de(c, "id"), cmd = texto_de(c, "comando");
        const std::string ref = c.is_object() && c.contains("impressora") ? texto_de(c["impressora"], "ref") : std::string();
        const auto        m   = std::find_if(ms.begin(), ms.end(), [&ref](const FarmMaquina& x) { return !ref.empty() && x.ref == ref; });
        if (cid.empty() || m == ms.end())
            continue;
        int rc = -1;
        if (m->ao_vivo)
            rc = FilunaAoVivo::comando(m->dev_id, cmd);
        else
            na_janela<int>([cmd, dev_id = m->dev_id]() -> int {
                DeviceManager* dev = wxGetApp().getDeviceManager();
                MachineObject* o   = dev != nullptr ? dev->get_selected_machine() : nullptr;
                if (o == nullptr || o->get_dev_id() != dev_id)
                    return -1;
                return cmd == "pausar" ? o->command_task_pause() : cmd == "retomar" ? o->command_task_resume() : cmd == "cancelar" ? o->command_task_abort() : -1;
            }, rc);
        farm_http("POST", "/comandos/" + cid, {{"estado", rc == 0 ? "feito" : "falhou"}}, token);
    }
}

// Acompanha a mesa que uma impressora está fazendo. true = a mesa saiu desta impressora (concluída ou falhou).
bool farm_acompanhar(FarmAtivo& ativo, const FarmMaquina& m, const std::string& token)
{
    const long long passou = (long long) std::time(nullptr) - ativo.desde;
    if (m.imprimindo || m.status == "RUNNING" || m.status == "PREPARE" || m.status == "PAUSE") {
        ativo.viu_imprimindo = true;
        farm_http("POST", "/trabalhos/" + ativo.id,
                  {{"estado", "imprimindo"}, {"progresso", m.progresso}, {"resta_s", m.resta_s}, {"pausada", m.pausada}}, token);
        return false;
    }
    if (ativo.viu_imprimindo && m.status == "FINISH") {
        farm_http("POST", "/trabalhos/" + ativo.id,
                  {{"estado", "concluido"}, {"tempo_s", passou}, {"filamentos", json::array({{{"posicao", ativo.posicao}, {"gramas", ativo.gramas}}})}}, token);
        return true;
    }
    if ((ativo.viu_imprimindo && (m.status == "FAILED" || m.status == "IDLE")) || (!ativo.viu_imprimindo && passou > 15 * 60)) {
        farm_http("POST", "/trabalhos/" + ativo.id, {{"estado", "falhou"}}, token);
        farm_avisar("a mesa não terminou na impressora (" + (m.status.empty() ? std::string("sem resposta") : m.status) + "); veja no painel da Farm");
        return true;
    }
    return false;
}

// Uma volta da Farm: acompanha a mesa de cada impressora ao vivo e manda a próxima mesa da fila para uma impressora
// livre (uma mesa nova por volta, para não fatiar várias ao mesmo tempo). Mesa de impressora que saiu do ar espera ela
// voltar.
void farm_volta(FarmAtivos& ativos)
{
    if (g_farm_destravar.exchange(false)) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna Farm: " << ativos.size() << " mesa(s) em andamento esquecidas pelo suporte";
        ativos.clear();
        farm_gravar_ativos(ativos);
    }
    const std::string token = wxGetApp().app_config->get("filuna_token");
    if (token.empty() || wxGetApp().app_config->get("filuna_farm_auto") == "0")
        return;
    std::vector<FarmMaquina> ms;
    if (!na_janela<std::vector<FarmMaquina>>([] { return farm_maquinas(); }, ms))
        return;
    ms.erase(std::remove_if(ms.begin(), ms.end(), [](const FarmMaquina& m) { return !m.ok || m.ref.empty(); }), ms.end());
    if (ms.empty())
        return;
    farm_comandos(token, ms);
    bool mudou = false;
    for (const FarmMaquina& m : ms) {
        auto it = ativos.find(m.dev_id);
        if (it == ativos.end())
            continue;
        const bool viu = it->second.viu_imprimindo;
        if (farm_acompanhar(it->second, m, token)) {
            ativos.erase(it);
            mudou = true;
        } else if (it->second.viu_imprimindo != viu)
            mudou = true;
    }
    if (mudou)
        farm_gravar_ativos(ativos);
    std::vector<const FarmMaquina*> livres;
    for (const FarmMaquina& m : ms)
        if (m.online && !m.imprimindo && !ativos.count(m.dev_id))
            livres.push_back(&m);
    if (livres.empty())
        return;
    FarmResp f = farm_http("GET", "/fila", json(), token);
    if (f.status != 200 || !f.corpo.is_object())
        return;
    for (const FarmMaquina* m : livres)
        for (const auto& t : f.corpo.value("trabalhos", json::array())) {
            const std::string ref = t.is_object() && t.contains("impressora") ? texto_de(t["impressora"], "ref") : std::string();
            if (ref != m->ref || !(t.contains("pode_iniciar") && t["pode_iniciar"].is_boolean() && t["pode_iniciar"].get<bool>()) ||
                texto_de(t, "estado") != "aguardando")
                continue;
            const auto rec = g_farm_recusadas.find(texto_de(t, "id"));
            if (rec != g_farm_recusadas.end() && (long long) std::time(nullptr) - rec->second < 10 * 60)
                continue; // recusada há pouco: não fatia de novo a cada volta
            FarmAtivo         novo;
            const std::string motivo = farm_imprimir_mesa(t, token, *m, novo);
            if (!motivo.empty()) {
                g_farm_recusadas[texto_de(t, "id")] = (long long) std::time(nullptr);
                farm_avisar(motivo);
                FilunaManutencao::diagnostico("farm", motivo, json{{"trabalho", texto_de(t, "id")}, {"agente", m->agente}});
            } else {
                g_farm_ultimo_aviso.clear();
                ativos[m->dev_id] = novo;
                farm_gravar_ativos(ativos);
            }
            return; // uma mesa por volta
        }
}
} // namespace

// ---------------- Farm local: ações da tela (abrir, analisar, medir, salvar) ----------------
namespace {
bool id_seguro(const std::string& s)
{
    return !s.empty() && s.size() <= 64 && s.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-_") == std::string::npos;
}
bool nome_seguro(const std::string& s)
{
    return !s.empty() && s.size() <= 200 && s.find("..") == std::string::npos && s.find(':') == std::string::npos && s[0] != '/' && s[0] != '\\';
}
fsf::path pasta_projeto(const std::string& id) { return fsf::path(data_dir()) / "filuna" / "farm" / "projetos" / id; }

json ler_json(const fsf::path& p)
{
    fsf::ifstream i(p, std::ios::binary);
    return json::parse(std::string(std::istreambuf_iterator<char>(i), std::istreambuf_iterator<char>()), nullptr, false);
}
} // namespace

json farm_acao(const std::string& acao, const json& d, std::function<void(const std::string&, const json&)> evento)
{
    if (acao == "farm.escolher") {
        wxFileDialog dlg(wxGetApp().mainframe, wxString::FromUTF8("Escolha o projeto para a Farm"), wxEmptyString, wxEmptyString,
                         wxString::FromUTF8("Modelos 3D (*.stl;*.3mf;*.zip)|*.stl;*.3mf;*.zip;*.STL;*.3MF;*.ZIP"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK)
            return json::object();
        const std::string c = dlg.GetPath().ToUTF8().data();
        boost::system::error_code ec;
        return {{"caminho", c}, {"nome", utf8_de(caminho_de(c).filename())}, {"tamanho", (long long) fsf::file_size(caminho_de(c), ec)}};
    }
    if (acao == "farm.projeto_atual") {   // "Enviar à Farm": o projeto aberto na Preparação vira o arquivo do projeto
        Plater* plater = wxGetApp().plater();
        if (plater == nullptr || plater->model().objects.empty())
            throw std::runtime_error("Não há nada na Preparação. Abra ou monte o projeto lá e volte aqui.");
        if (modelo_protegido())
            throw std::runtime_error("Modelos do catálogo Filuna são impressos pela Preparação e não vão para a Farm.");
        boost::system::error_code ec;
        const fsf::path pasta = fsf::path(data_dir()) / "filuna" / "farm";
        fsf::create_directories(pasta, ec);
        const fsf::path arq = pasta / ("preparacao-" + std::to_string((long long) std::time(nullptr)) + ".3mf");
        if (plater->export_3mf(arq, SaveStrategy::Silence) < 0 || !fsf::exists(arq, ec))
            throw std::runtime_error("Não foi possível preparar o projeto da Preparação.");
        std::string nome = std::string(plater->get_project_name().ToUTF8().data());
        if (nome.empty())
            nome = "Projeto da Preparação";
        return {{"caminho", utf8_de(arq)}, {"nome", nome + ".3mf"}, {"tamanho", (long long) fsf::file_size(arq, ec)}};
    }
    if (acao == "farm.analisar") {
        const std::string caminho = texto_de(d, "caminho");
        boost::system::error_code ec;
        if (caminho.empty() || !fsf::exists(caminho_de(caminho), ec))
            throw std::runtime_error("Arquivo não encontrado.");
        const std::string id    = std::to_string((long long) std::time(nullptr)) + "-" + std::to_string(std::rand() % 100000);
        const fsf::path   pasta = pasta_projeto(id);
        fsf::create_directories(pasta, ec);
        std::thread([id, pasta, caminho, evento] {
            const int rc = rodar_programa(python_do_programa(), {"-I", "-B", utf8_de(pasta_farm_py() / "analisar.py"), caminho, utf8_de(pasta)}, 300);
            json      r  = ler_json(pasta / "analise.json");
            if (!r.is_object())
                r = {{"ok", false}, {"erro", "Não foi possível ler esse arquivo (análise " + std::to_string(rc) + ")."}};
            r["projeto"] = id;
            evento("farm.analise", r);
        }).detach();
        return {{"projeto", id}};
    }
    if (acao == "farm.medir") {
        const std::string id = texto_de(d, "projeto"), material = texto_de(d, "material", "PLA");
        if (!id_seguro(id) || material.empty() || material.size() > 12 ||
            material.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-") != std::string::npos)
            throw std::runtime_error("Projeto inválido.");
        std::vector<std::string> arquivos;
        for (const auto& a : d.value("arquivos", json::array()))
            if (a.is_string() && nome_seguro(a.get<std::string>()))
                arquivos.push_back(a.get<std::string>());
        std::thread([id, material, arquivos, evento] {
            const fsf::path pasta = pasta_projeto(id), base = pasta / ("medida-" + material);
            boost::system::error_code ec;
            fsf::create_directories(base, ec);
            FarmPerfis perf;
            json       saida = {{"projeto", id}, {"material", material}, {"arquivos", json::object()}, {"erros", json::object()}};
            if (!na_janela<FarmPerfis>([material, base] { return farm_perfis(material, base); }, perf) || !perf.ok) {
                saida["erro"] = perf.erro.empty() ? std::string("não leu os perfis da impressora") : perf.erro;
                evento("farm.medida", saida);
                return;
            }
            int n = 0;
            for (const std::string& a : arquivos) {
                const fsf::path out = base / ("saida-" + std::to_string(++n));
                fsf::remove_all(out, ec);
                fsf::create_directories(out, ec);
                const int rc = rodar_fatiamento({"--datadir", utf8_de(pasta / "dados"), "--load-settings", perf.maquina + ";" + perf.processo,
                                                 "--load-filaments", perf.filamento, "--arrange", "1", "--ensure-on-bed", "--allow-newer-file",
                                                 "--slice", "0", "--export-statistics", "--outputdir", utf8_de(out),
                                                 utf8_de(pasta / "modelos" / caminho_de(a))},
                                                30 * 60);
                double tempo = 0, gramas = 0;
                for (fsf::directory_iterator it(out, ec), fim; !ec && it != fim; it.increment(ec)) {
                    const std::string nome = it->path().filename().string();
                    if (!boost::ends_with(nome, ".statistics.json"))
                        continue;
                    const json st = ler_json(it->path());
                    if (st.is_object() && st.contains("summary")) {
                        tempo += st["summary"].value("time", 0.0);
                        for (const auto& f : st["summary"].value("filaments", json::array()))
                            gramas += f.value("weight", 0.0);
                    }
                }
                if (rc != 0 || tempo <= 0) {
                    const json rr = ler_json(out / "result.json");
                    saida["erros"][a] = rr.is_object() && !texto_de(rr, "error_string").empty() ? texto_de(rr, "error_string")
                                                                                             : "não foi possível fatiar (" + std::to_string(rc) + ")";
                } else
                    saida["arquivos"][a] = {{"tempo_s", tempo}, {"gramas", gramas}, {"altura_camada", perf.altura}};
            }
            evento("farm.medida", saida);
        }).detach();
        return json::object();
    }
    if (acao == "farm.salvar") {
        const std::string id = texto_de(d, "projeto");
        if (!id_seguro(id) || !d.contains("dados") || !d["dados"].is_object())
            throw std::runtime_error("Projeto inválido.");
        const json        dados = d["dados"];
        const std::string token = wxGetApp().app_config->get("filuna_token");
        std::thread([id, dados, token, evento] {
            FarmResp r = farm_http("POST", "/farm-projetos", dados, token);
            if (r.status != 200 || !r.corpo.is_object()) {
                evento("farm.salvo", {{"erro", r.erro.empty() ? std::string("o servidor não guardou o projeto") : r.erro}});
                return;
            }
            // as mesas deste projeto imprimem com o arquivo que já está no computador
            const json ts = r.corpo.value("trabalhos", json::array()), ms = dados.value("mesas", json::array());
            {
                std::lock_guard<std::mutex> g(g_farm_indice_trava);
                json ind = farm_indice_ler();
                for (std::size_t i = 0; i < ts.size() && i < ms.size(); ++i) {
                    const std::string tid = texto_de(ts[i], "id"), arq = texto_de(ms[i], "arquivo");
                    if (!tid.empty() && nome_seguro(arq))
                        ind[tid] = utf8_de(pasta_projeto(id) / "modelos" / caminho_de(arq));
                }
                boost::system::error_code ec;
                fsf::create_directories(farm_indice().parent_path(), ec);
                fsf::ofstream o(farm_indice(), std::ios::binary | std::ios::trunc);
                o << ind.dump();
            }
            g_farm_agora = true;
            evento("farm.salvo", {{"pid", texto_de(r.corpo, "pid")}, {"numero", inteiro(r.corpo, "numero", 0)}});
        }).detach();
        return json::object();
    }
    if (acao == "modelo3d.imprimir" || acao == "modelo3d.abrir") {
        // Modelo do catálogo Filuna: o 3MF original vem só por rota autenticada, abre num projeto novo e fica travado
        // para salvar/exportar (só imprimir). "imprimir" pede o envio ao site; "abrir" abre um envio feito no site/app.
        const std::string slug = texto_de(d, "slug"), envio = texto_de(d, "id"), titulo = texto_de(d, "titulo", "Modelo Filuna");
        if ((acao == "modelo3d.imprimir" && !id_seguro(slug)) || (acao == "modelo3d.abrir" && !id_seguro(envio)))
            throw std::runtime_error("Modelo inválido.");
        const std::string token = wxGetApp().app_config->get("filuna_token");
        std::thread([acao, slug, envio, titulo, token, evento] {
            std::string eid = envio;
            if (acao == "modelo3d.imprimir") {
                FarmResp r = farm_http("POST", "/modelos3d/" + slug + "/imprimir", json::object(), token);
                if (r.status != 200 || !r.corpo.is_object() || texto_de(r.corpo, "id").empty()) {
                    evento("modelo3d.erro", {{"erro", r.erro.empty() ? std::string("o site não liberou este modelo") : r.erro}});
                    return;
                }
                eid = texto_de(r.corpo, "id");
            }
            boost::system::error_code ec;
            const fsf::path pasta = fsf::path(data_dir()) / "filuna" / "catalogo";
            fsf::create_directories(pasta, ec);
            const fsf::path arq = pasta / (eid + ".3mf");
            FarmResp d = farm_http("GET", "/modelos3d/envios/" + eid + "/arquivo", json(), token, arq);
            if (d.status != 200) {
                fsf::remove(arq, ec);
                evento("modelo3d.erro", {{"erro", d.erro.empty() ? std::string("o modelo não foi baixado") : d.erro}});
                return;
            }
            wxGetApp().CallAfter([arq, eid, titulo, token, evento] {
                boost::system::error_code ec2;
                Plater* plater = wxGetApp().plater();
                if (plater == nullptr || plater->new_project() == wxID_CANCEL) {
                    fsf::remove(arq, ec2);
                    evento("modelo3d.erro", {{"erro", "abertura cancelada"}});
                    return;
                }
                if (MainFrame* frame = wxGetApp().mainframe)
                    frame->select_filuna_tab(TAB_ID_PREPARE);
                plater->load_files(std::vector<fsf::path>{arq}, LoadStrategy::LoadModel | LoadStrategy::LoadConfig);
                proteger_modelo(true);
                fsf::remove(arq, ec2); // o arquivo original não fica no computador
                if (auto* nm = plater->get_notification_manager())
                    nm->push_notification(titulo + ": modelo do catálogo Filuna aberto para imprimir (não pode ser salvo nem exportado).");
                std::thread([eid, token] { farm_http("POST", "/modelos3d/envios/" + eid, {{"estado", "aberto"}}, token); }).detach();
                evento("modelo3d.aberto", {{"id", eid}});
            });
        }).detach();
        return json::object();
    }
    throw std::runtime_error("Ação da Farm desconhecida.");
}

// ---------------- Modelos 3D do catálogo: só imprimir ----------------
// A trava vale para os OBJETOS do catálogo, não para o programa: cada objeto aberto do catálogo leva a marca em
// input_file. Modelos do próprio cliente (outros arquivos, outros projetos) salvam e exportam normalmente; se ele
// apagar o modelo do catálogo da mesa, a trava sai junto.
namespace { const std::string MARCA_CATALOGO = "filuna-catalogo:"; }
bool modelo_protegido()
{
    Plater* plater = wxGetApp().plater();
    if (plater == nullptr)
        return false;
    for (const ModelObject* o : plater->model().objects)
        if (o != nullptr && o->input_file.rfind(MARCA_CATALOGO, 0) == 0)
            return true;
    return false;
}
void proteger_modelo(bool ligar)
{
    // ligar: marca os objetos que estão na mesa (o modelo do catálogo acabou de abrir num projeto novo).
    // desligar: nada a fazer — projeto novo ou outro projeto não têm objetos marcados.
    if (!ligar)
        return;
    if (Plater* plater = wxGetApp().plater())
        for (ModelObject* o : plater->model().objects)
            if (o != nullptr)
                o->input_file = MARCA_CATALOGO + o->input_file;
}
bool saida_barrada()
{
    if (!modelo_protegido())
        return false;
    MessageDialog dlg(wxGetApp().mainframe,
                      wxString::FromUTF8("Este projeto tem um modelo 3D do catálogo Filuna. Ele pode ser impresso nas suas impressoras, "
                                         "mas não pode ser salvo nem exportado (projeto, 3MF, STL, G-code ou arquivo fatiado).\n\n"
                                         "Para salvar ou exportar os seus modelos, tire o modelo do catálogo da mesa ou comece um projeto novo."),
                      wxString::FromUTF8("Modelo do catálogo Filuna"), wxOK | wxICON_INFORMATION);
    dlg.ShowModal();
    return true;
}

void farm_destravar()
{
    g_farm_destravar = true;
    g_farm_agora     = true;
}

// ---- Assistência remota: impressão de teste ----
namespace {
std::mutex g_teste_trava;
json       g_teste = {{"estado", "nenhum"}};

void teste_estado(json j)
{
    j["hora"] = (long long) std::time(nullptr);
    std::lock_guard<std::mutex> l(g_teste_trava);
    g_teste = std::move(j);
}

// Placa de 40 × 40 × 0,6 mm em STL binário: testa a primeira camada e o envio gastando quase nada.
bool gravar_placa_teste(const fsf::path& arq)
{
    const float    a = 40.f, h = 0.6f;
    const float    v[8][3] = {{0, 0, 0}, {a, 0, 0}, {a, a, 0}, {0, a, 0}, {0, 0, h}, {a, 0, h}, {a, a, h}, {0, a, h}};
    const int      f[12][3] = {{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7}, {0, 1, 5}, {0, 5, 4},
                               {1, 2, 6}, {1, 6, 5}, {2, 3, 7}, {2, 7, 6}, {3, 0, 4}, {3, 4, 7}};
    fsf::ofstream  o(arq, std::ios::binary | std::ios::trunc);
    char           cabecalho[80] = "Filuna - placa de teste";
    const uint32_t n             = 12;
    const float    normal[3]     = {0, 0, 0};
    const uint16_t atributo      = 0;
    o.write(cabecalho, sizeof(cabecalho));
    o.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (const auto& t : f) {
        o.write(reinterpret_cast<const char*>(normal), sizeof(normal));
        for (int k = 0; k < 3; ++k)
            o.write(reinterpret_cast<const char*>(v[t[k]]), sizeof(v[0]));
        o.write(reinterpret_cast<const char*>(&atributo), sizeof(atributo));
    }
    return o.good();
}
} // namespace

json impressao_teste(const std::string& dev_id, const std::string& material, int posicao)
{
    {
        std::lock_guard<std::mutex> l(g_teste_trava);
        if (g_teste.value("estado", std::string()) == "andamento")
            return {{"erro", "Já há uma impressão de teste sendo preparada."}};
    }
    // a impressora pedida (ou a aberta no programa), entre as conectadas: a mesma lista que a Farm usa
    FarmMaquina maq;
    for (const FarmMaquina& m : farm_maquinas())
        if (dev_id.empty() ? !m.ao_vivo : m.dev_id == dev_id) {
            maq = m;
            break;
        }
    if (!maq.ok)
        return {{"erro", "A impressora não está conectada no programa: conecte antes de mandar o teste."}};
    if (maq.imprimindo)
        return {{"erro", "A impressora está imprimindo agora."}};
    const std::string tid   = "teste-" + std::to_string((long long) std::time(nullptr));
    const fsf::path   pasta = fsf::path(data_dir()) / "filuna" / "teste";
    boost::system::error_code ec;
    fsf::create_directories(pasta, ec);
    const fsf::path stl = pasta / (tid + ".stl");
    if (!gravar_placa_teste(stl))
        return {{"erro", "Não consegui gravar a peça de teste."}};
    {
        // o caminho da Farm pega do índice local o modelo de um projeto feito aqui (não baixa do servidor)
        std::lock_guard<std::mutex> g(g_farm_indice_trava);
        json j = farm_indice_ler();
        j[tid] = utf8_de(stl);
        fsf::create_directories(farm_indice().parent_path(), ec);
        fsf::ofstream o(farm_indice(), std::ios::binary | std::ios::trunc);
        o << j.dump();
    }
    const std::string token = wxGetApp().app_config->get("filuna_token");
    const json        t     = {{"id", tid},           {"material", material.empty() ? std::string("PLA") : material},
                               {"posicao", std::max(1, posicao)}, {"quantidade", 1}, {"arquivo", "teste.stl"}, {"descricao", "Teste Filuna"}};
    teste_estado({{"estado", "andamento"}, {"dev_id", maq.dev_id}, {"etapa", "fatiando e enviando"}});
    std::thread([t, token, maq, tid, stl] {
        std::string erro;
        try {
            FarmAtivo ativo;
            erro = farm_imprimir_mesa(t, token, maq, ativo);
        } catch (const std::exception& e) {
            erro = e.what();
        }
        {
            std::lock_guard<std::mutex> g(g_farm_indice_trava);
            json j = farm_indice_ler();
            j.erase(tid);
            fsf::ofstream o(farm_indice(), std::ios::binary | std::ios::trunc);
            o << j.dump();
        }
        boost::system::error_code e2;
        fsf::remove(stl, e2);
        BOOST_LOG_TRIVIAL(info) << "Filuna: impressão de teste do suporte em " << maq.dev_id << ": " << (erro.empty() ? "enviada" : erro);
        teste_estado(erro.empty() ? json{{"estado", "enviado"}, {"dev_id", maq.dev_id}}
                                  : json{{"estado", "falhou"}, {"dev_id", maq.dev_id}, {"erro", erro}});
    }).detach();
    return {{"ok", true}, {"teste", tid}, {"impressora", maq.dev_id}};
}

json impressao_teste_estado()
{
    std::lock_guard<std::mutex> l(g_teste_trava);
    return g_teste;
}

void farm_iniciar()
{
    if (g_farm_iniciada.exchange(true))
        return;
    analise_monitor();
    const std::string salvos = wxGetApp().app_config->get("filuna_farm_ativos"), salvo = wxGetApp().app_config->get("filuna_farm_ativo");
    std::thread([salvos, salvo] {
        FarmAtivos ativos = farm_ler_ativos(salvos, salvo);
        std::this_thread::sleep_for(std::chrono::seconds(40));
        for (;;) {
            try {
                farm_volta(ativos);
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(warning) << "Filuna Farm: " << e.what();
            } catch (...) {
            }
            for (int k = 0; k < 20 && !g_farm_agora.exchange(false); ++k)
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();
}

// ============================================================================================================
// Análise da impressão: amostras de todas as impressoras a cada 5 s para o Filuna::Monitor; o relatório final vai
// para a conta do cliente no site (POST /analises) e fica uma cópia na pasta de dados (filuna/analise/relatorios).
// ============================================================================================================
namespace {

std::atomic<bool> g_analise_iniciada{false};

bool analise_ligada() { return wxGetApp().app_config != nullptr && wxGetApp().app_config->get("filuna_analise") != "0"; }

struct AmostraDe
{
    std::string     dev_id, nome;
    Filuna::Amostra amostra;
};

// Linha da tela: a impressora selecionada (pelo MachineObject) e as acompanhadas em segundo plano (relatório ao vivo).
std::vector<AmostraDe> analise_amostras()
{
    std::vector<AmostraDe> v;
    const double   agora = double(std::time(nullptr));
    DeviceManager* dev   = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return v;
    std::map<std::string, MachineObject*> todas = dev->get_my_machine_list();
    for (auto& [id, o] : dev->get_local_machinelist())
        todas.emplace(id, o);
    MachineObject* sel = dev->get_selected_machine();
    if (sel != nullptr) {
        json rel = {{"gcode_state", sel->print_status}, {"layer_num", sel->curr_layer}, {"total_layer_num", sel->total_layers},
                    {"mc_percent", sel->mc_print_percent}, {"stg_cur", sel->stage_curr}, {"print_error", sel->print_error},
                    {"mc_remaining_time", sel->mc_left_time > 0 ? sel->mc_left_time / 60 : -1}};
        if (DevExtderSystem* ext = sel->GetExtderSystem()) {
            rel["nozzle_temper"]        = ext->GetNozzleTempCurrent(0);
            rel["nozzle_target_temper"] = ext->GetNozzleTempTarget(0);
        }
        if (DevBed* bed = sel->GetBed()) {
            rel["bed_temper"]        = bed->GetBedTemp();
            rel["bed_target_temper"] = bed->GetBedTempTarget();
        }
        if (DevFan* fan = sel->GetFan())
            rel["cooling_fan_speed"] = std::to_string(fan->GetCoolingFanSpeed());
        if (DevPrintOptions* op = sel->GetPrintOptions())
            rel["spd_lvl"] = int(op->GetPrintingSpeedLevel());
        AmostraDe a{sel->get_dev_id(), sel->get_dev_name(), Filuna::amostra_do_relatorio_bambu(rel, agora)};
        if (!sel->is_online())
            a.amostra.estado = "desligada";
        v.push_back(std::move(a));
    }
    for (const FilunaAoVivo::Ligada& l : FilunaAoVivo::ligadas()) {
        if (sel != nullptr && l.dev_id == sel->get_dev_id())
            continue;
        auto        it   = todas.find(l.dev_id);
        std::string nome = it != todas.end() && it->second != nullptr ? it->second->get_dev_name() : l.dev_id;
        v.push_back({l.dev_id, nome, Filuna::amostra_do_relatorio_bambu(l.relatorio, agora)});
    }
    return v;
}

// Resposta da janela ao site, fora da linha da tela; `depois` roda na linha da tela com o corpo da resposta.
void analise_responder(const std::string& rota, const json& corpo, std::function<void(const json&)> depois)
{
    const std::string token = wxGetApp().app_config->get("filuna_token");
    std::thread([rota, corpo, token, depois] {
        FarmResp r = farm_http("POST", rota, corpo, token);
        const json j = r.status == 200 && r.corpo.is_object() ? r.corpo : json{{"ok", false}};
        wxGetApp().CallAfter([j, depois] { depois(j); });
    }).detach();
}

// Janela do alerta: a mensagem, a foto da mesa (quando a câmera mandou uma recente) e "Como está a impressão?".
// Não trava o programa.
void analise_janela(const std::string& titulo, const std::string& mensagem, const std::string& jpeg, const std::string& link,
                    const std::string& aid)
{
    wxDialog* d = new wxDialog(wxGetApp().mainframe, wxID_ANY, wxString::FromUTF8(titulo), wxDefaultPosition, wxDefaultSize,
                               wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP);
    auto* col = new wxBoxSizer(wxVERTICAL);
    if (!jpeg.empty()) {
        wxMemoryInputStream in(jpeg.data(), jpeg.size());
        wxImage             img;
        if (img.LoadFile(in, wxBITMAP_TYPE_JPEG) && img.IsOk()) {
            const int larg = std::min(560, img.GetWidth());
            img.Rescale(larg, std::max(1, img.GetHeight() * larg / std::max(1, img.GetWidth())), wxIMAGE_QUALITY_HIGH);
            col->Add(new wxStaticBitmap(d, wxID_ANY, wxBitmap(img)), 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 12);
        }
    }
    auto* txt = new wxStaticText(d, wxID_ANY, wxString::FromUTF8(mensagem));
    txt->Wrap(540);
    col->Add(txt, 0, wxLEFT | wxRIGHT | wxBOTTOM, 14);
    if (!aid.empty()) {
        // Como está a impressão? Boa / Ruim. Com Ruim (e a opção ligada na conta) vem a oferta do ajuste de 3 min.
        auto* pergunta = new wxStaticText(d, wxID_ANY, wxString::FromUTF8("Como está a impressão?"));
        pergunta->SetFont(pergunta->GetFont().Bold());
        auto* linha = new wxBoxSizer(wxHORIZONTAL);
        auto* boa   = new wxButton(d, wxID_ANY, wxString::FromUTF8("Boa"));
        auto* ruim  = new wxButton(d, wxID_ANY, wxString::FromUTF8("Ruim"));
        linha->Add(boa, 0, wxRIGHT, 8);
        linha->Add(ruim);
        col->Add(pergunta, 0, wxLEFT | wxRIGHT, 14);
        col->Add(linha, 0, wxALL, 10);
        wxWeakRef<wxDialog> ref(d);
        auto responder = [ref, pergunta, boa, ruim, linha, aid](const std::string& resposta) {
            boa->Disable();
            ruim->Disable();
            analise_responder("/analises/alertas/" + aid + "/resposta", json{{"resposta", resposta}}, [ref, pergunta, boa, ruim, linha, aid, resposta](const json& r) {
                if (!ref)
                    return;
                wxDialog* dd = ref.get();
                boa->Hide();
                ruim->Hide();
                if (!r.value("ok", false)) {
                    pergunta->SetLabel(wxString::FromUTF8("Não foi possível mandar a resposta. Responda pelo site."));
                } else if (r.value("oferecer_ajuste", false)) {
                    pergunta->SetLabel(wxString::FromUTF8("Quer que a Filuna ajuste e acompanhe a impressão por 3 minutos? Ela pode mudar a "
                                                          "temperatura do bico e da mesa, a velocidade e a ventoinha, dentro da faixa "
                                                          "segura do material, e no fim manda o resumo com a opção de desfazer. Só aceite "
                                                          "se você estiver junto da impressora, verificando a impressão."));
                    pergunta->Wrap(540);
                    auto* sim = new wxButton(dd, wxID_ANY, wxString::FromUTF8("Sim, estou junto da impressora"));
                    auto* nao = new wxButton(dd, wxID_ANY, wxString::FromUTF8("Não"));
                    linha->Add(sim, 0, wxRIGHT, 8);
                    linha->Add(nao);
                    auto decidir = [ref, pergunta, sim, nao, aid](const std::string& acao) {
                        sim->Disable();
                        nao->Disable();
                        // "Sim, estou junto da impressora": o clique é a confirmação de presença que o site exige
                        analise_responder("/analises/alertas/" + aid + "/ajuste", json{{"acao", acao}, {"presente", acao == "sim"}},
                                          [ref, pergunta, sim, nao, acao](const json& r2) {
                            if (!ref)
                                return;
                            sim->Hide();
                            nao->Hide();
                            pergunta->SetLabel(wxString::FromUTF8(!r2.value("ok", false) ? "Não foi possível mandar. Responda pelo site."
                                                                  : acao == "sim" ? "A Filuna vai ajustar e acompanhar por 3 minutos. O resumo chega no fim."
                                                                                  : "Tudo bem: a Filuna não vai mexer na impressora."));
                            pergunta->Wrap(540);
                            ref->Layout();
                            ref->Fit();
                        });
                    };
                    sim->Bind(wxEVT_BUTTON, [decidir](wxCommandEvent&) { decidir("sim"); });
                    nao->Bind(wxEVT_BUTTON, [decidir](wxCommandEvent&) { decidir("nao"); });
                } else if (resposta == "boa") {
                    pergunta->SetLabel(wxString::FromUTF8("Obrigado! A Filuna segue acompanhando."));
                } else {
                    pergunta->SetLabel(wxString::FromUTF8(r.value("pode_ligar", false)
                                                              ? "Obrigado. Para a Filuna poder ajustar a impressora numa próxima vez, ligue "
                                                                "\"Deixar a Filuna ajustar minha impressora\" em filuna3d.com › Relatórios."
                                                              : "Obrigado. Confira a peça e decida se segue a impressão."));
                    pergunta->Wrap(540);
                }
                dd->Layout();
                dd->Fit();
            });
        };
        boa->Bind(wxEVT_BUTTON, [responder](wxCommandEvent&) { responder("boa"); });
        ruim->Bind(wxEVT_BUTTON, [responder](wxCommandEvent&) { responder("ruim"); });
    }
    auto* botoes = new wxBoxSizer(wxHORIZONTAL);
    if (!link.empty()) {
        auto* ver = new wxButton(d, wxID_ANY, wxString::FromUTF8("Ver no site"));
        ver->Bind(wxEVT_BUTTON, [link](wxCommandEvent&) { wxLaunchDefaultBrowser(wxString::FromUTF8(link)); });
        botoes->Add(ver, 0, wxRIGHT, 8);
    }
    auto* fechar = new wxButton(d, wxID_OK, wxString::FromUTF8("Fechar"));
    fechar->Bind(wxEVT_BUTTON, [d](wxCommandEvent&) { d->Destroy(); });
    botoes->Add(fechar);
    col->Add(botoes, 0, wxALL | wxALIGN_RIGHT, 12);
    d->Bind(wxEVT_CLOSE_WINDOW, [d](wxCloseEvent&) { d->Destroy(); });
    d->SetSizerAndFit(col);
    d->CentreOnParent();
    d->Show();
    if (wxGetApp().mainframe != nullptr)
        wxGetApp().mainframe->RequestUserAttention();
}

// Alerta da análise: o site junta a foto da mesa (a última da câmera desta impressora), avisa no app, no site e por
// e-mail; aqui a janela mostra o mesmo, com a foto.
void analise_alertar(json alerta, const std::string& ref)
{
    BOOST_LOG_TRIVIAL(warning) << "Filuna análise: alerta (" << alerta.value("tipo", std::string()) << ") " << alerta.value("mensagem", std::string());
    alerta["cadastro_id"] = ref;
    const std::string token = wxGetApp().app_config->get("filuna_token");
    std::string       jpeg, link, aid;
    if (!token.empty()) {
        FarmResp resp = farm_http("POST", "/analises/alertas", alerta, token);
        if (resp.status == 200 && resp.corpo.is_object()) {
            link = resp.corpo.value("link", std::string());
            aid  = resp.corpo.value("id", std::string());
            const std::string foto = resp.corpo.value("foto_url", std::string());
            if (!foto.empty())
                Http::get(foto).timeout_connect(10).timeout_max(30).on_complete([&jpeg](std::string b, unsigned st) {
                    if (st == 200 && b.size() > 3 && (unsigned char) b[0] == 0xFF && (unsigned char) b[1] == 0xD8)
                        jpeg = std::move(b);
                }).perform_sync();
        } else
            BOOST_LOG_TRIVIAL(warning) << "Filuna análise: o site não recebeu o alerta (" << resp.erro << ")";
    }
    const std::string titulo = alerta.value("titulo", std::string("Impressão fora do previsto")), msg = alerta.value("mensagem", std::string());
    wxGetApp().CallAfter([titulo, msg, jpeg, link, aid] { analise_janela(titulo, msg, jpeg, link, aid); });
}

struct AjusteAtivo
{
    std::string                              dev_id;
    std::unique_ptr<Filuna::AjusteAssistido> ajuste;
    bool                                     terminado{false};
    long long                                fim{0};
};
std::map<std::string, AjusteAtivo> g_ajustes; // por alerta; só a linha do monitor mexe

// Manda os comandos do ajuste para a impressora: a selecionada pelo programa, as outras pelo acompanhamento ao vivo.
bool analise_executar(const std::string& dev_id, const std::vector<Filuna::ComandoImpressora>& cmds)
{
    if (cmds.empty())
        return true;
    int sel = -2;
    na_janela<int>([dev_id, cmds]() -> int {
        DeviceManager* dev = wxGetApp().getDeviceManager();
        MachineObject* o   = dev != nullptr ? dev->get_selected_machine() : nullptr;
        if (o == nullptr || o->get_dev_id() != dev_id)
            return -2; // não é a selecionada
        int rc = 0;
        for (const Filuna::ComandoImpressora& c : cmds) {
            const std::string t = c.texto();
            if (t.rfind("gcode:", 0) == 0)
                rc |= o->publish_gcode(t.substr(6) + "\n");
            else
                rc |= o->command_set_printing_speed(DevPrintingSpeedLevel(c.valor));
        }
        return rc;
    }, sel);
    if (sel != -2)
        return sel == 0;
    bool ok = true;
    for (const Filuna::ComandoImpressora& c : cmds)
        ok = FilunaAoVivo::comando(dev_id, c.texto()) == 0 && ok;
    return ok;
}

// Pedidos do site: começar, parar ou desfazer o ajuste de um alerta (só os das impressoras deste computador).
void analise_pedidos(const std::vector<AmostraDe>& aqui, const std::string& token)
{
    FarmResp r = farm_http("GET", "/analises/ajustes", json(), token);
    if (r.status != 200 || !r.corpo.is_object())
        return;
    for (const auto& p : r.corpo.value("ajustes", json::array())) {
        const std::string aid = texto_de(p, "id"), dev = texto_de(p, "dev_id"), acao = texto_de(p, "acao");
        const bool        meu = std::any_of(aqui.begin(), aqui.end(), [&dev](const AmostraDe& a) { return a.dev_id == dev; });
        if (aid.empty() || !meu)
            continue;
        auto it = g_ajustes.find(aid);
        if (acao == "iniciar" && it == g_ajustes.end()) {
            std::optional<Filuna::PlanoImpressao> plano = Filuna::Monitor::get().plano(dev);
            AjusteAtivo a;
            a.dev_id = dev;
            a.ajuste = std::make_unique<Filuna::AjusteAssistido>(plano ? *plano : Filuna::PlanoImpressao(), texto_de(p, "tipo"),
                                                                 p.value("conferencia", json::object()));
            g_ajustes.emplace(aid, std::move(a));
            farm_http("POST", "/analises/ajustes/" + aid, {{"estado", "em_andamento"}}, token);
            BOOST_LOG_TRIVIAL(warning) << "Filuna ajuste: começou em " << dev << " (alerta " << aid << ")";
        } else if (acao == "parar" && it != g_ajustes.end() && !it->second.terminado) {
            it->second.ajuste->parar(double(std::time(nullptr)));
        } else if (acao == "desfazer" && it != g_ajustes.end() && it->second.terminado) {
            const bool ok = analise_executar(dev, it->second.ajuste->desfazer());
            farm_http("POST", "/analises/ajustes/" + aid, {{"estado", ok ? "desfeito" : "falhou"}}, token);
            g_ajustes.erase(it);
        } else if (acao == "desfazer" && it == g_ajustes.end()) {
            farm_http("POST", "/analises/ajustes/" + aid, {{"estado", "falhou"}}, token); // o programa foi fechado: não sabe o que desfazer
        }
    }
}

// Uma volta dos ajustes em andamento: comandos da rodada e, no fim dos 3 min, o resumo para o site.
void analise_ajustes(const std::vector<AmostraDe>& aqui, const std::string& token)
{
    const long long agora = (long long) std::time(nullptr);
    for (auto it = g_ajustes.begin(); it != g_ajustes.end();) {
        AjusteAtivo& a = it->second;
        if (a.terminado) { // guarda 24 h para o desfazer
            it = agora - a.fim > 86400 ? g_ajustes.erase(it) : std::next(it);
            continue;
        }
        auto am = std::find_if(aqui.begin(), aqui.end(), [&a](const AmostraDe& x) { return x.dev_id == a.dev_id; });
        if (am != aqui.end()) {
            const std::vector<Filuna::ComandoImpressora> cmds = a.ajuste->passo(am->amostra);
            if (!analise_executar(a.dev_id, cmds)) {
                farm_http("POST", "/analises/ajustes/" + it->first, {{"estado", "falhou"}}, token);
                it = g_ajustes.erase(it);
                continue;
            }
            for (const Filuna::ComandoImpressora& c : cmds)
                BOOST_LOG_TRIVIAL(warning) << "Filuna ajuste: " << a.dev_id << " " << c.texto();
        }
        if (a.ajuste->terminou()) {
            const json r = a.ajuste->resumo();
            farm_http("POST", "/analises/ajustes/" + it->first, {{"estado", r.value("estado", std::string("feito"))}, {"resumo", r}}, token);
            a.terminado = true;
            a.fim       = agora;
        }
        ++it;
    }
}

void analise_enviar(const json& r)
{
    const std::string token = wxGetApp().app_config->get("filuna_token");
    BOOST_LOG_TRIVIAL(info) << "Filuna análise: relatório de " << r.value("nome", std::string()) << " nota " << r.value("nota", 0) << " ("
                            << r["ocorrencias"].size() << " ocorrências)";
    if (token.empty())
        return; // sem conta ligada: fica só a cópia no computador
    FarmResp resp = farm_http("POST", "/analises", r, token);
    if (resp.status != 200)
        BOOST_LOG_TRIVIAL(warning) << "Filuna análise: o site não recebeu o relatório (" << resp.erro << ")";
}

} // namespace

void analise_monitor()
{
    if (g_analise_iniciada.exchange(true))
        return;
    Filuna::Monitor::get().pasta(utf8_de(fsf::path(data_dir()) / "filuna" / "analise"));
    std::thread([] {
        std::this_thread::sleep_for(std::chrono::seconds(15));
        unsigned volta = 0;
        for (;;) {
            try {
                std::vector<AmostraDe> v;
                if (analise_ligada() && na_janela<std::vector<AmostraDe>>([] { return analise_amostras(); }, v, 10)) {
                    const std::string token = wxGetApp().app_config->get("filuna_token");
                    if (!token.empty()) {
                        if (++volta % 2 == 0) // pedidos do site a cada 10 s
                            analise_pedidos(v, token);
                        analise_ajustes(v, token);
                    }
                    for (const AmostraDe& a : v) {
                        Filuna::Monitor::Resultado r = Filuna::Monitor::get().amostra(a.dev_id, a.nome, a.amostra);
                        for (json& alerta : r.alertas)
                            analise_alertar(std::move(alerta), wxGetApp().app_config->get("filuna_dispositivo_" + a.dev_id));
                        if (r.relatorio)
                            analise_enviar(*r.relatorio);
                    }
                }
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(warning) << "Filuna análise: " << e.what();
            } catch (...) {
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }).detach();
}

void analise_comecar(const std::string& dev_id, const std::string& gcode_utf8, const std::string& nome, const std::string& impressora,
                     const std::string& trabalho_id)
{
    if (!analise_ligada() || dev_id.empty())
        return;
    analise_monitor();
    std::thread([=] { // G-code grande leva alguns segundos para ler: fora da linha da tela
        try {
            Filuna::PlanoImpressao plano = Filuna::plano_do_gcode(gcode_utf8);
            BOOST_LOG_TRIVIAL(info) << "Filuna análise: plano de " << nome << " com " << plano.camadas.size() << " camadas";
            Filuna::Monitor::get().comecar(dev_id, plano, nome, impressora, trabalho_id);
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "Filuna análise: plano não lido: " << e.what();
        }
    }).detach();
}


}}} // namespace Slic3r::GUI::FilunaImpressao
