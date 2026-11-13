#pragma once

// Filuna — agente LAN nativo das Elegoo com SDCP v3 (Centauri Carbon e linha Saturn/Mars de resina
// fora do escopo FDM), protocolo aberto publicado pela CBD (cbd-tech/SDCP) e usado pelo adaptador
// elegoo_fdm_cc do SDK aberto Elegoo-Link: descoberta UDP 3000 ("M99999"), WebSocket 3030
// /websocket com tópicos sdcp/request|response|status/<MainboardID>, envio por HTTP POST
// /uploadFile/upload em pedaços (S-File-MD5, Check, Offset, Uuid, TotalSize, File).
// Comandos: 0 estado, 1 atributos, 128 iniciar, 129 pausar, 130 parar, 131 retomar.

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "IPrinterAgent.hpp"
#include "bambu_networking.hpp"

namespace Slic3r {

static constexpr char FILUNA_ELEGOO_SDCP_AGENT_ID[] = "filuna_elegoo_sdcp";

class FilunaElegooSDCPAgent : public IPrinterAgent
{
public:
    explicit FilunaElegooSDCPAgent(std::string log_dir);
    ~FilunaElegooSDCPAgent() override;

    static AgentInfo get_agent_info_static();
    AgentInfo        get_agent_info() override { return get_agent_info_static(); }

    void set_cloud_agent(std::shared_ptr<ICloudServiceAgent>) override {}
    int  send_message(std::string dev_id, std::string json_str, int qos, int flag) override;
    int  connect_printer(const PrinterConnectionParams& params) override;
    int  disconnect_printer() override;
    int  send_message_to_printer(std::string dev_id, std::string json_str, int qos, int flag) override;
    bool start_discovery(bool start, bool sending) override;
    int  bind_detect(std::string dev_ip, std::string sec_link, detectResult& detect) override;
    int  set_server_callback(OnServerErrFn) override { return 0; }
    std::string get_user_selected_machine() override;
    int         set_user_selected_machine(std::string dev_id) override;

    int start_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, OnWaitFn wait_fn) override;
    int start_local_print_with_record(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, OnWaitFn wait_fn) override;
    int start_send_gcode_to_sdcard(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, OnWaitFn wait_fn) override;
    int start_local_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn) override;
    int start_sdcard_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn) override;

    int set_on_ssdp_msg_fn(OnMsgArrivedFn fn) override;
    int set_on_printer_connected_fn(OnPrinterConnectedFn fn) override;
    int set_on_subscribe_failure_fn(GetSubscribeFailureFn) override { return 0; }
    int set_on_message_fn(OnMessageFn fn) override;
    int set_on_user_message_fn(OnMessageFn) override { return 0; }
    int set_on_local_connect_fn(OnLocalConnectedFn fn) override;
    int set_on_local_message_fn(OnMessageFn fn) override;
    int set_queue_on_main_fn(QueueOnMainFn fn) override;

    FilamentSyncMode get_filament_sync_mode() const override { return FilamentSyncMode::none; }

    struct Ports
    {
        unsigned short ws = 3030;
        unsigned short http = 3030;
        unsigned short discovery = 3000;
    };
    void set_ports(const Ports& p) { m_ports = p; }

    static nlohmann::json to_push_status(const nlohmann::json& sdcp_status);
    static std::string    discovery_to_json(const std::string& reply, const std::string& ip);

private:
    struct Ws;
    bool send_cmd(int cmd, const nlohmann::json& data);
    bool upload(const std::string& local, const std::string& remote, OnUpdateStatusFn u, std::string& error);
    void read_loop();
    void dispatch(const std::string& payload);
    void dispatch_connect(int state, const std::string& msg);

    std::recursive_mutex m_mutex;
    std::unique_ptr<Ws>  m_ws;
    std::thread          m_reader;
    std::atomic<bool>    m_running{false}, m_discovering{false};
    std::thread          m_discovery;
    std::string          m_ip, m_mainboard, m_selected;
    nlohmann::json       m_status = nlohmann::json::object();
    Ports                m_ports;
    std::map<std::string, std::string> m_seen;

    OnMsgArrivedFn       m_on_ssdp;
    OnPrinterConnectedFn m_on_connected;
    OnMessageFn          m_on_message, m_on_local_message;
    OnLocalConnectedFn   m_on_local_connect;
    QueueOnMainFn        m_queue_on_main;
};

} // namespace Slic3r
