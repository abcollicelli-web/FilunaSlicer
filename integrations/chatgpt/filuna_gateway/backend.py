"""Draft backend adapter. Proposed endpoints are not deployed in the Filuna site."""
from __future__ import annotations

import os
from urllib.parse import urlparse

import httpx

from .contract import Principal, Refused


class RemoteBackend:
    """Delegates actions to an owner-approved Filuna Hub; never handles geometry."""

    def __init__(self):
        self.base = os.getenv("FILUNA_PLUGIN_BACKEND_URL", "").rstrip("/")
        p = urlparse(self.base)
        if p.scheme != "https" or not p.hostname or p.username or p.password or p.fragment:
            raise RuntimeError("FILUNA_PLUGIN_BACKEND_URL must be an approved HTTPS service")
        self.timeout = 12.0

    async def _call(self, route: str, payload: dict, principal: Principal) -> dict:
        # Before deployment the new Hub must independently verify the
        # bearer token, subject, tenant, ownership, plan and entitlements.
        async with httpx.AsyncClient(timeout=self.timeout, follow_redirects=False) as client:
            response = await client.post(
                self.base + route,
                json=payload,
                headers={"Authorization": "Bearer " + principal.access_token,
                         "Accept": "application/json"},
            )
        if response.status_code != 200:
            raise Refused("BACKEND_UNAVAILABLE_OR_DENIED")
        if len(response.content) > 65536:
            raise Refused("BACKEND_RESPONSE_TOO_LARGE")
        try:
            obj = response.json()
        except ValueError:
            raise Refused("BACKEND_RESPONSE_INVALID") from None
        if not isinstance(obj, dict):
            raise Refused("BACKEND_RESPONSE_INVALID")
        return obj

    async def available(self, principal: Principal) -> frozenset[str]:
        result = await self._call("/internal/chatgpt/v1/capabilities", {}, principal)
        return frozenset(t for t in result.get("certified_tools", []) if isinstance(t, str))

    async def execute(self, tool: str, args: dict, principal: Principal) -> dict:
        # No user-supplied URL/path, raw geometry or direct LAN print command.
        return await self._call(
            "/internal/chatgpt/v1/execute",
            {"tool": tool, "arguments": args}, principal)
