#include "FilunaAtualizacao.hpp"

#include "FilunaPanel.hpp"
#include "GUI_App.hpp"

#include "libslic3r/miniz_extension.hpp"
#include "slic3r/Filuna/FilunaCalculos.hpp"
#include "slic3r/Filuna/FilunaDb.hpp"
#include "slic3r/Filuna/FilunaRecursos.hpp"
#include "slic3r/Utils/Http.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/log/trivial.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <set>
#include <thread>
#include <utility>

namespace Slic3r { namespace GUI {

namespace {

using nlohmann::json;
namespace F  = Slic3r::Filuna;
namespace fs = boost::filesystem;
using Clock  = std::chrono::steady_clock;

const char* const SITE = "https://filuna3d.com";
// Chave pública Ed25519 do catálogo de módulos (filuna-modulos-ed25519-1). Fixa no binário:
// a assinatura vale mesmo que o servidor seja trocado. A privada só existe em /root/.filuna-modulos.
const char* const CHAVE_CATALOGO = "54e050b44876dfb8738a9913aa323f89b4df4cc20813ba8d7aa8e223706a43b1";
constexpr auto    PRIMEIRA = std::chrono::seconds(20);  // depois da abertura, sem atrasar a tela
constexpr auto    PERIODO  = std::chrono::hours(6);     // guia-v1/03 §5
constexpr std::size_t PACOTE_MAX = std::size_t(256) << 20; // 256 MB por pacote é erro, não download

bool nome_de_arquivo_valido(const std::string& n)
{
    if (n.empty() || n.size() > 100 || n.find("..") != std::string::npos)
        return false;
    return std::all_of(n.begin(), n.end(), [](unsigned char c) { return std::isalnum(c) || c == '.' || c == '_' || c == '-'; });
}

struct Resposta
{
    unsigned    status = 0;
    std::string corpo, erro;
};

Resposta baixar(const std::string& url, const std::string& token)
{
    Resposta r;
    Http http = Http::get(url);
    http.tls_verify(true);
    http.timeout_connect(10);
    http.timeout_max(600); // um módulo pode ter dezenas de MB
    http.header("Authorization", "Bearer " + token);
    http.header("Accept", "*/*");
    http.on_complete([&r](std::string body, unsigned status) {
            r.status = status;
            r.corpo  = std::move(body);
        })
        .on_error([&r](std::string body, std::string error, unsigned status) {
            r.status = status;
            r.corpo  = std::move(body);
            r.erro   = error.empty() ? "HTTP " + std::to_string(status) : error;
        })
        .perform_sync();
    return r;
}

} // namespace

struct FilunaAtualizacao::Impl
{
    std::string             raiz;
    std::thread             thread;
    mutable std::mutex      trava;
    std::condition_variable cv;
    std::string             token;
    Clock::time_point       proxima = Clock::now();
    bool                    sair    = false;
    Estado                  estado;
    json                    catalogo;            // último catálogo verificado (assinatura ok)
    std::vector<std::string> visiveis;           // ids visíveis para a conta

    void laco()
    {
        std::unique_lock<std::mutex> lk(trava);
        while (!sair) {
            cv.wait_until(lk, proxima, [this] { return sair || Clock::now() >= proxima; });
            if (sair)
                break;
            proxima                 = Clock::now() + PERIODO;
            const std::string conta = token;
            lk.unlock();
            if (!conta.empty())
                verificar(conta);
            lk.lock();
        }
    }

    void registrar(const std::string& erro)
    {
        std::lock_guard<std::mutex> g(trava);
        estado.ultimo_erro = erro;
        if (erro.empty())
            estado.ultimo_ok = F::FilunaDb::agora_utc();
    }

    void verificar(const std::string& conta)
    {
        try {
            registrar(ciclo(conta));
        } catch (const std::exception& e) {
            registrar(e.what());
            BOOST_LOG_TRIVIAL(error) << "Filuna: atualização: " << e.what();
        }
    }

    // Devolve "" quando tudo deu certo (mesmo sem nada para baixar).
    std::string ciclo(const std::string& conta)
    {
        const Resposta r = baixar(std::string(SITE) + "/api/slicer/v1/modulos", conta);
        if (r.status == 401)
            return "conta desconectada: entre de novo";
        if (r.status != 200)
            return r.erro.empty() ? "catálogo indisponível (" + std::to_string(r.status) + ")" : r.erro;
        json resposta;
        try {
            resposta = json::parse(r.corpo);
        } catch (const std::exception&) {
            return "resposta inválida do servidor";
        }
        const std::string texto      = resposta.value("catalogo", std::string());
        const std::string assinatura = resposta.value("assinatura", std::string());
        if (!F::Recursos::verificar_ed25519(texto, assinatura, CHAVE_CATALOGO)) {
            BOOST_LOG_TRIVIAL(error) << "Filuna: atualização: a assinatura do catálogo não confere; nada será baixado";
            return "assinatura do catálogo não confere";
        }
        json catalogo;
        try {
            catalogo = json::parse(texto);
        } catch (const std::exception&) {
            return "catálogo ilegível";
        }
        std::set<std::string> visiveis;
        for (const auto& v : resposta.value("visiveis", json::array()))
            if (v.is_string())
                visiveis.insert(v.get<std::string>());
        {
            std::lock_guard<std::mutex> g(trava);
            this->catalogo = catalogo;
            this->visiveis.assign(visiveis.begin(), visiveis.end());
        }
        // cópia local para a tela de Dispositivos funcionar sem internet
        try {
            fs::create_directories(fs::path(raiz));
            boost::filesystem::ofstream out(fs::path(raiz) / "catalogo.json", std::ios::binary | std::ios::trunc);
            out << json({{"catalogo", catalogo}, {"visiveis", json(this->visiveis)}}).dump();
        } catch (const std::exception&) {}

        std::string primeiro_erro;
        for (const auto& m : catalogo.value("modulos", json::array())) {
            const std::string id = m.value("id", std::string()), versao = m.value("versao", std::string());
            if (visiveis.count(id) == 0)
                continue;
            if (!F::Recursos::id_valido(id) || !F::Recursos::versao_valida(versao)) {
                BOOST_LOG_TRIVIAL(warning) << "Filuna: atualização: id ou versão inválidos no catálogo: " << id << " " << versao;
                continue;
            }
            if (F::Recursos::versao_atual(raiz, id) == versao)
                continue;
            // Módulo de fabricante (motor CLI, 160–190 MB) só é baixado quando o cliente conecta a impressora em
            // Dispositivos ("Baixar complemento"); aqui, em segundo plano, apenas ATUALIZA o que já está instalado.
            // Recursos leves (telas, perfis de módulo já instalado) continuam automáticos.
            const bool pesado = std::any_of(m.value("componentes", json::array()).begin(), m.value("componentes", json::array()).end(),
                                            [](const json& c) { return c.value("tipo", std::string()) == "engine_cli"; });
            if (pesado && F::Recursos::versao_atual(raiz, id).empty())
                continue;
            const std::string erro = instalar(conta, m, id, versao);
            if (!erro.empty() && primeiro_erro.empty())
                primeiro_erro = id + ": " + erro;
        }
        return primeiro_erro;
    }

    // Baixa todos os componentes de um módulo para a pasta da versão, confere cada SHA-256,
    // extrai os pacotes e só então ativa. Qualquer falha: apaga a pasta e mantém o que está.
    std::string instalar(const std::string& conta, const json& m, const std::string& id, const std::string& versao,
                         const std::function<void(const json&)>& progresso = {})
    {
        const auto comps = m.value("componentes", json::array());
        int feito = 0;
        auto avisar = [&](const std::string& etapa) {
            if (progresso)
                progresso({{"etapa", etapa}, {"feito", feito}, {"total", int(comps.size()) + 1}, {"ok", false}});
        };
        avisar("Preparando");
        const fs::path destino = F::Recursos::dir_versao(raiz, id, versao);
        boost::system::error_code ec;
        fs::remove_all(destino, ec); // resto de uma tentativa anterior
        fs::create_directories(destino);
        auto falha = [&](const std::string& motivo) {
            boost::system::error_code e2;
            fs::remove_all(destino, e2);
            BOOST_LOG_TRIVIAL(error) << "Filuna: atualização de " << id << " " << versao << ": " << motivo;
            return motivo;
        };
        for (const auto& c : comps) {
            const std::string nome = c.value("arquivo", std::string());
            avisar("Baixando " + c.value("tipo", std::string("componente")));
            const std::string sha  = c.value("sha256", std::string());
            std::string       url  = c.value("url", std::string());
            if (!nome_de_arquivo_valido(nome) || sha.size() != 64)
                return falha("componente com nome ou SHA-256 inválidos");
            if (!url.empty() && url.front() == '/')
                url = SITE + url;
            if (url.rfind(std::string(SITE) + "/", 0) != 0)
                return falha("endereço de download fora do servidor Filuna");
            const Resposta r = baixar(url, conta);
            if (r.status != 200)
                return falha("download recusado (" + (r.erro.empty() ? std::to_string(r.status) : r.erro) + ")");
            if (r.corpo.size() > PACOTE_MAX)
                return falha("pacote grande demais");
            const fs::path arquivo = destino / nome;
            {
                boost::filesystem::ofstream out(arquivo, std::ios::binary | std::ios::trunc);
                out.write(r.corpo.data(), static_cast<std::streamsize>(r.corpo.size()));
                if (!out)
                    return falha("não gravou o pacote no disco");
            }
            if (F::sha256_de_arquivo(arquivo.string()) != sha)
                return falha("SHA-256 do pacote não confere");
            if (arquivo.extension() == ".zip") {
                if (!extract_archive_confined(arquivo.string(), destino.string()))
                    return falha("não extraiu o pacote");
                fs::remove(arquivo, ec);
            }
            ++feito;
        }
        avisar("Ativando");
        F::Recursos::ativar(raiz, id, versao);
        bool avisar_telas = false;
        {
            std::lock_guard<std::mutex> g(trava);
            estado.baixados.push_back(id + " " + versao);
            avisar_telas = !sair; // fechando o programa, não tocar na interface
        }
        BOOST_LOG_TRIVIAL(info) << "Filuna: atualização aplicada: " << id << " " << versao;
        if (avisar_telas)
            // Telas atualizadas valem na próxima abertura da página; avisa as que estão abertas.
            wxGetApp().CallAfter([] { FilunaPanel::broadcast("recursos"); });
        return {};
    }
};

json FilunaAtualizacao::catalogo() const
{
    if (!m)
        return json::object();
    std::lock_guard<std::mutex> g(m->trava);
    if (!m->catalogo.is_null())
        return m->catalogo;
    // ainda sem consulta nesta execução: usa a cópia local da última vez
    try {
        boost::filesystem::ifstream in(fs::path(m->raiz) / "catalogo.json", std::ios::binary);
        if (in) {
            json salvo = json::parse(in);
            if (salvo.contains("catalogo")) {
                m->catalogo = salvo["catalogo"];
                for (const auto& v : salvo.value("visiveis", json::array()))
                    if (v.is_string())
                        m->visiveis.push_back(v.get<std::string>());
                return m->catalogo;
            }
        }
    } catch (const std::exception&) {}
    return json::object();
}

std::vector<std::string> FilunaAtualizacao::visiveis() const
{
    if (!m)
        return {};
    (void) catalogo(); // garante a cópia local carregada
    std::lock_guard<std::mutex> g(m->trava);
    return m->visiveis;
}

std::string FilunaAtualizacao::versao_instalada(const std::string& id) const
{
    return m ? F::Recursos::versao_atual(m->raiz, id) : std::string();
}

bool FilunaAtualizacao::modulo_atualizado(const std::string& id) const
{
    const json cat = catalogo();
    for (const auto& mod : cat.value("modulos", json::array()))
        if (mod.value("id", std::string()) == id) {
            const std::string versao = mod.value("versao", std::string());
            // módulo sem pacote (ex.: Anycubic em modo Orca nativo) não tem o que baixar
            if (versao.empty() || mod.value("componentes", json::array()).empty())
                return true;
            return versao_instalada(id) == versao;
        }
    return false;
}

void FilunaAtualizacao::instalar_modulo(const std::string& id, std::function<void(const json&)> progresso)
{
    if (!m) {
        if (progresso)
            progresso({{"ok", false}, {"erro", "atualização não iniciada"}});
        return;
    }
    json alvo;
    std::string conta;
    {
        const json cat = catalogo();
        for (const auto& mod : cat.value("modulos", json::array()))
            if (mod.value("id", std::string()) == id)
                alvo = mod;
        std::lock_guard<std::mutex> g(m->trava);
        conta = m->token;
    }
    if (alvo.is_null()) {
        if (progresso)
            progresso({{"ok", false}, {"erro", "módulo não está no catálogo desta conta"}});
        return;
    }
    const std::string versao = alvo.value("versao", std::string());
    if (versao.empty() || alvo.value("componentes", json::array()).empty() || F::Recursos::versao_atual(m->raiz, id) == versao) {
        if (progresso)
            progresso({{"ok", true}, {"etapa", "Pronto"}, {"feito", 1}, {"total", 1}});
        return;
    }
    if (conta.empty()) {
        if (progresso)
            progresso({{"ok", false}, {"erro", "Entre na conta Filuna para baixar o complemento."}});
        return;
    }
    Impl* impl = m;
    std::thread([impl, conta, alvo, id, versao, progresso] {
        std::string erro;
        try {
            erro = impl->instalar(conta, alvo, id, versao, progresso);
        } catch (const std::exception& e) {
            erro = e.what();
        }
        if (progresso)
            progresso(erro.empty() ? json({{"ok", true}, {"etapa", "Pronto"}, {"feito", 1}, {"total", 1}})
                                   : json({{"ok", false}, {"erro", erro}}));
    }).detach();
}

FilunaAtualizacao& FilunaAtualizacao::instancia()
{
    static FilunaAtualizacao a;
    return a;
}

FilunaAtualizacao::~FilunaAtualizacao() { parar(); }

void FilunaAtualizacao::iniciar(const std::string& raiz, const std::string& token)
{
    if (m)
        return;
    m                = new Impl();
    m->raiz          = raiz;
    m->token         = token;
    m->estado.ligado = !token.empty();
    m->proxima       = Clock::now() + PRIMEIRA;
    m->thread        = std::thread([this] { m->laco(); });
}

void FilunaAtualizacao::avisar(const std::string& token)
{
    if (!m)
        return;
    {
        std::lock_guard<std::mutex> g(m->trava);
        m->token         = token;
        m->estado.ligado = !token.empty();
        const auto daqui = Clock::now() + std::chrono::seconds(5);
        if (daqui < m->proxima)
            m->proxima = daqui;
    }
    m->cv.notify_all();
}

void FilunaAtualizacao::parar()
{
    if (!m || !m->thread.joinable())
        return;
    {
        std::lock_guard<std::mutex> g(m->trava);
        m->sair = true;
    }
    m->cv.notify_all();
    m->thread.detach(); // um download no meio termina sozinho; nada fica pela metade (ativar é o último passo)
}

FilunaAtualizacao::Estado FilunaAtualizacao::estado() const
{
    if (!m)
        return {};
    std::lock_guard<std::mutex> g(m->trava);
    return m->estado;
}

}} // namespace Slic3r::GUI
