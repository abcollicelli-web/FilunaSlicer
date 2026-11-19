"""Contratos seguros do plugin Filuna. Nenhum arquivo/URL de download sai pelo MCP."""
from __future__ import annotations

import asyncio
import json
import math
import os
import re
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass

MAX_RESPONSE = 65536
MAX_STRING = 360
IDENT = re.compile(r"^[A-Za-z0-9_-]{1,100}$")
BLOCKED = frozenset({
    "file", "files", "arquivo", "arquivo_url", "download", "download_url",
    "file_url", "url_download", "path", "caminho", "caminho_local",
    "payload", "raw", "content", "conteudo", "bytes", "binary", "binario",
    "blob", "base64", "b64", "gcode", "g_code", "mesh", "stl", "3mf",
    "obj", "amf", "step", "svg", "zip", "attachment", "anexo",
    "secret", "token", "access_token", "refresh_token", "authorization",
    "senha", "password", "api_key", "chave_api", "cookies", "headers",
    "internal_url", "preview_url", "image_url", "imagem_url", "geometry",
    "geometria", "command", "script", "source", "file_data", "data"
})
FIELDS = frozenset({
    "status", "mensagem", "total", "itens", "id", "biblioteca_id", "job_id",
    "trabalho_id", "preparo_id", "nota_id", "previsualizacao_id",
    "cadastro_id", "nome", "titulo", "descricao", "categoria", "tipo",
    "modelo", "material", "cor", "quantidade", "peso_g", "peso_restante_g",
    "peso_inicial_g", "tempo_estimado_min", "tempo_decorrido_min",
    "valor_total", "moeda", "preco", "preco_kg", "loja", "sku",
    "fabricante", "online", "estado", "impressora_id", "dispositivo_id",
    "etapa", "atualizado_em", "criado_em", "erro_codigo", "codigo",
    "severidade", "classe", "observacoes", "avisos", "detalhes",
    "medidas", "limites", "plano", "sucesso", "diagnostico", "percentual",
    "titulo_curto", "estoque_id", "bobina_id", "produto_id", "origem",
    "url_produto", "disponivel", "autorizacao_pendente", "confirmacao_local",
    "proxima_acao", "resultado", "fila", "impressoras", "bobinas",
    "ofertas", "equipamentos", "opcoes", "parametros", "cores", "placas",
    "codigo_erro", "categoria_erro", "acoes_recomendadas", "mensagem_erro"
})
FILE_SUFFIXES = (".stl", ".3mf", ".gcode", ".gco", ".zip", ".obj", ".step", ".amf", ".scad", ".pdf")


class FilunaError(Exception):
    """Falha controlada; não carrega payloads privados."""


@dataclass(frozen=True)
class Operation:
    method: str
    path: str
    scope: str
    required: tuple[str, ...] = ()
    optional: tuple[str, ...] = ()
    confirm: bool = False
    premium: bool = False


OPERATIONS: dict[str, Operation] = {
    "filuna_conta_resumo": Operation("GET", "/plugin/v1/conta", "filuna.read"),
    "filuna_biblioteca_listar": Operation("GET", "/plugin/v1/biblioteca", "filuna.library.read",
        optional=("busca", "tipo")),
    "filuna_catalogo_pesquisar": Operation("GET", "/plugin/v1/catalogo", "filuna.catalog.read",
        required=("busca",)),
    "filuna_biblioteca_guardar": Operation("POST", "/plugin/v1/biblioteca/guardar", "filuna.library.write",
        required=("catalogo_id",), premium=True),
    "filuna_parametrico_gerar": Operation("POST", "/plugin/v1/parametricos/gerar", "filuna.create",
        required=("codigo",), optional=("parametros",), premium=True),
    "filuna_ferramenta_executar": Operation("POST", "/plugin/v1/ferramentas/executar", "filuna.create",
        required=("ferramenta", "biblioteca_id"), optional=("parametros",), premium=True),
    "filuna_modelo_corrigir": Operation("POST", "/plugin/v1/modelos/corrigir", "filuna.create",
        required=("biblioteca_id", "tipo_correcao"), optional=("parametros",), premium=True),
    "filuna_estoque_listar": Operation("GET", "/plugin/v1/estoque", "filuna.stock.read"),
    "filuna_nota_fiscal_analisar": Operation("POST", "/plugin/v1/notas/analisar", "filuna.stock.write",
        required=("documento_biblioteca_id",), premium=True),
    "filuna_nota_fiscal_gravar": Operation("POST", "/plugin/v1/notas/gravar", "filuna.stock.write",
        required=("previsualizacao_id", "confirmacao"), confirm=True, premium=True),
    "filuna_impressoras_listar": Operation("GET", "/plugin/v1/impressoras", "filuna.printer.read"),
    "filuna_impressora_erros": Operation("GET", "/plugin/v1/impressoras/erros", "filuna.printer.read",
        required=("impressora_id",)),
    "filuna_impressao_preparar": Operation("POST", "/plugin/v1/impressao/preparar", "filuna.print",
        required=("biblioteca_id", "impressora_id"), premium=True),
    "filuna_impressao_confirmar": Operation("POST", "/plugin/v1/impressao/confirmar", "filuna.print",
        required=("preparo_id", "confirmacao"), confirm=True, premium=True),
    "filuna_impressao_status": Operation("GET", "/plugin/v1/impressao/status", "filuna.printer.read",
        required=("trabalho_id",)),
    "filuna_ofertas_listar": Operation("GET", "/plugin/v1/ofertas", "filuna.offers.read",
        optional=("busca", "material")),
}




# Cobertura expandida das áreas do cliente. Rotas CONTRATUAIS, não implantadas no site.
OPERATIONS["filuna_assinatura_consultar"] = Operation("GET", "/plugin/v1/assinatura", "filuna.billing.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_faturas_listar"] = Operation("GET", "/plugin/v1/faturas", "filuna.billing.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_fatura_consultar"] = Operation("GET", "/plugin/v1/faturas/detalhe", "filuna.billing.read", required=("fatura_id",), confirm=False, premium=False)
OPERATIONS["filuna_armazenamento_consultar"] = Operation("GET", "/plugin/v1/armazenamento", "filuna.library.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_arquivos_estatisticas"] = Operation("GET", "/plugin/v1/arquivos/estatisticas", "filuna.library.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_projetos_listar"] = Operation("GET", "/plugin/v1/projetos", "filuna.library.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_licencas_consultar"] = Operation("GET", "/plugin/v1/licencas", "filuna.library.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_producao_resumo"] = Operation("GET", "/plugin/v1/producao/resumo", "filuna.production.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_historico_impressoes"] = Operation("GET", "/plugin/v1/impressao/historico", "filuna.production.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_pedidos_listar"] = Operation("GET", "/plugin/v1/pedidos", "filuna.business.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_pedido_consultar"] = Operation("GET", "/plugin/v1/pedidos/detalhe", "filuna.business.read", required=("pedido_id",), confirm=False, premium=False)
OPERATIONS["filuna_orcamentos_listar"] = Operation("GET", "/plugin/v1/orcamentos", "filuna.business.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_financeiro_resumo"] = Operation("GET", "/plugin/v1/financeiro/resumo", "filuna.business.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_clientes_listar"] = Operation("GET", "/plugin/v1/clientes", "filuna.business.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_relatorios_resumo"] = Operation("GET", "/plugin/v1/relatorios", "filuna.business.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_loja_resumo"] = Operation("GET", "/plugin/v1/loja/resumo", "filuna.business.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_integracoes_listar"] = Operation("GET", "/plugin/v1/integracoes", "filuna.account.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_alertas_listar"] = Operation("GET", "/plugin/v1/alertas", "filuna.account.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_calibracoes_listar"] = Operation("GET", "/plugin/v1/calibracoes", "filuna.printer.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_manutencao_consultar"] = Operation("GET", "/plugin/v1/impressoras/manutencao", "filuna.printer.read", required=("impressora_id",), confirm=False, premium=False)
OPERATIONS["filuna_backup_status"] = Operation("GET", "/plugin/v1/backup/estado", "filuna.account.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_suporte_chamados"] = Operation("GET", "/plugin/v1/suporte/chamados", "filuna.account.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_consumo_materiais"] = Operation("GET", "/plugin/v1/estoque/consumo", "filuna.stock.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_estoque_movimentacoes"] = Operation("GET", "/plugin/v1/estoque/movimentacoes", "filuna.stock.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_dispositivos_listar"] = Operation("GET", "/plugin/v1/dispositivos", "filuna.account.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_preferencias_consultar"] = Operation("GET", "/plugin/v1/preferencias", "filuna.account.read", required=(), confirm=False, premium=False)
OPERATIONS["filuna_estoque_bobina_ajustar"] = Operation("POST", "/plugin/v1/estoque/bobinas/ajustar", "filuna.stock.write", required=("bobina_id","ajuste_motivo","quantidade",), optional=("confirmacao",), confirm=True, premium=True)
OPERATIONS["filuna_suporte_chamado_criar"] = Operation("POST", "/plugin/v1/suporte/chamados/criar", "filuna.account.write", required=("assunto","descricao",), optional=("confirmacao",), confirm=True, premium=True)
OPERATIONS["filuna_orcamento_criar"] = Operation("POST", "/plugin/v1/orcamentos/criar", "filuna.business.write", required=("pedido_id",), optional=("confirmacao",), confirm=True, premium=True)
OPERATIONS["filuna_projeto_renomear"] = Operation("POST", "/plugin/v1/projetos/renomear", "filuna.library.write", required=("biblioteca_id","novo_nome",), optional=("confirmacao",), confirm=True, premium=True)
OPERATIONS["filuna_pedido_status_atualizar"] = Operation("POST", "/plugin/v1/pedidos/estado", "filuna.business.write", required=("pedido_id","novo_estado",), optional=("confirmacao",), confirm=True, premium=True)
OPERATIONS["filuna_alerta_marcar_lido"] = Operation("POST", "/plugin/v1/alertas/lido", "filuna.account.write", required=("alerta_id",), optional=("confirmacao",), confirm=True, premium=True)
OPERATIONS["filuna_impressao_pausar"] = Operation("POST", "/plugin/v1/impressao/pausar", "filuna.print", required=("trabalho_id",), optional=("confirmacao",), confirm=True, premium=True)
OPERATIONS["filuna_impressao_cancelar"] = Operation("POST", "/plugin/v1/impressao/cancelar", "filuna.print", required=("trabalho_id",), optional=("confirmacao",), confirm=True, premium=True)
FIELDS = FIELDS | frozenset(["fatura_id","vence_em","proxima_cobranca","data_vencimento","vencida","valor","valor_centavos","desconto","total_pendente","recorrencia","status_pagamento","metodo_pagamento","data_pagamento","numero_fatura","periodo_inicio","periodo_fim","limite_bytes","usado_bytes","livre_bytes","total_bytes","quantidade_arquivos","quantidade_modelos","quantidade_projetos","quantidade_parametricos","quantidade_ferramentas","quantidade_impressoras","ultima_sincronizacao","ultima_copia","revisao","ativo","percentual_uso","total_gasto","total_recebido","despesas","receita","lucro","margem","nome_plano","periodo","expira_em","data_inicio","data_fim","valor_mensal","historico","consumo_g","data_movimento","pedido_id","faturamento","unidade","codigo_nf","num_nf","serie_nf","emissao","filamentos","em_impressao","em_fila","concluidos","falhados","impressao_ativa","tempo_restante_min","progress","uptime","falha_codigo","falha_descricao","recomendacao","resultado","projetos","armazenamento","faturas","pagamentos","assinatura","preferencias","integracoes","conexoes","chamados","alertas","producao","cliente_id","contagem","nome_fantasia","razao_social","destino","quantidade_bobinas","origem_dados","resumo","estado_sincronizacao","plataforma","canal","inicio","fim","responsavel","versao","dispositivo","limite","usado","livre","confirmacao_pendente","referencia","protocolo","saldo","disponibilidade","numero_serie_mascarado","nova_revisao_id","data","gravacao_id","registro_id","recurso","informacoes","movimentacoes","volumes","consumo","recebimentos","nome_categoria","tipo_plano","frequencia"])

def _safe_product_url(value: str) -> bool:
    try:
        u = urllib.parse.urlsplit(value)
        host = u.hostname or ""
        return (u.scheme == "https" and bool(host) and "." in host and
                not host.endswith((".local", ".internal")) and
                not host.startswith(("localhost", "127.", "10.", "192.168.", "169.254.")) and
                not u.username and not u.password and
                not u.path.lower().endswith(FILE_SUFFIXES) and
                not any(s in u.path.lower() for s in ("/download", "/arquivo", "/file/")))
    except ValueError:
        return False


def sanitize(value, *, offers=False, depth=0):
    """Lista positiva, recursiva e limitada. Desconfia até da resposta do Hub."""
    if depth > 5:
        return None
    if isinstance(value, bytes):
        return None
    if isinstance(value, bool) or value is None:
        return value
    if isinstance(value, (int, float)):
        return value if math.isfinite(value) else None
    if isinstance(value, str):
        if len(value) > MAX_STRING:
            return value[:MAX_STRING]
        if value.startswith(("data:", "file:", "blob:", "ftp:")):
            return None
        if re.search(r"(?i)(?:/api/.*(?:arquivo|download)|\\\\|/tmp/|/home/|/var/lib/)", value):
            return None
        return value
    if isinstance(value, list):
        return [x for item in value[:50] if (x := sanitize(item, offers=offers, depth=depth + 1)) is not None]
    if isinstance(value, dict):
        out = {}
        for key, val in list(value.items())[:100]:
            if not isinstance(key, str):
                continue
            low = key.lower()
            if low in BLOCKED or low not in FIELDS:
                continue
            if low == "url_produto":
                if not offers or not isinstance(val, str) or not _safe_product_url(val):
                    continue
            cleaned = sanitize(val, offers=offers, depth=depth + 1)
            if cleaned is not None:
                out[low] = cleaned
        return out
    return None


def check_args(name: str, args: dict) -> Operation:
    if name not in OPERATIONS:
        raise FilunaError("Função não reconhecida.")
    spec = OPERATIONS[name]
    if not isinstance(args, dict):
        raise FilunaError("Parâmetros inválidos.")
    permitted = set(spec.required + spec.optional)
    if set(args) - permitted or any(k not in args for k in spec.required):
        raise FilunaError("Parâmetros obrigatórios ausentes ou não permitidos.")
    if spec.confirm and args.get("confirmacao") is not True:
        raise FilunaError("Ação exige confirmação explícita.")
    for k, v in args.items():
        if k == "arquivo":
            validate_chatgpt_file(v)
        elif k == "confirmacao":
            if v is not True:
                raise FilunaError("Confirmação inválida.")
        elif k == "parametros":
            if not isinstance(v, dict) or len(v) > 32:
                raise FilunaError("Parâmetros do modelo inválidos.")
            for pk, pv in v.items():
                if not isinstance(pk, str) or not IDENT.fullmatch(pk) or isinstance(pv, (list, dict, bytes)):
                    raise FilunaError("Parâmetro não permitido.")
                if isinstance(pv, (int, float)) and (not math.isfinite(pv) or abs(pv) > 100000):
                    raise FilunaError("Medida fora do limite.")
                if not isinstance(pv, (bool, int, float, str)) or (isinstance(pv, str) and len(pv) > 160):
                    raise FilunaError("Valor de parâmetro inválido.")
        elif not isinstance(v, str) or not v.strip() or len(v) > 160:
            raise FilunaError("Identificador ou filtro inválido.")
    return spec


class _NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def _json_request(url: str, method: str, body: dict | None, headers: dict) -> dict:
    if not url.startswith("https://"):
        raise FilunaError("Integração ainda não configurada com HTTPS.")
    data = None if body is None else json.dumps(body, ensure_ascii=False).encode()
    req = urllib.request.Request(url, data=data, method=method, headers=headers)
    try:
        with urllib.request.build_opener(_NoRedirect()).open(req, timeout=12) as resp:
            payload = resp.read(MAX_RESPONSE + 1)
            if len(payload) > MAX_RESPONSE:
                raise FilunaError("Resposta do Filuna excedeu o limite.")
            val = json.loads(payload)
            if not isinstance(val, dict):
                raise FilunaError("Resposta do Filuna inválida.")
            return val
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError, ValueError) as exc:
        raise FilunaError("Serviço Filuna indisponível ou acesso não autorizado.") from exc


class Hub:
    def __init__(self, base: str | None = None):
        self.base = (base if base is not None else os.getenv("FILUNA_PLUGIN_HUB_URL", "")).rstrip("/")

    async def invoke(self, name: str, args: dict, token: str, scopes: list[str]) -> dict:
        spec = check_args(name, args)
        if not token or spec.scope not in scopes:
            raise FilunaError("Acesso à ferramenta não autorizado.")
        if os.getenv("FILUNA_PLUGIN_READY") != "1" or not self.base.startswith("https://"):
            raise FilunaError("Plugin em homologação: integração ao Hub ainda não liberada.")
        # A regra de plano/conta/dispositivo é obrigatória TAMBÉM no Hub.
        path = spec.path
        if spec.method == "GET" and args:
            path += "?" + urllib.parse.urlencode(args)
        body = args if spec.method == "POST" else None
        raw = await asyncio.to_thread(
            _json_request, self.base + path, spec.method, body,
            {"Authorization": "Bearer " + token, "Accept": "application/json",
             "Content-Type": "application/json", "User-Agent": "Filuna-ChatGPT-MCP/0.1"})
        if raw.get("status") == "erro":
            raise FilunaError("O Hub Filuna recusou esta operação.")
        # Proibido vazar arquivos mesmo se o backend enviar chaves indevidas.
        result = sanitize(raw, offers=(name == "filuna_ofertas_listar"))
        if name in ("filuna_biblioteca_guardar", "filuna_parametrico_gerar",
                    "filuna_ferramenta_executar", "filuna_modelo_corrigir"):
            if not result.get("biblioteca_id") and not result.get("job_id"):
                raise FilunaError("O Filuna não confirmou salvamento na biblioteca.")
        return result

OPERATIONS["filuna_ajuda_pesquisar"] = Operation("GET", "/plugin/v1/ajuda/pesquisar", "filuna.help.read", required=("busca",), optional=("modulo",), confirm=False, premium=False)

OPERATIONS["filuna_ajuda_artigo"] = Operation("GET", "/plugin/v1/ajuda/artigo", "filuna.help.read", required=("artigo_id",), optional=(), confirm=False, premium=False)

OPERATIONS["filuna_diagnostico_guiado"] = Operation("POST", "/plugin/v1/ajuda/diagnostico", "filuna.help.read", required=("categoria","descricao",), optional=("impressora_id",), confirm=False, premium=False)

OPERATIONS["filuna_suporte_chamado_consultar"] = Operation("GET", "/plugin/v1/suporte/chamado", "filuna.account.read", required=("chamado_id",), optional=(), confirm=False, premium=False)

OPERATIONS["filuna_suporte_chamado_responder"] = Operation("POST", "/plugin/v1/suporte/chamado/responder", "filuna.account.write", required=("chamado_id","mensagem",), optional=("confirmacao",), confirm=True, premium=False)

OPERATIONS["filuna_suporte_humano_solicitar"] = Operation("POST", "/plugin/v1/suporte/transferir", "filuna.account.write", required=("assunto","descricao",), optional=("confirmacao",), confirm=True, premium=False)

OPERATIONS["filuna_ajuda_capacidades"] = Operation("GET", "/plugin/v1/ajuda/capacidades", "filuna.help.read", required=(), optional=(), confirm=False, premium=False)

OPERATIONS["filuna_ajuda_atendimento_historico"] = Operation("GET", "/plugin/v1/ajuda/atendimento/historico", "filuna.account.read", required=(), optional=(), confirm=False, premium=False)

FIELDS = FIELDS | frozenset({"artigo_id", "modulo", "passos", "instrucoes", "versao_documentacao", "atendimento_id", "chamado_id", "resolvido", "proximo_passo", "tempo_estimado", "fonte", "documentacao", "solucao", "sintoma", "categoria", "orientacao", "prioridade", "mensagem", "status_chamado"})


# Upload somente de ANEXO DO USUÁRIO via ChatGPT. Nunca da biblioteca protegida.
TRUSTED_FILE_HOSTS = frozenset({
    "files.oaiusercontent.com", "chatgpt.com", "api.openai.com",
})
MAX_IMPORT_BYTES = 100 * 1024 * 1024
OPERATIONS["filuna_arquivo_importar"] = Operation(
    "POST", "/plugin/v1/arquivos/importar-chatgpt", "filuna.library.write",
    required=("arquivo", "dispositivo_id"), optional=("nome_projeto",), premium=False
)
OPERATIONS["filuna_arquivo_inspecionar"] = Operation(
    "GET", "/plugin/v1/arquivos/inspecionar", "filuna.create",
    required=("biblioteca_id",), premium=False
)
OPERATIONS["filuna_fatiamento_recomendar"] = Operation(
    "POST", "/plugin/v1/fatiamento/recomendar", "filuna.print",
    required=("biblioteca_id", "impressora_id"),
    optional=("material", "prioridade", "perfil_id"), premium=False
)
OPERATIONS["filuna_fatiamento_executar"] = Operation(
    "POST", "/plugin/v1/fatiamento/executar", "filuna.print",
    required=("recomendacao_id", "confirmacao"), confirm=True, premium=False
)
OPERATIONS["filuna_fatiamento_resultado"] = Operation(
    "GET", "/plugin/v1/fatiamento/resultado", "filuna.printer.read",
    required=("fatiamento_id",), premium=False
)

FIELDS = FIELDS | frozenset({
    "recomendacao_id", "fatiamento_id", "perfil_id", "prioridade", "estimativa",
    "resolucao_mm", "altura_camada_mm", "paredes", "preenchimento_percentual",
    "tipo_preenchimento", "velocidade_mm_s", "temperatura_bico_c",
    "temperatura_mesa_c", "suportes", "orientacao", "mesa", "risco",
    "avisos_seguranca", "temperatura", "tempo_restante_s", "tempo_s",
    "massa_estimada_g", "material_necessario_g", "lote_id", "aceite_local",
    "precisa_confirmacao", "motor", "perfil_origem", "modo_multicor",
    "preservado_multimesa", "somente_preparo", "perfil_comparado",
    "orientacoes", "job", "sem_alteracao"
})

def validate_chatgpt_file(file: dict) -> dict:
    """Aceita só referência efêmera de arquivo enviado pelo usuário ao ChatGPT."""
    if not isinstance(file, dict) or not {"download_url","file_id"} <= set(file):
        raise FilunaError("Anexo inválido: file_id e download_url são obrigatórios.")
    if set(file) - {"download_url","file_id","mime_type","file_name"}:
        raise FilunaError("Anexo contém campos não reconhecidos.")
    url, fid = file["download_url"], file["file_id"]
    if not isinstance(url, str) or not isinstance(fid, str):
        raise FilunaError("Referência do anexo inválida.")
    try:
        p = urllib.parse.urlsplit(url)
        hostname = (p.hostname or "").lower()
    except ValueError as exc:
        raise FilunaError("URL temporária inválida.") from exc
    if (p.scheme != "https" or p.port not in (None,443) or p.username or p.password
            or hostname not in TRUSTED_FILE_HOSTS or not p.path
            or len(url)>4096 or len(fid)>160 or not IDENT.fullmatch(fid)):
        raise FilunaError("Origem temporária do arquivo não autorizada.")
    filename = file.get("file_name") or ""
    mime = file.get("mime_type") or ""
    if not isinstance(filename, str) or len(filename)>140 or "/" in filename or "\\" in filename:
        raise FilunaError("Nome de arquivo inválido.")
    if not isinstance(mime, str) or len(mime)>100:
        raise FilunaError("Formato de arquivo inválido.")
    if filename and not filename.lower().endswith((".stl",".3mf")):
        raise FilunaError("Somente STL e 3MF podem ser importados neste fluxo.")
    # A validação REAL de tamanho, assinatura, ZIP bomb e malha ocorre no Hub
    # e no Windows, após streaming efêmero, antes de importar.
    return {"download_url":url, "file_id":fid, "mime_type":mime, "file_name":filename}
