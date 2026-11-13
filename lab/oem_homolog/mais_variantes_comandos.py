"""Como mais_variantes.py, mas no critério "comandos iguais" (criterio_comandos.py): reexecuta o OFICIAL
(e, se pedido, o compilado) e confere se cada camada do compilado tem os mesmos comandos de alguma saída
do oficial na mesma camada. Para oficiais muito variáveis (Creality K3).

Uso: python3 mais_variantes_comandos.py <pasta-do-caso> <cubo|cores> <execucoes-oficial> [execucoes-compilado]
Grava <pasta-do-caso>/<modelo>-mais-variantes-comandos.json.
"""
from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from criterio_comandos import camadas_cmd  # noqa: E402


def rodar(cmd: list[str], base: Path, pasta: Path, nome: str):
    if pasta.exists():
        shutil.rmtree(pasta)
    pasta.mkdir()
    subprocess.run([a.replace(str(base), str(pasta)) for a in cmd], stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL, timeout=1800)
    g = pasta / nome
    return camadas_cmd(g) if g.is_file() else None


def main() -> int:
    caso, modelo, n = Path(sys.argv[1]), sys.argv[2], int(sys.argv[3])
    nc = int(sys.argv[4]) if len(sys.argv) > 4 else 0
    base = caso / f"{modelo}-oficial-A"
    cmd = json.loads((base / "cli.log").read_text(encoding="utf-8", errors="replace").splitlines()[0][4:])
    nome = next(p.name for p in base.glob("*.gcode"))
    comp = camadas_cmd(next((caso / f"{modelo}-compilado").glob("*.gcode")))
    vistas = [set() for _ in comp]
    for p in sorted(caso.glob(f"{modelo}-oficial-*/*.gcode")):
        for k, c in enumerate(camadas_cmd(p)[: len(comp)]):
            vistas[k].add(c)

    def faltam(cam):
        return [k for k, c in enumerate(cam) if k >= len(vistas) or c not in vistas[k]]

    ex = 0
    while faltam(comp) and ex < n:
        cam = rodar(cmd, base, caso / f"{modelo}-oficial-extra", nome)
        ex += 1
        if cam:
            for k, c in enumerate(cam[: len(comp)]):
                vistas[k].add(c)
    exc = 0
    ok = not faltam(comp)
    if not ok and nc:
        base_c = caso / f"{modelo}-compilado"
        cmd_c = json.loads((base_c / "cli.log").read_text(encoding="utf-8", errors="replace").splitlines()[0][4:])
        while exc < nc and not ok:
            cam = rodar(cmd_c, base_c, caso / f"{modelo}-compilado-extra", nome)
            exc += 1
            ok = bool(cam) and len(cam) == len(comp) and not faltam(cam)
    out = {"modelo": modelo, "execucoes_oficial_extras": ex, "execucoes_compilado_extras": exc,
           "camadas": len(comp), "camadas_sem_par": [] if ok else faltam(comp),
           "veredito": "IDENTICO_POR_COMANDO" if ok else "DIVERGE"}
    (caso / f"{modelo}-mais-variantes-comandos.json").write_text(json.dumps(out, indent=1) + "\n", encoding="utf-8")
    print(json.dumps(out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
