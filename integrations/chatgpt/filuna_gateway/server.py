"""MCP SDK v2 server. Disabled until owner approves an independent Filuna Hub."""
from __future__ import annotations

import os
import time

import httpx
from pydantic import AnyHttpUrl
from mcp.server import MCPServer
from mcp.server.auth.provider import AccessToken, TokenVerifier
from mcp.server.auth.settings import AuthSettings
from mcp.server.auth.middleware.auth_context import get_access_token

from .backend import RemoteBackend
from .contract import Gateway, Principal, Refused


class IntrospectionVerifier(TokenVerifier):
    """OAuth 2.1 bearer verifier, no anonymous or hard-coded token fallback."""

    def __init__(self, resource: str, issuer: str):
        self.resource = resource
        self.issuer = issuer.rstrip("/")
        self.introspection = os.environ["FILUNA_PLUGIN_INTROSPECTION_URL"]
        if not self.introspection.startswith("https://"):
            raise RuntimeError("Token introspection must use HTTPS")
        self.client_id = os.environ["FILUNA_PLUGIN_INTROSPECTION_CLIENT_ID"]
        self.secret = os.environ["FILUNA_PLUGIN_INTROSPECTION_SECRET"]

    async def verify_token(self, token: str) -> AccessToken | None:
        if not token or len(token) > 4096:
            return None
        try:
            async with httpx.AsyncClient(timeout=5.0, follow_redirects=False) as client:
                response = await client.post(self.introspection, data={"token": token},
                                             auth=(self.client_id, self.secret))
            if response.status_code != 200 or len(response.content) > 8192:
                return None
            claims = response.json()
            aud = claims.get("aud", [])
            aud = [aud] if isinstance(aud, str) else aud
            if not (claims.get("active") is True and isinstance(aud, list) and
                    self.resource in aud and str(claims.get("iss", "")).rstrip("/") == self.issuer and
                    isinstance(claims.get("sub"), str) and claims["sub"] and
                    isinstance(claims.get("tenant_id"), str) and claims["tenant_id"] and
                    int(claims.get("exp", 0)) > time.time()):
                return None
            scopes = claims.get("scope", "").split()
            if "filuna.read" not in scopes:
                return None
            return AccessToken(token=token,
                               client_id=str(claims.get("client_id", "filuna")),
                               subject=claims["sub"], scopes=scopes, resource=self.resource,
                               expires_at=int(claims["exp"]),
                               claims={"tenant_id": claims["tenant_id"]})
        except (ValueError, KeyError, TypeError, AttributeError, httpx.HTTPError):
            return None


def build_server() -> MCPServer:
    if os.environ.get("FILUNA_PLUGIN_ENABLED") != "1":
        raise RuntimeError("Plugin disabled until site changes are reviewed and approved")
    resource = os.environ["FILUNA_PLUGIN_RESOURCE_URL"]
    issuer = os.environ["FILUNA_PLUGIN_OAUTH_ISSUER"]
    if not resource.startswith("https://") or not issuer.startswith("https://"):
        raise RuntimeError("MCP and OAuth endpoints must use public HTTPS")
    gateway = Gateway(RemoteBackend())
    mcp = MCPServer(
        "Filuna Slicer",
        token_verifier=IntrospectionVerifier(resource, issuer),
        auth=AuthSettings(issuer_url=AnyHttpUrl(issuer),
                          resource_server_url=AnyHttpUrl(resource),
                          required_scopes=["filuna.read"],
                          validate_token_resource=True),
    )

    async def call(name: str, args: dict) -> dict:
        token = get_access_token()
        if token is None or not token.subject or not token.claims or not token.claims.get("tenant_id"):
            raise Refused("AUTH_REQUIRED")
        principal = Principal(subject=token.subject, tenant=token.claims["tenant_id"],
                              scopes=frozenset(token.scopes), access_token=token.token)
        return await gateway.call(name, args, principal)

    @mcp.tool()
    async def filuna_features(category: str = "all") -> dict:
        """Consultar os recursos realmente certificados e disponíveis no Filuna."""
        return await call("filuna_features", {"category": category})

    @mcp.tool()
    async def filuna_library(query: str = "", limit: int = 20) -> dict:
        """Ver sua biblioteca privada. Sem arquivos ou links de download."""
        args = {"limit": limit}
        if query:
            args["query"] = query
        return await call("filuna_library", args)

    @mcp.tool()
    async def filuna_models(query: str = "", limit: int = 20) -> dict:
        """Pesquisar catálogo de modelos 3D, somente informações descritivas."""
        args = {"limit": limit}
        if query:
            args["query"] = query
        return await call("filuna_models", args)

    @mcp.tool()
    async def filuna_save_model(model_id: str) -> dict:
        """Salvar modelo do catálogo na biblioteca Filuna, nunca no chat."""
        return await call("filuna_save_model", {"model_id": model_id})

    @mcp.tool()
    async def filuna_import_model(attachment_id: str, title: str = "Modelo importado") -> dict:
        """Importar anexo do próprio cliente para a biblioteca privada."""
        return await call("filuna_import_model", {"attachment_id": attachment_id, "title": title})

    @mcp.tool()
    async def filuna_parametrics(query: str = "") -> dict:
        """Consultar geradores paramétricos do Filuna Slicer."""
        return await call("filuna_parametrics", {"query": query} if query else {})

    @mcp.tool()
    async def filuna_generate_parametric(parametric_id: str, parameters: dict) -> dict:
        """Gerar paramétrico e salvar na biblioteca sem STL/3MF no ChatGPT."""
        return await call("filuna_generate_parametric", {"parametric_id": parametric_id, "parameters": parameters})

    @mcp.tool()
    async def filuna_tools(query: str = "") -> dict:
        """Consultar ferramentas de modelagem disponíveis no Filuna."""
        return await call("filuna_tools", {"query": query} if query else {})

    @mcp.tool()
    async def filuna_run_tool(tool_id: str, parameters: dict, input_library_id: str = "") -> dict:
        """Executar ferramenta; armazenar o resultado somente na biblioteca."""
        args = {"tool_id": tool_id, "parameters": parameters}
        if input_library_id:
            args["input_library_id"] = input_library_id
        return await call("filuna_run_tool", args)

    @mcp.tool()
    async def filuna_repair_model(library_id: str, repair_mode: str = "automatic") -> dict:
        """Reparar malha própria do cliente, mantendo o arquivo no Filuna."""
        return await call("filuna_repair_model", {"library_id": library_id, "repair_mode": repair_mode})

    @mcp.tool()
    async def filuna_stock(limit: int = 50) -> dict:
        """Consultar bobinas e saldos do estoque vinculado à conta."""
        return await call("filuna_stock", {"limit": limit})

    @mcp.tool()
    async def filuna_invoice_preview(attachment_id: str) -> dict:
        """Conferir nota fiscal anexada antes de qualquer entrada em estoque."""
        return await call("filuna_invoice_preview", {"attachment_id": attachment_id})

    @mcp.tool()
    async def filuna_invoice_commit(preview_id: str, idempotency_key: str, confirm: bool) -> dict:
        """Cadastrar nota no estoque após aprovação explícita e prévia válida."""
        return await call("filuna_invoice_commit", {"preview_id": preview_id, "idempotency_key": idempotency_key, "confirm": confirm})

    @mcp.tool()
    async def filuna_printers() -> dict:
        """Consultar impressoras vinculadas ao cliente Filuna."""
        return await call("filuna_printers", {})

    @mcp.tool()
    async def filuna_printer_status(printer_id: str) -> dict:
        """Consultar estado real da impressora e progresso de impressão."""
        return await call("filuna_printer_status", {"printer_id": printer_id})

    @mcp.tool()
    async def filuna_printer_errors(printer_id: str, limit: int = 20) -> dict:
        """Consultar erros de impressora sem expor logs ou credenciais."""
        return await call("filuna_printer_errors", {"printer_id": printer_id, "limit": limit})

    @mcp.tool()
    async def filuna_prepare_print(library_id: str, printer_id: str) -> dict:
        """Preparar trabalho no Filuna Windows; não inicia a impressão."""
        return await call("filuna_prepare_print", {"library_id": library_id, "printer_id": printer_id})

    @mcp.tool()
    async def filuna_request_print(job_id: str, idempotency_key: str) -> dict:
        """Solicitar envio; hardware exige confirmação local no Filuna Slicer."""
        return await call("filuna_request_print", {"job_id": job_id, "idempotency_key": idempotency_key})

    @mcp.tool()
    async def filuna_print_job(job_id: str) -> dict:
        """Acompanhar andamento real sem devolver G-code ou arquivos."""
        return await call("filuna_print_job", {"job_id": job_id})

    @mcp.tool()
    async def filuna_offers(query: str = "", category: str = "all", limit: int = 20) -> dict:
        """Pesquisar ofertas de filamentos, impressoras e acessórios."""
        args = {"category": category, "limit": limit}
        if query:
            args["query"] = query
        return await call("filuna_offers", args)

    return mcp


if __name__ == "__main__":
    build_server().run(transport="streamable-http")
