#pragma once

// Filuna — cliente MQTT 3.1.1 mínimo (QoS 0) sobre TLS ou TCP, sem biblioteca extra: o que o modo
// LAN aberto das impressoras que falam MQTT precisa (Bambu Lab: TLS 8883, usuário "bblp" e código
// de acesso; Elegoo CC2 e Anycubic também usam MQTT local). Só conecta, assina, publica e mantém
// a sessão com PINGREQ; uma thread própria lê e entrega cada PUBLISH recebido.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Slic3r { namespace Filuna {

namespace mqtt {
// Codificação dos pacotes, exposta para os testes.
std::string encode_remaining_length(std::size_t n);
std::string encode_string(const std::string& s);
std::string connect_packet(const std::string& client_id, const std::string& user, const std::string& password, std::uint16_t keepalive_s);
std::string subscribe_packet(std::uint16_t packet_id, const std::string& topic);
std::string publish_packet(const std::string& topic, const std::string& payload);
// Lê um pacote completo do início de 'buf'. Devolve false se ainda faltam bytes.
bool        decode_packet(const std::string& buf, std::uint8_t& type_flags, std::string& body, std::size_t& consumed);
bool        decode_publish(std::uint8_t type_flags, const std::string& body, std::string& topic, std::string& payload);
} // namespace mqtt

class MqttClient
{
public:
    struct Options
    {
        std::string host;
        std::uint16_t port = 8883;
        bool        tls = true;
        bool        verify_peer = false; // impressoras usam certificado próprio, sem cadeia pública
        std::string client_id;
        std::string username;
        std::string password;
        std::chrono::seconds keepalive{30};
        std::chrono::seconds connect_timeout{10};
    };
    using MessageFn = std::function<void(const std::string& topic, const std::string& payload)>;
    using ClosedFn  = std::function<void(const std::string& reason)>;

    MqttClient();
    ~MqttClient();
    MqttClient(const MqttClient&)            = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    // Conecta (bloqueia até CONNACK ou erro), assina 'subscribe_topics' e inicia a leitura.
    bool connect(const Options& opt, const std::vector<std::string>& subscribe_topics, MessageFn on_message, ClosedFn on_closed,
                 std::string& error);
    bool publish(const std::string& topic, const std::string& payload);
    void close();
    bool connected() const { return m_connected.load(); }

private:
    struct Impl;
    std::unique_ptr<Impl> m;
    std::atomic<bool>     m_connected{false};
};

}} // namespace Slic3r::Filuna
