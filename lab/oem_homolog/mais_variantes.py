"""Reexecuta o OFICIAL de um caso da régua muitas vezes, para fabricantes cujo binário oficial
não é determinístico (Bambu), e confere camada a camada se o compilado coincide com alguma
saída do próprio oficial (critério IDENTICO_POR_CAMADA da régua).

Uso (no contêiner com as bibliotecas do AppImage e um X virtual):
  python3 mais_variantes.py <pasta-do-caso> <cubo|cores> <execucoes>
Grava <pasta-do-caso>/<modelo>-mais-variantes.json com o veredito.
"""
from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paridade import camadas, normalizado, sha  # noqa: E402


def main() -> int:
    caso, modelo, n = Path(sys.argv[1]), sys.argv[2], int(sys.argv[3])
    base = caso / f"{modelo}-oficial-A"
    cmd = json.loads((base / "cli.log").read_text(encoding="utf-8", errors="replace").splitlines()[0][4:])
    gcode_nome = next(p.name for p in base.glob("*.gcode"))
    comp = camadas(normalizado(next((caso / f"{modelo}-compilado").glob("*.gcode"))))
    por_camada: list[set[bytes]] = [set() for _ in comp]
    variantes: set[str] = set()
    for p in sorted(caso.glob(f"{modelo}-oficial-*")):
        for g in p.glob("*.gcode"):
            n_ = normalizado(g)
            variantes.add(sha(n_))
            for i, c in enumerate(camadas(n_)[: len(comp)]):
                por_camada[i].add(c)
    execucoes = 0
    for k in range(n):
        pasta = caso / f"{modelo}-oficial-extra"
        if pasta.exists():
            shutil.rmtree(pasta)
        pasta.mkdir()
        c = [a.replace(str(base), str(pasta)) for a in cmd]
        subprocess.run(c, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=900)
        execucoes += 1
        g = pasta / gcode_nome
        if not g.is_file():
            continue
        n_ = normalizado(g)
        variantes.add(sha(n_))
        cam = camadas(n_)
        for i, cc in enumerate(cam[: len(comp)]):
            por_camada[i].add(cc)
        faltam = [i for i, cc in enumerate(comp) if cc not in por_camada[i]]
        if not faltam:
            break
    faltam = [i for i, cc in enumerate(comp) if cc not in por_camada[i]]
    out = {"modelo": modelo, "execucoes_extras": execucoes, "variantes_oficiais": len(variantes),
           "camadas": len(comp), "camadas_sem_par_no_oficial": faltam,
           "veredito": "IDENTICO_POR_CAMADA" if not faltam else "DIVERGE"}
    (caso / f"{modelo}-mais-variantes.json").write_text(json.dumps(out, indent=1) + "\n", encoding="utf-8")
    print(json.dumps(out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
