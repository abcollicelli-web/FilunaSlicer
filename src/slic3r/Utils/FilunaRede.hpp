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

}} // namespace Slic3r::Filuna
