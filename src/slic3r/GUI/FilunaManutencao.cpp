#include "FilunaManutencao.hpp"

#include "FilunaPanel.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "WebViewDialog.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/miniz_extension.hpp"
#include "slic3r/Filuna/FilunaCalculos.hpp"
#include "slic3r/Filuna/FilunaRecursos.hpp"
#include "slic3r/Utils/Http.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>

#include <wx/stdpaths.h>
#include <wx/utils.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <wx/msw/registry.h>
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#endif

#include <atomic>
#include <chrono>
#include <iterator>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

// Descompressão zstd que já vem no programa (biblioteca Blosc do OpenVDB, zstd 1.4.x embutido): aplica a diferença
// binária do update (zstd --patch-from) contra o arquivo instalado. Só a parte estável da API.
extern "C" {
struct ZSTD_DCtx_s;
typedef struct ZSTD_DCtx_s ZSTD_DCtx;
ZSTD_DCtx*  ZSTD_createDCtx(void);
size_t      ZSTD_freeDCtx(ZSTD_DCtx* dctx);
size_t      ZSTD_DCtx_setParameter(ZSTD_DCtx* dctx, int param, int value);
size_t      ZSTD_DCtx_refPrefix(ZSTD_DCtx* dctx, const void* prefix, size_t prefixSize);
size_t      ZSTD_decompressDCtx(ZSTD_DCtx* dctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);
unsigned    ZSTD_isError(size_t code);
const char* ZSTD_getErrorName(size_t code);
}

namespace Slic3r { namespace GUI { namespace FilunaManutencao {

namespace {

using nlohmann::json;
namespace F  = Slic3r::Filuna;
namespace fs = boost::filesystem;

const char* const SITE  = "https://filuna3d.com";
const char* const CHAVE = "54e050b44876dfb8738a9913aa323f89b4df4cc20813ba8d7aa8e223706a43b1"; // mesma do catálogo
const size_t      PACOTE_MAX = 600u * 1024u * 1024u;

std::mutex  g_trava;
std::string g_pronto_para;   // versão já baixada e extraída, à espera de aplicar
fs::path    g_pronto_em;
json        g_estado = {{"situacao", "ocioso"}}; // ocioso | verificando | atual | disponivel | baixando | aplicando | erro
json        g_doc;                               // documento assinado da atualização disponível
std::string g_token;
std::atomic<bool> g_iniciado{false}, g_ocupado{false};
std::atomic<bool> g_sem_patch{false}; // a diferença binária falhou nesta sessão: pede o pacote normal

// Instalado pela Microsoft Store (pacote MSIX): a pasta do programa é só leitura e a Store faz as atualizações,
// então o atualizador próprio e a entrada de "iniciar com o Windows" no registro ficam desligados.
bool via_microsoft_store()
{
#ifdef _WIN32
    static const bool loja = [] {
        using Fn = LONG(WINAPI*)(UINT32*, PWSTR);
        const HMODULE kernel = ::GetModuleHandleW(L"kernel32.dll");
        const auto    fn     = kernel ? reinterpret_cast<Fn>(::GetProcAddress(kernel, "GetCurrentPackageFullName")) : nullptr;
        if (fn == nullptr)
            return false;
        UINT32 tamanho = 0;
        const LONG r   = fn(&tamanho, nullptr);
        return r == ERROR_INSUFFICIENT_BUFFER || r == ERROR_SUCCESS; // APPMODEL_ERROR_NO_PACKAGE = programa comum
    }();
    return loja;
#else
    return false;
#endif
}

std::string texto_cfg(const std::string& chave)
{
    AppConfig* cfg = wxGetApp().app_config;
    return cfg != nullptr ? cfg->get(chave) : std::string();
}

fs::path pasta_do_programa()
{
    return fs::path(wxStandardPaths::Get().GetExecutablePath().ToUTF8().data()).parent_path();
}

fs::path pasta_espera() { return fs::path(data_dir()) / "filuna" / "atualizacao"; }

json identidade()
{
    static json id;
    static std::once_flag uma;
    std::call_once(uma, [] {
        id = {{"build", "sem-identidade"}, {"versao", std::string(SLIC3R_VERSION)}};
        try {
            fs::ifstream f(pasta_do_programa() / "filuna-build.json", std::ios::binary);
            if (f.good()) {
                json d = json::parse(f, nullptr, false);
                if (d.is_object() && d.value("build", std::string()).size() > 0) {
                    id["build"]  = d.value("build", std::string());
                    id["versao"] = d.value("versao", id.value("versao", std::string()));
                }
            }
        } catch (const std::exception&) {}
    });
    return id;
}

// Fim do registro do programa (data_dir/log, arquivo mais novo), para o diagnóstico.
std::string fim_do_registro(size_t maximo)
{
    try {
        const fs::path dir = fs::path(data_dir()) / "log";
        if (!fs::exists(dir))
            return {};
        fs::path    novo;
        std::time_t quando = 0;
        for (auto& it : fs::directory_iterator(dir))
            if (fs::is_regular_file(it.path())) {
                const std::time_t t = fs::last_write_time(it.path());
                if (t >= quando)
                    quando = t, novo = it.path();
            }
        if (novo.empty())
            return {};
        fs::ifstream f(novo, std::ios::binary);
        f.seekg(0, std::ios::end);
        const std::streamoff tam = f.tellg();
        const std::streamoff de  = tam > std::streamoff(maximo) ? tam - std::streamoff(maximo) : 0;
        f.seekg(de);
        std::string s(size_t(tam - de), '\0');
        f.read(&s[0], std::streamsize(s.size()));
        // só texto: bytes inválidos em UTF-8 quebrariam o JSON
        for (char& c : s)
            if (static_cast<unsigned char>(c) < 9 || static_cast<unsigned char>(c) > 126)
                c = (c == '\n' || c == '\r' || c == '\t') ? c : '?';
        return s;
    } catch (const std::exception&) {
        return {};
    }
}

void avisar_telas()
{
    json e;
    {
        std::lock_guard<std::mutex> g(g_trava);
        e = g_estado;
    }
    const json id = identidade();
    e["build"]  = id.value("build", std::string());
    e["versao"] = id.value("versao", std::string());
    wxGetApp().CallAfter([e] {
        FilunaPanel::broadcast("atualizacao.estado", e);
        if (WebViewPanel* home = WebViewPanel::if_built())
            home->RunScript(wxString::FromUTF8("if (window.FlManutencao) FlManutencao(" + json({{"tipo", "estado"}, {"estado", e}}).dump() + ");"));
    });
}

void situacao(const std::string& s, const json& mais = json::object())
{
    {
        std::lock_guard<std::mutex> g(g_trava);
        json novo = mais.is_object() ? mais : json::object();
        novo["situacao"] = s;
        if (!g_doc.is_null()) {
            novo["nova_versao"] = g_doc.value("versao", std::string());
            novo["notas"]       = g_doc.value("notas", json::array());
            novo["internos"]    = g_doc.value("internos", json::array());
            novo["tamanho"]     = g_doc.contains("pacote") ? g_doc["pacote"].value("bytes", 0ull) : 0ull;
            novo["tipo"]        = g_doc.value("tipo", std::string());
        }
        g_estado = novo;
    }
    avisar_telas();
}

std::string token_atual()
{
    std::lock_guard<std::mutex> g(g_trava);
    return g_token;
}

void preparar_em_segundo_plano();

void verificar_agora()
{
    if (via_microsoft_store())
        return situacao("loja");
    const std::string token = token_atual();
    if (token.empty() || g_ocupado.exchange(true))
        return;
    std::thread([token] {
        situacao("verificando");
        std::string corpo, erro;
        unsigned    status = 0;
        Http http = Http::get(std::string(SITE) + "/api/slicer/v1/app/atualizacao?build=" + Http::url_encode(identidade().value("build", std::string())) +
                              (g_sem_patch ? "" : "&patch=zstd"));
        http.tls_verify(true);
        http.timeout_connect(10);
        http.timeout_max(60);
        http.header("Authorization", "Bearer " + token)
            .on_complete([&](std::string b, unsigned s) { corpo = std::move(b), status = s; })
            .on_error([&](std::string b, std::string e, unsigned s) { corpo = std::move(b), status = s, erro = e.empty() ? "HTTP " + std::to_string(s) : e; })
            .perform_sync();
        json r = json::parse(corpo, nullptr, false);
        if (status != 200 || !r.is_object()) {
            g_ocupado = false;
            situacao("ocioso"); // sem rede ou sem conta: não é erro para mostrar
            return;
        }
        if (r.value("atual", true)) {
            {
                std::lock_guard<std::mutex> g(g_trava);
                g_doc = json();
            }
            g_ocupado = false;
            situacao("atual");
            return;
        }
        const std::string texto = r.value("documento", std::string()), assinatura = r.value("assinatura", std::string());
        json doc = json::parse(texto, nullptr, false);
        const std::string meu = identidade().value("build", std::string());
        if (!F::Recursos::verificar_ed25519(texto, assinatura, CHAVE) || !doc.is_object() ||
            (doc.value("de", std::string()) != "*" && doc.value("de", std::string()) != meu) || !doc.contains("pacote")) {
            g_ocupado = false;
            diagnostico("atualizacao.verificar", "documento de atualização recusado (assinatura ou origem)");
            situacao("ocioso");
            return;
        }
        bool ja_pronto = false;
        {
            std::lock_guard<std::mutex> g(g_trava);
            g_doc = doc;
            ja_pronto = !g_pronto_para.empty() && g_pronto_para == doc.value("para", std::string());
        }
        g_ocupado = false;
        if (ja_pronto) {
            situacao("pronto"); // já baixada nesta sessão: só falta aplicar
            return;
        }
        situacao("disponivel");
        preparar_em_segundo_plano();
    }).detach();
}

#ifdef _WIN32
bool pasta_gravavel(const fs::path& p)
{
    boost::system::error_code ec;
    const fs::path t = p / ".filuna-teste-escrita";
    {
        fs::ofstream o(t, std::ios::binary | std::ios::trunc);
        if (!o.good())
            return false;
        o << "x";
    }
    fs::remove(t, ec);
    return true;
}

// Script que espera o programa fechar, copia os arquivos novos por cima, apaga os removidos e abre de novo.
// Outras janelas do Filuna abertas a partir desta mesma pasta (o executável é o mesmo arquivo): enquanto existirem, os
// arquivos do programa ficam travados e a atualização não consegue trocá-los.
int outras_janelas_do_programa()
{
    wchar_t meu[MAX_PATH] = {};
    if (::GetModuleFileNameW(nullptr, meu, MAX_PATH) == 0)
        return 0;
    const DWORD eu   = ::GetCurrentProcessId();
    HANDLE      foto = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (foto == INVALID_HANDLE_VALUE)
        return 0;
    int            n = 0;
    PROCESSENTRY32W p{};
    p.dwSize = sizeof(p);
    for (BOOL ok = ::Process32FirstW(foto, &p); ok; ok = ::Process32NextW(foto, &p)) {
        if (p.th32ProcessID == eu || _wcsicmp(p.szExeFile, fs::path(meu).filename().wstring().c_str()) != 0)
            continue;
        HANDLE h = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, p.th32ProcessID);
        if (h == nullptr)
            continue;
        wchar_t caminho[MAX_PATH] = {};
        DWORD   tam               = MAX_PATH;
        if (::QueryFullProcessImageNameW(h, 0, caminho, &tam) && _wcsicmp(caminho, meu) == 0)
            ++n;
        ::CloseHandle(h);
    }
    ::CloseHandle(foto);
    return n;
}

bool lancar_aplicacao(const fs::path& arquivos, const json& remover, std::string& erro)
{
    if (const int outras = outras_janelas_do_programa(); outras > 0) {
        erro = outras == 1 ? "há outra janela do Filuna Slicer aberta: feche-a e clique de novo em Atualizar" :
                             "há outras janelas do Filuna Slicer abertas: feche-as e clique de novo em Atualizar";
        return false;
    }
    const fs::path destino = pasta_do_programa();
    const fs::path exe     = fs::path(wxStandardPaths::Get().GetExecutablePath().ToUTF8().data());
    const fs::path script  = pasta_espera() / "aplicar.cmd";
    const fs::path falhou  = pasta_espera() / "falhou.txt";
    std::ostringstream s;
    s << "@echo off\r\nchcp 65001 >nul\r\nset /a N=0\r\n:espera\r\n"
      << "tasklist /FI \"PID eq " << ::GetCurrentProcessId() << "\" 2>nul | find \"" << ::GetCurrentProcessId() << "\" >nul\r\n"
      << "if not errorlevel 1 (\r\n  set /a N+=1\r\n  if %N% GEQ 900 goto desistiu\r\n  ping -n 2 127.0.0.1 >nul\r\n  goto espera\r\n)\r\n"
      << "set /a T=0\r\n:copia\r\n"
      << "robocopy \"" << arquivos.string() << "\" \"" << destino.string() << "\" /E /R:3 /W:1 /XF filuna-build.json /NFL /NDL /NJH /NJS /NP >nul\r\n"
      << "if not errorlevel 8 goto copiou\r\n"
      << "set /a T+=1\r\nif %T% GEQ 200 goto falhou\r\nping -n 3 127.0.0.1 >nul\r\ngoto copia\r\n"
      << ":copiou\r\n"
      // a versão nova só é marcada depois que todos os arquivos dela estão no lugar
      << "if exist \"" << (arquivos / "filuna-build.json").string() << "\" copy /y \"" << (arquivos / "filuna-build.json").string() << "\" \""
      << (destino / "filuna-build.json").string() << "\" >nul\r\n";
    for (const auto& r : remover)
        if (r.is_string() && r.get<std::string>().find("..") == std::string::npos) {
            std::string rel = r.get<std::string>();
            for (char& c : rel)
                if (c == '/')
                    c = '\\';
            s << "del /f /q \"" << (destino / rel).string() << "\" >nul 2>nul\r\n";
        }
    s << "rmdir /s /q \"" << arquivos.parent_path().string() << "\" >nul 2>nul\r\n"
      << "start \"\" \"" << exe.string() << "\"\r\nexit /b 0\r\n"
      << ":falhou\r\necho copia >\"" << falhou.string() << "\"\r\nstart \"\" \"" << exe.string() << "\"\r\nexit /b 1\r\n"
      << ":desistiu\r\necho espera >\"" << falhou.string() << "\"\r\nexit /b 2\r\n";
    {
        fs::ofstream o(script, std::ios::binary | std::ios::trunc);
        o << s.str();
        if (!o.good()) {
            erro = "não gravou o roteiro de aplicação";
            return false;
        }
    }
    const std::wstring param = L"/c \"\"" + script.wstring() + L"\"\"";
    // Pasta do programa protegida (Arquivos de Programas): pede a permissão do Windows uma vez.
    const wchar_t* verbo = pasta_gravavel(destino) ? L"open" : L"runas";
    const HINSTANCE h = ::ShellExecuteW(nullptr, verbo, L"cmd.exe", param.c_str(), nullptr, SW_HIDE);
    if (reinterpret_cast<INT_PTR>(h) <= 32) {
        erro = "o Windows não autorizou aplicar a atualização";
        return false;
    }
    return true;
}
#endif

std::string ler_tudo(const fs::path& p)
{
    fs::ifstream i(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(i), std::istreambuf_iterator<char>());
}

// Pacote reduzido (doc "patches"): o arquivo grande veio como <nome>.zpatch, a diferença zstd contra o arquivo que
// está instalado. Refaz o arquivo novo na pasta de espera e confere o SHA-256; qualquer diferença = falha.
bool aplicar_patches(const json& doc, const fs::path& arquivos, std::string& motivo)
{
    if (!doc.contains("patches") || !doc["patches"].is_object())
        return true;
    for (const auto& [rel, info] : doc["patches"].items()) {
        if (rel.empty() || rel.find("..") != std::string::npos || rel[0] == '/' || rel[0] == '\\' || !info.is_object()) {
            motivo = "diferença binária com caminho inválido";
            return false;
        }
        const fs::path base = pasta_do_programa() / fs::path(rel), patch = arquivos / fs::path(rel + ".zpatch"), destino = arquivos / fs::path(rel);
        const size_t   n    = info.value("bytes", size_t(0));
        if (n == 0 || n > PACOTE_MAX * 2) {
            motivo = "diferença binária com tamanho inválido";
            return false;
        }
        if (F::sha256_de_arquivo(base.string()) != info.value("base_sha256", std::string())) {
            motivo = "o arquivo instalado " + rel + " não é o da versão esperada";
            return false;
        }
        const std::string velho = ler_tudo(base), dif = ler_tudo(patch);
        if (velho.empty() || dif.empty()) {
            motivo = "não leu " + rel + " ou a diferença dele";
            return false;
        }
        std::string novo(n, '\0');
        ZSTD_DCtx*  d = ZSTD_createDCtx();
        if (d == nullptr) {
            motivo = "sem memória para a diferença binária";
            return false;
        }
        ZSTD_DCtx_setParameter(d, 100 /* ZSTD_d_windowLogMax */, 31);
        size_t r = ZSTD_DCtx_refPrefix(d, velho.data(), velho.size());
        if (!ZSTD_isError(r))
            r = ZSTD_decompressDCtx(d, &novo[0], novo.size(), dif.data(), dif.size());
        ZSTD_freeDCtx(d);
        if (ZSTD_isError(r) || r != n) {
            motivo = "a diferença binária de " + rel + " não aplicou (" + (ZSTD_isError(r) ? std::string(ZSTD_getErrorName(r)) : "tamanho") + ")";
            return false;
        }
        {
            boost::system::error_code ec;
            fs::create_directories(destino.parent_path(), ec);
            fs::ofstream o(destino, std::ios::binary | std::ios::trunc);
            o.write(novo.data(), std::streamsize(novo.size()));
            if (!o.good()) {
                motivo = "não gravou " + rel + " refeito";
                return false;
            }
        }
        if (F::sha256_de_arquivo(destino.string()) != info.value("sha256", std::string())) {
            motivo = "SHA-256 de " + rel + " refeito não confere";
            return false;
        }
        boost::system::error_code ec;
        fs::remove(patch, ec);
    }
    return true;
}

// Baixa, confere e extrai o pacote da atualização numa pasta de espera. Não fecha nem troca nada.
bool baixar_e_extrair(const json& doc, const std::string& token, fs::path& arquivos, std::string& motivo, bool mostrar)
{
    const std::string para = doc.value("para", std::string()), arquivo = doc["pacote"].value("arquivo", std::string()),
                      sha = doc["pacote"].value("sha256", std::string());
    if (!F::Recursos::versao_valida(para) || arquivo.find('/') != std::string::npos || arquivo.find("..") != std::string::npos) {
        motivo = "documento de atualização inválido";
        return false;
    }
    const fs::path base = pasta_espera() / para, zip = base / "pacote.zip";
    arquivos = base / "arquivos";
    boost::system::error_code ec;
    fs::remove_all(pasta_espera(), ec);
    fs::create_directories(arquivos);
    if (mostrar)
        situacao("baixando", {{"feito", 0}});
    std::string corpo, erro;
    unsigned    status = 0;
    int         ultimo = -1;
    Http http = Http::get(std::string(SITE) + "/api/slicer/v1/app/pacote/" + arquivo);
    http.tls_verify(true);
    http.timeout_connect(15);
    http.timeout_max(3600);
    http.header("Authorization", "Bearer " + token)
        .on_progress([&](Http::Progress p, bool&) {
            const int pct = p.dltotal > 0 ? int(p.dlnow * 100 / p.dltotal) : 0;
            if (pct != ultimo) {
                ultimo = pct;
                if (mostrar)
                    situacao("baixando", {{"feito", pct}});
            }
        })
        .on_complete([&](std::string b, unsigned st) { corpo = std::move(b), status = st; })
        .on_error([&](std::string b, std::string e, unsigned st) { status = st, erro = e.empty() ? "HTTP " + std::to_string(st) : e; })
        .perform_sync();
    if (status != 200) {
        motivo = "download da atualização recusado (" + (erro.empty() ? std::to_string(status) : erro) + ")";
        return false;
    }
    if (corpo.size() > PACOTE_MAX) {
        motivo = "pacote de atualização grande demais";
        return false;
    }
    {
        fs::ofstream o(zip, std::ios::binary | std::ios::trunc);
        o.write(corpo.data(), std::streamsize(corpo.size()));
        if (!o.good()) {
            motivo = "não gravou o pacote da atualização no disco";
            return false;
        }
    }
    if (F::sha256_de_arquivo(zip.string()) != sha) {
        motivo = "SHA-256 do pacote da atualização não confere";
        return false;
    }
    if (!extract_archive_confined(zip.string(), arquivos.string())) {
        motivo = "não extraiu o pacote da atualização";
        return false;
    }
    fs::remove(zip, ec);
    if (!aplicar_patches(doc, arquivos, motivo)) {
        g_sem_patch = true; // daqui em diante pede o pacote normal (só o que mudou, sem diferença binária)
        diagnostico("atualizacao.patch", motivo, {{"para", para}});
        motivo = "a atualização reduzida não deu certo (" + motivo + "); clique de novo para baixar a normal";
        return false;
    }
    return true;
}

// Achou atualização: baixa e deixa pronta com o programa aberto. Fechar fica só para a hora de aplicar.
void preparar_em_segundo_plano()
{
    json doc;
    {
        std::lock_guard<std::mutex> g(g_trava);
        doc = g_doc;
    }
    const std::string token = token_atual();
    if (via_microsoft_store() || doc.is_null() || token.empty() || g_ocupado.exchange(true))
        return;
    std::thread([doc, token] {
#ifdef _WIN32
        try {
            fs::path    arquivos;
            std::string motivo;
            if (baixar_e_extrair(doc, token, arquivos, motivo, false)) {
                {
                    std::lock_guard<std::mutex> g(g_trava);
                    g_pronto_para = doc.value("para", std::string());
                    g_pronto_em   = arquivos;
                }
                g_ocupado = false;
                situacao("pronto");
                return;
            }
            BOOST_LOG_TRIVIAL(warning) << "Filuna atualização: preparo em segundo plano falhou: " << motivo;
            if (g_sem_patch && doc.contains("patches")) { // a reduzida falhou: busca de novo o pacote normal
                g_ocupado = false;
                verificar_agora();
                return;
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "Filuna atualização: preparo em segundo plano: " << e.what();
        }
#endif
        g_ocupado = false;
        situacao("disponivel"); // não preparou: o aviso continua, e aplicar baixa na hora
    }).detach();
}

void aplicar_agora()
{
    json doc;
    {
        std::lock_guard<std::mutex> g(g_trava);
        doc = g_doc;
    }
    const std::string token = token_atual();
    if (via_microsoft_store() || doc.is_null() || token.empty() || g_ocupado.exchange(true))
        return;
    std::thread([doc, token] {
        auto falha = [&](const std::string& motivo) {
            g_ocupado = false;
            diagnostico("atualizacao.aplicar", motivo, {{"para", doc.value("para", std::string())}});
            situacao("erro", {{"erro", motivo}});
            if (g_sem_patch && doc.contains("patches")) { // a reduzida falhou: o "Tentar de novo" já usa a normal
                std::lock_guard<std::mutex> g(g_trava);
                g_doc = json();
            }
        };
#ifndef _WIN32
        falha("A atualização automática do programa vale para Windows nesta versão.");
        return;
#else
        try {
            fs::path arquivos;
            bool     pronto = false;
            {
                std::lock_guard<std::mutex> g(g_trava);
                pronto = !g_pronto_para.empty() && g_pronto_para == doc.value("para", std::string());
                if (pronto)
                    arquivos = g_pronto_em;
            }
            boost::system::error_code ec;
            if (!pronto || !fs::exists(arquivos, ec)) {
                std::string motivo;
                if (!baixar_e_extrair(doc, token, arquivos, motivo, true))
                    return falha(motivo);
            }
            situacao("aplicando", {{"feito", 100}});
            std::string motivo;
            if (!lancar_aplicacao(arquivos, doc.value("remover", json::array()), motivo))
                return falha(motivo);
            // o roteiro espera este programa fechar: fecha pela janela principal (pergunta de projeto não salvo vale)
            wxGetApp().CallAfter([] {
                if (wxGetApp().mainframe != nullptr)
                    wxGetApp().mainframe->Close(false);
            });
        } catch (const std::exception& e) {
            falha(std::string("falha ao aplicar a atualização: ") + e.what());
        }
#endif
    }).detach();
}

json preferencias()
{
    return {{"inicio_windows", texto_cfg("filuna_pref_inicio_windows")},
            {"manter_login", texto_cfg("filuna_pref_manter_login")},
            {"auto_dispositivos", texto_cfg("filuna_pref_auto_dispositivos")},
            {"primeira_ok", texto_cfg("filuna_pref_primeira_ok")}};
}

void aplicar_inicio_windows(bool ligar)
{
#ifdef _WIN32
    if (via_microsoft_store())
        return;
    wxRegKey k(wxRegKey::HKCU, "Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    if (!k.Exists() && !k.Create())
        return;
    if (ligar)
        k.SetValue("FilunaSlicer", "\"" + wxStandardPaths::Get().GetExecutablePath() + "\"");
    else if (k.HasValue("FilunaSlicer"))
        k.DeleteValue("FilunaSlicer");
#else
    (void) ligar;
#endif
}

void gravar_preferencias(const json& d)
{
    AppConfig* cfg = wxGetApp().app_config;
    if (cfg == nullptr)
        return;
    for (const char* k : {"inicio_windows", "manter_login", "auto_dispositivos"})
        if (d.contains(k))
            cfg->set(std::string("filuna_pref_") + k, d[k].is_boolean() ? (d[k].get<bool>() ? "1" : "0") : d.value(k, std::string()));
    cfg->set("filuna_pref_primeira_ok", "1");
    cfg->save();
    if (d.contains("inicio_windows"))
        aplicar_inicio_windows(cfg->get("filuna_pref_inicio_windows") == "1");
}

} // namespace

std::string build_local() { return identidade().value("build", std::string()); }

void diagnostico(const std::string& etapa, const std::string& mensagem, const json& contexto)
{
    BOOST_LOG_TRIVIAL(error) << "Filuna diagnóstico [" << etapa << "]: " << mensagem;
    static std::mutex                                                 trava;
    static std::map<std::string, std::chrono::steady_clock::time_point> vistos;
    {
        std::lock_guard<std::mutex> g(trava);
        const auto agora = std::chrono::steady_clock::now();
        auto&      v     = vistos[etapa + "|" + mensagem];
        if (v.time_since_epoch().count() != 0 && agora - v < std::chrono::seconds(60))
            return;
        v = agora;
        if (vistos.size() > 400)
            vistos.clear();
    }
    std::string token = token_atual();
    if (token.empty())
        token = texto_cfg("filuna_token");
    if (token.empty())
        return;
    const json id = identidade();
    json corpo = {{"build", id.value("build", std::string())}, {"versao", id.value("versao", std::string())},
                  {"so", std::string(wxGetOsDescription().ToUTF8().data())}, {"etapa", etapa}, {"mensagem", mensagem},
                  {"contexto", contexto.is_object() ? contexto : json::object()}};
    std::thread([corpo, token]() mutable {
        corpo["log"] = fim_do_registro(12000);
        Http http = Http::post(std::string(SITE) + "/api/slicer/v1/diagnostico");
        http.header("Authorization", "Bearer " + token).header("Content-Type", "application/json")
            .set_post_body(corpo.dump(-1, ' ', false, json::error_handler_t::replace))
            .timeout_connect(8).timeout_max(30)
            .on_error([](std::string, std::string erro, unsigned) { BOOST_LOG_TRIVIAL(debug) << "Filuna: diagnóstico não enviado: " << erro; })
            .perform_sync();
    }).detach();
}

void conta(const std::string& token)
{
    {
        std::lock_guard<std::mutex> g(g_trava);
        g_token = token;
    }
    if (!token.empty() && g_iniciado)
        verificar_agora();
}

void iniciar()
{
    if (g_iniciado.exchange(true))
        return;
    {
        std::lock_guard<std::mutex> g(g_trava);
        g_token = texto_cfg("filuna_token");
    }
    if (via_microsoft_store()) {
        situacao("loja");
        return;
    }
    // atualização anterior que não conseguiu copiar: avisa o servidor
    try {
        const fs::path falhou = pasta_espera() / "falhou.txt";
        if (fs::exists(falhou)) {
            boost::system::error_code ec;
            fs::remove(falhou, ec);
            diagnostico("atualizacao.aplicar", "a cópia dos arquivos da atualização falhou ou o programa não fechou");
        }
    } catch (const std::exception&) {}
    std::thread([] {
        std::this_thread::sleep_for(std::chrono::seconds(25));
        for (;;) {
            verificar_agora();
            std::this_thread::sleep_for(std::chrono::minutes(20)); // com o programa aberto, a atualização chega sem reabrir
        }
    }).detach();
}

bool acao(const std::string& nome, const json& d, json& resposta)
{
    if (nome == "diagnostico.enviar") {
        diagnostico("tela:" + d.value("etapa", std::string("sem etapa")), d.value("mensagem", std::string()),
                    d.contains("contexto") ? d["contexto"] : json::object());
        resposta = json::object();
        return true;
    }
    if (nome == "atualizacao.estado" || nome == "atualizacao.verificar" || nome == "atualizacao.aplicar") {
        if (nome == "atualizacao.verificar")
            verificar_agora();
        if (nome == "atualizacao.aplicar")
            aplicar_agora();
        std::lock_guard<std::mutex> g(g_trava);
        resposta           = g_estado;
        resposta["loja"]   = via_microsoft_store();
        resposta["build"]  = identidade().value("build", std::string());
        resposta["versao"] = identidade().value("versao", std::string());
        return true;
    }
    if (nome == "preferencias.ler") {
        resposta = preferencias();
        return true;
    }
    if (nome == "preferencias.gravar") {
        gravar_preferencias(d);
        resposta = preferencias();
        return true;
    }
    return false;
}

void comando_home(const std::string& corpo_json)
{
    json c = json::parse(corpo_json, nullptr, false);
    if (!c.is_object())
        return;
    const std::string nome = c.value("acao", std::string());
    json resposta;
    if (!acao(nome, c.contains("dados") && c["dados"].is_object() ? c["dados"] : json::object(), resposta))
        return;
    if (WebViewPanel* home = WebViewPanel::if_built())
        home->RunScript(wxString::FromUTF8("if (window.FlManutencao) FlManutencao(" +
                                           json({{"tipo", nome.rfind("preferencias", 0) == 0 ? "preferencias" : "estado"},
                                                 {nome.rfind("preferencias", 0) == 0 ? "preferencias" : "estado", resposta}}).dump() + ");"));
}

}}} // namespace Slic3r::GUI::FilunaManutencao
