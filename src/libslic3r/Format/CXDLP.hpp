#ifndef slic3r_Format_CXDLP_hpp_
#define slic3r_Format_CXDLP_hpp_

// Filuna — arquivo de resina da Creality (.cxdlpv4, o "CXSW3DV2" versão 4), o que as HALOT leem do pendrive.
// O formato foi levantado a partir do UVtools (AGPL-3.0, UVtools.Core/FileFormats/CrealityCXDLPv4File.cs) e
// reescrito aqui em C++.
//
// O desenho das camadas continua sendo o do motor SLA que já vem do PrusaSlicer (mesmo raster, mesmo espelho, mesma
// orientação). Muda só como cada camada é guardada (RLE de 7 bits de cinza em vez de PNG) e o arquivo final (um
// binário só, em vez do zip do .sl1). A opção de impressora "sla_archive_format" escolhe entre os dois.

#include "libslic3r/Format/SL1.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Slic3r {

class CXDLPArchive : public SL1Archive
{
    SLAPrinterConfig m_cfg;      // cópia nossa: a do SL1Archive é privada
    bool             m_cxdlp{false};

protected:
    sla::RasterEncoder get_encoder() const override;

public:
    CXDLPArchive() = default;
    explicit CXDLPArchive(const SLAPrinterConfig& cfg) : SL1Archive(cfg), m_cfg(cfg) { m_cxdlp = formato_cxdlp(cfg); }

    void apply(const SLAPrinterConfig& cfg) override;

    // true quando a impressora escolhida pede o .cxdlpv4
    bool cxdlp() const { return m_cxdlp; }

    // Grava o binário .cxdlpv4 com as camadas que o SLAPrint já desenhou (passo slapsRasterize).
    void exportar(const std::string& caminho, const SLAPrint& print) const;

    static bool        formato_cxdlp(const SLAPrinterConfig& cfg);
    static const char* extensao(const SLAPrinterConfig& cfg);
};

// RLE de 7 bits de cinza (o mesmo do CTB da Chitubox e do CXDLP v4). Exposto para o teste do laboratório.
std::vector<uint8_t> cxdlp_rle7_codificar(const uint8_t* pixels, size_t n);
// Volta ao mapa de cinza; devolve false se os dados estiverem truncados ou passarem do tamanho da imagem.
bool cxdlp_rle7_decodificar(const uint8_t* rle, size_t n, uint8_t* pixels, size_t npix);

// Prévia da Chitubox (RGB15 com repetição), usada nas duas miniaturas do arquivo.
std::vector<uint8_t> cxdlp_rgb15_codificar(const uint8_t* rgb, size_t largura, size_t altura);

// CRC-32 (polinômio 0xEDB88320, início 0, sem xor no fim) do arquivo inteiro; vai nos 4 últimos bytes, big-endian.
uint32_t cxdlp_crc32(const uint8_t* dados, size_t n, uint32_t inicial = 0);

} // namespace Slic3r

#endif // slic3r_Format_CXDLP_hpp_
