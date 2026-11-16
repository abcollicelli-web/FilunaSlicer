#pragma once

// Filuna — ajuste assistido da impressão (3 minutos), pedido pelo cliente depois de um alerta.
//
// O cliente recebe o alerta da análise (FilunaAnalise), responde que a impressão está ruim e, com a opção "Deixar a
// Filuna ajustar minha impressora" ligada na conta, aceita que a Filuna ajuste. Este módulo decide, a partir do
// motivo do alerta, do mapa da impressão e das leituras da impressora, o que mudar: temperatura do bico e da mesa,
// nível de velocidade e ventoinha da peça. Decide em 3 rodadas (no início, com 1 min e com 2 min), conferindo o
// efeito de cada mudança; aos 3 minutos para, mantém o que fez e devolve o resumo (com o que desfazer).
//
// Limites que nunca passam: a faixa segura do material (PLA 190–235 °C no bico e até 70 °C na mesa, PETG 220–260 e
// 90, ABS/ASA 230–270 e 110, TPU 200–240 e 70…), no máximo 15 °C acima e 10 °C abaixo da temperatura do arquivo,
// passos de 5 °C, mesa no máximo 5 °C acima do arquivo. Para na hora se a impressão pausar, terminar ou falhar.
// Erro da impressora e parada não se resolvem ajustando: para esses a Filuna não oferece o ajuste.
//
// Código puro (sem wx), testável.

#include "FilunaAnalise.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace Slic3r { namespace Filuna {

// Um comando para a impressora. texto(): "gcode:M104 S230", "gcode:M140 S65", "gcode:M106 P1 S204" ou "velocidade:1".
struct ComandoImpressora
{
    std::string tipo;  // bico, mesa, velocidade, ventoinha
    int         valor{0};
    std::string texto() const;
};

struct FaixaMaterial
{
    int bico_min{190}, bico_max{250}, mesa_max{100};
};
FaixaMaterial faixa_do_material(const std::string& material);

class AjusteAssistido
{
public:
    // O ajuste só vale para alerta de checkpoint, bico, mesa ou ventoinha.
    static bool pode_ajustar(const std::string& tipo_alerta);

    AjusteAssistido(const PlanoImpressao& plano, const std::string& tipo_alerta, const nlohmann::json& conferencia, double duracao_s = 180.);

    // Uma leitura da impressora; devolve os comandos a mandar agora (vazio na maior parte das leituras).
    std::vector<ComandoImpressora> passo(const Amostra& a);
    bool terminou() const { return m_fim; }
    // {"estado": "feito"|"interrompido", "causa", "mudancas": [{"tipo","de","para"}], "texto"}
    nlohmann::json resumo() const;
    // Comandos que voltam ao que a impressora tinha antes do ajuste.
    std::vector<ComandoImpressora> desfazer() const;
    // O cliente pediu para parar: mantém o que já foi feito.
    void parar(double t);

private:
    struct Mudanca
    {
        std::string tipo;
        int         de{0}, para{0};
    };
    void   mudar(std::vector<ComandoImpressora>& cmds, const std::string& tipo, int para);
    int    atual(const std::string& tipo) const;
    // tempo real / previsto (1 = no plano): camadas completas desde o início do ajuste, ou a camada atual quando ela já
    // passou de 1,3x o previsto; NAN quando ainda não dá para medir
    double ritmo(const Amostra& a) const;
    void   rodada(int n, const Amostra& a, std::vector<ComandoImpressora>& cmds);
    void   encerrar(double t, const std::string& estado);

    PlanoImpressao        m_plano;
    std::string           m_causa; // fluxo, bico_frio, bico_quente, mesa, ventoinha, rapido
    FaixaMaterial         m_faixa;
    double                m_duracao{180.}, m_t0{-1.}, m_t_fim{0.};
    int                   m_rodadas{0};
    int                   m_camada_ini{-1}, m_camada_ultima{-1};
    double                m_t_primeira_troca{-1.}, m_t_ultima_troca{-1.};
    int                   m_base_bico{0}, m_base_mesa{0}; // temperaturas do arquivo na camada do início
    int                   m_bico{0}, m_mesa{0}, m_vel{0}, m_vent{-1}; // o que a impressora tem agora (alvos)
    std::vector<Mudanca>  m_mudancas;
    std::vector<double>   m_ritmos; // ritmo medido em cada rodada
    double                m_bico_ini{NAN}, m_bico_fim{NAN}, m_mesa_ini{NAN}, m_mesa_fim{NAN};
    bool                  m_fim{false};
    std::string           m_estado{"feito"};
};

}} // namespace Slic3r::Filuna
