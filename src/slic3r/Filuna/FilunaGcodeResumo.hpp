#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Slic3r::Filuna {

// Resumo que o Orca escreve em comentários no fim (e às vezes no início) do G-code
// (guia-v1/02-DADOS §6). Campo que o G-code não traz fica vazio: desconhecido, nunca estimado.
struct GcodeResumo
{
    std::vector<double>          filamento_g;       // "; filament used [g] = a, b"  (por índice de filamento)
    std::vector<double>          filamento_mm;      // "; filament used [mm] = a, b" (conferência)
    std::optional<double>        total_g;           // "; total filament used [g] = x"
    std::optional<double>        purga_total_g;     // "; total filament used for wipe tower [g] = x"
    std::vector<double>          purga_g;           // purga por filamento, quando a fonte informa (estatística do fatiamento)
    std::optional<int>           trocas_filamento;  // "; total filament change = n"
    std::optional<std::int64_t>  tempo_previsto_s;  // "; estimated printing time (normal mode) = 1h 45m 3s"
    std::optional<int>           camadas_total;     // "; total layers count = n"

    // Algum campo do resumo foi encontrado.
    bool tem_dados() const;
};

// "1d 2h 3m 4s", "1h 45m 3s", "12m 5s", "40s" → segundos. Texto sem nenhuma parte reconhecida → vazio.
std::optional<std::int64_t> tempo_dhms_em_segundos(std::string_view texto);

// Lê o resumo de um trecho de texto de G-code. Só olha linhas de comentário (começam com ';').
// Quando um campo aparece mais de uma vez, vale a última ocorrência (o fim do arquivo é o resumo final).
GcodeResumo ler_resumo_gcode(std::string_view texto);

// Lê os primeiros e os últimos 64 KB do arquivo e junta os campos (o fim tem prioridade).
// Arquivo inexistente ou ilegível → vazio.
std::optional<GcodeResumo> ler_resumo_gcode_arquivo(const std::string& caminho_utf8);

} // namespace Slic3r::Filuna
