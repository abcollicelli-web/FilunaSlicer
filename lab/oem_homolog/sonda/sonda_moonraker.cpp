// Filuna — sonda de rede dos agentes Klipper/Moonraker que o Filuna já traz (MoonrakerPrinterAgent,
// SnapmakerPrinterAgent, QidiPrinterAgent), contra o emulador lab/oem_homolog/emuladores/moonraker.py
// ou contra uma impressora real. Fluxo: conectar, receber estado pelo websocket, enviar o arquivo,
// iniciar, pausar, retomar, mudar temperatura e cancelar, tudo pelos comandos que o DeviceManager
// manda ao agente. Sem a interface: os agentes não acham o preset ativo (wxTheApp nulo) e seguem
// sem ele, como em qualquer chamada antes da janela principal existir.
//
// filuna_sonda_moonraker --agente moonraker|snapmaker|qidi|creality --ip IP --porta 7125 --arquivo X --prova P [--sem-parar]
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "slic3r/Utils/CrealityPrintAgent.hpp"
#include "slic3r/Utils/MoonrakerPrinterAgent.hpp"
#include "slic3r/Utils/QidiPrinterAgent.hpp"
#include "slic3r/Utils/SnapmakerPrinterAgent.hpp"

using namespace Slic3r;
using json = nlohmann::json;

namespace {
std::mutex              mx;
std::condition_variable cv;
json                    ultimo;
int                     conexao = -1;

bool espera(std::chrono::seconds t, const std::function<bool()>& f)
{
    std::unique_lock<std::mutex> l(mx);
    return cv.wait_for(l, t, f);
}
} // namespace

int main(int argc, char** argv)
{
    std::map<std::string, std::string> a{{"--agente", "moonraker"}, {"--ip", "127.0.0.1"}, {"--porta", "7125"}};
    bool so_ler = false;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        if (k == "--sem-parar") { so_ler = true; continue; }
        if (i + 1 < argc) a[k] = argv[++i];
    }
    std::unique_ptr<IPrinterAgent> ag;
    if (a["--agente"] == "snapmaker")
        ag = std::make_unique<SnapmakerPrinterAgent>("");
    else if (a["--agente"] == "creality")
        ag = std::make_unique<CrealityPrintAgent>("");
    else if (a["--agente"] == "qidi")
        ag = std::make_unique<QidiPrinterAgent>("");
    else
        ag = std::make_unique<MoonrakerPrinterAgent>("");
    IPrinterAgent& agent = *ag;
    json prova = {{"agente", agent.get_agent_info().id}, {"ip", a["--ip"]}, {"etapas", json::array()},
                  {"nao_suportado_pelo_protocolo", json::array()}};
    bool ok_total = true;
    auto etapa = [&](const std::string& n, bool ok, const std::string& d = {}) {
        prova["etapas"].push_back({{"etapa", n}, {"ok", ok}, {"detalhe", d}});
        std::cout << (ok ? "PASS " : "FALHA ") << n << (d.empty() ? "" : " — " + d) << std::endl;
        ok_total &= ok;
    };
    agent.set_on_local_connect_fn([](int s, std::string, std::string) { std::lock_guard<std::mutex> l(mx); conexao = s; cv.notify_all(); });
    auto on_msg = [](std::string, std::string m) {
        json j = json::parse(m, nullptr, false);
        if (j.is_object() && j.contains("print")) {
            std::lock_guard<std::mutex> l(mx);
            if (ultimo.is_null())
                ultimo = json::object();
            ultimo.merge_patch(j["print"]);
            cv.notify_all();
        }
    };
    agent.set_on_local_message_fn(on_msg);
    agent.set_on_message_fn(on_msg);

    PrinterConnectionParams p;
    p.dev_id = a["--ip"] + ":" + a["--porta"];
    p.host   = a["--ip"];
    p.port   = a["--porta"];
    if (a.count("--chave"))
        p.password = a["--chave"];
    agent.connect_printer(p);
    etapa("conexao_moonraker", espera(std::chrono::seconds(15), [] { return conexao >= 0; }) && conexao == ConnectStatusOk,
          "estado " + std::to_string(conexao));
    etapa("estado_websocket", espera(std::chrono::seconds(15), [] { return !ultimo.is_null() && ultimo.contains("gcode_state"); }),
          ultimo.is_null() ? "" : ultimo.value("gcode_state", ""));

    // sistema de cores: o mesmo pedido que a GUI faz ao abrir o mapeamento de filamentos
    agent.fetch_filament_info(p.dev_id);
    int bandejas = 0;
    espera(std::chrono::seconds(15), [] { return ultimo.contains("ams"); });
    {
        std::lock_guard<std::mutex> l(mx);
        if (ultimo.contains("ams") && ultimo["ams"].contains("ams"))
            for (const auto& u : ultimo["ams"]["ams"])
                for (const auto& t : u.value("tray", json::array()))
                    bandejas += t.value("tray_type", std::string()).empty() ? 0 : 1;
    }
    if (a["--agente"] == "moonraker" && !a.count("--cores"))
        prova["nao_suportado_pelo_protocolo"].push_back("sistema_cores (Klipper sem AFC/Happy Hare nesta impressora)");
    else
        etapa("sistema_cores", bandejas > 0, std::to_string(bandejas) + " bandejas com filamento");

    if (!so_ler) {
        PrintParams pp{};
        pp.dev_id       = p.dev_id;
        pp.dev_ip       = a["--ip"];
        pp.filename     = a["--arquivo"];
        pp.project_name = "Filuna sonda";
        pp.plate_index  = 1;
        int pct = -1;
        std::string erro;
        const int rc = agent.start_local_print(pp, [&](int st, int code, std::string msg) {
            if (st == PrintingStageUpload) pct = code;
            if (st == PrintingStageERROR) erro = msg;
        }, [] { return false; });
        etapa("envio_e_inicio", rc == BAMBU_NETWORK_SUCCESS, erro);
        auto vira = [&](const char* s) {
            return espera(std::chrono::seconds(15), [&] { return ultimo.value("gcode_state", "") == s; });
        };
        etapa("inicio_impressao", vira("RUNNING"));
        const std::string base = R"({"print":{"sequence_id":"1","command":")";
        agent.send_message_to_printer(p.dev_id, base + "pause\"}}", 0, 0);
        etapa("pausar", vira("PAUSE"));
        agent.send_message_to_printer(p.dev_id, base + "resume\"}}", 0, 0);
        etapa("retomar", vira("RUNNING"));
        const int trc = agent.send_message_to_printer(p.dev_id, R"({"print":{"sequence_id":"2","command":"gcode_line","param":"M104 S200\n"}})", 0, 0);
        if (trc == ORCA_NETWORK_ERR_CMD_NOT_SUPPORTED)
            prova["nao_suportado_pelo_protocolo"].push_back("controle_temperatura");
        else
            etapa("controle_temperatura", espera(std::chrono::seconds(15), [] { return ultimo.value("nozzle_target_temper", 0.0) == 200.0; }));
        agent.send_message_to_printer(p.dev_id, base + "stop\"}}", 0, 0);
        etapa("parar", espera(std::chrono::seconds(15), [] {
                  const std::string s = ultimo.value("gcode_state", "");
                  return s == "FAILED" || s == "IDLE" || s == "FINISH";
              }));
    }
    agent.disconnect_printer();
    prova["aprovado"] = ok_total;
    std::ofstream(a["--prova"]) << prova.dump(1);
    return ok_total ? 0 : 1;
}
