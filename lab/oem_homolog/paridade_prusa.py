#!/usr/bin/env python3
"""Filuna — régua de paridade do PrusaSlicer (perfis em .ini, CLI diferente da família Orca).

Para cada impressora do pacote PrusaResearch.ini (seções [printer:...] instanciáveis), resolve a
herança 'inherits' do próprio .ini para achar o processo e o filamento padrão, e fatia o cubo no
binário oficial (várias execuções se ele variar) e no compilado com
  exe --datadir D --printer-profile P --print-profile Q --material-profile M --export-gcode -o out model
O pacote do fabricante é copiado para D/vendor, como o assistente de configuração faz. Mesmo
veredito da régua da família Orca: IDENTICO a uma variante oficial (só data/endereço normalizados).

paridade_prusa.py --oficial EXE --compilado EXE --pacote .../PrusaResearch.ini --modelo cube.stl --saida DIR
                  [--prefixo-oficial "flatpak run ..."] [--limite N] [--filtro "MK4S"]
"""
from __future__ import annotations

import argparse
import configparser
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bgcode  # noqa: E402
from paridade import normalizado, primeira_diferenca, sha  # noqa: E402


def ler_ini(p: Path) -> dict[str, dict[str, str]]:
    cp = configparser.RawConfigParser(strict=False, interpolation=None, delimiters=("=",), comment_prefixes=("#", ";"))
    cp.optionxform = str
    cp.read(p, encoding="utf-8")
    return {s: dict(cp[s]) for s in cp.sections()}


def efetivo(secoes: dict, tipo: str, nome: str, visto=None) -> dict:
    visto = visto or set()
    chave = f"{tipo}:{nome}"
    if chave in visto or chave not in secoes:
        return {}
    visto.add(chave)
    s = secoes[chave]
    out: dict = {}
    for pai in [x.strip() for x in s.get("inherits", "").split(";") if x.strip()]:
        out.update(efetivo(secoes, tipo, pai, visto))
    out.update({k: v for k, v in s.items() if k != "inherits"})
    return out


def impressoras(secoes: dict, filtro: str = "") -> list[dict]:
    out = []
    for s in secoes:
        if not s.startswith("printer:") or s.startswith("printer:*"):
            continue
        nome = s.split(":", 1)[1]
        if filtro and filtro not in nome:
            continue
        cfg = efetivo(secoes, "printer", nome)
        if not cfg.get("printer_model"):
            continue
        proc = cfg.get("default_print_profile", "")
        fil = cfg.get("default_filament_profile", "").split(";")[0].strip().strip('"')
        notas = []
        if f"print:{proc}" not in secoes:
            notas.append(f"PADRAO_OEM_AUSENTE print '{proc}'")
        if f"filament:{fil}" not in secoes:
            notas.append(f"PADRAO_OEM_AUSENTE filament '{fil}'")
        out.append({"maquina": nome, "processo": proc, "filamento": fil, "notas": notas,
                    "extrusoras": len(cfg.get("nozzle_diameter", "0.4").split(","))})
    return sorted(out, key=lambda x: x["maquina"])


def app_ini(secoes: dict, vendor: str) -> str:
    """PrusaSlicer.ini mínimo com todos os modelos do pacote instalados, como o assistente grava."""
    linhas = [f"[vendor:{vendor}]"]
    for s, d in secoes.items():
        if s.startswith("printer_model:"):
            var = d.get("variants", "")
            if var:
                # AppConfig::load só lê chaves "model:<id>" (MODEL_PREFIX); variantes em escape C, separadas por ';'
                vs = [v.strip() for v in var.split(";") if v.strip()]
                linhas.append(f"model:{s.split(':', 1)[1]} = " + ";".join(vs))
    return "\n".join(linhas) + "\n"


def fatiar(cmd_base: list[str], pacote: Path, imp: dict, modelo: Path, pasta: Path, timeout: int, ini_app: str = "",
           projeto: bool = False, exportar_3mf: Path | None = None, salvar_config: Path | None = None,
           carregar: Path | None = None, extra: list[str] | None = None) -> dict:
    if pasta.exists():
        shutil.rmtree(pasta)
    (pasta / "datadir" / "vendor").mkdir(parents=True)
    shutil.copy2(pacote, pasta / "datadir" / "vendor" / pacote.name)
    (pasta / "datadir" / "PrusaSlicer.ini").write_text(ini_app, encoding="utf-8")
    out = pasta / "saida.gcode"
    # projeto: o 3MF já traz a configuração inteira (gravada pelo oficial) e as extrusoras por objeto;
    # com perfis na linha de comando o PrusaSlicer carrega só a geometria e perde as extrusoras
    perfis = [] if projeto else ["--printer-profile", imp["maquina"], "--print-profile", imp["processo"],
                                 *sum((["--material-profile", imp["filamento"]] for _ in range(max(1, imp.get("extrusoras", 1)))), [])]
    if salvar_config:
        acao = ["--save", str(salvar_config)]
    elif exportar_3mf:
        acao = ["--export-3mf", "--output", str(exportar_3mf)]
    else:
        acao = ["--export-gcode", "--output", str(out)]
    cmd = [*cmd_base, "--datadir", str(pasta / "datadir"), *perfis, *(["--load", str(carregar)] if carregar else []), *(extra or []), *acao, str(modelo)]
    t0 = time.time()
    env = dict(os.environ, HOME=str(pasta / "home"))
    with (pasta / "cli.log").open("w", encoding="utf-8", errors="replace") as f:
        f.write("CMD=" + json.dumps(cmd, ensure_ascii=False) + "\n")
        f.flush()
        try:
            rc = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, timeout=timeout, env=env, check=False).returncode
        except subprocess.TimeoutExpired:
            rc = 124
    # morto por sinal: o subprocess dá -N quando o binário roda direto (compilado) e o shell do
    # oficial (Flatpak) devolve 128+N; os dois são a mesma queda
    if rc < 0:
        rc = 128 - rc
    r = {"rc": rc, "segundos": round(time.time() - t0, 1), "gcodes": [str(out)] if out.is_file() else []}
    # falta de recurso da máquina de teste não é recusa do fatiador: o caso não vale
    if "Resource temporarily unavailable" in (pasta / "cli.log").read_text(encoding="utf-8", errors="replace"):
        r["erro_ambiente"] = True
    return r


def compativeis(base_o: list[str], pacote: Path, ini_app: str, maquina: str, pasta: Path, timeout: int) -> dict[str, list[str]]:
    """Processos e filamentos que o PrusaSlicer OFICIAL dá como compatíveis com a impressora
    (--query-print-filament-profiles), para escolher como o assistente da GUI escolheria."""
    if pasta.exists():
        shutil.rmtree(pasta)
    (pasta / "datadir" / "vendor").mkdir(parents=True)
    shutil.copy2(pacote, pasta / "datadir" / "vendor" / pacote.name)
    (pasta / "datadir" / "PrusaSlicer.ini").write_text(ini_app, encoding="utf-8")
    saida = pasta / "compat.json"
    try:
        subprocess.run([*base_o, "--datadir", str(pasta / "datadir"), "--query-print-filament-profiles",
                        "--printer-profile", maquina, "--output", str(saida)],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=timeout)
        d = json.loads(saida.read_text(encoding="utf-8"))
    except (OSError, ValueError, subprocess.TimeoutExpired):
        return {}
    return {p["name"]: list(p.get("filament_profiles", [])) for p in d.get("print_profiles", [])}


def ajustar(imp: dict, compat: dict[str, list[str]]) -> None:
    """Padrão do fabricante quando compatível; senão o compatível de mesmo nome-base (antes do '@'),
    senão o primeiro em ordem alfabética. A troca fica registrada em 'ajustes'."""
    if not compat:
        return
    aj = []
    if imp["processo"] not in compat:
        base = imp["processo"].split("@")[0].strip()
        novo = next((p for p in sorted(compat) if p.split("@")[0].strip() == base), sorted(compat)[0])
        aj.append(f"PADRAO_OEM_INCOMPATIVEL print '{imp['processo']}' -> '{novo}'")
        imp["processo"] = novo
    fils = compat[imp["processo"]]
    if fils and imp["filamento"] not in fils:
        base = imp["filamento"].split("@")[0].strip()
        novo = next((f for f in sorted(fils) if f.split("@")[0].strip() == base), sorted(fils)[0])
        aj.append(f"PADRAO_OEM_INCOMPATIVEL filament '{imp['filamento']}' -> '{novo}'")
        imp["filamento"] = novo
    if aj:
        imp["ajustes"] = aj


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--oficial", required=True)
    ap.add_argument("--compilado", required=True)
    ap.add_argument("--prefixo-oficial", default="")
    ap.add_argument("--pacote", type=Path, required=True)
    ap.add_argument("--modelo", type=Path, required=True)
    ap.add_argument("--cores", action="store_true", help="caso de 4 cores nas impressoras com 4+ extrusoras (MMU3, XL 5T)")
    ap.add_argument("--saida", type=Path, required=True)
    ap.add_argument("--limite", type=int, default=0)
    ap.add_argument("--filtro", default="")
    ap.add_argument("--timeout", type=int, default=300)
    ap.add_argument("--max-oficial", type=int, default=6)
    ns = ap.parse_args()
    ns.saida.mkdir(parents=True, exist_ok=True)
    secoes = ler_ini(ns.pacote)
    imps = impressoras(secoes, ns.filtro)
    if ns.limite:
        imps = imps[: ns.limite]
    ini_app = app_ini(secoes, ns.pacote.stem)
    base_o = [*shlex.split(ns.prefixo_oficial), ns.oficial]
    base_c = [ns.compilado]
    resultados = []
    def caso_um(imp, caso, nome, modelo, projeto=False, carregar=None, extra=None):
        variantes, execs = {}, []
        for k in range(2):
            x = fatiar(base_o, ns.pacote, imp, modelo, caso / f"{nome}-oficial-{k}", ns.timeout, ini_app, projeto, carregar=carregar, extra=extra)
            execs.append(x)
            if x["rc"] == 0 and x["gcodes"]:
                n = normalizado(Path(x["gcodes"][0]))
                variantes.setdefault(sha(n), n)
        k = 2
        while len(variantes) > 1 and k < ns.max_oficial:
            x = fatiar(base_o, ns.pacote, imp, modelo, caso / f"{nome}-oficial-{k}", ns.timeout, ini_app, projeto, carregar=carregar, extra=extra)
            if x["rc"] == 0 and x["gcodes"]:
                n = normalizado(Path(x["gcodes"][0]))
                variantes.setdefault(sha(n), n)
            k += 1
        cc = fatiar(base_c, ns.pacote, imp, modelo, caso / f"{nome}-compilado", ns.timeout, ini_app, projeto, carregar=carregar, extra=extra)
        c = {"modelo": nome, "oficial": execs[0], "compilado": cc, "variantes_oficiais": len(variantes)}
        if any(x.get("erro_ambiente") for x in (*execs, cc)):
            c["veredito"] = "ERRO_DE_AMBIENTE"
        elif not variantes and cc["rc"] != 0 and execs[0]["rc"] == cc["rc"]:
            c["veredito"] = "MESMA_RECUSA_DO_OFICIAL"
        elif not variantes:
            c["veredito"] = "OFICIAL_NAO_FATIOU"
        elif cc["rc"] != 0 or not cc["gcodes"]:
            c["veredito"] = "COMPILADO_NAO_FATIOU"
        else:
            nc = normalizado(Path(cc["gcodes"][0]))
            c["sha_compilado"], c["sha_oficial"] = sha(nc), sorted(variantes)
            c["trocas_ferramenta"] = len(re.findall(rb"^T\d+\b", next(iter(variantes.values())), re.M))
            if sha(nc) in variantes:
                c["veredito"] = "IDENTICO"
            else:
                c["veredito"] = "DIVERGE"
                c["primeira_diferenca"] = primeira_diferenca(nc, next(iter(variantes.values())))
        return c

    for i, imp in enumerate(imps, 1):
        caso = ns.saida / "casos" / f"{i:03d}"
        r = dict(imp)
        if imp["notas"]:
            r["veredito"] = "PERFIL_INCOMPLETO"
        else:
            ajustar(imp, compativeis(base_o, ns.pacote, ini_app, imp["maquina"], caso / "compat", ns.timeout))
            r = dict(imp)
            r["casos"] = [caso_um(imp, caso, "cubo", ns.modelo)]
            if ns.cores and imp.get("extrusoras", 1) >= 4:
                # semente: o PrusaSlicer OFICIAL exporta o projeto desta impressora; viram 4 objetos nas extrusoras 1..4
                from fixture_cores import de_semente_prusa
                sem = caso / "semente.3mf"
                fatiar(base_o, ns.pacote, imp, ns.modelo, caso / "semente", ns.timeout, ini_app, exportar_3mf=sem)
                # configuração efetiva desta impressora = bloco "; prusaslicer_config" do G-code do cubo
                # que o oficial acabou de fatiar com os perfis (o --save da CLI grava só os padrões)
                cfg = caso / "projeto.ini"
                g = caso / "cubo-oficial-0" / "saida.gcode"
                if g.is_file() and bgcode.eh_bgcode(g.read_bytes()[:4]):
                    # binário: a configuração efetiva está no bloco de metadados do fatiador
                    cfg.write_text("".join(f"{k} = {v}\n" for k, v in bgcode.metadados(g.read_bytes()).items()), encoding="utf-8")
                elif g.is_file():
                    bloco, dentro = [], False
                    for l in g.read_text(encoding="utf-8", errors="replace").splitlines():
                        if l.startswith("; prusaslicer_config = begin"):
                            dentro = True
                            continue
                        if l.startswith("; prusaslicer_config = end"):
                            break
                        if dentro and l.startswith("; "):
                            bloco.append(l[2:])
                    cfg.write_text("\n".join(bloco) + "\n", encoding="utf-8")
                try:
                    de_semente_prusa(sem, caso / "cores.3mf")
                    # a configuração completa salva pelo oficial entra por --load; o 3MF continua projeto,
                    # então as extrusoras por objeto são mantidas
                    r["casos"].append(caso_um(imp, caso, "cores", caso / "cores.3mf", projeto=True, carregar=cfg))
                    # A CLI oficial 2.9.7 cai (SIGSEGV, rc 139) ao fatiar várias extrusoras COM torre de limpeza;
                    # o caso acima registra a mesma queda. A troca de cores (T, rampa, descarga/carga da MMU,
                    # G-code personalizado de troca) é provada sem a torre, com a mesma configuração.
                    r["casos"].append(caso_um(imp, caso, "cores-sem-torre", caso / "cores.3mf", projeto=True, carregar=cfg,
                                              extra=["--no-wipe-tower"]))
                except (OSError, AssertionError, KeyError, ValueError) as e:
                    r["casos"].append({"modelo": "cores", "veredito": "OFICIAL_NAO_FATIOU", "detalhe": f"semente: {e}"})
            vs = [c["veredito"] for c in r["casos"]]
            aceitos = ("IDENTICO", "MESMA_RECUSA_DO_OFICIAL")
            r["veredito"] = "IDENTICO" if all(v == "IDENTICO" for v in vs) else next((v for v in vs if v not in aceitos), "IDENTICO_COM_RECUSA_OFICIAL")
        resultados.append(r)
        print(f"[{i}/{len(imps)}] {r['veredito']:22} {imp['maquina']}", flush=True)
    resumo = {}
    for r in resultados:
        resumo[r["veredito"]] = resumo.get(r["veredito"], 0) + 1
    prova = {"id": "prusa", "oficial": ns.oficial, "compilado": ns.compilado, "impressoras": len(imps), "resumo": resumo,
             "resultados": resultados}
    (ns.saida / "prova.json").write_text(json.dumps(prova, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print("RESUMO " + json.dumps(resumo))
    return 0


if __name__ == "__main__":
    sys.exit(main())
