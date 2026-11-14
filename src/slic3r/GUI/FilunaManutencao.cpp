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
#endif

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace Slic3r { namespace GUI { namespace FilunaManutencao {

namespace {

using nlohmann::json;
namespace F  = Slic3r::Filuna;
namespace fs = boost::filesystem;

const char* const SITE  = "https://filuna3d.com";
const char* const CHAVE = "54e050b44876dfb8738a9913aa323f89b4df4cc20813ba8d7aa8e223706a43b1"; // mesma do catálogo
const size_t      PACOTE_MAX = 600u * 1024u * 1024u;

std::mutex  g_trava;
json        g_estado = {{"situacao", "ocioso"}}; // ocioso | verificando | atual | disponivel | baixando | aplicando | erro
json        g_doc;                               // documento assinado da atualização disponível
std::string g_token;
std::atomic<bool> g_iniciado{false}, g_ocupado{false};

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

void verificar_agora()
{
    const std::string token = token_atual();
    if (token.empty() || g_ocupado.exchange(true))
        return;
    std::thread([token] {
        situacao("verificando");
        std::string corpo, erro;
        unsigned    status = 0;
        Http http = Http::get(std::string(SITE) + "/api/slicer/v1/app/atualizacao?build=" + Http::url_encode(identidade().value("build", std::string())));
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
        {
            std::lock_guard<std::mutex> g(g_trava);
            g_doc = doc;
        }
        g_ocupado = false;
        situacao("disponivel");
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
bool lancar_aplicacao(const fs::path& arquivos, const json& remover, std::string& erro)
{
    const fs::path destino = pasta_do_programa();
    const fs::path exe     = fs::path(wxStandardPaths::Get().GetExecutablePath().ToUTF8().data());
    const fs::path script  = pasta_espera() / "aplicar.cmd";
    const fs::path falhou  = pasta_espera() / "falhou.txt";
    std::ostringstream s;
    s << "@echo off\r\nchcp 65001 >nul\r\nset /a N=0\r\n:espera\r\n"
      << "tasklist /FI \"PID eq " << ::GetCurrentProcessId() << "\" 2>nul | find \"" << ::GetCurrentProcessId() << "\" >nul\r\n"
      << "if not errorlevel 1 (\r\n  set /a N+=1\r\n  if %N% GEQ 900 goto desistiu\r\n  ping -n 2 127.0.0.1 >nul\r\n  goto espera\r\n)\r\n"
      << "robocopy \"" << arquivos.string() << "\" \"" << destino.string() << "\" /E /R:8 /W:1 /NFL /NDL /NJH /NJS /NP >nul\r\n"
      << "if errorlevel 8 goto falhou\r\n";
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

void aplicar_agora()
{
    json doc;
    {
        std::lock_guard<std::mutex> g(g_trava);
        doc = g_doc;
    }
    const std::string token = token_atual();
    if (doc.is_null() || token.empty() || g_ocupado.exchange(true))
        return;
    std::thread([doc, token] {
        auto falha = [&](const std::string& motivo) {
            g_ocupado = false;
            diagnostico("atualizacao.aplicar", motivo, {{"para", doc.value("para", std::string())}});
            situacao("erro", {{"erro", motivo}});
        };
#ifndef _WIN32
        falha("A atualização automática do programa vale para Windows nesta versão.");
        return;
#else
        try {
            const std::string para = doc.value("para", std::string()), arquivo = doc["pacote"].value("arquivo", std::string()),
                              sha = doc["pacote"].value("sha256", std::string());
            if (!F::Recursos::versao_valida(para) || arquivo.find('/') != std::string::npos || arquivo.find("..") != std::string::npos)
                return falha("documento de atualização inválido");
            const fs::path base = pasta_espera() / para, zip = base / "pacote.zip", arquivos = base / "arquivos";
            boost::system::error_code ec;
            fs::remove_all(pasta_espera(), ec);
            fs::create_directories(arquivos);
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
                        situacao("baixando", {{"feito", pct}});
                    }
                })
                .on_complete([&](std::string b, unsigned s) { corpo = std::move(b), status = s; })
                .on_error([&](std::string b, std::string e, unsigned s) { status = s, erro = e.empty() ? "HTTP " + std::to_string(s) : e; })
                .perform_sync();
            if (status != 200)
                return falha("download da atualização recusado (" + (erro.empty() ? std::to_string(status) : erro) + ")");
            if (corpo.size() > PACOTE_MAX)
                return falha("pacote de atualização grande demais");
            {
                fs::ofstream o(zip, std::ios::binary | std::ios::trunc);
                o.write(corpo.data(), std::streamsize(corpo.size()));
                if (!o.good())
                    return falha("não gravou o pacote da atualização no disco");
            }
            if (F::sha256_de_arquivo(zip.string()) != sha)
                return falha("SHA-256 do pacote da atualização não confere");
            situacao("aplicando", {{"feito", 100}});
            if (!extract_archive_confined(zip.string(), arquivos.string()))
                return falha("não extraiu o pacote da atualização");
            fs::remove(zip, ec);
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
            std::this_thread::sleep_for(std::chrono::hours(3));
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
