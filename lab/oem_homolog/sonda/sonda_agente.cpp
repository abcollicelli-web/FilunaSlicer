// Filuna — sonda de rede dos agentes LAN nativos do Filuna (sem plug-in fechado):
//   --agente bambu       FilunaBambuLanAgent  (SSDP 2021, MQTT/TLS 8883, FTPS 990)
//   --agente elegoo-cc2  FilunaElegooCC2Agent (UDP 52700, MQTT 1883, HTTP PUT /upload)
// Roda o fluxo inteiro contra uma impressora (emulada ou real): descobrir, recusar código errado,
// conectar, ler estado e sistema de cores, enviar o arquivo, iniciar com o mapeamento de cores,
// pausar, retomar, mudar temperatura (quando o protocolo tem) e parar. Grava uma prova JSON.
//
// sonda_agente --agente bambu --ip IP --serial SERIAL --codigo CODIGO --arquivo arquivo --prova prova.json
//              [--porta-a N --porta-b N --porta-c N] [--sem-parar]   (--sem-parar: impressora real, só lê)
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
#include "slic3r/Utils/FilunaElegooCC2Agent.hpp"
#include "slic3r/Utils/FilunaFlashforgeAgent.hpp"
#include "slic3r/Utils/FilunaPrusaLinkAgent.hpp"
#include <memory>

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
    std::map<std::string, std::string> a{{"--ip", "127.0.0.1"}, {"--agente", "bambu"}};
    bool so_ler = false;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        if (k == "--sem-parar") { so_ler = true; continue; }
        if (i + 1 < argc) a[k] = argv[++i];
    }
    const bool bambu = a["--agente"] == "bambu";
    auto porta = [&](const char* k, int padrao) { return (unsigned short) (a.count(k) ? std::stoi(a[k]) : padrao); };
    std::unique_ptr<IPrinterAgent> agente;
    if (bambu) {
        auto b = std::make_unique<FilunaBambuLanAgent>("");
        b->set_ports({porta("--porta-a", 8883), porta("--porta-b", 990), porta("--porta-c", 2021), true});
        agente = std::move(b);
    } else if (a["--agente"] == "prusa") {
        auto pl = std::make_unique<FilunaPrusaLinkAgent>("");
        pl->set_poll_ms(500);
        agente = std::move(pl);
    } else if (a["--agente"] == "flashforge") {
        auto f = std::make_unique<FilunaFlashforgeAgent>("");
        f->set_ports({porta("--porta-a", 8898), porta("--porta-b", 48899), porta("--porta-c", 18007), 500});
        agente = std::move(f);
    } else {
        auto e = std::make_unique<FilunaElegooCC2Agent>("");
        e->set_ports({porta("--porta-a", 1883), porta("--porta-b", 80), porta("--porta-c", 52700)});
        agente = std::move(e);
    }
    IPrinterAgent& agent = *agente;
    json prova = {{"fabricante", bambu ? "bambu" : (a["--agente"] == "flashforge" ? "flashforge" : (a["--agente"] == "prusa" ? "prusa" : "elegoo"))}, {"agente", agent.get_agent_info().id}, {"ip", a["--ip"]}, {"etapas", json::array()}};
    bool tudo_ok = true;
    auto etapa = [&](const std::string& nome, bool ok, const std::string& det = {}) {
        prova["etapas"].push_back({{"etapa", nome}, {"ok", ok}, {"detalhe", det}});
        std::cout << (ok ? "PASS " : "FALHA ") << nome << (det.empty() ? "" : " — " + det) << std::endl;
        tudo_ok &= ok;
    };

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

    const bool prusa = a["--agente"] == "prusa";
    // 1. descoberta (a PrusaLink só se anuncia por mDNS, que fica com o Bonjour do Orca)
    prova["nao_suportado_pelo_protocolo"] = json::array();
    bool achou = false;
    if (!prusa) {
        agent.start_discovery(true, false);
        achou = espera(std::chrono::seconds(10), [&] { return !descoberto.empty(); });
        agent.start_discovery(false, false);
    } else
        prova["nao_suportado_pelo_protocolo"].push_back("descoberta (mDNS, Bonjour do Orca, fora desta sonda)");
    std::string serial = a["--serial"];
    if (prusa) {
    } else if (achou) {
        json d = json::parse(descoberto);
        etapa(bambu ? "descoberta_ssdp" : "descoberta_udp", d.value("dev_id", "") == serial || serial.empty(), d.dump());
        if (serial.empty())
            serial = d.value("dev_id", "");
    } else
        etapa(bambu ? "descoberta_ssdp" : "descoberta_udp", false, "nenhum anúncio em 10 s");

    PrinterConnectionParams p;
    p.dev_id = serial;
    p.host   = a["--ip"];
    const bool ff = a["--agente"] == "flashforge";
    p.port   = std::to_string(porta("--porta-a", bambu ? 8883 : (ff ? 8898 : (prusa ? 80 : 1883))));
    if (prusa)
        p.username = "maker";

    // 2. código errado é recusado
    p.password = "00000000";
    etapa("recusa_codigo_errado", agent.connect_printer(p) != BAMBU_NETWORK_SUCCESS && estado_conexao == ConnectStatusFailed);

    // 3. conexão
    p.password = a["--codigo"];
    const int rc = agent.connect_printer(p);
    etapa(bambu ? "conexao_mqtt_tls" : (ff || prusa ? "conexao_http" : "conexao_mqtt"), rc == BAMBU_NETWORK_SUCCESS && estado_conexao == ConnectStatusOk);
    if (rc != BAMBU_NETWORK_SUCCESS) {
        std::ofstream(a["--prova"]) << prova.dump(1);
        return 1;
    }

    // 4. estado + sistema de cores
    const bool tem = espera(std::chrono::seconds(10), [&] { return !ultimo.is_null(); });
    etapa("estado_inicial", tem, tem ? ultimo.value("gcode_state", "") : "");
    // o sistema de cores pode chegar num relatório seguinte ao primeiro estado (CC2: canvas 2005)
    espera(std::chrono::seconds(5), [&] { return ultimo.contains("ams"); });
    int bandejas = 0;
    {
        std::lock_guard<std::mutex> l(mx);
        if (ultimo.contains("ams") && ultimo["ams"].contains("ams"))
            for (const auto& u : ultimo["ams"]["ams"])
                bandejas += int(u.value("tray", json::array()).size());
    }
    if (prusa)
        prova["nao_suportado_pelo_protocolo"].push_back("sistema_cores (a PrusaLink não expõe o MMU)");
    else
        etapa(bambu ? "sistema_cores_ams" : (ff ? "sistema_cores_ifs" : "sistema_cores_canvas"), bandejas > 0, std::to_string(bandejas) + " bandejas");

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
        etapa(bambu ? "envio_ftps" : "envio_http", prc == BAMBU_NETWORK_SUCCESS && ultimo_pct == 100, erro.empty() ? "100%" : erro);
        auto estado_vira = [&](const char* s) {
            return espera(std::chrono::seconds(10), [&] { return ultimo.value("gcode_state", "") == s; });
        };
        etapa("inicio_impressao", estado_vira("RUNNING"));
        const std::string base = R"({"print":{"sequence_id":"1","command":")";
        agent.send_message_to_printer(serial, base + "pause\"}}", 0, 0);
        etapa("pausar", estado_vira("PAUSE"));
        agent.send_message_to_printer(serial, base + "resume\"}}", 0, 0);
        etapa("retomar", estado_vira("RUNNING"));
        const int trc = agent.send_message_to_printer(serial, R"({"print":{"sequence_id":"2","command":"gcode_line","param":"M104 S200\n"}})", 0, 0);
        if (trc == ORCA_NETWORK_ERR_CMD_NOT_SUPPORTED)
            prova["nao_suportado_pelo_protocolo"].push_back("controle_temperatura");
        else
            etapa("controle_temperatura", espera(std::chrono::seconds(10), [&] { return ultimo.value("nozzle_target_temper", 0.0) == 200.0; }));
        agent.send_message_to_printer(serial, base + "stop\"}}", 0, 0);
        etapa("parar", estado_vira(bambu || ff || prusa ? "FAILED" : "IDLE"));
    }
    agent.disconnect_printer();
    prova["aprovado"] = tudo_ok;
    prova["resumo"]   = {{"etapas", prova["etapas"].size()}, {"aprovado", tudo_ok}};
    std::ofstream(a["--prova"]) << prova.dump(1);
    return tudo_ok ? 0 : 1;
}
