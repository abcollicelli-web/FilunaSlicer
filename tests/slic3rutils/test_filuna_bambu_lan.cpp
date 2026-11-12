#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <nlohmann/json.hpp>

#include "slic3r/Utils/FilunaBambuLanAgent.hpp"
#include "slic3r/Utils/FilunaMqttClient.hpp"

using namespace Slic3r;
namespace mqtt = Slic3r::Filuna::mqtt;
namespace net  = boost::asio;
using tcp      = net::ip::tcp;

namespace {

// Broker MQTT de teste (TCP sem TLS): aceita um cliente, confere usuário/senha, guarda o que ele
// publica e manda relatórios no tópico device/<serial>/report quando o teste pede.
class FakeBambuBroker
{
public:
    explicit FakeBambuBroker(std::string code) : m_code(std::move(code)), m_acceptor(m_ioc, tcp::endpoint(net::ip::make_address("127.0.0.1"), 0))
    {
        m_thread = std::thread([this] { run(); });
    }
    ~FakeBambuBroker()
    {
        m_stop = true;
        boost::system::error_code ec;
        m_acceptor.close(ec);
        {
            std::lock_guard<std::mutex> l(m_mx);
            if (m_sock)
                m_sock->close(ec);
        }
        m_thread.join();
    }
    unsigned short port() const { return m_acceptor.local_endpoint().port(); }

    std::vector<std::pair<std::string, std::string>> published()
    {
        std::lock_guard<std::mutex> l(m_mx);
        return m_published;
    }
    std::string user, password, subscribed;

    void report(const std::string& serial, const std::string& json)
    {
        std::lock_guard<std::mutex> l(m_mx);
        if (m_sock) {
            boost::system::error_code ec;
            net::write(*m_sock, net::buffer(mqtt::publish_packet("device/" + serial + "/report", json)), ec);
        }
    }
    bool wait_published(std::size_t n, std::chrono::milliseconds t = std::chrono::seconds(5))
    {
        std::unique_lock<std::mutex> l(m_mx);
        return m_cv.wait_for(l, t, [&] { return m_published.size() >= n; });
    }

private:
    void run()
    {
        boost::system::error_code ec;
        auto                      sock = std::make_shared<tcp::socket>(m_ioc);
        m_acceptor.accept(*sock, ec);
        if (ec)
            return;
        {
            std::lock_guard<std::mutex> l(m_mx);
            m_sock = sock;
        }
        std::string buf;
        char        tmp[4096];
        while (!m_stop) {
            const std::size_t n = sock->read_some(net::buffer(tmp), ec);
            if (ec)
                return;
            buf.append(tmp, n);
            std::uint8_t tf;
            std::string  body;
            std::size_t  used;
            while (mqtt::decode_packet(buf, tf, body, used)) {
                buf.erase(0, used);
                handle(*sock, tf, body);
            }
        }
    }
    static std::string read_str(const std::string& b, std::size_t& p)
    {
        const std::size_t l = (std::size_t(std::uint8_t(b[p])) << 8) | std::uint8_t(b[p + 1]);
        std::string       s = b.substr(p + 2, l);
        p += 2 + l;
        return s;
    }
    void handle(tcp::socket& s, std::uint8_t tf, const std::string& body)
    {
        boost::system::error_code ec;
        switch (tf & 0xF0) {
        case 0x10: { // CONNECT
            std::size_t p = 0;
            read_str(body, p); // "MQTT"
            const std::uint8_t flags = std::uint8_t(body[p + 1]);
            p += 4;
            read_str(body, p); // client id
            if (flags & 0x80) user = read_str(body, p);
            if (flags & 0x40) password = read_str(body, p);
            const char rc = (user == "bblp" && password == m_code) ? 0 : 5;
            const char ack[] = {0x20, 0x02, 0x00, rc};
            std::lock_guard<std::mutex> l(m_mx);
            net::write(s, net::buffer(ack, 4), ec);
            break;
        }
        case 0x80: { // SUBSCRIBE
            std::size_t p = 2;
            subscribed     = read_str(body, p);
            const char ack[] = {char(0x90), 0x03, body[0], body[1], 0x00};
            std::lock_guard<std::mutex> l(m_mx);
            net::write(s, net::buffer(ack, 5), ec);
            break;
        }
        case 0x30: {
            std::string topic, payload;
            if (mqtt::decode_publish(tf, body, topic, payload)) {
                std::lock_guard<std::mutex> l(m_mx);
                m_published.emplace_back(topic, payload);
                m_cv.notify_all();
            }
            break;
        }
        case 0xC0: {
            const char pong[] = {char(0xD0), 0x00};
            std::lock_guard<std::mutex> l(m_mx);
            net::write(s, net::buffer(pong, 2), ec);
            break;
        }
        default: break;
        }
    }

    std::string                  m_code;
    net::io_context              m_ioc;
    tcp::acceptor                m_acceptor;
    std::thread                  m_thread;
    std::atomic<bool>            m_stop{false};
    std::mutex                   m_mx;
    std::condition_variable      m_cv;
    std::shared_ptr<tcp::socket> m_sock;
    std::vector<std::pair<std::string, std::string>> m_published;
};

PrinterConnectionParams params_for(const FakeBambuBroker& b, const std::string& code)
{
    PrinterConnectionParams p;
    p.dev_id   = "01P00A123456789";
    p.host     = "127.0.0.1";
    p.port     = std::to_string(b.port());
    p.password = code;
    return p;
}

} // namespace

TEST_CASE("MQTT packets carry their remaining length and strings as the protocol defines", "[FilunaBambuLan]")
{
    CHECK(mqtt::encode_remaining_length(0) == std::string(1, '\0'));
    CHECK(mqtt::encode_remaining_length(127) == std::string(1, char(127)));
    CHECK(mqtt::encode_remaining_length(128) == std::string("\x80\x01", 2));
    CHECK(mqtt::encode_remaining_length(16384) == std::string("\x80\x80\x01", 3));

    const std::string pkt = mqtt::publish_packet("device/X/request", "{\"a\":1}");
    std::uint8_t      tf;
    std::string       body, topic, payload;
    std::size_t       used;
    REQUIRE(mqtt::decode_packet(pkt, tf, body, used));
    CHECK(used == pkt.size());
    REQUIRE(mqtt::decode_publish(tf, body, topic, payload));
    CHECK(topic == "device/X/request");
    CHECK(payload == "{\"a\":1}");
    CHECK_FALSE(mqtt::decode_packet(pkt.substr(0, pkt.size() - 1), tf, body, used));
}

TEST_CASE("A Bambu SSDP announcement becomes a discovered LAN printer", "[FilunaBambuLan]")
{
    const std::string d = "NOTIFY * HTTP/1.1\r\nHOST: 239.255.255.250:1990\r\nServer: UPnP/1.0\r\nLocation: 192.168.1.20\r\n"
                          "NT: urn:bambulab-com:device:3dprinter:1\r\nUSN: 01P00A123456789\r\nCache-Control: max-age=1800\r\n"
                          "DevModel.bambu.com: C11\r\nDevName.bambu.com: Minha P1P\r\nDevSignal.bambu.com: -45\r\n"
                          "DevConnect.bambu.com: lan\r\nDevBind.bambu.com: free\r\nDevseclink.bambu.com: secure\r\n\r\n";
    const auto j = nlohmann::json::parse(FilunaBambuLanAgent::ssdp_to_discovery_json(d, "192.168.1.99"));
    CHECK(j["dev_id"] == "01P00A123456789");
    CHECK(j["dev_ip"] == "192.168.1.20");
    CHECK(j["dev_type"] == "C11");
    CHECK(j["dev_name"] == "Minha P1P");
    CHECK(j["connect_type"] == "lan");
    CHECK(FilunaBambuLanAgent::ssdp_to_discovery_json("M-SEARCH * HTTP/1.1\r\nST: ssdp:all\r\n\r\n", "1.2.3.4").empty());
}

TEST_CASE("Starting a print asks the printer for the uploaded plate with the AMS mapping", "[FilunaBambuLan]")
{
    PrintParams p{};
    p.plate_index         = 2;
    p.project_name        = "Cubo";
    p.task_use_ams        = true;
    p.ams_mapping         = "[0,2,-1,3]";
    p.task_bed_leveling   = true;
    p.task_record_timelapse = false;
    const auto j = nlohmann::json::parse(FilunaBambuLanAgent::project_file_command(p, "cubo.gcode.3mf"))["print"];
    CHECK(j["command"] == "project_file");
    CHECK(j["param"] == "Metadata/plate_2.gcode");
    CHECK(j["url"] == "file:///sdcard/cubo.gcode.3mf");
    CHECK(j["subtask_name"] == "Cubo");
    CHECK(j["use_ams"] == true);
    CHECK(j["ams_mapping"] == nlohmann::json::array({0, 2, -1, 3}));
    CHECK(j["bed_levelling"] == true);
}

TEST_CASE("Connecting with the access code subscribes to the report and asks for a full status", "[FilunaBambuLan]")
{
    FakeBambuBroker     broker("12345678");
    FilunaBambuLanAgent agent("");
    agent.set_ports({broker.port(), 990, 2021, false});
    std::mutex              mx;
    std::condition_variable cv;
    int                     connect_state = -1;
    std::vector<std::string> messages;
    agent.set_on_local_connect_fn([&](int s, std::string, std::string) { std::lock_guard<std::mutex> l(mx); connect_state = s; cv.notify_all(); });
    agent.set_on_local_message_fn([&](std::string, std::string m) { std::lock_guard<std::mutex> l(mx); messages.push_back(m); cv.notify_all(); });

    REQUIRE(agent.connect_printer(params_for(broker, "12345678")) == BAMBU_NETWORK_SUCCESS);
    CHECK(connect_state == ConnectStatusOk);
    CHECK(broker.user == "bblp");
    CHECK(broker.subscribed == "device/01P00A123456789/report");
    REQUIRE(broker.wait_published(1));
    const auto first = broker.published().front();
    CHECK(first.first == "device/01P00A123456789/request");
    CHECK(nlohmann::json::parse(first.second)["pushing"]["command"] == "pushall");

    // relatório da impressora chega ao DeviceManager sem alteração
    const std::string status = R"({"print":{"command":"push_status","gcode_state":"RUNNING","mc_percent":42,"ams":{"ams":[{"id":"0","tray":[{"id":"0","tray_type":"PLA","tray_color":"FF0000FF"}]}]}}})";
    broker.report("01P00A123456789", status);
    {
        std::unique_lock<std::mutex> l(mx);
        REQUIRE(cv.wait_for(l, std::chrono::seconds(5), [&] { return !messages.empty(); }));
        CHECK(messages.front() == status);
    }

    // comandos de controle vão como o DeviceManager os escreve
    for (const char* cmd : {"pause", "resume", "stop"})
        REQUIRE(agent.send_message_to_printer("01P00A123456789", std::string(R"({"print":{"sequence_id":"1","command":")") + cmd + "\"}}", 0, 0) ==
                BAMBU_NETWORK_SUCCESS);
    REQUIRE(broker.wait_published(4));
    const auto pub = broker.published();
    CHECK(nlohmann::json::parse(pub[1].second)["print"]["command"] == "pause");
    CHECK(nlohmann::json::parse(pub[2].second)["print"]["command"] == "resume");
    CHECK(nlohmann::json::parse(pub[3].second)["print"]["command"] == "stop");
    agent.disconnect_printer();
}

TEST_CASE("A wrong access code is reported as a failed connection", "[FilunaBambuLan]")
{
    FakeBambuBroker     broker("12345678");
    FilunaBambuLanAgent agent("");
    agent.set_ports({broker.port(), 990, 2021, false});
    int state = -1;
    agent.set_on_local_connect_fn([&](int s, std::string, std::string) { state = s; });
    CHECK(agent.connect_printer(params_for(broker, "00000000")) == BAMBU_NETWORK_ERR_CONNECT_FAILED);
    CHECK(state == ConnectStatusFailed);
    CHECK(agent.send_message_to_printer("01P00A123456789", "{}", 0, 0) == BAMBU_NETWORK_ERR_CONNECT_FAILED);
}
