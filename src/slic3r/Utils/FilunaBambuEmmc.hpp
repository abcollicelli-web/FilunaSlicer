#pragma once
// Envio para a memória interna (eMMC) das Bambu Lab que imprimem sem cartão (P2S, H2D...) no modo LAN. O FTPS da porta
// 990 só enxerga o cartão: sem ele, a impressora não abre a transferência. Aqui o arquivo vai pelo túnel TLS da porta
// 6000 (login bblp + código de acesso, quadros de 16 bytes, JSON + "\n\n" + binário), para o armazenamento "emmc";
// a impressão começa com o endereço brtc://emmc/<nome>. Protocolo como levantado na P2S pelo projeto
// open-bambu-networking. Cada etapa tem prazo, como no FilunaFtps.hpp.

#include <array>
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
#include <boost/asio/read.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/write.hpp>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace Slic3r { namespace FilunaBambuEmmc {

struct Envio
{
    std::string    ip;
    unsigned short porta = 6000;
    std::string    codigo; // código de acesso da tela da impressora
    std::string    arquivo_local;
    std::string    nome_remoto; // termina em .gcode.3mf
    std::chrono::seconds limite_resposta{20};
    std::chrono::seconds limite_dados{60};
    std::chrono::seconds limite_fim{120};
    std::function<void(int)>                progresso; // 0 a 100
    std::function<bool()>                   cancelado;
    std::function<void(const std::string&)> registro;
};

// Endereço para o comando project_file depois do envio.
inline std::string endereco(const std::string& nome_remoto) { return "brtc://emmc/" + nome_remoto; }

inline std::string md5_do_arquivo(const std::string& caminho)
{
    std::ifstream in(caminho, std::ios::binary);
    EVP_MD_CTX*   c = EVP_MD_CTX_new();
    EVP_DigestInit_ex(c, EVP_md5(), nullptr);
    std::vector<char> b(1 << 16);
    while (in) {
        in.read(b.data(), std::streamsize(b.size()));
        if (in.gcount() > 0)
            EVP_DigestUpdate(c, b.data(), std::size_t(in.gcount()));
    }
    unsigned char d[EVP_MAX_MD_SIZE];
    unsigned int  n = 0;
    EVP_DigestFinal_ex(c, d, &n);
    EVP_MD_CTX_free(c);
    std::string hex;
    char        h[3];
    for (unsigned int i = 0; i < n; ++i) {
        std::snprintf(h, sizeof(h), "%02x", d[i]);
        hex += h;
    }
    return hex;
}

// Devolve vazio se a impressora confirmou o arquivo gravado; senão, a etapa em que parou e o motivo.
inline std::string enviar(const Envio& e)
{
    namespace net = boost::asio;
    namespace ssl = boost::asio::ssl;
    using tcp     = net::ip::tcp;
    using Erro    = boost::system::error_code;
    using json    = nlohmann::ordered_json; // "mtype" precisa vir primeiro
    constexpr std::uint32_t LOGIN = 0x0101013Fu, LOGIN_OK = 0x0001013Fu, CTRL = 0x0102013Fu;
    constexpr int           CTRL_SETUP = 12291, CTRL_JSON = 12289, ENVIO = 5;

    std::ifstream in(e.arquivo_local, std::ios::binary);
    if (!in)
        return "não foi possível abrir " + e.arquivo_local;
    in.seekg(0, std::ios::end);
    const std::uint64_t total = std::uint64_t(in.tellg());
    in.seekg(0);
    const std::string md5 = md5_do_arquivo(e.arquivo_local);

    net::io_context                ioc;
    ssl::context                   ctx(ssl::context::tls_client);
    ctx.set_verify_mode(ssl::verify_none); // certificado próprio da impressora
    ssl::stream<tcp::socket>       s(ioc, ctx);
    std::string                    etapa;
    std::uint32_t                  seq = 1;
    auto anotar = [&](const std::string& t) {
        etapa = t;
        if (e.registro)
            e.registro(t);
    };
    auto fechar = [&] {
        Erro ig;
        s.next_layer().close(ig);
        ioc.restart();
        ioc.run();
    };
    auto rodar = [&](std::chrono::seconds limite, auto iniciar) -> std::size_t {
        bool        feito = false;
        Erro        ec;
        std::size_t n = 0;
        iniciar([&](const Erro& erro, std::size_t k) { feito = true; ec = erro; n = k; });
        const auto fim = std::chrono::steady_clock::now() + limite;
        while (!feito && std::chrono::steady_clock::now() < fim) {
            ioc.restart();
            ioc.run_for(std::chrono::milliseconds(250));
            if (!feito && e.cancelado && e.cancelado())
                throw std::runtime_error("envio cancelado");
        }
        if (!feito)
            throw std::runtime_error(etapa + ": a impressora não respondeu em " + std::to_string(limite.count()) + " s");
        if (ec)
            throw std::runtime_error(etapa + ": " + ec.message());
        return n;
    };
    auto quadro = [&](std::uint32_t magia, const std::string& corpo, std::chrono::seconds limite) {
        std::string q(16, '\0');
        const std::uint32_t v[3] = {std::uint32_t(corpo.size()), magia, seq++};
        for (int i = 0; i < 3; ++i)
            for (int b = 0; b < 4; ++b)
                q[i * 4 + b] = char((v[i] >> (8 * b)) & 0xFF);
        q += corpo;
        rodar(limite, [&](auto h) { net::async_write(s, net::buffer(q), h); });
    };
    auto ler_quadro = [&](std::chrono::seconds limite, std::uint32_t& magia) -> std::string {
        std::array<unsigned char, 16> c{};
        rodar(limite, [&](auto h) { net::async_read(s, net::buffer(c), h); });
        const std::uint32_t tam = c[0] | (c[1] << 8) | (c[2] << 16) | (std::uint32_t(c[3]) << 24);
        magia                   = c[4] | (c[5] << 8) | (c[6] << 16) | (std::uint32_t(c[7]) << 24);
        if (tam > (8u << 20))
            throw std::runtime_error(etapa + ": resposta grande demais da impressora");
        std::string corpo(tam, '\0');
        if (tam > 0)
            rodar(limite, [&](auto h) { net::async_read(s, net::buffer(&corpo[0], tam), h); });
        return corpo;
    };
    // Lê quadros até chegar o JSON que a condição aceita (as respostas de outros comandos ficam para trás).
    auto ler_json = [&](std::chrono::seconds limite, auto aceita) -> json {
        for (int k = 0; k < 64; ++k) {
            std::uint32_t     magia = 0;
            const std::string corpo = ler_quadro(limite, magia);
            const json        j     = json::parse(corpo.substr(0, corpo.find("\n\n")), nullptr, false);
            if (j.is_object() && aceita(j))
                return j;
        }
        throw std::runtime_error(etapa + ": a impressora não deu a resposta esperada");
    };
    auto campo = [](std::string t) {
        t.resize(8, '\0');
        return t;
    };

    try {
        anotar("conexão com a porta " + std::to_string(e.porta));
        rodar(e.limite_resposta, [&](auto h) {
            s.next_layer().async_connect(tcp::endpoint(net::ip::make_address(e.ip), e.porta), [h](const Erro& erro) { h(erro, 0); });
        });
        anotar("TLS da porta " + std::to_string(e.porta));
        rodar(e.limite_resposta, [&](auto h) { s.async_handshake(ssl::stream_base::client, [h](const Erro& erro) { h(erro, 0); }); });

        anotar("login com o código de acesso");
        quadro(LOGIN, campo("bblp") + campo(e.codigo), e.limite_resposta);
        std::uint32_t magia = 0;
        ler_quadro(e.limite_resposta, magia);
        if (magia != LOGIN_OK)
            throw std::runtime_error("a impressora recusou o código de acesso na porta " + std::to_string(e.porta));

        anotar("abertura do canal de arquivos");
        json abre = {{"sequence", 0}, {"mtype", CTRL_SETUP},
                     {"req", {{"t_av", 1}, {"mtype", CTRL_JSON}, {"peer_t", 3}, {"pid", "filuna-slicer"}, {"ver", "02.07.00.55"}}}};
        quadro(CTRL, abre.dump(), e.limite_resposta);
        const json r0 = ler_json(e.limite_resposta, [&](const json& j) { return j.value("mtype", 0) == CTRL_SETUP; });
        if (r0.value("result", -1) != 0)
            throw std::runtime_error("a impressora não abriu o canal de arquivos (resultado " + std::to_string(r0.value("result", -1)) + ")");

        anotar("início do arquivo na memória interna");
        const int n_envio = 1;
        json      ini     = {{"mtype", CTRL_JSON}, {"cmdtype", ENVIO}, {"sequence", n_envio},
                             {"req", {{"type", "model"}, {"path", e.nome_remoto}, {"total", total}, {"storage", "emmc"}}}};
        quadro(CTRL, ini.dump(), e.limite_resposta);
        auto do_envio = [&](const json& j) { return j.value("cmdtype", 0) == ENVIO && j.value("sequence", -1) == n_envio; };
        const json r1 = ler_json(e.limite_resposta, do_envio);
        const int  res = r1.value("result", -1);
        if (res != 1 && res != 19) // 19: já existia, a impressora sobrescreve
            throw std::runtime_error("a impressora não aceitou o arquivo na memória interna (resultado " + std::to_string(res) + ")");
        std::size_t pedaco = 255 * 1024;
        if (r1.contains("reply") && r1["reply"].is_object() && r1["reply"].value("chunk_size", 0) > 0)
            pedaco = std::size_t(r1["reply"].value("chunk_size", 255)) * 1024;

        anotar("envio do arquivo");
        std::vector<char> b(pedaco);
        std::uint64_t     feito = 0;
        int               ultimo = -1;
        for (int frag = 0; feito < total || (total == 0 && frag == 0); ++frag) {
            in.read(b.data(), std::streamsize(b.size()));
            const std::size_t n = std::size_t(in.gcount());
            json              p = {{"mtype", CTRL_JSON}, {"cmdtype", ENVIO}, {"sequence", n_envio}, {"frag_id", frag},
                                   {"req", {{"offset", feito}, {"size", n}}}};
            if (feito + n >= total)
                p["req"]["file_md5"] = md5;
            quadro(CTRL, p.dump() + "\n\n" + std::string(b.data(), n), e.limite_dados);
            feito += n;
            const int pct = total ? int(feito * 100 / total) : 100;
            if (e.progresso && pct != ultimo)
                e.progresso(ultimo = pct);
            if (n == 0)
                break;
        }

        anotar("confirmação da impressora");
        const json r2 = ler_json(e.limite_fim, [&](const json& j) { return do_envio(j) && j.value("result", -1) != 1; });
        const int  fim = r2.value("result", -1);
        if (fim != 0 && fim != 19)
            throw std::runtime_error("a impressora não confirmou o arquivo gravado (resultado " + std::to_string(fim) + ")");
    } catch (const std::exception& ex) {
        fechar();
        return ex.what();
    }
    fechar();
    return {};
}

}} // namespace Slic3r::FilunaBambuEmmc
