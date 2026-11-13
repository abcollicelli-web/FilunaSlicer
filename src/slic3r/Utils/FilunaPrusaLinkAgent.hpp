#pragma once

// Filuna — agente LAN nativo da Prusa pela API PrusaLink v1 (MK4/MK4S, MK3.9, CORE One, XL, MINI),
// aberta e documentada pela Prusa: autenticação Digest (usuário "maker" e senha da tela) ou
// X-Api-Key; GET /api/v1/status; PUT /api/v1/files/usb/<arquivo> com Print-After-Upload;
// PUT /api/v1/job/<id>/pause|resume e DELETE /api/v1/job/<id>. O estado vira o "push_status"
// do DeviceManager. A PrusaLink não expõe cores do MMU nem comando de temperatura.

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "IPrinterAgent.hpp"
#include "bambu_networking.hpp"

namespace Slic3r {

static constexpr char FILUNA_PRUSALINK_AGENT_ID[] = "filuna_prusalink";

class FilunaPrusaLinkAgent : public IPrinterAgent
{
public:
    explicit FilunaPrusaLinkAgent(std::string log_dir);
    ~FilunaPrusaLinkAgent() override;

    static AgentInfo get_agent_info_static();
    AgentInfo        get_agent_info() override { return get_agent_info_static(); }
    std::string      default_lan_username() const override { return "maker"; }

    void set_cloud_agent(std::shared_ptr<ICloudServiceAgent>) override {}
    int  send_message(std::string dev_id, std::string json_str, int qos, int flag) override;
    int  connect_printer(const PrinterConnectionParams& params) override;
    int  disconnect_printer() override;
    int  send_message_to_printer(std::string dev_id, std::string json_str, int qos, int flag) override;
    bool start_discovery(bool, bool) override { return true; } // a PrusaLink se anuncia por mDNS (Bonjour do Orca)
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

    void set_poll_ms(int ms) { m_poll_ms = ms; }

    static nlohmann::json to_push_status(const nlohmann::json& status);
    // Cabeçalho Authorization: Digest para o desafio WWW-Authenticate recebido (RFC 2617, qop=auth).
    static std::string digest_header(const std::string& challenge, const std::string& user, const std::string& password,
                                     const std::string& method, const std::string& uri, const std::string& cnonce, int nc);

private:
    // Requisição com Digest/X-Api-Key; devolve o código HTTP (0 = falha de rede)
    int  call(const std::string& method, const std::string& path, const std::string& body, std::string& reply,
              const std::map<std::string, std::string>& headers = {});
    int  upload(const PrintParams& p, bool print, OnUpdateStatusFn u, std::string& error);
    void poll_loop();
    void dispatch(const std::string& payload);
    void dispatch_connect(int state, const std::string& msg);

    std::recursive_mutex m_mutex;
    std::string          m_ip, m_port = "80", m_user, m_password, m_api_key, m_dev_id, m_selected, m_challenge;
    int                  m_nc = 0, m_poll_ms = 1000, m_job_id = 0;
    std::atomic<bool>    m_polling{false};
    std::thread          m_poll;

    OnMsgArrivedFn       m_on_ssdp;
    OnPrinterConnectedFn m_on_connected;
    OnMessageFn          m_on_message, m_on_local_message;
    OnLocalConnectedFn   m_on_local_connect;
    QueueOnMainFn        m_queue_on_main;
};

} // namespace Slic3r
