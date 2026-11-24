#include "FilunaCurva.hpp"
#include "FilunaManutencao.hpp"
#include "GUI_App.hpp"
#include "NotificationManager.hpp"
#include "PartPlate.hpp"
#include "Plater.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/Process.hpp"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/nowide/convert.hpp>
#include <nlohmann/json.hpp>
#include <wx/stdpaths.h>
#include <atomic>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif

namespace Slic3r { namespace GUI { namespace FilunaCurva {

namespace {
namespace fsf = boost::filesystem;
using nlohmann::json;

// Limites de segurança da Luna (mais exigentes que os do motor).
constexpr double FOLGA_MINIMA_MM = 0.5;   // menor distância permitida entre a cabeça e a peça
constexpr double AREA_MINIMA     = 0.10;  // fração do topo que precisa ficar curva para valer a pena
constexpr double GANHO_MINIMO    = 0.25;  // o erro do topo precisa cair pelo menos 25%
constexpr int    LIMITE_S        = 900;

std::atomic<bool> g_ocupado{false};

struct Medida
{
    bool        ok{false};
    std::string erro;
    double      rms{0};
    double      cobertura{0};
    double      tempo_s{0};
    bool        verificou{false};
    long long   batidas{-1};
    double      folga_mm{-1};
    std::string cabeca;
};

fsf::path pasta_motor() { return fsf::path(boost::nowide::narrow(wxStandardPaths::Get().GetExecutablePath().ToStdWstring())).parent_path() / "curva"; }

int rodar(const fsf::path& exe, const std::vector<std::string>& args)
{
#ifdef _WIN32
    std::wstring linha = L"\"" + exe.wstring() + L"\"";
    for (const std::string& a : args)
        linha += L" \"" + boost::nowide::widen(a) + L"\"";
    STARTUPINFOW        si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    std::vector<wchar_t> buf(linha.begin(), linha.end());
    buf.push_back(0);
    if (!::CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS, nullptr,
                          exe.parent_path().wstring().c_str(), &si, &pi))
        return -1;
    const DWORD w      = ::WaitForSingleObject(pi.hProcess, DWORD(LIMITE_S) * 1000u);
    DWORD       codigo = 1;
    if (w == WAIT_TIMEOUT)
        ::TerminateProcess(pi.hProcess, 2), codigo = 2;
    else
        ::GetExitCodeProcess(pi.hProcess, &codigo);
    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
    return int(codigo);
#else
    (void) exe;
    (void) args;
    return -1;
#endif
}

Medida ler(const fsf::path& arq)
{
    Medida m;
    boost::system::error_code ec;
    if (!fsf::exists(arq, ec)) {
        m.erro = "o motor não gerou o resultado";
        return m;
    }
    json d;
    try {
        fsf::ifstream f(arq);
        d = json::parse(f);
    } catch (const std::exception& e) {
        m.erro = std::string("resultado ilegível: ") + e.what();
        return m;
    }
    const json t = d.value("totals", json::object());
    if (d.value("status", "") != "sliced") {
        const json ab = t.value("aborts", json::array());
        m.erro        = ab.is_array() && !ab.empty() && ab[0].is_string() ? ab[0].get<std::string>() : "o motor não fatiou";
        return m;
    }
    auto num = [](const json& o, const char* k, double padrao) { return o.contains(k) && o[k].is_number() ? o[k].get<double>() : padrao; };
    const json v = t.value("verifier", json::object());
    m.ok         = true;
    m.rms        = num(t, "whole_top_rms_mm", 0);
    m.cobertura  = num(t, "coverage", 0);
    m.tempo_s    = num(t, "estimated_time_s", 0);
    m.verificou  = v.value("ran", false);
    m.batidas    = (long long) (num(v, "hit_moves", -1) + std::max(0.0, num(v, "machine_hit_moves", 0)));
    m.folga_mm   = num(v, "min_clearance_mm", -1);
    const json in = d.value("inputs", json::object());
    if (in.contains("head") && in["head"].is_object())
        m.cabeca = in["head"].value("id", "");
    return m;
}

// "0.20mm Standard @BBL P2S" -> "0.20mm Standard": o catálogo do motor guarda o processo e o filamento sem a impressora.
std::string sem_impressora(const std::string& nome)
{
    const auto p = nome.find(" @");
    return p == std::string::npos ? nome : nome.substr(0, p);
}

// Cabeça da impressora no catálogo do motor (variante primeiro, depois a base do modelo). Vazio = sem modelo de
// cabeça: a curva não é oferecida. O projeto do Filuna não traz esse campo, então ele vai explícito.
std::string cabeca_do_catalogo(const fsf::path& catalogo, const std::string& impressora)
{
    fsf::ifstream f(catalogo / "printers.jsonl");
    std::string   linha;
    while (std::getline(f, linha)) {
        if (linha.find(impressora) == std::string::npos)
            continue;
        const json d = json::parse(linha, nullptr, false);
        if (!d.is_object() || !d.contains("variants") || !d["variants"].is_array())
            continue;
        for (const json& v : d["variants"]) {
            if (v.value("id", "") != impressora)
                continue;
            const std::string propria = v.contains("values") && v["values"].is_object() ? v["values"].value("nonplanar_head", "") : "";
            if (!propria.empty())
                return propria;
            return d.contains("base") && d["base"].is_object() ? d["base"].value("nonplanar_head", "") : "";
        }
    }
    return {};
}

std::string nome_base(const PresetCollection& col, const Preset& p)
{
    const Preset* base = col.get_preset_base(p);
    return base ? base->name : p.name;
}

void avisar(const std::string& texto, const std::string& link, const fsf::path& gcode)
{
    NotificationManager* nm = wxGetApp().plater() ? wxGetApp().plater()->get_notification_manager() : nullptr;
    if (!nm)
        return;
    if (link.empty()) {
        nm->push_notification(NotificationType::CustomNotification, NotificationManager::NotificationLevel::RegularNotificationLevel, texto);
        return;
    }
    const wxString caminho = wxString::FromUTF8(gcode.string());
    nm->push_notification(NotificationType::CustomNotification, NotificationManager::NotificationLevel::HintNotificationLevel, texto, link,
                          [caminho](wxEvtHandler*) {
                              // o Filuna não traz o visualizador separado da Bambu: outra janela do próprio
                              // Filuna Slicer abre o G-code na Pré-visualização, sem mexer no projeto aberto
                              start_new_slicer(&caminho, false);
                              return false;
                          });
}

} // namespace

void depois_de_fatiar(bool ok)
{
    if (!ok || g_ocupado.load())
        return;
    const fsf::path motor = pasta_motor() / "nonplanar_slice_tool.exe";
    boost::system::error_code ec;
    if (!fsf::exists(motor, ec))
        return;   // só a versão portátil de teste traz o motor
    Plater* plater = wxGetApp().plater();
    PresetBundle* pb = wxGetApp().preset_bundle;
    if (!plater || !pb || plater->model().objects.empty())
        return;

    const std::string impressora = nome_base(pb->printers, pb->printers.get_selected_preset());
    const std::string processo   = sem_impressora(nome_base(pb->prints, pb->prints.get_selected_preset()));
    std::string       filamento  = "Bambu PLA Basic";
    if (!pb->filament_presets.empty())
        if (const Preset* f = pb->filaments.find_preset(pb->filament_presets.front(), false))
            filamento = sem_impressora(nome_base(pb->filaments, *f));
    const int mesa = plater->get_partplate_list().get_curr_plate_index() + 1;

    const fsf::path pasta = fsf::path(data_dir()) / "filuna" / "curva";
    fsf::create_directories(pasta, ec);
    const fsf::path projeto = pasta / "mesa.3mf";
    if (plater->export_3mf(projeto, SaveStrategy::Silence) < 0 || !fsf::exists(projeto, ec))
        return;

    g_ocupado = true;
    plater->get_notification_manager()->push_notification(NotificationType::CustomNotification,
        NotificationManager::NotificationLevel::RegularNotificationLevel, "Luna: analisando se o topo desta mesa melhora com a impressão em curva...");

    std::thread([motor, pasta, projeto, impressora, processo, filamento, mesa] {
        const fsf::path   dados  = motor.parent_path() / "resources";
        const std::string cabeca = cabeca_do_catalogo(dados / "catalog", impressora);
        if (cabeca.empty()) {
            const std::string texto = "Luna (teste): impressão em curva não indicada - não há o modelo da cabeça da " + impressora +
                                      ", então não dá para garantir que o bico não bate.";
            wxGetApp().CallAfter([texto] {
                avisar(texto, "", {});
                g_ocupado = false;
            });
            return;
        }
        auto fatiar = [&](const std::string& nome, const std::string& modo) {
            std::vector<std::string> a = {"--catalog", (dados / "catalog").string(), "--profiles", dados.string(), "--datadir", dados.string(),
                                          "--printer", impressora, "--process", processo, "--filament", filamento,
                                          "--set", "nonplanar_head=" + cabeca, "--allow-mixed-temperatures", "--verify", "--surface-grid", "--plate-index", std::to_string(mesa)};
            if (modo.empty())
                a.push_back("--force-planar");
            else
                a.insert(a.end(), {"--set", "nonplanar_mode=" + modo});
            a.insert(a.end(), {"--json", (pasta / (nome + ".json")).string(), "-o", (pasta / (nome + ".gcode")).string(), projeto.string()});
            boost::system::error_code e;
            fsf::remove(pasta / (nome + ".json"), e);
            const int rc = rodar(motor, a);
            Medida m = ler(pasta / (nome + ".json"));
            if (!m.ok && m.erro.empty())
                m.erro = "código " + std::to_string(rc);
            return m;
        };
        const Medida plano = fatiar("plano", "");
        const Medida curvo = plano.ok ? fatiar("curvo", "nonplanar_top") : Medida{};

        std::string motivo;
        if (!plano.ok)
            motivo = plano.erro;
        else if (!curvo.ok)
            motivo = curvo.erro;
        else if (curvo.cabeca.empty() || curvo.cabeca == "generic-conservative")
            motivo = "não há o modelo da cabeça desta impressora, então não dá para garantir que o bico não bate";
        else if (!curvo.verificou || curvo.batidas != 0 || curvo.folga_mm < FOLGA_MINIMA_MM)
            motivo = curvo.batidas != 0
                         ? (boost::format("o bico bateria na peça em %1% movimentos") % curvo.batidas).str()
                         : (boost::format("a cabeça passaria a só %1$.2f mm da peça; o mínimo seguro é %2$.1f mm. "
                                          "Deixe uma peça só ou afaste as peças") % curvo.folga_mm % FOLGA_MINIMA_MM).str();
        else if (curvo.cobertura < AREA_MINIMA)
            motivo = (boost::format("o topo curvo cobre só %1$.0f%% da peça") % (curvo.cobertura * 100)).str();

        const double ganho = plano.ok && curvo.ok && plano.rms > 0 ? (plano.rms - curvo.rms) / plano.rms : 0;
        if (motivo.empty() && ganho < GANHO_MINIMO)
            motivo = (boost::format("o topo melhora só %1$.0f%%") % (ganho * 100)).str();

        FilunaManutencao::diagnostico("curva", motivo.empty() ? "indicada" : "não indicada: " + motivo,
                                      json{{"impressora", impressora}, {"processo", processo}, {"filamento", filamento}, {"mesa", mesa},
                                           {"erro_plano_mm", plano.rms}, {"erro_curvo_mm", curvo.rms}, {"cobertura", curvo.cobertura},
                                           {"choques", curvo.batidas}, {"folga_mm", curvo.folga_mm}, {"cabeca", curvo.cabeca},
                                           {"tempo_plano_s", plano.tempo_s}, {"tempo_curvo_s", curvo.tempo_s}});

        std::string texto, link;
        if (motivo.empty()) {
            texto = (boost::format("Luna: o topo desta peça pode ficar perfeito usando nosso sistema de impressão em curva. "
                                   "Topo %1$.0f%% mais liso, sem risco de o bico bater (folga mínima %2$.1f mm, %3% min a mais).")
                     % (ganho * 100) % curvo.folga_mm % int((curvo.tempo_s - plano.tempo_s) / 60)).str();
            link = "Ver a demonstração";
        } else {
            texto = "Luna (teste): impressão em curva não indicada para esta mesa - " + motivo + ".";
        }
        const fsf::path gcode = pasta / "curvo.gcode";
        wxGetApp().CallAfter([texto, link, gcode] {
            avisar(texto, link, gcode);
            g_ocupado = false;
        });
    }).detach();
}

}}} // namespace Slic3r::GUI::FilunaCurva
