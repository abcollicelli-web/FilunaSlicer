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


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("destino", type=Path)
    ap.add_argument("--lado", type=float, default=15.0)
    ns = ap.parse_args()
    print(gerar(ns.destino, ns.lado))
    return 0


if __name__ == "__main__":
    sys.exit(main())
