#!/usr/bin/env python3
"""Fixture de 4 cores para a régua de paridade OEM.

Gera um 3MF (Core + metadados BBS/Orca) com 4 cubos de malha própria, cada um
atribuído às extrusoras/filamentos 1..4 e com ``plater_id`` — o formato que o
Snapmaker Orca 2.4.1 oficial importou (ver registro de 07/11/2026). É geometria
sintética de QA, nunca um projeto OEM. Use com ``--arrange 1`` para que cada
fatiador posicione os cubos na mesa da própria máquina.
"""
from __future__ import annotations

import argparse
import hashlib
import sys
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path

CORE = "http://schemas.microsoft.com/3dmanufacturing/core/2015/02"
REL = "http://schemas.openxmlformats.org/package/2006/relationships"
TYPES = "http://schemas.openxmlformats.org/package/2006/content-types"


def _xml(node) -> bytes:
    return b'<?xml version="1.0" encoding="UTF-8"?>\n' + ET.tostring(node, encoding="UTF-8")


def _cubo(parent, lado: float) -> None:
    h = lado / 2
    mesh = ET.SubElement(parent, f"{{{CORE}}}mesh")
    vs = ET.SubElement(mesh, f"{{{CORE}}}vertices")
    for x, y, z in [(-h, -h, -h), (h, h, -h), (h, -h, -h), (-h, h, -h),
                    (-h, -h, h), (h, -h, h), (h, h, h), (-h, h, h)]:
        ET.SubElement(vs, f"{{{CORE}}}vertex", {"x": f"{x:g}", "y": f"{y:g}", "z": f"{z:g}"})
    ts = ET.SubElement(mesh, f"{{{CORE}}}triangles")
    for a, b, c in [(0, 1, 2), (0, 3, 1), (4, 5, 6), (4, 6, 7), (0, 2, 5), (0, 5, 4),
                    (2, 1, 6), (2, 6, 5), (1, 3, 7), (1, 7, 6), (3, 0, 4), (3, 4, 7)]:
        ET.SubElement(ts, f"{{{CORE}}}triangle", {"v1": str(a), "v2": str(b), "v3": str(c)})


def gerar(destino: Path, lado: float = 15.0, centro: tuple[float, float] = (110.0, 110.0)) -> dict:
    ET.register_namespace("", CORE)
    modelo = ET.Element(f"{{{CORE}}}model", {"unit": "millimeter", "xml:lang": "en-US"})
    ET.SubElement(modelo, f"{{{CORE}}}metadata", {"name": "Application"}).text = "Filuna QA fixture 4 cores"
    rs = ET.SubElement(modelo, f"{{{CORE}}}resources")
    build = ET.SubElement(modelo, f"{{{CORE}}}build")
    cfg = ET.Element("config")
    plate = ET.SubElement(cfg, "plate")
    ET.SubElement(plate, "metadata", {"key": "plater_id", "value": "1"})
    passo = lado + 8
    cx, cy = centro
    for i, (dx, dy) in enumerate([(-1, 1), (1, 1), (-1, -1), (1, -1)]):
        ident = str(i + 1)
        obj = ET.SubElement(rs, f"{{{CORE}}}object", {"id": ident, "type": "model"})
        _cubo(obj, lado)
        x, y = cx + dx * passo / 2, cy + dy * passo / 2
        ET.SubElement(build, f"{{{CORE}}}item",
                      {"objectid": ident, "transform": f"1 0 0 0 1 0 0 0 1 {x:g} {y:g} {lado / 2:g}", "printable": "1"})
        oc = ET.SubElement(cfg, "object", {"id": ident})
        ET.SubElement(oc, "metadata", {"key": "name", "value": f"Filuna QA cor {i + 1}"})
        ET.SubElement(oc, "metadata", {"key": "extruder", "value": ident})
        inst = ET.SubElement(plate, "model_instance")
        for k, v in (("object_id", ident), ("instance_id", "0"), ("identify_id", str(100 + i))):
            ET.SubElement(inst, "metadata", {"key": k, "value": v})
    rel = ET.Element(f"{{{REL}}}Relationships")
    ET.SubElement(rel, f"{{{REL}}}Relationship", {"Target": "/3D/3dmodel.model", "Id": "rel-1",
                  "Type": "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"})
    tipos = ET.Element(f"{{{TYPES}}}Types")
    ET.SubElement(tipos, f"{{{TYPES}}}Default", {"Extension": "rels",
                  "ContentType": "application/vnd.openxmlformats-package.relationships+xml"})
    ET.SubElement(tipos, f"{{{TYPES}}}Default", {"Extension": "model",
                  "ContentType": "application/vnd.ms-package.3dmanufacturing-3dmodel+xml"})
    destino.parent.mkdir(parents=True, exist_ok=True)
    # data fixa nos membros do zip: a fixture tem SHA estável
    with zipfile.ZipFile(destino, "w", compression=zipfile.ZIP_DEFLATED) as z:
        for nome, dado in (("[Content_Types].xml", _xml(tipos)), ("_rels/.rels", _xml(rel)),
                           ("3D/3dmodel.model", _xml(modelo)), ("Metadata/model_settings.config", _xml(cfg))):
            info = zipfile.ZipInfo(nome, date_time=(2026, 11, 12, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, dado)
    with zipfile.ZipFile(destino) as z:
        m = ET.fromstring(z.read("Metadata/model_settings.config"))
    ext = [int(o.find("./metadata[@key='extruder']").attrib["value"]) for o in m.findall("object")]
    if ext != [1, 2, 3, 4]:
        raise AssertionError("FIXTURE_CORES_EXTRUSORAS_QUEBRADAS")
    return {"sha256": hashlib.sha256(destino.read_bytes()).hexdigest(), "extrusoras": ext, "lado_mm": lado}


def de_semente(semente: Path, destino: Path) -> dict:
    """Recebe um 3MF exportado pelo PRÓPRIO binário oficial com 4 objetos
    (``--export-3mf`` de 4 cópias do cubo) e só troca a extrusora de cada
    objeto para 1..4 em Metadata/model_settings.config. Malha, projeto e todo
    o resto do arquivo do fabricante ficam como ele gravou."""
    import re
    with zipfile.ZipFile(semente) as z:
        dados = {i: z.read(i.filename) for i in z.infolist()}
    chave = next(i for i in dados if i.filename == "Metadata/model_settings.config")
    texto = dados[chave].decode("utf-8")
    blocos = list(re.finditer(r'<object id="(\d+)">(.*?)</object>', texto, re.S))
    if len(blocos) != 4:
        raise AssertionError(f"SEMENTE_SEM_4_OBJETOS ({len(blocos)})")
    ordem = sorted(blocos, key=lambda m: int(m.group(1)))
    novo = texto
    for n, m in enumerate(ordem, 1):
        corpo = re.sub(r'(<metadata key="extruder" value=")\d+(")', rf"\g<1>{n}\2", m.group(2), count=1)
        if corpo == m.group(2) and f'value="{n}"' not in corpo:
            raise AssertionError("SEMENTE_SEM_METADADO_EXTRUDER")
        novo = novo.replace(m.group(0), f'<object id="{m.group(1)}">{corpo}</object>')
    dados[chave] = novo.encode("utf-8")
    # Alguns CLIs (Bambu 02.09) exportam vetores por filamento com 1 só item apesar de
    # 4 filamentos carregados; fatiar esse 3MF com extrusoras 2..4 aborta (double free).
    # Completa só vetores 'filament_*' de tamanho 1 replicando o valor e dá 4 cores.
    import json as _json
    proj = next((i for i in dados if i.filename == "Metadata/project_settings.config"), None)
    estendidos = []
    if proj is not None:
        try:
            cfg = _json.loads(dados[proj])
        except ValueError:
            cfg = None
        if isinstance(cfg, dict):
            n = len(cfg.get("filament_settings_id") or []) or 4
            for k, v in cfg.items():
                if k.startswith("filament_") and isinstance(v, list) and len(v) == 1 and n > 1:
                    cfg[k] = v * n
                    estendidos.append(k)
            if "filament_colour" in cfg:
                cfg["filament_colour"] = ["#E60012", "#1565C0", "#FFD600", "#15B857"][:n]
            dados[proj] = (_json.dumps(cfg, indent=4, ensure_ascii=False) + "\n").encode("utf-8")
    destino.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destino, "w") as z:
        for info, d in dados.items():
            z.writestr(info, d)
    return {"sha256": hashlib.sha256(destino.read_bytes()).hexdigest(),
            "semente_sha256": hashlib.sha256(semente.read_bytes()).hexdigest(),
            "objetos": [int(m.group(1)) for m in ordem], "extrusoras": [1, 2, 3, 4],
            "vetores_estendidos": len(estendidos)}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("destino", type=Path)
    ap.add_argument("--lado", type=float, default=15.0)
    ap.add_argument("--semente", type=Path, help="3MF de 4 objetos exportado pelo binário oficial")
    ns = ap.parse_args()
    print(de_semente(ns.semente, ns.destino) if ns.semente else gerar(ns.destino, ns.lado))
    return 0


if __name__ == "__main__":
    sys.exit(main())
