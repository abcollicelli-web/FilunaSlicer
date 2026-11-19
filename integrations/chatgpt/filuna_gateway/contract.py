"""Fail-closed contracts for the Filuna ChatGPT integration (no binary output)."""
from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any, Protocol

from jsonschema import Draft202012Validator


class Refused(ValueError):
    """A user operation is not allowed or is not ready."""


@dataclass(frozen=True)
class Principal:
    subject: str
    tenant: str
    scopes: frozenset[str]
    access_token: str = ""  # Never log or return this value.


@dataclass(frozen=True)
class Tool:
    description: str
    scope: str
    props: dict[str, dict]
    required: tuple[str, ...] = ()
    write: bool = False
    item_fields: tuple[str, ...] = ()
    result_fields: tuple[str, ...] = ("status", "job_id", "library_id", "preview_id", "invoice_id", "count")

    @property
    def schema(self) -> dict:
        return {"type": "object", "properties": self.props,
                "required": list(self.required), "additionalProperties": False}


def s(maximum: int = 128) -> dict:
    return {"type": "string", "minLength": 1, "maxLength": maximum}


def ref() -> dict:
    return {"type": "string", "minLength": 6, "maxLength": 128,
            "pattern": r"^[A-Za-z0-9][A-Za-z0-9_.:-]*$"}


def limit() -> dict:
    return {"type": "integer", "minimum": 1, "maximum": 50}


def params() -> dict:
    return {"type": "object", "maxProperties": 40,
            "additionalProperties": {"oneOf": [
                {"type": "boolean"}, {"type": "number", "minimum": -1000000, "maximum": 1000000},
                {"type": "string", "maxLength": 256}
            ]}}


TOOLS: dict[str, Tool] = {
    "filuna_features": Tool("Consultar recursos disponíveis e seus estados no Filuna.", "filuna.read", {"category": s(30)}, item_fields=("id", "title", "category", "status", "plan")),
    "filuna_library": Tool("Ver itens da biblioteca privada do cliente, apenas metadados.", "filuna.read", {"limit": limit(), "query": s(120)}, item_fields=("id", "title", "kind", "status", "created_at")),
    "filuna_models": Tool("Pesquisar modelos do catálogo sem entregar arquivos ou geometrias.", "filuna.read", {"query": s(120), "limit": limit()}, item_fields=("id", "title", "category", "status")),
    "filuna_save_model": Tool("Salvar um modelo do catálogo na biblioteca privada, sem download no chat.", "filuna.write", {"model_id": ref()}, ("model_id",), True),
    "filuna_import_model": Tool("Importar anexo do próprio cliente para a biblioteca, sem devolver o arquivo.", "filuna.write", {"attachment_id": ref(), "title": s(100)}, ("attachment_id",), True),
    "filuna_parametrics": Tool("Listar geradores paramétricos disponíveis.", "filuna.read", {"query": s(120)}, item_fields=("id", "title", "status", "plan")),
    "filuna_generate_parametric": Tool("Gerar paramétrico e salvar na biblioteca do cliente; nunca devolver STL/3MF.", "filuna.write", {"parametric_id": ref(), "parameters": params()}, ("parametric_id", "parameters"), True),
    "filuna_tools": Tool("Listar ferramentas de modelagem disponíveis no Filuna.", "filuna.read", {"query": s(120)}, item_fields=("id", "title", "status", "plan")),
    "filuna_run_tool": Tool("Executar ferramenta no Filuna sobre item da biblioteca e salvar resultado interno.", "filuna.write", {"tool_id": ref(), "input_library_id": ref(), "parameters": params()}, ("tool_id", "parameters"), True),
    "filuna_repair_model": Tool("Corrigir malha 3D do próprio cliente; resultado apenas na biblioteca.", "filuna.write", {"library_id": ref(), "repair_mode": {"type": "string", "enum": ["automatic", "manifold", "normals", "holes"]}}, ("library_id",), True),
    "filuna_stock": Tool("Consultar bobinas de filamento e saldos do estoque privado.", "filuna.read", {"limit": limit()}, item_fields=("id", "code", "material", "color", "remaining_g", "status")),
    "filuna_invoice_preview": Tool("Ler nota fiscal anexada e mostrar prévia estruturada para conferir, sem registrar automaticamente.", "filuna.stock", {"attachment_id": ref()}, ("attachment_id",), True, item_fields=("sku", "description", "quantity", "unit")),
    "filuna_invoice_commit": Tool("Gravar itens da nota fiscal no estoque somente após confirmação explícita e prévia válida.", "filuna.stock", {"preview_id": ref(), "confirm": {"const": True}, "idempotency_key": ref()}, ("preview_id", "confirm", "idempotency_key"), True),
    "filuna_printers": Tool("Ver impressoras vinculadas à conta Filuna.", "filuna.read", {}, item_fields=("id", "name", "vendor", "model", "connection", "state")),
    "filuna_printer_status": Tool("Consultar situação de uma impressora e progresso real informado pelo dispositivo.", "filuna.read", {"printer_id": ref()}, ("printer_id",), item_fields=("code", "severity", "summary", "at"), result_fields=("status", "printer_id", "state", "progress_pct", "updated_at")),
    "filuna_printer_errors": Tool("Ver erros e alertas de uma impressora, sem logs, IPs ou credenciais.", "filuna.read", {"printer_id": ref(), "limit": limit()}, ("printer_id",), item_fields=("code", "severity", "summary", "at")),
    "filuna_prepare_print": Tool("Preparar impressão de modelo salvo; não inicia hardware.", "filuna.print", {"library_id": ref(), "printer_id": ref()}, ("library_id", "printer_id"), True, result_fields=("status", "job_id", "printer_id", "estimated_minutes", "estimated_grams")),
    "filuna_request_print": Tool("Solicitar envio de impressão já preparada; só inicia depois da confirmação local no Filuna Slicer.", "filuna.print", {"job_id": ref(), "idempotency_key": ref()}, ("job_id", "idempotency_key"), True, result_fields=("status", "job_id", "printer_id")),
    "filuna_print_job": Tool("Consultar estado real de preparação, envio ou impressão; não entrega G-code.", "filuna.read", {"job_id": ref()}, ("job_id",), result_fields=("status", "job_id", "printer_id", "progress_pct", "updated_at")),
    "filuna_offers": Tool("Consultar ofertas de produtos e materiais, sem links para arquivos 3D.", "filuna.read", {"query": s(120), "category": s(30), "limit": limit()}, item_fields=("id", "title", "category", "price", "currency", "seller", "product_url")),
}

BLOCKED_TEXT = re.compile(r"(?i)(?:\.stl\b|\.3mf\b|\.gcode\b|\.step\b|\.obj\b|\.zip\b|data:application|base64,|/api/[^\s]*/download|\\\\[^\s]+|file://)")
SAFE_STATUS = frozenset({"ready", "queued", "pending", "created", "saved", "prepared", "awaiting_device_confirmation", "printing", "paused", "completed", "failed", "offline", "online", "idle", "processing", "canceled", "rejected", "available", "unavailable"})


class Backend(Protocol):
    async def execute(self, tool: str, args: dict, principal: Principal) -> dict: ...
    async def available(self, principal: Principal) -> frozenset[str]: ...


def _clean_scalar(value: Any, field: str) -> Any:
    if field in {"remaining_g", "progress_pct", "price", "estimated_minutes", "estimated_grams", "quantity"}:
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            return None
        return value if -1e9 < float(value) < 1e9 else None
    if not isinstance(value, str) or len(value) > 512 or BLOCKED_TEXT.search(value):
        return None
    if field == "status" and value not in SAFE_STATUS:
        return None
    if field == "product_url":
        if not value.startswith("https://") or any(c.isspace() for c in value):
            return None
    elif "://" in value or value.startswith(("/", "\\")):
        return None
    return value


def safe_result(name: str, raw: dict) -> dict:
    """Positive allowlist: discard all binary/file keys, even nested provider metadata."""
    spec = TOOLS[name]
    if not isinstance(raw, dict):
        raise Refused("BACKEND_RESPONSE_INVALID")
    projected = {k: v for k in spec.result_fields
                 if (v := _clean_scalar(raw.get(k), k)) is not None}
    if "items" in raw and spec.item_fields:
        if not isinstance(raw["items"], list):
            raise Refused("BACKEND_RESPONSE_INVALID")
        projected["items"] = []
        for item in raw["items"][:50]:
            if not isinstance(item, dict):
                continue
            projected["items"].append({k: v for k in spec.item_fields
                                       if (v := _clean_scalar(item.get(k), k)) is not None})
    if name == "filuna_request_print" and projected.get("status") not in {"awaiting_device_confirmation", "rejected"}:
        raise Refused("PRINT_REQUIRES_LOCAL_CONFIRMATION")
    return projected


class Gateway:
    def __init__(self, backend: Backend):
        self.backend = backend

    async def call(self, name: str, args: dict, principal: Principal) -> dict:
        if name not in TOOLS:
            raise Refused("UNKNOWN_TOOL")
        if not principal.subject or not principal.tenant or not principal.access_token:
            raise Refused("AUTH_REQUIRED")
        tool = TOOLS[name]
        if tool.scope not in principal.scopes:
            raise Refused("SCOPE_DENIED")
        if not isinstance(args, dict) or list(Draft202012Validator(tool.schema).iter_errors(args)):
            raise Refused("BAD_ARGUMENTS")
        # Backend independently revalidates OAuth, user/tenant, ownership, plan and quotas.
        if name not in await self.backend.available(principal):
            raise Refused("TOOL_NOT_CERTIFIED")
        data = await self.backend.execute(name, args, principal)
        return safe_result(name, data)
