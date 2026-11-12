#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Fórmulas do guia-v1/02-DADOS-E-FORMULAS.md, sem banco e sem interface, para poderem ser testadas
// com os exemplos numéricos do guia. FilunaDb usa estas funções; nenhuma tela recalcula por conta própria.
namespace Slic3r::Filuna {

// §5 — peso restante da bobina = soma dos deltas dos eventos não excluídos.
struct RestanteBobina
{
    double restante_g  = 0.;    // nunca negativo
    double ajuste_g    = 0.;    // > 0 quando a soma deu negativa: valor do evento 'ajuste' a gravar
    bool   divergencia = false; // consumo maior que o estoque: pedir pesagem
};
RestanteBobina restante_bobina(const std::vector<double>& deltas_g);

// §5 — evento 'pesagem': delta = (balança − tara) − restante atual.
double delta_pesagem(double peso_balanca_g, double tara_g, double restante_atual_g);

// §4 — bobina já usada cadastrada sem histórico.
double restante_por_balanca(double peso_balanca_g, double tara_g);
double restante_por_fracao(double capacidade_g, double fracao); // ¾, ½, ¼ → estimado

// §6 — rateio da purga (o G-code só dá o total). Um filamento: toda a purga é dele (não estimado).
// Vários: proporcional ao previsto, marcado como estimado.
struct RateioPurga
{
    std::vector<double> purga_g;
    bool                estimado = false;
};
RateioPurga ratear_purga(const std::vector<double>& previsto_g, double purga_total_g);

// §7 — estado final de um trabalho.
enum class FimTrabalho { Concluido, Cancelado, Falhou };

// §7 passo 1 — fração impressa. Vazio quando falta dado e é preciso perguntar ao cliente.
struct FracaoImpressa
{
    double f        = 1.;
    bool   estimado = false;
};
std::optional<FracaoImpressa> fracao_impressa(FimTrabalho                 fim,
                                              std::optional<int>          camada_parada,
                                              std::optional<int>          camadas_total,
                                              std::optional<double>       progresso_percentual,
                                              std::optional<double>       resposta_cliente = std::nullopt);

// §7 passos 2–4 — eventos de estoque que o fim do trabalho gera, por filamento.
struct EventoEstoque
{
    int         indice   = 0;  // índice do filamento no trabalho
    std::string tipo;          // 'consumo', 'purga' ou 'falha'
    double      delta_g  = 0.; // negativo (saída)
    bool        estimado = false;
};
struct ResultadoFimTrabalho
{
    std::vector<double>        real_g;
    std::vector<EventoEstoque> eventos;
    std::int64_t               tempo_real_s = 0;
};
// purga_g pode ser vazio (purga desconhecida). tempo_informado_s: o que a impressora disse, se disse.
ResultadoFimTrabalho eventos_fim_trabalho(FimTrabalho                   fim,
                                          const FracaoImpressa&         fracao,
                                          const std::vector<double>&    previsto_g,
                                          const std::vector<double>&    purga_g,
                                          bool                          purga_estimada,
                                          int                           copias_ok,
                                          std::optional<std::int64_t>   tempo_previsto_s,
                                          std::optional<std::int64_t>   tempo_informado_s);

// §8 — manutenção por horas de máquina.
enum class SituacaoManutencao { EmDia, Proxima, Vencida };
struct EstadoManutencao
{
    double             usado_h  = 0.;
    double             faltam_h = 0.;
    SituacaoManutencao situacao = SituacaoManutencao::EmDia;
};
EstadoManutencao estado_manutencao(double horas_maquina, double intervalo_h, double feito_ate_h);
const char*      situacao_manutencao_texto(SituacaoManutencao s); // "em dia", "próxima", "vencida"

// §9 — custo e desperdício.
double custo_por_grama_centavos(std::int64_t preco_centavos, double capacidade_g);
// Pode dar vazio quando não houve consumo no período.
std::optional<double> taxa_desperdicio(double desperdicio_g, double consumo_total_g);

// §10 — previsão "vai faltar filamento".
struct Previsao
{
    bool   dados_suficientes = false; // ≥ 3 trabalhos e ≥ 7 dias de histórico do tipo
    bool   sem_consumo       = false; // média diária zero
    double media_diaria_g    = 0.;
    double dias_de_estoque   = 0.;
    double necessidade_7d_g  = 0.;
    double falta_g           = 0.;
    bool   alerta            = false;
};
Previsao prever_estoque(double gasto_janela_g,
                        double estoque_tipo_g,
                        int    trabalhos_no_historico,
                        double dias_de_historico,
                        double limite_dias = 7.,
                        double janela_dias = 28.);

// §4 — sugestão de bobina para um filamento do trabalho.
struct BobinaCandidata
{
    std::string id;
    std::string filamento_tipo_id;
    std::string estado;        // 'lacrada', 'em_uso', 'vazia', 'descartada'
    std::string local;         // 'impressora:<id>:slot:<n>' ou 'prateleira:<texto>'
    std::string ultimo_uso_em; // ISO-8601, vazio se nunca
    std::string comprada_em;   // ISO-8601, vazio se desconhecido
};
// Vazio = nenhuma bobina cadastrada daquele tipo (a tela oferece cadastrar nova ou já usada).
std::optional<std::string> sugerir_bobina(const std::vector<BobinaCandidata>& bobinas,
                                          const std::string&                  filamento_tipo_id,
                                          const std::string&                  local_do_slot);

// §3 — assinatura do produto (SHA-256 em hexadecimal).
struct VolumeMalha
{
    std::vector<float>        vertices; // x,y,z em sequência
    std::vector<std::int32_t> indices;  // 3 por triângulo
};
std::string sha256_hex(const std::string& dados);
std::string assinatura_objeto(const std::vector<VolumeMalha>& volumes, double escala_x, double escala_y, double escala_z);
std::string assinatura_produto(std::vector<std::string> assinaturas_objetos);
inline std::string assinatura_catalogo(const std::string& id_catalogo) { return "catalogo:" + id_catalogo; }

} // namespace Slic3r::Filuna
