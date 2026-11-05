#!/usr/bin/env python3
"""Verificações estáticas da fundação Filuna. NÃO substitui QA de Windows/hardware."""
from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ERRORS: list[str] = []


def assert_true(condition: bool, message: str) -> None:
    if condition:
        print("PASS:", message)
    else:
        ERRORS.append(message)
        print("FAIL:", message)


def read_required(path: str) -> str:
    file = ROOT / path
    assert_true(file.is_file(), f"arquivo obrigatório existe: {path}")
    return file.read_text(encoding="utf-8") if file.is_file() else ""


def contract_checks() -> None:
    rule = read_required("FILUNA/REGRA-MESTRE-QUALIDADE.md")
    migration = read_required("FILUNA/MAPA-MIGRACAO-PERSONALIZACAO.md")
    readme = read_required("FILUNA/README.md")
    acceptance = read_required("FILUNA/TEST-ACCEPTANCE.md")
    architecture = read_required("FILUNA/ARCHITECTURE.md")
    source = read_required("src/libslic3r/libslic3r.h")
    read_required("src/libslic3r/Print.cpp")
    read_required("src/slic3r/GUI/MainFrame.cpp")

    for key in ("G0", "G1", "G2", "G3", "G4", "G5", "G6"):
        assert_true(key in rule and key in acceptance,
                    f"portão {key} presente na regra e no aceite")

    assert_true("NENHUMA ETAPA AVANÇA" in rule and "NOT RUN" in rule,
                "bloqueio por teste não executado e transparência de estados")
    assert_true("REGRA-MESTRE-QUALIDADE.md" in readme
                and "MAPA-MIGRACAO-PERSONALIZACAO.md" in readme,
                "README contém vínculos para as regras")
    assert_true("2026-11-03-ESTRUTURA-HOME-PREPARAR-CALIBRACAO.md" in migration
                and "18-HOME-GROK-2026-11-03.md" in migration,
                "Home aprovada e precedência documentadas")
    assert_true("56 px" in migration and "208 px" in migration,
                "dimensões de referência da Home registradas")
    assert_true("PROPOSTO" in migration and "HOMOLOG" in migration,
                "distinção entre proposta e homologação")
    assert_true("upstream-mirror" in architecture,
                "preservação do upstream registrada")
    assert_true("FILUNA_DATA_DIR_NAME" in source
                and re.search(r'FILUNA_DATA_DIR_NAME[^\n]*Filuna', source) is not None,
                "diretório Filuna preservado no código fonte")

    for vendor, path, model in (
        ("Elegoo", "resources/profiles/Elegoo/machine/ECC2/Elegoo Centauri Carbon 2 0.4 nozzle.json", "Elegoo Centauri Carbon 2"),
        ("Anycubic", "resources/profiles/Anycubic/machine/Anycubic Kobra 3 V2 0.4 nozzle.json", "Anycubic Kobra 3 V2"),
    ):
        file = ROOT / path
        assert_true(file.is_file(), f"perfil {vendor} existe")
        if file.is_file():
            try:
                data = json.loads(file.read_text(encoding="utf-8"))
                assert_true(data.get("printer_model") == model,
                            f"modelo real do perfil {vendor} confere")
            except (ValueError, OSError) as exc:
                assert_true(False, f"JSON {vendor} válido: {exc}")

    assert_true("- [ ]" in acceptance,
                "evidência de aceite físico permanece explicitamente pendente")
    assert_true("G4 — Windows real" in acceptance,
                "homologação física não é omitida")
    assert_true("G-code" in acceptance and "roll" in acceptance.lower(),
                "regressão de fatiamento e rollback permanecem no aceite")


def release_checks() -> None:
    # Geração de artefato técnico não significa liberação; arquivo ausente bloqueia.
    path = ROOT / "FILUNA/QA/release-evidence.json"
    if not path.is_file():
        assert_true(False,
                    "release bloqueado: FILUNA/QA/release-evidence.json não existe")
        return
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except (ValueError, OSError) as exc:
        assert_true(False, f"evidências de release legíveis: {exc}")
        return
    sha = doc.get("commit_sha", "")
    assert_true(bool(re.fullmatch(r"[0-9a-f]{40}", sha)),
                "release com commit SHA completo")
    assert_true(doc.get("owner_approval") is True
                and bool(doc.get("owner_approval_evidence")),
                "aprovação explícita do proprietário vinculada à versão/SHA")
    gates = doc.get("gates", {})
    for gate in ("G0", "G1", "G2", "G3", "G4", "G5", "G6"):
        val = gates.get(gate, {})
        assert_true(val.get("result") == "PASS" and bool(val.get("evidence_url")),
                    f"release: {gate} PASS com evidência")
    hardware = doc.get("hardware", {})
    for model in ("Elegoo Centauri Carbon 2", "Anycubic Kobra 3 V2"):
        val = hardware.get(model, {})
        assert_true(val.get("result") == "PASS"
                    and bool(val.get("firmware"))
                    and bool(val.get("evidence_url")),
                    f"release: validação física {model} comprovada")
    assert_true(doc.get("package", {}).get("signed") is True
                and bool(doc.get("package", {}).get("sha256"))
                and bool(doc.get("package", {}).get("rollback_evidence_url")),
                "pacote assinado, hash e rollback documentados")
    # Evidência documental também exige revisão humana externa ao script.


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=("foundation", "release"), default="foundation")
    args = parser.parse_args()
    contract_checks()
    if args.mode == "release":
        release_checks()
    print(f"Resumo: {len(ERRORS)} falha(s); modo={args.mode}")
    return 1 if ERRORS else 0


if __name__ == "__main__":
    sys.exit(main())
