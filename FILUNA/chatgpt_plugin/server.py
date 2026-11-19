"""Servidor @Filuna MCP: autenticação obrigatória e nunca entregar arquivos."""
from __future__ import annotations

import asyncio
import os
import time
from pathlib import Path

from pydantic import AnyHttpUrl
from mcp.server.auth.provider import AccessToken, TokenVerifier
from mcp.server.auth.settings import AuthSettings
from mcp.server.auth.middleware.auth_context import get_access_token
from mcp.server.fastmcp import FastMCP

from .core import FilunaError, Hub, OPERATIONS, _json_request, validate_chatgpt_file


class FilunaTokenVerifier(TokenVerifier):
    """Tokens de clientes são introspectados pelo Hub, nunca aceitos por confiança."""

    async def verify_token(self, token: str) -> AccessToken | None:
        url = os.getenv("FILUNA_PLUGIN_INTROSPECTION_URL", "")
        issuer = os.getenv("FILUNA_PLUGIN_OAUTH_ISSUER", "")
        resource = os.getenv("FILUNA_PLUGIN_RESOURCE_URL", "")
        secret = os.getenv("FILUNA_PLUGIN_INTROSPECTION_KEY", "")
        if (os.getenv("FILUNA_PLUGIN_READY") != "1" or
                not all((url.startswith("https://"), issuer.startswith("https://"),
                         resource.startswith("https://"), secret, token))):
            return None
        try:
            data = await asyncio.to_thread(
                _json_request, url, "POST", {"token": token},
                {"Authorization": "Bearer " + secret,
                 "Content-Type": "application/json", "Accept": "application/json"})
            aud = data.get("aud")
            exp = data.get("exp")
            if (data.get("active") is not True or data.get("iss") != issuer or
                    (aud != resource and not (isinstance(aud, list) and resource in aud)) or
                    not isinstance(exp, (int, float)) or exp <= time.time() or
                    not data.get("sub") or not data.get("client_id")):
                return None
            scope = data.get("scope", "")
            scopes = scope.split() if isinstance(scope, str) else scope
            if not isinstance(scopes, list) or any(not isinstance(s, str) for s in scopes):
                return None
            return AccessToken(token=token, client_id=data["client_id"],
                               scopes=scopes, expires_at=int(exp))
        except (FilunaError, KeyError, ValueError, TypeError):
            return None


def make_server() -> FastMCP:
    mcp = FastMCP(
        name="Filuna", json_response=True,
        token_verifier=FilunaTokenVerifier(),
        auth=AuthSettings(
            issuer_url=AnyHttpUrl(os.getenv("FILUNA_PLUGIN_OAUTH_ISSUER",
                                          "https://filuna3d.com/")),
            resource_server_url=AnyHttpUrl(os.getenv(
                "FILUNA_PLUGIN_RESOURCE_URL", "https://filuna3d.com/filuna-plugin/mcp")),
            required_scopes=[],
        ),
    )
    hub = Hub()

    async def call(name: str, args: dict) -> dict:
        token = get_access_token()
        if token is None:
            raise FilunaError("Conecte sua conta Filuna para continuar.")
        return await hub.invoke(name, args, token.token, list(token.scopes))

    @mcp.tool()
    async def filuna_conta_resumo() -> dict:
        """Consultar sua conta e plano Filuna."""
        return await call("filuna_conta_resumo", {})

    @mcp.tool()
    async def filuna_biblioteca_listar(busca: str = "", tipo: str = "") -> dict:
        """Listar metadados da biblioteca; nunca baixar arquivos."""
        return await call("filuna_biblioteca_listar",
                          {k: v for k, v in {"busca": busca, "tipo": tipo}.items() if v})

    @mcp.tool()
    async def filuna_catalogo_pesquisar(busca: str) -> dict:
        """Pesquisar modelos, ferramentas e paramétricos autorizados."""
        return await call("filuna_catalogo_pesquisar", {"busca": busca})

    @mcp.tool()
    async def filuna_biblioteca_guardar(catalogo_id: str) -> dict:
        """Guardar item autorizado na biblioteca, sem link de download."""
        return await call("filuna_biblioteca_guardar", {"catalogo_id": catalogo_id})

    @mcp.tool()
    async def filuna_parametrico_gerar(codigo: str, parametros: dict | None = None) -> dict:
        """Gerar paramétrico no Slicer e salvar na biblioteca; jamais enviar STL/3MF ao chat."""
        return await call("filuna_parametrico_gerar",
                          {"codigo": codigo, **({"parametros": parametros} if parametros else {})})

    @mcp.tool()
    async def filuna_ferramenta_executar(ferramenta: str, biblioteca_id: str,
                                        parametros: dict | None = None) -> dict:
        """Executar ferramenta sobre modelo da biblioteca e salvar uma revisão."""
        return await call("filuna_ferramenta_executar",
                          {"ferramenta": ferramenta, "biblioteca_id": biblioteca_id,
                           **({"parametros": parametros} if parametros else {})})

    @mcp.tool()
    async def filuna_modelo_corrigir(biblioteca_id: str, tipo_correcao: str,
                                    parametros: dict | None = None) -> dict:
        """Corrigir geometria de modelo próprio e armazenar a revisão na biblioteca."""
        return await call("filuna_modelo_corrigir",
                          {"biblioteca_id": biblioteca_id, "tipo_correcao": tipo_correcao,
                           **({"parametros": parametros} if parametros else {})})

    @mcp.tool()
    async def filuna_estoque_listar() -> dict:
        """Consultar bobinas e quantidades do estoque do cliente."""
        return await call("filuna_estoque_listar", {})

    @mcp.tool()
    async def filuna_nota_fiscal_analisar(documento_biblioteca_id: str) -> dict:
        """Analisar nota previamente enviada ao Filuna; apenas prévia, sem cadastro."""
        return await call("filuna_nota_fiscal_analisar",
                          {"documento_biblioteca_id": documento_biblioteca_id})

    @mcp.tool()
    async def filuna_nota_fiscal_gravar(previsualizacao_id: str,
                                       confirmacao: bool) -> dict:
        """Gravar nota e bobinas só após conferência e confirmação explícita."""
        return await call("filuna_nota_fiscal_gravar",
                          {"previsualizacao_id": previsualizacao_id, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_impressoras_listar() -> dict:
        """Listar impressoras vinculadas, estados e conexão."""
        return await call("filuna_impressoras_listar", {})

    @mcp.tool()
    async def filuna_impressora_erros(impressora_id: str) -> dict:
        """Consultar diagnósticos sanitizados de uma impressora da conta."""
        return await call("filuna_impressora_erros", {"impressora_id": impressora_id})

    @mcp.tool(meta={"ui": {"resourceUri": "ui://filuna/impressao.html"}, "openai/widgetAccessible": True})
    async def filuna_impressao_preparar(biblioteca_id: str, impressora_id: str) -> dict:
        """Preparar impressão sem ligar ou iniciar hardware."""
        return await call("filuna_impressao_preparar",
                          {"biblioteca_id": biblioteca_id, "impressora_id": impressora_id})

    @mcp.tool()
    async def filuna_impressao_confirmar(preparo_id: str, confirmacao: bool) -> dict:
        """Solicitar impressão; também exige confirmação local pelo Filuna Windows."""
        return await call("filuna_impressao_confirmar",
                          {"preparo_id": preparo_id, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_impressao_status(trabalho_id: str) -> dict:
        """Consultar estado verdadeiro de trabalho sem expor G-code."""
        return await call("filuna_impressao_status", {"trabalho_id": trabalho_id})

    @mcp.tool()
    async def filuna_ofertas_listar(busca: str = "", material: str = "") -> dict:
        """Consultar ofertas de produtos e materiais, nunca links de arquivos 3D."""
        return await call("filuna_ofertas_listar",
                          {k: v for k, v in {"busca": busca, "material": material}.items() if v})


    # Consultas e alterações adicionais: usuário autenticado, por scope e tenant.
    @mcp.tool()
    async def filuna_assinatura_consultar() -> dict:
        """Consultar assinatura consultar. Não devolver arquivos nem segredos."""
        return await call("filuna_assinatura_consultar", {})

    @mcp.tool()
    async def filuna_faturas_listar() -> dict:
        """Consultar faturas listar. Não devolver arquivos nem segredos."""
        return await call("filuna_faturas_listar", {})

    @mcp.tool()
    async def filuna_fatura_consultar(fatura_id: str) -> dict:
        """Consultar fatura consultar. Não devolver arquivos nem segredos."""
        return await call("filuna_fatura_consultar", {"fatura_id": fatura_id})

    @mcp.tool()
    async def filuna_armazenamento_consultar() -> dict:
        """Consultar armazenamento consultar. Não devolver arquivos nem segredos."""
        return await call("filuna_armazenamento_consultar", {})

    @mcp.tool()
    async def filuna_arquivos_estatisticas() -> dict:
        """Consultar arquivos estatisticas. Não devolver arquivos nem segredos."""
        return await call("filuna_arquivos_estatisticas", {})

    @mcp.tool()
    async def filuna_projetos_listar() -> dict:
        """Consultar projetos listar. Não devolver arquivos nem segredos."""
        return await call("filuna_projetos_listar", {})

    @mcp.tool()
    async def filuna_licencas_consultar() -> dict:
        """Consultar licencas consultar. Não devolver arquivos nem segredos."""
        return await call("filuna_licencas_consultar", {})

    @mcp.tool()
    async def filuna_producao_resumo() -> dict:
        """Consultar producao resumo. Não devolver arquivos nem segredos."""
        return await call("filuna_producao_resumo", {})

    @mcp.tool()
    async def filuna_historico_impressoes() -> dict:
        """Consultar historico impressoes. Não devolver arquivos nem segredos."""
        return await call("filuna_historico_impressoes", {})

    @mcp.tool()
    async def filuna_pedidos_listar() -> dict:
        """Consultar pedidos listar. Não devolver arquivos nem segredos."""
        return await call("filuna_pedidos_listar", {})

    @mcp.tool()
    async def filuna_pedido_consultar(pedido_id: str) -> dict:
        """Consultar pedido consultar. Não devolver arquivos nem segredos."""
        return await call("filuna_pedido_consultar", {"pedido_id": pedido_id})

    @mcp.tool()
    async def filuna_orcamentos_listar() -> dict:
        """Consultar orcamentos listar. Não devolver arquivos nem segredos."""
        return await call("filuna_orcamentos_listar", {})

    @mcp.tool()
    async def filuna_financeiro_resumo() -> dict:
        """Consultar financeiro resumo. Não devolver arquivos nem segredos."""
        return await call("filuna_financeiro_resumo", {})

    @mcp.tool()
    async def filuna_clientes_listar() -> dict:
        """Consultar clientes listar. Não devolver arquivos nem segredos."""
        return await call("filuna_clientes_listar", {})

    @mcp.tool()
    async def filuna_relatorios_resumo() -> dict:
        """Consultar relatorios resumo. Não devolver arquivos nem segredos."""
        return await call("filuna_relatorios_resumo", {})

    @mcp.tool()
    async def filuna_loja_resumo() -> dict:
        """Consultar loja resumo. Não devolver arquivos nem segredos."""
        return await call("filuna_loja_resumo", {})

    @mcp.tool()
    async def filuna_integracoes_listar() -> dict:
        """Consultar integracoes listar. Não devolver arquivos nem segredos."""
        return await call("filuna_integracoes_listar", {})

    @mcp.tool()
    async def filuna_alertas_listar() -> dict:
        """Consultar alertas listar. Não devolver arquivos nem segredos."""
        return await call("filuna_alertas_listar", {})

    @mcp.tool()
    async def filuna_calibracoes_listar() -> dict:
        """Consultar calibracoes listar. Não devolver arquivos nem segredos."""
        return await call("filuna_calibracoes_listar", {})

    @mcp.tool()
    async def filuna_manutencao_consultar(impressora_id: str) -> dict:
        """Consultar manutencao consultar. Não devolver arquivos nem segredos."""
        return await call("filuna_manutencao_consultar", {"impressora_id": impressora_id})

    @mcp.tool()
    async def filuna_backup_status() -> dict:
        """Consultar backup status. Não devolver arquivos nem segredos."""
        return await call("filuna_backup_status", {})

    @mcp.tool()
    async def filuna_suporte_chamados() -> dict:
        """Consultar suporte chamados. Não devolver arquivos nem segredos."""
        return await call("filuna_suporte_chamados", {})

    @mcp.tool()
    async def filuna_consumo_materiais() -> dict:
        """Consultar consumo materiais. Não devolver arquivos nem segredos."""
        return await call("filuna_consumo_materiais", {})

    @mcp.tool()
    async def filuna_estoque_movimentacoes() -> dict:
        """Consultar estoque movimentacoes. Não devolver arquivos nem segredos."""
        return await call("filuna_estoque_movimentacoes", {})

    @mcp.tool()
    async def filuna_dispositivos_listar() -> dict:
        """Consultar dispositivos listar. Não devolver arquivos nem segredos."""
        return await call("filuna_dispositivos_listar", {})

    @mcp.tool()
    async def filuna_preferencias_consultar() -> dict:
        """Consultar preferencias consultar. Não devolver arquivos nem segredos."""
        return await call("filuna_preferencias_consultar", {})

    @mcp.tool()
    async def filuna_estoque_bobina_ajustar(bobina_id: str, ajuste_motivo: str, quantidade: str, confirmacao: bool) -> dict:
        """Solicitar alteração autorizada de estoque bobina ajustar. Não devolver arquivos nem segredos."""
        return await call("filuna_estoque_bobina_ajustar", {"bobina_id": bobina_id, "ajuste_motivo": ajuste_motivo, "quantidade": quantidade, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_suporte_chamado_criar(assunto: str, descricao: str, confirmacao: bool) -> dict:
        """Solicitar alteração autorizada de suporte chamado criar. Não devolver arquivos nem segredos."""
        return await call("filuna_suporte_chamado_criar", {"assunto": assunto, "descricao": descricao, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_orcamento_criar(pedido_id: str, confirmacao: bool) -> dict:
        """Solicitar alteração autorizada de orcamento criar. Não devolver arquivos nem segredos."""
        return await call("filuna_orcamento_criar", {"pedido_id": pedido_id, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_projeto_renomear(biblioteca_id: str, novo_nome: str, confirmacao: bool) -> dict:
        """Solicitar alteração autorizada de projeto renomear. Não devolver arquivos nem segredos."""
        return await call("filuna_projeto_renomear", {"biblioteca_id": biblioteca_id, "novo_nome": novo_nome, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_pedido_status_atualizar(pedido_id: str, novo_estado: str, confirmacao: bool) -> dict:
        """Solicitar alteração autorizada de pedido status atualizar. Não devolver arquivos nem segredos."""
        return await call("filuna_pedido_status_atualizar", {"pedido_id": pedido_id, "novo_estado": novo_estado, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_alerta_marcar_lido(alerta_id: str, confirmacao: bool) -> dict:
        """Solicitar alteração autorizada de alerta marcar lido. Não devolver arquivos nem segredos."""
        return await call("filuna_alerta_marcar_lido", {"alerta_id": alerta_id, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_impressao_pausar(trabalho_id: str, confirmacao: bool) -> dict:
        """Solicitar alteração autorizada de impressao pausar. Não devolver arquivos nem segredos."""
        return await call("filuna_impressao_pausar", {"trabalho_id": trabalho_id, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_impressao_cancelar(trabalho_id: str, confirmacao: bool) -> dict:
        """Solicitar alteração autorizada de impressao cancelar. Não devolver arquivos nem segredos."""
        return await call("filuna_impressao_cancelar", {"trabalho_id": trabalho_id, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_ajuda_pesquisar(busca: str, modulo: str = "") -> dict:
        """Pesquisar a documentação oficial de ajuda do Filuna e apresentar procedimentos verificados, nunca inventar funcionalidades."""
        return await call("filuna_ajuda_pesquisar", {"busca": busca, **{k: v for k, v in {"modulo": modulo}.items() if v}})

    @mcp.tool()
    async def filuna_ajuda_artigo(artigo_id: str) -> dict:
        """Consultar artigo e tutorial oficial com versão e passos."""
        return await call("filuna_ajuda_artigo", {"artigo_id": artigo_id})

    @mcp.tool()
    async def filuna_diagnostico_guiado(categoria: str, descricao: str, impressora_id: str = "") -> dict:
        """Diagnóstico guiado sem executar ação física; consultar logs sanitizados e sugerir passos seguros."""
        return await call("filuna_diagnostico_guiado", {"categoria": categoria, "descricao": descricao, **{k: v for k, v in {"impressora_id": impressora_id}.items() if v}})

    @mcp.tool()
    async def filuna_suporte_chamado_consultar(chamado_id: str) -> dict:
        """Consultar evolução de chamado próprio e próximo retorno, sem dados de terceiros."""
        return await call("filuna_suporte_chamado_consultar", {"chamado_id": chamado_id})

    @mcp.tool()
    async def filuna_suporte_chamado_responder(chamado_id: str, mensagem: str, confirmacao: bool) -> dict:
        """Responder ao chamado do próprio titular após consentimento; não enviar segredos ou anexos."""
        return await call("filuna_suporte_chamado_responder", {"chamado_id": chamado_id, "mensagem": mensagem, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_suporte_humano_solicitar(assunto: str, descricao: str, confirmacao: bool) -> dict:
        """Solicitar atendimento humano no canal oficial após autorização; sem fingir resposta humana."""
        return await call("filuna_suporte_humano_solicitar", {"assunto": assunto, "descricao": descricao, "confirmacao": confirmacao})

    @mcp.tool()
    async def filuna_ajuda_capacidades() -> dict:
        """Ver funções disponíveis de verdade no plano e versão atual, separadas de recursos planejados."""
        return await call("filuna_ajuda_capacidades", {})

    @mcp.tool()
    async def filuna_ajuda_atendimento_historico() -> dict:
        """Consultar somente o histórico de atendimento da própria conta, quando autorizado."""
        return await call("filuna_ajuda_atendimento_historico", {})


    @mcp.tool(meta={"openai/fileParams": ["arquivo"]})
    async def filuna_arquivo_importar(arquivo: dict, dispositivo_id: str,
                                     nome_projeto: str = "") -> dict:
        """Importar arquivo STL/3MF ANEXADO PELO CLIENTE no ChatGPT para o Filuna Slicer
        Windows autenticado. O Hub transfere temporariamente para o dispositivo
        vinculado (sem persistência pública), valida tamanho/assinatura e salva na
        biblioteca privada do titular; a saída nunca contém STL, 3MF ou G-code.
        Essa função NÃO inicia fatiamento nem impressão.
        """
        ref = validate_chatgpt_file(arquivo)
        return await call("filuna_arquivo_importar",
                          {"arquivo": ref, "dispositivo_id": dispositivo_id,
                           **({"nome_projeto": nome_projeto} if nome_projeto else {})})

    @mcp.tool()
    async def filuna_arquivo_inspecionar(biblioteca_id: str) -> dict:
        """Inspecionar geometria e placas de um STL/3MF na biblioteca privada."""
        return await call("filuna_arquivo_inspecionar", {"biblioteca_id": biblioteca_id})

    @mcp.tool()
    async def filuna_fatiamento_recomendar(biblioteca_id: str, impressora_id: str,
                                          material: str = "", prioridade: str = "",
                                          perfil_id: str = "") -> dict:
        """Consultar impressora e bobinas reais; recomendar perfil oficial OEM,
        suportes, orientação e opções seguras, preservando multimaterial e
        características originais do 3MF. Não iniciar fatiamento nem imprimir.
        """
        kwargs = {"biblioteca_id": biblioteca_id, "impressora_id": impressora_id}
        kwargs.update({k: v for k, v in
                      {"material": material, "prioridade": prioridade,
                       "perfil_id": perfil_id}.items() if v})
        return await call("filuna_fatiamento_recomendar", kwargs)

    @mcp.tool()
    async def filuna_fatiamento_executar(recomendacao_id: str,
                                        confirmacao: bool) -> dict:
        """Executar fatiamento local no Filuna com Orca/OEM oficial e recomendações
        aprovadas, sem extrair o G-code ao ChatGPT. Requer confirmação expressa.
        Apenas prepara o trabalho, não inicia impressão física.
        """
        return await call("filuna_fatiamento_executar",
                          {"recomendacao_id": recomendacao_id, "confirmacao": confirmacao})

    @mcp.tool(meta={"ui": {"resourceUri": "ui://filuna/impressao.html"}, "openai/widgetAccessible": True})
    async def filuna_fatiamento_resultado(fatiamento_id: str) -> dict:
        """Consultar status real do fatiamento, massa, tempo, alertas e preparo.
        Nunca entregar arquivos ou G-code ao chat.
        """
        return await call("filuna_fatiamento_resultado", {"fatiamento_id": fatiamento_id})

    @mcp.resource("ui://filuna/impressao.html", mime_type="text/html;profile=mcp-app")
    def filuna_impressao_widget() -> str:
        """Controle de revisão do fatiamento e autorização de impressão pelo cliente."""
        return (Path(__file__).parent / "package" / "widgets" / "impressao.html").read_text(encoding="utf-8")

    assert len(OPERATIONS) == 63
    return mcp


if __name__ == "__main__":
    if os.getenv("FILUNA_PLUGIN_READY") != "1":
        raise SystemExit("MCP Filuna desativado: aguardando homologação.")
    make_server().run(transport="streamable-http")
