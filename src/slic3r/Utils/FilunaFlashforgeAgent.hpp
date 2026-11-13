#pragma once

// Filuna — agente LAN nativo da Flashforge (Adventurer 5M/5M Pro, AD5X, Creator 5) pela API local
// HTTP das impressoras (porta 8898), sem a biblioteca fechada FlashNetwork do Orca-Flashforge:
//   descoberta UDP 48899 (mensagem de 20 bytes, resposta com nome em 0x00 e série em 0x92),
//   POST /detail {serialNumber, checkCode}            -> estado, temperaturas, estação de material (IFS)
//   POST /uploadGcode (multipart + cabeçalhos)         -> envio e início (printNow, materialMappings)
//   POST /control {payload:{cmd:"jobCtl_cmd", args:{action:"pause"|"continue"|"cancel"}}}
// O estado é traduzido para o "push_status" do DeviceManager; a estação IFS vira unidade "ams".

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "IPrinterAgent.hpp"
#include "bambu_networking.hpp"

namespace Slic3r {

static constexpr char FILUNA_FLASHFORGE_AGENT_ID[] = "filuna_flashforge_lan";

class FilunaFlashforgeAgent : public IPrinterAgent
{
public:
    explicit FilunaFlashforgeAgent(std::string log_dir);
    ~FilunaFlashforgeAgent() override;

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

    FilamentSyncMode get_filament_sync_mode() const override { return FilamentSyncMode::subscription; }

    struct Ports
    {
        unsigned short http      = 8898;
        unsigned short discovery = 48899;
        unsigned short listen    = 18007;
        int            poll_ms   = 1000;
    };
    void set_ports(const Ports& p) { m_ports = p; }

    static nlohmann::json to_push_status(const nlohmann::json& detail_reply);
    static std::string    action_for(const nlohmann::json& device_manager_command); // "" = sem equivalente
    static bool           parse_discovery(const std::string& datagram, std::string& name, std::string& serial);

private:
    bool post_json(const std::string& path, const nlohmann::json& body, nlohmann::json& reply, std::string& error);
    bool upload(const PrintParams& p, bool print_now, OnUpdateStatusFn update_fn, std::string& error);
    void poll_loop();
    void dispatch(const std::string& payload);
    void dispatch_connect(int state, const std::string& msg);

    std::recursive_mutex m_mutex;
    std::string          m_sn, m_ip, m_code, m_selected;
    Ports                m_ports;
    std::map<std::string, std::string> m_seen;
    std::atomic<bool>    m_polling{false}, m_discovering{false};
    std::thread          m_poll, m_discovery;

    OnMsgArrivedFn       m_on_ssdp;
    OnPrinterConnectedFn m_on_connected;
    OnMessageFn          m_on_message, m_on_local_message;
    OnLocalConnectedFn   m_on_local_connect;
    QueueOnMainFn        m_queue_on_main;
};

} // namespace Slic3r
