#include "FilunaMqttClient.hpp"

#include <deque>
#include <future>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/log/trivial.hpp>

namespace net   = boost::asio;
namespace beast = boost::beast;
namespace ssl = boost::asio::ssl;
using tcp     = boost::asio::ip::tcp;

namespace Slic3r { namespace Filuna {

namespace mqtt {

std::string encode_remaining_length(std::size_t n)
{
    std::string out;
    do {
        std::uint8_t b = n % 128;
        n /= 128;
        if (n > 0)
            b |= 0x80;
        out.push_back(char(b));
    } while (n > 0);
    return out;
}

std::string encode_string(const std::string& s)
{
    std::string out;
    out.push_back(char((s.size() >> 8) & 0xFF));
    out.push_back(char(s.size() & 0xFF));
    return out + s;
}

static std::string packet(std::uint8_t type_flags, const std::string& body)
{
    return std::string(1, char(type_flags)) + encode_remaining_length(body.size()) + body;
}

std::string connect_packet(const std::string& client_id, const std::string& user, const std::string& password, std::uint16_t keepalive_s)
{
    std::string vh = encode_string("MQTT");
    vh.push_back(char(4)); // 3.1.1
    std::uint8_t flags = 0x02; // clean session
    if (!user.empty())
        flags |= 0x80;
    if (!password.empty())
        flags |= 0x40;
    vh.push_back(char(flags));
    vh.push_back(char(keepalive_s >> 8));
    vh.push_back(char(keepalive_s & 0xFF));
    std::string pl = encode_string(client_id);
    if (!user.empty())
        pl += encode_string(user);
    if (!password.empty())
        pl += encode_string(password);
    return packet(0x10, vh + pl);
}

std::string subscribe_packet(std::uint16_t packet_id, const std::string& topic)
{
    std::string body;
    body.push_back(char(packet_id >> 8));
    body.push_back(char(packet_id & 0xFF));
    body += encode_string(topic);
    body.push_back(char(0)); // QoS 0
    return packet(0x82, body);
}

std::string publish_packet(const std::string& topic, const std::string& payload) { return packet(0x30, encode_string(topic) + payload); }

bool decode_packet(const std::string& buf, std::uint8_t& type_flags, std::string& body, std::size_t& consumed)
{
    if (buf.size() < 2)
        return false;
    std::size_t len = 0, mult = 1, i = 1;
    for (;; ++i) {
        if (i >= buf.size() || i > 4)
            return false;
        const std::uint8_t b = std::uint8_t(buf[i]);
        len += std::size_t(b & 0x7F) * mult;
        mult *= 128;
        if (!(b & 0x80))
            break;
    }
    const std::size_t start = i + 1;
    if (buf.size() < start + len)
        return false;
    type_flags = std::uint8_t(buf[0]);
    body       = buf.substr(start, len);
    consumed   = start + len;
    return true;
}

bool decode_publish(std::uint8_t type_flags, const std::string& body, std::string& topic, std::string& payload)
{
    if ((type_flags & 0xF0) != 0x30 || body.size() < 2)
        return false;
    const std::size_t tl = (std::size_t(std::uint8_t(body[0])) << 8) | std::uint8_t(body[1]);
    std::size_t       p  = 2 + tl;
    if (body.size() < p)
        return false;
    topic = body.substr(2, tl);
    if ((type_flags & 0x06) != 0) // QoS 1/2 traz identificador do pacote
        p += 2;
    payload = p <= body.size() ? body.substr(p) : std::string();
    return true;
}

} // namespace mqtt

struct MqttClient::Impl
{
    net::io_context                               ioc;
    ssl::context                                  ctx{ssl::context::tls_client};
    std::unique_ptr<beast::tcp_stream>            plain;
    std::unique_ptr<beast::ssl_stream<beast::tcp_stream>> secure;
    net::steady_timer                             ping{ioc};
    std::thread                                   thread;
    std::string                                   rx;
    std::deque<std::string>                       tx;
    char                                          buf[16384];
    MessageFn                                     on_message;
    ClosedFn                                      on_closed;
    std::chrono::seconds                          keepalive{30};
    std::atomic<bool>*                            connected = nullptr;
    bool                                          closing   = false;

    template<class F> void with_stream(F&& f)
    {
        if (secure)
            f(*secure);
        else
            f(*plain);
    }
    beast::tcp_stream& lowest() { return secure ? beast::get_lowest_layer(*secure) : *plain; }

    void fail(const std::string& why)
    {
        if (closing)
            return;
        closing = true;
        if (connected)
            connected->store(false);
        boost::system::error_code ec;
        ping.cancel();
        lowest().socket().close(ec);
        if (on_closed)
            on_closed(why);
    }

    void read_loop()
    {
        with_stream([this](auto& s) {
            s.async_read_some(net::buffer(buf), [this](boost::system::error_code ec, std::size_t n) {
                if (ec)
                    return fail(ec.message());
                rx.append(buf, n);
                std::uint8_t tf;
                std::string  body;
                std::size_t  used;
                while (mqtt::decode_packet(rx, tf, body, used)) {
                    rx.erase(0, used);
                    std::string topic, payload;
                    if (mqtt::decode_publish(tf, body, topic, payload) && on_message)
                        on_message(topic, payload);
                }
                read_loop();
            });
        });
    }

    void write_next()
    {
        if (tx.empty())
            return;
        with_stream([this](auto& s) {
            net::async_write(s, net::buffer(tx.front()), [this](boost::system::error_code ec, std::size_t) {
                if (ec)
                    return fail(ec.message());
                tx.pop_front();
                write_next();
            });
        });
    }

    void send(std::string pkt)
    {
        const bool idle = tx.empty();
        tx.push_back(std::move(pkt));
        if (idle)
            write_next();
    }

    void schedule_ping()
    {
        ping.expires_after(keepalive);
        ping.async_wait([this](boost::system::error_code ec) {
            if (ec || closing)
                return;
            send(std::string("\xC0\x00", 2));
            schedule_ping();
        });
    }
};

MqttClient::MqttClient() : m(std::make_unique<Impl>()) {}
MqttClient::~MqttClient() { close(); }

bool MqttClient::connect(const Options& opt, const std::vector<std::string>& topics, MessageFn on_message, ClosedFn on_closed, std::string& error)
{
    close();
    m = std::make_unique<Impl>();
    m->on_message = std::move(on_message);
    m->on_closed  = std::move(on_closed);
    m->keepalive  = opt.keepalive;
    m->connected  = &m_connected;
    try {
        tcp::resolver resolver(m->ioc);
        auto          results = resolver.resolve(opt.host, std::to_string(opt.port));
        if (opt.tls) {
            m->ctx.set_verify_mode(opt.verify_peer ? ssl::verify_peer : ssl::verify_none);
            m->secure = std::make_unique<beast::ssl_stream<beast::tcp_stream>>(m->ioc, m->ctx);
            beast::get_lowest_layer(*m->secure).expires_after(opt.connect_timeout);
            beast::get_lowest_layer(*m->secure).connect(results);
            SSL_set_tlsext_host_name(m->secure->native_handle(), opt.host.c_str());
            m->secure->handshake(ssl::stream_base::client);
        } else {
            m->plain = std::make_unique<beast::tcp_stream>(m->ioc);
            m->plain->expires_after(opt.connect_timeout);
            m->plain->connect(results);
        }
        const std::string con = mqtt::connect_packet(opt.client_id, opt.username, opt.password, std::uint16_t(opt.keepalive.count() * 2));
        m->with_stream([&](auto& s) { net::write(s, net::buffer(con)); });
        // CONNACK: 0x20 0x02 flags rc
        std::string acc;
        std::uint8_t tf = 0;
        std::string  body;
        std::size_t  used = 0;
        while (!mqtt::decode_packet(acc, tf, body, used)) {
            char        b[256];
            std::size_t n = 0;
            m->with_stream([&](auto& s) { n = s.read_some(net::buffer(b)); });
            acc.append(b, n);
        }
        if ((tf & 0xF0) != 0x20 || body.size() < 2 || body[1] != 0) {
            const int rc = body.size() >= 2 ? int(std::uint8_t(body[1])) : -1;
            error        = rc == 4 || rc == 5 ? "código de acesso recusado pela impressora" : "MQTT recusou a conexão (código " + std::to_string(rc) + ")";
            return false;
        }
        m->rx = acc.substr(used);
        m->lowest().expires_never();
        std::uint16_t id = 1;
        for (const auto& t : topics) {
            const std::string sub = mqtt::subscribe_packet(id++, t);
            m->with_stream([&](auto& s) { net::write(s, net::buffer(sub)); });
        }
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
    m_connected = true;
    m->read_loop();
    m->schedule_ping();
    m->thread = std::thread([impl = m.get()] {
        try {
            impl->ioc.run();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "FilunaMqttClient: " << e.what();
        }
    });
    return true;
}

bool MqttClient::publish(const std::string& topic, const std::string& payload)
{
    if (!m_connected || !m)
        return false;
    net::post(m->ioc, [impl = m.get(), pkt = mqtt::publish_packet(topic, payload)]() mutable { impl->send(std::move(pkt)); });
    return true;
}

void MqttClient::close()
{
    if (!m)
        return;
    if (m->thread.joinable()) {
        net::post(m->ioc, [impl = m.get()] {
            if (!impl->closing) {
                impl->send(std::string("\xE0\x00", 2)); // DISCONNECT
                impl->closing = true;
                boost::system::error_code ec;
                impl->ping.cancel();
                impl->lowest().socket().shutdown(tcp::socket::shutdown_both, ec);
                impl->lowest().socket().close(ec);
            }
            impl->ioc.stop();
        });
        m->thread.join();
    }
    m_connected = false;
    m.reset();
}

}} // namespace Slic3r::Filuna
