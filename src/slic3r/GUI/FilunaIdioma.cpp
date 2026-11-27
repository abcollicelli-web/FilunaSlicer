#include "FilunaIdioma.hpp"

#include "ActionRegistry.hpp"
#include "GUI_App.hpp"
#include "I18N.hpp"
#include "MainFrame.hpp"
#include "MsgDialog.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Filuna/FilunaRecursos.hpp"
#include "slic3r/Filuna/FilunaVersoes.hpp"

#include <atomic>
#include <map>
#include <mutex>
#include <cctype>
#include <sstream>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>
#include <nlohmann/json.hpp>

#include <wx/intl.h>
#include <wx/thread.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Slic3r { namespace Filuna {

using namespace Slic3r::GUI;

namespace {

using json = nlohmann::json;
namespace fs = boost::filesystem;

std::atomic<bool> g_ingles{false};
std::atomic<bool> g_ingles_lido{false};

// Pasta i18n das telas: a da correção vinda do servidor (recurso "telas"), se trouxer o dicionário; senão a embutida.
fs::path pasta_i18n()
{
    const std::string correcao = Recursos::dir_da_correcao((fs::path(data_dir()) / "filuna" / "recursos").string(), "telas",
                                                           versao_embutida("telas"));
    if (!correcao.empty() && fs::exists(fs::path(correcao) / "i18n" / "en.json"))
        return fs::path(correcao) / "i18n";
    return fs::path(resources_dir()) / "web" / "filuna" / "i18n";
}

std::string ler_arquivo(const fs::path& p)
{
    boost::nowide::ifstream f(p.string(), std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Mesmo jeito das telas e do extrair.py: espaços seguidos viram um, sem espaço nas pontas.
std::string normalizar(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    bool espaco = false;
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            espaco = !out.empty();
            continue;
        }
        if (espaco)
            out += ' ';
        espaco = false;
        out += c;
    }
    return out;
}

struct Dicionario
{
    std::string                        texto; // en.json como veio
    std::map<std::string, std::string> frases;
    std::map<std::string, std::string> padroes;
};

const Dicionario& dicionario()
{
    static Dicionario     d;
    static std::once_flag uma_vez;
    std::call_once(uma_vez, [] {
        const fs::path p = pasta_i18n() / "en.json";
        try {
            d.texto      = ler_arquivo(p);
            const json j = json::parse(d.texto);
            for (const char* secao : {"frases", "padroes"}) {
                if (!j.contains(secao) || !j[secao].is_object())
                    continue;
                auto& alvo = std::string(secao) == "frases" ? d.frases : d.padroes;
                for (auto it = j[secao].begin(); it != j[secao].end(); ++it)
                    if (it.value().is_string() && !it.value().get<std::string>().empty())
                        alvo[it.key()] = it.value().get<std::string>();
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Filuna: dicionário de inglês não abriu (" << p.string() << "): " << e.what();
            d.texto = "{}";
        }
    });
    return d;
}

// Tradução de uma linha só (sem \n) em frases (ou em padroes, se pedido); vazio se não houver.
std::string traduzir_linha(const std::string& linha, bool padrao = false)
{
    const Dicionario& d     = dicionario();
    const auto&       tab   = padrao ? d.padroes : d.frases;
    const std::string s     = normalizar(linha);
    if (s.empty())
        return std::string();
    auto it = tab.find(s);
    if (it != tab.end())
        return it->second;
    // "Arquivo:" quando o dicionário tem "Arquivo"
    const char fim = s.back();
    if (fim == ':' || fim == '.') {
        it = tab.find(s.substr(0, s.size() - 1));
        if (it != tab.end())
            return it->second + fim;
    }
    return padrao ? traduzir_linha(linha, false) : std::string();
}

// Texto inteiro: linha a linha (mantém as quebras dos avisos); se faltar alguma linha, o texto todo de uma vez.
// Os espaços das pontas ficam como no original ("Código " + número).
std::string traduzir_texto(const std::string& pt, bool padrao)
{
    const size_t ini = pt.find_first_not_of(" \t");
    if (ini == std::string::npos)
        return pt;
    const size_t      fim    = pt.find_last_not_of(" \t");
    const std::string antes  = pt.substr(0, ini), depois = pt.substr(fim + 1), miolo = pt.substr(ini, fim - ini + 1);
    std::string       out;
    if (miolo.find('\n') != std::string::npos) {
        std::string        linha;
        std::istringstream in(miolo);
        bool               tudo = true, primeira = true;
        while (std::getline(in, linha)) {
            const std::string t = traduzir_linha(linha, padrao);
            if (t.empty() && !normalizar(linha).empty())
                tudo = false;
            if (!primeira)
                out += '\n';
            out += t.empty() ? linha : t;
            primeira = false;
        }
        if (!tudo) {
            const std::string inteiro = traduzir_linha(miolo, padrao);
            if (!inteiro.empty())
                out = inteiro;
        }
    } else {
        out = traduzir_linha(miolo, padrao);
        if (out.empty())
            return pt;
    }
    return antes + out + depois;
}

std::string substituir(const std::string& modelo, const std::vector<std::string>& valores)
{
    std::string out;
    out.reserve(modelo.size() + 32);
    for (size_t i = 0; i < modelo.size(); ++i) {
        if (modelo[i] == '{') {
            size_t j = i + 1;
            while (j < modelo.size() && std::isdigit(static_cast<unsigned char>(modelo[j])))
                ++j;
            if (j > i + 1 && j < modelo.size() && modelo[j] == '}') {
                const size_t n = std::stoul(modelo.substr(i + 1, j - i - 1));
                if (n < valores.size())
                    out += valores[n];
                i = j;
                continue;
            }
        }
        out += modelo[i];
    }
    return out;
}

std::string sistema_ui()
{
#ifdef _WIN32
    wchar_t nome[LOCALE_NAME_MAX_LENGTH] = {0};
    if (::LCIDToLocaleName(MAKELCID(::GetUserDefaultUILanguage(), SORT_DEFAULT), nome, LOCALE_NAME_MAX_LENGTH, 0) > 0)
        return boost::algorithm::to_lower_copy(std::string(wxString(nome).ToUTF8().data()));
    return std::string();
#else
    const wxLanguageInfo* info = wxLocale::GetLanguageInfo(wxLocale::GetSystemLanguage());
    return info != nullptr ? boost::algorithm::to_lower_copy(std::string(info->CanonicalName.ToUTF8().data())) : std::string();
#endif
}

std::string sistema_regiao()
{
#ifdef _WIN32
    wchar_t nome[LOCALE_NAME_MAX_LENGTH] = {0};
    if (::GetUserDefaultLocaleName(nome, LOCALE_NAME_MAX_LENGTH) > 0)
        return boost::algorithm::to_lower_copy(std::string(wxString(nome).ToUTF8().data()));
#endif
    return std::string();
}

bool comeca(const std::string& s, const char* p) { return s.rfind(p, 0) == 0; }

} // namespace

bool ingles()
{
    // app_config só é lido na linha da tela; as outras linhas usam o último valor lido.
    if (wxThread::IsMain() || !g_ingles_lido.load()) {
        if (wxTheApp != nullptr && wxGetApp().app_config != nullptr) {
            g_ingles.store(comeca(wxGetApp().app_config->get("language"), "en"));
            g_ingles_lido.store(true);
        }
    }
    return g_ingles.load();
}

std::string tr(const std::string& pt)
{
    if (pt.empty() || !ingles())
        return pt;
    return traduzir_texto(pt, false);
}

wxString trw(const std::string& pt) { return wxString::FromUTF8(tr(pt)); }

std::string tr_valores(const std::string& modelo_pt, std::initializer_list<std::string> valores)
{
    const std::string modelo = ingles() ? traduzir_texto(modelo_pt, true) : modelo_pt;
    return substituir(modelo, std::vector<std::string>(valores));
}

wxString trw_valores(const std::string& modelo_pt, std::initializer_list<std::string> valores)
{
    return wxString::FromUTF8(tr_valores(modelo_pt, valores));
}

const std::string& dicionario_json() { return dicionario().texto; }

std::string roteiro_telas()
{
    // Só nas telas do próprio programa (file://); página da internet aberta na mesma janela não recebe nada.
    if (!ingles())
        return "if (location.protocol == 'file:') window.FILUNA_IDIOMA = 'pt';";
    static std::string tradutor;
    static std::once_flag uma_vez;
    std::call_once(uma_vez, [] { tradutor = ler_arquivo(pasta_i18n() / "traduzir.js"); });
    const std::string& dic = dicionario_json();
    if (tradutor.empty() || dic.size() < 3)
        return "if (location.protocol == 'file:') window.FILUNA_IDIOMA = 'en';"; // a ponte carrega o tradutor e pede o dicionário
    return "if (location.protocol == 'file:') {\nwindow.FILUNA_IDIOMA = 'en';\nwindow.FILUNA_I18N = " + dic + ";\n" + tradutor + "\n}";
}

std::string idioma_primeira_abertura()
{
    // Quem usa o Windows em português continua em português (o padrão da Filuna); em inglês, abre em inglês.
    const std::string ui = sistema_ui(), regiao = sistema_regiao();
    BOOST_LOG_TRIVIAL(info) << "Filuna: idioma do sistema " << ui << ", região " << regiao;
    if (comeca(ui, "pt"))
        return "pt_BR";
    if (comeca(ui, "en"))
        return "en";
    if (comeca(regiao, "en"))
        return "en";
    return "pt_BR";
}

std::string moeda_da_conta()
{
    AppConfig* c = wxGetApp().app_config;
    if (c == nullptr)
        return "BRL";
    std::string m = c->get("filuna_moeda");
    if (m.empty())
        m = boost::algorithm::to_upper_copy(c->get("filuna_conta_moeda"));
    return m == "USD" ? "USD" : "BRL";
}

bool idioma_automatico()
{
    AppConfig* c = wxGetApp().app_config;
    if (c == nullptr)
        return false;
    const std::string automatico = c->get("filuna_idioma_auto");
    return !automatico.empty() && automatico == c->get("language");
}

void regiao_da_conta(const std::string& regiao_bruta, const std::string& moeda)
{
    AppConfig* c = wxGetApp().app_config;
    if (c == nullptr)
        return;
    const std::string regiao = boost::algorithm::to_lower_copy(regiao_bruta);
    if (!regiao.empty())
        c->set("filuna_conta_regiao", regiao);
    // Mesma chave que as telas gravam (ponte.js lembrarMoeda); "filuna_conta_moeda" fica só como nome antigo de leitura.
    const std::string m = boost::algorithm::to_upper_copy(moeda);
    if (m == "USD" || m == "BRL")
        c->set("filuna_moeda", m);
    if (!idioma_automatico() || regiao.empty())
        return;
    c->set("filuna_idioma_auto", ""); // a conta decide uma vez só; depois vale o que o cliente escolher
    const std::string atual = c->get("language");
    const std::string alvo  = regiao == "us" ? "en" : regiao == "br" ? "pt_BR" : std::string();
    if (alvo.empty() || comeca(atual, alvo == "en" ? "en" : "pt"))
        return;
    wxGetApp().CallAfter([alvo] {
        const bool para_ingles = alvo == "en";
        MessageDialog dlg(wxGetApp().mainframe,
                          wxString::FromUTF8(para_ingles ? "Your Filuna account is in the United States. Switch Filuna Slicer to English?\n\n"
                                                           "(Sua conta Filuna é dos Estados Unidos. Mudar o Filuna Slicer para inglês?)" :
                                                           "Sua conta Filuna é do Brasil. Mudar o Filuna Slicer para português?\n\n"
                                                           "(Your Filuna account is in Brazil. Switch Filuna Slicer to Portuguese?)"),
                          wxString::FromUTF8(para_ingles ? "Language / Idioma" : "Idioma / Language"), wxYES_NO | wxICON_QUESTION);
        if (dlg.ShowModal() != wxID_YES)
            return;
        AppConfig* cfg = wxGetApp().app_config;
        const std::string antes = cfg->get("language");
        cfg->set("language", alvo);
        if (!wxGetApp().load_language(wxString::FromUTF8(alvo), false)) {
            cfg->set("language", antes);
            return;
        }
        g_ingles.store(para_ingles);
        wxGetApp().action_registry().relocalize_builtins();
        wxGetApp().recreate_GUI(_L("Changing application language"));
    });
}

}} // namespace Slic3r::Filuna
