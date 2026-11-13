#include "FilunaDados.hpp"

#include "FilunaPanel.hpp"
#include "FilunaSync.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "I18N.hpp"
#include "MainFrame.hpp"
#include "PartPlate.hpp"
#include "Plater.hpp"
#include "format.hpp"

#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Filuna/FilunaCalculos.hpp"
#include "slic3r/Filuna/FilunaDb.hpp"
#include "slic3r/Filuna/FilunaGcodeResumo.hpp"
#include "slic3r/Filuna/FilunaProduto.hpp"
#include "slic3r/Filuna/FilunaRecursos.hpp"
#include "slic3r/Filuna/FilunaVersoes.hpp"
#include "libslic3r_version.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Slic3r { namespace GUI {

using nlohmann::json;
namespace F = Slic3r::Filuna;

F::FilunaDb& filuna_db()
{
    static std::unique_ptr<F::FilunaDb> db;
    static std::mutex                   trava;
    std::lock_guard<std::mutex>         lock(trava);
    if (!db) {
        const boost::filesystem::path pasta = boost::filesystem::path(data_dir()) / "filuna";
        boost::filesystem::create_directories(pasta);
        db = F::FilunaDb::abrir((pasta / "filuna.db").string());
        BOOST_LOG_TRIVIAL(info) << "Filuna: banco local aberto, esquema " << db->versao_esquema();
        // Cópia no servidor (03-SERVIDOR §4): a sincronização usa uma conexão própria do mesmo arquivo.
        FilunaSync::instancia().iniciar((pasta / "filuna.db").string(), wxGetApp().app_config->get("filuna_token"),
                                        [] { wxGetApp().CallAfter([] { FilunaPanel::broadcast("estoque"); }); });
    }
    return *db;
}

void filuna_sync_avisar() { FilunaSync::instancia().avisar(wxGetApp().app_config->get("filuna_token")); }

namespace {

std::string minusculas(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

json tipo_json(const F::FilunaDb::TipoCadastrado& t)
{
    return {{"id", t.id},           {"marca", t.marca},       {"material", t.material}, {"nome_produto", t.nome_produto},
            {"cor_nome", t.cor_nome}, {"cor_hex", t.cor_hex}, {"diametro_mm", t.diametro_mm}, {"preset_orca", t.preset_orca}};
}

json bobina_json(const F::FilunaDb::Bobina& b, const std::map<std::string, json>& tipos)
{
    json j = {{"id", b.id},
              {"codigo", b.codigo},
              {"estado", b.estado},
              {"local", b.local},
              {"capacidade_g", b.capacidade_g},
              {"tara_g", b.tara_g},
              {"restante_g", b.restante_g},
              {"preco_centavos", b.preco_centavos},
              {"estimado", b.estimado},
              {"ultimo_uso_em", b.ultimo_uso_em},
              {"comprada_em", b.comprada_em}};
    const auto it = tipos.find(b.filamento_tipo_id);
    j["tipo"]     = it != tipos.end() ? it->second : json::object();
    return j;
}

std::map<std::string, json> mapa_tipos(const F::FilunaDb& db)
{
    std::map<std::string, json> m;
    for (const auto& t : db.tipos())
        m[t.id] = tipo_json(t);
    return m;
}

json opcional(const std::optional<std::int64_t>& v) { return v ? json(*v) : json(); }

F::FimTrabalho fim_de_texto(const std::string& s)
{
    if (s == "concluido")
        return F::FimTrabalho::Concluido;
    if (s == "cancelado")
        return F::FimTrabalho::Cancelado;
    if (s == "falhou")
        return F::FimTrabalho::Falhou;
    throw std::invalid_argument("fim inválido: use concluido, cancelado ou falhou");
}

std::string agora_menos_dias(int dias)
{
    std::time_t t = std::time(nullptr) - static_cast<std::time_t>(dias) * 86400;
    std::tm     tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

void avisar_telas() { FilunaPanel::broadcast("estoque"); }

// Situação do arquivo do produto: "aqui" (abre), "ausente" (não está no caminho) ou "outro_pc".
json arquivo_json(F::FilunaDb& db, const std::string& produto_id)
{
    const auto a = produto_id.empty() ? std::nullopt : db.arquivo_do_produto(produto_id);
    if (!a)
        return json();
    std::string situacao = "outro_pc";
    if (a->pc_id == db.id_do_computador())
        situacao = boost::filesystem::exists(a->caminho) ? "aqui" : "ausente";
    return json{{"nome", a->nome}, {"situacao", situacao}};
}

} // namespace

std::optional<json> filuna_dados_acao(const std::string& acao, const json& d)
{
    static const char* const PREFIXOS[] = {"estoque.", "bobina.", "historico.", "trabalho.", "produto.", "painel.", "previsao.", "manutencao.", "sync.", "arquivo.", "versoes."};
    if (std::none_of(std::begin(PREFIXOS), std::end(PREFIXOS), [&acao](const char* p) { return acao.rfind(p, 0) == 0; }))
        return std::nullopt;

    F::FilunaDb& db = filuna_db();
    // Ações que gravam no banco: a sincronização sobe em 5 segundos (03-SERVIDOR §4).
    static const char* const GRAVAM[] = {"estoque.criar", "estoque.pesar", "estoque.arquivar", "estoque.abrir", "estoque.local",
                                         "trabalho.finalizar", "produto.unir", "manutencao.adicionar", "manutencao.registrar"};
    if (std::any_of(std::begin(GRAVAM), std::end(GRAVAM), [&acao](const char* g) { return acao == g; }))
        wxGetApp().CallAfter([] { filuna_sync_avisar(); });

    if (acao == "estoque.listar") {
        const auto tipos = mapa_tipos(db);
        json       lista = json::array();
        for (const auto& b : db.bobinas(d.value("incluir_arquivadas", false)))
            lista.push_back(bobina_json(b, tipos));
        json t = json::array();
        for (const auto& [id, j] : tipos)
            t.push_back(j);
        return json{{"bobinas", lista}, {"tipos", t}};
    }
    if (acao == "estoque.criar") {
        F::FilunaDb::FilamentoTipo t;
        t.marca       = d.value("marca", std::string());
        t.material    = d.value("material", std::string("PLA"));
        t.cor_nome    = d.value("cor_nome", std::string());
        t.cor_hex     = d.value("cor_hex", std::string());
        t.diametro_mm = d.value("diametro_mm", 1.75);
        t.preset_orca = d.value("preset_orca", std::string());
        const double capacidade = d.value("capacidade_g", 1000.);
        const double tara       = d.value("tara_g", 0.);
        // Preço digitado por kg (R$); o banco guarda o preço da bobina em centavos (02-DADOS §9).
        const std::int64_t preco = static_cast<std::int64_t>(std::llround(d.value("preco_kg", 0.) * 100. * capacidade / 1000.));
        const std::string  tipo  = db.tipo_igual_ou_novo(t);
        json ids = json::array();
        if (d.value("usada", false)) {
            ids.push_back(db.cadastrar_bobina_usada(tipo, capacidade, tara, d.value("restante_g", capacidade), d.value("estimado", false), preco));
        } else {
            for (const auto& id : db.cadastrar_bobinas(tipo, std::max(1, d.value("quantidade", 1)), capacidade, tara, preco,
                                                       d.value("comprada_em", std::string())))
                ids.push_back(id);
        }
        avisar_telas();
        return json{{"ids", ids}};
    }
    const std::string id = d.value("id", std::string());
    if (acao == "estoque.pesar") {
        db.pesar_bobina(id, d.value("peso_g", 0.));
        avisar_telas();
        return json::object();
    }
    if (acao == "estoque.arquivar") {
        db.descartar_bobina(id, d.value("obs", std::string()));
        avisar_telas();
        return json::object();
    }
    if (acao == "estoque.abrir") {
        db.abrir_bobina(id);
        avisar_telas();
        return json::object();
    }
    if (acao == "estoque.local") {
        db.definir_local(id, d.value("local", std::string()));
        avisar_telas();
        return json::object();
    }
    if (acao == "bobina.eventos") {
        json l = json::array();
        for (const auto& e : db.eventos_da_bobina(id))
            l.push_back({{"id", e.id}, {"tipo", e.tipo}, {"delta_g", e.delta_g}, {"estimado", e.estimado}, {"quando", e.criado_em},
                         {"trabalho_id", e.trabalho_id}, {"obs", e.obs}});
        return json{{"eventos", l}};
    }
    if (acao == "bobina.etiqueta") {
        const auto b = db.bobina(id);
        if (!b)
            throw std::runtime_error("bobina não encontrada");
        const auto t = db.tipo(b->filamento_tipo_id);
        return json{{"codigo", b->codigo},
                    {"titulo", t ? t->material + " " + t->cor_nome : std::string()},
                    {"marca", t ? t->marca : std::string()},
                    {"cor_hex", t ? t->cor_hex : std::string()},
                    {"capacidade_g", b->capacidade_g},
                    // FLN-<id>: o mesmo texto que o app Filuna lê. A bobina vai para o servidor com este id,
                    // então a etiqueta impressa antes da sincronização continua valendo depois.
                    {"qr", "FLN-" + b->id}};
    }
    if (acao == "historico.listar") {
        json l = json::array();
        for (const auto& t : db.trabalhos(d.value("limite", 100)))
            l.push_back({{"id", t.id},
                         {"produto_id", t.produto_id},
                         {"produto", t.produto_nome},
                         {"impressora", t.impressora_nome},
                         {"estado", t.estado},
                         {"criado_em", t.criado_em},
                         {"terminado_em", t.terminado_em},
                         {"tempo_previsto_s", opcional(t.tempo_previsto_s)},
                         {"tempo_real_s", opcional(t.tempo_real_s)},
                         {"camadas_total", t.camadas_total ? json(*t.camadas_total) : json()},
                         {"previsto_g", t.previsto_g},
                         {"real_g", t.real_g},
                         {"estimado", t.estimado},
                         {"bobinas", t.bobinas},
                         {"arquivo", arquivo_json(db, t.produto_id)}});
        return json{{"trabalhos", l}};
    }
    if (acao == "trabalho.finalizar") {
        const F::FimTrabalho fim = fim_de_texto(d.value("fim", std::string()));
        std::optional<int>    camada, total;
        std::optional<double> progresso, resposta;
        if (d.contains("camada") && d["camada"].is_number()) camada = d["camada"].get<int>();
        if (d.contains("camadas_total") && d["camadas_total"].is_number()) total = d["camadas_total"].get<int>();
        if (d.contains("progresso") && d["progresso"].is_number()) progresso = d["progresso"].get<double>();
        if (d.contains("fracao") && d["fracao"].is_number()) resposta = d["fracao"].get<double>();
        const auto fracao = F::fracao_impressa(fim, camada, total, progresso, resposta);
        if (!fracao)
            return json{{"pergunta", "fracao"}}; // a tela pergunta "Quanto chegou a imprimir?"
        const auto r = db.finalizar_trabalho(id, fim, *fracao, std::nullopt, camada);
        avisar_telas();
        return json{{"tempo_real_s", r.tempo_real_s}, {"eventos", static_cast<int>(r.eventos.size())}};
    }
    if (acao == "produto.listar") {
        json l = json::array();
        for (const auto& p : db.produtos())
            l.push_back({{"id", p.id}, {"nome", p.nome}, {"concluidos", p.concluidos}, {"gramas", p.gramas}});
        return json{{"produtos", l}};
    }
    if (acao == "produto.unir") {
        db.unir_produtos(d.value("de", std::string()), d.value("para", std::string()));
        return json::object();
    }
    if (acao == "painel.resumo") {
        const int  dias = std::max(1, d.value("dias", 30));
        const auto w    = db.desperdicio(agora_menos_dias(dias), agora_menos_dias(-1));
        json       top  = json::array();
        for (const auto& p : db.produtos())
            if (p.concluidos > 0 && top.size() < 5)
                top.push_back({{"nome", p.nome}, {"concluidos", p.concluidos}, {"gramas", p.gramas}});
        return json{{"dias", dias},
                    {"desperdicio_g", w.desperdicio_g},
                    {"consumo_g", w.consumo_total_g},
                    {"taxa", w.taxa ? json(*w.taxa) : json()},
                    {"produtos", top},
                    {"pendentes_sync", db.pendentes_sincronizacao()}};
    }
    if (acao == "previsao.filamentos") {
        json l = json::array();
        for (const auto& t : db.tipos()) {
            const auto p = db.previsao_tipo(t.id, d.value("limite_dias", 7.));
            l.push_back({{"tipo", tipo_json(t)},
                         {"dados_suficientes", p.dados_suficientes},
                         {"sem_consumo", p.sem_consumo},
                         {"media_diaria_g", p.media_diaria_g},
                         {"dias_de_estoque", p.dias_de_estoque},
                         {"falta_g", p.falta_g},
                         {"alerta", p.alerta}});
        }
        return json{{"previsoes", l}};
    }
    if (acao == "manutencao.listar") {
        json l = json::array();
        for (const auto& imp : db.impressoras()) {
            json regras = json::array();
            for (const auto& r : db.manutencao(imp.id))
                regras.push_back({{"id", r.id},
                                  {"nome", r.nome},
                                  {"intervalo_h", r.intervalo_h},
                                  {"feito_ate_h", r.feito_ate_h},
                                  {"usado_h", r.estado.usado_h},
                                  {"faltam_h", r.estado.faltam_h},
                                  {"situacao", F::situacao_manutencao_texto(r.estado.situacao)}});
            l.push_back({{"id", imp.id}, {"nome", imp.nome}, {"horas", imp.horas}, {"regras", regras}});
        }
        return json{{"impressoras", l}};
    }
    if (acao == "manutencao.adicionar") {
        const std::string r = db.adicionar_regra_manutencao(d.value("impressora_id", std::string()), d.value("nome", std::string()),
                                                            d.value("intervalo_h", 100.), d.value("feito_ate_h", 0.));
        return json{{"id", r}};
    }
    if (acao == "manutencao.registrar") {
        db.registrar_manutencao_feita(id, d.value("obs", std::string()));
        return json::object();
    }
    if (acao == "arquivo.abrir" || acao == "arquivo.localizar") {
        const auto a = db.arquivo_do_produto(d.value("produto_id", std::string()));
        if (!a)
            throw std::runtime_error("Este produto não tem arquivo ligado.");
        std::string caminho = a->caminho;
        if (acao == "arquivo.localizar") {
            wxFileDialog dlg(wxGetApp().mainframe, _L("Locate file"), wxEmptyString, from_u8(a->nome),
                             "3MF, STL, OBJ, STEP (*.3mf;*.stl;*.obj;*.step;*.stp)|*.3mf;*.3MF;*.stl;*.STL;*.obj;*.OBJ;*.step;*.STEP;*.stp;*.STP",
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            if (dlg.ShowModal() != wxID_OK)
                return json{{"aberto", false}};
            caminho = into_u8(dlg.GetPath());
            const std::string sha = F::sha256_de_arquivo(caminho);
            if (!a->sha256.empty() && sha != a->sha256 &&
                wxMessageDialog(wxGetApp().mainframe, _L("This file is different from the original one. Use it anyway?"), _L("Locate file"),
                                wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION).ShowModal() != wxID_YES)
                return json{{"aberto", false}};
            db.relocalizar_arquivo(a->id, caminho);
            filuna_sync_avisar();
        } else if (a->pc_id != db.id_do_computador() || !boost::filesystem::exists(caminho)) {
            if (a->pc_id == db.id_do_computador())
                db.marcar_arquivo_ausente(a->id);
            return json{{"aberto", false}, {"situacao", a->pc_id == db.id_do_computador() ? "ausente" : "outro_pc"}};
        }
        wxGetApp().CallAfter([caminho] {
            Plater* plater = wxGetApp().plater();
            if (boost::filesystem::path(caminho).extension().string() == ".3mf")
                plater->load_project(from_u8(caminho));
            else
                plater->load_files(std::vector<boost::filesystem::path>{boost::filesystem::path(caminho)});
        });
        return json{{"aberto", true}};
    }
    if (acao == "versoes.listar") {
        // Identificação dos motores internos (FilunaVersoes.hpp): o suporte e a atualização por
        // pacotes usam estes ids e versões.
        json motores = json::array();
        for (const auto& m : F::motores_filuna())
            motores.push_back({{"id", m.id}, {"nome", m.nome}, {"versao", m.versao}});
        json recursos = json::object();
        for (const auto& [rid, rver] : F::Recursos::instalados((boost::filesystem::path(data_dir()) / "filuna" / "recursos").string()))
            recursos[rid] = rver;
        return json{{"produto", "Filuna Slicer"},
                    {"versao", F::VERSAO_FILUNA_SLICER},
                    {"base_orca", SoftFever_VERSION},
                    {"esquema_banco", db.versao_esquema()},
                    {"motores", motores},
                    {"recursos", recursos}};
    }
    if (acao == "sync.estado") {
        const FilunaSync::Estado e = FilunaSync::instancia().estado();
        return json{{"pendentes", db.pendentes_sincronizacao()}, {"ligado", e.ligado}, {"rev", e.rev},
                    {"ultimo_ok", e.ultimo_ok}, {"ultimo_erro", e.ultimo_erro}};
    }
    if (acao == "sync.agora") {
        filuna_sync_avisar();
        return json::object();
    }
    throw std::invalid_argument("ação de dados desconhecida: " + acao);
}

// --- Confirmar bobinas -------------------------------------------------------------------------------

namespace {

struct FilamentoUsado
{
    int         indice_filamento = 0; // índice do filamento no projeto (0 = primeiro)
    std::string preset;
    std::string material, cor_hex;
    double      gramas = 0., purga_g = 0.;
};

// Resumo da mesa atual a partir do fatiamento (mesmos números que o G-code exportado).
std::vector<FilamentoUsado> filamentos_do_fatiamento(std::optional<std::int64_t>& tempo_s)
{
    std::vector<FilamentoUsado> out;
    Plater*        plater = wxGetApp().plater();
    PresetBundle*  bundle = wxGetApp().preset_bundle;
    if (plater == nullptr || bundle == nullptr)
        return out;
    PartPlate* plate = plater->get_partplate_list().get_curr_plate();
    if (plate == nullptr || !plate->is_slice_result_valid() || plate->get_slice_result() == nullptr)
        return out;
    const auto& stats = plate->get_slice_result()->print_statistics;
    std::map<size_t, std::pair<double, double>> volumes; // filamento → (total mm³, torre mm³)
    for (const auto* m : {&stats.model_volumes_per_extruder, &stats.support_volumes_per_extruder})
        for (const auto& [id, v] : *m)
            volumes[id].first += v;
    for (const auto& [id, v] : stats.wipe_tower_volumes_per_extruder) {
        volumes[id].first += v;
        volumes[id].second += v;
    }
    for (const auto& [id, v] : volumes) {
        if (v.first <= 0.)
            continue;
        FilamentoUsado f;
        f.indice_filamento = static_cast<int>(id);
        double densidade   = 1.24;
        if (id < bundle->filament_presets.size()) {
            f.preset = bundle->filament_presets[id];
            if (const Preset* p = bundle->filaments.find_preset(f.preset)) {
                if (const auto* o = p->config.option<ConfigOptionFloats>("filament_density"); o && !o->values.empty() && o->values.front() > 0.)
                    densidade = o->values.front();
                if (const auto* o = p->config.option<ConfigOptionStrings>("filament_type"); o && !o->values.empty())
                    f.material = o->values.front();
            }
        }
        if (const auto* cores = bundle->project_config.option<ConfigOptionStrings>("filament_colour"); cores && id < cores->values.size())
            f.cor_hex = cores->values[id];
        f.gramas  = v.first * densidade / 1000.;
        f.purga_g = v.second * densidade / 1000.;
        out.push_back(f);
    }
    const float t = stats.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)].time;
    if (t > 0.f)
        tempo_s = static_cast<std::int64_t>(std::llround(t));
    return out;
}

std::string tipo_para(const F::FilunaDb& db, const FilamentoUsado& f)
{
    const auto tipos = db.tipos();
    for (const auto& t : tipos)
        if (!t.preset_orca.empty() && t.preset_orca == f.preset)
            return t.id;
    for (const auto& t : tipos)
        if (minusculas(t.material) == minusculas(f.material) && !f.cor_hex.empty() && minusculas(t.cor_hex) == minusculas(f.cor_hex))
            return t.id;
    for (const auto& t : tipos)
        if (minusculas(t.material) == minusculas(f.material))
            return t.id;
    return {};
}

class ConfirmarBobinasDialog : public wxDialog
{
public:
    ConfirmarBobinasDialog(wxWindow* parent, const std::vector<FilamentoUsado>& usados, const std::vector<F::FilunaDb::Bobina>& bobinas,
                           const std::map<std::string, std::string>& nomes, const std::vector<std::string>& sugestoes)
        : wxDialog(parent, wxID_ANY, _L("Confirm spools"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
        , m_bobinas(bobinas)
    {
        auto* raiz = new wxBoxSizer(wxVERTICAL);
        // TRN Dialog shown after exporting or sending a print, before the stock is updated.
        raiz->Add(new wxStaticText(this, wxID_ANY, _L("Which spool will each filament come from? The stock is updated when the print ends.")), 0,
                  wxALL, FromDIP(16));
        auto* grade = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
        grade->AddGrowableCol(1);
        wxArrayString opcoes;
        opcoes.Add(_L("Do not take from any spool"));
        for (const auto& b : bobinas) {
            const auto it = nomes.find(b.filamento_tipo_id);
            // TRN Spool option: %1% code, %2% material and colour, %3% grams left.
            opcoes.Add(format_wxstr(_L("%1% · %2% · %3% g left"), b.codigo, it != nomes.end() ? it->second : std::string(),
                                    static_cast<int>(std::lround(b.restante_g))));
        }
        for (std::size_t i = 0; i < usados.size(); ++i) {
            // TRN %1% filament number in the project, %2% filament preset, %3% grams for this print.
            grade->Add(new wxStaticText(this, wxID_ANY, format_wxstr(_L("Filament %1% (%2%) — %3% g"), usados[i].indice_filamento + 1, usados[i].preset,
                                                                     static_cast<int>(std::lround(usados[i].gramas)))),
                       0, wxALIGN_CENTER_VERTICAL);
            auto* escolha = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(360), -1), opcoes);
            int   sel     = 0;
            for (std::size_t k = 0; k < bobinas.size(); ++k)
                if (bobinas[k].id == sugestoes[i])
                    sel = static_cast<int>(k) + 1;
            escolha->SetSelection(sel);
            grade->Add(escolha, 1, wxEXPAND);
            m_escolhas.push_back(escolha);
        }
        raiz->Add(grade, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(16));
        auto* botoes = new wxBoxSizer(wxHORIZONTAL);
        botoes->AddStretchSpacer();
        botoes->Add(new wxButton(this, wxID_CANCEL, _L("Do not record this print")), 0, wxRIGHT, FromDIP(8));
        auto* ok = new wxButton(this, wxID_OK, _L("Confirm"));
        ok->SetDefault();
        botoes->Add(ok);
        raiz->Add(botoes, 0, wxEXPAND | wxALL, FromDIP(16));
        SetSizerAndFit(raiz);
        CentreOnParent();
        wxGetApp().UpdateDlgDarkUI(this);
    }

    // Bobina escolhida por filamento ('' = não descontar).
    std::vector<std::string> escolhidas() const
    {
        std::vector<std::string> out;
        for (const wxChoice* c : m_escolhas) {
            const int s = c->GetSelection();
            out.push_back(s > 0 ? m_bobinas[static_cast<std::size_t>(s - 1)].id : std::string());
        }
        return out;
    }

private:
    std::vector<F::FilunaDb::Bobina> m_bobinas;
    std::vector<wxChoice*>           m_escolhas;
};

} // namespace

void filuna_registrar_envio(wxWindow* parent)
{
    try {
        F::FilunaDb& db      = filuna_db();
        const auto   bobinas = db.bobinas();
        if (bobinas.empty())
            return; // estoque não usado: nada a perguntar
        std::optional<std::int64_t> tempo_s;
        const auto                  usados = filamentos_do_fatiamento(tempo_s);
        if (usados.empty())
            return;

        std::map<std::string, std::string> nomes;
        for (const auto& t : db.tipos())
            nomes[t.id] = t.material + " " + t.cor_nome + (t.marca.empty() ? std::string() : " (" + t.marca + ")");
        std::vector<std::string> sugestoes;
        for (const auto& f : usados) {
            const std::string tipo = tipo_para(db, f);
            const auto        s    = tipo.empty() ? std::nullopt : db.sugerir_bobina(tipo, std::string());
            sugestoes.push_back(s ? *s : std::string());
        }

        ConfirmarBobinasDialog dlg(parent, usados, bobinas, nomes, sugestoes);
        if (dlg.ShowModal() != wxID_OK)
            return;
        const auto escolhidas = dlg.escolhidas();

        Plater*           plater = wxGetApp().plater();
        const std::string assinatura = F::assinatura_do_modelo(plater->model());
        std::string       nome_produto = into_u8(plater->get_project_name());
        if (nome_produto.empty() && !plater->model().objects.empty())
            nome_produto = plater->model().objects.front()->name;
        const std::string produto = assinatura.empty() ? std::string() : db.produto_por_assinatura(assinatura, nome_produto);
        // Vínculo com o arquivo do projeto (ou do primeiro modelo): só caminho, nome, tamanho e SHA-256.
        std::string arquivo = into_u8(plater->get_project_filename(".3mf"));
        if ((arquivo.empty() || !boost::filesystem::exists(arquivo)) && !plater->model().objects.empty())
            arquivo = plater->model().objects.front()->input_file;
        if (!produto.empty() && !arquivo.empty() && boost::filesystem::exists(arquivo)) {
            F::FilunaDb::ArquivoLocal a;
            a.caminho       = arquivo;
            a.nome          = boost::filesystem::path(arquivo).filename().string();
            a.tamanho_bytes = static_cast<std::int64_t>(boost::filesystem::file_size(arquivo));
            a.sha256        = F::sha256_de_arquivo(arquivo);
            db.vincular_arquivo_produto(produto, db.registrar_arquivo(a));
        }

        std::string impressora_nome = wxGetApp().preset_bundle->physical_printers.get_selected_printer_name();
        if (impressora_nome.empty())
            impressora_nome = wxGetApp().preset_bundle->printers.get_edited_preset().name;
        const std::string impressora = db.impressora_por_nome(impressora_nome);

        F::GcodeResumo resumo;
        for (const auto& f : usados) {
            resumo.filamento_g.push_back(f.gramas);
            resumo.purga_g.push_back(f.purga_g);
        }
        resumo.tempo_previsto_s = tempo_s;
        const std::string trabalho = db.criar_trabalho(produto, impressora, resumo);
        for (std::size_t i = 0; i < escolhidas.size(); ++i)
            if (!escolhidas[i].empty())
                db.confirmar_bobina(trabalho, static_cast<int>(i), escolhidas[i]);
        db.marcar_estado(trabalho, "enviado");
        avisar_telas();
        filuna_sync_avisar();
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Filuna: registrar envio: " << e.what();
    }
}

}} // namespace Slic3r::GUI
