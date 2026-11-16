#pragma once

// Filuna — câmera RTSPS da Bambu (P2S, X1, H2) convertida para FLV, que o tocador da tela (mpegts.js) já usa.
// A impressora manda H.264 em RTP, intercalado na própria conexão RTSP (TLS, porta 322, usuário "bblp" e o código
// de acesso). Aqui: OPTIONS/DESCRIBE/SETUP/PLAY, desmonta o RTP (NAL simples, STAP-A e FU-A), junta cada quadro e
// escreve como etiqueta de vídeo FLV (AVC). Nada é decodificado: os bytes do vídeo passam como vieram.

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/write.hpp>

#include <openssl/evp.h>
#include <boost/log/trivial.hpp>

namespace Slic3r { namespace Filuna { namespace Rtsp {

inline std::string base64_decodificar(const std::string& in)
{
    static const std::string tab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int         val = 0, bits = -8;
    for (unsigned char c : in) {
        const std::size_t p = tab.find(char(c));
        if (p == std::string::npos)
            continue;
        val = (val << 6) + int(p);
        bits += 6;
        if (bits >= 0) {
            out.push_back(char((val >> bits) & 0xff));
            bits -= 8;
        }
    }
    return out;
}

inline std::string base64_codificar(const std::string& in)
{
    static const char tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int         val = 0, bits = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        bits += 8;
        while (bits >= 0) {
            out.push_back(tab[(val >> bits) & 0x3f]);
            bits -= 6;
        }
    }
    if (bits > -6)
        out.push_back(tab[((val << 8) >> (bits + 8)) & 0x3f]);
    while (out.size() % 4)
        out.push_back('=');
    return out;
}

inline std::string md5_hex(const std::string& s)
{
    unsigned char d[EVP_MAX_MD_SIZE];
    unsigned int  n = 0;
    EVP_Digest(s.data(), s.size(), d, &n, EVP_md5(), nullptr);
    static const char hx[] = "0123456789abcdef";
    std::string out;
    for (unsigned int i = 0; i < n; ++i) {
        out.push_back(hx[d[i] >> 4]);
        out.push_back(hx[d[i] & 15]);
    }
    return out;
}

// valor de um parâmetro em 'Digest realm="x", nonce="y"'
inline std::string parametro(const std::string& cab, const std::string& nome)
{
    const std::size_t i = cab.find(nome + "=\"");
    if (i == std::string::npos)
        return std::string();
    const std::size_t ini = i + nome.size() + 2;
    const std::size_t fim = cab.find('"', ini);
    return fim == std::string::npos ? std::string() : cab.substr(ini, fim - ini);
}

struct Resposta
{
    int                                status = 0;
    std::map<std::string, std::string> cab; // nomes em minúsculas
    std::string                        corpo;
};

// Monta o FLV a partir dos quadros H.264 (NALs sem prefixo).
class Flv
{
public:
    std::string cabecalho() const { return std::string("FLV\x01\x01\x00\x00\x00\x09\x00\x00\x00\x00", 13); }

    // devolve as etiquetas FLV deste quadro (vazio enquanto não houver SPS/PPS e um quadro-chave)
    std::string quadro(const std::vector<std::string>& nals, std::uint32_t ms)
    {
        bool chave = false;
        for (const auto& n : nals) {
            const int t = n.empty() ? 0 : (n[0] & 0x1f);
            if (t == 7)
                m_sps = n;
            else if (t == 8)
                m_pps = n;
            else if (t == 5)
                chave = true;
        }
        std::string out;
        if (m_sps.size() >= 4 && !m_pps.empty() && (m_sps != m_sps_enviado || m_pps != m_pps_enviado)) {
            std::string d("\x17\x00\x00\x00\x00", 5);
            d += char(1);
            d += m_sps.substr(1, 3);
            d += char(0xff);
            d += char(0xe1);
            d += u16(m_sps.size()) + m_sps;
            d += char(1);
            d += u16(m_pps.size()) + m_pps;
            out += etiqueta(d, ms);
            m_sps_enviado = m_sps;
            m_pps_enviado = m_pps;
        }
        if (m_sps_enviado.empty() || (!m_comecou && !chave))
            return out;
        m_comecou = true;
        std::string d(chave ? "\x17\x01\x00\x00\x00" : "\x27\x01\x00\x00\x00", 5);
        bool        tem = false;
        for (const auto& n : nals) {
            const int t = n.empty() ? 0 : (n[0] & 0x1f);
            if (n.empty() || t == 7 || t == 8 || t == 9)
                continue;
            d += u32(n.size()) + n;
            tem = true;
        }
        if (tem)
            out += etiqueta(d, ms);
        return out;
    }

    void set_sps_pps(const std::string& sps, const std::string& pps)
    {
        m_sps = sps;
        m_pps = pps;
    }

private:
    static std::string u16(std::size_t v) { return std::string{char((v >> 8) & 0xff), char(v & 0xff)}; }
    static std::string u24(std::size_t v) { return std::string{char((v >> 16) & 0xff), char((v >> 8) & 0xff), char(v & 0xff)}; }
    static std::string u32(std::size_t v) { return std::string{char((v >> 24) & 0xff), char((v >> 16) & 0xff), char((v >> 8) & 0xff), char(v & 0xff)}; }
    static std::string etiqueta(const std::string& dados, std::uint32_t ms)
    {
        std::string t;
        t += char(9);
        t += u24(dados.size());
        t += u24(ms & 0xffffff);
        t += char((ms >> 24) & 0xff);
        t += u24(0);
        t += dados;
        t += u32(dados.size() + 11);
        return t;
    }
    std::string m_sps, m_pps, m_sps_enviado, m_pps_enviado;
    bool        m_comecou = false;
};

// Desmonta o RTP H.264 (RFC 6184) em quadros completos.
class Depack
{
public:
    // true quando 'quadro' e 'ts' trazem um quadro completo
    bool pacote(const unsigned char* p, std::size_t n, std::vector<std::string>& quadro, std::uint32_t& ts)
    {
        if (n < 12 || (p[0] >> 6) != 2)
            return false;
        const int     cc     = p[0] & 0x0f;
        const bool    ext    = (p[0] & 0x10) != 0;
        const bool    pad    = (p[0] & 0x20) != 0;
        const bool    marca  = (p[1] & 0x80) != 0;
        const std::uint32_t t = (std::uint32_t(p[4]) << 24) | (std::uint32_t(p[5]) << 16) | (std::uint32_t(p[6]) << 8) | p[7];
        std::size_t   ini    = 12 + 4 * cc;
        if (ext) {
            if (ini + 4 > n)
                return false;
            ini += 4 + 4 * ((std::size_t(p[ini + 2]) << 8) | p[ini + 3]);
        }
        std::size_t fim = n;
        if (pad && n > 0)
            fim -= std::min<std::size_t>(p[n - 1], n);
        if (ini >= fim)
            return false;
        bool pronto = false;
        if (!m_nals.empty() && t != m_ts) { // começou outro quadro sem a marca no anterior
            quadro = std::move(m_nals);
            ts     = m_ts;
            m_nals.clear();
            pronto = true;
        }
        m_ts = t;
        const unsigned char* d   = p + ini;
        const std::size_t    len = fim - ini;
        const int            tipo = d[0] & 0x1f;
        if (tipo >= 1 && tipo <= 23)
            m_nals.emplace_back(reinterpret_cast<const char*>(d), len);
        else if (tipo == 24) { // STAP-A
            std::size_t i = 1;
            while (i + 2 <= len) {
                const std::size_t s = (std::size_t(d[i]) << 8) | d[i + 1];
                i += 2;
                if (i + s > len)
                    break;
                m_nals.emplace_back(reinterpret_cast<const char*>(d + i), s);
                i += s;
            }
        } else if (tipo == 28 && len >= 2) { // FU-A
            const bool inicio = (d[1] & 0x80) != 0, final = (d[1] & 0x40) != 0;
            if (inicio) {
                m_fu.assign(1, char((d[0] & 0xe0) | (d[1] & 0x1f)));
                m_fu_ok = true;
            }
            if (m_fu_ok)
                m_fu.append(reinterpret_cast<const char*>(d + 2), len - 2);
            if (final && m_fu_ok) {
                m_nals.push_back(m_fu);
                m_fu.clear();
                m_fu_ok = false;
            }
        }
        if (marca && !m_nals.empty() && !pronto) {
            quadro = std::move(m_nals);
            ts     = m_ts;
            m_nals.clear();
            pronto = true;
        }
        return pronto;
    }

private:
    std::vector<std::string> m_nals;
    std::string              m_fu;
    bool                     m_fu_ok = false;
    std::uint32_t            m_ts    = 0;
};

// Liga na câmera e chama 'escrever' com o FLV (primeiro o cabeçalho). 'escrever' devolve false quando a tela
// fechou o vídeo. Devolve o erro quando nem chegou a tocar (para a tela receber 502); vazio = tocou e acabou.
inline std::string tocar_(const std::string& ip, const std::string& porta, const std::string& caminho, const std::string& usuario,
                          const std::string& senha, const std::function<bool(const std::string&)>& escrever, const std::string& esquema);

// Registra no log do programa cada tentativa e o motivo de uma recusa (o log segue junto com o diagnóstico).
inline std::string tocar(const std::string& ip, const std::string& porta, const std::string& caminho, const std::string& usuario,
                         const std::string& senha, const std::function<bool(const std::string&)>& escrever)
{
    // a Bambu informa "rtsps://..."; os pedidos usam o mesmo esquema (o servidor pode recusar "rtsp://" em TLS)
    std::string erro = tocar_(ip, porta, caminho, usuario, senha, escrever, "rtsps");
    if (!erro.empty() && erro.rfind("sem conexão", 0) != 0) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna câmera RTSPS " << ip << ": " << erro << "; tentando com rtsp://";
        erro = tocar_(ip, porta, caminho, usuario, senha, escrever, "rtsp");
    }
    if (!erro.empty())
        BOOST_LOG_TRIVIAL(error) << "Filuna câmera RTSPS " << ip << ":" << porta << caminho << " falhou: " << erro;
    return erro;
}

inline std::string tocar_(const std::string& ip, const std::string& porta, const std::string& caminho, const std::string& usuario,
                          const std::string& senha, const std::function<bool(const std::string&)>& escrever, const std::string& esquema)
{
    namespace asio = boost::asio;
    namespace ssl  = asio::ssl;
    using tcp      = asio::ip::tcp;
    boost::system::error_code ec;
    asio::io_context          ioc;
    ssl::context              ctx(ssl::context::tls_client);
    ctx.set_verify_mode(ssl::verify_none); // certificado próprio da impressora
    ssl::stream<tcp::socket> s(ioc, ctx);
    tcp::resolver            res(ioc);
    asio::connect(s.next_layer(), res.resolve(ip, porta, ec), ec);
    if (!ec)
        s.handshake(ssl::stream_base::client, ec);
    if (ec)
        return "sem conexão com a câmera: " + ec.message();

    const std::string url  = esquema + "://" + ip + ":" + porta + caminho;
    int               seq  = 0;
    std::string       sobra; // bytes lidos e ainda não usados
    std::string       auth_digest_realm, auth_digest_nonce;
    bool              usar_digest = false;
    std::string       sessao;

    auto ler_mais = [&]() {
        char              b[16384];
        const std::size_t n = s.read_some(asio::buffer(b), ec);
        if (ec || n == 0)
            return false;
        sobra.append(b, n);
        return true;
    };
    auto autorizacao = [&](const std::string& metodo, const std::string& uri) {
        if (usar_digest) {
            const std::string ha1 = md5_hex(usuario + ":" + auth_digest_realm + ":" + senha);
            const std::string ha2 = md5_hex(metodo + ":" + uri);
            return "Authorization: Digest username=\"" + usuario + "\", realm=\"" + auth_digest_realm + "\", nonce=\"" +
                   auth_digest_nonce + "\", uri=\"" + uri + "\", response=\"" + md5_hex(ha1 + ":" + auth_digest_nonce + ":" + ha2) + "\"\r\n";
        }
        return "Authorization: Basic " + base64_codificar(usuario + ":" + senha) + "\r\n";
    };
    // lê uma resposta RTSP (pula pacotes RTP intercalados que cheguem antes)
    auto ler_resposta = [&](Resposta& r) {
        for (;;) {
            while (!sobra.empty() && sobra[0] == '$') {
                while (sobra.size() < 4)
                    if (!ler_mais())
                        return false;
                const std::size_t tam = (std::size_t((unsigned char) sobra[2]) << 8) | (unsigned char) sobra[3];
                while (sobra.size() < 4 + tam)
                    if (!ler_mais())
                        return false;
                sobra.erase(0, 4 + tam);
            }
            const std::size_t fim = sobra.find("\r\n\r\n");
            if (fim != std::string::npos && (sobra.empty() || sobra[0] != '$'))
                break;
            if (!ler_mais())
                return false;
        }
        const std::size_t  fim = sobra.find("\r\n\r\n");
        std::istringstream is(sobra.substr(0, fim));
        sobra.erase(0, fim + 4);
        std::string linha, versao;
        std::getline(is, linha);
        std::istringstream l0(linha);
        l0 >> versao >> r.status;
        while (std::getline(is, linha)) {
            if (!linha.empty() && linha.back() == '\r')
                linha.pop_back();
            const std::size_t dp = linha.find(':');
            if (dp == std::string::npos)
                continue;
            std::string nome = linha.substr(0, dp), valor = linha.substr(dp + 1);
            for (char& c : nome)
                c = char(std::tolower(static_cast<unsigned char>(c)));
            while (!valor.empty() && valor[0] == ' ')
                valor.erase(0, 1);
            r.cab[nome] = valor;
        }
        const std::size_t tam = r.cab.count("content-length") ? std::stoul(r.cab["content-length"]) : 0;
        while (sobra.size() < tam)
            if (!ler_mais())
                return false;
        r.corpo = sobra.substr(0, tam);
        sobra.erase(0, tam);
        return true;
    };
    auto pedir = [&](const std::string& metodo, const std::string& uri, const std::string& extra, Resposta& r) {
        for (int tentativa = 0; tentativa < 2; ++tentativa) {
            r = Resposta();
            std::string q = metodo + " " + uri + " RTSP/1.0\r\nCSeq: " + std::to_string(++seq) + "\r\nUser-Agent: Filuna\r\n" +
                            autorizacao(metodo, uri) + (sessao.empty() ? std::string() : "Session: " + sessao + "\r\n") + extra + "\r\n";
            asio::write(s, asio::buffer(q), ec);
            if (ec || !ler_resposta(r))
                return false;
            if (r.status == 401 && r.cab.count("www-authenticate") && r.cab["www-authenticate"].rfind("Digest", 0) == 0 && !usar_digest) {
                usar_digest       = true;
                auth_digest_realm = parametro(r.cab["www-authenticate"], "realm");
                auth_digest_nonce = parametro(r.cab["www-authenticate"], "nonce");
                continue;
            }
            return true;
        }
        return true;
    };

    Resposta r;
    if (!pedir("DESCRIBE", url, "Accept: application/sdp\r\n", r) || r.status != 200)
        return r.status == 401 ? std::string("código de acesso recusado pela câmera (") +
                                     (r.cab.count("www-authenticate") ? r.cab["www-authenticate"] : std::string("sem WWW-Authenticate")) + ")"
                               : "DESCRIBE " + url + " respondeu " + std::to_string(r.status) + (ec ? " (" + ec.message() + ")" : std::string());
    BOOST_LOG_TRIVIAL(info) << "Filuna câmera RTSPS " << ip << " SDP: " << r.corpo.substr(0, 700);
    // SDP: trilha de vídeo, controle e SPS/PPS
    std::string base = r.cab.count("content-base") ? r.cab["content-base"] : url;
    std::string controle, sprop;
    {
        std::istringstream sdp(r.corpo);
        std::string        linha;
        bool               video = false;
        while (std::getline(sdp, linha)) {
            if (!linha.empty() && linha.back() == '\r')
                linha.pop_back();
            if (linha.rfind("m=", 0) == 0)
                video = linha.rfind("m=video", 0) == 0;
            else if (video && linha.rfind("a=control:", 0) == 0)
                controle = linha.substr(10);
            else if (video && linha.find("sprop-parameter-sets=") != std::string::npos) {
                sprop = linha.substr(linha.find("sprop-parameter-sets=") + 21);
                sprop = sprop.substr(0, sprop.find(';'));
            }
        }
    }
    std::string trilha = controle.empty() || controle == "*" ? base
                       : controle.rfind("rtsp://", 0) == 0 ? controle
                       : base + (base.back() == '/' ? "" : "/") + controle;
    if (!pedir("SETUP", trilha, "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n", r) || r.status != 200)
        return "câmera recusou a transmissão (" + std::to_string(r.status) + ")";
    sessao = r.cab.count("session") ? r.cab["session"].substr(0, r.cab["session"].find(';')) : std::string();
    if (!pedir("PLAY", base, "Range: npt=0.000-\r\n", r) || r.status != 200)
        return "câmera não começou o vídeo (" + std::to_string(r.status) + ")";

    Flv flv;
    if (!sprop.empty()) {
        const std::size_t v = sprop.find(',');
        if (v != std::string::npos)
            flv.set_sps_pps(base64_decodificar(sprop.substr(0, v)), base64_decodificar(sprop.substr(v + 1)));
    }
    if (!escrever(flv.cabecalho()))
        return std::string();
    Depack                   dp;
    std::vector<std::string> quadro;
    std::uint32_t            ts = 0, ts0 = 0;
    bool                     tem_ts0 = false;
    auto                     ultimo_aviso = std::chrono::steady_clock::now();
    for (;;) {
        while (sobra.size() < 4)
            if (!ler_mais())
                return std::string();
        if (sobra[0] != '$') { // resposta RTSP no meio do vídeo (ex.: do OPTIONS de manter a sessão)
            Resposta ign;
            if (!ler_resposta(ign))
                return std::string();
            continue;
        }
        const int         canal = (unsigned char) sobra[1];
        const std::size_t tam   = (std::size_t((unsigned char) sobra[2]) << 8) | (unsigned char) sobra[3];
        while (sobra.size() < 4 + tam)
            if (!ler_mais())
                return std::string();
        if (canal == 0 && dp.pacote(reinterpret_cast<const unsigned char*>(sobra.data() + 4), tam, quadro, ts)) {
            if (!tem_ts0) {
                ts0     = ts;
                tem_ts0 = true;
            }
            const std::string tags = flv.quadro(quadro, std::uint32_t((ts - ts0) / 90));
            if (!tags.empty() && !escrever(tags))
                break;
        }
        sobra.erase(0, 4 + tam);
        // mantém a sessão viva (o padrão do RTSP derruba em 60 s sem pedidos)
        if (std::chrono::steady_clock::now() - ultimo_aviso > std::chrono::seconds(25)) {
            ultimo_aviso    = std::chrono::steady_clock::now();
            const std::string q = "OPTIONS " + url + " RTSP/1.0\r\nCSeq: " + std::to_string(++seq) + "\r\n" + autorizacao("OPTIONS", url) +
                                  "Session: " + sessao + "\r\n\r\n";
            asio::write(s, asio::buffer(q), ec);
            if (ec)
                break;
        }
    }
    const std::string fim = "TEARDOWN " + base + " RTSP/1.0\r\nCSeq: " + std::to_string(++seq) + "\r\n" + autorizacao("TEARDOWN", base) +
                            "Session: " + sessao + "\r\n\r\n";
    asio::write(s, asio::buffer(fim), ec);
    return std::string();
}

}}} // namespace Slic3r::Filuna::Rtsp
