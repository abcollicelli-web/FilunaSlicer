#pragma once

// Filuna — repasse local de câmera. Algumas marcas entregam o vídeo ao vivo como HTTP-FLV (Anycubic:
// http://<ip>:18088/flv), sem cabeçalho de permissão para páginas. A tela de Dispositivos (WebView) não pode
// ler esse endereço direto. Este repasse escuta só em 127.0.0.1, busca o vídeo na impressora e entrega os
// mesmos bytes com "Access-Control-Allow-Origin: *", para o tocador da tela (mpegts.js).
//
// Só repassa para endereços de rede local (10.x, 172.16-31.x, 192.168.x, 169.254.x) — não é um proxy aberto.
// Vale para qualquer marca: o agente só precisa informar o endereço da câmera (Filuna::set_camera_url).

#include <atomic>
#include <cstdint>
#include <cstring>
#include <map>
#include <cctype>
#include <istream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

namespace Slic3r { namespace Filuna {

class CameraRelay
{
public:
    // Porta do repasse (0 se não conseguiu abrir). Sobe na primeira chamada.
    static int porta()
    {
        static CameraRelay r;
        return r.m_porta;
    }

    // "http://192.168.1.99:18088/flv" -> host, porta, caminho; false se não for http de rede local
    static bool destino_valido(const std::string& url, std::string& host, std::string& port, std::string& caminho)
    {
        const std::string p = "http://";
        if (url.compare(0, p.size(), p) != 0)
            return false;
        const std::size_t barra = url.find('/', p.size());
        std::string       hp    = url.substr(p.size(), barra == std::string::npos ? std::string::npos : barra - p.size());
        caminho                 = barra == std::string::npos ? "/" : url.substr(barra);
        const std::size_t dp    = hp.find(':');
        host                    = hp.substr(0, dp);
        port                    = dp == std::string::npos ? "80" : hp.substr(dp + 1);
        boost::system::error_code ec;
        const auto a = boost::asio::ip::make_address_v4(host, ec);
        if (ec)
            return false;
        const auto b = a.to_bytes();
        return b[0] == 10 || (b[0] == 172 && b[1] >= 16 && b[1] <= 31) || (b[0] == 192 && b[1] == 168) || (b[0] == 169 && b[1] == 254);
    }

    // Câmera da Bambu em modo LAN (A1, A1 mini, P1, A2): JPEG por TLS na porta 6000, com o código de acesso. O
    // agente registra IP e código aqui; a tela só recebe "/bambu?d=<serie>" (o código não vai para a página).
    static void set_bambu(const std::string& dev_id, const std::string& ip, const std::string& codigo)
    {
        std::lock_guard<std::mutex> l(bambu_mutex());
        bambu_map()[dev_id] = {ip, codigo};
    }

    static std::string url_decodificar(const std::string& s)
    {
        std::string o;
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '%' && i + 2 < s.size()) {
                o += char(std::stoi(s.substr(i + 1, 2), nullptr, 16));
                i += 2;
            } else
                o += s[i] == '+' ? ' ' : s[i];
        }
        return o;
    }

private:
    CameraRelay()
    {
        namespace asio = boost::asio;
        using tcp      = asio::ip::tcp;
        try {
            m_acc = std::make_unique<tcp::acceptor>(m_ioc, tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), 0));
            m_porta = m_acc->local_endpoint().port();
        } catch (const std::exception&) {
            m_porta = 0;
            return;
        }
        std::thread([this] {
            for (;;) {
                auto s = std::make_shared<tcp::socket>(m_ioc);
                boost::system::error_code ec;
                m_acc->accept(*s, ec);
                if (ec)
                    continue;
                std::thread([s] { atender(*s); }).detach();
            }
        }).detach();
    }

    static void atender(boost::asio::ip::tcp::socket& cli)
    {
        namespace asio = boost::asio;
        using tcp      = asio::ip::tcp;
        boost::system::error_code ec;
        try {
            asio::streambuf pedido;
            asio::read_until(cli, pedido, "\r\n\r\n");
            std::istream is(&pedido);
            std::string  metodo, alvo;
            is >> metodo >> alvo;
            auto responder = [&](const std::string& estado) {
                const std::string r = "HTTP/1.1 " + estado + "\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                asio::write(cli, asio::buffer(r), ec);
            };
            const std::string chave_bambu = "/bambu?d=";
            if (metodo == "GET" && alvo.compare(0, chave_bambu.size(), chave_bambu) == 0) {
                camera_bambu(cli, url_decodificar(alvo.substr(chave_bambu.size())), responder);
                cli.close(ec);
                return;
            }
            const std::string chave = "/cam?u=";
            std::string       host, port, caminho;
            if (metodo != "GET" || alvo.compare(0, chave.size(), chave) != 0 ||
                !destino_valido(url_decodificar(alvo.substr(chave.size())), host, port, caminho)) {
                responder("400 Bad Request");
                return;
            }
            asio::io_context ioc;
            tcp::resolver    res(ioc);
            tcp::socket      imp(ioc);
            asio::connect(imp, res.resolve(host, port), ec);
            if (ec) {
                responder("502 Bad Gateway");
                return;
            }
            const std::string get = "GET " + caminho + " HTTP/1.1\r\nHost: " + host + ":" + port + "\r\nAccept: */*\r\nConnection: close\r\n\r\n";
            asio::write(imp, asio::buffer(get));
            // cabeçalho da impressora: só interessa saber se o corpo vem em pedaços (chunked); o que sobrar
            // depois dele já é vídeo
            asio::streambuf resposta;
            asio::read_until(imp, resposta, "\r\n\r\n");
            bool        pedacos = false;
            std::string sobra, tipo = "video/x-flv";
            {
                std::istream rs(&resposta);
                std::string  linha;
                while (std::getline(rs, linha) && linha != "\r") {
                    std::string l = linha;
                    for (char& c : l)
                        c = char(std::tolower(static_cast<unsigned char>(c)));
                    if (l.find("transfer-encoding:") == 0 && l.find("chunked") != std::string::npos)
                        pedacos = true;
                    if (l.find("content-type:") == 0) { // MJPEG (multipart) ou FLV: repassa como veio
                        tipo = linha.substr(13);
                        while (!tipo.empty() && (tipo.back() == '\r' || tipo.back() == ' '))
                            tipo.pop_back();
                        while (!tipo.empty() && tipo.front() == ' ')
                            tipo.erase(0, 1);
                    }
                }
                sobra.assign(std::istreambuf_iterator<char>(rs), std::istreambuf_iterator<char>());
            }
            const std::string cab = "HTTP/1.1 200 OK\r\nContent-Type: " + tipo + "\r\nAccess-Control-Allow-Origin: *\r\n"
                                    "Cache-Control: no-store\r\nConnection: close\r\n\r\n";
            asio::write(cli, asio::buffer(cab));
            char buf[16384];
            auto ler_mais = [&]() { // acrescenta bytes da impressora em `sobra`; false = acabou
                const std::size_t n = imp.read_some(asio::buffer(buf), ec);
                if (ec || n == 0)
                    return false;
                sobra.append(buf, n);
                return true;
            };
            if (!pedacos) {
                for (;;) {
                    if (!sobra.empty()) {
                        asio::write(cli, asio::buffer(sobra), ec);
                        if (ec)
                            break; // a tela fechou o vídeo
                        sobra.clear();
                    }
                    if (!ler_mais())
                        break;
                }
            } else {
                // "<tamanho em hex>\r\n<dados>\r\n" repetido; entrega só os dados
                for (;;) {
                    std::size_t fim;
                    while ((fim = sobra.find("\r\n")) == std::string::npos)
                        if (!ler_mais())
                            goto fim_do_video;
                    {
                        const std::size_t tam = std::stoul(sobra.substr(0, fim), nullptr, 16);
                        if (tam == 0)
                            break;
                        sobra.erase(0, fim + 2);
                        while (sobra.size() < tam + 2)
                            if (!ler_mais())
                                goto fim_do_video;
                        asio::write(cli, asio::buffer(sobra.data(), tam), ec);
                        if (ec)
                            break;
                        sobra.erase(0, tam + 2);
                    }
                }
            fim_do_video:;
            }
        } catch (const std::exception&) {}
        cli.close(ec);
    }

    struct AcessoBambu
    {
        std::string ip, codigo;
    };
    static std::mutex& bambu_mutex()
    {
        static std::mutex m;
        return m;
    }
    static std::map<std::string, AcessoBambu>& bambu_map()
    {
        static std::map<std::string, AcessoBambu> m;
        return m;
    }

    // Lê os quadros da câmera da Bambu (cabeçalho de 16 bytes com o tamanho em little-endian + JPEG) e entrega
    // como MJPEG (multipart/x-mixed-replace), que o <img> da tela mostra direto.
    template<class Responder> static void camera_bambu(boost::asio::ip::tcp::socket& cli, const std::string& dev_id, Responder& responder)
    {
        namespace asio = boost::asio;
        namespace ssl  = asio::ssl;
        using tcp      = asio::ip::tcp;
        AcessoBambu a;
        {
            std::lock_guard<std::mutex> l(bambu_mutex());
            auto it = bambu_map().find(dev_id);
            if (it != bambu_map().end())
                a = it->second;
        }
        std::string h, pt, cm;
        if (a.ip.empty() || a.codigo.empty() || !destino_valido("http://" + a.ip + "/", h, pt, cm)) {
            responder("404 Not Found");
            return;
        }
        boost::system::error_code ec;
        asio::io_context          ioc;
        ssl::context              ctx(ssl::context::tls_client);
        ctx.set_verify_mode(ssl::verify_none); // certificado próprio da impressora
        ssl::stream<tcp::socket> imp(ioc, ctx);
        tcp::resolver            res(ioc);
        asio::connect(imp.next_layer(), res.resolve(a.ip, "6000", ec), ec);
        if (!ec)
            imp.handshake(ssl::stream_base::client, ec);
        if (ec) {
            responder("502 Bad Gateway");
            return;
        }
        unsigned char auth[80] = {0};
        auto le32 = [&auth](int pos, std::uint32_t v) {
            for (int i = 0; i < 4; ++i)
                auth[pos + i] = static_cast<unsigned char>((v >> (8 * i)) & 0xff);
        };
        le32(0, 0x40);
        le32(4, 0x3000);
        std::memcpy(auth + 16, "bblp", 4);
        std::memcpy(auth + 48, a.codigo.data(), std::min<std::size_t>(32, a.codigo.size()));
        asio::write(imp, asio::buffer(auth), ec);
        if (ec) {
            responder("502 Bad Gateway");
            return;
        }
        const std::string cab = "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=filunaquadro\r\n"
                                "Access-Control-Allow-Origin: *\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
        asio::write(cli, asio::buffer(cab), ec);
        std::string jpg;
        while (!ec) {
            unsigned char q[16];
            asio::read(imp, asio::buffer(q), ec);
            if (ec)
                break;
            const std::uint32_t n = std::uint32_t(q[0]) | (std::uint32_t(q[1]) << 8) | (std::uint32_t(q[2]) << 16) | (std::uint32_t(q[3]) << 24);
            if (n == 0 || n > 8u * 1024u * 1024u)
                break;
            jpg.resize(n);
            asio::read(imp, asio::buffer(&jpg[0], n), ec);
            if (ec)
                break;
            const std::string parte = "--filunaquadro\r\nContent-Type: image/jpeg\r\nContent-Length: " + std::to_string(n) + "\r\n\r\n";
            asio::write(cli, asio::buffer(parte), ec);
            if (!ec)
                asio::write(cli, asio::buffer(jpg), ec);
            if (!ec)
                asio::write(cli, asio::buffer(std::string("\r\n")), ec);
        }
        imp.next_layer().close(ec);
    }

    boost::asio::io_context                         m_ioc;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acc;
    int                                             m_porta{0};
};

}} // namespace Slic3r::Filuna
