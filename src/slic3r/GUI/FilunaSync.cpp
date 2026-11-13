#include "FilunaSync.hpp"

#include "slic3r/Filuna/FilunaDb.hpp"
#include "slic3r/Utils/Http.hpp"

#include <boost/log/trivial.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace Slic3r { namespace GUI {

namespace {

using nlohmann::json;
using Clock = std::chrono::steady_clock;

const char* const API          = "https://filuna3d.com/api/slicer/v1/sync";
constexpr auto    ESPERA       = std::chrono::seconds(5);   // agrupa rajadas de gravação
constexpr auto    PERIODO      = std::chrono::minutes(5);   // mesmo sem mudança, confere o servidor
constexpr int     LOTE_ENVIO   = 200;
constexpr int     LOTE_RECEBER = 500;

struct Resposta
{
    unsigned    status = 0;
    std::string corpo, erro;
};

Resposta chamar(Http http, const std::string& token)
{
    Resposta r;
    http.tls_verify(true);
    http.timeout_connect(10);
    http.timeout_max(60);
    http.header("Authorization", "Bearer " + token);
    http.header("Accept", "application/json");
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

struct FilunaSync::Impl
{
    std::string                         caminho;
    std::function<void()>               ao_receber;
    std::unique_ptr<Filuna::FilunaDb>   db; // conexão só desta thread
    std::thread                         thread;
    mutable std::mutex                  trava;
    std::condition_variable             cv;
    std::string                         token;
    Clock::time_point                   proxima = Clock::now();
    bool                                sair    = false;
    Estado                              estado;

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
            sincronizar(conta);
            lk.lock();
        }
        lk.unlock();
        // Última tentativa de envio ao fechar (o tempo de espera é controlado em parar()).
        std::string conta;
        {
            std::lock_guard<std::mutex> g(trava);
            conta = token;
        }
        if (!conta.empty())
            enviar(conta);
    }

    void registrar(const std::string& erro)
    {
        std::lock_guard<std::mutex> g(trava);
        estado.ultimo_erro = erro;
        if (erro.empty())
            estado.ultimo_ok = Filuna::FilunaDb::agora_utc();
        if (db)
            estado.pendentes = db->pendentes_sincronizacao();
    }

    void sincronizar(const std::string& conta)
    {
        if (conta.empty())
            return;
        try {
            if (!db)
                db = Filuna::FilunaDb::abrir(caminho);
            std::string erro = enviar(conta);
            if (erro.empty())
                erro = receber(conta);
            registrar(erro);
            if (!erro.empty())
                BOOST_LOG_TRIVIAL(warning) << "Filuna: sincronização: " << erro;
        } catch (const std::exception& e) {
            registrar(e.what());
            BOOST_LOG_TRIVIAL(error) << "Filuna: sincronização: " << e.what();
        }
    }

    // Envia a fila em lotes. Devolve o erro (vazio = tudo enviado).
    std::string enviar(const std::string& conta)
    {
        if (!db)
            return {};
        const std::string pc = db->id_do_computador();
        for (;;) {
            const auto fila = db->fila_para_enviar(LOTE_ENVIO);
            if (fila.empty())
                return {};
            // A linha já é JSON: monta o corpo sem reinterpretar os dados.
            std::string corpo = "{\"pc_id\":" + json(pc).dump() + ",\"itens\":[";
            std::vector<std::int64_t> seqs;
            for (const auto& i : fila) {
                if (i.payload_json != "null") { // registro apagado antes de subir: nada a mandar
                    corpo += (seqs.empty() ? "" : ",") + std::string("{\"tabela\":") + json(i.tabela).dump() +
                             ",\"id\":" + json(i.id).dump() + ",\"dados\":" + i.payload_json + "}";
                }
                seqs.push_back(i.seq);
            }
            corpo += "]}";
            Http http = Http::post(std::string(API) + "/enviar");
            http.header("Content-Type", "application/json");
            http.set_post_body(corpo);
            const Resposta r = chamar(std::move(http), conta);
            if (r.status == 401)
                return "conta desconectada: entre de novo";
            if (r.status != 200)
                return r.erro.empty() ? "envio recusado (" + std::to_string(r.status) + ")" : r.erro;
            try {
                const json j = json::parse(r.corpo);
                for (const auto& x : j.value("recusados", json::array()))
                    BOOST_LOG_TRIVIAL(warning) << "Filuna: servidor recusou " << x.dump();
            } catch (const std::exception&) {}
            // Recusados também saem da fila: são linhas que o servidor nunca vai aceitar (ficam no log).
            db->marcar_enviados(seqs);
        }
    }

    std::string receber(const std::string& conta)
    {
        const std::string pc = db->id_do_computador();
        std::int64_t      rev = 0;
        try {
            rev = std::stoll(db->config_local("sync_rev").empty() ? "0" : db->config_local("sync_rev"));
        } catch (const std::exception&) {}
        for (int pagina = 0; pagina < 200; ++pagina) {
            const Resposta r = chamar(Http::get(std::string(API) + "/receber?desde=" + std::to_string(rev) +
                                                "&limite=" + std::to_string(LOTE_RECEBER) + "&pc_id=" + pc),
                                      conta);
            if (r.status == 401)
                return "conta desconectada: entre de novo";
            if (r.status != 200)
                return r.erro.empty() ? "recebimento recusado (" + std::to_string(r.status) + ")" : r.erro;
            json j;
            try {
                j = json::parse(r.corpo);
            } catch (const std::exception&) {
                return "resposta inválida do servidor";
            }
            std::vector<Filuna::FilunaDb::ItemRemoto> itens;
            for (const auto& i : j.value("itens", json::array()))
                if (i.contains("dados") && i["dados"].is_object())
                    itens.push_back({i.value("tabela", std::string()), i.value("id", std::string()), i["dados"].dump()});
            if (!itens.empty() && db->aplicar_remotos(itens) > 0 && ao_receber)
                ao_receber();
            rev = j.value("rev", rev);
            db->gravar_config_local("sync_rev", std::to_string(rev));
            {
                std::lock_guard<std::mutex> g(trava);
                estado.rev = rev;
            }
            if (!j.value("tem_mais", false))
                return {};
        }
        return {};
    }
};

FilunaSync& FilunaSync::instancia()
{
    static FilunaSync s;
    return s;
}

FilunaSync::~FilunaSync() { parar(); }

void FilunaSync::iniciar(const std::string& caminho_db, const std::string& token, std::function<void()> ao_receber)
{
    if (m)
        return;
    m             = new Impl();
    m->caminho    = caminho_db;
    m->ao_receber = std::move(ao_receber);
    m->token   = token;
    m->estado.ligado = !token.empty();
    m->proxima = Clock::now() + std::chrono::seconds(3); // ao abrir o programa, logo depois da tela inicial
    m->thread  = std::thread([this] { m->laco(); });
}

void FilunaSync::avisar(const std::string& token)
{
    if (!m)
        return;
    {
        std::lock_guard<std::mutex> g(m->trava);
        m->token         = token;
        m->estado.ligado = !token.empty();
        const auto daqui = Clock::now() + ESPERA;
        if (daqui < m->proxima)
            m->proxima = daqui;
    }
    m->cv.notify_all();
}

void FilunaSync::parar()
{
    if (!m || !m->thread.joinable())
        return;
    {
        std::lock_guard<std::mutex> g(m->trava);
        m->sair = true;
    }
    m->cv.notify_all();
    // Espera no máximo 3 segundos pelo último envio; depois deixa a thread terminar sozinha.
    auto feito  = std::make_shared<std::atomic<bool>>(false);
    std::thread t = std::move(m->thread);
    std::thread juntar([t = std::move(t), feito]() mutable {
        t.join();
        *feito = true;
    });
    for (int i = 0; i < 30 && !*feito; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (*feito)
        juntar.join();
    else
        juntar.detach();
}

FilunaSync::Estado FilunaSync::estado() const
{
    if (!m)
        return {};
    std::lock_guard<std::mutex> g(m->trava);
    return m->estado;
}

}} // namespace Slic3r::GUI
