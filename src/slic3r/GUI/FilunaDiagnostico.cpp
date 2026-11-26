#include "FilunaDiagnostico.hpp"

#include "FilunaAoVivo.hpp"
#include "FilunaDispositivos.hpp"
#include "FilunaPanel.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "DeviceManager.hpp"
#include "DeviceCore/DevManager.h"
#include "libslic3r/AppConfig.hpp"
#include "slic3r/Utils/FilunaCameraUrls.hpp"
#include "slic3r/Utils/Http.hpp"

#include <boost/log/trivial.hpp>
#include <wx/notifmsg.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <ctime>
#include <future>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace Slic3r { namespace GUI { namespace FilunaDiagnostico {

namespace {

const char* const SITE = "https://filuna3d.com";

using Relogio = std::chrono::steady_clock;

// Limites fixos (não vêm de fora): ver o cabeçalho.
constexpr int    BICO_MAX      = 240;
constexpr int    MESA_MAX      = 70;
constexpr int    BICO_TESTE    = 200;
constexpr int    MESA_TESTE    = 60;
constexpr int    PRESENCA_MIN  = 20;  // minutos que a confirmação de presença vale
constexpr int    ESPERA_MIN    = 10;  // minutos que um pedido remoto espera a confirmação de presença
constexpr double SEM_LEITURA_S = 30.; // sem leitura nova da impressora por mais que isto: encerra o teste

bool moonraker(const std::string& ag) { return ag == "snapmaker" || ag == "qidi" || ag == "crealityprint" || ag == "moonraker"; }
bool bambu(const std::string& ag) { return ag == "filuna_bambu_lan" || ag == "bbl"; }
bool filuna_ctl(const std::string& ag) { return ag == "filuna_elegoo_cc2" || ag == "filuna_anycubic_lan"; }

long long agora_unix() { return (long long) std::time(nullptr); }

// Roda f na linha da tela e espera a resposta (até 8 s). As leituras do DeviceManager só são seguras lá.
template<class F> auto na_tela(F f) -> decltype(f())
{
    using R    = decltype(f());
    auto prom  = std::make_shared<std::promise<R>>();
    auto fut   = prom->get_future();
    wxGetApp().CallAfter([prom, f]() mutable {
        try {
            prom->set_value(f());
        } catch (...) {
            prom->set_exception(std::current_exception());
        }
    });
    if (fut.wait_for(std::chrono::seconds(8)) != std::future_status::ready)
        throw std::runtime_error("o programa não respondeu a tempo");
    return fut.get();
}

// Cartão atual da impressora (o mesmo da tela Dispositivo), lido na linha da tela.
json cartao(const std::string& dev_id)
{
    return na_tela([dev_id]() -> json {
        const json e = FilunaDispositivos::estado();
        for (const auto& c : e.value("impressoras", json::array()))
            if (c.value("dev_id", std::string()) == dev_id)
                return c;
        return json::object();
    });
}

// Manda um pedido no formato dos agentes: pela impressora aberta no programa ou pelo acompanhamento em segundo plano.
bool enviar(const std::string& dev_id, const json& pedido)
{
    return na_tela([dev_id, pedido]() -> bool {
        DeviceManager* dev = wxGetApp().getDeviceManager();
        MachineObject* sel = dev != nullptr ? dev->get_selected_machine() : nullptr;
        if (sel != nullptr && sel->get_dev_id() == dev_id)
            return sel->publish_json(pedido, 1) == 0;
        return FilunaAoVivo::enviar(dev_id, pedido.dump()) == 0;
    });
}

json gcode(const std::string& linhas) { return {{"print", {{"command", "gcode_line"}, {"param", linhas + "\n"}, {"sequence_id", "0"}}}}; }

// Pedidos de aquecimento e cooler no idioma de cada motor de rede.
bool aquecer(const std::string& dev, const std::string& ag, bool mesa, int alvo)
{
    alvo = std::max(0, std::min(alvo, mesa ? MESA_MAX : BICO_MAX));
    if (moonraker(ag))
        return enviar(dev, gcode(std::string("SET_HEATER_TEMPERATURE HEATER=") + (mesa ? "heater_bed" : "extruder") + " TARGET=" + std::to_string(alvo)));
    if (filuna_ctl(ag))
        return enviar(dev, {{"filuna", {{"command", mesa ? "mesa" : "bico"}, {"valor", alvo}}}});
    if (bambu(ag))
        return enviar(dev, gcode(std::string(mesa ? "M140 S" : "M104 S") + std::to_string(alvo)));
    return false;
}
bool cooler(const std::string& dev, const std::string& ag, int pct)
{
    pct = std::max(0, std::min(pct, 100));
    if (moonraker(ag))
        return enviar(dev, gcode("M106 S" + std::to_string(pct * 255 / 100)));
    if (filuna_ctl(ag))
        return enviar(dev, {{"filuna", {{"command", "ventilador"}, {"valor", pct}}}});
    if (bambu(ag))
        return enviar(dev, gcode("M106 P1 S" + std::to_string(pct * 255 / 100)));
    return false;
}
void desligar_tudo(const std::string& dev, const std::string& ag)
{
    try {
        aquecer(dev, ag, false, 0);
        aquecer(dev, ag, true, 0);
        cooler(dev, ag, 0);
    } catch (...) {
    }
}

// Leitura profunda do agente: pede e espera a resposta nova (Filuna::diag com "em" depois do pedido).
json leitura_profunda(const std::string& dev, const std::string& ag, bool so_console, int espera_s)
{
    const long long antes = agora_unix();
    json            pedido;
    if (bambu(ag))
        pedido = {{"info", {{"command", "get_version"}, {"sequence_id", "0"}}}};
    else
        pedido = {{"filuna", {{"command", "diagnostico"}, {"so_console", so_console}}}};
    if (!enviar(dev, pedido))
        return json::object();
    const bool tem_resposta = moonraker(ag) || bambu(ag) || ag == "filuna_prusalink" || ag == "filuna_elegoo_sdcp";
    if (!tem_resposta)
        return json::object();
    for (int i = 0; i < espera_s * 4; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        const json d = json::parse(Filuna::diag(dev), nullptr, false);
        if (d.is_object() && d.value("em", 0LL) >= antes)
            return d;
    }
    const json d = json::parse(Filuna::diag(dev), nullptr, false);
    return d.is_object() ? d : json::object();
}

// Respostas do console do Klipper depois do comando enviado (o gcode_store guarda comandos e respostas em ordem).
std::vector<std::string> respostas_depois(const json& console, const std::string& comando)
{
    std::vector<std::string> r;
    const json lista = console.is_object() ? console.value("gcode_store", json::array()) : json::array();
    int        inicio = -1;
    for (int i = int(lista.size()) - 1; i >= 0; --i)
        if (lista[i].is_object() && lista[i].value("type", std::string()) == "command" &&
            lista[i].value("message", std::string()).rfind(comando, 0) == 0) {
            inicio = i;
            break;
        }
    if (inicio < 0)
        return r;
    for (int i = inicio + 1; i < int(lista.size()); ++i) {
        if (!lista[i].is_object())
            continue;
        if (lista[i].value("type", std::string()) == "command")
            break;
        r.push_back(lista[i].value("message", std::string()));
    }
    return r;
}

std::string token()
{
    AppConfig* cfg = wxGetApp().app_config;
    return cfg != nullptr ? cfg->get("filuna_token") : std::string();
}

// Registra no Raio X da conta e devolve a análise do site (achados, testes sugeridos, comparação).
json registrar(const json& corpo)
{
    const std::string tk = token();
    if (tk.empty())
        return {{"aviso", "Entre na sua conta Filuna para guardar o diagnóstico no Raio X."}};
    json        resposta = json::object();
    std::string falha;
    Http        http = Http::post(std::string(SITE) + "/api/slicer/v1/diagnostico/registrar");
    http.header("Authorization", "Bearer " + tk)
        .header("Content-Type", "application/json")
        .set_post_body(corpo.dump(-1, ' ', false, json::error_handler_t::replace))
        .timeout_connect(5)
        .timeout_max(40)
        .on_complete([&](std::string body, unsigned) {
            const json j = json::parse(body, nullptr, false);
            if (j.is_object())
                resposta = j;
        })
        .on_error([&](std::string body, std::string erro, unsigned st) {
            const json j = json::parse(body, nullptr, false);
            falha        = j.is_object() && j.contains("erro") ? j["erro"].get<std::string>() : erro + " (" + std::to_string(st) + ")";
        })
        .perform_sync();
    if (!falha.empty())
        resposta["aviso"] = "O Raio X da conta não recebeu: " + falha;
    return resposta;
}

// ---------- catálogo de testes ----------

struct Teste
{
    const char* id;
    const char* nome;
    const char* risco; // leitura, aquece, move
    int         minutos;
    const char* explicacao;
};

const Teste CATALOGO[] = {
    {"consultas", "Sensores e fins de curso", "leitura", 1,
     "Pergunta à impressora o estado dos fins de curso, da sonda e dos drivers dos motores. Não aquece nem move."},
    {"ruido_acelerometro", "Ruído do acelerômetro", "leitura", 1,
     "Mede o ruído do sensor de vibração parado. Acima de 1.000 indica sensor, cabo ou cooler com problema."},
    {"aquecimento_bico", "Aquecimento do bico", "aquece", 5,
     "Aquece o bico até 200 °C, mede o tempo, a estabilidade e a força do aquecedor e desliga."},
    {"aquecimento_mesa", "Aquecimento da mesa", "aquece", 10, "Aquece a mesa até 60 °C, mede o tempo e a estabilidade e desliga."},
    {"coolers", "Cooler da peça", "aquece", 1, "Liga o cooler da peça em 50% e 100% e confere a rotação informada."},
    {"pid_bico", "Ajuste do aquecedor do bico (PID)", "aquece", 8,
     "Roda a calibração do aquecedor do bico a 200 °C e mostra os valores. Não grava nada na impressora."},
    {"malha_mesa", "Mapa da mesa", "move", 6,
     "Zera os eixos e mede a altura da mesa em vários pontos. A mesa precisa estar limpa e vazia."},
    {"precisao_sonda", "Precisão da sonda", "move", 4, "Zera os eixos e mede a mesma posição 10 vezes para ver se a sonda repete o valor."},
};

std::vector<std::string> ids_do_agente(const std::string& ag)
{
    if (moonraker(ag))
        return {"consultas", "ruido_acelerometro", "aquecimento_bico", "aquecimento_mesa", "coolers", "pid_bico", "malha_mesa", "precisao_sonda"};
    if (filuna_ctl(ag) || bambu(ag))
        return {"aquecimento_bico", "aquecimento_mesa", "coolers"};
    return {};
}

// Testes que só a tela da impressora roda (o fabricante não abre para programas): o Filuna orienta o cliente.
json guiados(const std::string& ag)
{
    if (ag == "filuna_prusalink")
        return json::array({"Na impressora: Menu › Controle › Calibrações e testes › Autoteste (coolers, eixos, aquecedores e célula de carga).",
                            "Menu › Controle › Calibrações e testes › Calibração do input shaper, se houver acelerômetro."});
    if (ag == "filuna_flashforge_lan")
        return json::array({"Na tela da impressora: Ajustes › Calibração › Nivelamento automático.",
                            "Ajustes › Calibração › Compensação de vibração."});
    if (ag == "filuna_elegoo_sdcp")
        return json::array({"Na tela da impressora: Ferramentas › Autoverificação (nivelamento, vibração e aquecedores)."});
    if (ag == "filuna_anycubic_lan")
        return json::array({"Na tela da impressora: Ajustes › Autoverificação (nivelamento automático, vibração e fluxo)."});
    if (bambu(ag))
        return json::array({"Na tela da impressora: Ajustes › Calibração (nivelamento da mesa, compensação de vibração e ruído do motor).",
                            "Os testes que aquecem pelo Filuna só funcionam com a impressora em Modo LAN + Modo Desenvolvedor."});
    if (ag == "filuna_elegoo_cc2")
        return json::array({"Na tela da impressora: Ajustes › Autoverificação."});
    return json::array();
}

const Teste* teste_por_id(const std::string& id)
{
    for (const auto& t : CATALOGO)
        if (id == t.id)
            return &t;
    return nullptr;
}

// ---------- estado do executor ----------

std::mutex& mtx()
{
    static std::mutex m;
    return m;
}
json& atual()
{
    static json j = json::object(); // {dev_id, teste, estado: esperando_presenca|rodando|concluido|falhou|interrompido, ...}
    return j;
}
std::map<std::string, Relogio::time_point>& presencas()
{
    static std::map<std::string, Relogio::time_point> m;
    return m;
}
std::atomic<bool>& parar_pedido()
{
    static std::atomic<bool> p{false};
    return p;
}

bool presente(const std::string& dev)
{
    std::lock_guard<std::mutex> l(mtx());
    auto                        it = presencas().find(dev);
    return it != presencas().end() && Relogio::now() - it->second < std::chrono::minutes(PRESENCA_MIN);
}

void publicar(const json& e)
{
    {
        std::lock_guard<std::mutex> l(mtx());
        atual() = e;
    }
    wxGetApp().CallAfter([e] { FilunaPanel::broadcast("diagnostico.teste", e); });
}

double num(const json& j, const char* k, double padrao = NAN)
{
    return j.is_object() && j.contains(k) && j[k].is_number() ? j[k].get<double>() : padrao;
}

// Uma amostra da impressora: temperaturas, alvo, potência e rotação (do cartão + medidas extras do agente).
struct Amostra
{
    double t = 0, temp = NAN, alvo = NAN, potencia = NAN;
    bool   ligada = false;
    std::string estado;
    json        extra;
};
Amostra amostrar(const std::string& dev, bool mesa, Relogio::time_point t0)
{
    Amostra    a;
    const json c = cartao(dev);
    a.t          = std::chrono::duration<double>(Relogio::now() - t0).count();
    a.ligada     = c.value("ligada", false);
    a.estado     = c.value("estado", std::string());
    a.extra      = c.value("extra", json::object());
    const json h = c.value(mesa ? "mesa" : "bico", json::object());
    a.temp       = num(h, "atual");
    a.alvo       = mesa ? num(a.extra, "mesa_alvo", num(h, "alvo")) : num(h, "alvo");
    const json pot = a.extra.value("potencia", json::object());
    a.potencia     = num(pot, mesa ? "heater_bed" : "extruder");
    return a;
}

std::string fmt1(double v)
{
    char b[32];
    std::snprintf(b, sizeof b, "%.1f", v);
    return b;
}

// Aquecimento (bico ou mesa): sobe até o alvo, mantém, mede e desliga. Regra de falha do próprio Klipper: o
// aquecedor tem de subir pelo menos 2 °C a cada 20 s até chegar perto do alvo.
json teste_aquecimento(const std::string& dev, const std::string& ag, bool mesa, json& e)
{
    const int  alvo      = mesa ? MESA_TESTE : BICO_TESTE;
    const int  limite_s  = mesa ? 600 : 300;
    const int  manter_s  = mesa ? 90 : 60;
    const auto t0        = Relogio::now();
    json       amostras  = json::array();
    Amostra    a0        = amostrar(dev, mesa, t0);
    if (std::isnan(a0.temp))
        return {{"resultado", "falha"}, {"texto", "A impressora não informa a temperatura deste aquecedor."}};
    if (!aquecer(dev, ag, mesa, alvo))
        return {{"resultado", "falha"}, {"texto", "A impressora não aceitou o comando de aquecer."}};
    double t_chegou = -1, t_ultimo_ok = 0, temp_ultimo_ok = a0.temp, maximo = a0.temp;
    double ultima_leitura = 0;
    bool   aceitou        = false;
    std::vector<double> mantendo, pot_mantendo;
    std::string falha;
    for (;;) {
        if (parar_pedido().load()) {
            falha = "interrompido";
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        Amostra a;
        try {
            a = amostrar(dev, mesa, t0);
        } catch (...) {
            a.t = std::chrono::duration<double>(Relogio::now() - t0).count();
        }
        if (!a.ligada || std::isnan(a.temp)) {
            if (a.t - ultima_leitura > SEM_LEITURA_S) {
                falha = "A impressora parou de responder durante o teste. Confira se ela está ligada e na rede.";
                break;
            }
            continue;
        }
        ultima_leitura = a.t;
        if (a.estado == "imprimindo") {
            falha = "A impressora começou a imprimir: teste encerrado.";
            break;
        }
        amostras.push_back({{"t", std::round(a.t)}, {"temp", std::round(a.temp * 10) / 10}, {"alvo", std::isnan(a.alvo) ? json() : json(a.alvo)},
                            {"potencia", std::isnan(a.potencia) ? json() : json(a.potencia)}});
        e["amostras"] = amostras;
        publicar(e);
        if (!aceitou && !std::isnan(a.alvo) && std::fabs(a.alvo - alvo) < 1.0)
            aceitou = true;
        if (!aceitou && a.t > 15) {
            falha = bambu(ag) ? "A impressora recusou o comando de aquecer. Na Bambu Lab isso só funciona com o Modo LAN + Modo Desenvolvedor."
                              : "A impressora não confirmou o alvo de temperatura.";
            break;
        }
        maximo = std::max(maximo, a.temp);
        if (t_chegou < 0) {
            if (a.temp >= alvo - 2) {
                t_chegou = a.t;
            } else if (a.temp - temp_ultimo_ok >= 2.0) {
                t_ultimo_ok = a.t, temp_ultimo_ok = a.temp;
            } else if (a.t - t_ultimo_ok > 20.0 && a.temp < alvo - 10) {
                falha = "O aquecedor não subiu 2 °C em 20 s (" + fmt1(a.temp) + " °C). Possível aquecedor, termistor ou cabo com defeito.";
                break;
            }
            if (a.t > limite_s) {
                falha = "Não chegou a " + std::to_string(alvo) + " °C em " + std::to_string(limite_s / 60) + " min (" + fmt1(a.temp) + " °C).";
                break;
            }
        } else {
            mantendo.push_back(a.temp);
            if (!std::isnan(a.potencia))
                pot_mantendo.push_back(a.potencia);
            if (a.t - t_chegou >= manter_s)
                break;
        }
    }
    aquecer(dev, ag, mesa, 0);
    json r = {{"alvo", alvo}, {"inicio", std::round(a0.temp * 10) / 10}, {"amostras", amostras}};
    if (!falha.empty()) {
        r["resultado"] = falha == "interrompido" ? "interrompido" : "falha";
        r["texto"]     = falha == "interrompido" ? "Teste interrompido. O aquecedor foi desligado." : falha;
        return r;
    }
    double media = 0, desvio = 0;
    for (double v : mantendo)
        media += v;
    media /= std::max<size_t>(1, mantendo.size());
    for (double v : mantendo)
        desvio += (v - media) * (v - media);
    desvio = std::sqrt(desvio / std::max<size_t>(1, mantendo.size()));
    r["tempo_ate_alvo_s"] = std::round(t_chegou);
    r["sobressinal_c"]    = std::round((maximo - alvo) * 10) / 10;
    r["oscilacao_c"]      = std::round(desvio * 100) / 100;
    if (!pot_mantendo.empty()) {
        double p = 0;
        for (double v : pot_mantendo)
            p += v;
        r["potencia_mantendo"] = std::round(p / pot_mantendo.size() * 1000) / 1000;
    }
    std::vector<std::string> avisos;
    const double lento = mesa ? 300 : 150;
    if (t_chegou > lento)
        avisos.push_back("demorou " + std::to_string(int(t_chegou)) + " s para chegar (o comum é menos de " + std::to_string(int(lento)) + " s)");
    if (maximo - alvo > (mesa ? 4 : 8))
        avisos.push_back("passou " + fmt1(maximo - alvo) + " °C do alvo");
    if (desvio > (mesa ? 1.0 : 1.5))
        avisos.push_back("oscilou " + fmt1(desvio) + " °C mantendo a temperatura");
    if (r.contains("potencia_mantendo") && r["potencia_mantendo"].get<double>() > (mesa ? 0.8 : 0.7))
        avisos.push_back("precisou de " + std::to_string(int(r["potencia_mantendo"].get<double>() * 100)) + "% da força só para manter");
    r["resultado"] = avisos.empty() ? "ok" : "atencao";
    std::string txt = std::string(mesa ? "Mesa" : "Bico") + " chegou a " + std::to_string(alvo) + " °C em " + std::to_string(int(t_chegou)) + " s";
    for (const auto& av : avisos)
        txt += "; " + av;
    r["texto"] = txt + ".";
    return r;
}

json teste_coolers(const std::string& dev, const std::string& ag, json& e)
{
    json passos = json::array();
    for (int pct : {50, 100}) {
        if (parar_pedido().load())
            break;
        if (!cooler(dev, ag, pct)) {
            cooler(dev, ag, 0);
            return {{"resultado", "falha"}, {"texto", "A impressora não aceitou ligar o cooler."}};
        }
        std::this_thread::sleep_for(std::chrono::seconds(7));
        const json c   = cartao(dev);
        const json ex  = c.value("extra", json::object());
        const json aj  = c.value("ajustes", json::object());
        json       p   = {{"pedido", pct}};
        if (ex.contains("rpm") && ex["rpm"].is_object() && ex["rpm"].contains("fan"))
            p["rpm"] = ex["rpm"]["fan"];
        if (aj.contains("ventilador"))
            p["informado"] = aj["ventilador"];
        if (ex.contains("coolers") && ex["coolers"].is_object())
            p["informado"] = ex["coolers"].value("peca", 0);
        passos.push_back(p);
        e["passos"] = passos;
        publicar(e);
    }
    cooler(dev, ag, 0);
    json r = {{"passos", passos}};
    if (parar_pedido().load()) {
        r["resultado"] = "interrompido";
        r["texto"]     = "Teste interrompido. Cooler desligado.";
        return r;
    }
    bool        tem_rpm = false, parado = false;
    std::string txt;
    for (const auto& p : passos) {
        if (p.contains("rpm") && p["rpm"].is_number()) {
            tem_rpm = true;
            const double rpm = p["rpm"].get<double>();
            txt += (txt.empty() ? "" : "; ") + std::to_string(p.value("pedido", 0)) + "% = " + std::to_string(int(rpm)) + " RPM";
            if (rpm < 100)
                parado = true;
        } else if (p.contains("informado")) {
            txt += (txt.empty() ? "" : "; ") + std::to_string(p.value("pedido", 0)) + "%: a impressora informou " + p["informado"].dump();
        }
    }
    if (tem_rpm) {
        r["resultado"] = parado ? "falha" : "ok";
        r["texto"]     = (parado ? "O cooler da peça não girou: " : "Cooler da peça girando: ") + txt + ".";
    } else {
        r["resultado"]    = "confirmar";
        r["texto"]        = "Esta impressora não mede a rotação. " + (txt.empty() ? std::string() : txt + ". ") +
                            "Confirme se você ouviu o cooler acelerar e parar.";
        r["perguntar"]    = "Você ouviu o cooler da peça acelerar e depois parar?";
    }
    return r;
}

// Comando do Klipper com resposta no console (espera até 'limite_s' pelas linhas que interessam).
json teste_console(const std::string& dev, const std::string& ag, const std::string& comando, const std::string& espera,
                   int limite_s, json& e, bool move)
{
    if (move && !enviar(dev, gcode("G28")))
        return {{"resultado", "falha"}, {"texto", "A impressora não aceitou zerar os eixos."}};
    if (!enviar(dev, gcode(comando)))
        return {{"resultado", "falha"}, {"texto", "A impressora não aceitou o comando."}};
    const auto               t0 = Relogio::now();
    std::vector<std::string> linhas;
    while (Relogio::now() - t0 < std::chrono::seconds(limite_s)) {
        if (parar_pedido().load())
            return {{"resultado", "interrompido"}, {"texto", "Teste interrompido."}, {"console", linhas}};
        std::this_thread::sleep_for(std::chrono::seconds(3));
        const json d = leitura_profunda(dev, ag, true, 6);
        linhas       = respostas_depois(d.value("console", json::object()), comando.substr(0, comando.find('\n')));
        e["console"] = linhas;
        publicar(e);
        bool fim = false;
        for (const auto& l : linhas) {
            std::string m = l;
            std::transform(m.begin(), m.end(), m.begin(), ::tolower);
            if ((!espera.empty() && m.find(espera) != std::string::npos) || m.rfind("!!", 0) == 0 || m.find("unknown command") != std::string::npos)
                fim = true;
        }
        if (fim || (espera.empty() && !linhas.empty() && Relogio::now() - t0 > std::chrono::seconds(4)))
            break;
    }
    json        r = {{"console", linhas}};
    std::string tudo;
    for (const auto& l : linhas)
        tudo += l + "\n";
    std::string baixo = tudo;
    std::transform(baixo.begin(), baixo.end(), baixo.begin(), ::tolower);
    if (linhas.empty()) {
        r["resultado"] = "falha";
        r["texto"]     = "A impressora não respondeu ao comando a tempo.";
    } else if (baixo.find("unknown command") != std::string::npos) {
        r["resultado"] = "indisponivel";
        r["texto"]     = "Esta impressora não tem este recurso (comando desconhecido).";
    } else if (baixo.find("!!") != std::string::npos || baixo.find("error") != std::string::npos) {
        r["resultado"] = "falha";
        r["texto"]     = "A impressora respondeu com erro: " + tudo.substr(0, 400);
    } else {
        r["resultado"] = "ok";
        r["texto"]     = tudo.substr(0, 600);
    }
    return r;
}

void executar(json pedido)
{
    const std::string dev = pedido.value("dev_id", std::string()), id = pedido.value("teste", std::string());
    const std::string origem = pedido.value("origem", std::string("cliente"));
    const Teste*      t      = teste_por_id(id);
    json              e      = {{"dev_id", dev}, {"teste", id}, {"nome", t->nome}, {"risco", t->risco}, {"origem", origem},
                                {"estado", "rodando"}, {"inicio", agora_unix()}};
    parar_pedido() = false;
    json c;
    try {
        c = cartao(dev);
    } catch (...) {
    }
    const std::string ag = c.value("agente", std::string());
    // pedido remoto de teste físico: espera o cliente confirmar presença na tela do programa
    if (std::string(t->risco) != "leitura" && !presente(dev)) {
        e["estado"] = "esperando_presenca";
        publicar(e);
        wxGetApp().CallAfter([nome = std::string(t->nome), imp = c.value("nome", std::string("impressora"))] {
            wxNotificationMessage aviso(wxString::FromUTF8("Filuna — diagnóstico da " + imp),
                                        wxString::FromUTF8("Pedido de teste: " + nome +
                                                           ". Abra Dispositivo › Assistência técnica › Diagnóstico e confirme que está ao "
                                                           "lado da impressora para começar."),
                                        wxGetApp().mainframe);
            aviso.Show();
        });
        const auto t0 = Relogio::now();
        while (!presente(dev)) {
            if (parar_pedido().load() || Relogio::now() - t0 > std::chrono::minutes(ESPERA_MIN)) {
                e["estado"]    = "interrompido";
                e["resultado"] = {{"resultado", "interrompido"}, {"texto", "O teste não começou: ninguém confirmou estar ao lado da impressora."}};
                publicar(e);
                return;
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        e["estado"] = "rodando";
        e["inicio"] = agora_unix();
    }
    publicar(e);
    json r;
    try {
        if (id == "aquecimento_bico" || id == "aquecimento_mesa")
            r = teste_aquecimento(dev, ag, id == "aquecimento_mesa", e);
        else if (id == "coolers")
            r = teste_coolers(dev, ag, e);
        else if (id == "consultas") {
            const json d = leitura_profunda(dev, ag, false, 15); // drivers dos motores e sensores vêm nos objetos
            r            = teste_console(dev, ag, "QUERY_ENDSTOPS", "", 20, e, false);
            json tmc     = json::object();
            for (const auto& [k, v] : d.value("objetos", json::object()).value("status", json::object()).items())
                if (k.rfind("tmc", 0) == 0 || k.rfind("filament_", 0) == 0 || k == "probe")
                    tmc[k] = v;
            r["sensores"] = tmc;
        } else if (id == "ruido_acelerometro")
            r = teste_console(dev, ag, "MEASURE_AXES_NOISE", "axes noise", 60, e, false);
        else if (id == "pid_bico")
            r = teste_console(dev, ag, "PID_CALIBRATE HEATER=extruder TARGET=" + std::to_string(BICO_TESTE), "pid parameters", 480, e, false);
        else if (id == "malha_mesa") {
            r = teste_console(dev, ag, "BED_MESH_CALIBRATE", "mesh bed leveling complete", 360, e, true);
            const json d = leitura_profunda(dev, ag, false, 15);
            const json bm = d.value("objetos", json::object()).value("status", json::object()).value("bed_mesh", json::object());
            if (bm.contains("probed_matrix") && bm["probed_matrix"].is_array()) {
                double mn = 1e9, mx = -1e9;
                for (const auto& linha : bm["probed_matrix"])
                    for (const auto& v : linha)
                        if (v.is_number())
                            mn = std::min(mn, v.get<double>()), mx = std::max(mx, v.get<double>());
                if (mx > mn) {
                    r["malha"]       = bm["probed_matrix"];
                    r["variacao_mm"] = std::round((mx - mn) * 1000) / 1000;
                    r["texto"]       = "Variação da mesa: " + fmt1((mx - mn) * 100) + " centésimos de mm (" +
                                 (mx - mn > 0.3 ? "alta: confira parafusos e a placa" : "dentro do comum") + ").";
                    if (mx - mn > 0.3 && r.value("resultado", std::string()) == "ok")
                        r["resultado"] = "atencao";
                }
            }
        } else if (id == "precisao_sonda")
            r = teste_console(dev, ag, "PROBE_ACCURACY", "standard deviation", 240, e, true);
    } catch (const std::exception& ex) {
        r = {{"resultado", "falha"}, {"texto", std::string("O teste parou por um erro: ") + ex.what()}};
    }
    if (std::string(t->risco) == "aquece" || r.value("resultado", std::string()) != "ok")
        desligar_tudo(dev, ag); // ao fim de qualquer teste que aquece (e de qualquer falha): calor e cooler desligados
    e["fim"]       = agora_unix();
    e["estado"]    = r.value("resultado", std::string()) == "interrompido" ? "interrompido" : "concluido";
    e["resultado"] = r;
    e.erase("amostras");
    publicar(e);
    // registra no Raio X da conta
    const json analise = registrar({{"dev_id", dev}, {"cadastro_id", c.value("cadastro_id", std::string())}, {"nome", c.value("nome", std::string())},
                                    {"modelo", c.value("modelo_nome", std::string())}, {"agente", ag}, {"tipo", "teste"}, {"teste", id},
                                    {"origem", origem}, {"dados", r}});
    e["analise"] = analise;
    publicar(e);
}

} // namespace

json ler(const std::string& dev_id, const std::string& origem)
{
    json c;
    try {
        c = cartao(dev_id);
    } catch (const std::exception& ex) {
        return {{"erro", ex.what()}};
    }
    if (c.empty())
        return {{"erro", "Impressora não encontrada neste programa."}};
    if (!c.value("ligada", false))
        return {{"erro", "A impressora está desligada ou fora da rede: ligue-a e confira a conexão antes do diagnóstico."}};
    const std::string ag = c.value("agente", std::string());
    json leitura = {{"cartao", c}, {"agente", ag}, {"em", agora_unix()}};
    const json br = json::parse(Filuna::bruto(dev_id), nullptr, false);
    if (br.is_object())
        leitura["relatorio"] = br;
    leitura["profunda"] = leitura_profunda(dev_id, ag, false, 20);
    try {
        leitura["rede"] = FilunaDispositivos::rede_diagnostico(c.value("ip", std::string()), std::string());
    } catch (...) {
    }
    leitura["testes"] = testes(dev_id);
    json r = {{"leitura", leitura}};
    r["analise"] = registrar({{"dev_id", dev_id}, {"cadastro_id", c.value("cadastro_id", std::string())}, {"nome", c.value("nome", std::string())},
                              {"modelo", c.value("modelo_nome", std::string())}, {"agente", ag}, {"tipo", "leitura"}, {"origem", origem},
                              {"dados", leitura}});
    // a tela não precisa do relatório bruto inteiro (fica no site); devolve o resumo
    r["leitura"].erase("relatorio");
    if (r["leitura"]["profunda"].is_object())
        r["leitura"]["profunda"].erase("klippy_log");
    return r;
}

json testes(const std::string& dev_id)
{
    json c;
    try {
        c = cartao(dev_id);
    } catch (...) {
    }
    const std::string ag    = c.value("agente", std::string());
    json              lista = json::array();
    for (const std::string& id : ids_do_agente(ag))
        if (const Teste* t = teste_por_id(id))
            lista.push_back({{"id", t->id}, {"nome", t->nome}, {"risco", t->risco}, {"minutos", t->minutos}, {"explicacao", t->explicacao}});
    return {{"testes", lista}, {"guiados", guiados(ag)}, {"presenca", presente(dev_id)}, {"presenca_minutos", PRESENCA_MIN}};
}

json testar(const json& pedido)
{
    const std::string dev = pedido.value("dev_id", std::string()), id = pedido.value("teste", std::string());
    const Teste*      t   = teste_por_id(id);
    if (t == nullptr)
        return {{"erro", "Teste desconhecido."}};
    json c;
    try {
        c = cartao(dev);
    } catch (const std::exception& ex) {
        return {{"erro", ex.what()}};
    }
    const auto ids = ids_do_agente(c.value("agente", std::string()));
    if (std::find(ids.begin(), ids.end(), id) == ids.end())
        return {{"erro", "Esta impressora não permite este teste pelo Filuna."}};
    if (!c.value("ligada", false))
        return {{"erro", "A impressora está desligada ou fora da rede."}};
    const std::string est = c.value("estado", std::string());
    if (est == "imprimindo" || est == "pausada")
        return {{"erro", "A impressora está imprimindo: os testes só rodam com ela parada."}};
    {
        std::lock_guard<std::mutex> l(mtx());
        const std::string           e = atual().value("estado", std::string());
        if (e == "rodando" || e == "esperando_presenca")
            return {{"erro", "Já há um teste em andamento: " + atual().value("nome", std::string()) + "."}};
        atual() = {{"dev_id", dev}, {"teste", id}, {"nome", t->nome}, {"risco", t->risco}, {"estado", "rodando"}};
    }
    std::thread(executar, pedido).detach();
    return {{"ok", true}, {"teste", id}, {"risco", t->risco},
            {"presenca", std::string(t->risco) == "leitura" || presente(dev)}};
}

json teste_estado()
{
    std::lock_guard<std::mutex> l(mtx());
    return atual();
}

json parar()
{
    parar_pedido() = true;
    json e;
    {
        std::lock_guard<std::mutex> l(mtx());
        e = atual();
    }
    const std::string dev = e.value("dev_id", std::string());
    if (dev.empty())
        return {{"ok", true}};
    std::thread([dev, risco = e.value("risco", std::string())] {
        json c;
        try {
            c = cartao(dev);
        } catch (...) {
        }
        const std::string ag = c.value("agente", std::string());
        if (risco == "move" && moonraker(ag))
            enviar(dev, gcode("M112")); // parada de emergência do Klipper: o teste estava movendo os eixos
        desligar_tudo(dev, ag);
    }).detach();
    return {{"ok", true}};
}

json presenca(const std::string& dev_id)
{
    if (dev_id.empty())
        return {{"erro", "Impressora não informada."}};
    std::lock_guard<std::mutex> l(mtx());
    presencas()[dev_id] = Relogio::now();
    return {{"ok", true}, {"minutos", PRESENCA_MIN}};
}

}}} // namespace Slic3r::GUI::FilunaDiagnostico
