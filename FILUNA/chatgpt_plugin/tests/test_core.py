"""Testes locais sem rede, sem banco e sem modificar o site."""
import asyncio
import os
import unittest
from unittest.mock import patch

from FILUNA.chatgpt_plugin.core import (
    BLOCKED, FilunaError, Hub, OPERATIONS, check_args, sanitize
)


class SecurityTest(unittest.TestCase):
    def test_no_file_download_or_model_geometry_in_recursive_output(self):
        fake = {"status": "salvo", "biblioteca_id": "LIB_01", "download_url": "https://a.com/x.3mf",
                "mesh": "solid cube", "token": "SECRET", "itens": [
                    {"nome": "peça", "arquivo_url": "/api/slicer/v1/trabalhos/1/arquivo",
                     "url_produto": "https://site.com/file.stl", "base64": "QUJD", "peso_g": 20}]}
        clean = sanitize(fake, offers=True)
        self.assertEqual(clean["biblioteca_id"], "LIB_01")
        self.assertEqual(clean["itens"], [{"nome": "peça", "peso_g": 20}])
        self.assertNotIn("mesh", clean)
        self.assertNotIn("download_url", clean)

    def test_offers_allow_product_but_not_file_url(self):
        o = {"itens": [
            {"nome": "PLA", "url_produto": "https://loja.com/ofertas/pla"},
            {"nome": "Peça", "url_produto": "https://loja.com/file.3mf"},
            {"nome": "Risco", "url_produto": "https://127.0.0.1/private"}]}
        clean = sanitize(o, offers=True)
        self.assertEqual(len([x for x in clean["itens"] if "url_produto" in x]), 1)
        self.assertNotIn("url_produto", sanitize(o)["itens"][0])

    def test_confirmations(self):
        for name, payload in (
            ("filuna_impressao_confirmar", {"preparo_id": "P1", "confirmacao": False}),
            ("filuna_nota_fiscal_gravar", {"previsualizacao_id": "NF1", "confirmacao": "sim"}),
        ):
            with self.assertRaises(FilunaError):
                check_args(name, payload)
        self.assertTrue(check_args("filuna_impressao_confirmar",
                                   {"preparo_id": "P1", "confirmacao": True}).confirm)

    def test_no_arbitrary_backend_operation_or_file_input(self):
        with self.assertRaises(FilunaError):
            check_args("filuna_debug_exec", {"command": "rm -rf /"})
        with self.assertRaises(FilunaError):
            check_args("filuna_parametrico_gerar", {"codigo": "X", "download": True})
        with self.assertRaises(FilunaError):
            check_args("filuna_modelo_corrigir", {"biblioteca_id": "A", "tipo_correcao": "buracos",
                                                "parametros": {"payload": ["mesh"]}})

    def test_required_all_tools(self):
        self.assertEqual(len(OPERATIONS), 63)
        self.assertEqual(sum(x.confirm for x in OPERATIONS.values()), 13)
        self.assertTrue(all(x.path.startswith("/plugin/v1/") for x in OPERATIONS.values()))

    def test_prevents_unverified_storage_claim(self):
        with patch.dict(os.environ, {"FILUNA_PLUGIN_READY": "1"}):
            with patch("FILUNA.chatgpt_plugin.core._json_request",
                       return_value={"status": "salvo", "download_url": "https://files.com/a.stl"}):
                with self.assertRaises(FilunaError):
                    asyncio.run(Hub("https://hub.example").invoke(
                        "filuna_parametrico_gerar", {"codigo": "C1"}, "token", ["filuna.create"]))

    def test_disabled_by_default(self):
        with patch.dict(os.environ, {"FILUNA_PLUGIN_READY": "0"}):
            with self.assertRaises(FilunaError):
                asyncio.run(Hub("https://hub.example").invoke(
                    "filuna_estoque_listar", {}, "valid", ["filuna.stock.read"]))

    def test_expanded_tool_confirmations(self):
        self.assertEqual(len(OPERATIONS), 63)
        with self.assertRaises(FilunaError):
            check_args("filuna_impressao_pausar", {"trabalho_id": "A", "confirmacao": False})
        self.assertTrue(check_args("filuna_impressao_pausar",
                                   {"trabalho_id": "A", "confirmacao": True}).confirm)


    def test_support_tools_safety(self):
        self.assertTrue(check_args("filuna_ajuda_pesquisar", {"busca": "erro de conexão"}))
        with self.assertRaises(FilunaError):
            check_args("filuna_suporte_humano_solicitar",
                       {"assunto": "Erro", "descricao": "Impressora offline", "confirmacao": False})
        self.assertTrue(check_args("filuna_suporte_humano_solicitar",
                       {"assunto": "Erro", "descricao": "Impressora offline", "confirmacao": True}).confirm)

    def test_chatgpt_file_params_validation(self):
        from FILUNA.chatgpt_plugin.core import validate_chatgpt_file
        safe = {"file_id": "file_A123", "download_url":
                "https://files.oaiusercontent.com/download?id=A",
                "file_name":"meu-projeto.3mf", "mime_type":"application/octet-stream"}
        self.assertEqual(validate_chatgpt_file(safe)["file_name"], "meu-projeto.3mf")
        args = {"arquivo":safe,"dispositivo_id":"meu-windows"}
        self.assertTrue(check_args("filuna_arquivo_importar", args))
        for bad in ("http://files.oaiusercontent.com/a",
                    "https://127.0.0.1/private",
                    "https://filuna3d.com/arquivo.stl"):
            with self.assertRaises(FilunaError):
                validate_chatgpt_file({**safe, "download_url":bad})
        with self.assertRaises(FilunaError):
            validate_chatgpt_file({**safe,"file_name":"exploit.exe"})
        with self.assertRaises(FilunaError):
            check_args("filuna_fatiamento_executar",
                       {"recomendacao_id":"R1","confirmacao":False})

    def test_scopes(self):
        with patch.dict(os.environ, {"FILUNA_PLUGIN_READY": "1"}):
            with self.assertRaises(FilunaError):
                asyncio.run(Hub("https://hub.example").invoke(
                    "filuna_impressoras_listar", {}, "token", ["filuna.stock.read"]))


if __name__ == "__main__":
    unittest.main()
