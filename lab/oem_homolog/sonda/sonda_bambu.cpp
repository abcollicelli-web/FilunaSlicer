// Filuna — sonda de rede da Bambu Lab pelo agente nativo FilunaBambuLanAgent (sem plug-in fechado).
// Roda o fluxo inteiro contra uma impressora (emulada ou real): descobrir, recusar código errado,
// conectar, ler estado e AMS, enviar o 3MF por FTPS, iniciar com o mapeamento do AMS, pausar,
// retomar e parar. Escreve uma prova JSON com o resultado de cada etapa.
//
// sonda_bambu --ip IP --serial SERIAL --codigo CODIGO --arquivo cubo.gcode.3mf --prova prova.json
//             [--mqtt 8883 --ftps 990 --ssdp 2021] [--sem-parar]   (--sem-parar: impressora real, só lê)
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "slic3r/Utils/FilunaBambuLanAgent.hpp"

using namespace Slic3r;
using json = nlohmann::json;

namespace {
std::mutex              mx;
std::condition_variable cv;
json                    ultimo; // último push_status
std::string             descoberto;
int                     estado_conexao = -1;

bool espera(std::chrono::seconds t, const std::function<bool()>& f)
{
    std::unique_lock<std::mutex> l(mx);
    return cv.wait_for(l, t, f);
}
} // namespace

int main(int argc, char** argv)
{
    std::map<std::string, std::string> a{{"--ip", "127.0.0.1"}, {"--mqtt", "8883"}, {"--ftps", "990"}, {"--ssdp", "2021"}};
    bool so_ler = false;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        if (k == "--sem-parar") { so_ler = true; continue; }
        if (i + 1 < argc) a[k] = argv[++i];
    }
    json prova = {{"fabricante", "bambu"}, {"agente", FILUNA_BAMBU_LAN_AGENT_ID}, {"ip", a["--ip"]}, {"etapas", json::array()}};
    bool tudo_ok = true;
    auto etapa = [&](const std::string& nome, bool ok, const std::string& det = {}) {
        prova["etapas"].push_back({{"etapa", nome}, {"ok", ok}, {"detalhe", det}});
        std::cout << (ok ? "PASS " : "FALHA ") << nome << (det.empty() ? "" : " — " + det) << std::endl;
        tudo_ok &= ok;
    };

    FilunaBambuLanAgent agent("");
    agent.set_ports({(unsigned short) std::stoi(a["--mqtt"]), (unsigned short) std::stoi(a["--ftps"]), (unsigned short) std::stoi(a["--ssdp"]), true});
    agent.set_on_ssdp_msg_fn([](std::string js) {
        std::lock_guard<std::mutex> l(mx);
        descoberto = js;
        cv.notify_all();
    });
    agent.set_on_local_connect_fn([](int s, std::string, std::string) {
        std::lock_guard<std::mutex> l(mx);
        estado_conexao = s;
        cv.notify_all();
    });
    agent.set_on_local_message_fn([](std::string, std::string m) {
        json j = json::parse(m, nullptr, false);
        if (j.is_object() && j.contains("print") && j["print"].value("command", "") == "push_status") {
            std::lock_guard<std::mutex> l(mx);
            ultimo = j["print"];
            cv.notify_all();
        }
    });

    // 1. descoberta
    agent.start_discovery(true, false);
    const bool achou = espera(std::chrono::seconds(10), [&] { return !descoberto.empty(); });
    agent.start_discovery(false, false);
    std::string serial = a["--serial"];
    if (achou) {
        json d = json::parse(descoberto);
        etapa("descoberta_ssdp", d.value("dev_id", "") == serial || serial.empty(), d.dump());
        if (serial.empty())
            serial = d.value("dev_id", "");
    } else
        etapa("descoberta_ssdp", false, "nenhum anúncio em 10 s");

    PrinterConnectionParams p;
    p.dev_id = serial;
    p.host   = a["--ip"];
    p.port   = a["--mqtt"];

    // 2. código errado é recusado
    p.password = "00000000";
    etapa("recusa_codigo_errado", agent.connect_printer(p) != BAMBU_NETWORK_SUCCESS && estado_conexao == ConnectStatusFailed);

    // 3. conexão
    p.password = a["--codigo"];
    const int rc = agent.connect_printer(p);
    etapa("conexao_mqtt_tls", rc == BAMBU_NETWORK_SUCCESS && estado_conexao == ConnectStatusOk);
    if (rc != BAMBU_NETWORK_SUCCESS) {
        std::ofstream(a["--prova"]) << prova.dump(1);
        return 1;
    }

    // 4. estado + sistema de cores
    const bool tem = espera(std::chrono::seconds(10), [&] { return !ultimo.is_null(); });
    etapa("estado_inicial", tem, tem ? ultimo.value("gcode_state", "") : "");
    int bandejas = 0;
    if (tem && ultimo.contains("ams") && ultimo["ams"].contains("ams"))
        for (const auto& u : ultimo["ams"]["ams"])
            bandejas += int(u.value("tray", json::array()).size());
    etapa("sistema_cores_ams", bandejas > 0, std::to_string(bandejas) + " bandejas");

    if (!so_ler) {
        // 5. envio + início com mapeamento do AMS
        PrintParams pp{};
        pp.dev_id        = serial;
        pp.dev_ip        = a["--ip"];
        pp.password      = a["--codigo"];
        pp.filename      = a["--arquivo"];
        pp.project_name  = "Filuna sonda";
        pp.plate_index   = 1;
        pp.task_use_ams  = true;
        pp.ams_mapping   = "[0,1,2,3]";
        int  ultimo_pct  = -1;
        std::string erro;
        const int prc = agent.start_local_print(pp, [&](int st, int code, std::string msg) {
            if (st == PrintingStageUpload) ultimo_pct = code;
            if (st == PrintingStageERROR) erro = msg;
        }, [] { return false; });
        etapa("envio_ftps", prc == BAMBU_NETWORK_SUCCESS && ultimo_pct == 100, erro.empty() ? "100%" : erro);
        auto estado_vira = [&](const char* s) {
            return espera(std::chrono::seconds(10), [&] { return ultimo.value("gcode_state", "") == s; });
        };
        etapa("inicio_impressao", estado_vira("RUNNING"));
        const std::string base = R"({"print":{"sequence_id":"1","command":")";
        agent.send_message_to_printer(serial, base + "pause\"}}", 0, 0);
        etapa("pausar", estado_vira("PAUSE"));
        agent.send_message_to_printer(serial, base + "resume\"}}", 0, 0);
        etapa("retomar", estado_vira("RUNNING"));
        agent.send_message_to_printer(serial, R"({"print":{"sequence_id":"2","command":"gcode_line","param":"M104 S200\n"}})", 0, 0);
        etapa("controle_temperatura", espera(std::chrono::seconds(10), [&] { return ultimo.value("nozzle_target_temper", 0.0) == 200.0; }));
        agent.send_message_to_printer(serial, base + "stop\"}}", 0, 0);
        etapa("parar", estado_vira("FAILED"));
    }
    agent.disconnect_printer();
    prova["aprovado"] = tudo_ok;
    prova["resumo"]   = {{"etapas", prova["etapas"].size()}, {"aprovado", tudo_ok}};
    std::ofstream(a["--prova"]) << prova.dump(1);
    return tudo_ok ? 0 : 1;
}
