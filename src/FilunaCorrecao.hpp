#pragma once

// Correção do programa vinda do servidor Filuna (só Windows, versão da Microsoft Store).
//
// O pacote da loja não pode ser alterado, então uma correção de defeito no código chega como uma OrcaSlicer.dll
// corrigida numa pasta do usuário, e o lançador (este arquivo roda dentro dele, que faz parte do pacote assinado)
// decide se ela pode ser carregada. Só carrega quando TUDO isto confere:
//   1. o documento da correção tem a assinatura ECDSA P-256 da Filuna (chave pública fixa aqui embaixo);
//   2. o documento é para esta mesma compilação do pacote (filuna-build.json ao lado do executável);
//   3. a correção não venceu (toda correção tem validade: depois dela vale só o que veio pela loja);
//   4. o SHA-256 da DLL na pasta é o que o documento assinado diz;
//   5. a correção não derrubou o programa nas duas últimas aberturas (trava contra queda em sequência).
// Qualquer falha: carrega a DLL do pacote, como se a correção não existisse.
//
// O que pode ou não vir por aqui está em FILUNA/ATUALIZACAO-REMOTA.md; a ferramenta de publicação recusa o resto.
// Sem dependências além do Windows (bcrypt), de propósito: é a única peça que decide que código roda.

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace filuna_correcao {

// Chave pública ECDSA P-256 das correções do programa: X e Y (32 bytes cada) em hexadecimal.
// A chave privada fica só no servidor (/root/.filuna-correcao) e é diferente da chave do catálogo de módulos.
inline const char* const CHAVE_PUBLICA = "17cc9da727298401bbf181618af4e349c2b1da265ab6fa0b74e7ed767efcf123d091ecee949cbbbd58098b57ef3e06852c96133fd97632df1847281af4f77218";

inline const size_t MAXIMO_DOCUMENTO = 4096;

struct Documento
{
    std::string build;      // compilação do pacote a que a correção se aplica
    std::string dll_sha256; // SHA-256 da OrcaSlicer.dll corrigida
    std::string validade;   // AAAA-MM-DD (UTC); depois desse dia a correção não é mais carregada
    int         versao = 0; // 1, 2, 3… dentro da mesma compilação
};

inline bool de_hex(const std::string& hex, std::vector<unsigned char>& bytes)
{
    if (hex.size() % 2 != 0)
        return false;
    bytes.clear();
    bytes.reserve(hex.size() / 2);
    auto valor = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < hex.size(); i += 2) {
        const int a = valor(hex[i]), b = valor(hex[i + 1]);
        if (a < 0 || b < 0)
            return false;
        bytes.push_back(static_cast<unsigned char>((a << 4) | b));
    }
    return true;
}

inline std::string para_hex(const unsigned char* p, size_t n)
{
    static const char* const digitos = "0123456789abcdef";
    std::string s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) {
        s.push_back(digitos[p[i] >> 4]);
        s.push_back(digitos[p[i] & 15]);
    }
    return s;
}

// SHA-256 em partes (arquivos grandes não vão inteiros para a memória).
class Sha256
{
public:
    Sha256()
    {
        if (BCryptOpenAlgorithmProvider(&m_alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
            m_alg = nullptr;
        else if (BCryptCreateHash(m_alg, &m_hash, nullptr, 0, nullptr, 0, 0) != 0)
            m_hash = nullptr;
    }
    ~Sha256()
    {
        if (m_hash != nullptr) BCryptDestroyHash(m_hash);
        if (m_alg != nullptr) BCryptCloseAlgorithmProvider(m_alg, 0);
    }
    Sha256(const Sha256&)            = delete;
    Sha256& operator=(const Sha256&) = delete;

    bool mais(const unsigned char* dados, size_t n)
    {
        return m_hash != nullptr && BCryptHashData(m_hash, const_cast<PUCHAR>(dados), static_cast<ULONG>(n), 0) == 0;
    }
    bool fim(unsigned char saida[32]) { return m_hash != nullptr && BCryptFinishHash(m_hash, saida, 32, 0) == 0; }

private:
    BCRYPT_ALG_HANDLE  m_alg  = nullptr;
    BCRYPT_HASH_HANDLE m_hash = nullptr;
};

inline bool sha256_do_arquivo(const std::wstring& caminho, std::string& hex)
{
    const HANDLE arq = CreateFileW(caminho.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (arq == INVALID_HANDLE_VALUE)
        return false;
    Sha256                     h;
    std::vector<unsigned char> bloco(1 << 20);
    bool                       ok = true;
    for (;;) {
        DWORD lido = 0;
        if (!ReadFile(arq, bloco.data(), static_cast<DWORD>(bloco.size()), &lido, nullptr)) { ok = false; break; }
        if (lido == 0)
            break;
        if (!h.mais(bloco.data(), lido)) { ok = false; break; }
    }
    CloseHandle(arq);
    unsigned char resumo[32];
    if (!ok || !h.fim(resumo))
        return false;
    hex = para_hex(resumo, 32);
    return true;
}

// Assinatura ECDSA P-256 sobre o SHA-256 da mensagem. assinatura_hex: R e S (32 bytes cada), 128 dígitos.
inline bool assinatura_confere(const std::string& mensagem, const std::string& assinatura_hex, const char* chave_hex = CHAVE_PUBLICA)
{
    std::vector<unsigned char> chave, assinatura;
    if (!de_hex(chave_hex, chave) || chave.size() != 64 || !de_hex(assinatura_hex, assinatura) || assinatura.size() != 64)
        return false;
    unsigned char resumo[32];
    {
        Sha256 h;
        if (!h.mais(reinterpret_cast<const unsigned char*>(mensagem.data()), mensagem.size()) || !h.fim(resumo))
            return false;
    }
    std::vector<unsigned char> blob(sizeof(BCRYPT_ECCKEY_BLOB) + 64);
    BCRYPT_ECCKEY_BLOB         cab;
    cab.dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    cab.cbKey   = 32;
    memcpy(blob.data(), &cab, sizeof(cab));
    memcpy(blob.data() + sizeof(cab), chave.data(), 64);

    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_KEY_HANDLE key = nullptr;
    bool              ok  = false;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0) == 0) {
        if (BCryptImportKeyPair(alg, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key, blob.data(), static_cast<ULONG>(blob.size()), 0) == 0) {
            ok = BCryptVerifySignature(key, nullptr, resumo, 32, assinatura.data(), 64, 0) == 0;
            BCryptDestroyKey(key);
        }
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    return ok;
}

inline bool so_estes(const std::string& s, const char* permitidos)
{
    return !s.empty() && s.find_first_not_of(permitidos) == std::string::npos;
}

// Documento (texto exato que foi assinado):
//   filuna-correcao-1
//   build=<compilação>
//   dll_sha256=<64 dígitos>
//   versao=<número>
//   validade=<AAAA-MM-DD>
inline bool ler_documento(const std::string& texto, Documento& d)
{
    if (texto.size() > MAXIMO_DOCUMENTO)
        return false;
    d = Documento();
    size_t pos    = 0;
    int    linha  = 0;
    bool   versao = false;
    while (pos < texto.size()) {
        size_t fim = texto.find('\n', pos);
        if (fim == std::string::npos)
            fim = texto.size();
        const std::string l = texto.substr(pos, fim - pos);
        pos = fim + 1;
        if (linha++ == 0) {
            if (l != "filuna-correcao-1")
                return false;
            continue;
        }
        const size_t igual = l.find('=');
        if (igual == std::string::npos)
            return false;
        const std::string k = l.substr(0, igual), v = l.substr(igual + 1);
        if (k == "build")
            d.build = v;
        else if (k == "dll_sha256")
            d.dll_sha256 = v;
        else if (k == "validade")
            d.validade = v;
        else if (k == "versao") {
            if (!so_estes(v, "0123456789") || v.size() > 6)
                return false;
            d.versao = atoi(v.c_str());
            versao   = true;
        } else
            return false; // campo desconhecido: documento de outro formato
    }
    return versao && d.versao > 0 && so_estes(d.build, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._-") && d.build.size() <= 40 &&
           d.dll_sha256.size() == 64 && so_estes(d.dll_sha256, "0123456789abcdef") && d.validade.size() == 10 &&
           so_estes(d.validade, "0123456789-") && d.validade[4] == '-' && d.validade[7] == '-';
}

inline bool ler_arquivo_pequeno(const std::wstring& caminho, std::string& texto)
{
    const HANDLE arq = CreateFileW(caminho.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (arq == INVALID_HANDLE_VALUE)
        return false;
    std::vector<char> buf(MAXIMO_DOCUMENTO + 1);
    DWORD             lido = 0;
    const bool        ok   = ReadFile(arq, buf.data(), static_cast<DWORD>(buf.size()), &lido, nullptr) != 0;
    CloseHandle(arq);
    if (!ok || lido > MAXIMO_DOCUMENTO)
        return false;
    texto.assign(buf.data(), lido);
    return true;
}

inline bool gravar_arquivo_pequeno(const std::wstring& caminho, const std::string& texto)
{
    const HANDLE arq = CreateFileW(caminho.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (arq == INVALID_HANDLE_VALUE)
        return false;
    DWORD      gravado = 0;
    const bool ok      = WriteFile(arq, texto.data(), static_cast<DWORD>(texto.size()), &gravado, nullptr) != 0 && gravado == texto.size();
    CloseHandle(arq);
    return ok;
}

inline std::string sem_espacos_no_fim(std::string s)
{
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s;
}

// "build" de filuna-build.json ({"build":"c26687406","versao":"…"}), que vem dentro do pacote.
inline std::string build_do_pacote(const std::wstring& pasta_do_programa)
{
    std::string texto;
    if (!ler_arquivo_pequeno(pasta_do_programa + L"filuna-build.json", texto))
        return {};
    const size_t chave = texto.find("\"build\"");
    if (chave == std::string::npos)
        return {};
    const size_t abre = texto.find('"', texto.find(':', chave) == std::string::npos ? texto.size() : texto.find(':', chave));
    if (abre == std::string::npos)
        return {};
    const size_t fecha = texto.find('"', abre + 1);
    if (fecha == std::string::npos)
        return {};
    const std::string b = texto.substr(abre + 1, fecha - abre - 1);
    return so_estes(b, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._-") && b.size() <= 40 ? b : std::string();
}

// Pasta da correção desta compilação: %LOCALAPPDATA%\Filuna\correcao\<build>\ (termina com barra).
// FILUNA_CORRECAO_DIR troca a raiz (testes); não afrouxa nada: o conteúdo continua tendo de estar assinado.
inline std::wstring pasta_da_correcao(const std::string& build)
{
    wchar_t raiz[MAX_PATH + 1] = {0};
    DWORD   n = GetEnvironmentVariableW(L"FILUNA_CORRECAO_DIR", raiz, MAX_PATH);
    std::wstring base;
    if (n > 0 && n < MAX_PATH)
        base = raiz;
    else {
        n = GetEnvironmentVariableW(L"LOCALAPPDATA", raiz, MAX_PATH);
        if (n == 0 || n >= MAX_PATH)
            return {};
        base = std::wstring(raiz) + L"\\Filuna\\correcao";
    }
    return base + L"\\" + std::wstring(build.begin(), build.end()) + L"\\";
}

inline std::string hoje_utc()
{
    SYSTEMTIME t;
    GetSystemTime(&t);
    char s[16];
    snprintf(s, sizeof(s), "%04u-%02u-%02u", static_cast<unsigned>(t.wYear), static_cast<unsigned>(t.wMonth), static_cast<unsigned>(t.wDay));
    return s;
}

// Confere a correção que está na pasta (sem olhar a trava de quedas). Devolve true e preenche o documento.
inline bool correcao_valida(const std::wstring& pasta, const std::string& build, Documento& d, std::string& motivo)
{
    std::string texto, assinatura;
    if (!ler_arquivo_pequeno(pasta + L"correcao.txt", texto) || !ler_arquivo_pequeno(pasta + L"correcao.sig", assinatura)) {
        motivo = "sem correção";
        return false;
    }
    if (!assinatura_confere(texto, sem_espacos_no_fim(assinatura))) {
        motivo = "assinatura inválida";
        return false;
    }
    if (!ler_documento(texto, d)) {
        motivo = "documento ilegível";
        return false;
    }
    if (d.build != build) {
        motivo = "correção de outra compilação";
        return false;
    }
    if (hoje_utc() > d.validade) {
        motivo = "correção vencida";
        return false;
    }
    std::string sha;
    if (!sha256_do_arquivo(pasta + L"OrcaSlicer.dll", sha) || sha != d.dll_sha256) {
        motivo = "a DLL não é a assinada";
        return false;
    }
    return true;
}

// Quantas aberturas seguidas começaram com esta correção sem o programa confirmar que abriu bem.
inline int tentativas(const std::wstring& pasta)
{
    std::string t;
    return ler_arquivo_pequeno(pasta + L"tentativas", t) ? atoi(t.c_str()) : 0;
}

// Para o LANÇADOR: caminho da DLL corrigida a carregar, ou vazio (carregar a do pacote).
// pasta_do_programa termina com barra. Conta a tentativa antes de devolver; o programa zera ao abrir bem.
inline std::wstring escolher(const std::wstring& pasta_do_programa, std::string* porque = nullptr)
{
    std::string motivo;
    std::wstring dll;
    const std::string build = build_do_pacote(pasta_do_programa);
    const std::wstring pasta = build.empty() ? std::wstring() : pasta_da_correcao(build);
    Documento d;
    if (pasta.empty())
        motivo = "pacote sem identidade";
    else if (correcao_valida(pasta, build, d, motivo)) {
        const int n = tentativas(pasta);
        if (n >= 2)
            motivo = "a correção derrubou o programa duas vezes";
        else if (!gravar_arquivo_pequeno(pasta + L"tentativas", std::to_string(n + 1)))
            motivo = "não deu para registrar a tentativa";
        else {
            dll = pasta + L"OrcaSlicer.dll";
            SetEnvironmentVariableA("FILUNA_CORRECAO_EM_USO", std::to_string(d.versao).c_str());
        }
    }
    if (porque != nullptr)
        *porque = motivo;
    return dll;
}

// Para o PROGRAMA, depois de abrir a janela principal: a correção em uso não derrubou nada.
inline void abriu_bem(const std::wstring& pasta_do_programa)
{
    const std::string build = build_do_pacote(pasta_do_programa);
    if (!build.empty())
        DeleteFileW((pasta_da_correcao(build) + L"tentativas").c_str());
}

// ---------- Para o PROGRAMA (versão da loja): instalar, conferir e retirar a correção ----------

inline void criar_pastas(const std::wstring& caminho)
{
    for (size_t i = 3; i <= caminho.size(); ++i)
        if (i == caminho.size() || caminho[i] == L'\\')
            CreateDirectoryW(caminho.substr(0, i).c_str(), nullptr);
}

// Versão da correção instalada e válida para esta compilação (0 = nenhuma). Lê a DLL inteira: chamar pouco.
inline int versao_instalada(const std::wstring& pasta_do_programa)
{
    const std::string build = build_do_pacote(pasta_do_programa);
    Documento         d;
    std::string       motivo;
    return !build.empty() && correcao_valida(pasta_da_correcao(build), build, d, motivo) ? d.versao : 0;
}

// Sobras de uma DLL corrigida que estava em uso quando a seguinte chegou.
inline void limpar_velhas(const std::wstring& pasta)
{
    WIN32_FIND_DATAW achado;
    const HANDLE     busca = FindFirstFileW((pasta + L"OrcaSlicer.dll.velha-*").c_str(), &achado);
    if (busca == INVALID_HANDLE_VALUE)
        return;
    do
        DeleteFileW((pasta + achado.cFileName).c_str());
    while (FindNextFileW(busca, &achado));
    FindClose(busca);
}

// Tira a correção desta compilação (o servidor retirou, ou ela derrubou o programa). A DLL em uso fica até fechar.
inline void retirar(const std::wstring& pasta_do_programa)
{
    const std::string build = build_do_pacote(pasta_do_programa);
    if (build.empty())
        return;
    const std::wstring pasta = pasta_da_correcao(build);
    DeleteFileW((pasta + L"correcao.txt").c_str());
    DeleteFileW((pasta + L"correcao.sig").c_str());
    DeleteFileW((pasta + L"tentativas").c_str());
    DeleteFileW((pasta + L"OrcaSlicer.dll").c_str());
    limpar_velhas(pasta);
}

// Versão que o lançador recusou por derrubar o programa (0 = nenhuma): o programa não baixa a mesma de novo.
inline int recusada(const std::wstring& pasta_do_programa)
{
    const std::string build = build_do_pacote(pasta_do_programa);
    std::string       t;
    return !build.empty() && ler_arquivo_pequeno(pasta_da_correcao(build) + L"recusada", t) ? atoi(t.c_str()) : 0;
}

inline void marcar_recusada(const std::wstring& pasta_do_programa, int versao)
{
    const std::string build = build_do_pacote(pasta_do_programa);
    if (!build.empty())
        gravar_arquivo_pequeno(pasta_da_correcao(build) + L"recusada", std::to_string(versao));
}

// Instala a correção baixada: confere tudo de novo (assinatura, compilação, validade, SHA-256 da DLL) e só então
// troca os arquivos. O documento antigo sai antes de a DLL mudar e o novo entra por último: o lançador nunca vê
// documento de uma versão ao lado da DLL de outra.
inline bool instalar(const std::wstring& pasta_do_programa, const std::string& documento, const std::string& assinatura_hex,
                     const std::wstring& dll_baixada, std::string& motivo)
{
    const std::string  build = build_do_pacote(pasta_do_programa);
    const std::wstring pasta = build.empty() ? std::wstring() : pasta_da_correcao(build);
    Documento          d;
    std::string        sha;
    if (pasta.empty())
        motivo = "pacote sem identidade";
    else if (!assinatura_confere(documento, sem_espacos_no_fim(assinatura_hex)))
        motivo = "assinatura inválida";
    else if (!ler_documento(documento, d))
        motivo = "documento ilegível";
    else if (d.build != build)
        motivo = "correção de outra compilação";
    else if (hoje_utc() > d.validade)
        motivo = "correção vencida";
    else if (!sha256_do_arquivo(dll_baixada, sha) || sha != d.dll_sha256)
        motivo = "a DLL baixada não é a assinada";
    if (!motivo.empty())
        return false;

    criar_pastas(pasta.substr(0, pasta.size() - 1));
    limpar_velhas(pasta);
    DeleteFileW((pasta + L"correcao.txt").c_str());
    DeleteFileW((pasta + L"correcao.sig").c_str());
    DeleteFileW((pasta + L"tentativas").c_str());
    const std::wstring alvo = pasta + L"OrcaSlicer.dll";
    if (GetFileAttributesW(alvo.c_str()) != INVALID_FILE_ATTRIBUTES && !DeleteFileW(alvo.c_str())) {
        // a correção anterior está em uso por este programa: o Windows deixa trocar o nome, e ela some na próxima vez
        const std::wstring velha = alvo + L".velha-" + std::to_wstring(GetTickCount());
        if (!MoveFileExW(alvo.c_str(), velha.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            motivo = "não deu para afastar a correção anterior";
            return false;
        }
    }
    if (!MoveFileExW(dll_baixada.c_str(), alvo.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
        motivo = "não deu para gravar a DLL corrigida";
        return false;
    }
    if (!gravar_arquivo_pequeno(pasta + L"correcao.sig", sem_espacos_no_fim(assinatura_hex)) ||
        !gravar_arquivo_pequeno(pasta + L"correcao.txt", documento)) {
        motivo = "não deu para gravar o documento da correção";
        return false;
    }
    return true;
}

} // namespace filuna_correcao

#endif // _WIN32
