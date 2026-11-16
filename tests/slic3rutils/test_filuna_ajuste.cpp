// Ajuste assistido de 3 minutos depois do alerta: o que muda, os limites do material e o desfazer.
#include <catch2/catch_test_macros.hpp>

#include "slic3r/Filuna/FilunaAjuste.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace Slic3r::Filuna;
using nlohmann::json;

namespace {

PlanoImpressao plano_pla(int n, double dur, int temp = 220)
{
    PlanoImpressao p;
    p.valido    = true;
    p.temp_mesa = 60;
    p.material  = "PLA";
    double t    = 0.;
    for (int i = 0; i < n; ++i) {
        CamadaPlano c;
        c.numero    = i + 1;
        c.z         = 0.2 * (i + 1);
        c.inicio_s  = t;
        c.duracao_s = dur;
        c.temp_bico = temp;
        c.ventoinha = 100;
        t += dur;
        p.camadas.push_back(c);
    }
    return p;
}

Amostra leitura(double t, int camada, double bico_alvo = 220., double bico = 220., int vel = 2, int vent = 100)
{
    Amostra a;
    a.t          = t;
    a.estado     = "imprimindo";
    a.camada     = camada;
    a.bico_alvo  = bico_alvo;
    a.bico       = bico;
    a.mesa_alvo  = 60.;
    a.mesa       = 60.;
    a.velocidade = vel;
    a.ventoinha  = vent;
    return a;
}

// Roda o ajuste por 3 min (leituras a cada 5 s); a camada avança a cada `seg_por_camada` segundos.
std::vector<ComandoImpressora> rodar(AjusteAssistido& aj, double seg_por_camada, Amostra base)
{
    std::vector<ComandoImpressora> todos;
    for (double s = 0.; s <= 190. && !aj.terminou(); s += 5.) {
        Amostra a = base;
        a.t       = 1000. + s;
        a.camada  = base.camada + int(s / seg_por_camada);
        for (const ComandoImpressora& c : aj.passo(a))
            todos.push_back(c);
        // a impressora passa a ter o que foi mandado
        for (const ComandoImpressora& c : todos) {
            if (c.tipo == "bico")
                base.bico_alvo = c.valor;
            if (c.tipo == "velocidade")
                base.velocidade = c.valor;
            if (c.tipo == "ventoinha")
                base.ventoinha = c.valor;
            if (c.tipo == "mesa")
                base.mesa_alvo = c.valor;
        }
    }
    return todos;
}

int conta(const std::vector<ComandoImpressora>& v, const std::string& tipo)
{
    return int(std::count_if(v.begin(), v.end(), [&tipo](const ComandoImpressora& c) { return c.tipo == tipo; }));
}

} // namespace

TEST_CASE("Filuna ajuste: faixas seguras por material e comandos para a impressora", "[filuna][ajuste]")
{
    CHECK(faixa_do_material("PLA").bico_max == 235);
    CHECK(faixa_do_material("PLA Basic").mesa_max == 70);
    CHECK(faixa_do_material("PETG HF").bico_min == 220);
    CHECK(faixa_do_material("ABS").mesa_max == 110);
    CHECK(faixa_do_material("").bico_max == 250);
    CHECK((ComandoImpressora{"bico", 230}.texto() == "gcode:M104 S230"));
    CHECK((ComandoImpressora{"mesa", 65}.texto() == "gcode:M140 S65"));
    CHECK((ComandoImpressora{"ventoinha", 60}.texto() == "gcode:M106 P1 S153"));
    CHECK((ComandoImpressora{"velocidade", 1}.texto() == "velocidade:1"));
    CHECK(AjusteAssistido::pode_ajustar("checkpoint"));
    CHECK(AjusteAssistido::pode_ajustar("bico"));
    CHECK_FALSE(AjusteAssistido::pode_ajustar("erro"));
    CHECK_FALSE(AjusteAssistido::pode_ajustar("parada"));
}

TEST_CASE("Filuna ajuste: atraso sobe o bico e, se continuar lento, baixa a velocidade", "[filuna][ajuste]")
{
    const PlanoImpressao p = plano_pla(100, 30.);
    const json k = {{"camada_esperada", 47}, {"camada_real", 42}, {"bico_esperado", 220}, {"bico_real", 220}, {"mesa_esperada", 60}, {"mesa_real", 60}};
    AjusteAssistido aj(p, "checkpoint", k);
    const auto cmds = rodar(aj, 75., leitura(1000., 42)); // cada camada leva 75 s: 2,5x o previsto
    REQUIRE(aj.terminou());
    REQUIRE_FALSE(cmds.empty());
    CHECK(cmds[0].tipo == "bico");
    CHECK(cmds[0].valor == 225);
    CHECK(conta(cmds, "bico") == 2);
    CHECK(conta(cmds, "velocidade") >= 1);
    const json r = aj.resumo();
    CHECK(r["estado"] == "feito");
    CHECK(r["causa"] == "fluxo");
    CHECK(r["texto"].get<std::string>().find("bico 220 → 230 °C") != std::string::npos);
    const auto volta = aj.desfazer();
    REQUIRE(volta.size() == 2);
    CHECK(volta[0].tipo == "bico");
    CHECK(volta[0].valor == 220);
    CHECK(volta[1].valor == 2);
}

TEST_CASE("Filuna ajuste: nunca passa da faixa do material nem 15 graus acima do arquivo", "[filuna][ajuste]")
{
    const PlanoImpressao p = plano_pla(100, 30., 230);
    const json k = {{"camada_esperada", 47}, {"camada_real", 42}};
    AjusteAssistido aj(p, "checkpoint", k);
    const auto cmds = rodar(aj, 90., leitura(1000., 42, 230., 230.));
    for (const ComandoImpressora& c : cmds)
        if (c.tipo == "bico")
            CHECK(c.valor <= 235); // PLA: até 235 °C
}

TEST_CASE("Filuna ajuste: bico frio reduz a ventoinha e a velocidade, sem forcar a temperatura", "[filuna][ajuste]")
{
    const PlanoImpressao p = plano_pla(100, 30.);
    const json k = {{"bico_esperado", 220}, {"bico_real", 201}};
    AjusteAssistido aj(p, "bico", k);
    const auto cmds = rodar(aj, 30., leitura(1000., 50, 220., 201.));
    CHECK(conta(cmds, "bico") == 0);
    CHECK(conta(cmds, "ventoinha") == 1);
    CHECK(conta(cmds, "velocidade") >= 1);
    CHECK(aj.resumo()["causa"] == "bico_frio");
}

TEST_CASE("Filuna ajuste: mesa fria volta ao arquivo e sobe no maximo 5 graus", "[filuna][ajuste]")
{
    const PlanoImpressao p = plano_pla(100, 30.);
    const json k = {{"mesa_esperada", 60}, {"mesa_real", 48}};
    AjusteAssistido aj(p, "mesa", k);
    Amostra base   = leitura(1000., 30);
    base.mesa_alvo = 55.;
    base.mesa      = 48.;
    const auto cmds = rodar(aj, 30., base);
    REQUIRE(conta(cmds, "mesa") >= 1);
    CHECK(cmds[0].valor == 60);
    for (const ComandoImpressora& c : cmds)
        if (c.tipo == "mesa")
            CHECK(c.valor <= 65);
}

TEST_CASE("Filuna ajuste: para quando a impressao termina e nao manda mais nada", "[filuna][ajuste]")
{
    const PlanoImpressao p = plano_pla(100, 30.);
    AjusteAssistido aj(p, "checkpoint", json{{"camada_esperada", 47}, {"camada_real", 42}});
    CHECK_FALSE(aj.passo(leitura(1000., 42)).empty());
    Amostra fim = leitura(1010., 42);
    fim.estado  = "falhou";
    CHECK(aj.passo(fim).empty());
    CHECK(aj.terminou());
    CHECK(aj.resumo()["estado"] == "interrompido");
    CHECK(aj.passo(leitura(1020., 43)).empty());
}
