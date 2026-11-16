#include "FilunaAnalise.hpp"
#include "FilunaGcodeResumo.hpp"

#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iterator>
#include <random>
#include <sstream>

using nlohmann::json;

namespace Slic3r { namespace Filuna {

namespace {

// ---------- texto ----------
std::string num_br(double v, int casas)
{
    char b[32];
    std::snprintf(b, sizeof(b), "%.*f", casas, v);
    std::string s(b);
    std::replace(s.begin(), s.end(), '.', ',');
    return s;
}

std::string duracao(double s)
{
    const long long t = (long long) std::llround(std::max(0., s));
    const long long h = t / 3600, m = (t % 3600) / 60, seg = t % 60;
    char            b[48];
    if (h > 0)
        std::snprintf(b, sizeof(b), "%lld h %02lld min", h, m);
    else if (m > 0)
        std::snprintf(b, sizeof(b), seg >= 10 ? "%lld min %lld s" : "%lld min", m, seg);
    else
        std::snprintf(b, sizeof(b), "%lld s", seg);
    return b;
}

std::string camadas_txt(int de, int ate)
{
    if (de < 1)
        return "antes da primeira camada";
    if (ate <= de)
        return "camada " + std::to_string(de);
    return "camadas " + std::to_string(de) + " a " + std::to_string(ate);
}

std::string altura_txt(double z0, double z1)
{
    if (std::isnan(z0))
        return {};
    if (std::isnan(z1) || std::fabs(z1 - z0) < 0.05)
        return num_br(z0, 1) + " mm de altura";
    return "de " + num_br(z0, 1) + " a " + num_br(z1, 1) + " mm de altura";
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

// etapas Bambu (stg_cur) em que a impressora parou por um problema
const std::map<int, std::string>& etapas_parada()
{
    static const std::map<int, std::string> m{
        {6, "parou: acabou o filamento"},
        {17, "parou: a tampa frontal caiu"},
        {20, "parou: falha na temperatura do bico"},
        {21, "parou: falha na temperatura da mesa"},
        {26, "parou: perdeu a comunicação com o AMS"},
        {27, "parou: ventoinha do dissipador lenta"},
        {28, "parou: erro no controle de temperatura da câmara"},
        {32, "parou: bico coberto de filamento"},
        {33, "parou: erro no cortador de filamento"},
        {34, "parou: problema detectado na primeira camada"},
        {35, "parou: bico entupido"},
    };
    return m;
}

// ---------- leitura do G-code ----------
bool comeca(const std::string& s, const char* p) { return s.compare(0, std::strlen(p), p) == 0; }

std::string apara(const std::string& s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r'))
        ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r'))
        --b;
    return s.substr(a, b - a);
}

double tempo_do_texto(const std::string& s)
{
    const auto t = tempo_dhms_em_segundos(s);
    return t ? double(*t) : 0.;
}

bool letra(const std::string& cmd, char l, double& v)
{
    for (size_t i = 0; i < cmd.size(); ++i) {
        if ((cmd[i] == l || cmd[i] == char(l + 32)) && (i == 0 || cmd[i - 1] == ' ')) {
            char* fim = nullptr;
            v         = std::strtod(cmd.c_str() + i + 1, &fim);
            return fim != cmd.c_str() + i + 1;
        }
    }
    return false;
}

std::string tipo_legivel(const std::string& t)
{
    static const std::map<std::string, std::string> m{
        {"outer wall", "parede externa"}, {"external perimeter", "parede externa"}, {"inner wall", "parede interna"},
        {"perimeter", "parede interna"}, {"overhang wall", "parede em balanço"}, {"overhang perimeter", "parede em balanço"},
        {"sparse infill", "preenchimento"}, {"internal infill", "preenchimento"}, {"internal solid infill", "preenchimento sólido"},
        {"solid infill", "preenchimento sólido"}, {"top surface", "superfície de cima"}, {"top solid infill", "superfície de cima"},
        {"bottom surface", "superfície de baixo"}, {"bridge", "ponte"}, {"bridge infill", "ponte"},
        {"internal bridge", "ponte interna"}, {"gap infill", "preenchimento de vãos"}, {"support", "suporte"},
        {"support material", "suporte"}, {"support interface", "interface do suporte"}, {"support material interface", "interface do suporte"},
        {"skirt", "saia"}, {"brim", "borda (brim)"}, {"prime tower", "torre de purga"}, {"wipe tower", "torre de purga"},
        {"ironing", "passar a ferro"}, {"floating vertical shell", "parede flutuante"}};
    std::string k;
    for (char c : t)
        k += char(std::tolower((unsigned char) c));
    k        = apara(k);
    auto it = m.find(k);
    return it != m.end() ? it->second : std::string();
}

} // namespace

// ---------- plano ----------
double PlanoImpressao::camadas_s() const
{
    double s = 0.;
    for (const CamadaPlano& c : camadas)
        s += c.duracao_s;
    return s;
}

int PlanoImpressao::camada_no_tempo(double t) const
{
    if (camadas.empty())
        return -1;
    auto it = std::upper_bound(camadas.begin(), camadas.end(), t, [](double v, const CamadaPlano& c) { return v < c.inicio_s; });
    if (it == camadas.begin())
        return 0;
    return int(std::distance(camadas.begin(), it)) - 1;
}

int PlanoImpressao::camada_no_progresso(double p) const
{
    if (camadas.empty())
        return -1;
    return camada_no_tempo(std::clamp(p, 0., 1.) * camadas_s());
}

json PlanoImpressao::to_json() const
{
    json cs = json::array();
    for (const CamadaPlano& c : camadas)
        cs.push_back({c.numero, c.z, c.altura, c.inicio_s, c.duracao_s, c.extrusao_mm, c.temp_bico, c.ventoinha, c.tipos, c.troca_cor});
    return {{"valido", valido}, {"preparo_s", preparo_s}, {"tempo_total_s", tempo_total_s}, {"filamento_mm", filamento_mm},
            {"temp_mesa", temp_mesa}, {"material", material}, {"camadas", cs}};
}

PlanoImpressao PlanoImpressao::from_json(const json& j)
{
    PlanoImpressao p;
    if (!j.is_object())
        return p;
    p.valido        = j.value("valido", false);
    p.preparo_s     = j.value("preparo_s", 0.);
    p.tempo_total_s = j.value("tempo_total_s", 0.);
    p.filamento_mm  = j.value("filamento_mm", 0.);
    p.temp_mesa     = j.value("temp_mesa", 0);
    p.material      = j.value("material", std::string());
    for (const auto& c : j.value("camadas", json::array())) {
        if (!c.is_array() || c.size() < 9)
            continue;
        CamadaPlano k;
        k.numero      = c[0].get<int>();
        k.z           = c[1].get<double>();
        k.altura      = c[2].get<double>();
        k.inicio_s    = c[3].get<double>();
        k.duracao_s   = c[4].get<double>();
        k.extrusao_mm = c[5].get<double>();
        k.temp_bico   = c[6].get<int>();
        k.ventoinha   = c[7].get<int>();
        if (c[8].is_array())
            for (const auto& t : c[8])
                if (t.is_string())
                    k.tipos.push_back(t.get<std::string>());
        k.troca_cor = c.size() > 9 && c[9].is_boolean() && c[9].get<bool>();
        p.camadas.push_back(std::move(k));
    }
    p.valido = p.valido && !p.camadas.empty();
    return p;
}

PlanoImpressao plano_do_texto(const std::string& g)
{
    PlanoImpressao p;
    double         x = 0, y = 0, z = 0, e = 0, f = 1800.;
    bool           e_relativo = true; // o Orca usa extrusão relativa (M83) por padrão
    double         bruto = 0., bruto_camada0 = 0., bruto_preparo = -1.;
    double         tempo_modelo = 0., tempo_total = 0.;
    int            temp = 0, vent = -1, ferramenta = -1;
    bool           em_camada = false;
    CamadaPlano    atual;
    std::vector<double> brutos;

    auto fechar = [&]() {
        if (!em_camada)
            return;
        atual.duracao_s = bruto - bruto_camada0; // ainda em tempo "bruto"; escala no fim
        if (atual.temp_bico == 0)
            atual.temp_bico = temp;
        p.camadas.push_back(atual);
        brutos.push_back(atual.duracao_s);
        em_camada = false;
    };
    auto abrir = [&]() {
        fechar();
        if (bruto_preparo < 0.)
            bruto_preparo = bruto;
        atual           = CamadaPlano();
        atual.numero    = int(p.camadas.size()) + 1;
        atual.temp_bico = temp;
        atual.ventoinha = vent;
        atual.z         = z;
        bruto_camada0   = bruto;
        em_camada       = true;
    };

    std::istringstream in(g);
    std::string        linha;
    while (std::getline(in, linha)) {
        std::string l = apara(linha);
        if (l.empty())
            continue;
        if (l[0] == ';') {
            const std::string c = apara(l.substr(1));
            if (comeca(c, "CHANGE_LAYER") || comeca(c, "LAYER_CHANGE"))
                abrir();
            else if (comeca(c, "Z_HEIGHT:") || comeca(c, "Z:")) {
                if (em_camada)
                    atual.z = std::atof(c.substr(c.find(':') + 1).c_str());
            } else if (comeca(c, "LAYER_HEIGHT:") || comeca(c, "HEIGHT:")) {
                if (em_camada)
                    atual.altura = std::atof(c.substr(c.find(':') + 1).c_str());
            } else if (comeca(c, "FEATURE:") || comeca(c, "TYPE:")) {
                if (em_camada) {
                    const std::string t = tipo_legivel(c.substr(c.find(':') + 1));
                    if (!t.empty() && atual.tipos.size() < 8 && std::find(atual.tipos.begin(), atual.tipos.end(), t) == atual.tipos.end())
                        atual.tipos.push_back(t);
                }
            } else if (c.find("model printing time:") != std::string::npos || c.find("total estimated time:") != std::string::npos) {
                const size_t a = c.find("model printing time:"), b = c.find("total estimated time:");
                if (a != std::string::npos)
                    tempo_modelo = tempo_do_texto(c.substr(a + 20, b != std::string::npos && b > a ? b - a - 20 : std::string::npos));
                if (b != std::string::npos)
                    tempo_total = tempo_do_texto(c.substr(b + 21));
            } else if (comeca(c, "estimated printing time") && c.find('=') != std::string::npos) {
                if (tempo_total == 0.)
                    tempo_total = tempo_do_texto(c.substr(c.find('=') + 1));
            } else if (comeca(c, "filament_type =") && p.material.empty()) {
                std::string v = apara(c.substr(c.find('=') + 1));
                p.material    = apara(v.substr(0, v.find(';')));
            } else if (comeca(c, "filament used [mm]") && c.find('=') != std::string::npos) {
                std::string v = c.substr(c.find('=') + 1);
                double      soma = 0.;
                for (std::string::size_type i = 0; i < v.size();) { // várias extrusoras: "123.4, 56.7"
                    char* fim = nullptr;
                    const double n = std::strtod(v.c_str() + i, &fim);
                    if (fim == v.c_str() + i) {
                        ++i;
                        continue;
                    }
                    soma += n;
                    i = size_t(fim - v.c_str());
                }
                p.filamento_mm = soma;
            }
            continue;
        }
        const size_t pv  = l.find(';');
        std::string  cmd = apara(pv == std::string::npos ? l : l.substr(0, pv));
        if (cmd.empty())
            continue;
        for (char& ch : cmd)
            ch = char(std::toupper((unsigned char) ch));
        double v = 0.;
        if (comeca(cmd, "G0 ") || comeca(cmd, "G1 ") || comeca(cmd, "G2 ") || comeca(cmd, "G3 ") || cmd == "G0" || cmd == "G1") {
            double nx = x, ny = y, nz = z, de = 0.;
            if (letra(cmd, 'F', v) && v > 0)
                f = v;
            if (letra(cmd, 'X', v))
                nx = v;
            if (letra(cmd, 'Y', v))
                ny = v;
            if (letra(cmd, 'Z', v))
                nz = v;
            if (letra(cmd, 'E', v)) {
                de = e_relativo ? v : v - e;
                e  = e_relativo ? e + v : v;
            }
            double dist = std::sqrt((nx - x) * (nx - x) + (ny - y) * (ny - y) + (nz - z) * (nz - z));
            if (comeca(cmd, "G2") || comeca(cmd, "G3"))
                dist *= 1.15; // arco: um pouco mais que a corda
            if (dist <= 0.)
                dist = std::fabs(de);
            bruto += dist / (f / 60.);
            if (em_camada && de > 0.)
                atual.extrusao_mm += de;
            x = nx, y = ny, z = nz;
        } else if ((cmd.size() >= 2 && cmd.size() <= 3 && cmd[0] == 'T' && std::isdigit((unsigned char) cmd[1])) || comeca(cmd, "M620 S")) {
            const int f = std::atoi(cmd.c_str() + (cmd[0] == 'T' ? 1 : 6));
            if (f >= 0 && f < 64 && (cmd[0] == 'T' || cmd.find('A') != std::string::npos)) {
                if (em_camada && ferramenta >= 0 && f != ferramenta)
                    atual.troca_cor = true;
                ferramenta = f;
            }
        } else if (comeca(cmd, "G4")) {
            if (letra(cmd, 'P', v))
                bruto += v / 1000.;
            else if (letra(cmd, 'S', v))
                bruto += v;
        } else if (comeca(cmd, "G92")) {
            if (letra(cmd, 'E', v))
                e = v;
        } else if (cmd == "M82")
            e_relativo = false;
        else if (cmd == "M83")
            e_relativo = true;
        else if (comeca(cmd, "M104") || comeca(cmd, "M109")) {
            if (letra(cmd, 'S', v) && v >= 0) {
                temp = int(std::lround(v));
                if (em_camada && temp > 0)
                    atual.temp_bico = temp;
            }
        } else if (comeca(cmd, "M140") || comeca(cmd, "M190")) {
            if (letra(cmd, 'S', v) && v > 0 && p.temp_mesa == 0)
                p.temp_mesa = int(std::lround(v));
        } else if (comeca(cmd, "M106")) {
            double pn = 0.;
            const bool tem_p = letra(cmd, 'P', pn);
            if ((!tem_p || pn <= 1.) && letra(cmd, 'S', v)) { // P0/P1 = ventoinha da peça (P2 auxiliar, P3 câmara)
                vent = int(std::lround(std::clamp(v, 0., 255.) / 255. * 100.));
                if (em_camada)
                    atual.ventoinha = std::max(atual.ventoinha, vent);
            }
        } else if (comeca(cmd, "M107")) {
            double pn = 0.;
            if (!letra(cmd, 'P', pn) || pn <= 1.)
                vent = 0;
        }
    }
    fechar();
    if (p.camadas.empty())
        return p;
    // O tempo "bruto" ignora aceleração: escala as camadas para bater com a estimativa do fatiador.
    double soma = 0.;
    for (double b : brutos)
        soma += b;
    const double preparo_bruto = std::max(0., bruto_preparo);
    double       k             = 1.25;
    if (tempo_modelo > 0. && soma > 0.)
        k = tempo_modelo / soma;
    else if (tempo_total > 0. && soma + preparo_bruto > 0.)
        k = tempo_total / (soma + preparo_bruto);
    double t = 0.;
    for (CamadaPlano& c : p.camadas) {
        c.duracao_s *= k;
        c.inicio_s = t;
        t += c.duracao_s;
    }
    p.preparo_s     = tempo_total > 0. && tempo_modelo > 0. ? std::max(0., tempo_total - tempo_modelo) : preparo_bruto * k;
    p.tempo_total_s = tempo_total > 0. ? tempo_total : t + p.preparo_s;
    p.valido        = true;
    return p;
}

json Checkpoint::to_json() const
{
    return {{"t", std::lround(t)}, {"camada", camada}, {"z", std::round(z * 100.) / 100.}, {"temp_bico", temp_bico}, {"temp_mesa", temp_mesa},
            {"progresso", progresso}, {"critico", critico}, {"motivo", motivo}};
}

std::vector<Checkpoint> mapa_checkpoints(const PlanoImpressao& p)
{
    std::vector<Checkpoint> v;
    if (!p.valido || p.camadas.empty())
        return v;
    const double total = p.camadas_s();
    if (total <= 0.)
        return v;
    auto ponto = [&](double t, bool critico, const std::string& motivo) {
        t           = std::clamp(t, 0., total);
        const int i = std::max(0, p.camada_no_tempo(std::max(0., t - 0.5)));
        Checkpoint k;
        k.t         = t;
        k.camada    = i + 1;
        k.z         = p.camadas[i].z;
        k.temp_bico = p.camadas[i].temp_bico;
        k.temp_mesa = p.temp_mesa;
        k.progresso = int(std::lround(t / total * 100.));
        k.critico   = critico;
        k.motivo    = motivo;
        return k;
    };
    // pontos críticos da peça: conferidos quando a camada delicada acabou de ser impressa
    std::vector<Checkpoint> criticos;
    auto fim_da = [&](size_t i) { return p.camadas[i].inicio_s + p.camadas[i].duracao_s; };
    auto tem    = [&](size_t i, const char* tipo) {
        return std::find(p.camadas[i].tipos.begin(), p.camadas[i].tipos.end(), tipo) != p.camadas[i].tipos.end();
    };
    criticos.push_back(ponto(fim_da(0), true, "fim da primeira camada (aderência na mesa)"));
    for (size_t i = 1; i < p.camadas.size(); ++i) {
        const std::pair<const char*, const char*> trechos[] = {{"ponte", "ponte (parte impressa no ar)"},
                                                               {"parede em balanço", "parte em balanço"},
                                                               {"superfície de cima", "superfície de cima (acabamento que aparece)"}};
        for (const auto& [tipo, motivo] : trechos)
            if (tem(i, tipo) && !tem(i - 1, tipo))
                criticos.push_back(ponto(fim_da(i), true, motivo));
        if (p.camadas[i].troca_cor && !p.camadas[i - 1].troca_cor)
            criticos.push_back(ponto(fim_da(i), true, "troca de cor"));
    }
    if (p.camadas.size() > 2)
        criticos.push_back(ponto(p.camadas.back().inicio_s, true, "última camada"));
    std::sort(criticos.begin(), criticos.end(), [](const Checkpoint& a, const Checkpoint& b) { return a.t < b.t; });
    // críticos colados (menos de 1 min) viram um só; muitos críticos ficam espaçados por igual (no máximo 24)
    std::vector<Checkpoint> unicos;
    for (const Checkpoint& k : criticos)
        if (unicos.empty() || k.t - unicos.back().t >= 60.)
            unicos.push_back(k);
        else if (unicos.back().motivo.find(k.motivo) == std::string::npos)
            unicos.back().motivo += " e " + k.motivo;
    if (unicos.size() > 24) {
        std::vector<Checkpoint> menos;
        for (size_t i = 0; i < 24; ++i)
            menos.push_back(unicos[i * (unicos.size() - 1) / 23]);
        unicos = std::move(menos);
    }
    // rotina: quanto mais longa a impressão, mais pontos (sem repetir um crítico a menos de 90 s)
    const double passo = total <= 3600. ? 300. : total <= 3. * 3600. ? 600. : total <= 8. * 3600. ? 900. : 1200.;
    for (double t = passo; t < total - 60.; t += passo) {
        const bool perto = std::any_of(unicos.begin(), unicos.end(), [t](const Checkpoint& k) { return std::fabs(k.t - t) < 90.; });
        if (!perto)
            v.push_back(ponto(t, false, "conferência de rotina"));
    }
    v.insert(v.end(), unicos.begin(), unicos.end());
    std::sort(v.begin(), v.end(), [](const Checkpoint& a, const Checkpoint& b) { return a.t < b.t; });
    return v;
}

PlanoImpressao plano_do_gcode(const std::string& caminho)
{
    boost::nowide::ifstream i(caminho, std::ios::binary);
    if (!i)
        return {};
    const std::string texto((std::istreambuf_iterator<char>(i)), std::istreambuf_iterator<char>());
    return plano_do_texto(texto);
}

// ---------- amostra ----------
Amostra amostra_do_relatorio_bambu(const json& p, double t)
{
    Amostra a;
    a.t = t;
    if (!p.is_object())
        return a;
    auto inteiro = [&p](const char* k, int padrao) -> int {
        if (!p.contains(k))
            return padrao;
        const json& v = p[k];
        if (v.is_number())
            return v.get<int>();
        if (v.is_string()) {
            try {
                return std::stoi(v.get<std::string>());
            } catch (...) {
            }
        }
        return padrao;
    };
    auto real = [&p](const char* k) -> double { return p.contains(k) && p[k].is_number() ? p[k].get<double>() : NAN; };
    const std::string gs = p.value("gcode_state", std::string());
    a.estado          = gs == "RUNNING" || gs == "PREPARE" || gs == "SLICING" ? "imprimindo"
                      : gs == "PAUSE"                                         ? "pausada"
                      : gs == "FINISH"                                        ? "concluida"
                      : gs == "FAILED"                                        ? "falhou"
                      : gs.empty()                                            ? ""
                                                                              : "livre";
    a.camada        = inteiro("layer_num", -1);
    a.total_camadas = inteiro("total_layer_num", -1);
    a.progresso     = inteiro("mc_percent", -1);
    a.bico          = real("nozzle_temper");
    a.bico_alvo     = real("nozzle_target_temper");
    a.mesa          = real("bed_temper");
    a.mesa_alvo     = real("bed_target_temper");
    const int fan   = inteiro("cooling_fan_speed", -1); // 0..15
    a.ventoinha     = fan < 0 ? -1 : int(std::lround(std::min(fan, 15) / 15. * 100.));
    a.velocidade    = inteiro("spd_lvl", -1);
    a.etapa         = inteiro("stg_cur", -1);
    const int pe    = inteiro("print_error", 0);
    if (pe != 0) {
        char b[24];
        std::snprintf(b, sizeof(b), "%08X", unsigned(pe));
        a.erro = std::string("código ") + std::string(b).substr(0, 4) + "-" + std::string(b).substr(4);
    }
    return a;
}

// ---------- ocorrência ----------
json Ocorrencia::to_json() const
{
    json j = {{"tipo", tipo}, {"gravidade", gravidade}, {"camada_de", camada_de}, {"camada_ate", camada_ate},
              {"t_de", std::lround(t_de)}, {"t_ate", std::lround(t_ate)}, {"titulo", titulo}, {"detalhe", detalhe}, {"sugestao", sugestao}};
    j["z_de"]  = std::isnan(z_de) ? json(nullptr) : json(std::round(z_de * 100.) / 100.);
    j["z_ate"] = std::isnan(z_ate) ? json(nullptr) : json(std::round(z_ate * 100.) / 100.);
    return j;
}

// ---------- análise ----------
Analise::Analise(PlanoImpressao plano, std::string nome, std::string impressora)
    : m_plano(std::move(plano)), m_nome(std::move(nome)), m_impressora(std::move(impressora)), m_checkpoints(mapa_checkpoints(m_plano))
{}

namespace {
double mediana(std::vector<double> v)
{
    if (v.empty())
        return NAN;
    std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
    return v[v.size() / 2];
}

void lembrar(std::vector<double>& v, double x)
{
    if (std::isnan(x))
        return;
    v.push_back(x);
    if (v.size() > 6) // ~30 s de leituras: uma leitura isolada não vira alerta
        v.erase(v.begin());
}
} // namespace

std::vector<json> Analise::alertas()
{
    std::vector<json> v;
    v.swap(m_alertas_novos);
    return v;
}

void Analise::alertar(const Ocorrencia& o, const std::string& mensagem, const json& conferencia, bool urgente)
{
    if (int(m_alertas.size()) >= (urgente ? 12 : 8) || m_ultimo_t - m_ultimo_alerta < (urgente ? 120. : 600.))
        return; // sem avalanche: o relatório guarda tudo, o cliente recebe o essencial
    m_ultimo_alerta = m_ultimo_t;
    json j          = {{"tipo", o.tipo}, {"gravidade", o.gravidade}, {"titulo", "Sua impressão não está conforme o previsto"},
                       {"mensagem", mensagem}, {"camada", o.camada_de}, {"t", std::lround(o.t_de)}, {"nome", m_nome}, {"impressora", m_impressora},
                       {"conferencia", conferencia}};
    j["z"]          = std::isnan(o.z_de) ? json(nullptr) : json(std::round(o.z_de * 100.) / 100.);
    m_alertas.push_back(j);
    m_alertas_novos.push_back(j);
}

void Analise::conferir(const Checkpoint& cp, const Amostra& a, int idx)
{
    const int                n = int(m_plano.camadas.size());
    std::vector<std::string> frases;
    std::string              grav = "atencao";
    json                     r    = {{"t", std::lround(cp.t)}, {"camada_esperada", cp.camada}, {"z_esperado", std::round(cp.z * 100.) / 100.},
                                     {"bico_esperado", cp.temp_bico}, {"mesa_esperada", cp.temp_mesa}, {"progresso_esperado", cp.progresso},
                                     {"critico", cp.critico}, {"motivo", cp.motivo}, {"progresso_real", a.progresso}};
    // onde a impressão está contra onde deveria estar (em tempo do plano)
    if (idx >= 0 && idx < n) {
        const CamadaPlano& k = m_plano.camadas[idx];
        r["camada_real"]     = idx + 1;
        r["z_real"]          = std::round(k.z * 100.) / 100.;
        const double fim     = k.inicio_s + k.duracao_s;
        const double atraso  = cp.t > fim ? cp.t - fim : cp.t < k.inicio_s ? -(k.inicio_s - cp.t) : 0.;
        // o que pesa é o atraso que CRESCEU desde o checkpoint anterior (um atraso antigo que parou de crescer não
        // avisa de novo) ou um atraso total grande desde a última sincronia
        const double base   = cp.critico ? 180. : 240.;
        const double cresceu = atraso - m_atraso_ant;
        const bool   fora    = std::fabs(cresceu) > std::max(base, 0.25 * (cp.t - m_t_cp_ant)) ||
                            std::fabs(atraso) > std::max(600., 0.25 * (cp.t - m_t_sincronia));
        m_atraso_ant = atraso;
        m_t_cp_ant   = cp.t;
        if (fora) {
            frases.push_back("Era para estar na camada " + std::to_string(cp.camada) + " (" + num_br(cp.z, 1) + " mm) e está na camada " +
                             std::to_string(idx + 1) + " (" + num_br(k.z, 1) + " mm): " +
                             (atraso > 0. ? "cerca de " + duracao(atraso) + " de atraso." : "cerca de " + duracao(-atraso) + " à frente do previsto."));
            if (std::fabs(cresceu) > 3. * base)
                grav = "problema";
            m_t_plano = m_t_sincronia = m_t_cp_ant = atraso > 0. ? fim : k.inicio_s; // o mapa segue da posição real
            m_atraso_ant = 0.;
        }
    }
    const double bico = mediana(m_bicos), mesa = mediana(m_mesas);
    r["bico_real"]    = std::isnan(bico) ? json(nullptr) : json(std::lround(bico));
    r["mesa_real"]    = std::isnan(mesa) ? json(nullptr) : json(std::lround(mesa));
    if (cp.temp_bico > 0 && !std::isnan(bico) && std::fabs(bico - cp.temp_bico) > 10.) {
        std::string f = "O bico deveria estar a " + std::to_string(cp.temp_bico) + " °C e está a " + std::to_string(std::lround(bico)) + " °C.";
        if (!std::isnan(a.bico_alvo) && std::fabs(a.bico_alvo - cp.temp_bico) >= 5. && std::fabs(bico - a.bico_alvo) <= 5.)
            f += " A temperatura pedida foi mudada na impressora para " + std::to_string(std::lround(a.bico_alvo)) + " °C.";
        frases.push_back(f);
        if (std::fabs(bico - cp.temp_bico) > 18.)
            grav = "problema";
    }
    if (cp.temp_mesa > 0 && !std::isnan(mesa) && mesa < cp.temp_mesa - 8.)
        frases.push_back("A mesa deveria estar a " + std::to_string(cp.temp_mesa) + " °C e está a " + std::to_string(std::lround(mesa)) + " °C.");
    r["ok"]         = frases.empty();
    r["diferencas"] = frases;
    m_conferidos.push_back(r);
    if (frases.empty())
        return;
    // o ponto crítico só entra no texto se a impressão está nele (atrasada, ela ainda não chegou lá)
    const bool no_ponto = cp.critico && (idx < 0 || std::abs(idx + 1 - cp.camada) <= 2);
    Ocorrencia o;
    o.tipo      = "checkpoint";
    o.gravidade = grav;
    o.camada_de = o.camada_ate = idx >= 0 ? idx + 1 : cp.camada;
    o.t_de = o.t_ate = desde_inicio(a.t);
    o.titulo        = "Fora do previsto no checkpoint" + std::string(cp.critico ? " crítico" : "");
    std::string det;
    for (const std::string& f : frases)
        det += (det.empty() ? "" : " ") + f;
    o.detalhe  = det;
    o.sugestao = no_ponto ? "Neste ponto a peça tem " + cp.motivo + ", onde o acabamento costuma sofrer. Verifique se há partes imperfeitas nessa altura."
                          : "Verifique se há partes imperfeitas na peça nessa altura e se o acabamento está como você deseja.";
    anotar(o);
    const Ocorrencia& anotada = m_ocorrencias.back();
    const std::string alt     = std::isnan(anotada.z_de) ? std::string() : " (" + num_br(anotada.z_de, 1) + " mm de altura)";
    alertar(anotada,
            m_nome + (m_impressora.empty() ? "" : " na " + m_impressora) + ": " + det + " " +
                (no_ponto ? "Neste ponto a peça tem " + cp.motivo + ", onde o acabamento costuma sofrer. " : std::string()) +
                "Verifique se há partes imperfeitas na impressão" + alt + " e se o acabamento está como você deseja.",
            r, false);
}

double Analise::fator_velocidade(int nivel) const
{
    switch (nivel) { // tempo relativo ao Normal (velocidade 50%, 100%, 124%, 166%)
    case 1: return 2.0;
    case 3: return 0.81;
    case 4: return 0.6;
    default: return 1.0;
    }
}

int Analise::camada_atual(const Amostra& a) const
{
    const int n = int(m_plano.camadas.size());
    if (a.camada >= 1)
        return n > 0 ? std::min(a.camada, n) - 1 : a.camada - 1;
    if (m_plano.valido && a.progresso >= 0)
        return m_plano.camada_no_progresso(a.progresso / 100.);
    return -1;
}

void Analise::preencher_altura(Ocorrencia& o) const
{
    const int n = int(m_plano.camadas.size());
    if (o.camada_de >= 1 && o.camada_de <= n)
        o.z_de = m_plano.camadas[o.camada_de - 1].z;
    if (o.camada_ate >= 1 && o.camada_ate <= n)
        o.z_ate = m_plano.camadas[o.camada_ate - 1].z;
}

void Analise::anotar(Ocorrencia o)
{
    preencher_altura(o);
    const std::string onde = camadas_txt(o.camada_de, o.camada_ate), alt = altura_txt(o.z_de, o.z_ate);
    if (o.camada_de >= 1)
        o.titulo += " (" + onde + (alt.empty() ? "" : ", " + alt) + ")";
    m_ocorrencias.push_back(std::move(o));
    const Ocorrencia& k = m_ocorrencias.back();
    if (k.tipo == "erro" || k.tipo == "parada" || (k.tipo == "fim" && k.gravidade == "problema"))
        alertar(k, m_nome + (m_impressora.empty() ? "" : " na " + m_impressora) + ": " + k.titulo + ". " + k.detalhe + " " + k.sugestao, json::object(),
                k.tipo != "parada");
}

void Analise::fechar_janela(Janela& j, const std::string& tipo, double t, double minimo_s)
{
    if (!j.ativo)
        return;
    j.ativo         = false;
    const double dur = t - j.t0;
    if (dur < minimo_s)
        return;
    Ocorrencia o;
    o.tipo      = tipo;
    o.camada_de = j.camada0 + 1;
    o.camada_ate = std::max(j.camada0, j.camada1) + 1;
    o.t_de      = desde_inicio(j.t0);
    o.t_ate     = desde_inicio(t);
    if (tipo == "bico") {
        const bool baixo = j.pior < 0;
        o.gravidade      = std::fabs(j.pior) >= 15. || dur >= 180. ? "problema" : "atencao";
        o.titulo         = baixo ? "Bico abaixo da temperatura" : "Bico acima da temperatura";
        o.detalhe        = "Ficou até " + num_br(std::fabs(j.pior), 0) + " °C " + (baixo ? "abaixo" : "acima") + " do pedido por " + duracao(dur) + ".";
        o.sugestao       = baixo ? "Bico frio extruda menos: confira nessa altura se há camadas fracas, falhas nas paredes ou pouca adesão entre camadas."
                                 : "Bico quente demais costuma deixar fios (stringing), bolhas e detalhes derretidos nessa altura.";
    } else if (tipo == "mesa") {
        o.gravidade = dur >= 300. ? "problema" : "atencao";
        o.titulo    = "A mesa esfriou durante a impressão";
        o.detalhe   = "Ficou até " + num_br(std::fabs(j.pior), 0) + " °C abaixo do pedido por " + duracao(dur) + ".";
        o.sugestao  = "Mesa fria faz a peça empenar ou soltar nas bordas: confira a base e os cantos.";
    } else if (tipo == "ventoinha") {
        o.gravidade = "atencao";
        o.titulo    = "Ventoinha da peça parada";
        o.detalhe   = "O G-code pedia ventilação e a impressora informou a ventoinha parada por " + duracao(dur) + ".";
        o.sugestao  = "Sem ventilação, pontes e partes em balanço caem ou ficam ásperas, e detalhes pequenos derretem. Confira a ventoinha.";
    }
    anotar(std::move(o));
}

void Analise::fechar_temperaturas(double t)
{
    fechar_janela(m_bico, "bico", t, 30.);
    fechar_janela(m_mesa, "mesa", t, 60.);
    fechar_janela(m_vent, "ventoinha", t, 60.);
}

void Analise::anotar_ritmo(const RitmoAberto& r)
{
    Ocorrencia   o = r.o;
    const double f = r.previsto > 0. ? r.real / r.previsto : 1.;
    o.detalhe      = "Levou " + duracao(r.real) + " onde o previsto era " + duracao(r.previsto) + " (" + num_br(f, 1) + "× o tempo).";
    if (o.tipo == "lento") {
        o.gravidade = f >= 2.5 ? "problema" : "atencao";
        o.titulo    = "Mais lento que o previsto";
        o.sugestao  = "Lentidão fora do plano costuma vir de bico parcialmente entupido, filamento travando no caminho ou a "
                      "impressora reduzindo a velocidade: confira se as paredes dessa faixa saíram finas ou falhadas.";
    } else {
        o.gravidade = "atencao";
        o.titulo    = "Mais rápido que o previsto";
        o.sugestao  = "Camadas muito mais rápidas que o plano podem indicar extrusão interrompida ou trechos pulados: "
                      "confira se faltam linhas ou preenchimento nessa altura.";
    }
    anotar(std::move(o));
}

void Analise::avaliar_ritmo(bool fim)
{
    Ritmo& r = m_janela_ritmo;
    // compara janelas de pelo menos 2 min previstos (camadas curtas sozinhas são ruído)
    if (r.de >= 0 && r.previsto > 0. && (fim || r.previsto >= 120.)) {
        const double      fator = r.real / r.previsto;
        const std::string tipo  = fator >= 1.6 && r.real - r.previsto >= 60.     ? "lento"
                                  : fator <= 0.55 && r.previsto - r.real >= 60.  ? "rapido"
                                                                                 : "";
        // trecho aberto de outro tipo (ou o ritmo voltou ao normal): fecha
        for (auto it = m_ritmo_aberto.begin(); it != m_ritmo_aberto.end();) {
            if (it->o.tipo != tipo) {
                anotar_ritmo(*it);
                it = m_ritmo_aberto.erase(it);
            } else
                ++it;
        }
        if (!tipo.empty()) {
            if (m_ritmo_aberto.empty()) {
                RitmoAberto ab;
                ab.o.tipo      = tipo;
                ab.o.camada_de = r.de;
                ab.o.t_de      = desde_inicio(r.t0);
                m_ritmo_aberto.push_back(ab);
            }
            RitmoAberto& ab = m_ritmo_aberto.back();
            ab.o.camada_ate = r.ate;
            ab.o.t_ate      = desde_inicio(r.t0) + r.real;
            ab.real += r.real;
            ab.previsto += r.previsto;
        }
        r = Ritmo();
    }
    if (fim) {
        for (const RitmoAberto& ab : m_ritmo_aberto)
            anotar_ritmo(ab);
        m_ritmo_aberto.clear();
    }
}

void Analise::amostra(const Amostra& a)
{
    const double dt = m_ultimo_t < 0. ? 0. : std::clamp(a.t - m_ultimo_t, 0., 120.);
    m_ultimo_t      = a.t;
    if (m_primeira_t < 0.)
        m_primeira_t = a.t;
    const int idx = camada_atual(a);
    const int cam = idx + 1; // 1..N (0 = desconhecida)

    // início: 1ª camada (ou progresso) informada pela impressora
    if (!comecou() && a.estado == "imprimindo" &&
        (a.camada >= 1 || a.progresso >= 1 || (a.camada < 0 && a.progresso < 0 && !std::isnan(a.bico_alvo) && a.bico_alvo > 0 &&
                                               !std::isnan(a.bico) && a.bico >= a.bico_alvo - 5))) {
        m_inicio         = a.t;
        m_camada_inicial = std::max(cam, 1);
        m_ultima_camada  = a.camada >= 1 ? a.camada : -1;
        m_t_camada       = a.t;
        m_pausa_na_camada = m_pausado_s;
        if (m_plano.valido && idx > 0 && idx < int(m_plano.camadas.size())) {
            m_t_plano = m_t_sincronia = m_t_cp_ant = m_plano.camadas[idx].inicio_s;
            while (m_proximo < m_checkpoints.size() && m_checkpoints[m_proximo].t <= m_t_plano)
                ++m_proximo;
        }
    }

    // erro informado pela impressora (antes ou durante)
    if (!a.erro.empty() && a.erro != m_erro_ant) {
        Ocorrencia o;
        o.tipo      = "erro";
        o.gravidade = "problema";
        o.camada_de = o.camada_ate = comecou() ? cam : -1;
        o.t_de = o.t_ate = desde_inicio(a.t);
        o.titulo        = "A impressora avisou um erro";
        o.detalhe       = a.erro;
        o.sugestao      = "Confira a peça nessa altura: o erro pode ter interrompido a extrusão ou mexido na posição.";
        anotar(o);
    }
    m_erro_ant = a.erro;
    // etapa em que a impressora parou por um problema (Bambu)
    if (a.etapa != m_etapa_ant) {
        auto it = etapas_parada().find(a.etapa);
        if (it != etapas_parada().end()) {
            Ocorrencia o;
            o.tipo      = "erro";
            o.gravidade = "problema";
            o.camada_de = o.camada_ate = comecou() ? cam : -1;
            o.t_de = o.t_ate = desde_inicio(a.t);
            o.titulo        = "A impressora " + it->second;
            o.detalhe       = "Informado pela impressora durante a impressão.";
            o.sugestao      = "Confira a peça nessa altura: a volta depois da parada costuma deixar marca ou falha de camada.";
            anotar(o);
        }
        m_etapa_ant = a.etapa;
    }

    // pausa
    if (a.estado == "pausada") {
        if (m_pausa_t0 < 0.) {
            m_pausa_t0     = a.t;
            m_pausa_camada = cam;
        }
        m_pausado_s += dt;
    } else if (m_pausa_t0 >= 0. && a.estado == "imprimindo") {
        const double dur = a.t - m_pausa_t0;
        Ocorrencia   o;
        o.tipo      = "pausa";
        o.gravidade = dur >= 600. ? "atencao" : "info";
        o.camada_de = o.camada_ate = m_pausa_camada;
        o.t_de      = desde_inicio(m_pausa_t0);
        o.t_ate     = desde_inicio(a.t);
        o.titulo    = "Pausa de " + duracao(dur);
        o.detalhe   = "A impressão ficou pausada e depois continuou.";
        o.sugestao  = "A volta de uma pausa costuma deixar uma linha visível nessa altura; em pausas longas a camada seguinte pode colar menos.";
        anotar(o);
        m_pausa_t0 = -1.;
    }

    if (!comecou() || a.estado != "imprimindo") {
        if (a.estado != "imprimindo")
            fechar_temperaturas(a.t);
        return;
    }
    const double ds = desde_inicio(a.t);
    m_maior_camada  = std::max(m_maior_camada, cam);
    lembrar(m_bicos, a.bico);
    lembrar(m_mesas, a.mesa);
    if (a.velocidade >= 1)
        m_velocidade = a.velocidade;
    m_t_plano += dt / fator_velocidade(m_velocidade);
    while (m_proximo < m_checkpoints.size() && m_t_plano >= m_checkpoints[m_proximo].t)
        conferir(m_checkpoints[m_proximo++], a, idx);

    // número de camadas diferente do plano: a impressora pode estar com outro arquivo
    if (!m_camadas_anotado && m_plano.valido && a.total_camadas > 0 && std::abs(a.total_camadas - int(m_plano.camadas.size())) > 1) {
        m_camadas_anotado = true;
        Ocorrencia o;
        o.tipo      = "camadas";
        o.gravidade = "atencao";
        o.t_de = o.t_ate = ds;
        o.titulo        = "Número de camadas diferente do plano";
        o.detalhe       = "A impressora informa " + std::to_string(a.total_camadas) + " camadas e o arquivo fatiado tem " +
                    std::to_string(m_plano.camadas.size()) + ".";
        o.sugestao = "A impressora pode estar com outro arquivo: as comparações de ritmo deste relatório ficam menos precisas.";
        anotar(o);
    }

    // velocidade mudada na impressora
    if (a.velocidade >= 1) {
        if (m_velocidade_ant >= 1 && a.velocidade != m_velocidade_ant) {
            Ocorrencia o;
            o.tipo      = "velocidade";
            o.gravidade = "info";
            o.camada_de = o.camada_ate = cam;
            o.t_de = o.t_ate = ds;
            o.titulo        = "Velocidade mudada para " + nome_velocidade(a.velocidade);
            o.detalhe       = "Antes estava em " + nome_velocidade(m_velocidade_ant) + ".";
            o.sugestao      = a.velocidade > m_velocidade_ant ? "Velocidade maior pode deixar paredes menos lisas e piorar detalhes a partir dessa altura."
                                                              : "Velocidade menor costuma melhorar o acabamento; a impressão demora mais.";
            anotar(o);
        }
        m_velocidade_ant = a.velocidade;
        m_velocidade     = a.velocidade;
    }

    // ritmo: duração real de cada camada contra o plano (sem o tempo de pausa)
    if (a.camada >= 1 && m_plano.valido && a.camada != m_ultima_camada) {
        if (m_ultima_camada >= 1 && a.camada > m_ultima_camada && a.camada <= int(m_plano.camadas.size()) + 1) {
            double previsto = 0.;
            for (int c = m_ultima_camada; c < a.camada && c <= int(m_plano.camadas.size()); ++c)
                previsto += m_plano.camadas[c - 1].duracao_s;
            previsto *= fator_velocidade(m_velocidade);
            const double real = std::max(0., (a.t - m_t_camada) - (m_pausado_s - m_pausa_na_camada));
            m_previsto_feito += previsto;
            if (m_ultima_camada > m_camada_inicial || m_camada_inicial <= 1) { // a 1ª camada vista pode ter começado antes
                Ritmo& r = m_janela_ritmo;
                if (r.de < 0) {
                    r.de = m_ultima_camada;
                    r.t0 = m_t_camada;
                }
                r.ate = a.camada - 1;
                r.real += real;
                r.previsto += previsto;
                avaliar_ritmo(false);
            }
        }
        m_ultima_camada   = a.camada;
        m_t_camada        = a.t;
        m_pausa_na_camada = m_pausado_s;
    }

    // parada: sem avançar camada nem progresso por muito mais que o previsto
    if (cam == m_parado_camada && a.progresso == m_parado_progresso) {
        if (m_parado_desde < 0.)
            m_parado_desde = a.t;
        double limite = 300.;
        if (m_plano.valido && idx >= 0 && idx < int(m_plano.camadas.size()))
            limite = std::max(limite, 3. * m_plano.camadas[idx].duracao_s * fator_velocidade(m_velocidade));
        if (m_plano.valido && a.camada < 1)
            limite = std::max(limite, 0.025 * m_plano.camadas_s()); // só progresso: 1% pode levar minutos
        if (!m_parada_anotada && a.t - m_parado_desde > limite) {
            m_parada_anotada = true;
            Ocorrencia o;
            o.tipo      = "parada";
            o.gravidade = "problema";
            o.camada_de = o.camada_ate = cam;
            o.t_de      = desde_inicio(m_parado_desde);
            o.t_ate     = ds;
            o.titulo    = "Impressora parada sem avançar";
            o.detalhe   = "Ficou " + duracao(a.t - m_parado_desde) + " sem avançar, bem mais que o previsto para esse trecho.";
            o.sugestao  = "Parada com o bico sobre a peça costuma deixar uma bolinha ou marca; se a extrusão travou, a camada seguinte pode não ter colado.";
            anotar(o);
        }
    } else {
        if (m_parada_anotada && !m_ocorrencias.empty())
            for (auto it = m_ocorrencias.rbegin(); it != m_ocorrencias.rend(); ++it)
                if (it->tipo == "parada") {
                    it->t_ate   = ds;
                    it->detalhe = "Ficou " + duracao(a.t - m_parado_desde) + " sem avançar, bem mais que o previsto para esse trecho.";
                    break;
                }
        m_parado_desde     = -1.;
        m_parada_anotada   = false;
        m_parado_camada    = cam;
        m_parado_progresso = a.progresso;
    }

    // bico: longe do alvo por mais de 30 s (depois da 1ª camada aquecer)
    const bool aquecido = cam > 1 || ds > 120.;
    if (aquecido && !std::isnan(a.bico) && !std::isnan(a.bico_alvo) && a.bico_alvo > 0.) {
        const double dev = a.bico - a.bico_alvo;
        if (std::fabs(dev) > 8.) {
            if (m_bico.ativo && (dev < 0) != (m_bico.pior < 0))
                fechar_janela(m_bico, "bico", a.t, 30.);
            if (!m_bico.ativo)
                m_bico = {true, a.t, a.t, dev, idx, idx};
            if (std::fabs(dev) > std::fabs(m_bico.pior))
                m_bico.pior = dev;
            m_bico.camada1 = idx;
            if (!m_bico_alertado && std::fabs(m_bico.pior) >= 15. && a.t - m_bico.t0 >= 30.) {
                m_bico_alertado = true;
                Ocorrencia o;
                o.tipo      = "bico";
                o.gravidade = "problema";
                o.camada_de = o.camada_ate = cam;
                o.t_de = o.t_ate = ds;
                preencher_altura(o);
                const std::string alt = std::isnan(o.z_de) ? std::string() : " (" + num_br(o.z_de, 1) + " mm de altura)";
                alertar(o,
                        m_nome + (m_impressora.empty() ? "" : " na " + m_impressora) + ": o bico deveria estar a " +
                            std::to_string(std::lround(a.bico_alvo)) + " °C e está a " + std::to_string(std::lround(a.bico)) + " °C há " +
                            duracao(a.t - m_bico.t0) + ". Bico " + (dev < 0 ? "frio extruda menos" : "quente demais deixa fios e bolhas") +
                            ": verifique se há partes imperfeitas na impressão" + alt + " e se o acabamento está como você deseja.",
                        json{{"bico_esperado", std::lround(a.bico_alvo)}, {"bico_real", std::lround(a.bico)}}, true);
            }
        } else {
            fechar_janela(m_bico, "bico", a.t, 30.);
            m_bico_alertado = false;
        }
        // alvo mudado na impressora (diferente do G-code) por mais de 1 min
        const int pedido = m_plano.valido && idx >= 0 && idx < int(m_plano.camadas.size()) ? m_plano.camadas[idx].temp_bico : 0;
        const int alvo   = int(std::lround(a.bico_alvo));
        if (pedido > 0 && std::abs(alvo - pedido) >= 5) {
            if (std::isnan(m_alvo_visto) || int(m_alvo_visto) != alvo) {
                m_alvo_visto   = alvo;
                m_alvo_mudou_t = a.t;
            } else if (a.t - m_alvo_mudou_t >= 60. &&
                       std::find(m_alvos_anotados.begin(), m_alvos_anotados.end(), alvo) == m_alvos_anotados.end()) {
                m_alvos_anotados.push_back(alvo);
                Ocorrencia o;
                o.tipo      = "temp_alterada";
                o.gravidade = "info";
                o.camada_de = o.camada_ate = cam;
                o.t_de = o.t_ate = ds;
                o.titulo        = "Temperatura do bico mudada na impressora";
                o.detalhe       = "O arquivo pedia " + std::to_string(pedido) + " °C e a impressora passou a usar " + std::to_string(alvo) + " °C.";
                o.sugestao      = alvo < pedido ? "Temperatura menor que a do perfil pode enfraquecer a união das camadas a partir daqui."
                                                : "Temperatura maior que a do perfil pode deixar fios e brilho diferente a partir daqui.";
                anotar(o);
            }
        } else
            m_alvo_visto = NAN;
    }
    // mesa: abaixo do alvo por mais de 1 min, ou aquecimento desligado no meio
    if (!std::isnan(a.mesa) && !std::isnan(a.mesa_alvo)) {
        if (a.mesa_alvo > 0. && a.mesa < a.mesa_alvo - 5.) {
            if (!m_mesa.ativo)
                m_mesa = {true, a.t, a.t, a.mesa - a.mesa_alvo, idx, idx};
            m_mesa.pior    = std::min(m_mesa.pior, a.mesa - a.mesa_alvo);
            m_mesa.camada1 = idx;
        } else
            fechar_janela(m_mesa, "mesa", a.t, 60.);
        if (a.mesa_alvo <= 0. && m_plano.temp_mesa > 0 && !m_mesa_desligada_anotada && cam > 1) {
            m_mesa_desligada_anotada = true;
            Ocorrencia o;
            o.tipo      = "mesa";
            o.gravidade = "atencao";
            o.camada_de = o.camada_ate = cam;
            o.t_de = o.t_ate = ds;
            o.titulo        = "Aquecimento da mesa desligado no meio da impressão";
            o.detalhe       = "O arquivo pedia a mesa a " + std::to_string(m_plano.temp_mesa) + " °C.";
            o.sugestao      = "Com a mesa fria a peça pode soltar ou empenar; confira a base.";
            anotar(o);
        }
    }
    // ventoinha: o plano pede e a impressora informa parada (a partir da 3ª camada)
    if (a.ventoinha >= 0 && m_plano.valido && idx >= 2 && idx < int(m_plano.camadas.size())) {
        if (m_plano.camadas[idx].ventoinha >= 30 && a.ventoinha == 0) {
            if (!m_vent.ativo)
                m_vent = {true, a.t, a.t, 0., idx, idx};
            m_vent.camada1 = idx;
        } else
            fechar_janela(m_vent, "ventoinha", a.t, 60.);
    }

    // série para o gráfico do relatório (a cada minuto)
    if (a.t - m_ultima_serie >= 60.) {
        m_ultima_serie = a.t;
        m_serie.push_back({{"t", std::lround(ds)}, {"camada", cam}, {"bico", std::isnan(a.bico) ? json(nullptr) : json(std::lround(a.bico))},
                           {"alvo", std::isnan(a.bico_alvo) ? json(nullptr) : json(std::lround(a.bico_alvo))},
                           {"mesa", std::isnan(a.mesa) ? json(nullptr) : json(std::lround(a.mesa))}, {"progresso", a.progresso}});
        if (m_serie.size() > 1500)
            m_serie.erase(m_serie.begin(), m_serie.begin() + 500);
    }
    m_ultimo_progresso = a.progresso;
}

json Analise::relatorio(const std::string& estado_final)
{
    const double t_fim = m_ultimo_t;
    fechar_temperaturas(t_fim);
    avaliar_ritmo(true);
    if (m_pausa_t0 >= 0.)
        m_pausa_t0 = -1.;
    const int n = int(m_plano.camadas.size());
    // como terminou
    if (estado_final != "concluida") {
        Ocorrencia o;
        o.tipo      = "fim";
        o.gravidade = estado_final == "cancelada" ? "atencao" : "problema";
        o.camada_de = o.camada_ate = m_maior_camada;
        o.t_de = o.t_ate = desde_inicio(t_fim);
        o.titulo        = estado_final == "cancelada" ? "Impressão cancelada" : estado_final == "falhou" ? "A impressão falhou" : "A impressão foi interrompida";
        o.detalhe       = m_ultimo_progresso >= 0 ? "Parou com " + std::to_string(m_ultimo_progresso) + "% feito." : "Parou antes do fim.";
        o.sugestao      = "A peça ficou incompleta a partir dessa altura.";
        anotar(o);
    } else if (m_plano.valido && m_maior_camada > 0 && m_maior_camada < n - 2 && m_camada_inicial <= 1) {
        Ocorrencia o;
        o.tipo      = "fim";
        o.gravidade = "atencao";
        o.camada_de = o.camada_ate = m_maior_camada;
        o.t_de = o.t_ate = desde_inicio(t_fim);
        o.titulo        = "Terminou antes da última camada do plano";
        o.detalhe       = "A última camada informada foi a " + std::to_string(m_maior_camada) + " de " + std::to_string(n) + ".";
        o.sugestao      = "Confira se o topo da peça está completo.";
        anotar(o);
    }
    // tempo total contra o previsto
    const double real = comecou() ? std::max(0., t_fim - m_inicio - m_pausado_s) : 0.;
    double prev = 0.;
    if (m_plano.valido && m_previsto_feito > 0.) { // camadas feitas na velocidade de cada momento + as que faltaram
        prev = m_previsto_feito;
        for (int c = std::max(m_ultima_camada, 1); c <= n; ++c)
            prev += m_plano.camadas[c - 1].duracao_s * fator_velocidade(m_velocidade);
    } else if (m_plano.valido)
        prev = m_plano.camadas_s() * fator_velocidade(m_velocidade);
    // a demora já explicada por um trecho lento ou uma parada não vira outra ocorrência
    const bool explicada = std::any_of(m_ocorrencias.begin(), m_ocorrencias.end(),
                                       [](const Ocorrencia& o) { return o.tipo == "lento" || o.tipo == "parada"; });
    if (estado_final == "concluida" && prev > 0. && !explicada && m_camada_inicial <= 1 && real > prev * 1.25 && real - prev > 600.) {
        Ocorrencia o;
        o.tipo      = "tempo";
        o.gravidade = "info";
        o.t_de = o.t_ate = desde_inicio(t_fim);
        o.titulo        = "Demorou mais que o previsto";
        o.detalhe       = "Levou " + duracao(real) + " imprimindo; o previsto era " + duracao(prev) + ".";
        o.sugestao      = "Se não houve mudança de velocidade, vale conferir o bico e o caminho do filamento antes da próxima.";
        anotar(o);
    }
    std::sort(m_ocorrencias.begin(), m_ocorrencias.end(), [](const Ocorrencia& a, const Ocorrencia& b) { return a.t_de < b.t_de; });
    int problemas = 0, atencoes = 0, infos = 0;
    json lista    = json::array();
    for (const Ocorrencia& o : m_ocorrencias) {
        (o.gravidade == "problema" ? problemas : o.gravidade == "atencao" ? atencoes : infos)++;
        lista.push_back(o.to_json());
    }
    const int nota = std::clamp(100 - 25 * problemas - 10 * atencoes - 2 * infos, 0, 100);
    std::string resumo;
    if (problemas + atencoes == 0)
        resumo = estado_final == "concluida" ? "Impressão sem anomalias: o que a impressora informou bateu com o plano do arquivo."
                                             : "A impressão não terminou.";
    else
        resumo = std::to_string(problemas + atencoes) + (problemas + atencoes == 1 ? " ponto para conferir na peça" : " pontos para conferir na peça") +
                 (problemas > 0 ? " (" + std::to_string(problemas) + (problemas == 1 ? " problema)" : " problemas)") : "") + ".";
    json r = {{"versao", 1},
              {"nome", m_nome},
              {"impressora", m_impressora},
              {"estado_final", estado_final},
              {"com_plano", m_plano.valido},
              {"camadas_plano", n},
              {"camada_max", m_maior_camada},
              {"acompanhou_desde_camada", m_camada_inicial},
              {"imprimindo_s", std::lround(real)},
              {"previsto_s", std::lround(prev)},
              {"pausado_s", std::lround(m_pausado_s)},
              {"altura_mm", n > 0 ? m_plano.camadas.back().z : 0.},
              {"nota", nota},
              {"problemas", problemas},
              {"atencoes", atencoes},
              {"infos", infos},
              {"resumo", resumo},
              {"ocorrencias", lista},
              {"checkpoints_previstos", m_checkpoints.size()},
              {"checkpoints", m_conferidos},
              {"alertas", m_alertas},
              {"serie", m_serie}};
    return r;
}

// ---------- monitor ----------
Monitor& Monitor::get()
{
    static Monitor m;
    return m;
}

void Monitor::pasta(const std::string& caminho)
{
    std::lock_guard<std::mutex> l(m_trava);
    m_pasta = caminho;
}

std::string Monitor::arquivo_plano(const std::string& dev_id) const
{
    std::string limpo;
    for (char c : dev_id)
        limpo += std::isalnum((unsigned char) c) ? c : '_';
    return m_pasta.empty() ? std::string() : (boost::filesystem::path(m_pasta) / ("plano-" + limpo + ".json")).string();
}

void Monitor::guardar_plano(const std::string& dev_id, const Sessao& s, const PlanoImpressao& p) const
{
    const std::string arq = arquivo_plano(dev_id);
    if (arq.empty())
        return;
    boost::system::error_code ec;
    boost::filesystem::create_directories(boost::filesystem::path(arq).parent_path(), ec);
    boost::nowide::ofstream o(arq, std::ios::binary | std::ios::trunc);
    o << json{{"id", s.id}, {"nome", s.nome}, {"impressora", s.impressora}, {"trabalho_id", s.trabalho_id}, {"criado", (long long) std::time(nullptr)},
              {"plano", p.to_json()}}
             .dump();
}

bool Monitor::ler_plano(const std::string& dev_id, Sessao& s, double t)
{
    const std::string arq = arquivo_plano(dev_id);
    if (arq.empty())
        return false;
    boost::nowide::ifstream i(arq, std::ios::binary);
    if (!i)
        return false;
    const json j = json::parse(std::string((std::istreambuf_iterator<char>(i)), std::istreambuf_iterator<char>()), nullptr, false);
    if (!j.is_object() || (long long) std::time(nullptr) - j.value("criado", 0LL) > 3 * 86400) // plano velho: era outra impressão
        return false;
    s.nome        = j.value("nome", std::string());
    s.impressora  = j.value("impressora", s.impressora);
    s.trabalho_id = j.value("trabalho_id", std::string());
    s.analise     = std::make_unique<Analise>(PlanoImpressao::from_json(j.value("plano", json::object())), s.nome, s.impressora);
    s.id          = j.value("id", std::string());
    s.criada      = t;
    return true;
}

void Monitor::esquecer_plano(const std::string& dev_id) const
{
    const std::string arq = arquivo_plano(dev_id);
    boost::system::error_code ec;
    if (!arq.empty())
        boost::filesystem::remove(arq, ec);
}

namespace {
std::string novo_id()
{
    static std::mt19937_64 g{std::random_device{}()};
    char b[24];
    std::snprintf(b, sizeof(b), "%016llx", (unsigned long long) g());
    return b;
}
} // namespace

void Monitor::marcar(const Sessao& s, const std::string& dev_id, std::vector<json>& alertas) const
{
    for (json& a : alertas) {
        a["sessao"]      = s.id;
        a["trabalho_id"] = s.trabalho_id;
        a["dev_id"]      = dev_id;
    }
}

void Monitor::comecar(const std::string& dev_id, const PlanoImpressao& plano, const std::string& nome, const std::string& impressora,
                      const std::string& trabalho_id)
{
    std::lock_guard<std::mutex> l(m_trava);
    Sessao s;
    s.id          = novo_id();
    s.nome        = nome;
    s.impressora  = impressora;
    s.trabalho_id = trabalho_id;
    s.analise     = std::make_unique<Analise>(plano, nome, impressora);
    s.criada      = double(std::time(nullptr));
    guardar_plano(dev_id, s, plano);
    m_sessoes[dev_id] = std::move(s);
}

json Monitor::encerrar(const std::string& dev_id, Sessao& s, const std::string& estado)
{
    json r = s.analise->relatorio(estado);
    r["sessao"]      = s.id;
    r["trabalho_id"] = s.trabalho_id;
    r["dev_id"]      = dev_id;
    r["fim_em"]      = (long long) std::time(nullptr);
    if (!m_pasta.empty()) { // cópia no computador do cliente
        boost::system::error_code ec;
        const boost::filesystem::path dir = boost::filesystem::path(m_pasta) / "relatorios";
        boost::filesystem::create_directories(dir, ec);
        char nome[64];
        std::snprintf(nome, sizeof(nome), "%lld.json", (long long) std::time(nullptr));
        boost::nowide::ofstream o((dir / nome).string(), std::ios::binary | std::ios::trunc);
        o << r.dump(1);
    }
    esquecer_plano(dev_id);
    return r;
}

std::optional<PlanoImpressao> Monitor::plano(const std::string& dev_id)
{
    std::lock_guard<std::mutex> l(m_trava);
    auto it = m_sessoes.find(dev_id);
    if (it == m_sessoes.end() || !it->second.analise)
        return std::nullopt;
    return it->second.analise->plano();
}

Monitor::Resultado Monitor::amostra(const std::string& dev_id, const std::string& impressora, const Amostra& a)
{
    std::lock_guard<std::mutex> l(m_trava);
    Resultado res;
    auto it = m_sessoes.find(dev_id);
    const bool imprimindo = a.estado == "imprimindo" || a.estado == "pausada";
    if (it == m_sessoes.end()) {
        if (!imprimindo)
            return res;
        Sessao s;
        s.impressora = impressora;
        s.id = novo_id();
        if (!ler_plano(dev_id, s, a.t)) { // impressão começada fora do Filuna (ou plano perdido): só o que a impressora informa
            s.nome    = "Impressão em " + impressora;
            s.analise = std::make_unique<Analise>(PlanoImpressao(), s.nome, impressora);
            s.criada  = a.t;
        }
        it = m_sessoes.emplace(dev_id, std::move(s)).first;
    }
    Sessao& s = it->second;
    if (imprimindo) {
        s.viu_imprimindo  = true;
        s.livres          = 0;
        s.desligada_desde = -1.;
    }
    s.analise->amostra(a);
    res.alertas = s.analise->alertas();
    std::string fim;
    if (a.estado == "concluida" || a.estado == "falhou" || a.estado == "cancelada")
        fim = s.viu_imprimindo ? a.estado : std::string();
    else if (a.estado == "livre" && s.viu_imprimindo && ++s.livres >= 3)
        fim = s.analise->comecou() ? "cancelada" : "interrompida";
    else if (a.estado == "desligada" && s.viu_imprimindo) {
        if (s.desligada_desde < 0.)
            s.desligada_desde = a.t;
        else if (a.t - s.desligada_desde > 30 * 60)
            fim = "interrompida";
    } else if (!s.viu_imprimindo && a.t - s.criada > 30 * 60) { // plano registrado e a impressão nunca começou
        esquecer_plano(dev_id);
        m_sessoes.erase(it);
        return res;
    }
    if (fim.empty()) {
        marcar(s, dev_id, res.alertas);
        return res;
    }
    json r = encerrar(dev_id, s, fim);
    for (json& x : s.analise->alertas()) // a falha no fim também avisa
        res.alertas.push_back(std::move(x));
    marcar(s, dev_id, res.alertas);
    m_sessoes.erase(it);
    if (!r.value("com_plano", false) && r.value("camada_max", -1) < 1 && r.value("imprimindo_s", 0) < 60)
        return res; // não chegou a imprimir: nada para relatar
    res.relatorio = std::move(r);
    return res;
}

}} // namespace Slic3r::Filuna
