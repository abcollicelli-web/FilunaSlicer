#pragma once

// Filuna — agente LAN nativo da Anycubic (Kobra 3 / 3 V2 / 3 Max / S1, firmware de fábrica em "Modo LAN").
// Protocolo documentado e validado em hardware real pelo projeto aberto anycubic_ha_local (AGPL-3.0,
// research/PROTOCOL-VALIDATED.md): NÃO é Moonraker (a porta 7125 fica fechada no firmware original).
//   1. GET  http://<ip>:18910/info           -> token, ctrlInfoUrl, modelId, cn (série), modelName, ctrlType
//   2. POST <ctrlInfoUrl>?ts&nonce&sign&did  -> sign = md5(md5(token[:16]) + ts + nonce)
//      resposta data.info em AES-128-CBC (chave token[16:32], IV = data.token) -> broker, usuário, senha, deviceId
//   3. MQTT sobre TLS na porta do broker (9883), certificado próprio da impressora
//      pedidos:   anycubic/anycubicCloud/v1/web/printer/<modelId>/<deviceId>/<tipo>   {type,action,timestamp,msgid,data}
//      relatórios: anycubic/anycubicCloud/v1/printer/public/<modelId>/<deviceId>/<tipo>/report
//   4. envio do arquivo: POST multipart em info.fileUploadurl (campos "filename" e "gcode", X-File-Length)
// O estado é traduzido para o "push_status" que o DeviceManager entende; ACE Pro vira unidade "ams".
// Descoberta: varredura da sub-rede local na porta 18910 (o firmware não anuncia por mDNS/SSDP de forma útil).

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

static constexpr char FILUNA_ANYCUBIC_LAN_AGENT_ID[] = "filuna_anycubic_lan";

class FilunaAnycubicLanAgent : public IPrinterAgent
{
public:
    explicit FilunaAnycubicLanAgent(std::string log_dir);
    ~FilunaAnycubicLanAgent() override;

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

    // Partes puras do protocolo, expostas para os testes.
    static std::string    sign(const std::string& token, long long ts, const std::string& nonce);
    static std::string    decrypt_ctrl(const std::string& info_b64, const std::string& token, const std::string& local_token);
    static nlohmann::json to_push_status(const nlohmann::json& estado);
    static std::string    info_to_discovery_json(const nlohmann::json& info, const std::string& ip);

    struct Sessao
    {
        std::string broker_host, usuario, senha, device_id, model_id, serie, modelo;
        unsigned short broker_port = 9883;
    };

private:
    bool aperto_de_mao(const std::string& ip, Sessao& s, std::string& erro);
    bool pedir(const std::string& tipo, const std::string& acao, const nlohmann::json& dados);
    bool enviar_arquivo(const std::string& local, const std::string& remoto, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn,
                        std::string& erro);
    void on_mqtt(const std::string& topic, const std::string& payload);
    void dispatch(const std::string& payload);
    void dispatch_connect(int state, const std::string& msg);

    std::recursive_mutex                m_mutex;
    std::unique_ptr<Filuna::MqttClient> m_mqtt;
    Sessao                              m_s;
    std::string                         m_ip, m_selected, m_client_id;
    nlohmann::json                      m_estado = nlohmann::json::object(); // relatórios mesclados por tipo
    std::map<std::string, std::string>  m_seen;                             // série -> JSON de descoberta
    std::atomic<bool>                   m_discovering{false};
    std::thread                         m_discovery;

    OnMsgArrivedFn       m_on_ssdp;
    OnPrinterConnectedFn m_on_connected;
    OnMessageFn          m_on_message, m_on_local_message;
    OnLocalConnectedFn   m_on_local_connect;
    QueueOnMainFn        m_queue_on_main;
};

} // namespace Slic3r
