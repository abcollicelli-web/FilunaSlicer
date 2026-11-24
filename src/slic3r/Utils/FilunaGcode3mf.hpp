#pragma once
// G-code solto -> .gcode.3mf de uma mesa (Metadata/plate_1.gcode, a soma MD5 e o slice_info.config).
//
// A Bambu sem cartão (P2S, H2D...) só imprime da memória interna o que chega pela porta 6000 como .gcode.3mf e
// é iniciado com project_file "Metadata/plate_1.gcode". A Farm, a impressão de teste e o envio direto do Filuna
// geram um .gcode solto; este empacotamento deixa todos esses caminhos irem para a memória interna.
#include <string>
#include <vector>

namespace Slic3r { namespace FilunaGcode3mf {

// Cria <destino> com o G-code de <gcode> como a mesa 1. Devolve vazio se deu certo, ou o motivo da falha.
std::string embrulhar(const std::string& gcode, const std::string& destino);

// O resumo que vai no slice_info.config, lido dos comentários do próprio G-code (exposto para os testes).
struct Resumo
{
    std::string tempo_s, peso_g;
    struct Fil { std::string id, tipo, cor, gramas; };
    std::vector<Fil> filamentos;
};
Resumo ler_resumo(const std::string& gcode);
std::string slice_info(const Resumo& r);

}} // namespace Slic3r::FilunaGcode3mf
