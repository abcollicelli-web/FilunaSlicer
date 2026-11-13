#pragma once

// Filuna — agente LAN aberto para impressoras Bambu Lab, sem o plug-in fechado bambu_networking
// (REGRA-CANONICA-CONECTIVIDADE-OPEN). Fala o mesmo protocolo local que a impressora oferece no
// "modo LAN": MQTT sobre TLS na porta 8883 (usuário "bblp", senha = código de acesso), envio do
// arquivo por FTPS implícito na porta 990, e descoberta pelos anúncios SSDP na porta UDP 2021.
//
// Os relatórios JSON da impressora (push_status, AMS, HMS) passam sem alteração para o
// DeviceManager, que já entende esse formato; os comandos do DeviceManager (pausar, retomar,
// parar, temperaturas, AMS) vão sem alteração para device/<serial>/request.

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "IPrinterAgent.hpp"
#include "bambu_networking.hpp"

namespace Slic3r {

namespace Filuna { class MqttClient; }

static constexpr char FILUNA_BAMBU_LAN_AGENT_ID[] = "filuna_bambu_lan";

class FilunaBambuLanAgent : public IPrinterAgent
{
public:
    explicit FilunaBambuLanAgent(std::string log_dir);
    ~FilunaBambuLanAgent() override;

    static AgentInfo get_agent_info_static();
    AgentInfo        get_agent_info() override { return get_agent_info_static(); }
    std::string      default_lan_username() const override { return "bblp"; }

    void set_cloud_agent(std::shared_ptr<ICloudServiceAgent> cloud) override { m_cloud = std::move(cloud); }

    int send_message(std::string dev_id, std::string json_str, int qos, int flag) override;
    int connect_printer(const PrinterConnectionParams& params) override;
    int disconnect_printer() override;
    int send_message_to_printer(std::string dev_id, std::string json_str, int qos, int flag) override;

    bool start_discovery(bool start, bool sending) override;

    int bind_detect(std::string dev_ip, std::string sec_link, detectResult& detect) override;
    int set_server_callback(OnServerErrFn fn) override;

    std::string get_user_selected_machine() override;
    int         set_user_selected_machine(std::string dev_id) override;

    int start_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, OnWaitFn wait_fn) override;
    int start_local_print_with_record(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, OnWaitFn wait_fn) override;
    int start_send_gcode_to_sdcard(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, OnWaitFn wait_fn) override;
    int start_local_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn) override;
    int start_sdcard_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn) override;

    int set_on_ssdp_msg_fn(OnMsgArrivedFn fn) override;
    int set_on_printer_connected_fn(OnPrinterConnectedFn fn) override;
    int set_on_subscribe_failure_fn(GetSubscribeFailureFn fn) override;
    int set_on_message_fn(OnMessageFn fn) override;
    int set_on_user_message_fn(OnMessageFn fn) override;
    int set_on_local_connect_fn(OnLocalConnectedFn fn) override;
    int set_on_local_message_fn(OnMessageFn fn) override;
    int set_queue_on_main_fn(QueueOnMainFn fn) override;

    FilamentSyncMode get_filament_sync_mode() const override { return FilamentSyncMode::subscription; }

    // Portas do protocolo; os testes apontam para emuladores locais.
    struct Ports
    {
        unsigned short mqtt = 8883;
        unsigned short ftps = 990;
        unsigned short ssdp = 2021;
        bool           tls  = true;
    };
    void set_ports(const Ports& p) { m_ports = p; }

    // Monta o comando "project_file" que inicia a impressão do 3MF já enviado.
    static std::string project_file_command(const PrintParams& params, const std::string& remote_name);
    // Lê um anúncio SSDP da Bambu e devolve o JSON de descoberta do DeviceManager (vazio se não for Bambu).
    static std::string ssdp_to_discovery_json(const std::string& datagram, const std::string& sender_ip);

private:
    bool upload_ftps(const PrintParams& params, const std::string& local_path, const std::string& remote_name,
                     OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, std::string& error);
    void dispatch_local_connect(int state, const std::string& dev_id, const std::string& msg);
    void dispatch_message(const std::string& dev_id, const std::string& payload);
    void discovery_loop();

    std::shared_ptr<ICloudServiceAgent> m_cloud;
    std::recursive_mutex                m_mutex;
    std::unique_ptr<Filuna::MqttClient> m_mqtt;
    std::string                         m_dev_id, m_dev_ip, m_access_code, m_selected;
    Ports                               m_ports;

    OnMsgArrivedFn        m_on_ssdp;
    OnPrinterConnectedFn  m_on_connected;
    GetSubscribeFailureFn m_on_sub_fail;
    OnMessageFn           m_on_message, m_on_user_message, m_on_local_message;
    OnLocalConnectedFn    m_on_local_connect;
    QueueOnMainFn         m_queue_on_main;
    OnServerErrFn         m_on_server_err;

    std::atomic<bool> m_discovering{false};
    std::thread       m_discovery;
    std::map<std::string, std::string> m_seen; // serial -> último JSON de descoberta
};

} // namespace Slic3r
