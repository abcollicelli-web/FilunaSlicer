#include "FilunaGcodeResumo.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/nowide/convert.hpp>

#include <cctype>
#include <clocale>
#include <cstdlib>
#include <ios>

namespace Slic3r::Filuna {

namespace {

constexpr std::size_t TRECHO_BYTES = 64 * 1024;

std::string_view aparar(std::string_view s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

// Número com ponto decimal, independente da localidade do sistema (o Orca sempre escreve com ponto).
std::optional<double> ler_numero(std::string_view s)
{
    s = aparar(s);
    if (s.empty())
        return std::nullopt;
    std::string txt(s);
    for (char c : txt)
        if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E'))
            return std::nullopt;
    // strtod depende do separador da localidade; troca o ponto pelo separador atual se preciso.
    const char sep = *std::localeconv()->decimal_point;
    if (sep != '.')
        for (char& c : txt)
            if (c == '.')
                c = sep;
    char* fim = nullptr;
    const double v = std::strtod(txt.c_str(), &fim);
    if (fim == txt.c_str() || *fim != '\0')
        return std::nullopt;
    return v;
}

std::vector<double> ler_lista(std::string_view s)
{
    std::vector<double> out;
    while (true) {
        const auto virgula = s.find(',');
        const auto item    = ler_numero(s.substr(0, virgula));
        if (!item)
            return {};
        out.push_back(*item);
        if (virgula == std::string_view::npos)
            break;
        s.remove_prefix(virgula + 1);
    }
    return out;
}

void juntar(GcodeResumo& destino, const GcodeResumo& fonte)
{
    if (!fonte.filamento_g.empty())  destino.filamento_g  = fonte.filamento_g;
    if (!fonte.filamento_mm.empty()) destino.filamento_mm = fonte.filamento_mm;
    if (fonte.total_g)               destino.total_g = fonte.total_g;
    if (fonte.purga_total_g)         destino.purga_total_g = fonte.purga_total_g;
    if (fonte.trocas_filamento)      destino.trocas_filamento = fonte.trocas_filamento;
    if (fonte.tempo_previsto_s)      destino.tempo_previsto_s = fonte.tempo_previsto_s;
    if (fonte.camadas_total)         destino.camadas_total = fonte.camadas_total;
}

} // namespace

bool GcodeResumo::tem_dados() const
{
    return !filamento_g.empty() || !filamento_mm.empty() || total_g || purga_total_g || trocas_filamento || tempo_previsto_s ||
           camadas_total;
}

std::optional<std::int64_t> tempo_dhms_em_segundos(std::string_view texto)
{
    std::int64_t total = 0;
    bool         achou = false;
    std::size_t  i     = 0;
    while (i < texto.size()) {
        while (i < texto.size() && !std::isdigit(static_cast<unsigned char>(texto[i])))
            ++i;
        if (i >= texto.size())
            break;
        const std::size_t ini = i;
        while (i < texto.size() && (std::isdigit(static_cast<unsigned char>(texto[i])) || texto[i] == '.'))
            ++i;
        const auto valor = ler_numero(texto.substr(ini, i - ini));
        if (!valor || i >= texto.size())
            return std::nullopt;
        std::int64_t mult = 0;
        switch (texto[i]) {
        case 'd': mult = 86400; break;
        case 'h': mult = 3600; break;
        case 'm': mult = 60; break;
        case 's': mult = 1; break;
        default: return std::nullopt;
        }
        total += static_cast<std::int64_t>(*valor) * mult;
        achou = true;
        ++i;
    }
    if (!achou)
        return std::nullopt;
    return total;
}

GcodeResumo ler_resumo_gcode(std::string_view texto)
{
    GcodeResumo r;
    while (!texto.empty()) {
        const auto  fim   = texto.find('\n');
        std::string_view linha = aparar(texto.substr(0, fim));
        texto.remove_prefix(fim == std::string_view::npos ? texto.size() : fim + 1);
        if (linha.empty() || linha.front() != ';')
            continue;
        linha.remove_prefix(1);
        const auto igual = linha.find('=');
        if (igual == std::string_view::npos)
            continue;
        const std::string_view chave = aparar(linha.substr(0, igual));
        const std::string_view valor = aparar(linha.substr(igual + 1));

        if (chave == "filament used [g]") {
            if (auto l = ler_lista(valor); !l.empty()) r.filamento_g = std::move(l);
        } else if (chave == "filament used [mm]") {
            if (auto l = ler_lista(valor); !l.empty()) r.filamento_mm = std::move(l);
        } else if (chave == "total filament used [g]") {
            if (auto v = ler_numero(valor)) r.total_g = v;
        } else if (chave == "total filament used for wipe tower [g]") {
            if (auto v = ler_numero(valor)) r.purga_total_g = v;
        } else if (chave == "total filament change") {
            if (auto v = ler_numero(valor)) r.trocas_filamento = static_cast<int>(*v);
        } else if (chave == "total layers count") {
            if (auto v = ler_numero(valor)) r.camadas_total = static_cast<int>(*v);
        } else if (chave == "estimated printing time (normal mode)") {
            if (auto v = tempo_dhms_em_segundos(valor)) r.tempo_previsto_s = v;
        }
    }
    return r;
}

std::optional<GcodeResumo> ler_resumo_gcode_arquivo(const std::string& caminho_utf8)
{
    boost::filesystem::ifstream f(boost::filesystem::path(boost::nowide::widen(caminho_utf8)), std::ios::binary);
    if (!f)
        return std::nullopt;
    f.seekg(0, std::ios::end);
    const std::streamoff tamanho = f.tellg();
    if (tamanho < 0)
        return std::nullopt;

    auto ler = [&f](std::streamoff de, std::size_t n) {
        std::string buf(n, '\0');
        f.seekg(de, std::ios::beg);
        f.read(buf.data(), static_cast<std::streamsize>(n));
        buf.resize(static_cast<std::size_t>(f.gcount()));
        f.clear();
        return buf;
    };

    const std::size_t total = static_cast<std::size_t>(tamanho);
    GcodeResumo r;
    if (total <= 2 * TRECHO_BYTES) {
        r = ler_resumo_gcode(ler(0, total));
    } else {
        r = ler_resumo_gcode(ler(0, TRECHO_BYTES));
        std::string fim = ler(static_cast<std::streamoff>(total - TRECHO_BYTES), TRECHO_BYTES);
        // A primeira linha do trecho final pode estar cortada no meio: descarta.
        if (const auto nl = fim.find('\n'); nl != std::string::npos)
            fim.erase(0, nl + 1);
        juntar(r, ler_resumo_gcode(fim));
    }
    return r;
}

} // namespace Slic3r::Filuna
