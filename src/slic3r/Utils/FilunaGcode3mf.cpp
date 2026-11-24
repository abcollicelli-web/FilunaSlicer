#include "FilunaGcode3mf.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <regex>
#include <sstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/nowide/fstream.hpp>
#include "libslic3r/Utils.hpp"
#include "libslic3r/miniz_extension.hpp"

namespace Slic3r { namespace FilunaGcode3mf {

namespace {

// "1d 2h 3m 4s" -> segundos
long segundos(const std::string& t)
{
    long        total = 0;
    std::smatch m;
    std::string resto = t;
    static const std::regex parte(R"((\d+)\s*([dhms]))");
    while (std::regex_search(resto, m, parte)) {
        const long v = std::stol(m[1].str());
        const char u = m[2].str()[0];
        total += u == 'd' ? v * 86400 : u == 'h' ? v * 3600 : u == 'm' ? v * 60 : v;
        resto = m.suffix().str();
    }
    return total;
}

std::vector<std::string> lista(const std::string& v, const char* sep)
{
    std::vector<std::string> out;
    boost::split(out, v, boost::is_any_of(sep));
    for (std::string& x : out)
        boost::trim(x);
    return out;
}

std::string xml(std::string t)
{
    boost::replace_all(t, "&", "&amp;");
    boost::replace_all(t, "\"", "&quot;");
    boost::replace_all(t, "<", "&lt;");
    boost::replace_all(t, ">", "&gt;");
    return t;
}

// O começo e o fim do arquivo: o tempo fica no cabeçalho, o consumo e a configuração no fim.
std::string pontas(const std::string& caminho)
{
    boost::nowide::ifstream f(caminho, std::ios::binary);
    if (!f)
        return {};
    f.seekg(0, std::ios::end);
    const std::streamoff tam = f.tellg();
    const std::streamoff ini = std::min<std::streamoff>(tam, 256 * 1024), fim = std::min<std::streamoff>(tam - ini, 1024 * 1024);
    std::string          s(size_t(ini + fim), '\0');
    f.seekg(0);
    f.read(&s[0], ini);
    if (fim > 0) {
        f.seekg(tam - fim);
        f.read(&s[size_t(ini)], fim);
    }
    return s;
}

} // namespace

Resumo ler_resumo(const std::string& gcode)
{
    Resumo             r;
    std::istringstream in(pontas(gcode));
    std::string        linha, tipos, cores, gramas;
    while (std::getline(in, linha)) {
        if (linha.empty() || linha[0] != ';')
            continue;
        const auto valor = [&linha](const char* chave) -> std::string {
            const auto p = linha.find(chave);
            if (p == std::string::npos)
                return {};
            const auto ig = linha.find_first_of("=:", p + std::strlen(chave));
            return ig == std::string::npos ? std::string() : boost::trim_copy(linha.substr(ig + 1));
        };
        if (r.tempo_s.empty()) {
            std::string t = valor("total estimated time");
            if (t.empty())
                t = valor("estimated printing time (normal mode)");
            if (!t.empty())
                r.tempo_s = std::to_string(segundos(t));
        }
        if (tipos.empty() && boost::starts_with(linha, "; filament_type ="))
            tipos = valor("filament_type");
        if (cores.empty() && boost::starts_with(linha, "; filament_colour ="))
            cores = valor("filament_colour");
        if (gramas.empty() && boost::starts_with(linha, "; filament used [g] ="))
            gramas = valor("filament used [g]");
        if (r.peso_g.empty() && boost::starts_with(linha, "; total filament used [g] ="))
            r.peso_g = valor("total filament used [g]");
    }
    const std::vector<std::string> t = lista(tipos, ";"), c = lista(cores, ";"), g = lista(gramas, ",");
    for (size_t i = 0; i < t.size() && !t[i].empty(); ++i) {
        const std::string usado = i < g.size() ? g[i] : std::string();
        if (!g.empty() && (usado.empty() || std::atof(usado.c_str()) <= 0.))
            continue; // filamento do perfil que esta mesa não usa
        r.filamentos.push_back({std::to_string(i + 1), t[i], i < c.size() ? c[i] : std::string(), usado.empty() ? "0" : usado});
    }
    if (r.peso_g.empty() && !r.filamentos.empty()) { // sem o total no G-code: a soma dos filamentos
        double soma = 0.;
        for (const Resumo::Fil& f : r.filamentos)
            soma += std::atof(f.gramas.c_str());
        std::ostringstream o;
        o.setf(std::ios::fixed);
        o.precision(2);
        o << soma;
        r.peso_g = o.str();
    }
    return r;
}

std::string slice_info(const Resumo& r)
{
    std::ostringstream s;
    s << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<config>\n  <header>\n"
      << "    <header_item key=\"X-BBL-Client-Type\" value=\"slicer\"/>\n  </header>\n  <plate>\n"
      << "    <metadata key=\"index\" value=\"1\"/>\n"
      << "    <metadata key=\"prediction\" value=\"" << xml(r.tempo_s.empty() ? "0" : r.tempo_s) << "\"/>\n"
      << "    <metadata key=\"weight\" value=\"" << xml(r.peso_g.empty() ? "0" : r.peso_g) << "\"/>\n"
      << "    <metadata key=\"outside\" value=\"false\"/>\n";
    for (const Resumo::Fil& f : r.filamentos) {
        s << "    <filament id=\"" << xml(f.id) << "\" type=\"" << xml(f.tipo) << "\" color=\"" << xml(f.cor) << "\" used_m=\"0\" used_g=\"" << xml(f.gramas)
          << "\"/>\n";
    }
    s << "  </plate>\n</config>\n";
    return s.str();
}

std::string embrulhar(const std::string& gcode, const std::string& destino)
{
    boost::system::error_code ec;
    if (!boost::filesystem::is_regular_file(gcode, ec))
        return "G-code não encontrado";
    std::string origem = gcode, md5;
    if (!bbl_calc_md5(origem, md5))
        return "não deu para calcular a soma do G-code";
    const std::string tipos =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
        " <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
        " <Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n"
        " <Default Extension=\"gcode\" ContentType=\"text/x.gcode\"/>\n</Types>\n";
    const std::string rels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        " <Relationship Target=\"/3D/3dmodel.model\" Id=\"rel-1\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>\n</Relationships>\n";
    const std::string modelo =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<model unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">\n"
        " <metadata name=\"Application\">Filuna Slicer</metadata>\n <resources/>\n <build/>\n</model>\n";
    const std::string info = slice_info(ler_resumo(gcode));

    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!open_zip_writer(&zip, destino))
        return "não deu para criar o arquivo para a memória interna";
    const auto mem = [&zip](const char* nome, const std::string& dados) {
        return mz_zip_writer_add_mem(&zip, nome, dados.data(), dados.size(), MZ_DEFAULT_COMPRESSION) != 0;
    };
    bool ok = mem("[Content_Types].xml", tipos) && mem("_rels/.rels", rels) && mem("3D/3dmodel.model", modelo);
    if (ok) {
        // o G-code sai do arquivo aos pedaços (pode ter centenas de MB)
        boost::nowide::ifstream f(gcode, std::ios::binary);
        ok = f && mz_zip_writer_add_read_buf_callback(
                      &zip, "Metadata/plate_1.gcode",
                      [](void* p, mz_uint64 pos, void* buf, size_t n) -> size_t {
                          auto* in = static_cast<boost::nowide::ifstream*>(p);
                          in->clear();
                          in->seekg(std::streamoff(pos));
                          in->read(static_cast<char*>(buf), std::streamsize(n));
                          return size_t(in->gcount());
                      },
                      &f, mz_uint64(boost::filesystem::file_size(gcode, ec)), nullptr, nullptr, 0, MZ_DEFAULT_COMPRESSION, nullptr, 0, nullptr, 0);
    }
    ok = ok && mem("Metadata/plate_1.gcode.md5", md5) && mem("Metadata/slice_info.config", info);
    ok = ok && mz_zip_writer_finalize_archive(&zip);
    close_zip_writer(&zip);
    if (!ok) {
        boost::filesystem::remove(destino, ec);
        return "não deu para montar o arquivo para a memória interna";
    }
    return {};
}

}} // namespace Slic3r::FilunaGcode3mf
