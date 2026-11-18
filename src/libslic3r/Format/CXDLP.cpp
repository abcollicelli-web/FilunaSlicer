#include "libslic3r/Format/CXDLP.hpp"

#include "libslic3r/SLAPrint.hpp"
#include "libslic3r/SLA/RasterBase.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/Exception.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstring>

namespace Slic3r {

namespace {

// ---------------------------------------------------------------- bytes no arquivo

// Tudo é little-endian, menos três campos do cabeçalho e o CRC do fim, que o formato guarda big-endian.
void por_u8(std::vector<uint8_t>& b, uint8_t v) { b.push_back(v); }

void por_u16(std::vector<uint8_t>& b, uint16_t v)
{
    b.push_back(uint8_t(v));
    b.push_back(uint8_t(v >> 8));
}

void por_u32(std::vector<uint8_t>& b, uint32_t v)
{
    b.push_back(uint8_t(v));
    b.push_back(uint8_t(v >> 8));
    b.push_back(uint8_t(v >> 16));
    b.push_back(uint8_t(v >> 24));
}

void por_u16_be(std::vector<uint8_t>& b, uint16_t v)
{
    b.push_back(uint8_t(v >> 8));
    b.push_back(uint8_t(v));
}

void por_u32_be(std::vector<uint8_t>& b, uint32_t v)
{
    b.push_back(uint8_t(v >> 24));
    b.push_back(uint8_t(v >> 16));
    b.push_back(uint8_t(v >> 8));
    b.push_back(uint8_t(v));
}

void por_f32(std::vector<uint8_t>& b, float v)
{
    uint32_t u;
    std::memcpy(&u, &v, 4);
    por_u32(b, u);
}

void grava_u32(std::vector<uint8_t>& b, size_t pos, uint32_t v)
{
    b[pos]     = uint8_t(v);
    b[pos + 1] = uint8_t(v >> 8);
    b[pos + 2] = uint8_t(v >> 16);
    b[pos + 3] = uint8_t(v >> 24);
}

void grava_f32(std::vector<uint8_t>& b, size_t pos, float v)
{
    uint32_t u;
    std::memcpy(&u, &v, 4);
    grava_u32(b, pos, u);
}

// ---------------------------------------------------------------- nome da máquina

// O arquivo guarda o código de fábrica da Creality ("CL-60", "CT-005"), não o nome comercial. O perfil põe esse
// código em printer_model; se vier o nome comercial, pegamos o código de dentro dele.
std::string codigo_da_maquina(const std::string& nome)
{
    if (nome.empty()) return "CL-60";
    if (nome.rfind("CL-", 0) == 0 || nome.rfind("CT-", 0) == 0 || nome.rfind("CL9", 0) == 0) return nome;

    for (size_t i = 0; i + 2 < nome.size(); ++i) {
        const char a = char(std::toupper((unsigned char) nome[i])), b = char(std::toupper((unsigned char) nome[i + 1]));
        if (a != 'C' || (b != 'L' && b != 'T')) continue;
        size_t j = i + 2;
        if (j < nome.size() && nome[j] == '-') ++j;
        const size_t ini_num = j;
        while (j < nome.size() && std::isdigit((unsigned char) nome[j])) ++j;
        if (j == ini_num) continue; // sem número: não é o código
        if (j < nome.size() && std::isalpha((unsigned char) nome[j])) ++j;
        std::string cod = nome.substr(i, j - i);
        for (char& c : cod) c = char(std::toupper((unsigned char) c));
        return cod;
    }
    return nome;
}

// ---------------------------------------------------------------- valores do perfil

double opt_f(const DynamicPrintConfig& cfg, const char* chave, double padrao)
{
    if (const ConfigOption* o = cfg.option(chave)) return o->getFloat();
    return padrao;
}

int opt_i(const DynamicPrintConfig& cfg, const char* chave, int padrao)
{
    if (const ConfigOption* o = cfg.option(chave)) return o->getInt();
    return padrao;
}

// ---------------------------------------------------------------- prévia

// A sombra do modelo (a união de todas as camadas, vista de cima) num mapa de cinza do tamanho pedido.
std::vector<uint8_t> sombra_do_modelo(const SLAPrint& print, size_t largura, size_t altura)
{
    std::vector<uint8_t> cinza(largura * altura, 0);

    ExPolygons uniao;
    for (const SLAPrint::PrintLayer& camada : print.print_layers())
        for (const ExPolygon& ex : camada.transformed_slices()) uniao.emplace_back(ex);
    if (uniao.empty()) return cinza;
    uniao = union_ex(uniao);
    if (uniao.empty()) return cinza;

    const BoundingBox caixa = get_extents(uniao);
    const double      lx    = unscaled<double>(caixa.size().x()), ly = unscaled<double>(caixa.size().y());
    if (lx <= 0. || ly <= 0.) return cinza;

    // Uma margem de 6% de cada lado para a peça não encostar na borda da miniatura.
    const double mm_por_px = std::max(lx / (double(largura) * 0.88), ly / (double(altura) * 0.88));

    sla::RasterBase::Trafo tr{sla::RasterBase::roLandscape, sla::RasterBase::NoMirror};
    tr.center_x = scaled<coord_t>(double(largura) / 2. * mm_por_px) - caixa.center().x();
    tr.center_y = scaled<coord_t>(double(altura) / 2. * mm_por_px) - caixa.center().y();

    auto rst = sla::create_raster_grayscale_aa(sla::Resolution{largura, altura}, sla::PixelDim{mm_por_px, mm_por_px}, 1., tr);
    for (const ExPolygon& ex : uniao) rst->draw(ex);

    rst->encode([&cinza](const void* p, size_t w, size_t h, size_t nc) {
        std::memcpy(cinza.data(), p, std::min(cinza.size(), w * h * nc));
        return sla::EncodedRaster({}, "raw");
    });
    return cinza;
}

// A miniatura do arquivo é colorida: a peça sai no magenta do Filuna sobre o fundo escuro.
std::vector<uint8_t> previa_colorida(const SLAPrint& print, size_t largura, size_t altura)
{
    const std::vector<uint8_t> cinza = sombra_do_modelo(print, largura, altura);
    std::vector<uint8_t>       bgr(largura * altura * 3);
    for (size_t i = 0; i < cinza.size(); ++i) {
        const double t = cinza[i] / 255.;
        bgr[i * 3 + 0] = uint8_t(0x2A + t * (0x6B - 0x2A)); // azul
        bgr[i * 3 + 1] = uint8_t(0x29 + t * (0x18 - 0x29)); // verde
        bgr[i * 3 + 2] = uint8_t(0x2E + t * (0xE0 - 0x2E)); // vermelho
    }
    return bgr;
}

// ---------------------------------------------------------------- RLE de 7 bits

void rle7_repeticao(std::vector<uint8_t>& saida, uint32_t quantas, uint8_t cor)
{
    constexpr uint32_t MAIOR = 0xfffffff;
    if (quantas == 0) return;
    if (quantas > MAIOR) {
        rle7_repeticao(saida, MAIOR, cor);
        rle7_repeticao(saida, quantas - MAIOR, cor);
        return;
    }
    saida.push_back(uint8_t(quantas > 1 ? (cor | 0x80) : cor));
    if (quantas <= 1) return;
    if (quantas <= 0x7f) {
        saida.push_back(uint8_t(quantas));
    } else if (quantas <= 0x3fff) {
        saida.push_back(uint8_t((quantas >> 8) | 0x80));
        saida.push_back(uint8_t(quantas));
    } else if (quantas <= 0x1fffff) {
        saida.push_back(uint8_t((quantas >> 16) | 0xc0));
        saida.push_back(uint8_t(quantas >> 8));
        saida.push_back(uint8_t(quantas));
    } else {
        saida.push_back(uint8_t((quantas >> 24) | 0xe0));
        saida.push_back(uint8_t(quantas >> 16));
        saida.push_back(uint8_t(quantas >> 8));
        saida.push_back(uint8_t(quantas));
    }
}

} // namespace

std::vector<uint8_t> cxdlp_rle7_codificar(const uint8_t* pixels, size_t n)
{
    std::vector<uint8_t> saida;
    saida.reserve(std::max<size_t>(64, n / 64));

    uint8_t  cor     = 0x7f; // começa diferente do preto para a primeira repetição não sair vazia
    uint32_t quantas = 0;

    size_t i = 0;
    while (i < n) {
        const uint8_t cinza7 = uint8_t(pixels[i] >> 1);
        // Os dois valores de 8 bits que dão este mesmo cinza de 7 bits.
        const uint8_t baixo = uint8_t(cinza7 << 1), alto = uint8_t(baixo | 1);
        size_t        corrida = 1;
        while (i + corrida < n && (pixels[i + corrida] == baixo || pixels[i + corrida] == alto)) ++corrida;

        if (cinza7 == cor) {
            quantas += uint32_t(corrida);
        } else {
            rle7_repeticao(saida, quantas, cor);
            cor     = cinza7;
            quantas = uint32_t(corrida);
        }
        i += corrida;
    }
    rle7_repeticao(saida, quantas, cor);
    return saida;
}

bool cxdlp_rle7_decodificar(const uint8_t* rle, size_t n, uint8_t* pixels, size_t npix)
{
    std::memset(pixels, 0, npix);
    size_t pixel = 0;
    for (size_t i = 0; i < n; ++i) {
        uint8_t  codigo = rle[i];
        uint32_t passo  = 1;

        if (codigo & 0x80) {
            codigo &= 0x7f;
            if (++i >= n) return false;
            const uint8_t s = rle[i];
            if ((s & 0x80) == 0) {
                passo = s;
            } else if ((s & 0xc0) == 0x80) {
                if (i + 1 >= n) return false;
                passo = (uint32_t(s & 0x3f) << 8) + rle[i + 1];
                i += 1;
            } else if ((s & 0xe0) == 0xc0) {
                if (i + 2 >= n) return false;
                passo = (uint32_t(s & 0x1f) << 16) + (uint32_t(rle[i + 1]) << 8) + rle[i + 2];
                i += 2;
            } else if ((s & 0xf0) == 0xe0) {
                if (i + 3 >= n) return false;
                passo = (uint32_t(s & 0x0f) << 24) + (uint32_t(rle[i + 1]) << 16) + (uint32_t(rle[i + 2]) << 8) + rle[i + 3];
                i += 3;
            } else {
                return false;
            }
        }

        if (codigo != 0) {
            // O preto não precisa ser escrito (a imagem já vem zerada) e pode passar do fim como enchimento.
            if (passo > npix - pixel) return false;
            std::memset(pixels + pixel, uint8_t((codigo << 1) | 1), passo);
        }
        pixel += passo;
    }
    return true;
}

// ---------------------------------------------------------------- prévia RGB15

std::vector<uint8_t> cxdlp_rgb15_codificar(const uint8_t* bgr, size_t largura, size_t altura)
{
    constexpr uint16_t REPETE = 0x20, LIMITE = 0xFFF;
    std::vector<uint8_t> saida;
    saida.reserve(largura * altura / 8 + 64);

    uint16_t cor = 0;
    uint32_t rep = 0;

    auto solta = [&saida](uint32_t rep, uint16_t cor) {
        if (rep == 0) return;
        if (rep <= 2) {
            for (uint32_t k = 0; k < rep; ++k) por_u16(saida, uint16_t(cor & ~REPETE));
            return;
        }
        por_u16(saida, uint16_t(cor | REPETE));
        por_u16(saida, uint16_t((rep - 1) | 0x3000));
    };

    const size_t n = largura * altura;
    for (size_t i = 0; i < n; ++i) {
        const uint8_t  b = bgr[i * 3 + 0], g = bgr[i * 3 + 1], r = bgr[i * 3 + 2];
        const uint16_t nova = uint16_t((b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11));
        if (nova == cor) {
            if (++rep == LIMITE) {
                solta(rep, cor);
                rep = 0;
            }
        } else {
            solta(rep, cor);
            cor = nova;
            rep = 1;
        }
    }
    solta(rep, cor);
    return saida;
}

// ---------------------------------------------------------------- CRC-32

uint32_t cxdlp_crc32(const uint8_t* dados, size_t n, uint32_t inicial)
{
    static uint32_t tabela[256];
    static bool     pronta = false;
    if (!pronta) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t v = i;
            for (int j = 0; j < 8; ++j) v = (v & 1) ? ((v >> 1) ^ 0xEDB88320u) : (v >> 1);
            tabela[i] = v;
        }
        pronta = true;
    }
    uint32_t c = inicial;
    for (size_t i = 0; i < n; ++i) c = tabela[uint8_t(c) ^ dados[i]] ^ (c >> 8);
    return c;
}

// ---------------------------------------------------------------- o arquivador

bool CXDLPArchive::formato_cxdlp(const SLAPrinterConfig& cfg)
{
    if (const ConfigOption* o = cfg.option("sla_archive_format")) return boost::iequals(o->serialize(), "CXDLPV4");
    return false;
}

const char* CXDLPArchive::extensao(const SLAPrinterConfig& cfg) { return formato_cxdlp(cfg) ? ".cxdlpv4" : ".sl1"; }

void CXDLPArchive::apply(const SLAPrinterConfig& cfg)
{
    SL1Archive::apply(cfg); // mantém o raster do SL1 em dia (e limpa as camadas quando algo muda)
    auto diff = m_cfg.diff(cfg);
    if (!diff.empty()) m_cfg.apply_only(cfg, diff);
    m_cxdlp = formato_cxdlp(cfg);
}

sla::RasterEncoder CXDLPArchive::get_encoder() const
{
    if (!m_cxdlp) return SL1Archive::get_encoder();

    return [](const void* ptr, size_t w, size_t h, size_t num_components) {
        return sla::EncodedRaster(cxdlp_rle7_codificar(static_cast<const uint8_t*>(ptr), w * h * num_components), "cxdlp");
    };
}

void CXDLPArchive::exportar(const std::string& caminho, const SLAPrint& print) const
{
    const DynamicPrintConfig& cfg = print.full_print_config();

    const uint32_t n_camadas = uint32_t(m_layers.size());
    if (n_camadas == 0) throw Slic3r::ExportError("Nada para gravar: o fatiamento não deixou nenhuma camada.");

    // --- tela e mesa, já na orientação em que o raster foi desenhado
    double larg_mm = m_cfg.display_width.getFloat(), alt_mm = m_cfg.display_height.getFloat();
    auto   px_x = uint32_t(m_cfg.display_pixels_x.getInt()), px_y = uint32_t(m_cfg.display_pixels_y.getInt());
    if (m_cfg.display_orientation.getInt() == sla::RasterBase::roPortrait) {
        std::swap(larg_mm, alt_mm);
        std::swap(px_x, px_y);
    }

    // --- valores do perfil
    const double altura_camada = opt_f(cfg, "layer_height", 0.05);
    const double exp_normal    = opt_f(cfg, "exposure_time", 2.5);
    const double exp_base      = opt_f(cfg, "initial_exposure_time", 30.);
    const int    n_base        = std::max(0, opt_i(cfg, "sla_bottom_layers", 5));
    const int    n_transicao   = std::max(0, opt_i(cfg, "faded_layers", 0));

    const double subida        = opt_f(cfg, "sla_lift_height", 6.);
    const double subida_vel    = opt_f(cfg, "sla_lift_speed", 80.);
    const double subida_base   = opt_f(cfg, "sla_bottom_lift_height", 7.);
    const double subida_base_v = opt_f(cfg, "sla_bottom_lift_speed", 80.);
    const double descida_vel   = opt_f(cfg, "sla_retract_speed", 150.);
    const int    luz           = opt_i(cfg, "sla_light_pwm", 255);
    const int    luz_base      = opt_i(cfg, "sla_bottom_light_pwm", 255);
    const double pausa         = opt_f(cfg, "sla_light_off_delay", 0.);
    const double pausa_base    = opt_f(cfg, "sla_bottom_light_off_delay", 0.);
    const double espera_antes  = opt_f(cfg, "sla_rest_time_before_lift", 0.);
    const double espera_depois = opt_f(cfg, "sla_rest_time_after_lift", 0.);

    const SLAPrintStatistics& est = print.print_statistics();
    const double              ml  = (est.objects_used_material + est.support_used_material) / 1000.;
    // O motor nem sempre preenche o peso; nesse caso sai da densidade da resina do perfil.
    const double gramas = est.total_weight > 0. ? est.total_weight : ml * opt_f(cfg, "material_density", 1.1);

    std::string modelo_z = codigo_da_maquina(cfg.has("printer_model") ? cfg.opt_string("printer_model") : std::string());
    modelo_z.push_back('\0');

    // --- cabeçalho: gravado no fim, mas o tamanho já é conhecido (99 bytes mais o nome da máquina)
    const size_t tam_cabecalho = 99 + modelo_z.size();

    std::vector<uint8_t> arq;
    arq.reserve(size_t(16) << 20);
    arq.resize(tam_cabecalho, 0);

    // --- as duas miniaturas
    uint32_t previa_peq = 0, previa_gra = 0;
    const struct { size_t w, h; } tamanhos[2] = {{120, 120}, {300, 300}};
    for (int i = 0; i < 2; ++i) {
        const std::vector<uint8_t> bgr   = previa_colorida(print, tamanhos[i].w, tamanhos[i].h);
        const std::vector<uint8_t> bytes = cxdlp_rgb15_codificar(bgr.data(), tamanhos[i].w, tamanhos[i].h);
        if (bytes.empty()) continue;

        (i == 0 ? previa_peq : previa_gra) = uint32_t(arq.size());
        por_u32(arq, uint32_t(tamanhos[i].w));
        por_u32(arq, uint32_t(tamanhos[i].h));
        por_u32(arq, uint32_t(arq.size() + 24)); // os bytes começam logo depois dos 8 campos desta ficha
        por_u32(arq, uint32_t(bytes.size()));
        for (int k = 0; k < 4; ++k) por_u32(arq, 0); // quatro campos que o formato não usa
        arq.insert(arq.end(), bytes.begin(), bytes.end());
    }

    // --- parâmetros de impressão
    const uint32_t end_parametros = uint32_t(arq.size());
    por_f32(arq, float(subida_base));
    por_f32(arq, float(subida_base_v));
    por_f32(arq, float(subida));
    por_f32(arq, float(subida_vel));
    por_f32(arq, float(descida_vel));
    por_f32(arq, float(ml));
    por_f32(arq, float(gramas));
    por_f32(arq, float(est.total_cost));
    por_f32(arq, float(pausa_base));
    por_f32(arq, float(pausa));
    por_u32(arq, uint32_t(n_base));
    por_f32(arq, float(exp_normal));
    por_f32(arq, float(exp_base));
    for (int k = 0; k < 4; ++k) por_u32(arq, 0);
    const uint32_t tam_parametros = uint32_t(arq.size()) - end_parametros;

    // --- dados do fatiador
    const uint32_t end_fatiador = uint32_t(arq.size());
    por_f32(arq, 0.f); // segunda subida da base: não usamos
    por_f32(arq, 0.f);
    por_f32(arq, 0.f); // segunda subida normal
    por_f32(arq, 0.f);
    por_f32(arq, 0.f); // segunda descida
    por_f32(arq, 0.f);
    por_f32(arq, float(espera_depois));
    por_u32(arq, 0); // ajustes por camada: não usamos
    const auto agora = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::system_clock::now().time_since_epoch());
    por_u32(arq, uint32_t(agora.count()));
    por_u32(arq, 1); // suavização
    por_u32(arq, 0); // versão do programa que gerou
    por_f32(arq, float(espera_antes));
    por_f32(arq, float(espera_antes));
    por_f32(arq, float(exp_normal));
    por_f32(arq, float(exp_base));
    por_f32(arq, float(espera_depois));
    por_u32(arq, uint32_t(n_transicao));
    por_u32(arq, 0);
    por_u32(arq, 0);
    const uint32_t tam_fatiador = uint32_t(arq.size()) - end_fatiador;

    // --- as camadas: a tabela vem antes dos dados e só é preenchida no fim
    constexpr uint32_t TAM_LINHA = 40, TAM_LINHA_EX = 44;
    const uint32_t     end_tabela = uint32_t(arq.size());
    arq.resize(arq.size() + size_t(TAM_LINHA) * n_camadas, 0);

    const auto& camadas = print.print_layers();

    for (uint32_t i = 0; i < n_camadas; ++i) {
        const bool   base = int(i) < n_base;
        const double z    = i < camadas.size() ? unscaled<double>(camadas[i].level()) : altura_camada * (i + 1);

        const uint32_t endereco = uint32_t(arq.size());

        // cabeçalho da camada (44 bytes) seguido do RLE
        por_f32(arq, float(base ? subida_base : subida));
        por_f32(arq, float(base ? subida_base_v : subida_vel));
        por_f32(arq, 0.f); // segunda subida
        por_f32(arq, 0.f);
        por_f32(arq, float(descida_vel));
        por_f32(arq, 0.f); // segunda descida
        por_f32(arq, 0.f);
        por_f32(arq, float(espera_antes));
        por_f32(arq, float(espera_depois));
        por_f32(arq, float(espera_antes));
        por_f32(arq, float(base ? luz_base : luz));

        const auto* p = static_cast<const uint8_t*>(m_layers[i].data());
        arq.insert(arq.end(), p, p + m_layers[i].size());

        const size_t pos = end_tabela + size_t(TAM_LINHA) * i;
        grava_f32(arq, pos, float(z));
        grava_f32(arq, pos + 4, float(base ? exp_base : exp_normal));
        grava_f32(arq, pos + 8, float(base ? pausa_base : pausa));
        grava_u32(arq, pos + 12, endereco);
        grava_u32(arq, pos + 16, uint32_t(m_layers[i].size()) + TAM_LINHA_EX);
        grava_u32(arq, pos + 20, 0); // tipo dos dados: 0 = RLE de 7 bits
        grava_u32(arq, pos + 24, 0); // distância do centro e maior área: a impressora não precisa
        grava_u32(arq, pos + 28, 0);
        grava_u32(arq, pos + 32, 0);
        grava_u32(arq, pos + 36, 0);
    }

    // --- cabeçalho
    std::vector<uint8_t> cab;
    cab.reserve(tam_cabecalho);
    por_u32_be(cab, 9);
    const char* magico = "CXSW3DV2";
    for (int i = 0; i < 8; ++i) por_u8(cab, uint8_t(magico[i]));
    por_u8(cab, 0);
    por_u16_be(cab, 4);
    por_u32_be(cab, uint32_t(modelo_z.size()));
    for (char c : modelo_z) por_u8(cab, uint8_t(c));
    por_u16(cab, uint16_t(px_x));
    por_u16(cab, uint16_t(px_y));
    por_f32(cab, float(larg_mm));
    por_f32(cab, float(alt_mm));
    por_f32(cab, float(m_cfg.printable_height.getFloat()));
    por_f32(cab, float(n_camadas > 0 && !camadas.empty() ? unscaled<double>(camadas.back().level()) : altura_camada * n_camadas));
    por_f32(cab, float(altura_camada));
    por_u32(cab, uint32_t(n_base));
    por_u32(cab, previa_peq);
    por_u32(cab, end_tabela);
    por_u32(cab, n_camadas);
    por_u32(cab, previa_gra);
    por_u32(cab, uint32_t(std::lround(est.estimated_print_time)));
    por_u32(cab, 0); // projeção normal: o espelho do perfil já entrou no desenho
    por_u32(cab, end_parametros);
    por_u32(cab, tam_parametros);
    por_u32(cab, 1); // suavização
    por_u16(cab, uint16_t(luz));
    por_u16(cab, uint16_t(luz_base));
    por_u32(cab, 0); // sem embaralhamento dos dados
    por_u32(cab, end_fatiador);
    por_u32(cab, tam_fatiador);

    if (cab.size() != tam_cabecalho) throw Slic3r::ExportError("O cabeçalho do .cxdlpv4 saiu com o tamanho errado.");
    std::copy(cab.begin(), cab.end(), arq.begin());

    // --- conferência no fim: CRC-32 do arquivo inteiro, big-endian
    por_u32_be(arq, cxdlp_crc32(arq.data(), arq.size()));

    boost::filesystem::ofstream saida(boost::filesystem::path(caminho), std::ios::binary | std::ios::trunc);
    if (!saida.good()) throw Slic3r::ExportError(std::string("Não consegui gravar o arquivo ") + caminho);
    saida.write(reinterpret_cast<const char*>(arq.data()), std::streamsize(arq.size()));
    saida.close();
    if (!saida.good()) throw Slic3r::ExportError(std::string("Falhou ao terminar de gravar ") + caminho);

    BOOST_LOG_TRIVIAL(info) << "cxdlpv4 gravado: " << caminho << " (" << n_camadas << " camadas, " << arq.size() << " bytes)";
}

} // namespace Slic3r
