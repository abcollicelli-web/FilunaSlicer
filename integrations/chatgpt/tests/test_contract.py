"""Contract/security tests: no live customers, printers or site are touched."""
import asyncio
import json
import unittest
from pathlib import Path

from filuna_gateway.contract import Gateway, Principal, Refused, TOOLS, safe_result


class FakeBackend:
    def __init__(self, response=None, enabled=None):
        self.response = response or {"status": "saved", "library_id": "lib_123456"}
        self.enabled = frozenset(enabled if enabled is not None else TOOLS)
        self.calls = []

    async def available(self, principal):
        return self.enabled

    async def execute(self, tool, args, principal):
        self.calls.append((tool, args, principal.subject, principal.tenant))
        return self.response


class ContractTests(unittest.TestCase):
    def setUp(self):
        self.principal = Principal("person_123", "tenant_abc",
                                   frozenset({"filuna.read", "filuna.write", "filuna.stock", "filuna.print"}),
                                   "redacted_token")

    def call(self, backend, tool, arguments, principal=None):
        return asyncio.run(Gateway(backend).call(tool, arguments, principal or self.principal))

    def test_20_tools_no_unchecked_arguments(self):
        self.assertEqual(len(TOOLS), 20)
        for name, tool in TOOLS.items():
            self.assertEqual(tool.schema["type"], "object")
            self.assertFalse(tool.schema["additionalProperties"], name)

    def test_reject_cross_tenant_or_paths_in_tool_args(self):
        b = FakeBackend()
        for key in ("tenant", "customer_id", "path", "download_url", "gcode"):
            with self.assertRaisesRegex(Refused, "BAD_ARGUMENTS"):
                self.call(b, "filuna_save_model", {"model_id": "FLN-B5Q79R", key: "x"})
        self.assertEqual(b.calls, [])

    def test_reject_missing_token(self):
        with self.assertRaisesRegex(Refused, "AUTH_REQUIRED"):
            self.call(FakeBackend(), "filuna_printers", {},
                      Principal("u", "t", frozenset({"filuna.read"})))

    def test_reject_missing_scope(self):
        with self.assertRaisesRegex(Refused, "SCOPE_DENIED"):
            self.call(FakeBackend(), "filuna_invoice_commit",
                      {"preview_id": "prev_12345", "confirm": True, "idempotency_key": "idem_12345"},
                      Principal("u", "t", frozenset({"filuna.read"}), "token"))

    def test_certification_gate(self):
        b = FakeBackend(enabled={"filuna_printers"})
        with self.assertRaisesRegex(Refused, "TOOL_NOT_CERTIFIED"):
            self.call(b, "filuna_generate_parametric", {"parametric_id": "FLN-B5Q79R", "parameters": {}})
        self.assertEqual(b.calls, [])

    def test_invoice_commit_requires_real_consent(self):
        b = FakeBackend()
        for args in (
            {"preview_id": "prev_12345", "confirm": False, "idempotency_key": "idem_12345"},
            {"preview_id": "prev_12345", "confirm": True},
        ):
            with self.assertRaisesRegex(Refused, "BAD_ARGUMENTS"):
                self.call(b, "filuna_invoice_commit", args)
        self.assertEqual(b.calls, [])

    def test_no_geometry_binary_path_or_download_in_responses(self):
        b = FakeBackend({"status": "saved", "library_id": "lib_123456",
                         "gcode": "G1 X20", "bytes": "AAAA",
                         "download_url": "https://x/mesh.3mf",
                         "items": [{"id": "lib_123456", "title": "part.stl",
                                    "download": "file:///etc/passwd", "status": "saved"}]})
        result = self.call(b, "filuna_library", {})
        self.assertEqual(result, {"status": "saved", "library_id": "lib_123456",
                                  "items": [{"id": "lib_123456", "status": "saved"}]})
        self.assertNotIn("download", json.dumps(result))

    def test_model_saved_with_opaque_library_id_only(self):
        b = FakeBackend({"status": "saved", "library_id": "lib_123456",
                         "stl": "binary", "preview_url": "https://x/mesh.3mf"})
        self.assertEqual(
            self.call(b, "filuna_save_model", {"model_id": "FLN-B5Q79R"}),
            {"status": "saved", "library_id": "lib_123456"},
        )
        self.assertEqual(b.calls[0][2:], ("person_123", "tenant_abc"))

    def test_print_requires_local_approval(self):
        args = {"job_id": "job_123456", "idempotency_key": "idem_12345"}
        with self.assertRaisesRegex(Refused, "PRINT_REQUIRES_LOCAL_CONFIRMATION"):
            self.call(FakeBackend({"status": "printing"}), "filuna_request_print", args)
        result = self.call(FakeBackend({"status": "awaiting_device_confirmation"}),
                           "filuna_request_print", args)
        self.assertEqual(result["status"], "awaiting_device_confirmation")

    def test_bad_attachment_refs_rejected(self):
        for value in ("../../passwd", "https://bad.test/a.stl", "C:\\Windows\\file", ""):
            with self.assertRaisesRegex(Refused, "BAD_ARGUMENTS"):
                self.call(FakeBackend(), "filuna_import_model", {"attachment_id": value})

    def test_shopping_urls_ok_not_3d_model_files(self):
        result = safe_result("filuna_offers", {"items": [
            {"id": "offer_123", "product_url": "https://shop.example/produto", "title": "PLA"},
            {"id": "offer_456", "product_url": "https://example.test/mesh.3mf"},
        ]})
        self.assertEqual(result["items"][0]["product_url"], "https://shop.example/produto")
        self.assertNotIn("product_url", result["items"][1])

    def test_package_not_falsely_configured_for_public_mcp(self):
        folder = Path(__file__).resolve().parents[1] / "package"
        manifest = json.loads((folder / "plugin.json").read_text(encoding="utf8"))
        self.assertEqual(manifest["name"], "filuna-slicer")
        self.assertFalse((folder / "mcp.json").exists())


if __name__ == "__main__":
    unittest.main()
