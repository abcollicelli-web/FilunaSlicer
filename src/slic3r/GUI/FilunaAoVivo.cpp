#include "FilunaAoVivo.hpp"

#include "DeviceManager.hpp"
#include "GUI_App.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/IPrinterAgent.hpp"
#include "slic3r/Utils/NetworkAgentFactory.hpp"
#include "slic3r/Utils/bambu_networking.hpp"

#include <boost/log/trivial.hpp>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <functional>

namespace Slic3r { namespace GUI { namespace FilunaAoVivo {

using json  = nlohmann::json;
using Clock = std::chrono::steady_clock;

namespace {

const char* const SECAO = "filuna_ao_vivo";

struct Acesso
{
    std::string ip, agente, codigo;
};

struct Monitorada
{
    Acesso                         acesso;
    std::shared_ptr<IPrinterAgent> agente;
    json                           relatorio  = json::object(); // "print" acumulado (os agentes mandam partes)
    bool                           conectada  = false;
    bool                           conectando = false;
    unsigned                       geracao    = 0; // retornos de uma conexão antiga são ignorados
    Clock::time_point              ultima_msg{}, proxima_tentativa{}, ultimo_pushall{}, inicio_conexao{};
};

std::mutex& mtx()
{
    static std::mutex m;
    return m;
}
std::map<std::string, Monitorada>& mapa()
{
    static std::map<std::string, Monitorada> m;
    return m;
}
std::string& ativa_id()
{
    static std::string s;
    return s;
}

bool agente_moonraker(const std::string& ag) { return ag == "snapmaker" || ag == "qidi" || ag == "crealityprint" || ag == "moonraker"; }
bool acompanhavel(const std::string& ag) { return ag.rfind("filuna_", 0) == 0 || agente_moonraker(ag); }

void mesclar(json& alvo, const json& novo)
{
    for (auto it = novo.begin(); it != novo.end(); ++it) {
        if (it.value().is_object() && alvo.contains(it.key()) && alvo[it.key()].is_object())
            mesclar(alvo[it.key()], it.value());
        else
            alvo[it.key()] = it.value();
    }
}

double num(const json& p, const char* k)
{
    if (!p.contains(k))
        return 0.0;
    const json& v = p[k];
    if (v.is_number())
        return v.get<double>();
    if (v.is_string()) {
        try {
            return std::stod(v.get<std::string>());
        } catch (const std::exception&) {
            return 0.0;
        }
    }
    return 0.0;
}

// Desliga o agente numa linha separada (o desligamento de alguns agentes espera linhas internas terminarem).
// Chamar com mtx() travado.
void desligar(Monitorada& m)
{
    if (m.agente) {
        std::shared_ptr<IPrinterAgent> ag = std::move(m.agente);
        std::thread([ag] { ag->disconnect_printer(); }).detach();
    }
    m.conectada  = false;
    m.conectando = false;
    ++m.geracao;
}

void ao_receber(const std::string& dev, unsigned geracao, const std::string& msg)
{
    const json j = json::parse(msg, nullptr, false);
    if (!j.is_object() || !j.contains("print") || !j["print"].is_object())
        return;
    std::lock_guard<std::mutex> l(mtx());
    auto                        it = mapa().find(dev);
    if (it == mapa().end() || it->second.geracao != geracao)
        return;
    mesclar(it->second.relatorio, j["print"]);
    it->second.ultima_msg = Clock::now();
    it->second.conectada  = true;
}

void ao_conectar(const std::string& dev, unsigned geracao, int status, const std::string& motivo)
{
    std::lock_guard<std::mutex> l(mtx());
    auto                        it = mapa().find(dev);
    if (it == mapa().end() || it->second.geracao != geracao)
        return;
    Monitorada& m = it->second;
    if (status == ConnectStatusOk) {
        m.conectada  = true;
        m.conectando = false;
        m.ultima_msg = Clock::now();
        return;
    }
    BOOST_LOG_TRIVIAL(info) << "Filuna ao vivo: " << dev << " sem conexão (" << status << "): " << motivo;
    desligar(m);
    m.proxima_tentativa = Clock::now() + std::chrono::seconds(60);
}

void ligar(const std::string& dev, const Acesso& a, unsigned geracao)
{
    std::shared_ptr<IPrinterAgent> ag = NetworkAgentFactory::create_new_printer_agent(a.agente, data_dir() + "/log");
    if (!ag) {
        std::lock_guard<std::mutex> l(mtx());
        auto                        it = mapa().find(dev);
        if (it != mapa().end() && it->second.geracao == geracao) {
            it->second.conectando        = false;
            it->second.proxima_tentativa = Clock::now() + std::chrono::minutes(10);
        }
        return;
    }
    // os retornos rodam na própria linha do agente: só mexem no mapa daqui (travado), nada da tela
    ag->set_queue_on_main_fn([](std::function<void()> f) {
        if (f)
            f();
    });
    ag->set_on_ssdp_msg_fn([](std::string) {});
    ag->set_on_printer_connected_fn([](std::string) {});
    ag->set_on_subscribe_failure_fn([](std::string) {});
    ag->set_on_user_message_fn([](std::string, std::string) {});
    ag->set_server_callback([](std::string, int) {});
    auto receber = [dev, geracao](std::string, std::string msg) { ao_receber(dev, geracao, msg); };
    ag->set_on_message_fn(receber);
    ag->set_on_local_message_fn(receber);
    ag->set_on_local_connect_fn([dev, geracao](int status, std::string, std::string motivo) { ao_conectar(dev, geracao, status, motivo); });
    {
        std::lock_guard<std::mutex> l(mtx());
        auto                        it = mapa().find(dev);
        if (it == mapa().end() || it->second.geracao != geracao)
            return; // desligada enquanto criava (ficou ativa no programa)
        it->second.agente = ag;
    }
    PrinterConnectionParams p;
    p.dev_id = dev;
    p.host   = a.ip;
    if (const auto dp = a.ip.find(':'); dp != std::string::npos) {
        p.host = a.ip.substr(0, dp);
        p.port = a.ip.substr(dp + 1);
    }
    // Moonraker usa a senha como chave de API: o código neutro "lan" do Filuna não pode ir
    p.password = agente_moonraker(a.agente) ? std::string() : a.codigo;
    int r      = -1;
    try {
        r = ag->connect_printer(p);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna ao vivo: conectar " << dev << ": " << e.what();
    }
    std::lock_guard<std::mutex> l(mtx());
    auto                        it = mapa().find(dev);
    if (it == mapa().end() || it->second.geracao != geracao)
        return;
    it->second.conectando = false;
    if (r != 0 && !it->second.conectada) {
        desligar(it->second);
        it->second.proxima_tentativa = Clock::now() + std::chrono::seconds(60);
    }
}

void gerente()
{
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::vector<std::tuple<std::string, Acesso, unsigned>> conectar;
        std::vector<std::pair<std::shared_ptr<IPrinterAgent>, std::string>> pedir_tudo;
        {
            std::lock_guard<std::mutex> l(mtx());
            const auto                  agora = Clock::now();
            for (auto& [dev, m] : mapa()) {
                if (dev == ativa_id()) {
                    if (m.agente || m.conectando)
                        desligar(m);
                    continue;
                }
                // agente que não conectou em 2 min e não avisou (o Moonraker não avisa falha): desiste e tenta depois
                if (m.agente && !m.conectada && !m.conectando && agora - m.inicio_conexao > std::chrono::minutes(2)) {
                    desligar(m);
                    m.proxima_tentativa = agora + std::chrono::seconds(60);
                }
                // conectada mas muda há 5 min: refaz a conexão
                if (m.agente && m.conectada && agora - m.ultima_msg > std::chrono::minutes(5)) {
                    desligar(m);
                    m.proxima_tentativa = agora;
                }
                // Bambu só manda o que mudou: um relatório completo a cada 2 min mantém o estado certo
                if (m.agente && m.conectada && m.acesso.agente == "filuna_bambu_lan" && agora - m.ultimo_pushall > std::chrono::minutes(2)) {
                    m.ultimo_pushall = agora;
                    pedir_tudo.emplace_back(m.agente, dev);
                }
                if (!m.agente && !m.conectando && agora >= m.proxima_tentativa) {
                    m.conectando     = true;
                    m.inicio_conexao = agora;
                    ++m.geracao;
                    conectar.emplace_back(dev, m.acesso, m.geracao);
                }
            }
        }
        for (auto& [ag, dev] : pedir_tudo)
            ag->send_message_to_printer(dev, R"({"pushing":{"sequence_id":"0","command":"pushall","version":1,"push_target":1}})", 0, 0);
        for (auto& [dev, a, g] : conectar) {
            if (agente_moonraker(a.agente)) {
                // o agente Moonraker lê o perfil do programa ao conectar: começa na linha da tela (a conexão dele já é
                // assíncrona por dentro)
                const std::string d = dev;
                const Acesso      ac = a;
                const unsigned    gg = g;
                wxGetApp().CallAfter([d, ac, gg] { ligar(d, ac, gg); });
            } else {
                const std::string d = dev;
                const Acesso      ac = a;
                const unsigned    gg = g;
                std::thread([d, ac, gg] { ligar(d, ac, gg); }).detach();
            }
        }
    }
}

} // namespace

void iniciar()
{
    static std::atomic<bool> iniciado{false};
    if (iniciado.exchange(true))
        return;
    // get_section() do AppConfig não confere se a seção existe (find()->second): sem impressora guardada ainda, a
    // seção não existe e ler dela fechava o programa na abertura (79 e 80)
    if (AppConfig* cfg = wxGetApp().app_config; cfg != nullptr && cfg->has_section(SECAO)) {
        std::lock_guard<std::mutex> l(mtx());
        for (const auto& [dev, txt] : cfg->get_section(SECAO)) {
            const json j = json::parse(txt, nullptr, false);
            if (!j.is_object() || dev.empty())
                continue;
            Monitorada& m = mapa()[dev];
            m.acesso      = {j.value("ip", std::string()), j.value("agente", std::string()), j.value("codigo", std::string())};
        }
    }
    std::thread(gerente).detach();
}

void lembrar(MachineObject* o)
{
    if (o == nullptr || !acompanhavel(o->printer_agent_id) || o->get_dev_ip().empty())
        return;
    const std::string dev = o->get_dev_id();
    const Acesso      a{o->get_dev_ip(), o->printer_agent_id, o->get_access_code()};
    {
        std::lock_guard<std::mutex> l(mtx());
        Monitorada&                 m = mapa()[dev];
        if (m.acesso.ip == a.ip && m.acesso.agente == a.agente && m.acesso.codigo == a.codigo)
            return;
        m.acesso = a;
        if (m.agente || m.conectando)
            desligar(m); // mudou IP/código: a próxima conexão usa os novos
    }
    if (AppConfig* cfg = wxGetApp().app_config)
        cfg->set(SECAO, dev, json{{"ip", a.ip}, {"agente", a.agente}, {"codigo", a.codigo}}.dump());
}

void ativa(const std::string& dev_id)
{
    std::lock_guard<std::mutex> l(mtx());
    if (ativa_id() == dev_id)
        return;
    ativa_id() = dev_id;
    auto it    = mapa().find(dev_id);
    if (it != mapa().end() && (it->second.agente || it->second.conectando))
        desligar(it->second);
}

json estado_de(const std::string& dev_id)
{
    std::lock_guard<std::mutex> l(mtx());
    auto                        it = mapa().find(dev_id);
    if (dev_id.empty() || it == mapa().end() || dev_id == ativa_id())
        return json::object();
    const Monitorada& m      = it->second;
    const json&       p      = m.relatorio;
    const bool        ligada = m.conectada && Clock::now() - m.ultima_msg < std::chrono::minutes(5);
    json              r      = {{"ao_vivo", true}, {"ligada", ligada}};
    if (!ligada) {
        r["estado"] = "desligada";
        return r;
    }
    const std::string gs = p.value("gcode_state", std::string());
    r["estado"]          = gs == "RUNNING" || gs == "PREPARE" || gs == "SLICING" ? "imprimindo"
                         : gs == "PAUSE"                                         ? "pausada"
                         : gs == "FINISH"                                        ? "concluida"
                         : gs == "FAILED"                                        ? "falhou"
                                                                                 : "livre";
    r["progresso"]  = int(num(p, "mc_percent"));
    r["restante_s"] = int(num(p, "mc_remaining_time") * 60);
    r["bico"]       = {{"atual", num(p, "nozzle_temper")}, {"alvo", num(p, "nozzle_target_temper")}};
    r["mesa"]       = {{"atual", num(p, "bed_temper")}};
    json cores      = json::array();
    if (p.contains("ams") && p["ams"].is_object() && p["ams"].contains("ams") && p["ams"]["ams"].is_array())
        for (const auto& u : p["ams"]["ams"]) {
            if (!u.is_object() || !u.contains("tray") || !u["tray"].is_array())
                continue;
            const std::string uid = u.contains("id") ? (u["id"].is_string() ? u["id"].get<std::string>() : u["id"].dump()) : std::string("0");
            for (const auto& t : u["tray"]) {
                if (!t.is_object())
                    continue;
                const std::string tid = t.contains("id") ? (t["id"].is_string() ? t["id"].get<std::string>() : t["id"].dump()) : std::string();
                const std::string cor = t.value("tray_color", std::string());
                cores.push_back({{"unidade", uid}, {"slot", tid}, {"cor", cor}, {"material", t.value("tray_type", std::string())},
                                 {"marca", t.value("tray_sub_brands", std::string())}, {"vazio", cor.empty()}});
            }
        }
    r["cores"] = cores;
    return r;
}

std::vector<std::string> acompanhadas()
{
    std::lock_guard<std::mutex> l(mtx());
    std::vector<std::string>    v;
    for (const auto& [dev, m] : mapa())
        v.push_back(dev);
    return v;
}

void esquecer(const std::string& dev_id)
{
    {
        std::lock_guard<std::mutex> l(mtx());
        auto                        it = mapa().find(dev_id);
        if (it != mapa().end()) {
            desligar(it->second);
            mapa().erase(it);
        }
    }
    if (AppConfig* cfg = wxGetApp().app_config)
        cfg->erase(SECAO, dev_id);
}

std::vector<Ligada> ligadas()
{
    std::lock_guard<std::mutex> l(mtx());
    std::vector<Ligada>         v;
    for (const auto& [dev, m] : mapa())
        if (dev != ativa_id() && m.agente && m.conectada && Clock::now() - m.ultima_msg < std::chrono::minutes(5))
            v.push_back({dev, m.acesso.agente, m.relatorio});
    return v;
}

namespace {
std::shared_ptr<IPrinterAgent> agente_ligado(const std::string& dev_id)
{
    std::lock_guard<std::mutex> l(mtx());
    auto                        it = mapa().find(dev_id);
    if (it == mapa().end() || dev_id == ativa_id() || !it->second.conectada)
        return nullptr;
    return it->second.agente;
}
} // namespace

int imprimir(const std::string& dev_id, PrintParams p, std::string& erro)
{
    std::shared_ptr<IPrinterAgent> ag = agente_ligado(dev_id);
    if (!ag) {
        erro = "a impressora não está conectada";
        return -1;
    }
    int r = -1;
    try {
        r = ag->start_local_print(p, [&erro](int etapa, int, std::string msg) { if (etapa == PrintingStageERROR) erro = msg; },
                                  [] { return false; });
    } catch (const std::exception& e) {
        erro = e.what();
    }
    return r;
}

int comando(const std::string& dev_id, const std::string& acao)
{
    std::shared_ptr<IPrinterAgent> ag = agente_ligado(dev_id);
    if (!ag)
        return -1;
    // mesmo formato dos comandos do programa (MachineObject::command_task_pause/resume/abort)
    static std::atomic<unsigned> seq{1000};
    const std::string cmd = acao == "pausar" ? "pause" : acao == "retomar" ? "resume" : acao == "cancelar" ? "stop" : "";
    if (cmd.empty())
        return -1;
    const json j = {{"print", {{"command", cmd}, {"param", ""}, {"sequence_id", std::to_string(seq++)}}}};
    try {
        return ag->send_message_to_printer(dev_id, j.dump(), 1, 0);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna ao vivo: comando " << acao << " em " << dev_id << ": " << e.what();
        return -1;
    }
}

}}} // namespace Slic3r::GUI::FilunaAoVivo
