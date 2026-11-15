#include "FilunaAutoajuda.hpp"

#include "GUI_App.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"

#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <nlohmann/json.hpp>

#include <map>
#include <mutex>

namespace Slic3r { namespace GUI { namespace FilunaAutoajuda {

namespace {

const std::map<std::string, wxString>& cartoes()
{
    static std::map<std::string, wxString> m;
    static std::once_flag                  uma_vez;
    std::call_once(uma_vez, [] {
        try {
            const boost::filesystem::path p = boost::filesystem::path(resources_dir()) / "filuna" / "autoajuda" / "pt_BR.json";
            boost::nowide::ifstream       in(p.string());
            const nlohmann::json          j = nlohmann::json::parse(in, nullptr, false);
            if (!j.is_object())
                return;
            for (auto it = j.begin(); it != j.end(); ++it) {
                if (!it.value().is_object())
                    continue;
                const std::string titulo = it.value().value("titulo", std::string()), oque = it.value().value("oque", std::string()),
                                  efeito = it.value().value("efeito", std::string());
                if (titulo.empty() || oque.empty())
                    continue;
                std::string t = titulo + "\n\n" + oque;
                if (!efeito.empty())
                    t += "\n\n" + efeito;
                m[it.key()] = wxString::FromUTF8(t);
            }
        } catch (const std::exception&) {}
    });
    return m;
}

} // namespace

bool ligada()
{
    AppConfig* cfg = wxGetApp().app_config;
    return cfg == nullptr || cfg->get("filuna_pref_interface_autoajuda") != "false";
}

void ligar(bool sim)
{
    if (AppConfig* cfg = wxGetApp().app_config)
        cfg->set("filuna_pref_interface_autoajuda", sim ? "true" : "false");
}

wxString cartao(const std::string& opt_key)
{
    if (!ligada())
        return wxString();
    // "nozzle_temperature#0" -> "nozzle_temperature"
    const std::string chave = opt_key.substr(0, opt_key.find('#'));
    const auto&       m     = cartoes();
    const auto        it    = m.find(chave);
    return it == m.end() ? wxString() : it->second;
}

std::string cartao_ferramenta(const std::string& chave)
{
    if (!ligada())
        return {};
    std::string k = chave;
    for (const char* fim : {".svg", "_dark"}) {
        const std::string f(fim);
        if (k.size() > f.size() && k.compare(k.size() - f.size(), f.size(), f) == 0)
            k.resize(k.size() - f.size());
    }
    const auto& m  = cartoes();
    const auto  it = m.find(k);
    return it == m.end() ? std::string() : std::string(it->second.ToUTF8().data());
}

}}} // namespace Slic3r::GUI::FilunaAutoajuda
