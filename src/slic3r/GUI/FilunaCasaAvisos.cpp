#include "FilunaCasaAvisos.hpp"
#include "FilunaCasa.hpp"
#include "FilunaPanel.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/Http.hpp"

#include <wx/dcbuffer.h>
#include <wx/display.h>
#include <wx/frame.h>
#include <wx/graphics.h>
#include <wx/timer.h>
#ifdef _WIN32
#include <wx/msw/wrapwin.h>
#endif

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <chrono>
#include <ctime>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace Slic3r { namespace GUI { namespace FilunaCasaAvisos {

namespace {

const char* const SITE = "https://filuna3d.com";

struct Evento
{
    const char* titulo;
    const char* texto; // modelo padrão; o cliente troca em Personalizar
    const char* cor;
};
const std::map<std::string, Evento>& eventos()
{
    static const std::map<std::string, Evento> E = {
        {"concluida", {"Impressão concluída", "{impressora}: {peca} terminou. Retire a peça e limpe a mesa.", "#1F8F5F"}},
        {"erro", {"Erro na impressão", "{impressora} parou: {erro}", "#E0261B"}},
        {"pausada", {"Impressão pausada", "{impressora} pausou {peca}. Confira a impressora.", "#F59E0B"}},
        {"filamento", {"Filamento trocado", "{impressora}: {texto}", "#8B5CF6"}},
        {"venda", {"Nova venda", "{texto}", "#2F6FEB"}},
        {"mensagem", {"Aviso do Filuna", "{texto}", "#E6007E"}},
        {"teste", {"Aviso de teste", "Se você está vendo isto, {aparelho} está ligado ao Filuna.", "#E6007E"}},
    };
    return E;
}

std::string minusc(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

std::string agora_iso()
{
    const std::time_t t = std::time(nullptr);
    char              b[32];
    std::tm           tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::strftime(b, sizeof(b), "%Y-%m-%dT%H:%M:%S", &tm);
    return b;
}

int minutos_do_dia()
{
    const std::time_t t = std::time(nullptr);
    std::tm           tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm.tm_hour * 60 + tm.tm_min;
}

int hhmm(const std::string& s, int padrao)
{
    const auto p = s.find(':');
    if (p == std::string::npos)
        return padrao;
    try { return std::stoi(s.substr(0, p)) * 60 + std::stoi(s.substr(p + 1)); } catch (...) { return padrao; }
}

// ---------------------------------------------------------------- regras gravadas neste computador

json ler_regras()
{
    AppConfig* cfg = wxGetApp().app_config;
    json       r   = cfg != nullptr ? json::parse(cfg->get("filuna_casa_regras"), nullptr, false) : json();
    return r.is_object() ? r : json::object();
}

void gravar_regras(const json& r)
{
    if (AppConfig* cfg = wxGetApp().app_config) {
        cfg->set("filuna_casa_regras", r.dump());
        cfg->save();
    }
}

// ---------------------------------------------------------------- histórico

std::mutex& trava_hist()
{
    static std::mutex m;
    return m;
}
json& hist()
{
    static json h = [] {
        json j;
        try {
            boost::nowide::ifstream f((boost::filesystem::path(data_dir()) / "filuna" / "casa_historico.json").string());
            if (f)
                j = json::parse(f, nullptr, false);
        } catch (...) {}
        return j.is_array() ? j : json::array();
    }();
    return h;
}

void registrar(const json& item)
{
    json copia;
    {
        std::lock_guard<std::mutex> g(trava_hist());
        hist().insert(hist().begin(), item);
        if (hist().size() > 200)
            hist().erase(hist().begin() + 200, hist().end());
        copia = hist();
    }
    try {
        const auto pasta = boost::filesystem::path(data_dir()) / "filuna";
        boost::filesystem::create_directories(pasta);
        boost::nowide::ofstream f((pasta / "casa_historico.json").string());
        f << copia.dump();
    } catch (...) {}
    wxGetApp().CallAfter([item] { FilunaPanel::broadcast("casa.aviso", item); });
}

// ---------------------------------------------------------------- aviso montado

std::string preencher(std::string s, const std::map<std::string, std::string>& v)
{
    for (const auto& [k, val] : v) {
        const std::string marca = "{" + k + "}";
        for (std::size_t p = s.find(marca); p != std::string::npos; p = s.find(marca, p + val.size()))
            s.replace(p, marca.size(), val);
    }
    return s;
}

std::map<std::string, std::string> variaveis(const json& aviso)
{
    std::map<std::string, std::string> v;
    for (const char* k : {"impressora", "peca", "erro", "texto", "titulo", "evento", "aparelho", "cor", "pedido", "produto"})
        v[k] = aviso.value(k, std::string());
    v["progresso"] = std::to_string(aviso.value("progresso", 0));
    if (v["peca"].empty())
        v["peca"] = "a impressão";
    if (v["impressora"].empty())
        v["impressora"] = "A impressora";
    return v;
}

// O aviso como cada aparelho vai mostrar: título, texto pelo modelo do cliente e a cor escolhida para o evento.
json montar(const std::string& tipo, const json& dados, const json& cfg, const std::string& nome_aparelho)
{
    const auto& E  = eventos();
    const auto  it = E.find(tipo);
    const Evento ev = it != E.end() ? it->second : E.at("mensagem");
    json aviso = dados;
    aviso["evento"]   = tipo;
    aviso["aparelho"] = nome_aparelho;
    if (aviso.value("titulo", std::string()).empty())
        aviso["titulo"] = ev.titulo;
    std::string modelo = ev.texto;
    if (cfg.contains("textos") && cfg["textos"].is_object() && !cfg["textos"].value(tipo, std::string()).empty())
        modelo = cfg["textos"].value(tipo, std::string());
    aviso["texto"] = preencher(modelo, variaveis(aviso));
    std::string cor = ev.cor;
    const json  cores = cfg.contains("estilo") && cfg["estilo"].is_object() ? cfg["estilo"].value("cores", json::object()) :
                        cfg.value("cores", json::object()); // versão anterior guardava as cores soltas
    if (cores.is_object() && !cores.value(tipo, std::string()).empty())
        cor = cores.value(tipo, std::string());
    aviso["cor"]  = cor;
    aviso["quando"] = agora_iso();
    return aviso;
}

bool em_silencio(const json& cfg, const std::string& tipo)
{
    if (tipo == "teste")
        return false;
    const json& s = cfg.contains("silencio") ? cfg["silencio"] : json(false);
    bool        ligado = s.is_boolean() ? s.get<bool>() : s.is_object() && s.value("ligado", false);
    if (!ligado)
        return false;
    const int de  = s.is_object() ? hhmm(s.value("de", std::string("22:00")), 22 * 60) : 22 * 60;
    const int ate = s.is_object() ? hhmm(s.value("ate", std::string("07:00")), 7 * 60) : 7 * 60;
    if (tipo == "erro" && s.is_object() && s.value("erro_acorda", false))
        return false;
    const int m = minutos_do_dia();
    return de <= ate ? (m >= de && m < ate) : (m >= de || m < ate);
}

bool quer_evento(const json& cfg, const std::string& tipo)
{
    if (tipo == "teste")
        return true;
    const json av = cfg.value("avisos", json::object());
    if (!av.is_object() || !av.contains(tipo))
        return tipo == "concluida" || tipo == "erro" || tipo == "mensagem"; // padrão de quem ainda não escolheu
    return av[tipo].is_boolean() && av[tipo].get<bool>();
}

bool da_impressora(const json& cfg, const json& dados)
{
    const json lista = cfg.value("impressoras", json::array());
    const std::string imp = minusc(dados.value("impressora", std::string()));
    if (!lista.is_array() || lista.empty() || imp.empty())
        return true;
    for (const auto& x : lista)
        if (x.is_string()) {
            std::string k = minusc(x.get<std::string>());
            if (k.rfind("impressora:", 0) == 0)
                k = k.substr(11);
            if (k == imp)
                return true;
        }
    return false;
}

// ---------------------------------------------------------------- popup deste computador

class AvisoJanela : public wxFrame
{
public:
    static std::vector<AvisoJanela*>& abertas()
    {
        static std::vector<AvisoJanela*> v;
        return v;
    }

    AvisoJanela(const json& aviso, const json& estilo)
        : wxFrame(nullptr, wxID_ANY, "Filuna", wxDefaultPosition, wxDefaultSize,
                  wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxBORDER_NONE | wxFRAME_TOOL_WINDOW)
        , m_aviso(aviso)
        , m_modelo(estilo.value("modelo", std::string("cartao")))
        , m_tema(estilo.value("tema", std::string("claro")))
        , m_timer(this)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        m_cor = cor(aviso.value("cor", std::string("#E6007E")));
        const int indice = int(abertas().size());
        abertas().push_back(this);
        const wxRect area = wxDisplay(wxDisplay::GetFromWindow(wxGetApp().mainframe) == wxNOT_FOUND ? 0u :
                                          unsigned(wxDisplay::GetFromWindow(wxGetApp().mainframe))).GetClientArea();
        const int m = FromDIP(16);
        if (m_modelo == "faixa") {
            SetSize(area.x, area.y + indice * FromDIP(80), area.width, FromDIP(76));
        } else if (m_modelo == "destaque") {
            const wxSize t(FromDIP(560), FromDIP(260));
            SetSize(area.x + (area.width - t.x) / 2, area.y + (area.height - t.y) / 2 + indice * FromDIP(24), t.x, t.y);
        } else {
            const wxSize t(FromDIP(400), FromDIP(132));
            SetSize(area.GetRight() - t.x - m, area.GetBottom() - (t.y + m) * (indice + 1), t.x, t.y);
        }
        Bind(wxEVT_PAINT, &AvisoJanela::pintar, this);
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { fechar(); });
        Bind(wxEVT_TIMER, [this](wxTimerEvent&) { fechar(); });
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { fechar(); });
        const int dur = estilo.value("duracao_s", m_modelo == "destaque" ? 0 : 10);
        if (dur > 0)
            m_timer.StartOnce(dur * 1000);
        if (estilo.value("som", true)) {
#ifdef _WIN32
            MessageBeep(aviso.value("evento", std::string()) == "erro" ? MB_ICONHAND : MB_ICONASTERISK);
#else
            wxBell();
#endif
        }
        ShowWithoutActivating();
    }

private:
    static wxColour cor(const std::string& hex)
    {
        wxColour c(wxString::FromUTF8(hex));
        return c.IsOk() ? c : wxColour(0xE6, 0x00, 0x7E);
    }

    void fechar()
    {
        if (m_fechando)
            return;
        m_fechando = true;
        m_timer.Stop();
        auto& v = abertas();
        v.erase(std::remove(v.begin(), v.end(), this), v.end());
        Destroy();
    }

    static std::vector<wxString> quebrar(wxGraphicsContext* gc, const wxString& texto, double largura, int max_linhas)
    {
        std::vector<wxString> linhas;
        wxString              atual;
        for (const wxString& palavra : wxSplit(texto, ' ')) {
            const wxString tenta = atual.empty() ? palavra : atual + " " + palavra;
            double         w = 0, h = 0;
            gc->GetTextExtent(tenta, &w, &h);
            if (w > largura && !atual.empty()) {
                linhas.push_back(atual);
                atual = palavra;
            } else
                atual = tenta;
        }
        if (!atual.empty())
            linhas.push_back(atual);
        if (int(linhas.size()) > max_linhas) {
            linhas.resize(max_linhas);
            linhas.back() += wxString::FromUTF8("…");
        }
        return linhas;
    }

    void pintar(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
            return;
        const wxSize   t      = GetClientSize();
        const wxString titulo = wxString::FromUTF8(m_aviso.value("titulo", std::string()));
        const wxString texto  = wxString::FromUTF8(m_aviso.value("texto", std::string()));
        const wxString rodape = wxString::FromUTF8("Filuna" + (m_aviso.value("impressora", std::string()).empty() ? std::string() : " · " + m_aviso.value("impressora", std::string())));
        wxFont fonte = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
        const bool escuro = m_tema == "escuro";
        if (m_modelo == "faixa") {
            gc->SetBrush(wxBrush(m_cor));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRectangle(0, 0, t.x, t.y);
            wxFont f = fonte;
            f.SetPointSize(13);
            f.SetWeight(wxFONTWEIGHT_BOLD);
            gc->SetFont(f, *wxWHITE);
            double wt = 0, ht = 0;
            gc->GetTextExtent(titulo, &wt, &ht);
            gc->DrawText(titulo, FromDIP(24), (t.y - ht) / 2);
            f.SetWeight(wxFONTWEIGHT_NORMAL);
            f.SetPointSize(12);
            gc->SetFont(f, *wxWHITE);
            const auto l = quebrar(gc.get(), texto, t.x - wt - FromDIP(80), 1);
            if (!l.empty()) {
                double w2 = 0, h2 = 0;
                gc->GetTextExtent(l[0], &w2, &h2);
                gc->DrawText(l[0], FromDIP(24) + wt + FromDIP(20), (t.y - h2) / 2);
            }
            return;
        }
        if (m_modelo == "destaque") {
            const wxColour escuro_c((unsigned char) (m_cor.Red() * 0.6), (unsigned char) (m_cor.Green() * 0.6), (unsigned char) (m_cor.Blue() * 0.6));
            gc->SetBrush(gc->CreateLinearGradientBrush(0, 0, t.x, t.y, m_cor, escuro_c));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRoundedRectangle(0, 0, t.x, t.y, FromDIP(18));
            wxFont f = fonte;
            f.SetPointSize(22);
            f.SetWeight(wxFONTWEIGHT_BOLD);
            gc->SetFont(f, *wxWHITE);
            gc->DrawText(titulo, FromDIP(32), FromDIP(30));
            f.SetPointSize(14);
            f.SetWeight(wxFONTWEIGHT_NORMAL);
            gc->SetFont(f, *wxWHITE);
            double y = FromDIP(88);
            for (const auto& l : quebrar(gc.get(), texto, t.x - FromDIP(64), 4)) {
                gc->DrawText(l, FromDIP(32), y);
                y += FromDIP(26);
            }
            f.SetPointSize(10);
            gc->SetFont(f, wxColour(255, 255, 255, 200));
            gc->DrawText(rodape + wxString::FromUTF8(" · clique para fechar"), FromDIP(32), t.y - FromDIP(34));
            return;
        }
        // cartão no canto
        const wxColour fundo = escuro ? wxColour(0x1C, 0x19, 0x21) : *wxWHITE;
        const wxColour tinta = escuro ? *wxWHITE : wxColour(0x1C, 0x19, 0x21);
        const wxColour suave = escuro ? wxColour(0xB8, 0xB2, 0xC0) : wxColour(0x6B, 0x66, 0x72);
        gc->SetBrush(wxBrush(fundo));
        gc->SetPen(wxPen(escuro ? wxColour(0x33, 0x2E, 0x3A) : wxColour(0xE4, 0xE0, 0xEA)));
        gc->DrawRoundedRectangle(0, 0, t.x - 1, t.y - 1, FromDIP(14));
        gc->SetBrush(wxBrush(m_cor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, FromDIP(8), t.y - 1, FromDIP(4));
        gc->DrawEllipse(FromDIP(22), FromDIP(20), FromDIP(14), FromDIP(14));
        wxFont f = fonte;
        f.SetPointSize(12);
        f.SetWeight(wxFONTWEIGHT_BOLD);
        gc->SetFont(f, tinta);
        gc->DrawText(titulo, FromDIP(46), FromDIP(16));
        f.SetPointSize(10);
        f.SetWeight(wxFONTWEIGHT_NORMAL);
        gc->SetFont(f, tinta);
        double y = FromDIP(44);
        for (const auto& l : quebrar(gc.get(), texto, t.x - FromDIP(66), 3)) {
            gc->DrawText(l, FromDIP(46), y);
            y += FromDIP(18);
        }
        f.SetPointSize(8);
        gc->SetFont(f, suave);
        gc->DrawText(rodape, FromDIP(46), t.y - FromDIP(22));
    }

    json        m_aviso;
    std::string m_modelo, m_tema;
    wxColour    m_cor;
    wxTimer     m_timer;
    bool        m_fechando = false;
};

// ---------------------------------------------------------------- plugins (receita de pedido HTTP)

std::string url_cod(const std::string& s)
{
    static const char* H = "0123456789ABCDEF";
    std::string        o;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            o += char(c);
        else {
            o += '%';
            o += H[c >> 4];
            o += H[c & 15];
        }
    }
    return o;
}

// Troca {x} nos textos do corpo. "{aviso}" sozinho vira o aviso inteiro; "{cor_int}" sozinho vira número (Discord).
json preencher_json(const json& modelo, const std::map<std::string, std::string>& v, const json& aviso)
{
    if (modelo.is_string()) {
        const std::string s = modelo.get<std::string>();
        if (s == "{aviso}")
            return aviso;
        if (s == "{cor_int}") {
            try { return std::stoi(aviso.value("cor", std::string("#E6007E")).substr(1), nullptr, 16); } catch (...) { return 0xE6007E; }
        }
        return preencher(s, v);
    }
    if (modelo.is_object()) {
        json o = json::object();
        for (const auto& [k, x] : modelo.items())
            o[k] = preencher_json(x, v, aviso);
        return o;
    }
    if (modelo.is_array()) {
        json o = json::array();
        for (const auto& x : modelo)
            o.push_back(preencher_json(x, v, aviso));
        return o;
    }
    return modelo;
}

bool plugin_pedido(const json& receita, const json& campos, const json& aviso, std::string& erro)
{
    if (!receita.is_object() || receita.value("url", std::string()).empty()) {
        erro = "Este plugin não sabe fazer isso.";
        return false;
    }
    std::map<std::string, std::string> v = variaveis(aviso), na_url;
    for (const auto& [k, x] : campos.items())
        v[k] = x.is_string() ? x.get<std::string>() : x.dump();
    for (const auto& [k, x] : v) // na URL, o que vem do aviso vai codificado; os campos do cliente vão como estão
        na_url[k] = campos.contains(k) ? x : url_cod(x);
    const std::string url    = preencher(receita.value("url", std::string()), na_url);
    const std::string metodo = receita.value("metodo", std::string("POST"));
    if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) {
        erro = "Endereço inválido: " + url;
        return false;
    }
    Http http = metodo == "GET" ? Http::get(url) : metodo == "PUT" ? Http::put(url) : Http::post(url);
    for (const auto& [k, x] : receita.value("cabecalhos", json::object()).items())
        http.header(k, preencher(x.is_string() ? x.get<std::string>() : x.dump(), v));
    if (metodo != "GET" && receita.contains("corpo")) {
        const json corpo = preencher_json(receita["corpo"], v, aviso);
        if (corpo.is_string())
            http.set_post_body(corpo.get<std::string>());
        else {
            if (!receita.value("cabecalhos", json::object()).contains("Content-Type"))
                http.header("Content-Type", "application/json");
            http.set_post_body(corpo.dump());
        }
    }
    bool ok = false;
    http.timeout_connect(4)
        .timeout_max(12)
        .on_complete([&](std::string, unsigned st) { ok = st < 400; if (!ok) erro = "O serviço respondeu " + std::to_string(st) + "."; })
        .on_error([&](std::string corpo, std::string e, unsigned st) { erro = "Não deu certo (" + (st ? std::to_string(st) : e) + ")" + (corpo.empty() ? "" : ": " + corpo.substr(0, 160)); })
        .perform_sync();
    return ok;
}

// ---------------------------------------------------------------- entrega

struct Entrega
{
    std::string chave, tipo, protocolo, ip, nome, chave_lg;
    json        plugin, campos, cfg_ha, aviso, estilo;
};

// Prepara na linha da tela (lê chaves e senhas gravadas); entrega numa linha própria.
Entrega preparar(const std::string& chave, const json& aparelho, const json& cfg, const json& aviso)
{
    Entrega e;
    e.chave     = chave;
    e.tipo      = aparelho.value("tipo", std::string());
    e.protocolo = aparelho.value("protocolo", aparelho.value("dados", json::object()).value("protocolo", std::string()));
    e.ip        = aparelho.value("ip", std::string());
    e.nome      = aparelho.value("nome", std::string("aparelho"));
    e.plugin    = aparelho.value("plugin", json::object());
    e.campos    = cfg.value("campos", json::object());
    e.aviso     = aviso;
    e.estilo    = cfg.value("estilo", json::object());
    if (e.tipo == "tv_lg")
        e.chave_lg = FilunaCasa::chave_lg(e.ip);
    if (e.tipo == "home_assistant")
        if (AppConfig* c = wxGetApp().app_config)
            e.cfg_ha = json::parse(c->get("filuna_casa_ha"), nullptr, false);
    return e;
}

json entregar(const Entrega& e)
{
    std::string erro;
    bool        ok = false;
    const int   segundos = e.estilo.value("duracao_s", 8);
    if (e.tipo == "este_pc") {
        const json aviso = e.aviso, estilo = e.estilo;
        wxGetApp().CallAfter([aviso, estilo] { new AvisoJanela(aviso, estilo); });
        ok = true;
    } else if (e.tipo == "tv_lg") {
        ok = FilunaCasa::tv_lg_aviso(e.ip, e.chave_lg, e.aviso.value("titulo", std::string()) + ": " + e.aviso.value("texto", std::string()), erro);
    } else if (e.tipo == "luz") {
        ok = FilunaCasa::luz_aviso(e.protocolo, e.ip, e.aviso.value("cor", std::string("#E6007E")), segundos, erro);
    } else if (e.tipo == "home_assistant") {
        if (!e.cfg_ha.is_object() || e.cfg_ha.value("broker", std::string()).empty())
            erro = "Configure o MQTT do Home Assistant primeiro.";
        else
            ok = FilunaCasa::ha_evento(e.cfg_ha, e.aviso, erro);
    } else if (e.tipo == "plugin") {
        ok = plugin_pedido(e.plugin.value("avisar", json::object()), e.campos, e.aviso, erro);
    } else if (e.tipo == "tomada") {
        erro = "Tomadas não mostram avisos: elas desligam a impressora quando o bico esfria.";
    } else if (e.tipo == "alexa" || e.tipo == "tv_android") {
        erro = e.tipo == "alexa" ? "A skill Filuna da Alexa ainda não foi publicada." : "O app Filuna TV ainda não foi publicado.";
    } else {
        erro = "Este aparelho ainda não recebe avisos do Filuna.";
    }
    return {{"quando", e.aviso.value("quando", agora_iso())}, {"evento", e.aviso.value("evento", std::string())},
            {"titulo", e.aviso.value("titulo", std::string())}, {"texto", e.aviso.value("texto", std::string())},
            {"cor", e.aviso.value("cor", std::string())}, {"aparelho", e.nome}, {"chave", e.chave}, {"ok", ok},
            {"msg", ok ? (e.tipo == "este_pc" ? "Mostrado neste computador." : "Entregue.") : erro}};
}

// Mesmo aviso, mesmo aparelho: no máximo um a cada 30 s (a leitura das impressoras pode repetir o estado).
bool repetido(const std::string& chave)
{
    static std::map<std::string, std::chrono::steady_clock::time_point> ult;
    const auto agora = std::chrono::steady_clock::now();
    auto       it    = ult.find(chave);
    if (it != ult.end() && agora - it->second < std::chrono::seconds(30))
        return true;
    ult[chave] = agora;
    return false;
}

// ---------------------------------------------------------------- tomadas esperando o bico esfriar

struct Espera
{
    std::string chave, protocolo, ip, nome, impressora;
    double      limite = 50;
    std::chrono::steady_clock::time_point desde;
};
std::map<std::string, Espera>& esperas()
{
    static std::map<std::string, Espera> m;
    return m;
}

} // namespace

json regras() { return {{"regras", ler_regras()}}; }

json salvar_regra(const json& pedido)
{
    const std::string chave = pedido.value("chave", std::string());
    if (chave.empty())
        return {{"ok", false}, {"erro", "Aparelho sem chave."}};
    json r = ler_regras();
    if (pedido.value("remover", false))
        r.erase(chave);
    else {
        json ap = pedido.value("aparelho", json::object()), cfg = pedido.value("cfg", json::object());
        cfg.erase("senha");
        r[chave] = {{"aparelho", ap}, {"cfg", cfg}};
    }
    gravar_regras(r);
    return {{"ok", true}};
}

json testar(const json& pedido)
{
    const json        ap   = pedido.value("aparelho", json::object());
    const json        cfg  = pedido.value("cfg", json::object());
    const std::string tipo = pedido.value("evento", std::string("teste"));
    json              dados = {{"impressora", pedido.value("impressora", std::string("Impressora de teste"))}, {"peca", "Peça de teste"},
                               {"erro", "filamento acabou (teste)"}, {"texto", pedido.value("texto", std::string("Aviso de teste do Filuna."))}};
    const json  aviso = montar(tipo, dados, cfg, ap.value("nome", std::string("este aparelho")));
    Entrega     e     = preparar(pedido.value("chave", std::string("teste")), ap, cfg, aviso);
    const std::string id = pedido.value("pedido", std::string());
    std::thread([e, id] {
        json r = entregar(e);
        r["teste"] = true;
        registrar(r);
        r["pedido"] = id;
        wxGetApp().CallAfter([r] { FilunaPanel::broadcast("casa.teste", r); });
    }).detach();
    return {{"aceito", true}};
}

json historico()
{
    std::lock_guard<std::mutex> g(trava_hist());
    return {{"itens", hist()}};
}

void evento(const std::string& tipo, const json& dados)
{
    const json r = ler_regras();
    for (const auto& [chave, regra] : r.items()) {
        const json ap = regra.value("aparelho", json::object()), cfg = regra.value("cfg", json::object());
        const std::string t = ap.value("tipo", std::string());
        if (t == "tomada") { // tomada: na conclusão, espera o bico esfriar para desligar
            if (tipo == "concluida" && cfg.value("desligarFrio", false)) {
                std::string imp = minusc(cfg.value("impressora", std::string()));
                if (imp.rfind("impressora:", 0) == 0)
                    imp = imp.substr(11);
                if (!imp.empty() && imp == minusc(dados.value("impressora", std::string()))) {
                    Espera w;
                    w.chave      = chave;
                    w.protocolo  = ap.value("protocolo", ap.value("dados", json::object()).value("protocolo", std::string()));
                    w.ip         = ap.value("ip", std::string());
                    w.nome       = ap.value("nome", std::string("Tomada"));
                    w.impressora = imp;
                    w.limite     = cfg.value("limite_c", 50.0);
                    w.desde      = std::chrono::steady_clock::now();
                    esperas()[chave] = w;
                }
            }
            continue;
        }
        if (!cfg.value("_pronto", false) && t != "este_pc")
            continue; // só avisa quem terminou de configurar (e passou no teste)
        if (!quer_evento(cfg, tipo) || !da_impressora(cfg, dados) || em_silencio(cfg, tipo))
            continue;
        if (repetido(chave + "|" + tipo + "|" + dados.value("impressora", std::string()) + "|" + dados.value("id", std::string())))
            continue;
        const json aviso = montar(tipo, dados, cfg, ap.value("nome", std::string("aparelho")));
        Entrega    e     = preparar(chave, ap, cfg, aviso);
        std::thread([e] { registrar(entregar(e)); }).detach();
    }
}

void vigiar(const json& estado_impressoras, const std::string& token)
{
    // tomadas esperando o bico esfriar
    for (auto it = esperas().begin(); it != esperas().end();) {
        Espera& w = it->second;
        bool    desligar = false, desistir = false;
        double  bico = -1;
        for (const auto& c : estado_impressoras.value("impressoras", json::array())) {
            if (minusc(c.value("nome", std::string())) != w.impressora)
                continue;
            if (c.value("estado", std::string()) == "imprimindo")
                desistir = true; // começou outra impressão: a tomada fica ligada
            if (c.contains("bico") && c["bico"].is_object())
                bico = c["bico"].value("atual", -1.0);
            if (!c.value("ligada", true))
                desistir = true;
        }
        if (!desistir && bico >= 0 && bico < w.limite)
            desligar = true;
        if (std::chrono::steady_clock::now() - w.desde > std::chrono::hours(3))
            desistir = true;
        if (desligar) {
            const Espera copia = w;
            std::thread([copia, bico] {
                std::string erro;
                const bool  ok = FilunaCasa::tomada_ligar(copia.protocolo, copia.ip, false, erro);
                char        t[16];
                std::snprintf(t, sizeof(t), "%.0f", bico);
                registrar({{"quando", agora_iso()}, {"evento", "energia"}, {"titulo", "Tomada desligada"},
                           {"texto", "A " + copia.impressora + " terminou e o bico esfriou (" + t + " °C). " + copia.nome + " desligou."},
                           {"cor", "#1F8F5F"}, {"aparelho", copia.nome}, {"chave", copia.chave}, {"ok", ok}, {"msg", ok ? "Desligada." : erro}});
            }).detach();
        }
        if (desligar || desistir)
            it = esperas().erase(it);
        else
            ++it;
    }

    // avisos que chegaram pelo site: vendas das lojas ligadas e mensagens mandadas pela API
    static std::chrono::steady_clock::time_point ultima;
    static bool                                  buscando = false;
    if (token.empty() || buscando || std::chrono::steady_clock::now() - ultima < std::chrono::seconds(30))
        return;
    ultima   = std::chrono::steady_clock::now();
    buscando = true;
    std::thread([token] {
        json lista = json::array();
        Http::get(std::string(SITE) + "/api/slicer/v1/casa/avisos")
            .header("Authorization", "Bearer " + token)
            .timeout_connect(5)
            .timeout_max(15)
            .on_complete([&](std::string b, unsigned) {
                const json j = json::parse(b, nullptr, false);
                if (j.is_object() && j.contains("avisos") && j["avisos"].is_array())
                    lista = j["avisos"];
            })
            .on_error([](std::string, std::string e, unsigned) { BOOST_LOG_TRIVIAL(debug) << "Filuna Home: avisos do site: " << e; })
            .perform_sync();
        for (const auto& a : lista) {
            const std::string id = a.value("id", std::string());
            if (id.empty())
                continue;
            Http::post(std::string(SITE) + "/api/slicer/v1/casa/avisos/" + id + "/entregue")
                .header("Authorization", "Bearer " + token)
                .timeout_connect(5)
                .timeout_max(10)
                .perform_sync();
        }
        wxGetApp().CallAfter([lista] {
            buscando = false;
            for (const auto& a : lista) {
                json dados = a.value("dados", json::object());
                if (!dados.is_object())
                    dados = json::object();
                dados["id"] = a.value("id", std::string());
                for (const char* k : {"titulo", "texto"})
                    if (!a.value(k, std::string()).empty())
                        dados[k] = a.value(k, std::string());
                const std::string tipo = a.value("tipo", std::string("mensagem"));
                evento(eventos().count(tipo) ? tipo : "mensagem", dados);
            }
        });
    }).detach();
}

}}} // namespace Slic3r::GUI::FilunaCasaAvisos
