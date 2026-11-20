#pragma once
// Envio de arquivo por FTPS implícito (porta 990) para impressoras Bambu Lab no modo LAN. Cada etapa tem prazo: se a
// impressora para de responder, o envio termina dizendo a etapa, em vez de ficar em 0% para sempre. No fim, o
// close_notify do TLS sai sem esperar o da impressora: a A1 não responde e só grava o arquivo quando a conexão fecha.
// Só depende de Boost.Asio e OpenSSL, para ser ensaiado contra lab/oem_homolog/emuladores/bambu_lan.py.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/write.hpp>

namespace Slic3r { namespace FilunaFtps {

struct Envio
{
    std::string    ip;
    unsigned short porta = 990;
    bool           tls   = true;
    std::string    usuario = "bblp";
    std::string    senha;
    std::string    arquivo_local;
    std::string    nome_remoto;
    std::chrono::seconds limite_resposta{20}; // cada resposta do canal de controle
    std::chrono::seconds limite_dados{30};    // cada bloco de 256 KiB enviado
    std::chrono::seconds limite_fim{90};      // a impressora confirmar o arquivo gravado no cartão
    std::function<void(int)>                progresso; // 0 a 100
    std::function<bool()>                   cancelado;
    std::function<void(const std::string&)> registro;  // cada etapa, para o registro do programa
};

// Devolve vazio se o arquivo chegou inteiro; senão, a etapa em que parou e o motivo.
inline std::string enviar(const Envio& e)
{
    namespace net = boost::asio;
    namespace ssl = boost::asio::ssl;
    using tcp     = net::ip::tcp;
    using Erro    = boost::system::error_code;
    using Stream  = ssl::stream<tcp::socket>;

    std::ifstream in(e.arquivo_local, std::ios::binary);
    if (!in)
        return "não foi possível abrir " + e.arquivo_local;
    in.seekg(0, std::ios::end);
    const std::uint64_t total = std::uint64_t(in.tellg());
    in.seekg(0);

    net::io_context ioc;
    ssl::context    ctx(ssl::context::tls_client);
    ctx.set_verify_mode(ssl::verify_none); // certificado próprio da impressora
    Stream      ctl(ioc, ctx), dados(ioc, ctx);
    std::string etapa;
    auto        anotar = [&](const std::string& s) {
        etapa = s;
        if (e.registro)
            e.registro(s);
    };
    auto fechar = [&] {
        Erro ig;
        ctl.next_layer().close(ig);
        dados.next_layer().close(ig);
        ioc.restart();
        ioc.run(); // as operações pendentes terminam aqui: os manipuladores apontam para esta pilha
    };
    // Roda a operação iniciada até terminar, o prazo acabar ou o cliente cancelar.
    auto rodar = [&](std::chrono::seconds limite, auto iniciar) -> std::size_t {
        bool        feito = false;
        Erro        ec;
        std::size_t n = 0;
        iniciar([&](const Erro& erro, std::size_t k) { feito = true; ec = erro; n = k; });
        const auto fim = std::chrono::steady_clock::now() + limite;
        while (!feito && std::chrono::steady_clock::now() < fim) {
            ioc.restart();
            ioc.run_for(std::chrono::milliseconds(250));
            if (!feito && e.cancelado && e.cancelado()) {
                fechar();
                throw std::runtime_error("envio cancelado");
            }
        }
        if (!feito) {
            fechar();
            throw std::runtime_error(etapa + ": a impressora não respondeu em " + std::to_string(limite.count()) + " s");
        }
        if (ec)
            throw std::runtime_error(etapa + ": " + ec.message());
        return n;
    };
    auto sem_n = [](auto h) { return [h](const Erro& erro) { h(erro, 0); }; };
    auto escrever = [&](Stream& s, const char* b, std::size_t tam, std::chrono::seconds limite) {
        rodar(limite, [&](auto h) {
            if (e.tls)
                net::async_write(s, net::buffer(b, tam), h);
            else
                net::async_write(s.next_layer(), net::buffer(b, tam), h);
        });
    };
    auto conectar = [&](tcp::socket& s, const tcp::endpoint& alvo) {
        rodar(e.limite_resposta, [&](auto h) { s.async_connect(alvo, sem_n(h)); });
    };
    auto aperto_de_mao = [&](Stream& s) {
        rodar(e.limite_resposta, [&](auto h) { s.async_handshake(ssl::stream_base::client, sem_n(h)); });
    };

    std::string pend;
    auto        ler_linha = [&](std::chrono::seconds limite) -> std::string {
        for (;;) {
            const auto nl = pend.find("\r\n");
            if (nl != std::string::npos) {
                std::string l = pend.substr(0, nl);
                pend.erase(0, nl + 2);
                return l;
            }
            char b[512];
            pend.append(b, rodar(limite, [&](auto h) {
                if (e.tls)
                    ctl.async_read_some(net::buffer(b), h);
                else
                    ctl.next_layer().async_read_some(net::buffer(b), h);
            }));
        }
    };
    // resposta completa (multilinha "123-" ... "123 ")
    auto resposta = [&](std::chrono::seconds limite) -> std::string {
        std::string l = ler_linha(limite);
        if (l.size() >= 4 && l[3] == '-') {
            const std::string fim = l.substr(0, 3) + " ";
            std::string       m;
            while ((m = ler_linha(limite)).compare(0, 4, fim) != 0) {}
            return m;
        }
        return l;
    };
    auto comando = [&](const std::string& c) -> std::string {
        const std::string linha = c + "\r\n";
        escrever(ctl, linha.data(), linha.size(), e.limite_resposta);
        return resposta(e.limite_resposta);
    };
    auto esperar_codigo = [&](const std::string& r, char primeiro) {
        if (r.empty() || r[0] != primeiro)
            throw std::runtime_error(etapa + ": " + r);
    };

    try {
        anotar("conexão com " + e.ip + ":" + std::to_string(e.porta));
        conectar(ctl.next_layer(), tcp::endpoint(net::ip::make_address(e.ip), e.porta));
        if (e.tls) {
            anotar("TLS do controle");
            aperto_de_mao(ctl);
        }
        anotar("boas-vindas");
        esperar_codigo(resposta(e.limite_resposta), '2');
        anotar("login");
        const std::string u = comando("USER " + e.usuario);
        esperar_codigo(!u.empty() && u[0] == '3' ? comando("PASS " + e.senha) : u, '2');
        if (e.tls) {
            anotar("PBSZ e PROT");
            esperar_codigo(comando("PBSZ 0"), '2');
            esperar_codigo(comando("PROT P"), '2');
        }
        anotar("TYPE e PASV");
        esperar_codigo(comando("TYPE I"), '2');
        const std::string pasv = comando("PASV");
        esperar_codigo(pasv, '2');
        const auto l = pasv.find('('), r = pasv.find(')');
        int        h[6]{};
        if (l == std::string::npos || r == std::string::npos ||
            std::sscanf(pasv.substr(l + 1, r - l - 1).c_str(), "%d,%d,%d,%d,%d,%d", &h[0], &h[1], &h[2], &h[3], &h[4], &h[5]) != 6)
            throw std::runtime_error("resposta PASV inválida: " + pasv);
        const auto porta_dados = static_cast<unsigned short>(h[4] * 256 + h[5]);
        anotar("conexão de dados na porta " + std::to_string(porta_dados));
        // IP do controle (a resposta PASV pode trazer endereço interno)
        conectar(dados.next_layer(), tcp::endpoint(ctl.next_layer().remote_endpoint().address(), porta_dados));
        anotar("STOR " + e.nome_remoto);
        const std::string stor = "STOR " + e.nome_remoto + "\r\n";
        escrever(ctl, stor.data(), stor.size(), e.limite_resposta);
        if (e.tls) { // a impressora exige reaproveitar no canal de dados a sessão TLS do controle
            if (SSL_SESSION* sess = SSL_get1_session(ctl.native_handle())) {
                SSL_set_session(dados.native_handle(), sess);
                SSL_SESSION_free(sess);
            }
            anotar("TLS dos dados");
            aperto_de_mao(dados);
        }
        anotar("aceite do STOR");
        esperar_codigo(resposta(e.limite_resposta), '1');

        anotar("envio de " + std::to_string(total) + " bytes");
        std::vector<char> buf(256 * 1024);
        std::uint64_t     enviados = 0;
        int               ultimo   = 0;
        if (e.progresso)
            e.progresso(0);
        while (in) {
            in.read(buf.data(), std::streamsize(buf.size()));
            const std::size_t n = std::size_t(in.gcount());
            if (n == 0)
                break;
            escrever(dados, buf.data(), n, e.limite_dados);
            enviados += n;
            const int pct = total ? int(enviados * 100 / total) : 100;
            if (e.progresso && pct != ultimo)
                e.progresso(ultimo = pct);
        }
        if (enviados != total)
            throw std::runtime_error("enviados " + std::to_string(enviados) + " de " + std::to_string(total) + " bytes");

        anotar("fechamento dos dados");
        if (e.tls) { // close_notify sai na hora; a resposta da impressora, se vier, não é esperada
            dados.async_shutdown([](const Erro&) {});
            ioc.restart();
            ioc.run_for(std::chrono::milliseconds(500));
        }
        Erro ig;
        dados.next_layer().shutdown(tcp::socket::shutdown_both, ig);
        dados.next_layer().close(ig);
        ioc.restart();
        ioc.run();
        // a confirmação pode demorar: a impressora grava o arquivo no cartão antes de responder
        anotar("confirmação do arquivo gravado");
        esperar_codigo(resposta(e.limite_fim), '2');
        anotar("fim");
        try {
            comando("QUIT");
        } catch (const std::exception&) {} // o arquivo já está gravado; a despedida não importa
        fechar();
    } catch (const std::exception& ex) {
        fechar();
        return ex.what();
    }
    return {};
}

}} // namespace Slic3r::FilunaFtps
