#!/usr/bin/env python3
"""Publica uma versão do Filuna Slicer para a atualização automática — só o que mudou.

Cada versão publicada vira um "build": a lista de arquivos com SHA-256 (builds/<id>/manifesto.json) e os
arquivos guardados uma vez só, pelo conteúdo (blobs/<sha>). Para cada build anterior é gerado um pacote
só com os arquivos diferentes (pacotes/<de>__<para>.zip) e um documento assinado (docs/<de>__<para>.json
+ .sig, Ed25519, mesma chave do catálogo de módulos). Quem tem um build desconhecido recebe o pacote completo.

Três jeitos de publicar:
  --origem PASTA        pasta do programa compilado (a que tem FilunaSlicer.exe)
  --run ID              baixa o artefato "-exe" desse run do GitHub Actions
  --base BUILD --sobrepor PASTA
                        NÃO compila nada: parte do build BASE e troca/acrescenta os arquivos de PASTA
                        (mesma estrutura do programa, ex.: resources/web/filuna/dispositivo.html).
                        É a correção rápida de tela, texto, perfil e dado.

  python3 publicar_app.py --run 123 --build 440d178 --versao 2.5.0-beta.3 --nota "Conexão da Elegoo corrigida" \
      --interno "update de recursos internos" --canal beta

Várias sessões publicam no mesmo servidor. Para uma publicação nunca derrubar a outra (regra de 15/11/2026):
  - TRAVA: só uma publicação por vez (<destino>/.publicar.lock). A segunda para na hora e diz quem está publicando.
  - SEM RETROCESSO: o build novo tem de conter o commit que já está no canal (comparação no GitHub). Se o canal
    está em um commit que o build novo não tem, a publicação é recusada: junte os commits e compile de novo.
  - VERSÃO SÓ SOBE: a versão nova tem de ser maior que a do canal e não pode já existir com outro build.
  - REGISTRO: toda publicação fica em <destino>/publicacoes.log (uma linha JSON por publicação).
  --forcar-retrocesso passa por cima das duas checagens; só usar com ordem expressa do dono.
"""
from __future__ import annotations
import argparse, base64, datetime, fcntl, hashlib, json, os, re, shutil, subprocess, sys, tempfile, zipfile
from pathlib import Path

def sha(p: Path) -> str:
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()

def seguro(s: str) -> bool:
    return bool(s) and len(s) <= 80 and all(c.isalnum() or c in "-._" for c in s)

def raiz_do_programa(p: Path) -> Path:
    """A pasta que contém o executável (o artefato pode trazer um nível a mais)."""
    for cand in [p] + sorted(x for x in p.rglob("*") if x.is_dir()):
        if any((cand / n).is_file() for n in ("FilunaSlicer.exe", "filuna-slicer.exe", "OrcaSlicer.exe")):
            return cand
    return p

def zipar(destino: Path, arquivos: dict[str, Path]):
    """Pasta ANTES dos arquivos dela: o extrator do Filuna só cria pasta a partir da entrada de pasta."""
    pastas = set()
    for rel in arquivos:
        partes = rel.split("/")
        for k in range(1, len(partes)):
            pastas.add("/".join(partes[:k]) + "/")
    with zipfile.ZipFile(destino, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for p in sorted(pastas):
            zi = zipfile.ZipInfo(p, date_time=(2026, 1, 1, 0, 0, 0)); zi.external_attr = 0x10
            z.writestr(zi, b"")
        for rel in sorted(arquivos):
            z.write(arquivos[rel], rel)

def commit_do_build(build: str) -> str:
    """O id do build é o commit abreviado, às vezes com -rN (republicação por --base)."""
    return re.sub(r"-r\d+$", "", build)

def chave_versao(v: str) -> tuple:
    """2.5.0-beta.64 -> (2, 5, 0, 0, 64); versão sem -beta fica acima de qualquer beta dela."""
    m = re.match(r"^(\d+)\.(\d+)\.(\d+)(?:-beta\.(\d+))?$", v)
    if not m:
        return ()
    a, b, c, beta = m.groups()
    return (int(a), int(b), int(c), 0 if beta is not None else 1, int(beta or 0))

def conferir_canal(D: Path, ns) -> str | None:
    """Devolve o motivo da recusa, ou None se pode publicar. Roda já com a trava tomada."""
    if ns.canal == "nenhum":
        return None
    cf = D / "canal.json"
    atual = (json.loads(cf.read_text()) if cf.is_file() else {}).get(ns.canal, "")
    if not atual or atual == ns.build:
        return None
    # versão
    mv = D / "builds" / atual / "manifesto.json"
    versao_atual = json.loads(mv.read_text()).get("versao", "") if mv.is_file() else ""
    if not chave_versao(ns.versao):
        return f"versão {ns.versao} fora do formato 2.5.0 ou 2.5.0-beta.N"
    if versao_atual and chave_versao(versao_atual) and chave_versao(ns.versao) <= chave_versao(versao_atual):
        return f"o canal {ns.canal} já está na {versao_atual} (build {atual}); a versão nova tem de ser maior que ela"
    for p in (D / "builds").iterdir():
        m = p / "manifesto.json"
        if p.name != ns.build and m.is_file() and json.loads(m.read_text()).get("versao") == ns.versao:
            return f"a versão {ns.versao} já foi publicada com o build {p.name}"
    # sem retrocesso: o commit novo precisa conter o commit do canal
    de, para = commit_do_build(atual), commit_do_build(ns.build)
    if de == para:
        return None
    r = subprocess.run(["gh", "api", f"repos/{ns.repo_gh}/compare/{de}...{para}", "--jq", ".status"],
                       capture_output=True, text=True)
    st = r.stdout.strip()
    if r.returncode != 0 or st not in ("ahead", "identical"):
        return (f"o canal {ns.canal} está no build {atual} (versão {versao_atual or '?'}) e o build {ns.build} "
                f"{'não contém esse commit' if st in ('behind', 'diverged') else 'não pôde ser comparado (' + (r.stderr.strip() or st) + ')'}. "
                "Publicar assim apagaria do canal o que outra sessão publicou: junte os commits no ramo, compile de novo e publique esse build.")
    return None

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--origem", type=Path); ap.add_argument("--run")
    ap.add_argument("--base"); ap.add_argument("--sobrepor", type=Path)
    ap.add_argument("--repo-gh", default="abcollicelli-web/FilunaSlicer")
    ap.add_argument("--build", required=True); ap.add_argument("--versao", required=True)
    ap.add_argument("--nota", action="append", default=[]); ap.add_argument("--interno", action="append", default=[])
    ap.add_argument("--canal", default="beta", choices=["beta", "todos", "nenhum"])
    ap.add_argument("--destino", type=Path, default=Path("/var/lib/filuna-v2/app"))
    ap.add_argument("--chave", type=Path, default=Path("/root/.filuna-modulos/ed25519.pem"))
    ap.add_argument("--deltas", type=int, default=8, help="quantos builds anteriores recebem pacote só do que mudou")
    ap.add_argument("--patches", type=int, default=3, help="quantos builds anteriores recebem também a versão com diferença binária (zstd)")
    ap.add_argument("--forcar-retrocesso", action="store_true", help="ignora as checagens de versão e de commit (só com ordem do dono)")
    ns = ap.parse_args()
    if not seguro(ns.build):
        print("build inválido"); return 2
    if ns.canal == "todos":
        print("RECUSADO: o canal de todos os clientes é liberado pelo admin do site (/admin/atualizacoes), com uma versão "
              "que já passou pelo beta. Publique com --canal beta (ou nenhum)."); return 2
    D = ns.destino
    for s in ("builds", "blobs", "docs", "pacotes"):
        (D / s).mkdir(parents=True, exist_ok=True)
    # trava: uma publicação por vez, em qualquer sessão
    trava = open(D / ".publicar.lock", "a+")
    try:
        fcntl.flock(trava, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        trava.seek(0)
        print("RECUSADO: outra publicação está em andamento: " + (trava.read().strip() or "?") + ". Espere ela terminar e confira o canal de novo.")
        return 3
    trava.seek(0); trava.truncate()
    trava.write(f"pid {os.getpid()}, build {ns.build}, versão {ns.versao}, canal {ns.canal}, desde "
                f"{datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')}"); trava.flush()
    motivo = conferir_canal(D, ns)
    if motivo and not ns.forcar_retrocesso:
        print("RECUSADO: " + motivo); return 4
    if motivo:
        print("AVISO (--forcar-retrocesso): " + motivo)
    cf_antes = D / "canal.json"
    anterior = (json.loads(cf_antes.read_text()) if cf_antes.is_file() else {}).get(ns.canal, "")
    tmp = Path(tempfile.mkdtemp(prefix="filuna-app-", dir=str(D)))
    try:
        arquivos: dict[str, Path] = {}   # caminho relativo -> arquivo de origem
        if ns.base:
            if not ns.sobrepor or not ns.sobrepor.is_dir():
                print("--base pede --sobrepor PASTA"); return 2
            man = json.loads((D / "builds" / ns.base / "manifesto.json").read_text())
            for rel, info in man["arquivos"].items():
                arquivos[rel] = D / "blobs" / info["sha256"]
            n = 0
            for f in sorted(ns.sobrepor.rglob("*")):
                if f.is_file():
                    arquivos[f.relative_to(ns.sobrepor).as_posix()] = f; n += 1
            print(f"base {ns.base}: {len(man['arquivos'])} arquivos; sobrepostos {n}")
        else:
            origem = ns.origem
            if ns.run:
                baixar = tmp / "artefato"; baixar.mkdir()
                subprocess.run(["gh", "run", "download", str(ns.run), "-R", ns.repo_gh, "-p", "*-exe-*", "-D", str(baixar)], check=True)
                origem = baixar
            if not origem or not origem.is_dir():
                print("informe --origem, --run ou --base/--sobrepor"); return 2
            origem = raiz_do_programa(origem)
            for f in sorted(origem.rglob("*")):
                if f.is_file():
                    arquivos[f.relative_to(origem).as_posix()] = f
            print(f"origem {origem}: {len(arquivos)} arquivos")
        # identidade do build dentro do próprio programa (o cliente lê para saber o que tem instalado)
        ident = tmp / "filuna-build.json"
        ident.write_text(json.dumps({"build": ns.build, "versao": ns.versao}, ensure_ascii=False), encoding="utf-8")
        arquivos["filuna-build.json"] = ident
        # guarda os arquivos pelo conteúdo e escreve o manifesto
        man = {"build": ns.build, "versao": ns.versao, "arquivos": {}}
        for rel, f in arquivos.items():
            h = sha(f); blob = D / "blobs" / h
            if not blob.is_file():
                shutil.copyfile(f, str(blob) + ".tmp"); os.replace(str(blob) + ".tmp", blob)
            man["arquivos"][rel] = {"sha256": h, "bytes": blob.stat().st_size}
        (D / "builds" / ns.build).mkdir(exist_ok=True)
        (D / "builds" / ns.build / "manifesto.json").write_text(json.dumps(man, ensure_ascii=False), encoding="utf-8")
        agora = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        (D / "builds" / ns.build / "publicado_em").write_text(agora)

        def documento(nome: str, de: str, escolhidos: dict[str, Path], remover: list[str], tipo: str, patches: dict | None = None):
            pacote = D / "pacotes" / f"{nome}.zip"
            zipar(pacote, escolhidos)
            doc = {"schema": 1, "de": de, "para": ns.build, "versao": ns.versao, "tipo": tipo, "publicado_em": agora,
                   "notas": ns.nota, "internos": ns.interno, "arquivos": len(escolhidos), "remover": remover,
                   "pacote": {"arquivo": pacote.name, "sha256": sha(pacote), "bytes": pacote.stat().st_size}}
            if patches:
                doc["patches"] = patches
            texto = json.dumps(doc, ensure_ascii=False, sort_keys=True)
            (D / "docs" / f"{nome}.json").write_text(texto, encoding="utf-8")
            assin = subprocess.run(["openssl", "pkeyutl", "-sign", "-inkey", str(ns.chave), "-rawin", "-in",
                                    str(D / "docs" / f"{nome}.json")], check=True, capture_output=True).stdout
            (D / "docs" / f"{nome}.json.sig").write_text(base64.b64encode(assin).decode() + "\n", encoding="ascii")
            print(f"  {nome}: {len(escolhidos)} arquivos, {pacote.stat().st_size / 1048576:.1f} MB, remover {len(remover)}")

        blobs = {rel: D / "blobs" / i["sha256"] for rel, i in man["arquivos"].items()}
        documento(f"completo__{ns.build}", "*", blobs, [], "completo")
        anteriores = sorted((p for p in (D / "builds").iterdir() if p.is_dir() and p.name != ns.build and (p / "manifesto.json").is_file()),
                            key=lambda p: (p / "publicado_em").read_text() if (p / "publicado_em").is_file() else "", reverse=True)
        zp_tarefas = []
        for k, p in enumerate(anteriores[: ns.deltas]):
            velho = json.loads((p / "manifesto.json").read_text())["arquivos"]
            mudou = {rel: blobs[rel] for rel, i in man["arquivos"].items() if velho.get(rel, {}).get("sha256") != i["sha256"]}
            remover = sorted(rel for rel in velho if rel not in man["arquivos"])
            documento(f"{p.name}__{ns.build}", p.name, mudou, remover, "delta")
            if k < ns.patches:
                zp_tarefas.append((p.name, velho, mudou, remover))
        # Versão menor do mesmo pacote para os programas que sabem aplicar diferença binária (a partir da 2.5.0-beta.67,
        # pedem ?patch=zstd): arquivo grande que mudou vai como diferença zstd contra a versão que o cliente já tem.
        # Quem não pede continua recebendo o pacote normal acima.
        if zp_tarefas and shutil.which("zstd"):
            import concurrent.futures
            pz = tmp / "zp"; pz.mkdir(exist_ok=True)
            def fazer_patch(de: str, rel: str, base_sha: str):
                saida = pz / f"{de}__{rel.replace('/', '_')}.zpatch"
                r = subprocess.run(["zstd", "-q", "-f", "-19", "--long=27", "-T0", f"--patch-from={D / 'blobs' / base_sha}",
                                    str(blobs[rel]), "-o", str(saida)], capture_output=True)
                return saida if r.returncode == 0 and saida.is_file() else None
            pedidos = {}
            with concurrent.futures.ThreadPoolExecutor(max_workers=3) as ex:
                for de, velho, mudou, _ in zp_tarefas:
                    for rel in mudou:
                        base = velho.get(rel, {}).get("sha256", "")
                        if base and (D / "blobs" / base).is_file() and man["arquivos"][rel]["bytes"] >= 2 * 1048576:
                            pedidos[(de, rel)] = ex.submit(fazer_patch, de, rel, base)
            for de, velho, mudou, remover in zp_tarefas:
                escolhidos, patches = dict(mudou), {}
                for rel in list(mudou):
                    f = pedidos.get((de, rel))
                    saida = f.result() if f else None
                    if saida and saida.stat().st_size < man["arquivos"][rel]["bytes"] * 0.5:
                        del escolhidos[rel]
                        escolhidos[rel + ".zpatch"] = saida
                        patches[rel] = {"base_sha256": velho[rel]["sha256"], "sha256": man["arquivos"][rel]["sha256"],
                                        "bytes": man["arquivos"][rel]["bytes"]}
                if patches:
                    documento(f"{de}__{ns.build}.zp", de, escolhidos, remover, "delta-zstd", patches)
        if ns.canal != "nenhum":
            cf = D / "canal.json"
            canal = json.loads(cf.read_text()) if cf.is_file() else {}
            canal[ns.canal] = ns.build
            (D / "canal.json.tmp").write_text(json.dumps(canal, ensure_ascii=False), encoding="utf-8")
            os.replace(D / "canal.json.tmp", cf)
            print(f"canal {ns.canal} -> {ns.build}")
        with open(D / "publicacoes.log", "a", encoding="utf-8") as lg:
            lg.write(json.dumps({"em": agora, "build": ns.build, "versao": ns.versao, "canal": ns.canal, "anterior": anterior,
                                 "run": ns.run or "", "base": ns.base or "", "forcado": bool(motivo), "notas": ns.nota},
                                ensure_ascii=False) + "\n")
        subprocess.run(["chmod", "-R", "a+rX", str(D)], check=False)
        return 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

if __name__ == "__main__":
    sys.exit(main())
