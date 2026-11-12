#include "FilunaCalculos.hpp"

#include <openssl/evp.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <numeric>
#include <stdexcept>

namespace Slic3r::Filuna {

RestanteBobina restante_bobina(const std::vector<double>& deltas_g)
{
    RestanteBobina r;
    const double soma = std::accumulate(deltas_g.begin(), deltas_g.end(), 0.);
    if (soma < 0.) {
        r.restante_g  = 0.;
        r.ajuste_g    = -soma;
        r.divergencia = true;
    } else {
        r.restante_g = soma;
    }
    return r;
}

double delta_pesagem(double peso_balanca_g, double tara_g, double restante_atual_g)
{
    return restante_por_balanca(peso_balanca_g, tara_g) - restante_atual_g;
}

double restante_por_balanca(double peso_balanca_g, double tara_g) { return std::max(0., peso_balanca_g - tara_g); }

double restante_por_fracao(double capacidade_g, double fracao) { return capacidade_g * std::clamp(fracao, 0., 1.); }

RateioPurga ratear_purga(const std::vector<double>& previsto_g, double purga_total_g)
{
    RateioPurga r;
    r.purga_g.assign(previsto_g.size(), 0.);
    if (previsto_g.empty())
        return r;
    if (previsto_g.size() == 1) {
        r.purga_g[0] = purga_total_g;
        return r;
    }
    const double soma = std::accumulate(previsto_g.begin(), previsto_g.end(), 0.);
    r.estimado = true;
    if (soma <= 0.)
        return r;
    for (std::size_t i = 0; i < previsto_g.size(); ++i)
        r.purga_g[i] = purga_total_g * previsto_g[i] / soma;
    return r;
}

std::optional<FracaoImpressa> fracao_impressa(FimTrabalho           fim,
                                              std::optional<int>    camada_parada,
                                              std::optional<int>    camadas_total,
                                              std::optional<double> progresso_percentual,
                                              std::optional<double> resposta_cliente)
{
    if (fim == FimTrabalho::Concluido)
        return FracaoImpressa{1., false};
    FracaoImpressa r;
    r.estimado = true;
    if (camada_parada && camadas_total && *camadas_total > 0)
        r.f = static_cast<double>(*camada_parada) / static_cast<double>(*camadas_total);
    else if (progresso_percentual)
        r.f = *progresso_percentual / 100.;
    else if (resposta_cliente)
        r.f = *resposta_cliente;
    else
        return std::nullopt;
    r.f = std::clamp(r.f, 0., 1.);
    return r;
}

ResultadoFimTrabalho eventos_fim_trabalho(FimTrabalho                 fim,
                                          const FracaoImpressa&       fracao,
                                          const std::vector<double>&  previsto_g,
                                          const std::vector<double>&  purga_g,
                                          bool                        purga_estimada,
                                          int                         copias_ok,
                                          std::optional<std::int64_t> tempo_previsto_s,
                                          std::optional<std::int64_t> tempo_informado_s)
{
    ResultadoFimTrabalho r;
    const double mult = fracao.f * std::max(copias_ok, 0);
    for (std::size_t i = 0; i < previsto_g.size(); ++i) {
        const double real = previsto_g[i] * mult;
        r.real_g.push_back(real);
        const int idx = static_cast<int>(i);
        if (fim == FimTrabalho::Concluido) {
            const double purga = i < purga_g.size() ? purga_g[i] * mult : 0.;
            r.eventos.push_back({idx, "consumo", -(real - purga), fracao.estimado || (purga > 0. && purga_estimada)});
            if (purga > 0.)
                r.eventos.push_back({idx, "purga", -purga, fracao.estimado || purga_estimada});
        } else {
            // Cancelado ou falhou: tudo o que saiu da bobina vira desperdício.
            r.eventos.push_back({idx, "falha", -real, fracao.estimado});
        }
    }
    if (tempo_informado_s)
        r.tempo_real_s = *tempo_informado_s;
    else if (tempo_previsto_s)
        r.tempo_real_s = static_cast<std::int64_t>(std::llround(static_cast<double>(*tempo_previsto_s) * fracao.f));
    return r;
}

EstadoManutencao estado_manutencao(double horas_maquina, double intervalo_h, double feito_ate_h)
{
    EstadoManutencao e;
    e.usado_h  = horas_maquina - feito_ate_h;
    e.faltam_h = intervalo_h - e.usado_h;
    if (e.usado_h >= intervalo_h)
        e.situacao = SituacaoManutencao::Vencida;
    else if (e.usado_h >= 0.9 * intervalo_h)
        e.situacao = SituacaoManutencao::Proxima;
    else
        e.situacao = SituacaoManutencao::EmDia;
    return e;
}

const char* situacao_manutencao_texto(SituacaoManutencao s)
{
    switch (s) {
    case SituacaoManutencao::Proxima: return "próxima";
    case SituacaoManutencao::Vencida: return "vencida";
    default: return "em dia";
    }
}

double custo_por_grama_centavos(std::int64_t preco_centavos, double capacidade_g)
{
    return capacidade_g > 0. ? static_cast<double>(preco_centavos) / capacidade_g : 0.;
}

std::optional<double> taxa_desperdicio(double desperdicio_g, double consumo_total_g)
{
    if (consumo_total_g <= 0.)
        return std::nullopt;
    return desperdicio_g / consumo_total_g;
}

Previsao prever_estoque(double gasto_janela_g,
                        double estoque_tipo_g,
                        int    trabalhos_no_historico,
                        double dias_de_historico,
                        double limite_dias,
                        double janela_dias)
{
    Previsao p;
    p.dados_suficientes = trabalhos_no_historico >= 3 && dias_de_historico >= 7.;
    p.media_diaria_g    = janela_dias > 0. ? gasto_janela_g / janela_dias : 0.;
    p.necessidade_7d_g  = p.media_diaria_g * 7.;
    p.falta_g           = std::max(0., p.necessidade_7d_g - estoque_tipo_g);
    if (p.media_diaria_g <= 0.) {
        p.sem_consumo = true;
        return p;
    }
    p.dias_de_estoque = estoque_tipo_g / p.media_diaria_g;
    p.alerta          = p.dados_suficientes && p.dias_de_estoque < limite_dias;
    return p;
}

std::optional<std::string> sugerir_bobina(const std::vector<BobinaCandidata>& bobinas,
                                          const std::string&                  filamento_tipo_id,
                                          const std::string&                  local_do_slot)
{
    // 1. A bobina que já está neste slot desta impressora.
    if (!local_do_slot.empty())
        for (const auto& b : bobinas)
            if (b.local == local_do_slot && (b.estado == "em_uso" || b.estado == "lacrada"))
                return b.id;

    const BobinaCandidata* escolhida = nullptr;
    // 2. Em uso do mesmo tipo, a usada mais recentemente.
    for (const auto& b : bobinas)
        if (b.filamento_tipo_id == filamento_tipo_id && b.estado == "em_uso")
            if (!escolhida || b.ultimo_uso_em > escolhida->ultimo_uso_em)
                escolhida = &b;
    if (escolhida)
        return escolhida->id;

    // 3. Lacrada do mesmo tipo, a comprada há mais tempo (data vazia fica por último).
    for (const auto& b : bobinas)
        if (b.filamento_tipo_id == filamento_tipo_id && b.estado == "lacrada") {
            if (!escolhida)
                escolhida = &b;
            else if (!b.comprada_em.empty() && (escolhida->comprada_em.empty() || b.comprada_em < escolhida->comprada_em))
                escolhida = &b;
        }
    if (escolhida)
        return escolhida->id;
    return std::nullopt;
}

std::string sha256_hex(const std::string& dados)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  n   = 0;
    EVP_MD_CTX*   ctx = EVP_MD_CTX_new();
    if (ctx == nullptr)
        throw std::runtime_error("SHA-256 indisponível");
    const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 && EVP_DigestUpdate(ctx, dados.data(), dados.size()) == 1 &&
                    EVP_DigestFinal_ex(ctx, digest, &n) == 1;
    EVP_MD_CTX_free(ctx);
    if (!ok)
        throw std::runtime_error("SHA-256 falhou");
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(n * 2);
    for (unsigned int i = 0; i < n; ++i) {
        out.push_back(hex[digest[i] >> 4]);
        out.push_back(hex[digest[i] & 0xF]);
    }
    return out;
}

namespace {

void anexar_le32(std::string& out, std::uint32_t v)
{
    for (int i = 0; i < 4; ++i)
        out.push_back(static_cast<char>((v >> (8 * i)) & 0xFF));
}

std::string tres_casas(double v)
{
    char buf[64];
    const auto res = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::fixed, 3);
    return std::string(buf, res.ptr);
}

} // namespace

std::string assinatura_objeto(const std::vector<VolumeMalha>& volumes, double escala_x, double escala_y, double escala_z)
{
    std::string hashes;
    for (const VolumeMalha& v : volumes) {
        std::string bytes;
        bytes.reserve(v.vertices.size() * 4 + v.indices.size() * 4);
        for (float f : v.vertices) {
            std::uint32_t u;
            std::memcpy(&u, &f, sizeof(u));
            anexar_le32(bytes, u);
        }
        for (std::int32_t idx : v.indices)
            anexar_le32(bytes, static_cast<std::uint32_t>(idx));
        hashes += sha256_hex(bytes);
    }
    const std::string escala = tres_casas(escala_x) + "," + tres_casas(escala_y) + "," + tres_casas(escala_z);
    return sha256_hex(hashes + "|" + escala);
}

std::string assinatura_produto(std::vector<std::string> assinaturas_objetos)
{
    // Cópias do mesmo objeto não mudam o produto (vão em trabalho.copias).
    std::sort(assinaturas_objetos.begin(), assinaturas_objetos.end());
    assinaturas_objetos.erase(std::unique(assinaturas_objetos.begin(), assinaturas_objetos.end()), assinaturas_objetos.end());
    std::string juntas;
    for (std::size_t i = 0; i < assinaturas_objetos.size(); ++i) {
        if (i > 0)
            juntas += '\n';
        juntas += assinaturas_objetos[i];
    }
    return sha256_hex(juntas);
}

} // namespace Slic3r::Filuna
