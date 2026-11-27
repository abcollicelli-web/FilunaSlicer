#include "FilunaNovidades.hpp"

#include "FilunaManutencao.hpp"
#include "FilunaPanel.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/Http.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <cstdio>
#include <thread>

#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>

#include <wx/dialog.h>
#include <wx/sizer.h>

namespace Slic3r { namespace GUI { namespace FilunaNovidades {

namespace {

using json   = nlohmann::json;
using Versao = std::array<int, 3>;

const char* const CHAVE_VISTAS = "filuna_novidades_vistas"; // última versão cujas novidades o cliente viu
const char* const CHAVE_ADIADA = "filuna_novidades_adiada"; // versão em que ele tocou em "Ver depois" (só uma vez)

// "2.5.93.0", "2.5.93" ou "2.5.0-beta.r12" → os três primeiros números. Nada quando não começa assim.
std::optional<Versao> numeros(const std::string& texto)
{
    Versao v{};
    char   fim = 0;
    if (std::sscanf(texto.c_str(), "%d.%d.%d%c", &v[0], &v[1], &v[2], &fim) < 3)
        return std::nullopt;
    return v;
}

std::string texto_da_versao(const Versao& v) { return std::to_string(v[0]) + "." + std::to_string(v[1]) + "." + std::to_string(v[2]); }

// Versões do arquivo embutido, cada uma {"versao", "itens"}.
json embutidas()
{
    const boost::filesystem::path arquivo = boost::filesystem::path(resources_dir()) / "web" / "filuna" / "novidades.json";
    boost::nowide::ifstream       f(arquivo.string(), std::ios::binary);
    const json                    doc = f.good() ? json::parse(f, nullptr, false) : json();
    if (!doc.is_object() || !doc.contains("versoes") || !doc["versoes"].is_array()) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna: novidades.json ausente ou inválido em " << arquivo.string();
        return json::array();
    }
    return doc["versoes"];
}

// O que o site tem para esta versão (Admin › Novidades). Vazio sem internet ou sem nada cadastrado.
json do_site(const std::string& base, const std::string& versao)
{
    json versoes = json::array();
    Http http    = Http::get(base + "/api/slicer/v1/novidades?versao=" + Http::url_encode(versao));
    http.tls_verify(true);
    http.timeout_connect(6);
    http.timeout_max(10);
    http.on_complete([&](std::string corpo, unsigned status) {
            const json doc = status == 200 ? json::parse(corpo, nullptr, false) : json();
            if (doc.is_object() && doc.contains("versoes") && doc["versoes"].is_array())
                versoes = doc["versoes"];
        })
        .on_error([&](std::string, std::string erro, unsigned status) {
            BOOST_LOG_TRIVIAL(info) << "Filuna: novidades do site indisponíveis (" << status << " " << erro << "); vale o texto embutido";
        })
        .perform_sync();
    return versoes;
}

// A versão do site troca a embutida de mesmo número (lista vazia esconde a versão).
json juntar(json lista, const json& do_site)
{
    for (const json& nova : do_site) {
        if (!nova.is_object() || !numeros(nova.value("versao", std::string())))
            continue;
        const Versao v  = *numeros(nova.value("versao", std::string()));
        const auto   it = std::find_if(lista.begin(), lista.end(), [&](const json& e) {
            return e.is_object() && numeros(e.value("versao", std::string())) == v;
        });
        if (it != lista.end())
            *it = nova;
        else
            lista.push_back(nova);
    }
    return lista;
}

std::optional<Versao> mais_nova(const json& lista)
{
    std::optional<Versao> maior;
    for (const json& e : lista)
        if (const auto v = e.is_object() ? numeros(e.value("versao", std::string())) : std::nullopt; v && (!maior || *v > *maior))
            maior = v;
    return maior;
}

// Pacote da Store: a versão dele. Pacote de teste (fora da Store): a mais nova do arquivo.
std::optional<Versao> versao_instalada(const json& lista)
{
    if (const auto loja = numeros(FilunaManutencao::versao_do_pacote()))
        return loja;
    return mais_nova(lista);
}

// Até `maximo` versões com itens entre a última vista (exclusive) e a instalada (inclusive), a mais nova primeiro.
json escolher(const json& lista, const std::optional<Versao>& desde, const Versao& instalada, size_t maximo)
{
    std::vector<std::pair<Versao, json>> achadas;
    for (const json& e : lista) {
        const auto v = e.is_object() ? numeros(e.value("versao", std::string())) : std::nullopt;
        if (!v || *v > instalada || (desde && *v <= *desde) || !e.contains("itens") || !e["itens"].is_array() || e["itens"].empty())
            continue;
        achadas.emplace_back(*v, e);
    }
    std::sort(achadas.begin(), achadas.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    json saida = json::array();
    for (size_t i = 0; i < achadas.size() && i < maximo; ++i) {
        json e      = achadas[i].second;
        e["versao"] = texto_da_versao(achadas[i].first);
        saida.push_back(std::move(e));
    }
    return saida;
}

std::string idioma()
{
    return wxGetApp().current_language_code_safe().Lower().StartsWith("en") ? "en" : "pt";
}

void gravar(const char* chave, const std::string& valor)
{
    AppConfig* cfg = wxGetApp().app_config;
    if (cfg == nullptr || cfg->get(chave) == valor)
        return;
    cfg->set(chave, valor);
    cfg->save();
}

class Janela;
Janela* g_janela = nullptr;

// O cartão das novidades: a tela novidades.html numa janela própria, por cima da principal.
class Janela : public wxDialog
{
public:
    explicit Janela(json dados)
        : wxDialog(wxGetApp().mainframe, wxID_ANY, wxString::FromUTF8(dados.value("idioma", std::string()) == "en" ? "What's new" : "Novidades"),
                   wxDefaultPosition, wxGetApp().mainframe->FromDIP(wxSize(640, 720)), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
        , m_dados(std::move(dados))
    {
        SetBackgroundColour(wxColour(0xF6, 0xF2, 0xEB));
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(new FilunaPanel(this, "novidades.html"), 1, wxEXPAND);
        SetSizer(sizer);
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { fechar(false); });
        CentreOnParent();
        g_janela = this;
    }
    ~Janela() override
    {
        if (g_janela == this)
            g_janela = nullptr;
    }

    const json& dados() const { return m_dados; }

    // depois: o cliente tocou em "Ver depois" (volta na próxima abertura, uma vez). Fechar de outro jeito conta como visto.
    void fechar(bool depois)
    {
        if (m_fechando)
            return;
        m_fechando          = true;
        const std::string v = m_dados.value("instalada", std::string());
        if (!v.empty()) {
            if (depois && m_dados.value("pode_adiar", false))
                gravar(CHAVE_ADIADA, v);
            else
                gravar(CHAVE_VISTAS, v);
        }
        Hide();
        CallAfter([this] { Destroy(); });
    }

private:
    json m_dados;
    bool m_fechando{false};
};

void abrir_janela(json dados)
{
    if (wxGetApp().mainframe == nullptr)
        return;
    if (g_janela != nullptr) {
        g_janela->Raise();
        return;
    }
    (new Janela(std::move(dados)))->Show();
}

// Lê o site fora da linha da tela e, de volta nela, decide e abre.
void com_lista_completa(std::function<void(const json&)> depois, int espera_s)
{
    const std::string base   = filuna_site_base();
    const json        lista  = embutidas();
    const auto        inst   = versao_instalada(lista);
    const std::string versao = inst ? texto_da_versao(*inst) : std::string();
    std::thread([base, lista, versao, depois, espera_s] {
        std::this_thread::sleep_for(std::chrono::seconds(espera_s)); // a janela principal termina de abrir antes
        const json completa = juntar(lista, do_site(base, versao));
        wxGetApp().CallAfter([completa, depois] { depois(completa); });
    }).detach();
}

} // namespace

void ao_abrir(bool instalacao_nova)
{
    AppConfig* cfg = wxGetApp().app_config;
    if (cfg == nullptr)
        return;
    const std::string vistas = cfg->get(CHAVE_VISTAS);
    if (vistas.empty() && instalacao_nova) {
        // Primeira instalação: nada é novidade. Fica marcada a versão de agora.
        if (const auto v = versao_instalada(embutidas()))
            gravar(CHAVE_VISTAS, texto_da_versao(*v));
        return;
    }
    const auto loja = numeros(FilunaManutencao::versao_do_pacote());
    if (loja && numeros(vistas) && *numeros(vistas) >= *loja)
        return; // já viu as desta versão: nem pergunta ao site
    com_lista_completa(
        [vistas](const json& lista) {
            const auto inst = versao_instalada(lista);
            if (!inst)
                return;
            const std::string instalada = texto_da_versao(*inst);
            const json        escolha   = escolher(lista, numeros(vistas), *inst, vistas.empty() ? 1 : 5);
            if (escolha.empty()) {
                gravar(CHAVE_VISTAS, instalada);
                return;
            }
            AppConfig* cfg = wxGetApp().app_config;
            const bool adiada = cfg != nullptr && cfg->get(CHAVE_ADIADA) == instalada;
            abrir_janela({{"modo", "atualizacao"}, {"instalada", instalada}, {"idioma", idioma()}, {"versoes", escolha}, {"pode_adiar", !adiada}});
        },
        3);
}

void mostrar()
{
    com_lista_completa(
        [](const json& lista) {
            const auto inst = versao_instalada(lista);
            const json escolha = inst ? escolher(lista, std::nullopt, *inst, 5) : json::array();
            abrir_janela({{"modo", "menu"},
                          {"instalada", inst ? texto_da_versao(*inst) : std::string()},
                          {"idioma", idioma()},
                          {"versoes", escolha},
                          {"pode_adiar", false}});
        },
        0);
}

std::string rotulo_menu() { return idioma() == "en" ? "What's new" : "O que há de novo"; }

std::optional<json> acao(const std::string& nome, const json& dados)
{
    if (nome == "novidades.dados")
        return std::make_optional(g_janela != nullptr ? g_janela->dados() : json{{"versoes", json::array()}, {"idioma", idioma()}});
    if (nome == "novidades.fechar") {
        const bool depois = dados.value("depois", false);
        if (g_janela != nullptr)
            g_janela->CallAfter([depois] {
                if (g_janela != nullptr)
                    g_janela->fechar(depois);
            });
        return json::object();
    }
    return std::nullopt;
}

}}} // namespace Slic3r::GUI::FilunaNovidades
