#pragma once

// Filuna — redes locais do computador e envio direto a cada endereço delas.
//
// A busca das marcas que respondem a um pedido UDP (Elegoo SDCP e CC2) mandava só para a rede inteira (broadcast).
// No Windows, o firewall barra a resposta a um broadcast quando o programa não foi liberado no aviso de rede: a
// impressora responde e a resposta não entra. A resposta a um pedido mandado DIRETO para o IP da impressora o
// firewall deixa passar (é a resposta de uma conversa que o próprio programa abriu). Por isso, além do broadcast,
// o pedido vai também para cada endereço da rede.

#include <algorithm>
#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <memory>
#include <openssl/x509.h>

namespace Slic3r { namespace Filuna {

// Prefixos "a.b.c." das redes privadas deste computador (no máximo 4 redes /24). A rede por onde o computador sai
// (rota padrão) vem primeiro: placas virtuais (WSL, Hyper-V, VirtualBox) não tomam o lugar da rede de verdade.
inline std::vector<std::string> redes_locais()
{
    namespace net = boost::asio;
    std::vector<std::string> prefixos;
    auto incluir = [&prefixos](const net::ip::address& a) {
        if (!a.is_v4())
            return;
        const auto        b       = a.to_v4().to_bytes();
        const bool        privada = b[0] == 10 || (b[0] == 172 && b[1] >= 16 && b[1] <= 31) || (b[0] == 192 && b[1] == 168);
        const std::string p       = std::to_string(b[0]) + "." + std::to_string(b[1]) + "." + std::to_string(b[2]) + ".";
        if (privada && std::find(prefixos.begin(), prefixos.end(), p) == prefixos.end())
            prefixos.push_back(p);
    };
    boost::system::error_code ec;
    net::io_context           io;
    {
        // "connect" em UDP não envia nada: só escolhe a interface de saída padrão
        net::ip::udp::socket s(io);
        s.open(net::ip::udp::v4(), ec);
        if (!ec)
            s.connect({net::ip::make_address_v4("192.0.2.1"), 9}, ec);
        if (!ec)
            incluir(s.local_endpoint(ec).address());
    }
    net::ip::tcp::resolver r(io);
    const std::string      host  = net::ip::host_name(ec);
    const auto             lista = r.resolve(host, "", ec);
    if (!ec)
        for (const auto& e : lista)
            incluir(e.endpoint().address());
    if (prefixos.size() > 4)
        prefixos.resize(4);
    return prefixos;
}

// Manda 'msg' para cada endereço (1 a 254) das redes locais, na porta dada. Socket UDP já aberto (não bloqueante).
inline void enviar_para_rede(boost::asio::ip::udp::socket& s, unsigned short porta, const std::string& msg,
                             const std::vector<std::string>& redes)
{
    namespace net = boost::asio;
    for (const auto& prefixo : redes)
        for (int i = 1; i < 255; ++i) {
            boost::system::error_code ec;
            const auto                a = net::ip::make_address_v4(prefixo + std::to_string(i), ec);
            if (!ec)
                s.send_to(net::buffer(msg), net::ip::udp::endpoint(a, porta), 0, ec);
        }
}

// IPs que aceitam conexão TCP na porta dentro do tempo dado (todos ao mesmo tempo).
inline std::vector<std::string> portas_abertas(const std::vector<std::string>& ips, unsigned short porta, std::chrono::milliseconds espera)
{
    namespace net = boost::asio;
    net::io_context                                    io;
    std::vector<std::string>                           abertos;
    std::vector<std::unique_ptr<net::ip::tcp::socket>> socks;
    socks.reserve(ips.size());
    for (const auto& ip : ips) {
        boost::system::error_code ec;
        const auto                addr = net::ip::make_address(ip, ec);
        if (ec)
            continue;
        socks.push_back(std::make_unique<net::ip::tcp::socket>(io));
        socks.back()->async_connect({addr, porta}, [ip, &abertos](const boost::system::error_code& e) {
            if (!e)
                abertos.push_back(ip);
        });
    }
    net::steady_timer t(io, espera);
    t.async_wait([&socks](const boost::system::error_code&) {
        for (auto& s : socks) {
            boost::system::error_code ig;
            s->close(ig);
        }
    });
    io.run();
    return abertos;
}

// Todos os endereços (1 a 254) das redes locais do computador.
inline std::vector<std::string> enderecos_locais()
{
    std::vector<std::string> ips;
    for (const auto& prefixo : redes_locais())
        for (int i = 1; i < 255; ++i)
            ips.push_back(prefixo + std::to_string(i));
    return ips;
}

// Nome comum (CN) do certificado TLS que o aparelho apresenta na porta. Na Bambu (porta 8883) é o número de série.
// Confere a porta antes (até 1,5 s), para um IP errado não prender quem chamou. Vazio se não deu.
inline std::string cn_do_certificado(const std::string& ip, unsigned short porta)
{
    namespace net = boost::asio;
    namespace ssl = net::ssl;
    if (portas_abertas({ip}, porta, std::chrono::milliseconds(1500)).empty())
        return std::string();
    boost::system::error_code             ec;
    net::io_context                       io;
    ssl::context                          ctx(ssl::context::tls_client);
    ctx.set_verify_mode(ssl::verify_none); // só lemos o certificado; a conexão de verdade é feita pelo agente
    ssl::stream<net::ip::tcp::socket>     s(io, ctx);
    s.next_layer().connect({net::ip::make_address(ip, ec), porta}, ec);
    if (!ec)
        s.handshake(ssl::stream_base::client, ec);
    if (ec)
        return std::string();
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    X509* cert = SSL_get1_peer_certificate(s.native_handle());
#else
    X509* cert = SSL_get_peer_certificate(s.native_handle());
#endif
    std::string cn;
    if (cert != nullptr) {
        char buf[256] = {0};
        if (X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, buf, sizeof(buf)) > 0)
            cn = buf;
        X509_free(cert);
    }
    s.next_layer().close(ec);
    return cn;
}

}} // namespace Slic3r::Filuna
