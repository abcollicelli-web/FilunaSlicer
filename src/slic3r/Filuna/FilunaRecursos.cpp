#include "FilunaRecursos.hpp"

#include <openssl/evp.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>

namespace Slic3r::Filuna::Recursos {

namespace fs = boost::filesystem;

bool id_valido(const std::string& id)
{
    if (id.empty() || id.size() > 40)
        return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'; });
}

bool versao_valida(const std::string& versao)
{
    if (versao.empty() || versao.size() > 40 || versao.find("..") != std::string::npos)
        return false;
    return std::all_of(versao.begin(), versao.end(), [](unsigned char c) { return std::isalnum(c) || c == '.' || c == '_' || c == '-'; });
}

std::string dir_versao(const std::string& raiz, const std::string& id, const std::string& versao)
{
    return (fs::path(raiz) / id / versao).string();
}

static std::string ler_ponteiro(const fs::path& arquivo)
{
    boost::filesystem::ifstream in(arquivo);
    if (!in)
        return {};
    std::string v;
    std::getline(in, v);
    while (!v.empty() && (v.back() == '\r' || v.back() == '\n' || v.back() == ' '))
        v.pop_back();
    return versao_valida(v) ? v : std::string();
}

std::string versao_atual(const std::string& raiz, const std::string& id)
{
    if (!id_valido(id))
        return {};
    return ler_ponteiro(fs::path(raiz) / id / "atual");
}

std::string dir_atual(const std::string& raiz, const std::string& id)
{
    const std::string v = versao_atual(raiz, id);
    if (v.empty())
        return {};
    const fs::path dir = fs::path(raiz) / id / v;
    boost::system::error_code ec;
    return fs::is_directory(dir, ec) ? dir.string() : std::string();
}

void ativar(const std::string& raiz, const std::string& id, const std::string& versao)
{
    if (!id_valido(id) || !versao_valida(versao))
        throw std::invalid_argument("FilunaRecursos: id ou versão inválidos");
    const fs::path pasta = fs::path(raiz) / id;
    const std::string anterior = ler_ponteiro(pasta / "atual");
    fs::create_directories(pasta);
    {
        boost::filesystem::ofstream out(pasta / "atual.novo", std::ios::trunc);
        out << versao << '\n';
        if (!out)
            throw std::runtime_error("FilunaRecursos: não gravou o ponteiro de " + id);
    }
    fs::rename(pasta / "atual.novo", pasta / "atual"); // troca atômica: ou a versão nova, ou a antiga
    // Poda: fica a versão ativada e a imediatamente anterior (para voltar); o resto sai.
    boost::system::error_code ec;
    for (fs::directory_iterator it(pasta, ec), fim; !ec && it != fim; it.increment(ec)) {
        if (!fs::is_directory(it->path(), ec))
            continue;
        const std::string nome = it->path().filename().string();
        if (nome != versao && nome != anterior)
            fs::remove_all(it->path(), ec);
    }
}

std::vector<std::pair<std::string, std::string>> instalados(const std::string& raiz)
{
    std::vector<std::pair<std::string, std::string>> lista;
    boost::system::error_code ec;
    for (fs::directory_iterator it(fs::path(raiz), ec), fim; !ec && it != fim; it.increment(ec)) {
        const std::string id = it->path().filename().string();
        const std::string v  = versao_atual(raiz, id);
        if (!v.empty())
            lista.emplace_back(id, v);
    }
    std::sort(lista.begin(), lista.end());
    return lista;
}

std::vector<unsigned char> base64_decodificar(const std::string& texto)
{
    static const auto valor = [] {
        std::array<int, 256> t{};
        t.fill(-1);
        const char* a = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; ++i)
            t[static_cast<unsigned char>(a[i])] = i;
        return t;
    }();
    std::vector<unsigned char> saida;
    int acc = 0, bits = 0;
    for (unsigned char c : texto) {
        if (c == '=' || c == '\n' || c == '\r')
            continue;
        const int v = valor[c];
        if (v < 0)
            return {};
        acc = (acc << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            saida.push_back(static_cast<unsigned char>((acc >> bits) & 0xFF));
        }
    }
    return saida;
}

bool verificar_ed25519(const std::string& mensagem, const std::string& assinatura_b64, const std::string& chave_publica_hex)
{
    if (chave_publica_hex.size() != 64)
        return false;
    unsigned char chave[32];
    for (int i = 0; i < 32; ++i) {
        const auto nib = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        const int a = nib(chave_publica_hex[2 * i]), b = nib(chave_publica_hex[2 * i + 1]);
        if (a < 0 || b < 0)
            return false;
        chave[i] = static_cast<unsigned char>((a << 4) | b);
    }
    const std::vector<unsigned char> assinatura = base64_decodificar(assinatura_b64);
    if (assinatura.size() != 64)
        return false;
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, chave, sizeof(chave));
    if (pkey == nullptr)
        return false;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    bool ok = ctx != nullptr && EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) == 1 &&
              EVP_DigestVerify(ctx, assinatura.data(), assinatura.size(),
                               reinterpret_cast<const unsigned char*>(mensagem.data()), mensagem.size()) == 1;
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ok;
}

} // namespace Slic3r::Filuna::Recursos
