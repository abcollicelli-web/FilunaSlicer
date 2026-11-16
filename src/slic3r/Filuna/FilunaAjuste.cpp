#include "FilunaAjuste.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>

using nlohmann::json;

namespace Slic3r { namespace Filuna {

namespace {

std::string maiusculas(std::string s)
{
    for (char& c : s)
        c = char(std::toupper((unsigned char) c));
    return s;
}

std::string nome_velocidade(int n)
{
    switch (n) {
    case 1: return "Silencioso";
    case 2: return "Normal";
    case 3: return "Esporte";
    case 4: return "Máximo";
    default: return "nível " + std::to_string(n);
    }
}

std::string num1(double v)
{
    char b[24];
    std::snprintf(b, sizeof(b), "%.1f", v);
    std::string s(b);
    std::replace(s.begin(), s.end(), '.', ',');
    return s;
}

} // namespace

std::string ComandoImpressora::texto() const
{
    if (tipo == "bico")
        return "gcode:M104 S" + std::to_string(valor);
    if (tipo == "mesa")
        return "gcode:M140 S" + std::to_string(valor);
    if (tipo == "ventoinha")
        return "gcode:M106 P1 S" + std::to_string(int(std::lround(std::clamp(valor, 0, 100) * 2.55)));
    return "velocidade:" + std::to_string(valor);
}

FaixaMaterial faixa_do_material(const std::string& material)
{
    const std::string m = maiusculas(material);
    if (m.find("PETG") != std::string::npos || m.find("PCTG") != std::string::npos)
        return {220, 260, 90};
    if (m.find("ASA") != std::string::npos)
        return {235, 270, 110};
    if (m.find("ABS") != std::string::npos)
        return {230, 270, 110};
    if (m.find("TPU") != std::string::npos || m.find("TPE") != std::string::npos)
        return {200, 240, 70};
    if (m.find("PA") == 0 || m.find("NYLON") != std::string::npos)
        return {240, 290, 100};
    if (m.find("PC") == 0)
        return {250, 290, 115};
    if (m.find("PLA") != std::string::npos)
        return {190, 235, 70};
    return {190, 250, 100};
}

bool AjusteAssistido::pode_ajustar(const std::string& tipo_alerta)
{
    return tipo_alerta == "checkpoint" || tipo_alerta == "bico" || tipo_alerta == "mesa" || tipo_alerta == "ventoinha";
}

AjusteAssistido::AjusteAssistido(const PlanoImpressao& plano, const std::string& tipo_alerta, const json& k, double duracao_s)
    : m_plano(plano), m_faixa(faixa_do_material(plano.material)), m_duracao(duracao_s)
{
    auto num = [&k](const char* c, double padrao) {
        return k.is_object() && k.contains(c) && k[c].is_number() ? k[c].get<double>() : padrao;
    };
    const double bico_esp = num("bico_esperado", 0.), bico_real = num("bico_real", NAN);
    const double mesa_esp = num("mesa_esperada", 0.), mesa_real = num("mesa_real", NAN);
    const int    cam_esp = int(num("camada_esperada", -1.)), cam_real = int(num("camada_real", -1.));
    if (tipo_alerta == "ventoinha")
        m_causa = "ventoinha";
    else if (tipo_alerta == "mesa" || (!std::isnan(mesa_real) && mesa_esp > 0. && mesa_real < mesa_esp - 8.))
        m_causa = "mesa";
    else if (!std::isnan(bico_real) && bico_esp > 0. && std::fabs(bico_real - bico_esp) > 8.)
        m_causa = bico_real < bico_esp ? "bico_frio" : "bico_quente";
    else if (cam_esp > 0 && cam_real > 0 && cam_real > cam_esp)
        m_causa = "rapido";
    else
        m_causa = "fluxo"; // atrasada: o bico não está dando conta (entupindo, filamento travando, frio)
}

int AjusteAssistido::atual(const std::string& tipo) const
{
    return tipo == "bico" ? m_bico : tipo == "mesa" ? m_mesa : tipo == "velocidade" ? m_vel : m_vent;
}

void AjusteAssistido::mudar(std::vector<ComandoImpressora>& cmds, const std::string& tipo, int para)
{
    if (tipo == "bico") {
        const int base = m_base_bico > 0 ? m_base_bico : m_bico;
        para           = std::clamp(para, std::max(m_faixa.bico_min, base - 10), std::min(m_faixa.bico_max, base + 15));
    } else if (tipo == "mesa") {
        const int base = m_base_mesa > 0 ? m_base_mesa : m_mesa;
        para           = std::clamp(para, 0, std::min(m_faixa.mesa_max, base + 5));
    } else if (tipo == "velocidade")
        para = std::clamp(para, 1, 4);
    else
        para = std::clamp(para, 0, 100);
    const int de = atual(tipo);
    if (para == de || (de <= 0 && tipo != "ventoinha" && tipo != "mesa"))
        return; // sem leitura do valor atual não muda (não dá para desfazer depois)
    cmds.push_back({tipo, para});
    auto it = std::find_if(m_mudancas.begin(), m_mudancas.end(), [&tipo](const Mudanca& m) { return m.tipo == tipo; });
    if (it == m_mudancas.end())
        m_mudancas.push_back({tipo, de, para});
    else
        it->para = para;
    (tipo == "bico" ? m_bico : tipo == "mesa" ? m_mesa : tipo == "velocidade" ? m_vel : m_vent) = para;
}

double AjusteAssistido::ritmo(const Amostra& a) const
{
    const int n = int(m_plano.camadas.size());
    if (!m_plano.valido || a.camada < 1 || a.camada > n || m_camada_ini < 1)
        return NAN;
    double r = NAN;
    // camadas que começaram e terminaram dentro do ajuste
    double previsto = 0.;
    for (int k = m_camada_ini + 1; k < m_camada_ultima && k <= n; ++k)
        previsto += m_plano.camadas[k - 1].duracao_s;
    if (previsto > 0. && m_t_ultima_troca > m_t_primeira_troca)
        r = (m_t_ultima_troca - m_t_primeira_troca) / previsto;
    // a camada atual já passou bem do previsto: está lenta, mesmo sem camada completa para medir
    const double na_atual = a.t - (m_t_ultima_troca >= 0. ? m_t_ultima_troca : m_t0);
    const double dur      = std::max(1., m_plano.camadas[a.camada - 1].duracao_s);
    if (na_atual > 1.3 * dur)
        r = std::isnan(r) ? na_atual / dur : std::max(r, na_atual / dur);
    return r;
}

void AjusteAssistido::rodada(int n, const Amostra& a, std::vector<ComandoImpressora>& cmds)
{
    const double r = n == 0 ? NAN : ritmo(a);
    if (n > 0)
        m_ritmos.push_back(r);
    const bool ainda_lento = !std::isnan(r) && r > 1.3; // sem medida, não escala
    if (m_causa == "fluxo") {
        if (n == 0)
            mudar(cmds, "bico", m_bico + 5); // bico um pouco mais quente: o plástico flui melhor
        else if (n == 1 && ainda_lento) {
            mudar(cmds, "bico", m_bico + 5);
            if (m_vel > 1)
                mudar(cmds, "velocidade", m_vel - 1); // menos fluxo pedido ao bico
        } else if (n == 2 && ainda_lento && m_vel > 1)
            mudar(cmds, "velocidade", m_vel - 1);
    } else if (m_causa == "bico_frio" && m_base_bico > 0 && m_bico < m_base_bico - 5) {
        if (n == 0)
            mudar(cmds, "bico", m_base_bico); // a temperatura pedida foi baixada: volta à do arquivo
    } else if (m_causa == "bico_frio") {
        if (n == 0) {
            if (m_vent > 60)
                mudar(cmds, "ventoinha", 60); // o vento da peça também esfria o bico
            if (m_vel > 1)
                mudar(cmds, "velocidade", m_vel - 1); // menos plástico por segundo para aquecer
        } else if (n == 1 && !std::isnan(m_bico_fim) && m_bico > 0 && m_bico_fim < m_bico - 8 && m_vel > 1)
            mudar(cmds, "velocidade", m_vel - 1);
    } else if (m_causa == "bico_quente") {
        if (n == 0)
            mudar(cmds, "bico", m_base_bico > 0 && m_bico > m_base_bico ? m_base_bico : m_bico - 5);
    } else if (m_causa == "mesa") {
        if (n == 0)
            mudar(cmds, "mesa", std::max(m_mesa, m_base_mesa));
        else if (n == 1 && !std::isnan(m_mesa_fim) && m_mesa_fim < m_mesa - 5)
            mudar(cmds, "mesa", m_mesa + 5);
    } else if (m_causa == "ventoinha") {
        const int idx = a.camada >= 1 && a.camada <= int(m_plano.camadas.size()) ? a.camada - 1 : -1;
        const int pedido = idx >= 0 && m_plano.camadas[idx].ventoinha > 0 ? m_plano.camadas[idx].ventoinha : 100;
        if (n == 0 || m_vent <= 0)
            mudar(cmds, "ventoinha", pedido);
    } else if (m_causa == "rapido") {
        if (n == 0 && m_vel > 2)
            mudar(cmds, "velocidade", 2); // rápido demais para o plano: volta ao Normal
    }
}

std::vector<ComandoImpressora> AjusteAssistido::passo(const Amostra& a)
{
    std::vector<ComandoImpressora> cmds;
    if (m_fim)
        return cmds;
    if (a.estado == "concluida" || a.estado == "falhou" || a.estado == "cancelada" || a.estado == "livre" || a.estado == "desligada") {
        encerrar(a.t, "interrompido");
        return cmds;
    }
    if (a.camada >= 1 && a.camada != m_camada_ultima) { // troca de camada: base da medida do ritmo
        if (m_camada_ultima >= 1) {
            if (m_t_primeira_troca < 0.)
                m_t_primeira_troca = a.t;
            m_t_ultima_troca = a.t;
        }
        if (m_camada_ini < 1)
            m_camada_ini = a.camada;
        m_camada_ultima = a.camada;
    }
    if (!std::isnan(a.bico))
        m_bico_fim = a.bico;
    if (!std::isnan(a.mesa))
        m_mesa_fim = a.mesa;
    if (m_t0 < 0.) { // início: guarda o que a impressora tem, para mudar a partir daí e para desfazer
        m_t0 = a.t;
        m_bico = std::isnan(a.bico_alvo) ? 0 : int(std::lround(a.bico_alvo));
        m_mesa = std::isnan(a.mesa_alvo) ? 0 : int(std::lround(a.mesa_alvo));
        m_vel = std::max(0, a.velocidade);
        m_vent = a.ventoinha;
        m_bico_ini = a.bico;
        m_mesa_ini = a.mesa;
        const int idx = a.camada >= 1 && a.camada <= int(m_plano.camadas.size()) ? a.camada - 1 : -1;
        m_base_bico   = idx >= 0 ? m_plano.camadas[idx].temp_bico : m_bico;
        m_base_mesa   = m_plano.temp_mesa > 0 ? m_plano.temp_mesa : m_mesa;
        rodada(m_rodadas++, a, cmds);
        return cmds;
    }
    if (a.estado == "pausada")
        return cmds; // espera voltar (o tempo dos 3 min segue)
    if (a.t - m_t0 >= m_duracao) {
        m_ritmos.push_back(ritmo(a));
        encerrar(a.t, "feito");
        return cmds;
    }
    if (m_rodadas < 3 && a.t - m_t0 >= m_rodadas * (m_duracao / 3.))
        rodada(m_rodadas++, a, cmds);
    return cmds;
}

void AjusteAssistido::parar(double t)
{
    if (!m_fim)
        encerrar(t, "interrompido");
}

void AjusteAssistido::encerrar(double t, const std::string& estado)
{
    m_fim    = true;
    m_estado = estado;
    m_t_fim  = t;
}

std::vector<ComandoImpressora> AjusteAssistido::desfazer() const
{
    std::vector<ComandoImpressora> v;
    for (const Mudanca& m : m_mudancas)
        v.push_back({m.tipo, m.de});
    return v;
}

json AjusteAssistido::resumo() const
{
    static const std::map<std::string, std::string> causas{
        {"fluxo", "a impressão estava atrasada (o bico não estava dando conta do plástico)"},
        {"bico_frio", "o bico estava abaixo da temperatura do arquivo"},
        {"bico_quente", "o bico estava acima da temperatura"},
        {"mesa", "a mesa estava fria"},
        {"ventoinha", "a ventoinha da peça estava parada"},
        {"rapido", "a impressão estava mais rápida que o plano"}};
    json        mud = json::array();
    std::string feito;
    for (const Mudanca& m : m_mudancas) {
        mud.push_back({{"tipo", m.tipo}, {"de", m.de}, {"para", m.para}});
        const std::string trecho = m.tipo == "velocidade" ? "velocidade " + nome_velocidade(m.de) + " → " + nome_velocidade(m.para)
                                   : m.tipo == "ventoinha" ? "ventoinha " + std::to_string(m.de) + "% → " + std::to_string(m.para) + "%"
                                                           : m.tipo + " " + std::to_string(m.de) + " → " + std::to_string(m.para) + " °C";
        feito += (feito.empty() ? "" : ", ") + trecho;
    }
    std::string texto = "A Filuna acompanhou a impressão por " + std::to_string(int(std::lround((m_t_fim - std::max(0., m_t0)) / 60.))) +
                        " min porque " + causas.at(m_causa) + ". ";
    texto += feito.empty() ? "Não foi preciso mudar nada." : "Ajustes: " + feito + ".";
    const double r1 = m_ritmos.empty() ? NAN : m_ritmos.back();
    if (m_causa == "fluxo" && !std::isnan(r1))
        texto += " Ritmo das camadas no fim: " + num1(r1) + "× o previsto" + (r1 <= 1.3 ? " (voltou ao normal)." : " (ainda lento: confira o bico).");
    else if (m_causa == "fluxo")
        texto += " As camadas são longas: o efeito no ritmo aparece nos próximos checkpoints.";
    if ((m_causa == "bico_frio" || m_causa == "bico_quente") && !std::isnan(m_bico_ini) && !std::isnan(m_bico_fim))
        texto += " Bico: " + std::to_string(int(std::lround(m_bico_ini))) + " °C no início, " + std::to_string(int(std::lround(m_bico_fim))) +
                 " °C no fim.";
    if (m_causa == "mesa" && !std::isnan(m_mesa_ini) && !std::isnan(m_mesa_fim))
        texto += " Mesa: " + std::to_string(int(std::lround(m_mesa_ini))) + " °C no início, " + std::to_string(int(std::lround(m_mesa_fim))) +
                 " °C no fim.";
    if (m_estado == "interrompido")
        texto += " O acompanhamento parou antes dos 3 minutos.";
    if (!m_mudancas.empty())
        texto += " Os ajustes ficam na impressão; se preferir, desfaça.";
    return {{"estado", m_estado}, {"causa", m_causa}, {"mudancas", mud}, {"texto", texto}};
}

}} // namespace Slic3r::Filuna
