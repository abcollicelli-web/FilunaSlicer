// Banco local e fórmulas do Filuna (FILUNA/guia-v1/02-DADOS-E-FORMULAS.md). Os números dos testes
// são os exemplos do guia: se uma fórmula mudar, o exemplo do guia deixa de passar.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "slic3r/Filuna/FilunaCalculos.hpp"
#include "slic3r/Filuna/FilunaDb.hpp"
#include "slic3r/Filuna/FilunaGcodeResumo.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace Slic3r::Filuna;
using Catch::Matchers::WithinAbs;

namespace {

// Relógio controlado pelo teste, para eventos com data conhecida.
struct RelogioTeste
{
    std::shared_ptr<std::string> agora = std::make_shared<std::string>("2026-11-01T12:00:00Z");
    FilunaDb::Relogio funcao() const
    {
        auto a = agora;
        return [a] { return *a; };
    }
};

std::unique_ptr<FilunaDb> banco_em_memoria(const RelogioTeste& r = {}) { return FilunaDb::abrir(":memory:", r.funcao()); }

std::string tipo_pla_branco(FilunaDb& db)
{
    FilunaDb::FilamentoTipo t;
    t.marca    = "Marca";
    t.material = "PLA";
    t.cor_nome = "Branco";
    t.cor_hex  = "#FFFFFF";
    return db.criar_filamento_tipo(t);
}

} // namespace

TEST_CASE("Printing time in days, hours, minutes and seconds converts to seconds", "[FilunaDados]")
{
    CHECK(tempo_dhms_em_segundos("1h 45m 3s") == 6303);
    CHECK(tempo_dhms_em_segundos("1d 2h 0m 5s") == 86400 + 7200 + 5);
    CHECK(tempo_dhms_em_segundos("40s") == 40);
    CHECK_FALSE(tempo_dhms_em_segundos("sem tempo").has_value());
}

TEST_CASE("The G-code summary reader takes the fields the slicer writes and leaves missing ones unknown", "[FilunaDados]")
{
    const std::string gcode = "G1 X10 Y10\n"
                              "; filament used [mm] = 26820.10, 6705.03\n"
                              "; filament used [g] = 80.00, 20.00\n"
                              "; total filament used [g] = 100.00\n"
                              "; total filament change = 14\n"
                              "; total layers count = 200\n"
                              "; estimated printing time (normal mode) = 2h 30m 0s\n";
    const GcodeResumo r = ler_resumo_gcode(gcode);
    REQUIRE(r.filamento_g.size() == 2);
    CHECK_THAT(r.filamento_g[0], WithinAbs(80., 1e-9));
    CHECK_THAT(r.filamento_g[1], WithinAbs(20., 1e-9));
    CHECK_THAT(*r.total_g, WithinAbs(100., 1e-9));
    CHECK(r.trocas_filamento == 14);
    CHECK(r.camadas_total == 200);
    CHECK(r.tempo_previsto_s == 9000);
    // O Orca não escreve a linha da torre de purga: purga desconhecida, não zero.
    CHECK_FALSE(r.purga_total_g.has_value());
    CHECK(r.tem_dados());
}

TEST_CASE("The G-code summary reader prefers the end of a large file", "[FilunaDados]")
{
    const boost::filesystem::path caminho = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("filuna-%%%%-%%%%.gcode");
    {
        std::string conteudo = "; total layers count = 10\n";
        conteudo += std::string(200 * 1024, 'G') + "\n";
        conteudo += "; total layers count = 240\n; filament used [g] = 12.34\n";
        FILE* f = std::fopen(caminho.string().c_str(), "wb");
        REQUIRE(f != nullptr);
        std::fwrite(conteudo.data(), 1, conteudo.size(), f);
        std::fclose(f);
    }
    const auto r = ler_resumo_gcode_arquivo(caminho.string());
    boost::filesystem::remove(caminho);
    REQUIRE(r.has_value());
    CHECK(r->camadas_total == 240);
    REQUIRE(r->filamento_g.size() == 1);
    CHECK_THAT(r->filamento_g[0], WithinAbs(12.34, 1e-9));
    CHECK_FALSE(ler_resumo_gcode_arquivo((caminho.parent_path() / "nao-existe.gcode").string()).has_value());
}

TEST_CASE("Spool B-0007 ends with 730 g after consumption, purge, failure and weighing", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    const std::string tipo   = tipo_pla_branco(*db);
    const std::string bobina = db->cadastrar_bobinas(tipo, 1, 1000.).front();
    db->registrar_evento(bobina, "consumo", -212.4);
    db->registrar_evento(bobina, "purga", -18.0);
    db->registrar_evento(bobina, "falha", -35.5);
    db->pesar_bobina(bobina, 730.); // tara 0: a balança mostrou 730 g líquidos → pesagem −4,1
    const auto b = db->bobina(bobina);
    REQUIRE(b.has_value());
    CHECK_THAT(b->restante_g, WithinAbs(730.0, 1e-9));
    CHECK(b->codigo == "B-0001");
    CHECK_FALSE(b->estimado);
}

TEST_CASE("A spool that would go negative is set to zero with an adjustment and becomes empty", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    const std::string bobina = db->cadastrar_bobinas(tipo_pla_branco(*db), 1, 100.).front();
    db->registrar_evento(bobina, "consumo", -130.);
    const auto b = db->bobina(bobina);
    REQUIRE(b.has_value());
    CHECK_THAT(b->restante_g, WithinAbs(0., 1e-9));
    CHECK(b->estado == "vazia");
}

TEST_CASE("Batch registration creates one sealed spool per unit with sequential codes", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    const std::string tipo = tipo_pla_branco(*db);
    const auto        ids  = db->cadastrar_bobinas(tipo, 10, 1000.);
    REQUIRE(ids.size() == 10);
    const auto bobinas = db->bobinas_do_tipo(tipo);
    REQUIRE(bobinas.size() == 10);
    CHECK(bobinas.front().codigo == "B-0001");
    CHECK(bobinas.back().codigo == "B-0010");
    for (const auto& b : bobinas) {
        CHECK(b.estado == "lacrada");
        CHECK_THAT(b.restante_g, WithinAbs(1000., 1e-9));
    }
    CHECK(db->pendentes_sincronizacao() > 0);
}

TEST_CASE("A used spool registered by fraction is marked as estimated until weighed", "[FilunaDados]")
{
    CHECK_THAT(restante_por_fracao(1000., 0.75), WithinAbs(750., 1e-9));
    CHECK_THAT(restante_por_balanca(980., 230.), WithinAbs(750., 1e-9));
    auto db = banco_em_memoria();
    const std::string bobina = db->cadastrar_bobina_usada(tipo_pla_branco(*db), 1000., 230., restante_por_fracao(1000., 0.5), true);
    CHECK(db->bobina(bobina)->estimado);
    CHECK_THAT(db->bobina(bobina)->restante_g, WithinAbs(500., 1e-9));
    db->pesar_bobina(bobina, 640.);
    CHECK_FALSE(db->bobina(bobina)->estimado);
    CHECK_THAT(db->bobina(bobina)->restante_g, WithinAbs(410., 1e-9));
}

TEST_CASE("Spool suggestion follows slot, then most recently used, then oldest sealed", "[FilunaDados]")
{
    const std::vector<BobinaCandidata> bobinas = {
        {"slot", "outro", "em_uso", "impressora:p1:slot:0", "2026-11-01", ""},
        {"uso-antigo", "pla", "em_uso", "", "2026-10-01", ""},
        {"uso-recente", "pla", "em_uso", "", "2026-11-05", ""},
        {"lacrada-nova", "pla", "lacrada", "", "", "2026-10-20"},
        {"lacrada-velha", "pla", "lacrada", "", "", "2026-09-01"},
        {"lacrada-velha-abs", "abs", "lacrada", "", "", "2026-09-01"},
    };
    CHECK(sugerir_bobina(bobinas, "pla", "impressora:p1:slot:0") == std::optional<std::string>("slot"));
    CHECK(sugerir_bobina(bobinas, "pla", "impressora:p1:slot:1") == std::optional<std::string>("uso-recente"));
    CHECK(sugerir_bobina(bobinas, "abs", "") == std::optional<std::string>("lacrada-velha-abs"));
    const std::vector<BobinaCandidata> so_lacradas(bobinas.begin() + 3, bobinas.begin() + 5);
    CHECK(sugerir_bobina(so_lacradas, "pla", "") == std::optional<std::string>("lacrada-velha"));
    CHECK_FALSE(sugerir_bobina(bobinas, "petg", "").has_value());
}

TEST_CASE("A finished two-colour job splits the purge in proportion and books consumption and purge", "[FilunaDados]")
{
    const RateioPurga rateio = ratear_purga({80., 20.}, 6.);
    REQUIRE(rateio.purga_g.size() == 2);
    CHECK_THAT(rateio.purga_g[0], WithinAbs(4.8, 1e-9));
    CHECK_THAT(rateio.purga_g[1], WithinAbs(1.2, 1e-9));
    CHECK(rateio.estimado);
    CHECK_FALSE(ratear_purga({50.}, 3.).estimado);

    const auto f = fracao_impressa(FimTrabalho::Concluido, std::nullopt, std::nullopt, std::nullopt);
    REQUIRE(f.has_value());
    const auto r = eventos_fim_trabalho(FimTrabalho::Concluido, *f, {80., 20.}, rateio.purga_g, true, 1, 9000, std::nullopt);
    REQUIRE(r.eventos.size() == 4);
    CHECK(r.eventos[0].tipo == "consumo");
    CHECK_THAT(r.eventos[0].delta_g, WithinAbs(-75.2, 1e-9));
    CHECK(r.eventos[1].tipo == "purga");
    CHECK_THAT(r.eventos[1].delta_g, WithinAbs(-4.8, 1e-9));
    CHECK_THAT(r.eventos[2].delta_g, WithinAbs(-18.8, 1e-9));
    CHECK_THAT(r.eventos[3].delta_g, WithinAbs(-1.2, 1e-9));
    CHECK(r.tempo_real_s == 9000);
}

TEST_CASE("A job cancelled at layer 50 of 200 books a quarter as estimated failure", "[FilunaDados]")
{
    const auto f = fracao_impressa(FimTrabalho::Cancelado, 50, 200, std::nullopt);
    REQUIRE(f.has_value());
    CHECK_THAT(f->f, WithinAbs(0.25, 1e-9));
    CHECK(f->estimado);
    const auto r = eventos_fim_trabalho(FimTrabalho::Cancelado, *f, {80., 20.}, {4.8, 1.2}, true, 1, 9000, std::nullopt);
    REQUIRE(r.eventos.size() == 2);
    CHECK(r.eventos[0].tipo == "falha");
    CHECK_THAT(r.eventos[0].delta_g, WithinAbs(-20.0, 1e-9));
    CHECK_THAT(r.eventos[1].delta_g, WithinAbs(-5.0, 1e-9));
    CHECK(r.eventos[0].estimado);
    CHECK(r.tempo_real_s == 2250);
    // Sem camada nem progresso, o sistema pergunta ao cliente.
    CHECK_FALSE(fracao_impressa(FimTrabalho::Falhou, std::nullopt, std::nullopt, std::nullopt).has_value());
    CHECK_THAT(fracao_impressa(FimTrabalho::Falhou, std::nullopt, std::nullopt, std::nullopt, 0.5)->f, WithinAbs(0.5, 1e-9));
}

TEST_CASE("Nozzle cleaning due every 100 h, done at 340 h, is next at 432 h with 8 h left", "[FilunaDados]")
{
    const EstadoManutencao e = estado_manutencao(432., 100., 340.);
    CHECK_THAT(e.usado_h, WithinAbs(92., 1e-9));
    CHECK_THAT(e.faltam_h, WithinAbs(8., 1e-9));
    CHECK(e.situacao == SituacaoManutencao::Proxima);
    CHECK(estado_manutencao(400., 100., 340.).situacao == SituacaoManutencao::EmDia);
    CHECK(estado_manutencao(440., 100., 340.).situacao == SituacaoManutencao::Vencida);
}

TEST_CASE("Cost per gram and waste rate follow the guide example", "[FilunaDados]")
{
    CHECK_THAT(custo_por_grama_centavos(12000, 1000.), WithinAbs(12., 1e-9));
    CHECK_THAT(80. * custo_por_grama_centavos(12000, 1000.), WithinAbs(960., 1e-9));
    CHECK_THAT(*taxa_desperdicio(150. + 90., 2000.), WithinAbs(0.12, 1e-9));
    CHECK_FALSE(taxa_desperdicio(0., 0.).has_value());
}

TEST_CASE("White PLA with 450 g left and 100 g a day raises the shortage alert", "[FilunaDados]")
{
    const Previsao p = prever_estoque(2800., 450., 5, 28.);
    CHECK_THAT(p.media_diaria_g, WithinAbs(100., 1e-9));
    CHECK_THAT(p.dias_de_estoque, WithinAbs(4.5, 1e-9));
    CHECK_THAT(p.necessidade_7d_g, WithinAbs(700., 1e-9));
    CHECK_THAT(p.falta_g, WithinAbs(250., 1e-9));
    CHECK(p.alerta);
    CHECK_FALSE(prever_estoque(2800., 450., 2, 28.).dados_suficientes);
    CHECK_FALSE(prever_estoque(2800., 450., 5, 6.).alerta);
    CHECK(prever_estoque(0., 450., 5, 28.).sem_consumo);
}

TEST_CASE("The product signature ignores object order and copies but not scale", "[FilunaDados]")
{
    const VolumeMalha cubo{{0, 0, 0, 1, 0, 0, 0, 1, 0}, {0, 1, 2}};
    const VolumeMalha outro{{0, 0, 0, 2, 0, 0, 0, 2, 0}, {0, 1, 2}};
    const std::string a = assinatura_objeto({cubo}, 1., 1., 1.);
    const std::string b = assinatura_objeto({outro}, 1., 1., 1.);
    CHECK(a.size() == 64);
    CHECK(assinatura_produto({a, b}) == assinatura_produto({b, a}));
    CHECK(assinatura_produto({a, a, b}) == assinatura_produto({a, b}));
    CHECK(assinatura_objeto({cubo}, 2., 1., 1.) != a);
    CHECK(sha256_hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_CASE("Printing the same model three times gives one product with three finished jobs", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    const std::string impressora = db->criar_impressora("luca", "Snapmaker", "U1");
    const std::string bobina     = db->cadastrar_bobinas(tipo_pla_branco(*db), 1, 1000.).front();
    GcodeResumo resumo;
    resumo.filamento_g      = {25.};
    resumo.tempo_previsto_s = 3600;
    resumo.camadas_total    = 100;
    for (int i = 0; i < 3; ++i) {
        const std::string produto  = db->produto_por_assinatura("assinatura-da-peca", "Peça");
        const std::string trabalho = db->criar_trabalho(produto, impressora, resumo);
        db->confirmar_bobina(trabalho, 0, bobina);
        db->finalizar_trabalho(trabalho, FimTrabalho::Concluido, FracaoImpressa{});
    }
    const std::string produto = db->produto_por_assinatura("assinatura-da-peca", "Peça");
    CHECK(db->trabalhos_concluidos_do_produto(produto) == 3);
    CHECK_THAT(db->bobina(bobina)->restante_g, WithinAbs(925., 1e-9));
    CHECK(db->bobina(bobina)->estado == "em_uso");
    CHECK_THAT(db->horas_maquina(impressora), WithinAbs(3., 1e-9));
}

TEST_CASE("A job cancelled halfway books half the weight as waste on the dashboard", "[FilunaDados]")
{
    RelogioTeste relogio;
    auto db = banco_em_memoria(relogio);
    const std::string bobina = db->cadastrar_bobinas(tipo_pla_branco(*db), 1, 1000.).front();
    GcodeResumo resumo;
    resumo.filamento_g      = {40.};
    resumo.tempo_previsto_s = 7200;
    resumo.camadas_total    = 100;
    const std::string trabalho = db->criar_trabalho({}, {}, resumo);
    db->confirmar_bobina(trabalho, 0, bobina);
    *relogio.agora = "2026-11-02T08:00:00Z";
    const auto res = db->finalizar_trabalho(trabalho, FimTrabalho::Cancelado, *fracao_impressa(FimTrabalho::Cancelado, 50, 100, std::nullopt),
                                            std::nullopt, 50);
    CHECK(res.tempo_real_s == 3600);
    CHECK_THAT(db->bobina(bobina)->restante_g, WithinAbs(980., 1e-9));
    const auto d = db->desperdicio("2026-11-01T00:00:00Z", "2026-11-08T00:00:00Z");
    CHECK_THAT(d.desperdicio_g, WithinAbs(20., 1e-9));
    CHECK_THAT(*d.taxa, WithinAbs(1., 1e-9));
}

TEST_CASE("The shortage forecast needs three jobs and a week of history", "[FilunaDados]")
{
    RelogioTeste relogio;
    auto db = banco_em_memoria(relogio);
    const std::string tipo   = tipo_pla_branco(*db);
    const std::string bobina = db->cadastrar_bobinas(tipo, 1, 3250.).front();
    // 4 trabalhos de 700 g ao longo de 28 dias: 2.800 g → 100 g/dia, sobram 450 g.
    const char* datas[] = {"2026-10-05T10:00:00Z", "2026-10-12T10:00:00Z", "2026-10-19T10:00:00Z", "2026-10-26T10:00:00Z"};
    for (const char* d : datas) {
        *relogio.agora = d;
        db->registrar_evento(bobina, "consumo", -700., FilunaDb::novo_uuid());
    }
    *relogio.agora = "2026-11-01T12:00:00Z";
    const Previsao p = db->previsao_tipo(tipo);
    CHECK(p.dados_suficientes);
    CHECK_THAT(p.media_diaria_g, WithinAbs(100., 1e-9));
    CHECK_THAT(p.falta_g, WithinAbs(250., 1e-9));
    CHECK(p.alerta);

    auto vazio = banco_em_memoria(relogio);
    CHECK_FALSE(vazio->previsao_tipo(tipo_pla_branco(*vazio)).dados_suficientes);
}

TEST_CASE("Maintenance done resets the counter at the current machine hours", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    const std::string impressora = db->criar_impressora("luca", "Snapmaker", "U1");
    const std::string regra      = db->adicionar_regra_manutencao(impressora, "Limpar bico", 100.);
    GcodeResumo resumo;
    resumo.filamento_g      = {10.};
    resumo.tempo_previsto_s = 95 * 3600;
    db->finalizar_trabalho(db->criar_trabalho({}, impressora, resumo), FimTrabalho::Concluido, FracaoImpressa{});
    REQUIRE(db->manutencao(impressora).size() == 1);
    CHECK(db->manutencao(impressora)[0].estado.situacao == SituacaoManutencao::Proxima);
    db->registrar_manutencao_feita(regra);
    CHECK_THAT(db->manutencao(impressora)[0].feito_ate_h, WithinAbs(95., 1e-9));
    CHECK(db->manutencao(impressora)[0].estado.situacao == SituacaoManutencao::EmDia);
}

TEST_CASE("Reopening the database keeps the data and does not migrate twice", "[FilunaDados]")
{
    const boost::filesystem::path caminho = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("filuna-%%%%-%%%%.db");
    std::string tipo;
    {
        auto db = FilunaDb::abrir(caminho.string());
        CHECK(db->versao_esquema() == FilunaDb::versao_esquema_atual());
        tipo = tipo_pla_branco(*db);
        db->cadastrar_bobinas(tipo, 2, 1000.);
    }
    {
        auto db = FilunaDb::abrir(caminho.string());
        CHECK(db->versao_esquema() == FilunaDb::versao_esquema_atual());
        CHECK(db->bobinas_do_tipo(tipo).size() == 2);
    }
    boost::system::error_code ec;
    for (const char* sufixo : {"", "-wal", "-shm"})
        boost::filesystem::remove(caminho.string() + sufixo, ec);
}

TEST_CASE("Registering the same brand, material and colour twice reuses the filament type", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    FilunaDb::FilamentoTipo t;
    t.marca    = "eSun";
    t.material = "PLA";
    t.cor_nome = "Branco";
    t.cor_hex  = "#FFFFFF";
    const std::string a = db->tipo_igual_ou_novo(t);
    t.marca = "ESUN";
    CHECK(db->tipo_igual_ou_novo(t) == a);
    t.cor_nome = "Preto";
    CHECK(db->tipo_igual_ou_novo(t) != a);
    CHECK(db->tipos().size() == 2);
}

TEST_CASE("Archiving a spool books what was left as discarded and hides it from the active list", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    const auto ids = db->cadastrar_bobinas(tipo_pla_branco(*db), 2, 1000.);
    db->registrar_evento(ids[0], "consumo", -400.);
    db->descartar_bobina(ids[0]);
    const auto b = db->bobina(ids[0]);
    REQUIRE(b.has_value());
    CHECK(b->estado == "descartada");
    CHECK_THAT(b->restante_g, WithinAbs(0., 1e-9));
    CHECK(db->eventos_da_bobina(ids[0]).front().tipo == "descarte");
    CHECK(db->bobinas().size() == 1);
    CHECK(db->bobinas(true).size() == 2);
}

TEST_CASE("Purge known per filament is booked as exact, not estimated", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    const auto  ids = db->cadastrar_bobinas(tipo_pla_branco(*db), 2, 1000.);
    GcodeResumo resumo;
    resumo.filamento_g = {80., 20.};
    resumo.purga_g     = {5., 1.};
    const std::string trabalho = db->criar_trabalho({}, {}, resumo);
    db->confirmar_bobina(trabalho, 0, ids[0]);
    db->confirmar_bobina(trabalho, 1, ids[1]);
    const auto r = db->finalizar_trabalho(trabalho, FimTrabalho::Concluido, FracaoImpressa{});
    REQUIRE(r.eventos.size() == 4);
    CHECK_FALSE(r.eventos[1].estimado);
    CHECK_THAT(r.eventos[0].delta_g, WithinAbs(-75., 1e-9));
    CHECK_THAT(db->bobina(ids[1])->restante_g, WithinAbs(980., 1e-9));
    const auto lista = db->trabalhos();
    REQUIRE(lista.size() == 1);
    CHECK(lista[0].estado == "concluido");
    CHECK_THAT(lista[0].real_g, WithinAbs(100., 1e-9));
    REQUIRE(lista[0].bobinas.size() == 2);
    CHECK(lista[0].bobinas[1] == ids[1]);
}

TEST_CASE("The product ranking puts the most printed product first", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    GcodeResumo resumo;
    resumo.filamento_g = {10.};
    const std::string pouco = db->produto_por_assinatura("a", "Pouco");
    const std::string muito = db->produto_por_assinatura("b", "Muito");
    db->finalizar_trabalho(db->criar_trabalho(pouco, {}, resumo), FimTrabalho::Concluido, FracaoImpressa{});
    for (int i = 0; i < 2; ++i)
        db->finalizar_trabalho(db->criar_trabalho(muito, {}, resumo), FimTrabalho::Concluido, FracaoImpressa{});
    const auto r = db->produtos();
    REQUIRE(r.size() == 2);
    CHECK(r[0].nome == "Muito");
    CHECK(r[0].concluidos == 2);
    CHECK_THAT(r[0].gramas, WithinAbs(20., 1e-9));
    CHECK(db->impressora_por_nome("luca") == db->impressora_por_nome("luca"));
}

// --- Sincronização (03-SERVIDOR §4) ---------------------------------------------------------

TEST_CASE("Fila de sincronização: envia, marca e não volta", "[FilunaDados]")
{
    auto db = banco_em_memoria();
    const auto tipo = tipo_pla_branco(*db);
    db->cadastrar_bobinas(tipo, 1, 1000.);
    const auto fila = db->fila_para_enviar();
    REQUIRE(fila.size() >= 3); // tipo, bobina, evento de entrada
    CHECK(fila.front().tabela == "filamento_tipo");
    CHECK(fila.front().payload_json.find("\"id\":\"" + tipo + "\"") != std::string::npos);
    std::vector<std::int64_t> seqs;
    for (const auto& i : fila)
        seqs.push_back(i.seq);
    db->marcar_enviados(seqs);
    CHECK(db->pendentes_sincronizacao() == 0);
    CHECK(db->fila_para_enviar().empty());
    const std::string pc = db->id_do_computador();
    CHECK(pc.size() == 36);
    CHECK(db->id_do_computador() == pc);
}

TEST_CASE("Restauração num computador novo e eventos de outro computador", "[FilunaDados]")
{
    // Computador A grava; o que ele enviou é aplicado num banco vazio (B), como no receber?desde=0.
    auto a = banco_em_memoria();
    const auto tipo = tipo_pla_branco(*a);
    const auto bob  = a->cadastrar_bobinas(tipo, 1, 1000.).front();
    a->registrar_evento(bob, "consumo", -120.);
    std::vector<FilunaDb::ItemRemoto> itens;
    for (const auto& i : a->fila_para_enviar(500))
        itens.push_back({i.tabela, i.id, i.payload_json});
    auto b = banco_em_memoria();
    CHECK(b->aplicar_remotos(itens) > 0);
    REQUIRE(b->bobina(bob));
    CHECK_THAT(b->bobina(bob)->restante_g, WithinAbs(880., 0.01));
    CHECK(b->pendentes_sincronizacao() == 0); // o que veio de fora não volta para a fila
    // Aplicar de novo não duplica eventos.
    b->aplicar_remotos(itens);
    CHECK(b->eventos_da_bobina(bob).size() == 2);
    // Ajuste feito no app chega como evento: restante = soma dos eventos.
    b->aplicar_remotos({{"estoque_evento", "srv-1",
                         R"({"id":"srv-1","criado_em":"2026-11-02T10:00:00Z","atualizado_em":"2026-11-02T10:00:00Z","excluido":0,"rev":0,)"
                         R"("bobina_id":")" + bob + R"(","tipo":"ajuste","delta_g":-80,"trabalho_id":null,"estimado":0,"obs":"app"})"}});
    CHECK_THAT(b->bobina(bob)->restante_g, WithinAbs(800., 0.01));
    // Versão mais velha do tipo não sobrescreve a mais nova; tabela desconhecida e JSON com outro id são ignorados.
    CHECK(b->aplicar_remotos({{"filamento_tipo", tipo, R"({"id":")" + tipo + R"(","atualizado_em":"2000-01-01T00:00:00Z","cor_nome":"Velho"})"},
                              {"sync_fila", "1", R"({"id":"1"})"},
                              {"bobina", "x", R"({"id":"y"})"}}) == 0);
    CHECK(b->tipo(tipo)->cor_nome != "Velho");
}

TEST_CASE("Código de bobina repetido vindo de outro computador ganha sufixo", "[FilunaDados]")
{
    auto a = banco_em_memoria(), b = banco_em_memoria();
    const auto ta = tipo_pla_branco(*a);
    const auto ba = a->cadastrar_bobinas(ta, 1, 1000.).front(); // B-0001 em A
    const auto tb = tipo_pla_branco(*b);
    b->cadastrar_bobinas(tb, 1, 1000.);                        // B-0001 em B
    std::vector<FilunaDb::ItemRemoto> itens;
    for (const auto& i : a->fila_para_enviar(500))
        itens.push_back({i.tabela, i.id, i.payload_json});
    b->aplicar_remotos(itens);
    REQUIRE(b->bobina(ba));
    CHECK(b->bobina(ba)->codigo == "B-0001-" + ba.substr(0, 4));
    CHECK(b->bobinas().size() == 2);
}
