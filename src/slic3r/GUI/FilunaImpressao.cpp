#include "FilunaImpressao.hpp"

#include "FilunaManutencao.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "MsgDialog.hpp"
#include "PartPlate.hpp"
#include "Plater.hpp"
#include "DeviceManager.hpp"
#include "DeviceCore/DevManager.h"
#include "DeviceCore/DevFilaSystem.h"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "slic3r/Utils/NetworkAgent.hpp"
#include "slic3r/Utils/bambu_networking.hpp"

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
           const std::vector<Entrada>& es)
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
                const int k   = melhor_entrada(f, es);
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
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr || bundle->use_bbl_network())
        return false;
    return boost::starts_with(agente_ativo(), "filuna_");
}

std::string filamento_id_do_tipo(const std::string& tipo)
{
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr || tipo.empty())
        return {};
    const Preset&     impressora = bundle->printers.get_edited_preset();
    const std::string marca      = impressora.vendor != nullptr ? impressora.vendor->id : std::string();
    std::string       da_marca, generico, qualquer;
    for (const Preset& f : bundle->filaments) {
        if (!f.is_compatible || f.filament_id.empty() || bundle->filaments.get_preset_base(f) != &f)
            continue;
        if (!boost::iequals(f.config.opt_string("filament_type", 0u), tipo))
            continue;
        if (da_marca.empty() && !marca.empty() && boost::istarts_with(f.name, marca))
            da_marca = f.filament_id;
        if (generico.empty() && boost::istarts_with(f.name, "Generic"))
            generico = f.filament_id;
        if (qualquer.empty())
            qualquer = f.filament_id;
    }
    return !da_marca.empty() ? da_marca : !generico.empty() ? generico : qualquer;
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

    Janela    janela(wxGetApp().mainframe, o->get_dev_name(), nome, fila, es);
    const int resposta = janela.ShowModal();
    if (resposta != wxID_OK && resposta != wxID_APPLY)
        return;
    const Escolha esc      = janela.escolha();
    const bool    so_envio = resposta == wxID_APPLY;
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

    {
        wxProgressDialog barra(wxString::FromUTF8("Imprimir em " + o->get_dev_name()), wxString::FromUTF8("Preparando o envio…"), 100,
                               wxGetApp().mainframe, wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_SMOOTH);
        while (!and_->fim) {
            wxMilliSleep(80);
            int etapa, pct;
            {
                std::lock_guard<std::mutex> l(and_->m);
                etapa = and_->etapa, pct = and_->pct;
            }
            wxString texto = etapa == PrintingStageUpload ? wxString::FromUTF8("Enviando o arquivo… ") + wxString::Format("%d%%", pct) :
                             etapa == PrintingStageSending ? wxString::FromUTF8("Pedindo o início da impressão…") :
                                                             wxString::FromUTF8("Preparando o envio…");
            if (!barra.Update(std::max(0, std::min(pct, 99)), texto))
                and_->cancelar = true;
        }
    }
    tarefa.join();
    fs::remove(envio, ec);

    int         rc;
    std::string msg;
    {
        std::lock_guard<std::mutex> l(and_->m);
        rc = and_->rc, msg = and_->msg;
    }
    if (rc == BAMBU_NETWORK_SUCCESS) {
        BOOST_LOG_TRIVIAL(info) << "FilunaImpressao: " << (so_envio ? "arquivo enviado" : "impressão pedida") << " agente=" << agente
                                << " arquivo=" << nome << " mapa=" << p.ams_mapping;
        avisar(wxString::FromUTF8(so_envio ? "Arquivo enviado para " + o->get_dev_name() + "." :
                                             "Enviado. A impressão começa em " + o->get_dev_name() + ".\nAcompanhe na aba Dispositivo."));
        return;
    }
    if (and_->cancelar) {
        avisar(wxString::FromUTF8("Envio cancelado."));
        return;
    }
    FilunaManutencao::diagnostico("impressao", msg.empty() ? "falha ao enviar/imprimir (código " + std::to_string(rc) + ")" : msg,
                                  json{{"agente", agente}, {"codigo", rc}, {"so_envio", so_envio}, {"arquivo", nome},
                                       {"mapa", p.ams_mapping}, {"ip", p.dev_ip}});
    avisar(wxString::FromUTF8("Não foi possível " + std::string(so_envio ? "enviar o arquivo" : "iniciar a impressão") + ".\n\n" +
                              (msg.empty() ? "Código " + std::to_string(rc) : msg) + "\n\nO erro foi enviado ao suporte da Filuna."),
           wxOK | wxICON_ERROR);
}

}}} // namespace Slic3r::GUI::FilunaImpressao
