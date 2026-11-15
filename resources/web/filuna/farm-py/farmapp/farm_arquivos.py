"""Abre o que o cliente envia para a Farm (ZIP, RAR, 3MF, STL) e descreve as peças.

Para cada peça: nome, medidas (mm), volume, cores lidas do arquivo e uma foto (PNG) desenhada a partir da malha.
Somente biblioteca padrão. Nada do arquivo é executado; tudo o que vem do cliente é conferido.
"""
from __future__ import annotations

import json
import math
import os
import re
import shutil
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zipfile
import zlib
from io import BytesIO
from pathlib import Path

from .fatiamento import motor

UNAR = os.environ.get("FILUNA_UNAR_BIN", "/usr/bin/unar")
MAX_MODELOS = 60            # arquivos de modelo dentro de um ZIP/RAR
MAX_TRIANGULOS = 4_000_000  # por envio; acima disso a foto e o volume são pulados
MAX_FOTO = 250_000          # triângulos usados no desenho da foto
LADO_FOTO = 200

Tri = tuple  # (x1,y1,z1,x2,y2,z2,x3,y3,z3)


class ErroArquivo(Exception):
    """Erro com mensagem que pode ser mostrada ao cliente."""


# ---------- pacotes ----------
def abrir(nome: str, dados: bytes) -> list[tuple[str, bytes]]:
    """Devolve os modelos (nome, bytes) de um envio: o próprio STL/3MF ou o que há dentro do ZIP/RAR."""
    if len(dados) > motor.LIMITE_BYTES:
        raise ErroArquivo(f"O arquivo passa de {motor.LIMITE_BYTES // 1024 // 1024} MB.")
    ext = Path(nome or "").suffix.lower()
    if ext in (".stl", ".3mf"):
        return [(Path(nome).name[-120:], dados)]
    if ext == ".zip":
        modelos = _abrir_zip(dados)
    elif ext == ".rar":
        modelos = _abrir_rar(dados)
    else:
        raise ErroArquivo("Envie um arquivo .zip, .rar, .3mf ou .stl.")
    if not modelos:
        raise ErroArquivo("Não há nenhum arquivo .stl ou .3mf dentro desse pacote.")
    vistos: dict[str, int] = {}
    saida = []
    for arq, conteudo in modelos:
        n = vistos.get(arq.lower(), 0)
        vistos[arq.lower()] = n + 1
        saida.append((f"{Path(arq).stem}-{n + 1}{Path(arq).suffix}" if n else arq, conteudo))
    return saida


def _serve(caminho: str) -> bool:
    p = Path(caminho)
    return p.suffix.lower() in (".stl", ".3mf") and not p.name.startswith(".") and "__MACOSX" not in p.parts


def _abrir_zip(dados: bytes) -> list[tuple[str, bytes]]:
    if dados[:4] != b"PK\x03\x04":
        raise ErroArquivo("Esse ZIP não é válido.")
    try:
        z = zipfile.ZipFile(BytesIO(dados))
    except zipfile.BadZipFile as exc:
        raise ErroArquivo("Esse ZIP não é válido.") from exc
    with z:
        itens = [i for i in z.infolist() if not i.is_dir() and _serve(i.filename)]
        if len(itens) > MAX_MODELOS:
            raise ErroArquivo(f"O pacote tem mais de {MAX_MODELOS} modelos. Envie em partes.")
        if sum(i.file_size for i in itens) > motor.LIMITE_DESCOMPACTADO:
            raise ErroArquivo("O pacote é grande demais depois de descompactado.")
        saida = []
        for i in itens:
            if i.flag_bits & 0x1:
                raise ErroArquivo("O pacote tem senha. Envie sem senha.")
            saida.append((Path(i.filename).name[-120:], z.read(i)))
        return saida


def _abrir_rar(dados: bytes) -> list[tuple[str, bytes]]:
    if dados[:7] not in (b"Rar!\x1a\x07\x00", b"Rar!\x1a\x07\x01"):
        raise ErroArquivo("Esse RAR não é válido.")
    if not Path(UNAR).exists():
        raise ErroArquivo("O servidor não consegue abrir RAR agora. Envie em ZIP.")
    motor.TRABALHO.mkdir(parents=True, exist_ok=True)
    pasta = Path(tempfile.mkdtemp(prefix="r-", dir=motor.TRABALHO))
    try:
        (pasta / "envio.rar").write_bytes(dados)
        destino = pasta / "fora"
        destino.mkdir()
        try:
            r = subprocess.run([UNAR, "-quiet", "-no-directory", "-force-overwrite", "-output-directory", str(destino),
                                str(pasta / "envio.rar")], env={"PATH": "/usr/bin:/bin", "HOME": str(pasta), "LC_ALL": "C.UTF-8"},
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, stdin=subprocess.DEVNULL,
                               timeout=60, preexec_fn=_limites_rar, check=False)
        except subprocess.TimeoutExpired as exc:
            raise ErroArquivo("O RAR demorou demais para abrir. Envie em ZIP.") from exc
        if r.returncode != 0:
            raise ErroArquivo("Não foi possível abrir esse RAR (com senha ou danificado). Envie em ZIP.")
        saida, total = [], 0
        for arq in sorted(destino.rglob("*")):
            if arq.is_symlink() or not arq.is_file() or not _serve(str(arq.relative_to(destino))):
                continue
            total += arq.stat().st_size
            if len(saida) >= MAX_MODELOS or total > motor.LIMITE_DESCOMPACTADO:
                raise ErroArquivo("O pacote é grande demais. Envie em partes.")
            saida.append((arq.name[-120:], arq.read_bytes()))
        return saida
    finally:
        shutil.rmtree(pasta, ignore_errors=True)


def _limites_rar():
    import resource
    os.setsid()
    resource.setrlimit(resource.RLIMIT_FSIZE, (2 << 30, 2 << 30))
    resource.setrlimit(resource.RLIMIT_CPU, (120, 120))
    os.nice(10)


# ---------- STL ----------
def ler_stl(dados: bytes) -> list[Tri]:
    if len(dados) >= 84:
        n = struct.unpack_from("<I", dados, 80)[0]
        if n > 0 and 84 + n * 50 == len(dados):
            return [t[3:12] for t in struct.iter_unpack("<12fH", dados[84:])]
    texto = dados.decode("ascii", "ignore")
    nums = re.findall(r"vertex\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)", texto)
    try:
        v = [float(x) for trio in nums for x in trio]
    except ValueError as exc:
        raise ErroArquivo("Esse STL não é válido.") from exc
    return [tuple(v[i:i + 9]) for i in range(0, len(v) - 8, 9)]


# ---------- 3MF ----------
def _local(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def _attr(el, nome: str):
    for k, v in el.attrib.items():
        if _local(k) == nome:
            return v
    return None


def _matriz(texto: str | None):
    if not texto:
        return None
    try:
        m = [float(x) for x in texto.split()]
    except ValueError:
        return None
    return m if len(m) == 12 else None


def _aplica(m, x, y, z):
    return (x * m[0] + y * m[3] + z * m[6] + m[9], x * m[1] + y * m[4] + z * m[7] + m[10],
            x * m[2] + y * m[5] + z * m[8] + m[11])


def _cor_hex(c: str | None) -> str | None:
    if not c:
        return None
    c = c.strip()
    if re.fullmatch(r"#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?", c):
        return c[:7].upper()
    return None


def _ler_modelo(z: zipfile.ZipFile, caminho: str, cache: dict) -> dict:
    """Um arquivo .model: objetos (malha ou componentes), materiais e itens de montagem."""
    caminho = caminho.lstrip("/")
    if caminho in cache:
        return cache[caminho]
    doc = {"objetos": {}, "itens": [], "cores": {}, "pintado": False}
    cache[caminho] = doc
    try:
        fonte = z.open(caminho)
    except KeyError:
        return doc
    obj = None
    with fonte:
        for evento, el in ET.iterparse(fonte, events=("start", "end")):
            nome = _local(el.tag)
            if evento == "start":
                if nome == "object":
                    obj = {"id": el.get("id"), "nome": el.get("name") or "", "pid": el.get("pid"),
                           "pindex": el.get("pindex"), "v": [], "t": [], "comp": []}
                continue
            if nome == "vertex" and obj is not None:
                try:
                    obj["v"].append((float(el.get("x")), float(el.get("y")), float(el.get("z"))))
                except (TypeError, ValueError):
                    obj["v"].append((0.0, 0.0, 0.0))
                el.clear()
            elif nome == "triangle" and obj is not None:
                try:
                    obj["t"].append((int(el.get("v1")), int(el.get("v2")), int(el.get("v3"))))
                except (TypeError, ValueError):
                    pass
                if not doc["pintado"] and any(_local(k) in ("paint_color", "mmu_segmentation") for k in el.attrib):
                    doc["pintado"] = True
                el.clear()
            elif nome == "component" and obj is not None:
                obj["comp"].append({"id": el.get("objectid"), "caminho": _attr(el, "path"),
                                    "m": _matriz(el.get("transform"))})
            elif nome == "object" and obj is not None:
                doc["objetos"][obj["id"]] = obj
                obj = None
            elif nome == "basematerials":
                doc["cores"][el.get("id")] = [_cor_hex(b.get("displaycolor")) for b in el if _local(b.tag) == "base"]
            elif nome == "colorgroup":
                doc["cores"][el.get("id")] = [_cor_hex(b.get("color")) for b in el if _local(b.tag) == "color"]
            elif nome == "item":
                doc["itens"].append({"id": el.get("objectid"), "m": _matriz(el.get("transform")),
                                     "imprimivel": el.get("printable", "1") != "0"})
    return doc


def _config_bambu(z: zipfile.ZipFile) -> tuple[dict, list[str]]:
    """Extrusor por objeto e por parte (Bambu Studio, Orca, Filuna) e as cores dos filamentos do projeto."""
    extrusores: dict = {}
    cores: list[str] = []
    nomes = set(z.namelist())
    if "Metadata/model_settings.config" in nomes:
        try:
            raiz = ET.fromstring(z.read("Metadata/model_settings.config"))
        except ET.ParseError:
            raiz = None
        for o in (raiz.findall("object") if raiz is not None else []):
            meta = {m.get("key"): m.get("value") for m in o.findall("metadata")}
            partes = {}
            for p in o.findall("part"):
                if (p.get("subtype") or "normal_part") != "normal_part":
                    continue
                pm = {m.get("key"): m.get("value") for m in p.findall("metadata")}
                partes[p.get("id")] = pm.get("extruder") or meta.get("extruder")
            extrusores[o.get("id")] = {"nome": meta.get("name") or "", "extrusor": meta.get("extruder"), "partes": partes}
    if "Metadata/project_settings.config" in nomes:
        try:
            cfg = json.loads(z.read("Metadata/project_settings.config"))
            cores = [(_cor_hex(c) or "") for c in (cfg.get("filament_colour") or [])]
        except ValueError:
            pass
    return extrusores, cores


def ler_3mf(dados: bytes) -> list[dict]:
    """Peças do 3MF: [{nome, tris:[(tri, cor|None)], cores:[hex], pintado}] — uma por item de montagem."""
    try:
        z = zipfile.ZipFile(BytesIO(dados))
    except zipfile.BadZipFile as exc:
        raise ErroArquivo("Esse 3MF não é válido.") from exc
    with z:
        if sum(i.file_size for i in z.infolist()) > motor.LIMITE_DESCOMPACTADO:
            raise ErroArquivo("Esse 3MF é grande demais depois de descompactado.")
        if "3D/3dmodel.model" not in z.namelist():
            raise ErroArquivo("Esse 3MF não tem modelo 3D.")
        cache: dict = {}
        raiz = _ler_modelo(z, "3D/3dmodel.model", cache)
        extrusores, cores_fil = _config_bambu(z)

        def cor_extrusor(e) -> str | None:
            try:
                i = int(e) - 1
            except (TypeError, ValueError):
                return None
            return (cores_fil[i] or None) if 0 <= i < len(cores_fil) else None

        def malha(doc, oid, m, cor, nivel, saida, origem_cfg):
            o = doc["objetos"].get(oid)
            if not o or nivel > 6:
                return
            if o["pid"] in doc["cores"] and o["pindex"] is not None and cor is None:
                try:
                    cor = doc["cores"][o["pid"]][int(o["pindex"])]
                except (ValueError, IndexError):
                    pass
            if o["t"]:
                v = o["v"] if m is None else [_aplica(m, *p) for p in o["v"]]
                nv = len(v)
                for a, b, c in o["t"]:
                    if a < nv and b < nv and c < nv:
                        saida.append((v[a] + v[b] + v[c], cor))
            for comp in o["comp"]:
                alvo = _ler_modelo(z, comp["caminho"], cache) if comp["caminho"] else doc
                cm = comp["m"] if m is None else (_compoe(comp["m"], m) if comp["m"] else m)
                c2 = cor_extrusor((origem_cfg or {}).get("partes", {}).get(comp["id"])) or cor
                malha(alvo, comp["id"], cm, c2, nivel + 1, saida, None)

        pecas = []
        for n, item in enumerate(raiz["itens"], start=1):
            if not item["imprimivel"]:
                continue
            cfg = extrusores.get(item["id"])
            tris: list = []
            malha(raiz, item["id"], item["m"], cor_extrusor((cfg or {}).get("extrusor")), 0, tris, cfg)
            if not tris:
                continue
            o = raiz["objetos"].get(item["id"]) or {}
            cores = []
            for _, c in tris:
                if c and c not in cores:
                    cores.append(c)
            pecas.append({"nome": ((cfg or {}).get("nome") or o.get("nome") or f"Peça {n}")[:80], "tris": tris,
                          "cores": cores, "pintado": any(d["pintado"] for d in cache.values())})
        if not pecas:
            raise ErroArquivo("Esse 3MF não tem nenhuma peça para imprimir.")
        return pecas


def _placas_3mf(dados: bytes) -> int:
    try:
        with zipfile.ZipFile(BytesIO(dados)) as z:
            raiz = ET.fromstring(z.read("Metadata/model_settings.config"))
            return max(1, len(raiz.findall("plate")))
    except (KeyError, ET.ParseError, zipfile.BadZipFile):
        return 1


def _compoe(a, b):
    """Transformação a seguida de b (matrizes 4x3 do 3MF, vetor em linha)."""
    r = []
    for i in range(3):
        for j in range(3):
            r.append(a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j])
    for j in range(3):
        r.append(a[9] * b[j] + a[10] * b[3 + j] + a[11] * b[6 + j] + b[9 + j])
    return r


# ---------- medidas e foto ----------
def medir(tris: list[Tri]) -> dict:
    x0 = y0 = z0 = math.inf
    x1 = y1 = z1 = -math.inf
    vol = 0.0
    for ax, ay, az, bx, by, bz, cx, cy, cz in tris:
        x0 = min(x0, ax, bx, cx); x1 = max(x1, ax, bx, cx)
        y0 = min(y0, ay, by, cy); y1 = max(y1, ay, by, cy)
        z0 = min(z0, az, bz, cz); z1 = max(z1, az, bz, cz)
        vol += ax * (by * cz - bz * cy) - ay * (bx * cz - bz * cx) + az * (bx * cy - by * cx)
    if not tris or not all(map(math.isfinite, (x0, x1, y0, y1, z0, z1))):
        raise ErroArquivo("O modelo não tem geometria válida.")
    return {"x": round(x1 - x0, 2), "y": round(y1 - y0, 2), "z": round(z1 - z0, 2), "volume_mm3": round(abs(vol) / 6, 1)}


def _rgb(hexa: str | None) -> tuple[int, int, int]:
    h = (hexa or "#B9B0A6").lstrip("#")
    return int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)


def foto(tris_cor: list[tuple[Tri, str | None]], lado: int = LADO_FOTO) -> bytes:
    """Desenha a peça em perspectiva isométrica (z-buffer simples) e devolve um PNG com fundo transparente."""
    passo = max(1, len(tris_cor) // MAX_FOTO)
    amostra = tris_cor[::passo]
    ca, sa = math.cos(math.radians(-35)), math.sin(math.radians(-35))
    cb, sb = math.cos(math.radians(60)), math.sin(math.radians(60))

    def girar(x, y, z):
        x, y = x * ca - y * sa, x * sa + y * ca
        return x, y * cb - z * sb, y * sb + z * cb   # tela x, tela y (para cima = -), profundidade

    pts = []
    u0 = v0 = math.inf
    u1 = v1 = -math.inf
    for t, cor in amostra:
        a, b, c = girar(*t[0:3]), girar(*t[3:6]), girar(*t[6:9])
        pts.append((a, b, c, cor))
        for p in (a, b, c):
            u0 = min(u0, p[0]); u1 = max(u1, p[0]); v0 = min(v0, p[1]); v1 = max(v1, p[1])
    margem = 8
    esc = (lado - 2 * margem) / max(u1 - u0, v1 - v0, 1e-6)
    ox = margem + (lado - 2 * margem - (u1 - u0) * esc) / 2 - u0 * esc
    oy = margem + (lado - 2 * margem - (v1 - v0) * esc) / 2 - v0 * esc
    fundo = [-math.inf] * (lado * lado)
    pix = bytearray(lado * lado * 4)
    lx, ly, lz = 0.35, -0.45, 0.82
    for a, b, c, cor in pts:
        ux, uy, uz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
        wx, wy, wz = c[0] - a[0], c[1] - a[1], c[2] - a[2]
        nx, ny, nz = uy * wz - uz * wy, uz * wx - ux * wz, ux * wy - uy * wx
        nn = math.sqrt(nx * nx + ny * ny + nz * nz)
        if nn == 0:
            continue
        luz = 0.42 + 0.58 * abs((nx * lx + ny * ly + nz * lz) / nn)
        r, g, bl = _rgb(cor)
        r, g, bl = min(255, int(r * luz + 14)), min(255, int(g * luz + 14)), min(255, int(bl * luz + 14))
        x1, y1, x2, y2, x3, y3 = (a[0] * esc + ox, a[1] * esc + oy, b[0] * esc + ox, b[1] * esc + oy,
                                  c[0] * esc + ox, c[1] * esc + oy)
        area = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)
        px0, px1 = max(0, int(min(x1, x2, x3))), min(lado - 1, int(max(x1, x2, x3)) + 1)
        py0, py1 = max(0, int(min(y1, y2, y3))), min(lado - 1, int(max(y1, y2, y3)) + 1)
        if abs(area) < 1e-9:
            # triângulo menor que um ponto: marca o ponto do centro
            cxp, cyp = int((x1 + x2 + x3) / 3), int((y1 + y2 + y3) / 3)
            if 0 <= cxp < lado and 0 <= cyp < lado:
                d = (a[2] + b[2] + c[2]) / 3
                i = cyp * lado + cxp
                if d > fundo[i]:
                    fundo[i] = d
                    pix[i * 4:i * 4 + 4] = bytes((r, g, bl, 255))
            continue
        inv = 1.0 / area
        for py in range(py0, py1 + 1):
            yc = py + 0.5
            for px in range(px0, px1 + 1):
                xc = px + 0.5
                w1 = ((x2 - xc) * (y3 - yc) - (x3 - xc) * (y2 - yc)) * inv
                w2 = ((x3 - xc) * (y1 - yc) - (x1 - xc) * (y3 - yc)) * inv
                w3 = 1.0 - w1 - w2
                if w1 < -0.02 or w2 < -0.02 or w3 < -0.02:
                    continue
                d = w1 * a[2] + w2 * b[2] + w3 * c[2]
                i = py * lado + px
                if d > fundo[i]:
                    fundo[i] = d
                    pix[i * 4:i * 4 + 4] = bytes((r, g, bl, 255))
    return _png(lado, lado, pix)


def _png(w: int, h: int, rgba: bytearray) -> bytes:
    def bloco(tipo: bytes, corpo: bytes) -> bytes:
        return struct.pack(">I", len(corpo)) + tipo + corpo + struct.pack(">I", zlib.crc32(tipo + corpo) & 0xFFFFFFFF)
    linhas = b"".join(b"\x00" + bytes(rgba[y * w * 4:(y + 1) * w * 4]) for y in range(h))
    return (b"\x89PNG\r\n\x1a\n" + bloco(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
            + bloco(b"IDAT", zlib.compress(linhas, 6)) + bloco(b"IEND", b""))


# ---------- análise completa ----------
def analisar(nome: str, dados: bytes) -> dict:
    return analisar_modelos(abrir(nome, dados))


def analisar_modelos(modelos: list[tuple[str, bytes]]) -> dict:
    """Tudo o que a Farm precisa saber do envio. As fotos vêm em bytes (campo `foto`) para quem chama guardar."""
    pecas: list[dict] = []
    placas_arquivo = 0
    triangulos = 0
    for arq, conteudo in modelos:
        tipo = motor.tipo_arquivo(arq, conteudo)
        if tipo == "stl":
            tris = ler_stl(conteudo)
            brutas = [{"nome": Path(arq).stem[:80], "tris": [(t, None) for t in tris], "cores": [], "pintado": False}]
        else:
            brutas = ler_3mf(conteudo)
            placas_arquivo += _placas_3mf(conteudo)
        for b in brutas:
            triangulos += len(b["tris"])
            if triangulos > MAX_TRIANGULOS:
                raise ErroArquivo("O envio tem triângulos demais para analisar. Envie em partes menores.")
            med = medir([t for t, _ in b["tris"]])
            if min(med["x"], med["y"], med["z"]) <= 0:
                continue
            chave = (arq, b["nome"], med["x"], med["y"], med["z"], med["volume_mm3"], tuple(b["cores"]))
            igual = next((p for p in pecas if p["_chave"] == chave), None)
            if igual:
                igual["copias"] += 1
                continue
            pecas.append({"_chave": chave, "nome": b["nome"], "arquivo": arq, "tipo": tipo, **med,
                          "cores": b["cores"], "pintado": b["pintado"], "copias": 1,
                          "triangulos": len(b["tris"]), "foto": foto(b["tris"])})
    if not pecas:
        raise ErroArquivo("Não encontramos nenhuma peça para imprimir nesse envio.")
    for p in pecas:
        del p["_chave"]
    return {"arquivos": len(modelos), "modelos": [m[0] for m in modelos], "pecas": pecas,
            "placas_arquivo": placas_arquivo}


def foto_da_peca(arquivo: str, dados: bytes, nome: str, cor: str | None) -> bytes | None:
    """Refaz a foto de uma peça sem cor no arquivo (STL) já na cor que o cliente escolheu."""
    if Path(arquivo).suffix.lower() != ".stl":
        return None
    return foto([(t, cor) for t in ler_stl(dados)])
