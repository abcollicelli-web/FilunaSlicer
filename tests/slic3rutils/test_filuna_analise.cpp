// Análise da impressão (relatório para o cliente): plano do G-code e anomalias a partir do que a impressora informa.
#include <catch2/catch_test_macros.hpp>

#include "slic3r/Filuna/FilunaAnalise.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <cmath>
#include <functional>
#include <string>

using namespace Slic3r::Filuna;
using nlohmann::json;

namespace {

bool perto(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

// Plano de n camadas de 0,2 mm, cada uma com `dur` segundos, bico a 220 °C, ventoinha a 100% da 3ª em diante.
PlanoImpressao plano_simples(int n, double dur)
{
    PlanoImpressao p;
    p.valido    = true;
    p.temp_mesa = 60;
    double t    = 0.;
    for (int i = 0; i < n; ++i) {
        CamadaPlano c;
        c.numero    = i + 1;
        c.z         = 0.2 * (i + 1);
        c.altura    = 0.2;
        c.inicio_s  = t;
        c.duracao_s = dur;
        c.temp_bico = 220;
        c.ventoinha = i >= 2 ? 100 : 0;
        t += dur;
        p.camadas.push_back(c);
    }
    p.tempo_total_s = t;
    return p;
}

Amostra normal(double t, int camada, int total, int progresso)
{
    Amostra a;
    a.t             = t;
    a.estado        = "imprimindo";
    a.camada        = camada;
    a.total_camadas = total;
    a.progresso     = progresso;
    a.bico = a.bico_alvo = 220.;
    a.mesa = a.mesa_alvo = 60.;
    a.ventoinha  = camada >= 3 ? 100 : 0;
    a.velocidade = 2;
    return a;
}

// Simula a impressão do plano: cada camada leva duracao(i) segundos; amostras a cada 5 s; `mexe` altera a amostra.
json simular(Analise& an, const PlanoImpressao& p, const std::function<double(int)>& duracao,
             const std::function<void(Amostra&)>& mexe = nullptr, const std::string& fim = "concluida")
{
    const int n   = int(p.camadas.size());
    double    t   = 1000.;
    double    tot = 0.;
    for (int i = 0; i < n; ++i)
        tot += duracao(i);
    double feito = 0.;
    for (int i = 0; i < n; ++i) {
        const double d = duracao(i);
        for (double s = 0.; s < d; s += 5.) {
            Amostra a = normal(t + s, i + 1, n, int((feito + s) / tot * 100.));
            if (mexe)
                mexe(a);
            an.amostra(a);
        }
        t += d;
        feito += d;
    }
    Amostra a = normal(t, n, n, 100);
    a.estado  = fim;
    an.amostra(a);
    return an.relatorio(fim);
}

int conta(const json& r, const std::string& tipo)
{
    int n = 0;
    for (const auto& o : r["ocorrencias"])
        n += o["tipo"] == tipo;
    return n;
}

const json* primeira(const json& r, const std::string& tipo)
{
    for (const auto& o : r["ocorrencias"])
        if (o["tipo"] == tipo)
            return &o;
    return nullptr;
}

} // namespace

TEST_CASE("Filuna analise: plano do G-code da Bambu (CHANGE_LAYER)", "[filuna][analise]")
{
    const std::string g = "; HEADER_BLOCK_START\n"
                          "; model printing time: 10m 0s; total estimated time: 12m 0s\n"
                          "; HEADER_BLOCK_END\n"
                          "M140 S60\nM104 S220\nM83\nG1 X10 Y10 F6000\n"
                          "; CHANGE_LAYER\n; Z_HEIGHT: 0.2\n; LAYER_HEIGHT: 0.2\n; FEATURE: Outer wall\n"
                          "G1 Z0.2 F600\nG1 X60 Y10 E2.5 F1200\nG1 X60 Y60 E2.5\n"
                          "; CHANGE_LAYER\n; Z_HEIGHT: 0.4\n; LAYER_HEIGHT: 0.2\n; FEATURE: Sparse infill\nM106 P1 S255\n"
                          "G1 Z0.4\nG1 X10 Y60 E2.5 F3000\nG1 X10 Y10 E2.5\n"
                          "; CHANGE_LAYER\n; Z_HEIGHT: 0.6\n; LAYER_HEIGHT: 0.2\n; FEATURE: Bridge\nM104 S215\n"
                          "G1 Z0.6\nG1 X60 Y10 E2.5 F3000\n"
                          "; filament used [mm] = 12.50\n";
    const PlanoImpressao p = plano_do_texto(g);
    REQUIRE(p.valido);
    REQUIRE(p.camadas.size() == 3);
    CHECK(perto(p.camadas[0].z, 0.2, 1e-9));
    CHECK(perto(p.camadas[2].z, 0.6, 1e-9));
    CHECK(perto(p.camadas[1].altura, 0.2, 1e-9));
    CHECK(p.camadas[0].temp_bico == 220);
    CHECK(p.camadas[2].temp_bico == 215);
    CHECK(p.camadas[1].ventoinha == 100);
    CHECK(p.camadas[0].tipos.front() == "parede externa");
    CHECK(p.camadas[2].tipos.front() == "ponte");
    CHECK(perto(p.camadas[0].extrusao_mm, 5.0, 1e-9));
    CHECK(perto(p.camadas_s(), 600., 1.));    // "model printing time"
    CHECK(perto(p.preparo_s, 120., 1.));      // total - modelo
    CHECK(perto(p.tempo_total_s, 720., 1.));
    CHECK(p.temp_mesa == 60);
    CHECK(perto(p.filamento_mm, 12.5, 1e-9));
    // a camada 1 imprime mais devagar (F1200) que a 2 (F3000) no mesmo percurso
    CHECK(p.camadas[0].duracao_s > p.camadas[1].duracao_s);
    CHECK(p.camada_no_tempo(0.) == 0);
    CHECK(p.camada_no_tempo(p.camadas[2].inicio_s + 1.) == 2);
    CHECK(p.camada_no_progresso(1.0) == 2);
    // o plano vai e volta do arquivo
    const PlanoImpressao q = PlanoImpressao::from_json(p.to_json());
    REQUIRE(q.camadas.size() == 3);
    CHECK(perto(q.camadas[1].duracao_s, p.camadas[1].duracao_s, 1e-9));
    CHECK(q.camadas[2].tipos == p.camadas[2].tipos);
}

TEST_CASE("Filuna analise: plano do G-code comum (LAYER_CHANGE) e extrusao absoluta", "[filuna][analise]")
{
    const std::string g = "M82\nM104 S200\nG92 E0\n"
                          ";LAYER_CHANGE\n;Z:0.3\n;HEIGHT:0.3\n;TYPE:External perimeter\nG1 Z0.3 F600\nG1 X20 Y0 E1.0 F1800\nG1 X20 Y20 E2.0\n"
                          ";LAYER_CHANGE\n;Z:0.5\n;HEIGHT:0.2\nG92 E0\nG1 Z0.5\nG1 X0 Y20 E1.5\n"
                          "; estimated printing time (normal mode) = 5m 0s\n";
    const PlanoImpressao p = plano_do_texto(g);
    REQUIRE(p.camadas.size() == 2);
    CHECK(perto(p.camadas[0].z, 0.3, 1e-9));
    CHECK(perto(p.camadas[0].extrusao_mm, 2.0, 1e-9)); // absoluta: 0 -> 2
    CHECK(perto(p.camadas[1].extrusao_mm, 1.5, 1e-9)); // G92 E0 zerou
    CHECK(perto(p.tempo_total_s, 300., 1e-6));
    CHECK(perto(p.camadas_s() + p.preparo_s, 300., 1.));
    CHECK(p.camadas[0].temp_bico == 200);
    CHECK(plano_do_texto("G1 X1 Y1\n").valido == false); // sem camadas: sem plano
}

TEST_CASE("Filuna analise: impressao que bate com o plano nao tem ocorrencias", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(30, 60.);
    Analise              an(p, "Peça", "P2S");
    const json           r = simular(an, p, [](int) { return 60.; });
    CHECK(r["estado_final"] == "concluida");
    CHECK(r["ocorrencias"].empty());
    CHECK(r["nota"] == 100);
    CHECK(r["camada_max"] == 30);
    CHECK(r["com_plano"] == true);
    CHECK(perto(r["imprimindo_s"].get<double>(), 1800., 10.));
    CHECK_FALSE(r["serie"].empty());
}

TEST_CASE("Filuna analise: camadas lentas viram um trecho com a altura", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(30, 60.);
    Analise              an(p, "Peça", "P2S");
    const json           r = simular(an, p, [](int i) { return i >= 10 && i < 16 ? 180. : 60.; });
    REQUIRE(conta(r, "lento") == 1);
    const json& o = *primeira(r, "lento");
    CHECK(o["camada_de"].get<int>() >= 9);
    CHECK(o["camada_de"].get<int>() <= 12);
    CHECK(o["camada_ate"].get<int>() >= 15);
    CHECK(o["camada_ate"].get<int>() <= 17);
    CHECK(o["z_de"].get<double>() > 1.7);
    CHECK(o["gravidade"] != "info");
    CHECK(o["titulo"].get<std::string>().find("mm de altura") != std::string::npos);
    CHECK(r["nota"].get<int>() < 100);
}

TEST_CASE("Filuna analise: bico frio por mais de 30 s", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(30, 60.);
    Analise              an(p, "Peça", "P2S");
    const json r = simular(an, p, [](int) { return 60.; }, [](Amostra& a) {
        if (a.t >= 1600. && a.t < 1700.)
            a.bico = 202.; // 18 °C abaixo por 100 s
    });
    REQUIRE(conta(r, "bico") == 1);
    const json& o = *primeira(r, "bico");
    CHECK(o["gravidade"] == "problema");
    CHECK(o["titulo"].get<std::string>().find("abaixo") != std::string::npos);
    CHECK(o["camada_de"].get<int>() == 11);
    // variação curta (15 s) não conta
    Analise    an2(p, "Peça", "P2S");
    const json r2 = simular(an2, p, [](int) { return 60.; }, [](Amostra& a) {
        if (a.t >= 1600. && a.t < 1615.)
            a.bico = 200.;
    });
    CHECK(conta(r2, "bico") == 0);
}

TEST_CASE("Filuna analise: pausa anotada e fora da conta do ritmo", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(20, 60.);
    Analise              an(p, "Peça", "A1");
    double               t = 1000.;
    for (int i = 0; i < 20; ++i) {
        for (double s = 0.; s < 60.; s += 5.)
            an.amostra(normal(t + s, i + 1, 20, i * 5));
        t += 60.;
        if (i == 7) { // pausa de 15 min na camada 8
            for (double s = 0.; s < 900.; s += 5.) {
                Amostra a = normal(t + s, 8, 20, 40);
                a.estado  = "pausada";
                an.amostra(a);
            }
            t += 900.;
        }
    }
    Amostra a = normal(t, 20, 20, 100);
    a.estado  = "concluida";
    an.amostra(a);
    const json r = an.relatorio("concluida");
    REQUIRE(conta(r, "pausa") == 1);
    CHECK((*primeira(r, "pausa"))["camada_de"] == 8);
    CHECK((*primeira(r, "pausa"))["gravidade"] == "atencao");
    CHECK(conta(r, "lento") == 0);
    CHECK(conta(r, "parada") == 0);
    CHECK(perto(r["pausado_s"].get<double>(), 900., 10.));
}

TEST_CASE("Filuna analise: erro da impressora e parada sem avancar", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(10, 60.);
    Analise              an(p, "Peça", "P2S");
    double               t = 1000.;
    for (int i = 0; i < 5; ++i) {
        for (double s = 0.; s < 60.; s += 5.)
            an.amostra(normal(t + s, i + 1, 10, i * 10));
        t += 60.;
    }
    // camada 6 parada por 8 min, com um erro no meio
    for (double s = 0.; s < 480.; s += 5.) {
        Amostra a = normal(t + s, 6, 10, 50);
        if (s >= 100. && s < 110.)
            a.erro = "Filamento rompido";
        an.amostra(a);
    }
    t += 480.;
    Amostra a = normal(t, 6, 10, 50);
    a.estado  = "falhou";
    an.amostra(a);
    const json r = an.relatorio("falhou");
    CHECK(conta(r, "erro") == 1);
    CHECK((*primeira(r, "erro"))["detalhe"] == "Filamento rompido");
    CHECK(conta(r, "parada") == 1);
    CHECK((*primeira(r, "parada"))["camada_de"] == 6);
    REQUIRE(conta(r, "fim") == 1);
    CHECK((*primeira(r, "fim"))["gravidade"] == "problema");
    CHECK(r["problemas"].get<int>() >= 3);
    CHECK(r["nota"].get<int>() <= 25);
}

TEST_CASE("Filuna analise: relatorio da Bambu vira amostra", "[filuna][analise]")
{
    const json p = {{"gcode_state", "RUNNING"}, {"layer_num", 12}, {"total_layer_num", 80}, {"mc_percent", 15},
                    {"nozzle_temper", 219.6}, {"nozzle_target_temper", 220}, {"bed_temper", 59.8}, {"bed_target_temper", 60},
                    {"cooling_fan_speed", "15"}, {"spd_lvl", 3}, {"stg_cur", 0}, {"print_error", 0x07002000}};
    const Amostra a = amostra_do_relatorio_bambu(p, 42.);
    CHECK(a.estado == "imprimindo");
    CHECK(a.camada == 12);
    CHECK(a.total_camadas == 80);
    CHECK(a.progresso == 15);
    CHECK(perto(a.bico, 219.6, 1e-9));
    CHECK(a.ventoinha == 100);
    CHECK(a.velocidade == 3);
    CHECK(a.erro == "código 0700-2000");
    CHECK(amostra_do_relatorio_bambu(json{{"gcode_state", "PAUSE"}}, 0.).estado == "pausada");
    CHECK(amostra_do_relatorio_bambu(json{{"gcode_state", "FINISH"}}, 0.).estado == "concluida");
    CHECK(amostra_do_relatorio_bambu(json{{"gcode_state", "IDLE"}}, 0.).estado == "livre");
}

TEST_CASE("Filuna analise: monitor guarda o plano e entrega o relatorio no fim", "[filuna][analise]")
{
    const boost::filesystem::path dir = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("filuna-analise-%%%%%%");
    Monitor& m = Monitor::get();
    m.pasta(dir.string());
    const PlanoImpressao p = plano_simples(10, 30.);
    m.comecar("DEV1", p, "Suporte", "P2S-KYLLE", "trab-1");
    CHECK(boost::filesystem::exists(dir / "plano-DEV1.json"));
    double t = 5000.;
    std::optional<json> r;
    for (int i = 0; i < 10 && !r; ++i)
        for (double s = 0.; s < 30. && !r; s += 5.)
            r = m.amostra("DEV1", "P2S-KYLLE", normal(t + i * 30. + s, i + 1, 10, i * 10)).relatorio;
    CHECK_FALSE(r.has_value());
    Amostra fim = normal(t + 300., 10, 10, 100);
    fim.estado  = "concluida";
    r           = m.amostra("DEV1", "P2S-KYLLE", fim).relatorio;
    REQUIRE(r.has_value());
    CHECK((*r)["trabalho_id"] == "trab-1");
    CHECK((*r)["sessao"].get<std::string>().size() == 16);
    CHECK((*r)["nome"] == "Suporte");
    CHECK((*r)["estado_final"] == "concluida");
    CHECK_FALSE(boost::filesystem::exists(dir / "plano-DEV1.json"));
    CHECK(boost::filesystem::exists(dir / "relatorios"));
    // impressão começada fora do Filuna: análise sem plano, só com o que a impressora informa
    for (int k = 0; k < 40; ++k) {
        Amostra a = normal(9000. + k * 5., -1, -1, k);
        a.camada  = -1;
        m.amostra("DEV2", "A1", a);
    }
    Amostra livre = normal(9300., -1, -1, 0);
    for (int k = 0; k < 3; ++k) {
        livre.estado = "livre";
        livre.t += 5.;
        r = m.amostra("DEV2", "A1", livre).relatorio;
    }
    REQUIRE(r.has_value());
    CHECK((*r)["com_plano"] == false);
    CHECK((*r)["estado_final"] == "cancelada");
    boost::system::error_code ec;
    boost::filesystem::remove_all(dir, ec);
}

TEST_CASE("Filuna analise: previsto acompanha a velocidade de cada trecho", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(20, 60.);
    Analise              an(p, "Peça", "P2S");
    // da camada 11 em diante no Esporte (0,81 do tempo): o previsto é 10 x 60 + 10 x 48,6 s
    const json r = simular(an, p, [](int i) { return i >= 10 ? 48.6 : 60.; }, [](Amostra& a) {
        if (a.camada >= 11)
            a.velocidade = 3;
    });
    CHECK(perto(r["previsto_s"].get<double>(), 1086., 60.));
    CHECK(conta(r, "velocidade") == 1);
    CHECK(conta(r, "tempo") == 0);
    CHECK(conta(r, "lento") == 0);
}

TEST_CASE("Filuna analise: mapa de checkpoints cresce com a impressao e marca os pontos criticos", "[filuna][analise]")
{
    // 1 h de impressão: rotina a cada 5 min
    PlanoImpressao uma = plano_simples(60, 60.);
    std::vector<Checkpoint> m1 = mapa_checkpoints(uma);
    int rotina1 = 0;
    for (const Checkpoint& k : m1)
        rotina1 += !k.critico;
    CHECK(rotina1 >= 9);
    CHECK(rotina1 <= 11);
    // 6 h: rotina a cada 15 min, mais pontos no total
    std::vector<Checkpoint> m6 = mapa_checkpoints(plano_simples(360, 60.));
    CHECK(m6.size() > m1.size());
    // o 1º é o fim da 1ª camada; o último crítico é a última camada
    REQUIRE_FALSE(m1.empty());
    CHECK(m1.front().critico);
    CHECK(m1.front().camada == 1);
    CHECK(m1.front().motivo.find("primeira camada") != std::string::npos);
    CHECK(m1.back().motivo.find("última camada") != std::string::npos);
    // ponte e troca de cor viram pontos críticos, conferidos no fim da camada
    PlanoImpressao p = plano_simples(60, 60.);
    p.camadas[20].tipos = {"ponte"};
    p.camadas[21].tipos = {"ponte"};
    p.camadas[40].troca_cor = true;
    std::vector<Checkpoint> m = mapa_checkpoints(p);
    int pontes = 0, trocas = 0;
    for (const Checkpoint& k : m) {
        if (k.motivo.find("ponte") != std::string::npos) {
            ++pontes;
            CHECK(k.camada == 21);
            CHECK(std::fabs(k.t - 21 * 60.) < 1.);
        }
        trocas += k.motivo.find("troca de cor") != std::string::npos;
        CHECK(k.temp_bico == 220);
        CHECK(k.temp_mesa == 60);
    }
    CHECK(pontes == 1); // a ponte de 2 camadas é um ponto só
    CHECK(trocas == 1);
    for (size_t i = 1; i < m.size(); ++i)
        CHECK(m[i].t >= m[i - 1].t);
    CHECK(mapa_checkpoints(PlanoImpressao()).empty());
}

TEST_CASE("Filuna analise: troca de cor no G-code", "[filuna][analise]")
{
    const std::string g = "T0\n; CHANGE_LAYER\n; Z_HEIGHT: 0.2\nG1 X10 E1 F1200\n"
                          "; CHANGE_LAYER\n; Z_HEIGHT: 0.4\nG1 X0 E1\n"
                          "; CHANGE_LAYER\n; Z_HEIGHT: 0.6\nM620 S1A\nT1\nG1 X10 E1\n"
                          "; CHANGE_LAYER\n; Z_HEIGHT: 0.8\nG1 X0 E1\n";
    const PlanoImpressao p = plano_do_texto(g);
    REQUIRE(p.camadas.size() == 4);
    CHECK_FALSE(p.camadas[0].troca_cor); // a ferramenta inicial não é troca
    CHECK_FALSE(p.camadas[1].troca_cor);
    CHECK(p.camadas[2].troca_cor);
    CHECK_FALSE(p.camadas[3].troca_cor);
    CHECK(PlanoImpressao::from_json(p.to_json()).camadas[2].troca_cor);
}

TEST_CASE("Filuna analise: impressao conforme o plano passa por todos os checkpoints sem alerta", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(60, 60.);
    Analise              an(p, "Peça", "P2S");
    const json           r = simular(an, p, [](int) { return 60.; });
    CHECK(an.alertas().empty());
    CHECK(r["alertas"].empty());
    CHECK(r["checkpoints"].size() >= mapa_checkpoints(p).size() - 1);
    for (const auto& k : r["checkpoints"])
        CHECK(k["ok"] == true);
}

TEST_CASE("Filuna analise: atraso grande no checkpoint avisa com a camada esperada e a real, sem repetir depois", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(60, 60.);
    Analise              an(p, "Suporte", "P2S-KYLLE");
    std::vector<json>    alertas;
    double               t = 1000.;
    for (int i = 0; i < 60; ++i) {
        const double d = i >= 10 && i < 14 ? 240. : 60.; // camadas 11 a 14 levam 4x: ~12 min de atraso
        for (double s = 0.; s < d; s += 5.) {
            an.amostra(normal(t + s, i + 1, 60, i * 100 / 60));
            for (json& a : an.alertas())
                alertas.push_back(a);
        }
        t += d;
    }
    // a lentidão contínua pode avisar de novo (10 min depois); quando o ritmo volta ao normal, o atraso não se repete
    REQUIRE_FALSE(alertas.empty());
    CHECK(alertas.size() <= 2);
    for (const json& x : alertas)
        CHECK(x["camada"].get<int>() <= 16);
    const json& a = alertas[0];
    CHECK(a["tipo"] == "checkpoint");
    CHECK(a["titulo"] == "Sua impressão não está conforme o previsto");
    CHECK(a["mensagem"].get<std::string>().find("de atraso") != std::string::npos);
    CHECK(a["mensagem"].get<std::string>().find("Verifique se há partes imperfeitas") != std::string::npos);
    CHECK(a["conferencia"]["camada_esperada"].get<int>() > a["conferencia"]["camada_real"].get<int>());
    CHECK(a["conferencia"]["ok"] == false);
}

TEST_CASE("Filuna analise: bico frio dispara alerta na hora e o checkpoint confere a temperatura", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(60, 60.);
    Analise              an(p, "Peça", "A1");
    std::vector<json>    alertas;
    double               t = 1000.;
    for (int i = 0; i < 30; ++i) {
        for (double s = 0.; s < 60.; s += 5.) {
            Amostra a = normal(t + s, i + 1, 60, i * 100 / 60);
            if (i >= 12 && i < 20)
                a.bico = 198.; // 22 °C abaixo por 8 min (passa pelo checkpoint dos 15 min)
            an.amostra(a);
            for (json& x : an.alertas())
                alertas.push_back(x);
        }
        t += 60.;
    }
    REQUIRE_FALSE(alertas.empty());
    CHECK(alertas[0]["tipo"] == "bico");
    CHECK(alertas[0]["gravidade"] == "problema");
    CHECK(alertas[0]["mensagem"].get<std::string>().find("198 °C") != std::string::npos);
    CHECK(alertas.size() <= 2); // 10 min entre alertas que não são erro
}

TEST_CASE("Filuna analise: erro da impressora e alerta urgente e ha limite de alertas", "[filuna][analise]")
{
    const PlanoImpressao p = plano_simples(200, 60.);
    Analise              an(p, "Peça", "P2S");
    std::vector<json>    alertas;
    double               t = 1000.;
    for (int i = 0; i < 200; ++i) {
        for (double s = 0.; s < 60.; s += 5.) {
            Amostra a = normal(t + s, i + 1, 200, i / 2);
            if (i % 5 == 0 && s == 0.)
                a.erro = "erro " + std::to_string(i); // um erro novo a cada 5 min
            an.amostra(a);
            for (json& x : an.alertas())
                alertas.push_back(x);
        }
        t += 60.;
    }
    CHECK(alertas.size() == 12);
    CHECK(alertas[0]["tipo"] == "erro");
}
