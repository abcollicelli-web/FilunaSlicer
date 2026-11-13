#include "FilunaDb.hpp"

#include <sqlite3.h>

#include <cstdio>
#include <map>
#include <ctime>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Slic3r::Filuna {

namespace {

// Migrações numeradas: o índice + 1 é o PRAGMA user_version depois de aplicada. Nunca edite uma
// migração já publicada; acrescente outra no fim.
const char* const MIGRACOES[] = {
    // 1 — esquema inicial (guia-v1/02-DADOS §2).
    R"SQL(
CREATE TABLE impressora(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    nome TEXT NOT NULL, fabricante TEXT, modelo TEXT, numero_serie TEXT, modulo_id TEXT,
    horas_impressao_s INTEGER NOT NULL DEFAULT 0);

CREATE TABLE filamento_tipo(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    marca TEXT, material TEXT, nome_produto TEXT, cor_nome TEXT, cor_hex TEXT,
    diametro_mm REAL NOT NULL DEFAULT 1.75, densidade_g_cm3 REAL, preset_orca TEXT);

CREATE TABLE bobina(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    filamento_tipo_id TEXT NOT NULL REFERENCES filamento_tipo(id),
    codigo TEXT UNIQUE,
    estado TEXT NOT NULL CHECK(estado IN ('lacrada','em_uso','vazia','descartada')),
    capacidade_g REAL NOT NULL, tara_g REAL NOT NULL DEFAULT 0, restante_g REAL NOT NULL,
    estimado INTEGER NOT NULL DEFAULT 0,
    preco_centavos INTEGER NOT NULL DEFAULT 0, comprada_em TEXT, aberta_em TEXT,
    local TEXT, ultimo_uso_em TEXT);

CREATE TABLE estoque_evento(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    bobina_id TEXT NOT NULL REFERENCES bobina(id),
    tipo TEXT NOT NULL CHECK(tipo IN ('entrada','abertura','consumo','purga','falha','pesagem','ajuste','descarte')),
    delta_g REAL NOT NULL, trabalho_id TEXT, estimado INTEGER NOT NULL DEFAULT 0, obs TEXT);
CREATE INDEX estoque_evento_bobina ON estoque_evento(bobina_id);

CREATE TABLE arquivo_local(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    pc_id TEXT, caminho TEXT, nome TEXT, tamanho_bytes INTEGER, sha256 TEXT, modificado_em TEXT,
    encontrado INTEGER NOT NULL DEFAULT 1);

CREATE TABLE produto(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    assinatura TEXT UNIQUE, nome TEXT, arquivo_local_id TEXT, miniatura BLOB, preco_venda_centavos INTEGER);

CREATE TABLE trabalho(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    produto_id TEXT REFERENCES produto(id), impressora_id TEXT REFERENCES impressora(id),
    gcode_sha256 TEXT, gcode_arquivo_local_id TEXT,
    estado TEXT NOT NULL CHECK(estado IN ('fatiado','enviado','imprimindo','concluido','cancelado','falhou')),
    copias INTEGER NOT NULL DEFAULT 1,
    tempo_previsto_s INTEGER, tempo_real_s INTEGER, camadas_total INTEGER, camada_parada INTEGER,
    trocas_filamento INTEGER, purga_total_g REAL, iniciado_em TEXT, terminado_em TEXT);

CREATE TABLE trabalho_filamento(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    trabalho_id TEXT NOT NULL REFERENCES trabalho(id), indice INTEGER NOT NULL,
    bobina_id TEXT REFERENCES bobina(id),
    previsto_g REAL, real_g REAL, purga_g REAL, estimado INTEGER NOT NULL DEFAULT 0);

CREATE TABLE manutencao_regra(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    impressora_id TEXT NOT NULL REFERENCES impressora(id), nome TEXT NOT NULL,
    intervalo_h REAL NOT NULL, feito_ate_h REAL NOT NULL DEFAULT 0, ativa INTEGER NOT NULL DEFAULT 1);

CREATE TABLE manutencao_registro(
    id TEXT PRIMARY KEY, criado_em TEXT NOT NULL, atualizado_em TEXT NOT NULL,
    excluido INTEGER NOT NULL DEFAULT 0, rev INTEGER NOT NULL DEFAULT 0,
    regra_id TEXT NOT NULL REFERENCES manutencao_regra(id), horas_maquina REAL, obs TEXT);

CREATE TABLE sync_fila(
    seq INTEGER PRIMARY KEY AUTOINCREMENT, tabela TEXT NOT NULL, registro_id TEXT NOT NULL,
    op TEXT NOT NULL, payload_json TEXT, enviado INTEGER NOT NULL DEFAULT 0);
)SQL",
    // 2 — sincronização com o servidor (03-SERVIDOR §4): identificação do computador e último rev recebido.
    R"SQL(
CREATE TABLE config_local(chave TEXT PRIMARY KEY, valor TEXT);
CREATE INDEX sync_fila_pendente ON sync_fila(enviado, seq);
)SQL",
};

[[noreturn]] void falhar(sqlite3* db, const std::string& onde)
{
    throw std::runtime_error("FilunaDb: " + onde + ": " + (db ? sqlite3_errmsg(db) : "sem banco"));
}

// Comando preparado com liberação automática.
class Comando
{
public:
    Comando(sqlite3* db, const std::string& sql) : m_db(db)
    {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &m_st, nullptr) != SQLITE_OK)
            falhar(db, sql);
    }
    ~Comando() { sqlite3_finalize(m_st); }
    Comando(const Comando&)            = delete;
    Comando& operator=(const Comando&) = delete;

    Comando& texto(int i, const std::string& v)
    {
        sqlite3_bind_text(m_st, i, v.c_str(), static_cast<int>(v.size()), SQLITE_TRANSIENT);
        return *this;
    }
    // Texto vazio grava NULL.
    Comando& texto_ou_nulo(int i, const std::string& v) { return v.empty() ? nulo(i) : texto(i, v); }
    Comando& real(int i, double v)
    {
        sqlite3_bind_double(m_st, i, v);
        return *this;
    }
    Comando& inteiro(int i, std::int64_t v)
    {
        sqlite3_bind_int64(m_st, i, v);
        return *this;
    }
    template<class T> Comando& opcional(int i, const std::optional<T>& v)
    {
        if (!v)
            return nulo(i);
        if constexpr (std::is_floating_point_v<T>)
            return real(i, *v);
        else
            return inteiro(i, static_cast<std::int64_t>(*v));
    }
    Comando& nulo(int i)
    {
        sqlite3_bind_null(m_st, i);
        return *this;
    }
    // true enquanto houver linha.
    bool passo()
    {
        const int rc = sqlite3_step(m_st);
        if (rc == SQLITE_ROW)
            return true;
        if (rc != SQLITE_DONE)
            falhar(m_db, sqlite3_sql(m_st));
        return false;
    }
    void executar() { while (passo()) {} }

    bool        nulo_em(int c) const { return sqlite3_column_type(m_st, c) == SQLITE_NULL; }
    std::string texto_em(int c) const
    {
        const auto* p = sqlite3_column_text(m_st, c);
        return p ? std::string(reinterpret_cast<const char*>(p), static_cast<std::size_t>(sqlite3_column_bytes(m_st, c))) : std::string();
    }
    double       real_em(int c) const { return sqlite3_column_double(m_st, c); }
    std::int64_t inteiro_em(int c) const { return sqlite3_column_int64(m_st, c); }

private:
    sqlite3*      m_db = nullptr;
    sqlite3_stmt* m_st = nullptr;
};

} // namespace

// Transação que só abre se ainda não houver uma (métodos públicos podem se chamar).
class FilunaDb::Transacao
{
public:
    explicit Transacao(sqlite3* db) : m_db(db), m_dona(sqlite3_get_autocommit(db) != 0)
    {
        if (m_dona && sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK)
            falhar(db, "BEGIN");
    }
    ~Transacao()
    {
        if (m_dona && !m_confirmada)
            sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, nullptr);
    }
    void confirmar()
    {
        if (m_dona && sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK)
            falhar(m_db, "COMMIT");
        m_confirmada = true;
    }

private:
    sqlite3* m_db;
    bool     m_dona;
    bool     m_confirmada = false;
};

// ---------------------------------------------------------------------------------------------

std::unique_ptr<FilunaDb> FilunaDb::abrir(const std::string& caminho_utf8, Relogio relogio)
{
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(caminho_utf8.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
        const std::string msg = db ? sqlite3_errmsg(db) : "sem memória";
        sqlite3_close(db);
        throw std::runtime_error("FilunaDb: não abriu " + caminho_utf8 + ": " + msg);
    }
    std::unique_ptr<FilunaDb> out(new FilunaDb(db, std::move(relogio)));
    out->executar("PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON; PRAGMA busy_timeout=5000;");
    out->migrar();
    return out;
}

FilunaDb::FilunaDb(sqlite3* db, Relogio relogio) : m_db(db), m_relogio(std::move(relogio)) {}

FilunaDb::~FilunaDb() { sqlite3_close(m_db); }

int FilunaDb::versao_esquema_atual() { return static_cast<int>(sizeof(MIGRACOES) / sizeof(MIGRACOES[0])); }

int FilunaDb::versao_esquema() const
{
    Comando c(m_db, "PRAGMA user_version");
    return c.passo() ? static_cast<int>(c.inteiro_em(0)) : 0;
}

void FilunaDb::executar(const std::string& sql) const
{
    char* erro = nullptr;
    if (sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &erro) != SQLITE_OK) {
        const std::string msg = erro ? erro : "erro";
        sqlite3_free(erro);
        throw std::runtime_error("FilunaDb: " + msg);
    }
}

void FilunaDb::migrar()
{
    const int de = versao_esquema();
    if (de > versao_esquema_atual())
        throw std::runtime_error("FilunaDb: banco de uma versão mais nova do Filuna (esquema " + std::to_string(de) + ")");
    for (int v = de; v < versao_esquema_atual(); ++v) {
        Transacao t(m_db);
        executar(MIGRACOES[v]);
        executar("PRAGMA user_version=" + std::to_string(v + 1));
        t.confirmar();
    }
}

std::string FilunaDb::novo_uuid()
{
    thread_local std::mt19937_64 gerador{std::random_device{}()};
    std::uniform_int_distribution<std::uint64_t> dist;
    std::uint64_t a = dist(gerador), b = dist(gerador);
    a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL; // versão 4
    b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL; // variante RFC 4122
    char buf[37];
    std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx", static_cast<unsigned>(a >> 32), static_cast<unsigned>((a >> 16) & 0xFFFF),
                  static_cast<unsigned>(a & 0xFFFF), static_cast<unsigned>(b >> 48), static_cast<unsigned long long>(b & 0xFFFFFFFFFFFFULL));
    return buf;
}

std::string FilunaDb::agora_utc()
{
    const std::time_t t = std::time(nullptr);
    std::tm           tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

std::string FilunaDb::agora() const { return m_relogio ? m_relogio() : agora_utc(); }

void FilunaDb::enfileirar(const std::string& tabela, const std::string& id, const std::string& op)
{
    // Cópia da linha em JSON (sem colunas BLOB, que não viajam no JSON da sincronização).
    std::string campos;
    {
        Comando info(m_db, "PRAGMA table_info(" + tabela + ")");
        while (info.passo()) {
            if (info.texto_em(2) == "BLOB")
                continue;
            const std::string col = info.texto_em(1);
            campos += (campos.empty() ? "" : ",") + ("'" + col + "'," + col);
        }
    }
    Comando c(m_db, "INSERT INTO sync_fila(tabela, registro_id, op, payload_json) SELECT ?1, ?2, ?3, json_object(" + campos +
                        ") FROM " + tabela + " WHERE id = ?2");
    c.texto(1, tabela).texto(2, id).texto(3, op).executar();
}

int FilunaDb::pendentes_sincronizacao() const
{
    Comando c(m_db, "SELECT COUNT(*) FROM sync_fila WHERE enviado = 0");
    return c.passo() ? static_cast<int>(c.inteiro_em(0)) : 0;
}

std::vector<FilunaDb::ItemFila> FilunaDb::fila_para_enviar(int limite) const
{
    std::vector<ItemFila> l;
    Comando c(m_db, "SELECT seq, tabela, registro_id, COALESCE(payload_json, 'null') FROM sync_fila WHERE enviado = 0 ORDER BY seq LIMIT ?1");
    c.inteiro(1, limite);
    while (c.passo())
        l.push_back({c.inteiro_em(0), c.texto_em(1), c.texto_em(2), c.texto_em(3)});
    return l;
}

void FilunaDb::marcar_enviados(const std::vector<std::int64_t>& seqs)
{
    Transacao t(m_db);
    for (std::int64_t s : seqs)
        Comando(m_db, "UPDATE sync_fila SET enviado = 1 WHERE seq = ?1").inteiro(1, s).executar();
    // Guarda só os últimos 2000 itens já enviados: a fila não cresce para sempre.
    executar("DELETE FROM sync_fila WHERE enviado = 1 AND seq < (SELECT COALESCE(MAX(seq), 0) - 2000 FROM sync_fila)");
    t.confirmar();
}

std::string FilunaDb::config_local(const std::string& chave) const
{
    Comando c(m_db, "SELECT valor FROM config_local WHERE chave = ?1");
    c.texto(1, chave);
    return c.passo() ? c.texto_em(0) : std::string();
}

void FilunaDb::gravar_config_local(const std::string& chave, const std::string& valor)
{
    Comando(m_db, "INSERT INTO config_local(chave, valor) VALUES(?1, ?2) ON CONFLICT(chave) DO UPDATE SET valor = excluded.valor")
        .texto(1, chave).texto(2, valor).executar();
}

std::string FilunaDb::id_do_computador()
{
    std::string id = config_local("pc_id");
    if (id.empty()) {
        id = novo_uuid();
        gravar_config_local("pc_id", id);
    }
    return id;
}

int FilunaDb::aplicar_remotos(const std::vector<ItemRemoto>& itens)
{
    // Tabelas que vêm do servidor e, entre elas, as que só recebem inserção (nunca conflitam).
    static const char* const PERMITIDAS[] = {"impressora", "filamento_tipo", "bobina", "estoque_evento", "arquivo_local", "produto",
                                             "trabalho", "trabalho_filamento", "manutencao_regra", "manutencao_registro"};
    static const char* const SO_INSERE[]  = {"estoque_evento", "trabalho_filamento", "manutencao_registro"};
    auto contem = [](const auto& lista, const std::string& t) {
        return std::any_of(std::begin(lista), std::end(lista), [&t](const char* x) { return t == x; });
    };
    // Os registros podem chegar antes dos que eles apontam (outro computador, outra ordem): sem checar
    // chaves estrangeiras enquanto aplica. Só nesta conexão; volta a ligar no fim.
    executar("PRAGMA foreign_keys=OFF");
    int                      aplicados = 0;
    std::vector<std::string> bobinas;
    try {
        Transacao t(m_db);
        std::map<std::string, std::vector<std::string>> colunas;
        for (const ItemRemoto& it : itens) {
            if (!contem(PERMITIDAS, it.tabela) || it.id.empty())
                continue;
            auto& cols = colunas[it.tabela];
            if (cols.empty()) {
                Comando info(m_db, "PRAGMA table_info(" + it.tabela + ")");
                while (info.passo())
                    if (info.texto_em(2) != "BLOB")
                        cols.push_back(info.texto_em(1));
            }
            std::string dados = it.dados_json;
            {
                Comando c(m_db, "SELECT json_valid(?1) AND json_extract(?1, '$.id') = ?2");
                c.texto(1, dados).texto(2, it.id);
                if (!c.passo() || !c.inteiro_em(0))
                    continue;
            }
            // Códigos e assinaturas são únicos no banco; o mesmo código vindo de outro computador ganha um sufixo
            // (bobina) ou fica sem assinatura (produto, que pode ser unido depois).
            if (it.tabela == "bobina" || it.tabela == "produto") {
                const std::string campo = it.tabela == "bobina" ? "codigo" : "assinatura";
                Comando c(m_db, "SELECT 1 FROM " + it.tabela + " WHERE " + campo + " = json_extract(?1, '$." + campo + "') AND id <> ?2");
                c.texto(1, dados).texto(2, it.id);
                if (c.passo()) {
                    Comando j(m_db, it.tabela == "bobina" ? "SELECT json_set(?1, '$.codigo', json_extract(?1, '$.codigo') || '-' || substr(?2, 1, 4))"
                                                          : "SELECT json_set(?1, '$.assinatura', NULL)");
                    j.texto(1, dados).texto(2, it.id);
                    if (j.passo())
                        dados = j.texto_em(0);
                }
            }
            // Só as colunas que vieram no JSON (as que faltam ficam como estão; a miniatura nunca viaja).
            std::vector<std::string> presentes;
            for (const std::string& col : cols) {
                Comando c(m_db, "SELECT json_type(?1, '$.' || ?2) IS NOT NULL");
                c.texto(1, dados).texto(2, col);
                if (c.passo() && c.inteiro_em(0))
                    presentes.push_back(col);
            }
            std::string nomes, valores, atualiza;
            for (std::size_t k = 0; k < presentes.size(); ++k) {
                const std::string& col = presentes[k];
                nomes += (k ? "," : "") + col;
                valores += std::string(k ? "," : "") + "json_extract(?1, '$." + col + "')";
                if (col != "id")
                    atualiza += (atualiza.empty() ? "" : ",") + col + "=excluded." + col;
            }
            // "WHERE 1" separa o SELECT do ON CONFLICT (ambiguidade documentada do SQLite).
            std::string sql = "INSERT INTO " + it.tabela + "(" + nomes + ") SELECT " + valores + " WHERE 1";
            if (contem(SO_INSERE, it.tabela) || atualiza.empty())
                sql += " ON CONFLICT(id) DO NOTHING";
            else // vence o mais novo (03-SERVIDOR §4)
                sql += " ON CONFLICT(id) DO UPDATE SET " + atualiza + " WHERE excluded.atualizado_em >= " + it.tabela + ".atualizado_em";
            try {
                Comando(m_db, sql).texto(1, dados).executar();
            } catch (const std::runtime_error&) {
                continue; // linha incompleta ou inválida: só ela fica de fora (o erro desfaz apenas este comando)
            }
            if (sqlite3_changes(m_db) > 0) {
                ++aplicados;
                if (it.tabela == "bobina")
                    bobinas.push_back(it.id);
                else if (it.tabela == "estoque_evento") {
                    Comando c(m_db, "SELECT json_extract(?1, '$.bobina_id')");
                    c.texto(1, dados);
                    if (c.passo())
                        bobinas.push_back(c.texto_em(0));
                }
            }
        }
        // O restante da bobina é a soma dos eventos (02-DADOS §5), sem voltar para a fila.
        std::sort(bobinas.begin(), bobinas.end());
        bobinas.erase(std::unique(bobinas.begin(), bobinas.end()), bobinas.end());
        for (const std::string& b : bobinas)
            Comando(m_db, "UPDATE bobina SET restante_g = max(0, (SELECT COALESCE(SUM(delta_g), 0) FROM estoque_evento "
                          "WHERE bobina_id = ?1 AND excluido = 0)), "
                          "estado = CASE WHEN estado IN ('descartada', 'lacrada') THEN estado "
                          "              WHEN (SELECT COALESCE(SUM(delta_g), 0) FROM estoque_evento WHERE bobina_id = ?1 AND excluido = 0) <= 0 "
                          "                   THEN 'vazia' ELSE 'em_uso' END WHERE id = ?1")
                .texto(1, b).executar();
        t.confirmar();
    } catch (...) {
        executar("PRAGMA foreign_keys=ON");
        throw;
    }
    executar("PRAGMA foreign_keys=ON");
    return aplicados;
}

// --- Impressoras e manutenção -----------------------------------------------------------------

std::string FilunaDb::criar_impressora(const std::string& nome, const std::string& fabricante, const std::string& modelo,
                                       const std::string& numero_serie, const std::string& modulo_id)
{
    Transacao t(m_db);
    const std::string id = novo_uuid(), ts = agora();
    Comando(m_db, "INSERT INTO impressora(id, criado_em, atualizado_em, nome, fabricante, modelo, numero_serie, modulo_id) "
                  "VALUES(?1, ?2, ?2, ?3, ?4, ?5, ?6, ?7)")
        .texto(1, id).texto(2, ts).texto(3, nome).texto_ou_nulo(4, fabricante).texto_ou_nulo(5, modelo)
        .texto_ou_nulo(6, numero_serie).texto_ou_nulo(7, modulo_id).executar();
    enfileirar("impressora", id, "upsert");
    t.confirmar();
    return id;
}

std::string FilunaDb::adicionar_regra_manutencao(const std::string& impressora_id, const std::string& nome, double intervalo_h,
                                                 double feito_ate_h)
{
    Transacao t(m_db);
    const std::string id = novo_uuid(), ts = agora();
    Comando(m_db, "INSERT INTO manutencao_regra(id, criado_em, atualizado_em, impressora_id, nome, intervalo_h, feito_ate_h) "
                  "VALUES(?1, ?2, ?2, ?3, ?4, ?5, ?6)")
        .texto(1, id).texto(2, ts).texto(3, impressora_id).texto(4, nome).real(5, intervalo_h).real(6, feito_ate_h).executar();
    enfileirar("manutencao_regra", id, "upsert");
    t.confirmar();
    return id;
}

double FilunaDb::horas_maquina(const std::string& impressora_id) const
{
    Comando c(m_db, "SELECT horas_impressao_s FROM impressora WHERE id = ?1");
    c.texto(1, impressora_id);
    return c.passo() ? static_cast<double>(c.inteiro_em(0)) / 3600. : 0.;
}

std::vector<FilunaDb::RegraManutencao> FilunaDb::manutencao(const std::string& impressora_id) const
{
    const double horas = horas_maquina(impressora_id);
    std::vector<RegraManutencao> out;
    Comando c(m_db, "SELECT id, nome, intervalo_h, feito_ate_h FROM manutencao_regra "
                    "WHERE impressora_id = ?1 AND ativa = 1 AND excluido = 0 ORDER BY nome");
    c.texto(1, impressora_id);
    while (c.passo()) {
        RegraManutencao r;
        r.id          = c.texto_em(0);
        r.nome        = c.texto_em(1);
        r.intervalo_h = c.real_em(2);
        r.feito_ate_h = c.real_em(3);
        r.estado      = estado_manutencao(horas, r.intervalo_h, r.feito_ate_h);
        out.push_back(std::move(r));
    }
    return out;
}

void FilunaDb::registrar_manutencao_feita(const std::string& regra_id, const std::string& obs)
{
    Transacao t(m_db);
    double horas = 0.;
    {
        Comando c(m_db, "SELECT i.horas_impressao_s FROM manutencao_regra r JOIN impressora i ON i.id = r.impressora_id WHERE r.id = ?1");
        c.texto(1, regra_id);
        if (!c.passo())
            throw std::runtime_error("FilunaDb: regra de manutenção não encontrada");
        horas = static_cast<double>(c.inteiro_em(0)) / 3600.;
    }
    const std::string id = novo_uuid(), ts = agora();
    Comando(m_db, "INSERT INTO manutencao_registro(id, criado_em, atualizado_em, regra_id, horas_maquina, obs) VALUES(?1, ?2, ?2, ?3, ?4, ?5)")
        .texto(1, id).texto(2, ts).texto(3, regra_id).real(4, horas).texto_ou_nulo(5, obs).executar();
    Comando(m_db, "UPDATE manutencao_regra SET feito_ate_h = ?2, atualizado_em = ?3 WHERE id = ?1")
        .texto(1, regra_id).real(2, horas).texto(3, ts).executar();
    enfileirar("manutencao_registro", id, "upsert");
    enfileirar("manutencao_regra", regra_id, "upsert");
    t.confirmar();
}

// --- Filamentos e bobinas -----------------------------------------------------------------------

std::string FilunaDb::criar_filamento_tipo(const FilamentoTipo& f)
{
    Transacao t(m_db);
    const std::string id = novo_uuid(), ts = agora();
    Comando c(m_db, "INSERT INTO filamento_tipo(id, criado_em, atualizado_em, marca, material, nome_produto, cor_nome, cor_hex, "
                    "diametro_mm, densidade_g_cm3, preset_orca) VALUES(?1, ?2, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10)");
    c.texto(1, id).texto(2, ts).texto_ou_nulo(3, f.marca).texto_ou_nulo(4, f.material).texto_ou_nulo(5, f.nome_produto)
        .texto_ou_nulo(6, f.cor_nome).texto_ou_nulo(7, f.cor_hex).real(8, f.diametro_mm).texto_ou_nulo(10, f.preset_orca);
    if (f.densidade_g_cm3 > 0.)
        c.real(9, f.densidade_g_cm3);
    else
        c.nulo(9);
    c.executar();
    enfileirar("filamento_tipo", id, "upsert");
    t.confirmar();
    return id;
}

std::string FilunaDb::proximo_codigo_bobina() const
{
    Comando c(m_db, "SELECT COALESCE(MAX(CAST(substr(codigo, 3) AS INTEGER)), 0) FROM bobina WHERE codigo LIKE 'B-%'");
    const std::int64_t n = c.passo() ? c.inteiro_em(0) + 1 : 1;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "B-%04lld", static_cast<long long>(n));
    return buf;
}

void FilunaDb::inserir_evento(const std::string& bobina_id, const std::string& tipo, double delta_g, const std::string& trabalho_id,
                              bool estimado, const std::string& obs)
{
    const std::string id = novo_uuid(), ts = agora();
    Comando(m_db, "INSERT INTO estoque_evento(id, criado_em, atualizado_em, bobina_id, tipo, delta_g, trabalho_id, estimado, obs) "
                  "VALUES(?1, ?2, ?2, ?3, ?4, ?5, ?6, ?7, ?8)")
        .texto(1, id).texto(2, ts).texto(3, bobina_id).texto(4, tipo).real(5, delta_g).texto_ou_nulo(6, trabalho_id)
        .inteiro(7, estimado ? 1 : 0).texto_ou_nulo(8, obs).executar();
    enfileirar("estoque_evento", id, "insert");
}

void FilunaDb::recalcular_bobina(const std::string& bobina_id)
{
    auto somar = [&] {
        std::vector<double> deltas;
        Comando c(m_db, "SELECT delta_g FROM estoque_evento WHERE bobina_id = ?1 AND excluido = 0");
        c.texto(1, bobina_id);
        while (c.passo())
            deltas.push_back(c.real_em(0));
        return restante_bobina(deltas);
    };
    RestanteBobina r = somar();
    if (r.divergencia) {
        inserir_evento(bobina_id, "ajuste", r.ajuste_g, {}, false, "divergência: consumo maior que o estoque");
        r = somar();
    }
    const std::string ts = agora();
    Comando(m_db, "UPDATE bobina SET restante_g = ?2, atualizado_em = ?3, "
                  "estado = CASE WHEN ?2 <= 0 AND estado <> 'descartada' THEN 'vazia' "
                  "              WHEN ?2 > 0 AND estado = 'vazia' THEN 'em_uso' ELSE estado END "
                  "WHERE id = ?1")
        .texto(1, bobina_id).real(2, r.restante_g).texto(3, ts).executar();
    enfileirar("bobina", bobina_id, "upsert");
}

std::vector<std::string> FilunaDb::cadastrar_bobinas(const std::string& filamento_tipo_id, int quantidade, double capacidade_g,
                                                     double tara_g, std::int64_t preco_centavos, const std::string& comprada_em)
{
    if (quantidade <= 0 || capacidade_g <= 0.)
        throw std::invalid_argument("FilunaDb: quantidade e capacidade precisam ser positivas");
    Transacao t(m_db);
    std::vector<std::string> ids;
    for (int i = 0; i < quantidade; ++i) {
        const std::string id = novo_uuid(), ts = agora();
        Comando(m_db, "INSERT INTO bobina(id, criado_em, atualizado_em, filamento_tipo_id, codigo, estado, capacidade_g, tara_g, "
                      "restante_g, preco_centavos, comprada_em) VALUES(?1, ?2, ?2, ?3, ?4, 'lacrada', ?5, ?6, 0, ?7, ?8)")
            .texto(1, id).texto(2, ts).texto(3, filamento_tipo_id).texto(4, proximo_codigo_bobina()).real(5, capacidade_g)
            .real(6, tara_g).inteiro(7, preco_centavos).texto_ou_nulo(8, comprada_em).executar();
        inserir_evento(id, "entrada", capacidade_g, {}, false, {});
        recalcular_bobina(id);
        ids.push_back(id);
    }
    t.confirmar();
    return ids;
}

std::string FilunaDb::cadastrar_bobina_usada(const std::string& filamento_tipo_id, double capacidade_g, double tara_g, double restante_g,
                                             bool estimado, std::int64_t preco_centavos)
{
    Transacao t(m_db);
    const std::string id = novo_uuid(), ts = agora();
    Comando(m_db, "INSERT INTO bobina(id, criado_em, atualizado_em, filamento_tipo_id, codigo, estado, capacidade_g, tara_g, "
                  "restante_g, estimado, preco_centavos, aberta_em) VALUES(?1, ?2, ?2, ?3, ?4, 'em_uso', ?5, ?6, 0, ?7, ?8, ?2)")
        .texto(1, id).texto(2, ts).texto(3, filamento_tipo_id).texto(4, proximo_codigo_bobina()).real(5, capacidade_g).real(6, tara_g)
        .inteiro(7, estimado ? 1 : 0).inteiro(8, preco_centavos).executar();
    inserir_evento(id, "entrada", restante_g, {}, estimado, {});
    recalcular_bobina(id);
    t.confirmar();
    return id;
}

void FilunaDb::abrir_bobina(const std::string& bobina_id)
{
    Transacao t(m_db);
    const std::string ts = agora();
    Comando(m_db, "UPDATE bobina SET estado = 'em_uso', aberta_em = ?2, atualizado_em = ?2 WHERE id = ?1 AND estado = 'lacrada'")
        .texto(1, bobina_id).texto(2, ts).executar();
    if (sqlite3_changes(m_db) > 0) {
        inserir_evento(bobina_id, "abertura", 0., {}, false, {});
        enfileirar("bobina", bobina_id, "upsert");
    }
    t.confirmar();
}

void FilunaDb::definir_local(const std::string& bobina_id, const std::string& local)
{
    Transacao t(m_db);
    Comando(m_db, "UPDATE bobina SET local = ?2, atualizado_em = ?3 WHERE id = ?1").texto(1, bobina_id).texto_ou_nulo(2, local).texto(3, agora()).executar();
    enfileirar("bobina", bobina_id, "upsert");
    t.confirmar();
}

void FilunaDb::pesar_bobina(const std::string& bobina_id, double peso_balanca_g)
{
    Transacao t(m_db);
    const auto b = bobina(bobina_id);
    if (!b)
        throw std::runtime_error("FilunaDb: bobina não encontrada");
    inserir_evento(bobina_id, "pesagem", delta_pesagem(peso_balanca_g, b->tara_g, b->restante_g), {}, false, {});
    Comando(m_db, "UPDATE bobina SET estimado = 0 WHERE id = ?1").texto(1, bobina_id).executar();
    recalcular_bobina(bobina_id);
    t.confirmar();
}

void FilunaDb::registrar_evento(const std::string& bobina_id, const std::string& tipo, double delta_g, const std::string& trabalho_id,
                                bool estimado, const std::string& obs)
{
    Transacao t(m_db);
    inserir_evento(bobina_id, tipo, delta_g, trabalho_id, estimado, obs);
    recalcular_bobina(bobina_id);
    t.confirmar();
}

namespace {
const char* const COLUNAS_BOBINA = "id, filamento_tipo_id, COALESCE(codigo,''), estado, COALESCE(local,''), COALESCE(ultimo_uso_em,''), "
                                   "COALESCE(comprada_em,''), capacidade_g, tara_g, restante_g, preco_centavos, estimado";
FilunaDb::Bobina ler_bobina(const Comando& c)
{
    FilunaDb::Bobina b;
    b.id                = c.texto_em(0);
    b.filamento_tipo_id = c.texto_em(1);
    b.codigo            = c.texto_em(2);
    b.estado            = c.texto_em(3);
    b.local             = c.texto_em(4);
    b.ultimo_uso_em     = c.texto_em(5);
    b.comprada_em       = c.texto_em(6);
    b.capacidade_g      = c.real_em(7);
    b.tara_g            = c.real_em(8);
    b.restante_g        = c.real_em(9);
    b.preco_centavos    = c.inteiro_em(10);
    b.estimado          = c.inteiro_em(11) != 0;
    return b;
}
} // namespace

std::optional<FilunaDb::Bobina> FilunaDb::bobina(const std::string& bobina_id) const
{
    Comando c(m_db, std::string("SELECT ") + COLUNAS_BOBINA + " FROM bobina WHERE id = ?1 AND excluido = 0");
    c.texto(1, bobina_id);
    if (!c.passo())
        return std::nullopt;
    return ler_bobina(c);
}

std::vector<FilunaDb::Bobina> FilunaDb::bobinas_do_tipo(const std::string& filamento_tipo_id) const
{
    std::vector<Bobina> out;
    Comando c(m_db, std::string("SELECT ") + COLUNAS_BOBINA + " FROM bobina WHERE filamento_tipo_id = ?1 AND excluido = 0 ORDER BY codigo");
    c.texto(1, filamento_tipo_id);
    while (c.passo())
        out.push_back(ler_bobina(c));
    return out;
}

std::optional<std::string> FilunaDb::sugerir_bobina(const std::string& filamento_tipo_id, const std::string& local_do_slot) const
{
    std::vector<BobinaCandidata> candidatas;
    Comando c(m_db, "SELECT id, filamento_tipo_id, estado, COALESCE(local,''), COALESCE(ultimo_uso_em,''), COALESCE(comprada_em,'') "
                    "FROM bobina WHERE excluido = 0 AND estado IN ('lacrada','em_uso') AND (filamento_tipo_id = ?1 OR local = ?2)");
    c.texto(1, filamento_tipo_id).texto(2, local_do_slot);
    while (c.passo())
        candidatas.push_back({c.texto_em(0), c.texto_em(1), c.texto_em(2), c.texto_em(3), c.texto_em(4), c.texto_em(5)});
    return Slic3r::Filuna::sugerir_bobina(candidatas, filamento_tipo_id, local_do_slot);
}

// --- Produtos e trabalhos -------------------------------------------------------------------------

std::string FilunaDb::produto_por_assinatura(const std::string& assinatura, const std::string& nome)
{
    Transacao t(m_db);
    {
        Comando c(m_db, "SELECT id FROM produto WHERE assinatura = ?1 AND excluido = 0");
        c.texto(1, assinatura);
        if (c.passo())
            return c.texto_em(0);
    }
    const std::string id = novo_uuid(), ts = agora();
    Comando(m_db, "INSERT INTO produto(id, criado_em, atualizado_em, assinatura, nome) VALUES(?1, ?2, ?2, ?3, ?4)")
        .texto(1, id).texto(2, ts).texto(3, assinatura).texto_ou_nulo(4, nome).executar();
    enfileirar("produto", id, "upsert");
    t.confirmar();
    return id;
}

// --- Vínculo de arquivos ---------------------------------------------------------------------

std::string FilunaDb::registrar_arquivo(const ArquivoLocal& a)
{
    Transacao t(m_db);
    const std::string pc = id_do_computador(), ts = agora();
    {
        Comando c(m_db, "SELECT id FROM arquivo_local WHERE pc_id = ?1 AND caminho = ?2 AND COALESCE(sha256, '') = ?3 AND excluido = 0");
        c.texto(1, pc).texto(2, a.caminho).texto(3, a.sha256);
        if (c.passo()) {
            const std::string id = c.texto_em(0);
            Comando(m_db, "UPDATE arquivo_local SET encontrado = 1, atualizado_em = ?2 WHERE id = ?1").texto(1, id).texto(2, ts).executar();
            enfileirar("arquivo_local", id, "upsert");
            t.confirmar();
            return id;
        }
    }
    const std::string id = novo_uuid();
    Comando(m_db, "INSERT INTO arquivo_local(id, criado_em, atualizado_em, pc_id, caminho, nome, tamanho_bytes, sha256, modificado_em, encontrado) "
                  "VALUES(?1, ?2, ?2, ?3, ?4, ?5, ?6, ?7, ?8, 1)")
        .texto(1, id).texto(2, ts).texto(3, pc).texto(4, a.caminho).texto(5, a.nome).inteiro(6, a.tamanho_bytes)
        .texto_ou_nulo(7, a.sha256).texto_ou_nulo(8, a.modificado_em).executar();
    enfileirar("arquivo_local", id, "upsert");
    t.confirmar();
    return id;
}

namespace {
const char* const COLUNAS_ARQUIVO = "a.id, COALESCE(a.pc_id, ''), COALESCE(a.caminho, ''), COALESCE(a.nome, ''), COALESCE(a.sha256, ''), "
                                    "COALESCE(a.modificado_em, ''), COALESCE(a.tamanho_bytes, 0), a.encontrado";
}

std::optional<FilunaDb::ArquivoLocal> FilunaDb::arquivo(const std::string& id) const
{
    Comando c(m_db, std::string("SELECT ") + COLUNAS_ARQUIVO + " FROM arquivo_local a WHERE a.id = ?1 AND a.excluido = 0");
    c.texto(1, id);
    if (!c.passo())
        return std::nullopt;
    return ArquivoLocal{c.texto_em(0), c.texto_em(1), c.texto_em(2), c.texto_em(3), c.texto_em(4), c.texto_em(5), c.inteiro_em(6), c.inteiro_em(7) != 0};
}

std::optional<FilunaDb::ArquivoLocal> FilunaDb::arquivo_do_produto(const std::string& produto_id) const
{
    Comando c(m_db, std::string("SELECT ") + COLUNAS_ARQUIVO +
                        " FROM produto p JOIN arquivo_local a ON a.id = p.arquivo_local_id AND a.excluido = 0 WHERE p.id = ?1");
    c.texto(1, produto_id);
    if (!c.passo())
        return std::nullopt;
    return ArquivoLocal{c.texto_em(0), c.texto_em(1), c.texto_em(2), c.texto_em(3), c.texto_em(4), c.texto_em(5), c.inteiro_em(6), c.inteiro_em(7) != 0};
}

void FilunaDb::vincular_arquivo_produto(const std::string& produto_id, const std::string& arquivo_id)
{
    Transacao t(m_db);
    Comando(m_db, "UPDATE produto SET arquivo_local_id = ?2, atualizado_em = ?3 WHERE id = ?1 AND COALESCE(arquivo_local_id, '') <> ?2")
        .texto(1, produto_id).texto(2, arquivo_id).texto(3, agora()).executar();
    if (sqlite3_changes(m_db) > 0)
        enfileirar("produto", produto_id, "upsert");
    t.confirmar();
}

void FilunaDb::relocalizar_arquivo(const std::string& id, const std::string& caminho)
{
    Transacao t(m_db);
    Comando(m_db, "UPDATE arquivo_local SET caminho = ?2, pc_id = ?3, encontrado = 1, atualizado_em = ?4 WHERE id = ?1")
        .texto(1, id).texto(2, caminho).texto(3, id_do_computador()).texto(4, agora()).executar();
    enfileirar("arquivo_local", id, "upsert");
    t.confirmar();
}

void FilunaDb::marcar_arquivo_ausente(const std::string& id)
{
    Transacao t(m_db);
    Comando(m_db, "UPDATE arquivo_local SET encontrado = 0, atualizado_em = ?2 WHERE id = ?1 AND encontrado = 1")
        .texto(1, id).texto(2, agora()).executar();
    if (sqlite3_changes(m_db) > 0)
        enfileirar("arquivo_local", id, "upsert");
    t.confirmar();
}

void FilunaDb::unir_produtos(const std::string& de, const std::string& para)
{
    Transacao t(m_db);
    const std::string ts = agora();
    std::vector<std::string> trabalhos;
    {
        Comando c(m_db, "SELECT id FROM trabalho WHERE produto_id = ?1");
        c.texto(1, de);
        while (c.passo())
            trabalhos.push_back(c.texto_em(0));
    }
    Comando(m_db, "UPDATE trabalho SET produto_id = ?2, atualizado_em = ?3 WHERE produto_id = ?1").texto(1, de).texto(2, para).texto(3, ts).executar();
    // A assinatura sai do produto excluído para não bloquear um novo produto com a mesma peça.
    Comando(m_db, "UPDATE produto SET excluido = 1, assinatura = NULL, atualizado_em = ?2 WHERE id = ?1").texto(1, de).texto(2, ts).executar();
    for (const auto& id : trabalhos)
        enfileirar("trabalho", id, "upsert");
    enfileirar("produto", de, "upsert");
    t.confirmar();
}

int FilunaDb::trabalhos_concluidos_do_produto(const std::string& produto_id) const
{
    Comando c(m_db, "SELECT COUNT(*) FROM trabalho WHERE produto_id = ?1 AND estado = 'concluido' AND excluido = 0");
    c.texto(1, produto_id);
    return c.passo() ? static_cast<int>(c.inteiro_em(0)) : 0;
}

std::string FilunaDb::criar_trabalho(const std::string& produto_id, const std::string& impressora_id, const GcodeResumo& resumo, int copias,
                                     const std::string& gcode_sha256)
{
    Transacao t(m_db);
    const std::string id = novo_uuid(), ts = agora();
    Comando(m_db, "INSERT INTO trabalho(id, criado_em, atualizado_em, produto_id, impressora_id, gcode_sha256, estado, copias, "
                  "tempo_previsto_s, camadas_total, trocas_filamento, purga_total_g) VALUES(?1, ?2, ?2, ?3, ?4, ?5, 'fatiado', ?6, ?7, ?8, ?9, ?10)")
        .texto(1, id).texto(2, ts).texto_ou_nulo(3, produto_id).texto_ou_nulo(4, impressora_id).texto_ou_nulo(5, gcode_sha256)
        .inteiro(6, std::max(copias, 1)).opcional(7, resumo.tempo_previsto_s).opcional(8, resumo.camadas_total)
        .opcional(9, resumo.trocas_filamento).opcional(10, resumo.purga_total_g).executar();
    enfileirar("trabalho", id, "upsert");

    // Purga por filamento informada pela fonte (exata); senão, rateio do total do G-code (estimado);
    // sem nenhum dos dois, a purga fica desconhecida.
    RateioPurga rateio;
    const bool  purga_exata = resumo.purga_g.size() == resumo.filamento_g.size() && !resumo.purga_g.empty();
    const bool  tem_purga   = purga_exata || resumo.purga_total_g.has_value();
    if (purga_exata)
        rateio.purga_g = resumo.purga_g;
    else if (resumo.purga_total_g)
        rateio = ratear_purga(resumo.filamento_g, *resumo.purga_total_g);
    for (std::size_t i = 0; i < resumo.filamento_g.size(); ++i) {
        const std::string fid = novo_uuid();
        Comando c(m_db, "INSERT INTO trabalho_filamento(id, criado_em, atualizado_em, trabalho_id, indice, previsto_g, purga_g, estimado) "
                        "VALUES(?1, ?2, ?2, ?3, ?4, ?5, ?6, ?7)");
        c.texto(1, fid).texto(2, ts).texto(3, id).inteiro(4, static_cast<std::int64_t>(i)).real(5, resumo.filamento_g[i]);
        if (tem_purga)
            c.real(6, rateio.purga_g[i]);
        else
            c.nulo(6);
        c.inteiro(7, rateio.estimado ? 1 : 0).executar();
        enfileirar("trabalho_filamento", fid, "upsert");
    }
    t.confirmar();
    return id;
}

void FilunaDb::confirmar_bobina(const std::string& trabalho_id, int indice, const std::string& bobina_id)
{
    Transacao t(m_db);
    std::string fid;
    {
        Comando c(m_db, "SELECT id FROM trabalho_filamento WHERE trabalho_id = ?1 AND indice = ?2");
        c.texto(1, trabalho_id).inteiro(2, indice);
        if (!c.passo())
            throw std::runtime_error("FilunaDb: filamento do trabalho não encontrado");
        fid = c.texto_em(0);
    }
    Comando(m_db, "UPDATE trabalho_filamento SET bobina_id = ?2, atualizado_em = ?3 WHERE id = ?1").texto(1, fid).texto(2, bobina_id).texto(3, agora()).executar();
    enfileirar("trabalho_filamento", fid, "upsert");
    t.confirmar();
}

void FilunaDb::marcar_estado(const std::string& trabalho_id, const std::string& estado)
{
    Transacao t(m_db);
    const std::string ts = agora();
    Comando(m_db, "UPDATE trabalho SET estado = ?2, atualizado_em = ?3, "
                  "iniciado_em = CASE WHEN ?2 = 'imprimindo' AND iniciado_em IS NULL THEN ?3 ELSE iniciado_em END WHERE id = ?1")
        .texto(1, trabalho_id).texto(2, estado).texto(3, ts).executar();
    enfileirar("trabalho", trabalho_id, "upsert");
    t.confirmar();
}

ResultadoFimTrabalho FilunaDb::finalizar_trabalho(const std::string& trabalho_id, FimTrabalho fim, const FracaoImpressa& fracao,
                                                  std::optional<std::int64_t> tempo_informado_s, std::optional<int> camada_parada)
{
    Transacao t(m_db);
    std::string                 impressora_id;
    std::optional<std::int64_t> tempo_previsto_s;
    {
        Comando c(m_db, "SELECT COALESCE(impressora_id,''), tempo_previsto_s FROM trabalho WHERE id = ?1");
        c.texto(1, trabalho_id);
        if (!c.passo())
            throw std::runtime_error("FilunaDb: trabalho não encontrado");
        impressora_id = c.texto_em(0);
        if (!c.nulo_em(1))
            tempo_previsto_s = c.inteiro_em(1);
    }
    struct Linha
    {
        std::string id, bobina_id;
    };
    std::vector<Linha>  linhas;
    std::vector<double> previsto, purga;
    bool                purga_estimada = false;
    {
        Comando c(m_db, "SELECT id, COALESCE(bobina_id,''), COALESCE(previsto_g,0), purga_g, estimado FROM trabalho_filamento "
                        "WHERE trabalho_id = ?1 AND excluido = 0 ORDER BY indice");
        c.texto(1, trabalho_id);
        while (c.passo()) {
            linhas.push_back({c.texto_em(0), c.texto_em(1)});
            previsto.push_back(c.real_em(2));
            purga.push_back(c.nulo_em(3) ? 0. : c.real_em(3));
            purga_estimada = purga_estimada || c.inteiro_em(4) != 0;
        }
    }
    // O G-code já traz todas as peças da mesa; as cópias ficam só em trabalho.copias (ranking).
    ResultadoFimTrabalho res = eventos_fim_trabalho(fim, fracao, previsto, purga, purga_estimada, 1, tempo_previsto_s, tempo_informado_s);

    const std::string ts = agora();
    for (const EventoEstoque& e : res.eventos) {
        const std::string& bobina_id = linhas[static_cast<std::size_t>(e.indice)].bobina_id;
        if (bobina_id.empty())
            continue; // sem bobina confirmada: o consumo fica só no trabalho
        Comando(m_db, "UPDATE bobina SET estado = CASE WHEN estado = 'lacrada' THEN 'em_uso' ELSE estado END, "
                      "aberta_em = COALESCE(aberta_em, ?2), ultimo_uso_em = ?2 WHERE id = ?1")
            .texto(1, bobina_id).texto(2, ts).executar();
        inserir_evento(bobina_id, e.tipo, e.delta_g, trabalho_id, e.estimado, {});
    }
    for (std::size_t i = 0; i < linhas.size(); ++i) {
        Comando(m_db, "UPDATE trabalho_filamento SET real_g = ?2, estimado = CASE WHEN ?3 THEN 1 ELSE estimado END, atualizado_em = ?4 WHERE id = ?1")
            .texto(1, linhas[i].id).real(2, res.real_g[i]).inteiro(3, fracao.estimado ? 1 : 0).texto(4, ts).executar();
        enfileirar("trabalho_filamento", linhas[i].id, "upsert");
        if (!linhas[i].bobina_id.empty())
            recalcular_bobina(linhas[i].bobina_id);
    }
    const char* estado = fim == FimTrabalho::Concluido ? "concluido" : fim == FimTrabalho::Cancelado ? "cancelado" : "falhou";
    Comando(m_db, "UPDATE trabalho SET estado = ?2, tempo_real_s = ?3, camada_parada = ?4, terminado_em = ?5, atualizado_em = ?5 WHERE id = ?1")
        .texto(1, trabalho_id).texto(2, estado).inteiro(3, res.tempo_real_s).opcional(4, camada_parada).texto(5, ts).executar();
    enfileirar("trabalho", trabalho_id, "upsert");
    if (!impressora_id.empty()) {
        Comando(m_db, "UPDATE impressora SET horas_impressao_s = horas_impressao_s + ?2, atualizado_em = ?3 WHERE id = ?1")
            .texto(1, impressora_id).inteiro(2, res.tempo_real_s).texto(3, ts).executar();
        enfileirar("impressora", impressora_id, "upsert");
    }
    t.confirmar();
    return res;
}

// --- Painel -------------------------------------------------------------------------------------

FilunaDb::Desperdicio FilunaDb::desperdicio(const std::string& desde, const std::string& ate) const
{
    Desperdicio d;
    Comando c(m_db, "SELECT COALESCE(SUM(CASE WHEN tipo IN ('purga','falha') THEN ABS(delta_g) END), 0), "
                    "       COALESCE(SUM(ABS(delta_g)), 0) "
                    "FROM estoque_evento WHERE excluido = 0 AND tipo IN ('consumo','purga','falha') "
                    "AND julianday(criado_em) >= julianday(?1) AND julianday(criado_em) < julianday(?2)");
    c.texto(1, desde).texto(2, ate);
    if (c.passo()) {
        d.desperdicio_g   = c.real_em(0);
        d.consumo_total_g = c.real_em(1);
    }
    d.taxa = taxa_desperdicio(d.desperdicio_g, d.consumo_total_g);
    return d;
}

Previsao FilunaDb::previsao_tipo(const std::string& filamento_tipo_id, double limite_dias) const
{
    const std::string ts = agora();
    double gasto = 0., estoque = 0., dias_hist = 0.;
    int    trabalhos = 0;
    {
        Comando c(m_db, "SELECT COALESCE(SUM(CASE WHEN julianday(e.criado_em) >= julianday(?2) - 28 THEN ABS(e.delta_g) END), 0), "
                        "       COUNT(DISTINCT e.trabalho_id), "
                        "       COALESCE(julianday(?2) - MIN(julianday(e.criado_em)), 0) "
                        "FROM estoque_evento e JOIN bobina b ON b.id = e.bobina_id "
                        "WHERE b.filamento_tipo_id = ?1 AND e.excluido = 0 AND e.tipo IN ('consumo','purga','falha')");
        c.texto(1, filamento_tipo_id).texto(2, ts);
        if (c.passo()) {
            gasto     = c.real_em(0);
            trabalhos = static_cast<int>(c.inteiro_em(1));
            dias_hist = c.real_em(2);
        }
    }
    {
        Comando c(m_db, "SELECT COALESCE(SUM(restante_g), 0) FROM bobina WHERE filamento_tipo_id = ?1 AND excluido = 0 "
                        "AND estado IN ('lacrada','em_uso')");
        c.texto(1, filamento_tipo_id);
        if (c.passo())
            estoque = c.real_em(0);
    }
    return prever_estoque(gasto, estoque, trabalhos, dias_hist, limite_dias);
}

// --- Listagens para as telas -----------------------------------------------------------------------

std::vector<FilunaDb::Bobina> FilunaDb::bobinas(bool incluir_inativas) const
{
    std::vector<Bobina> out;
    Comando c(m_db, std::string("SELECT ") + COLUNAS_BOBINA + " FROM bobina WHERE excluido = 0" +
                        (incluir_inativas ? "" : " AND estado IN ('lacrada','em_uso')") + " ORDER BY codigo");
    while (c.passo())
        out.push_back(ler_bobina(c));
    return out;
}

void FilunaDb::descartar_bobina(const std::string& bobina_id, const std::string& obs)
{
    Transacao t(m_db);
    const auto b = bobina(bobina_id);
    if (!b)
        throw std::runtime_error("FilunaDb: bobina não encontrada");
    if (b->restante_g > 0.)
        inserir_evento(bobina_id, "descarte", -b->restante_g, {}, false, obs);
    Comando(m_db, "UPDATE bobina SET estado = 'descartada', atualizado_em = ?2 WHERE id = ?1").texto(1, bobina_id).texto(2, agora()).executar();
    recalcular_bobina(bobina_id);
    t.confirmar();
}

std::vector<FilunaDb::Evento> FilunaDb::eventos_da_bobina(const std::string& bobina_id) const
{
    std::vector<Evento> out;
    Comando c(m_db, "SELECT id, tipo, criado_em, COALESCE(trabalho_id,''), COALESCE(obs,''), delta_g, estimado FROM estoque_evento "
                    "WHERE bobina_id = ?1 AND excluido = 0 ORDER BY criado_em DESC, rowid DESC");
    c.texto(1, bobina_id);
    while (c.passo())
        out.push_back({c.texto_em(0), c.texto_em(1), c.texto_em(2), c.texto_em(3), c.texto_em(4), c.real_em(5), c.inteiro_em(6) != 0});
    return out;
}

namespace {
const char* const COLUNAS_TIPO = "id, COALESCE(marca,''), COALESCE(material,''), COALESCE(nome_produto,''), COALESCE(cor_nome,''), "
                                 "COALESCE(cor_hex,''), COALESCE(preset_orca,''), diametro_mm, COALESCE(densidade_g_cm3,0)";
FilunaDb::TipoCadastrado ler_tipo(const Comando& c)
{
    FilunaDb::TipoCadastrado t;
    t.id              = c.texto_em(0);
    t.marca           = c.texto_em(1);
    t.material        = c.texto_em(2);
    t.nome_produto    = c.texto_em(3);
    t.cor_nome        = c.texto_em(4);
    t.cor_hex         = c.texto_em(5);
    t.preset_orca     = c.texto_em(6);
    t.diametro_mm     = c.real_em(7);
    t.densidade_g_cm3 = c.real_em(8);
    return t;
}
} // namespace

std::vector<FilunaDb::TipoCadastrado> FilunaDb::tipos() const
{
    std::vector<TipoCadastrado> out;
    Comando c(m_db, std::string("SELECT ") + COLUNAS_TIPO + " FROM filamento_tipo WHERE excluido = 0 ORDER BY material, cor_nome");
    while (c.passo())
        out.push_back(ler_tipo(c));
    return out;
}

std::optional<FilunaDb::TipoCadastrado> FilunaDb::tipo(const std::string& id) const
{
    Comando c(m_db, std::string("SELECT ") + COLUNAS_TIPO + " FROM filamento_tipo WHERE id = ?1");
    c.texto(1, id);
    if (!c.passo())
        return std::nullopt;
    return ler_tipo(c);
}

std::string FilunaDb::tipo_igual_ou_novo(const FilamentoTipo& t)
{
    Transacao tr(m_db);
    {
        Comando c(m_db, "SELECT id FROM filamento_tipo WHERE excluido = 0 AND lower(COALESCE(marca,'')) = lower(?1) "
                        "AND lower(COALESCE(material,'')) = lower(?2) AND lower(COALESCE(cor_nome,'')) = lower(?3) "
                        "AND lower(COALESCE(cor_hex,'')) = lower(?4) AND abs(diametro_mm - ?5) < 0.001 LIMIT 1");
        c.texto(1, t.marca).texto(2, t.material).texto(3, t.cor_nome).texto(4, t.cor_hex).real(5, t.diametro_mm);
        if (c.passo())
            return c.texto_em(0);
    }
    const std::string id = criar_filamento_tipo(t);
    tr.confirmar();
    return id;
}

std::vector<FilunaDb::ProdutoRanking> FilunaDb::produtos() const
{
    std::vector<ProdutoRanking> out;
    Comando c(m_db, "SELECT p.id, COALESCE(p.nome,''), "
                    "  (SELECT COUNT(*) FROM trabalho t WHERE t.produto_id = p.id AND t.estado = 'concluido' AND t.excluido = 0), "
                    "  (SELECT COALESCE(SUM(f.real_g),0) FROM trabalho t JOIN trabalho_filamento f ON f.trabalho_id = t.id "
                    "     WHERE t.produto_id = p.id AND t.estado = 'concluido' AND t.excluido = 0) "
                    "FROM produto p WHERE p.excluido = 0 ORDER BY 3 DESC, 2");
    while (c.passo())
        out.push_back({c.texto_em(0), c.texto_em(1), static_cast<int>(c.inteiro_em(2)), c.real_em(3)});
    return out;
}

std::vector<FilunaDb::TrabalhoLista> FilunaDb::trabalhos(int limite) const
{
    std::vector<TrabalhoLista> out;
    {
        Comando c(m_db, "SELECT t.id, COALESCE(t.produto_id,''), COALESCE(p.nome,''), COALESCE(t.impressora_id,''), COALESCE(i.nome,''), "
                        "t.estado, t.criado_em, COALESCE(t.terminado_em,''), t.tempo_previsto_s, t.tempo_real_s, t.camadas_total, "
                        "(SELECT COALESCE(SUM(previsto_g),0) FROM trabalho_filamento WHERE trabalho_id = t.id), "
                        "(SELECT COALESCE(SUM(real_g),0) FROM trabalho_filamento WHERE trabalho_id = t.id), "
                        "(SELECT COALESCE(MAX(estimado),0) FROM trabalho_filamento WHERE trabalho_id = t.id) "
                        "FROM trabalho t LEFT JOIN produto p ON p.id = t.produto_id LEFT JOIN impressora i ON i.id = t.impressora_id "
                        "WHERE t.excluido = 0 ORDER BY t.criado_em DESC, t.rowid DESC LIMIT ?1");
        c.inteiro(1, limite);
        while (c.passo()) {
            TrabalhoLista t;
            t.id              = c.texto_em(0);
            t.produto_id      = c.texto_em(1);
            t.produto_nome    = c.texto_em(2);
            t.impressora_id   = c.texto_em(3);
            t.impressora_nome = c.texto_em(4);
            t.estado          = c.texto_em(5);
            t.criado_em       = c.texto_em(6);
            t.terminado_em    = c.texto_em(7);
            if (!c.nulo_em(8)) t.tempo_previsto_s = c.inteiro_em(8);
            if (!c.nulo_em(9)) t.tempo_real_s = c.inteiro_em(9);
            if (!c.nulo_em(10)) t.camadas_total = static_cast<int>(c.inteiro_em(10));
            t.previsto_g = c.real_em(11);
            t.real_g     = c.real_em(12);
            t.estimado   = c.inteiro_em(13) != 0;
            out.push_back(std::move(t));
        }
    }
    for (TrabalhoLista& t : out) {
        Comando c(m_db, "SELECT COALESCE(bobina_id,'') FROM trabalho_filamento WHERE trabalho_id = ?1 ORDER BY indice");
        c.texto(1, t.id);
        while (c.passo())
            t.bobinas.push_back(c.texto_em(0));
    }
    return out;
}

std::vector<FilunaDb::ImpressoraCadastrada> FilunaDb::impressoras() const
{
    std::vector<ImpressoraCadastrada> out;
    Comando c(m_db, "SELECT id, nome, COALESCE(fabricante,''), COALESCE(modelo,''), horas_impressao_s FROM impressora WHERE excluido = 0 ORDER BY nome");
    while (c.passo())
        out.push_back({c.texto_em(0), c.texto_em(1), c.texto_em(2), c.texto_em(3), static_cast<double>(c.inteiro_em(4)) / 3600.});
    return out;
}

std::string FilunaDb::impressora_por_nome(const std::string& nome, const std::string& fabricante, const std::string& modelo)
{
    {
        Comando c(m_db, "SELECT id FROM impressora WHERE excluido = 0 AND nome = ?1 LIMIT 1");
        c.texto(1, nome);
        if (c.passo())
            return c.texto_em(0);
    }
    return criar_impressora(nome, fabricante, modelo);
}

} // namespace Slic3r::Filuna
