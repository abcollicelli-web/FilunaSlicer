#pragma once

// Filuna — agente LAN nativo da Elegoo Centauri Carbon 2, escrito a partir do protocolo do SDK
// aberto Elegoo-Link (Apache-2.0, adaptador elegoo_fdm_cc2): descoberta UDP 52700 {"method":7000},
// MQTT na porta 1883 (usuário "elegoo", senha = código de acesso), registro em
// elegoo/<sn>/api_register, pedidos em elegoo/<sn>/<cliente>/api_request e respostas/estado em
// .../api_response e elegoo/<sn>/api_status; envio do arquivo por HTTP PUT /upload em pedaços de
// 1 MB com Content-Range, X-File-Name, X-File-MD5 e X-Token.
//
// O estado da CC2 é traduzido para o relatório "push_status" que o DeviceManager entende, e os
// comandos dele (pausar, retomar, parar, pedir tudo) viram os métodos da CC2 (1021/1023/1022/1002).

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

namespace Filuna { class MqttClient; }

static constexpr char FILUNA_ELEGOO_CC2_AGENT_ID[] = "filuna_elegoo_cc2";

class FilunaElegooCC2Agent : public IPrinterAgent
{
public:
    explicit FilunaElegooCC2Agent(std::string log_dir);
    ~FilunaElegooCC2Agent() override;

    static AgentInfo get_agent_info_static();
    AgentInfo        get_agent_info() override { return get_agent_info_static(); }
    std::string      default_lan_username() const override { return "elegoo"; }

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

    FilamentSyncMode get_filament_sync_mode() const override { return FilamentSyncMode::subscription; }

    struct Ports
    {
        unsigned short mqtt = 1883;
        unsigned short http = 80;
        unsigned short discovery = 52700;
    };
    void set_ports(const Ports& p) { m_ports = p; }

    // Tradução de estado e de comandos, expostas para os testes.
    static nlohmann::json to_push_status(const nlohmann::json& cc2_status);
    // Devolve {} se o comando não tem equivalente na CC2.
    static nlohmann::json to_cc2_request(const nlohmann::json& device_manager_command, int request_id);
    // Lê a resposta ao {"method":7000} e devolve o JSON de descoberta do DeviceManager.
    static std::string discovery_reply_to_json(const std::string& reply, const std::string& sender_ip);

private:
    bool upload_http(const std::string& local_path, const std::string& remote_name, OnUpdateStatusFn update_fn,
                     WasCancelledFn cancel_fn, std::string& error);
    bool request(int method, const nlohmann::json& params);
    void on_mqtt(const std::string& topic, const std::string& payload);
    void dispatch(const std::string& payload);
    void dispatch_connect(int state, const std::string& msg);

    std::recursive_mutex                m_mutex;
    std::unique_ptr<Filuna::MqttClient> m_mqtt;
    std::string                         m_sn, m_ip, m_code, m_client_id, m_selected;
    int                                 m_next_id = 1;
    nlohmann::json                      m_status = nlohmann::json::object(); // estado completo, mesclado
    Ports                               m_ports;
    std::map<std::string, std::string>  m_seen;
    std::atomic<bool>                   m_discovering{false};
    std::thread                         m_discovery;

    OnMsgArrivedFn       m_on_ssdp;
    OnPrinterConnectedFn m_on_connected;
    OnMessageFn          m_on_message, m_on_local_message;
    OnLocalConnectedFn   m_on_local_connect;
    QueueOnMainFn        m_queue_on_main;
};

} // namespace Slic3r
