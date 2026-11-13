#pragma once

#include "FilunaCalculos.hpp"
#include "FilunaGcodeResumo.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;

namespace Slic3r::Filuna {

// Banco local único do Filuna (guia-v1/02-DADOS §1–§2): <data_dir>/filuna/filuna.db.
// É a única porta de acesso ao SQLite; nenhuma tela faz SQL direto. Cada gravação entra na
// fila de sincronização (sync_fila). Erros de banco viram std::runtime_error.
class FilunaDb
{
public:
    using Relogio = std::function<std::string()>; // agora em UTC ISO-8601 ("2026-11-12T21:40:00Z")

    // Abre ou cria o banco e aplica as migrações pendentes. ":memory:" serve para testes.
    static std::unique_ptr<FilunaDb> abrir(const std::string& caminho_utf8, Relogio relogio = {});
    ~FilunaDb();
    FilunaDb(const FilunaDb&)            = delete;
    FilunaDb& operator=(const FilunaDb&) = delete;

    static int versao_esquema_atual();
    int        versao_esquema() const;
    static std::string novo_uuid();
    static std::string agora_utc();

    // --- Impressoras e manutenção (§8) ---
    std::string criar_impressora(const std::string& nome, const std::string& fabricante, const std::string& modelo,
                                 const std::string& numero_serie = {}, const std::string& modulo_id = {});
    std::string adicionar_regra_manutencao(const std::string& impressora_id, const std::string& nome, double intervalo_h,
                                           double feito_ate_h = 0.);
    double      horas_maquina(const std::string& impressora_id) const;
    struct RegraManutencao
    {
        std::string      id;
        std::string      nome;
        double           intervalo_h = 0.;
        double           feito_ate_h = 0.;
        EstadoManutencao estado;
    };
    std::vector<RegraManutencao> manutencao(const std::string& impressora_id) const;
    void                         registrar_manutencao_feita(const std::string& regra_id, const std::string& obs = {});

    // --- Filamentos e bobinas (§4, §5) ---
    struct FilamentoTipo
    {
        std::string marca, material, nome_produto, cor_nome, cor_hex, preset_orca;
        double      diametro_mm     = 1.75;
        double      densidade_g_cm3 = 0.;
    };
    std::string criar_filamento_tipo(const FilamentoTipo& t);
    // Reaproveita o tipo com a mesma marca, material, cor e diâmetro (sem diferenciar maiúsculas) ou cria um.
    std::string tipo_igual_ou_novo(const FilamentoTipo& t);
    struct TipoCadastrado : FilamentoTipo
    {
        std::string id;
    };
    std::vector<TipoCadastrado>   tipos() const;
    std::optional<TipoCadastrado> tipo(const std::string& id) const;

    // Cadastro em lote: N bobinas 'lacrada', cada uma com código B-0001… e evento 'entrada'.
    std::vector<std::string> cadastrar_bobinas(const std::string& filamento_tipo_id, int quantidade, double capacidade_g,
                                               double tara_g = 0., std::int64_t preco_centavos = 0,
                                               const std::string& comprada_em = {});
    // Bobina já usada sem histórico: restante pela balança ou pela fração (estimado).
    std::string cadastrar_bobina_usada(const std::string& filamento_tipo_id, double capacidade_g, double tara_g,
                                       double restante_g, bool estimado, std::int64_t preco_centavos = 0);
    void abrir_bobina(const std::string& bobina_id);
    void definir_local(const std::string& bobina_id, const std::string& local);
    void pesar_bobina(const std::string& bobina_id, double peso_balanca_g);
    void registrar_evento(const std::string& bobina_id, const std::string& tipo, double delta_g,
                          const std::string& trabalho_id = {}, bool estimado = false, const std::string& obs = {});

    struct Bobina
    {
        std::string  id, filamento_tipo_id, codigo, estado, local, ultimo_uso_em, comprada_em;
        double       capacidade_g = 0., tara_g = 0., restante_g = 0.;
        std::int64_t preco_centavos = 0;
        bool         estimado       = false;
    };
    std::optional<Bobina> bobina(const std::string& bobina_id) const;
    // Todas as bobinas não excluídas; com incluir_inativas=false ficam só 'lacrada' e 'em_uso'.
    std::vector<Bobina>   bobinas(bool incluir_inativas = false) const;
    // Arquivar: estado 'descartada' e evento 'descarte' com o que restava.
    void                  descartar_bobina(const std::string& bobina_id, const std::string& obs = {});
    struct Evento
    {
        std::string id, tipo, criado_em, trabalho_id, obs;
        double      delta_g  = 0.;
        bool        estimado = false;
    };
    std::vector<Evento> eventos_da_bobina(const std::string& bobina_id) const;
    std::vector<Bobina>   bobinas_do_tipo(const std::string& filamento_tipo_id) const;
    // Sugestão do diálogo "Confirmar bobinas".
    std::optional<std::string> sugerir_bobina(const std::string& filamento_tipo_id, const std::string& local_do_slot) const;

    // --- Produtos e trabalhos (§3, §6, §7) ---
    // Reutiliza o produto com a mesma assinatura ou cria um novo.
    std::string produto_por_assinatura(const std::string& assinatura, const std::string& nome);
    // Une o produto 'de' ao 'para': trabalhos passam a apontar para 'para' e 'de' fica excluído.
    void unir_produtos(const std::string& de, const std::string& para);
    int  trabalhos_concluidos_do_produto(const std::string& produto_id) const;
    struct ProdutoRanking
    {
        std::string id, nome;
        int         concluidos = 0;
        double      gramas     = 0.;
    };
    std::vector<ProdutoRanking> produtos() const; // mais impressos primeiro
    struct TrabalhoLista
    {
        std::string                 id, produto_id, produto_nome, impressora_id, impressora_nome, estado, criado_em, terminado_em;
        std::optional<std::int64_t> tempo_previsto_s, tempo_real_s;
        std::optional<int>          camadas_total;
        double                      previsto_g = 0., real_g = 0.;
        bool                        estimado   = false;
        std::vector<std::string>    bobinas; // id por índice de filamento ('' = sem bobina)
    };
    std::vector<TrabalhoLista> trabalhos(int limite = 100) const;
    struct ImpressoraCadastrada
    {
        std::string id, nome, fabricante, modelo;
        double      horas = 0.;
    };
    std::vector<ImpressoraCadastrada> impressoras() const;
    // Reaproveita a impressora com o mesmo nome ou cria uma.
    std::string impressora_por_nome(const std::string& nome, const std::string& fabricante = {}, const std::string& modelo = {});

    // Cria o trabalho 'fatiado' com uma linha por filamento a partir do resumo do G-code.
    // A purga rateada só é gravada quando o G-code informou o total (senão fica desconhecida).
    std::string criar_trabalho(const std::string& produto_id, const std::string& impressora_id, const GcodeResumo& resumo,
                               int copias = 1, const std::string& gcode_sha256 = {});
    void        confirmar_bobina(const std::string& trabalho_id, int indice, const std::string& bobina_id);
    void        marcar_estado(const std::string& trabalho_id, const std::string& estado);
    // Fecha o trabalho e grava os eventos nas bobinas confirmadas (tudo em uma transação).
    ResultadoFimTrabalho finalizar_trabalho(const std::string& trabalho_id, FimTrabalho fim, const FracaoImpressa& fracao,
                                            std::optional<std::int64_t> tempo_informado_s = std::nullopt,
                                            std::optional<int>          camada_parada     = std::nullopt);

    // --- Painel (§9, §10) ---
    struct Desperdicio
    {
        double                desperdicio_g   = 0.;
        double                consumo_total_g = 0.;
        std::optional<double> taxa;
    };
    // Período [desde, ate) em ISO-8601 UTC.
    Desperdicio desperdicio(const std::string& desde, const std::string& ate) const;
    Previsao    previsao_tipo(const std::string& filamento_tipo_id, double limite_dias = 7.) const;

    // --- Sincronização (03-SERVIDOR §4) ---
    int pendentes_sincronizacao() const;

private:
    explicit FilunaDb(sqlite3* db, Relogio relogio);
    void        executar(const std::string& sql) const;
    void        migrar();
    std::string agora() const;
    void        enfileirar(const std::string& tabela, const std::string& id, const std::string& op);
    void        recalcular_bobina(const std::string& bobina_id);
    void        inserir_evento(const std::string& bobina_id, const std::string& tipo, double delta_g,
                               const std::string& trabalho_id, bool estimado, const std::string& obs);
    std::string proximo_codigo_bobina() const;

    class Transacao;
    sqlite3* m_db = nullptr;
    Relogio  m_relogio;
};

} // namespace Slic3r::Filuna
