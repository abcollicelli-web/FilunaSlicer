#pragma once

// Filuna — análise da impressão (relatório para o cliente).
//
// O Filuna Slicer gerou o G-code, então ele monta no computador do cliente o PLANO da impressão (camada a camada:
// altura, tempo previsto, extrusão, temperatura e ventoinha pedidas). Durante a impressão o monitor das impressoras
// entrega AMOSTRAS do que a impressora informa (estado, camada, progresso, temperaturas, ventoinha, velocidade, erros).
// A análise compara as duas coisas e anota cada anomalia com a camada e a altura em milímetros, para o cliente achar
// na peça onde o acabamento pode ter saído ruim. Nada interfere na impressão (não pausa, não para) e o G-code e o
// arquivo nunca saem do computador: só o relatório (texto e números) vai para a conta do cliente no site.
// Sem plano (impressão iniciada fora do Filuna), a análise fica com o que a impressora informa (temperaturas,
// pausas, paradas, erros).
//
// Código puro (sem wx), testável.

#include <nlohmann/json.hpp>

#include <cmath>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace Slic3r { namespace Filuna {

struct CamadaPlano
{
    int                      numero{0};     // 1, 2, 3…
    double                   z{0.};         // altura do topo da camada (mm)
    double                   altura{0.};    // espessura (mm)
    double                   inicio_s{0.};  // tempo previsto desde o início da 1ª camada (s)
    double                   duracao_s{0.}; // tempo previsto da camada (s)
    double                   extrusao_mm{0.};
    int                      temp_bico{0};  // alvo pedido no G-code (°C); 0 = não informado
    int                      ventoinha{-1}; // maior % pedida na camada; -1 = não informado
    std::vector<std::string> tipos;         // trechos: parede externa, preenchimento, ponte…
};

struct PlanoImpressao
{
    bool                     valido{false};
    std::vector<CamadaPlano> camadas;
    double                   preparo_s{0.}; // antes da 1ª camada (aquecer, nivelar)
    double                   tempo_total_s{0.};
    double                   filamento_mm{0.};
    int                      temp_mesa{0};
    double camadas_s() const; // soma do tempo previsto das camadas
    // Camada (índice em camadas) em que o tempo previsto desde a 1ª camada é t; -1 se não há plano.
    int camada_no_tempo(double t) const;
    // Camada em que a fração p (0..1) do tempo das camadas foi atingida (impressoras que só informam o progresso).
    int camada_no_progresso(double p) const;
    nlohmann::json to_json() const;
    static PlanoImpressao from_json(const nlohmann::json& j);
};

// Lê o G-code do Orca (formato Bambu "; CHANGE_LAYER / ; Z_HEIGHT" e o comum ";LAYER_CHANGE / ;Z:").
PlanoImpressao plano_do_gcode(const std::string& caminho);
PlanoImpressao plano_do_texto(const std::string& gcode);

struct Amostra
{
    double      t{0.};          // segundos (relógio do computador)
    std::string estado;         // imprimindo, pausada, concluida, falhou, cancelada, livre, desligada
    int         camada{-1};     // 1..N quando a impressora informa
    int         total_camadas{-1};
    int         progresso{-1};  // 0..100
    double      bico{NAN}, bico_alvo{NAN}, mesa{NAN}, mesa_alvo{NAN};
    int         ventoinha{-1};  // % da ventoinha da peça
    int         velocidade{-1}; // nível de velocidade (Bambu: 1 silencioso, 2 normal, 3 esporte, 4 máximo)
    int         etapa{-1};      // etapa Bambu (stg_cur)
    std::string erro;
};

// Amostra a partir do relatório "print" da Bambu (MQTT), usado pelas impressoras acompanhadas ao vivo.
Amostra amostra_do_relatorio_bambu(const nlohmann::json& p, double t);

struct Ocorrencia
{
    std::string tipo;      // lento, rapido, parada, pausa, bico, mesa, temp_alterada, ventoinha, erro, camadas, velocidade, fim, tempo
    std::string gravidade; // info, atencao, problema
    int         camada_de{-1}, camada_ate{-1}; // 1..N
    double      z_de{NAN}, z_ate{NAN};
    double      t_de{0.}, t_ate{0.};           // segundos desde o início da 1ª camada
    std::string titulo, detalhe, sugestao;
    nlohmann::json to_json() const;
};

class Analise
{
public:
    Analise(PlanoImpressao plano, std::string nome, std::string impressora);
    void amostra(const Amostra& a);
    bool comecou() const { return m_inicio >= 0.; }
    // Relatório (estado_final: concluida, falhou, cancelada, interrompida).
    nlohmann::json                 relatorio(const std::string& estado_final);
    const PlanoImpressao&          plano() const { return m_plano; }
    const std::vector<Ocorrencia>& ocorrencias() const { return m_ocorrencias; }

private:
    struct Janela
    {
        bool   ativo{false};
        double t0{0.}, t1{0.}, pior{0.};
        int    camada0{-1}, camada1{-1};
    };
    struct Ritmo
    {
        double real{0.}, previsto{0.};
        int    de{-1}, ate{-1};
        double t0{0.};
    };
    struct RitmoAberto // trecho lento ou rápido ainda em andamento
    {
        Ocorrencia o;
        double     real{0.}, previsto{0.};
    };
    void   anotar(Ocorrencia o);
    void   anotar_ritmo(const RitmoAberto& r);
    void   fechar_temperaturas(double t);
    void   fechar_janela(Janela& j, const std::string& tipo, double t, double minimo_s);
    void   avaliar_ritmo(bool fim);
    double desde_inicio(double t) const { return m_inicio < 0. ? 0. : t - m_inicio; }
    int    camada_atual(const Amostra& a) const; // índice no plano (0..), -1 se desconhecida
    void   preencher_altura(Ocorrencia& o) const;
    double fator_velocidade(int nivel) const;

    PlanoImpressao          m_plano;
    std::string             m_nome, m_impressora;
    std::vector<Ocorrencia> m_ocorrencias;
    double                  m_inicio{-1.}, m_ultimo_t{-1.}, m_pausado_s{0.}, m_primeira_t{-1.};
    int                     m_camada_inicial{-1}; // 1ª camada vista (o acompanhamento pode começar no meio)
    // ritmo por camada
    int                     m_ultima_camada{-1};
    double                  m_t_camada{0.}, m_pausa_na_camada{0.};
    Ritmo                   m_janela_ritmo;
    std::vector<RitmoAberto> m_ritmo_aberto; // lento/rapido em andamento (um por tipo)
    // parada
    double                  m_parado_desde{-1.};
    int                     m_parado_camada{-2}, m_parado_progresso{-2};
    bool                    m_parada_anotada{false};
    // pausa
    double                  m_pausa_t0{-1.};
    int                     m_pausa_camada{-1};
    // temperaturas e ventoinha
    Janela                  m_bico, m_mesa, m_vent;
    double                  m_alvo_mudou_t{-1.}, m_alvo_visto{NAN};
    std::vector<int>        m_alvos_anotados;
    int                     m_velocidade_ant{-1}, m_velocidade{-1};
    std::string             m_erro_ant;
    int                     m_etapa_ant{-1};
    bool                    m_camadas_anotado{false}, m_mesa_desligada_anotada{false};
    int                     m_maior_camada{-1};
    int                     m_ultimo_progresso{-1};
    // séries para o gráfico
    std::vector<nlohmann::json> m_serie;
    double                      m_ultima_serie{-1e9};
};

// Acompanhamento de todas as impressoras do computador: guarda a análise de cada impressora entre as amostras,
// sobrevive a fechar e abrir o programa (o plano fica num arquivo da pasta) e devolve o relatório quando a impressão
// termina. Seguro entre threads.
class Monitor
{
public:
    static Monitor& get();
    void pasta(const std::string& caminho); // onde guardar planos e relatórios (pasta de dados do Filuna)
    // A impressão desta impressora foi enviada pelo Filuna: começa a análise com o plano do G-code.
    void comecar(const std::string& dev_id, const PlanoImpressao& plano, const std::string& nome, const std::string& impressora,
                 const std::string& trabalho_id);
    // Uma amostra da impressora; devolve o relatório quando a impressão terminou.
    std::optional<nlohmann::json> amostra(const std::string& dev_id, const std::string& impressora, const Amostra& a);

private:
    struct Sessao
    {
        std::unique_ptr<Analise> analise;
        std::string              nome, impressora, trabalho_id;
        bool                     viu_imprimindo{false};
        int                      livres{0};
        double                   desligada_desde{-1.};
        double                   criada{0.};
    };
    std::string arquivo_plano(const std::string& dev_id) const;
    void        guardar_plano(const std::string& dev_id, const Sessao& s, const PlanoImpressao& p) const;
    bool        ler_plano(const std::string& dev_id, Sessao& s, double t);
    void        esquecer_plano(const std::string& dev_id) const;
    nlohmann::json encerrar(const std::string& dev_id, Sessao& s, const std::string& estado);

    std::mutex                    m_trava;
    std::string                   m_pasta;
    std::map<std::string, Sessao> m_sessoes;
};

}} // namespace Slic3r::Filuna
