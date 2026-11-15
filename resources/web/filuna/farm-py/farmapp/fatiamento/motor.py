"""Só o que farm_arquivos.py usa do fatiamento do site (site-v2/app/fatiamento/motor.py), para rodar no Filuna Slicer.
tipo_arquivo é cópia exata; o fatiamento de verdade no Slicer é o próprio programa sem tela."""
from __future__ import annotations

import os
import struct
import tempfile
from pathlib import Path

TRABALHO = Path(os.environ.get("FILUNA_TRABALHO") or tempfile.gettempdir()) / "filuna-farm"
LIMITE_BYTES = int(os.environ.get("FILUNA_LIMITE_MB", "100")) * 1024 * 1024
LIMITE_DESCOMPACTADO = 1536 * 1024 * 1024


class ErroFatiamento(Exception):
    """Erro com mensagem que pode ser mostrada ao cliente."""


def tipo_arquivo(nome: str, dados: bytes) -> str:
    if len(dados) > LIMITE_BYTES:
        raise ErroFatiamento(f"O arquivo passa de {LIMITE_BYTES // 1024 // 1024} MB.")
    ext = Path(nome or "").suffix.lower()
    if ext == ".3mf":
        if dados[:4] != b"PK\x03\x04":
            raise ErroFatiamento("Esse 3MF não é válido.")
        return "3mf"
    if ext == ".stl":
        if len(dados) >= 84:
            n = struct.unpack_from("<I", dados, 80)[0]
            if 84 + n * 50 == len(dados) and n > 0:
                return "stl"
        if dados.lstrip()[:5].lower() == b"solid" and b"facet" in dados[:4096].lower():
            return "stl"
        raise ErroFatiamento("Esse STL não é válido.")
    raise ErroFatiamento("Envie um arquivo .stl ou .3mf.")
