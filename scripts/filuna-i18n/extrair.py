#!/usr/bin/env python3
"""Frases em português das telas Filuna e do C++ da Filuna, para o dicionário de inglês.

Varre as telas web (resources/web/filuna: HTML, textos e atributos, e as strings dos scripts; e as partes Filuna da
tela Início, resources/web/homepage/index.html e js/filuna-*.js) e o C++ da Filuna
(src/slic3r/GUI/Filuna*.cpp, CAD/*Filuna*, src/slic3r/Filuna). Junta as partes de frases montadas com variáveis
("A impressora " + nome + " parou") em padrões com {0}, {1}... e compara com resources/web/filuna/i18n/en.json.

Uso (na raiz do repositório):
  python3 scripts/filuna-i18n/extrair.py                 # relatório de cobertura
  python3 scripts/filuna-i18n/extrair.py --faltando      # lista o que falta traduzir (texto e onde aparece)
  python3 scripts/filuna-i18n/extrair.py --json saida.json   # o que falta, como {"frases": {pt: ""}, "padroes": {...}}
  python3 scripts/filuna-i18n/extrair.py --todas todas.json  # todas as frases achadas, com os arquivos

Para completar: traduza o que saiu em --json, junte em en.json ("frases" para textos inteiros, "padroes" para os com
{0}) ou, se não for texto que o usuário vê, ponha em scripts/filuna-i18n/ignorar.json.
"""
import argparse
import glob
import html
import json
import os
import re
import sys
from html.parser import HTMLParser

RAIZ = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TELAS = os.path.join(RAIZ, "resources", "web", "filuna")
DICIONARIO = os.path.join(TELAS, "i18n", "en.json")
INICIO = os.path.join(RAIZ, "resources", "web", "homepage")
IGNORAR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ignorar.json")

# Telas: tudo em resources/web/filuna menos bibliotecas de terceiros e o próprio dicionário.
FORA_WEB = {"mpegts.js", "qr.js", "m3d/three.js", "m3d/orbit.js", "i18n/traduzir.js"}
CPP = ["src/slic3r/GUI/Filuna*.cpp", "src/slic3r/GUI/CAD/DesignPanelFiluna.cpp", "src/slic3r/GUI/CAD/FilunaCadUi.cpp",
       "src/slic3r/Filuna/*.cpp"]

PH = "\ue000"  # marca de variável dentro do modelo (vira {n})
ATRIBUTOS = {"placeholder", "title", "aria-label", "alt"}
LETRA = re.compile(r"[A-Za-zÀ-ÿ]")
ACENTO = re.compile(r"[À-ÿ]")
LIGACAO_PT = set("""de da do das dos para com não nao você voce sua seu suas seus uma um os as em no na nos nas que ao aos à às
mais sem por está esta este isso essa esse pela pelo pelas pelos já ja ou se é foi ser ter tem há ainda depois antes
quando como onde aqui agora nenhum nenhuma cada outra outro também tambem e o a""".split())
PALAVRAS_PT = LIGACAO_PT | set("""mesa bico impressora filamento conta peça peças minha meu meus minhas nova novo bobina bobinas
ativa ativas ver abrir salvar""".split())
PALAVRAS_EN = set("""the and of to with for your is are this that from by on in at or be not you it an as can will""".split())
# chamadas cujo argumento de texto não é texto de tela
CHAMADAS_FORA = re.compile(
    r"(querySelector(All)?|getElementById|getElementsBy\w+|closest|matches|nativo|api|rede|enviarArquivo|imagem|classList\.\w+|"
    r"setAttribute|getAttribute|removeAttribute|hasAttribute|addEventListener|removeEventListener|ouvir|_evento|lerLocal|"
    r"guardarLocal|createElement|setProperty|getPropertyValue|getItem|setItem|removeItem|postMessage|console\.\w+|"
    r"BOOST_LOG_TRIVIAL\([^)]*\)\s*<<|get|set|value|contains|find|count|at|erase|emplace|opt_string|opt_int|"
    r"has|key|load_from_svg_file|push_back|insert|starts_with|ends_with|rfind|regex|split|join|indexOf|startsWith|"
    r"endsWith|test|exec|match|replace|fetch|open|send_event|broadcast|RunScript|AddUserScript|LoadURL|"
    r"select_filuna_tab|abrirLink|PostMessage)\s*\(\s*(\w+\s*,\s*)?$")
LOG = re.compile(r"(BOOST_LOG_TRIVIAL\s*\(\s*\w+\s*\)|console\.\w+\s*\()[^;]*$")


# ---------------------------------------------------------------- leitura de JS / C++
class Tok:
    __slots__ = ("t", "v", "i", "pos")

    def __init__(self, t, v, pos):
        self.t, self.v, self.pos = t, v, pos  # t: s (texto), m (modelo com variáveis), w (palavra), p (pontuação)


def ler_tokens(src, cpp=False):
    """Tokens com strings decodificadas. Modelos `...${x}...` viram ('m', [partes, ...exprs])."""
    toks, i, n = [], 0, len(src)
    ultimo = ""  # último token significativo (para saber se / começa regex)
    while i < n:
        c = src[i]
        if c in " \t\r\n":
            i += 1
            continue
        if src.startswith("//", i):
            j = src.find("\n", i)
            i = n if j < 0 else j
            continue
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            i = n if j < 0 else j + 2
            continue
        if cpp and c == "#" and (i == 0 or src[i - 1] == "\n"):  # diretiva do pré-processador
            j = src.find("\n", i)
            i = n if j < 0 else j
            continue
        if cpp and c == "R" and src.startswith('R"', i) and not (i and (src[i - 1].isalnum() or src[i - 1] == "_")):
            m = re.match(r'R"([^(\s]*)\(', src[i:])
            if m:
                fim = src.find(")" + m.group(1) + '"', i)
                i = n if fim < 0 else fim + len(m.group(1)) + 2
                toks.append(Tok("p", "R", i))  # string crua (código embutido): não entra
                ultimo = "x"
                continue
        if c in "\"'" or (c == "`" and not cpp):
            j, partes, buf, exprs = i + 1, [], [], []
            while j < n and src[j] != c:
                if src[j] == "\\" and j + 1 < n:
                    buf.append(src[j:j + 2])
                    j += 2
                    continue
                if c == "`" and src.startswith("${", j):
                    k, prof = j + 2, 1
                    while k < n and prof:
                        if src[k] == "{":
                            prof += 1
                        elif src[k] == "}":
                            prof -= 1
                        k += 1
                    partes.append("".join(buf))
                    buf = []
                    exprs.append(src[j + 2:k - 1])
                    j = k
                    continue
                if src[j] == "\n" and c != "`":
                    break
                buf.append(src[j])
                j += 1
            partes.append("".join(buf))
            texto = [decodificar(p, cpp) for p in partes]
            if cpp and c == "'":
                pass  # caractere em C++
            elif len(texto) == 1:
                toks.append(Tok("s", texto[0], i))
            else:
                toks.append(Tok("m", (texto, exprs), i))
            i = j + 1
            ultimo = "x"
            continue
        if c == "/" and not cpp and (ultimo in ("", "(", ",", "=", ":", "[", "!", "&", "|", "?", "{", "}", ";", "+", "-", "*", "%", "<", ">", "~", "^")
                                     or ultimo in ("return", "typeof", "case", "do", "else", "in", "of")):
            j, classe = i + 1, False
            while j < n and src[j] != "\n":
                if src[j] == "\\":
                    j += 2
                    continue
                if src[j] == "[":
                    classe = True
                elif src[j] == "]":
                    classe = False
                elif src[j] == "/" and not classe:
                    break
                j += 1
            j += 1
            while j < n and src[j].isalpha():
                j += 1
            toks.append(Tok("p", "/re/", i))
            i = j
            ultimo = "x"
            continue
        if c.isalnum() or c == "_" or c == "$":
            j = i
            while j < n and (src[j].isalnum() or src[j] in "_$"):
                j += 1
            w = src[i:j]
            # prefixos de string do C++ (u8"...", L"...")
            if cpp and w in ("u8", "L", "u", "U") and j < n and src[j] == '"':
                i = j
                continue
            toks.append(Tok("w", w, i))
            ultimo = w
            i = j
            continue
        toks.append(Tok("p", c, i))
        ultimo = c
        i += 1
    # C++: "a" "b" seguidos viram uma string só
    if cpp:
        junt = []
        for t in toks:
            if t.t == "s" and junt and junt[-1].t == "s":
                junt[-1] = Tok("s", junt[-1].v + t.v, junt[-1].pos)
            else:
                junt.append(t)
        toks = junt
    for k, t in enumerate(toks):
        t.i = k
    return toks


def decodificar(s, cpp):
    if "\\" not in s:
        return s
    if cpp:
        # bytes \xNN (UTF-8 escrito à mão) e escapes simples
        b, i = bytearray(), 0
        while i < len(s):
            if s[i] == "\\" and i + 1 < len(s):
                e = s[i + 1]
                if e == "x":
                    m = re.match(r"[0-9a-fA-F]{1,2}", s[i + 2:])
                    if m:
                        b.append(int(m.group(0), 16))
                        i += 2 + len(m.group(0))
                        continue
                b += {"n": b"\n", "t": b"\t", "\\": b"\\", '"': b'"', "'": b"'", "r": b""}.get(e, e.encode())
                i += 2
                continue
            b += s[i].encode("utf-8")
            i += 1
        return b.decode("utf-8", "replace")

    def um(m):
        e = m.group(1)
        if e[0] == "u":
            try:
                return chr(int(e[1:].strip("{}"), 16))
            except ValueError:
                return ""
        if e[0] == "x":
            return chr(int(e[1:], 16))
        return {"n": "\n", "t": "\t", "r": "", "b": "", "f": "", "v": ""}.get(e, e)
    return re.sub(r"\\(u\{[0-9a-fA-F]+\}|u[0-9a-fA-F]{4}|x[0-9a-fA-F]{2}|.)", um, s)


FECHA = {")": "(", "]": "[", "}": "{"}
PARA = re.compile(r"^[,;?:]$")


def operando(toks, k, passo):
    """Índice logo depois do operando que começa em k (passo +1) ou termina em k (passo -1); None se não houver."""
    abre, fecha = ("(", "[", "{"), (")", "]", "}")
    if passo < 0:
        abre, fecha = fecha, abre
    prof, j, viu = 0, k, False
    while 0 <= j < len(toks):
        t = toks[j]
        if t.t == "p":
            if t.v in abre:
                prof += 1
            elif t.v in fecha:
                if prof == 0:
                    break
                prof -= 1
            elif prof == 0 and t.v in ",;?:+=&|<>" or (prof == 0 and t.v == "!" and passo < 0):
                break
        elif prof == 0 and passo < 0 and t.t == "w" and t.v in ("return", "case", "throw", "typeof", "else", "new", "var", "let", "const"):
            break
        viu = True
        j += passo
        if prof == 0 and passo > 0 and j < len(toks) and toks[j].t in ("w", "s", "m") and toks[j - 1].t in ("w", "s", "m"):
            break  # dois termos seguidos sem operador: acabou
    return j if viu else None


def tem_texto(toks, a, b):
    return any(t.t in ("s", "m") for t in toks[min(a, b):max(a, b) + 1])


def cadeias(toks):
    """Concatenações com '+' que têm ao menos uma string: lista de (modelo, índice do 1º token)."""
    usados, saida = set(), []
    for k, t in enumerate(toks):
        if t.t not in ("s", "m") or k in usados:
            continue
        # volta até o começo da cadeia
        ini = k
        partes, meus = [], set()
        while ini - 1 >= 0 and toks[ini - 1].t == "p" and toks[ini - 1].v == "+" and not (ini - 2 >= 0 and toks[ini - 2].t == "p" and toks[ini - 2].v == "+"):
            fim_ant = ini - 2
            if fim_ant < 0:
                break
            if toks[fim_ant].t in ("s", "m"):
                ini = fim_ant
                continue
            comeco = operando(toks, fim_ant, -1)
            if comeco is None:
                break
            ini = comeco + 1
        # agora anda para frente juntando
        j = ini
        while j < len(toks):
            tj = toks[j]
            if tj.t == "s":
                partes.append(("s", tj.v))
                usados.add(j)
                meus.add(j)
                j += 1
            elif tj.t == "m":
                txt, exprs = tj.v
                for q, p in enumerate(txt):
                    partes.append(("s", p))
                    if q < len(exprs):
                        partes.append(("v", exprs[q]))
                usados.add(j)
                j += 1
            else:
                fim = operando(toks, j, +1)
                if fim is None or fim == j:
                    break
                partes.append(("v", None))
                j = fim
            if j < len(toks) and toks[j].t == "p" and toks[j].v == "+" and not (j + 1 < len(toks) and toks[j + 1].t == "p" and toks[j + 1].v in "+="):
                j += 1
                continue
            break
        if sum(1 for p in partes if p[0] == "s") and len(partes) > 1:
            modelo = "".join(p[1] if p[0] == "s" else PH for p in partes)
            saida.append((modelo, ini, meus))
    return saida


def contexto_fora(src, pos, cpp):
    antes = src[max(0, src.rfind("\n", 0, pos) - 200):pos]
    if LOG.search(antes):
        return True
    if CHAMADAS_FORA.search(antes):
        return True
    if cpp and re.search(r"\b(_L|_u8L|L|_CTX|_CTX_utf8|_utf8)\s*\(\s*$", antes):
        return True  # já passa pelo gettext do Orca
    return False


# ---------------------------------------------------------------- texto de tela?
TAG = re.compile(r"<(/?)([a-zA-Z][\w-]*)((?:[^>\"']|\"[^\"]*\"|'[^']*')*)>|<!--.*?-->", re.S)
ATTR = re.compile(r"([\w-]+)\s*=\s*(\"([^\"]*)\"|'([^']*)')")


def segmentos(modelo):
    """Partes de texto de um trecho com HTML, e os atributos visíveis das tags."""
    out = []
    pos = 0
    for m in TAG.finditer(modelo):
        out.append(modelo[pos:m.start()])
        if m.group(3):
            for a in ATTR.finditer(m.group(3)):
                nome = a.group(1).lower()
                if nome in ATRIBUTOS or (nome == "value" and re.search(r"type\s*=\s*[\"']?(button|submit|reset)", m.group(3))):
                    out.append(a.group(3) if a.group(3) is not None else a.group(4))
        pos = m.end()
    out.append(modelo[pos:])
    return out


def normalizar(s):
    return re.sub(r"\s+", " ", html.unescape(s.replace(PH, "\ue001"))).replace("\ue001", PH).strip()


CODIGO = re.compile(r"(=>|\bfunction\b|\(\)|;\s*$|^[.#][\w-]+$|\.(html|js|css|png|svg|json|stl|3mf|gcode)\b|https?:|/api/|\\|&&|\|\||===|"
                    r"^[\w.-]+\([^)]*\)$|^[a-z-]+\s*:\s*[^ ]+;|^\w+(\.\w+)+$|^[\w-]+/[\w/.-]*$|^[%{}\d\s.,:;/+*()\[\]#-]+$|"
                    r"^(GET|POST|PUT|DELETE)$|rgba?\(|\bpx\b|^\s*[\w-]+=|"
                    r"\b(SELECT|INSERT|UPDATE|DELETE|CREATE|FROM|WHERE|COALESCE|VALUES|PRAGMA|ALTER|JOIN)\b|"
                    r"^[GMT]\d+\b|\b[A-Z][A-Z0-9]+_[A-Z0-9_]+\b)")


def e_texto(s):
    """s já normalizado (com PH). Decide se é texto em português para o usuário."""
    if len(s) < 2 or not LETRA.search(s.replace(PH, "")):
        return False
    so = s.replace(PH, " ").strip()
    if CODIGO.search(so):
        return False
    if re.search(r"[{}<>]", so):
        return False
    palavras = re.findall(r"[A-Za-zÀ-ÿ]+", so)
    if not palavras:
        return False
    baixas = [p.lower() for p in palavras]
    if ACENTO.search(so):
        return True
    if any(p in PALAVRAS_PT for p in baixas):
        return True
    if len(palavras) == 1:
        p = palavras[0]
        # uma palavra só: em maiúscula ("Salvar", "SALVAR") é rótulo; minúscula sem acento costuma ser código
        return bool(re.match(r"^[A-ZÀ-Ý][a-zà-ÿ]{2,}$", p) or re.match(r"^[A-ZÀ-Ý]{4,}$", p)) and so == p or (
            bool(re.match(r"^[A-ZÀ-Ý][a-zà-ÿ]{2,}[:.!?]?$", so)))
    if all(p in PALAVRAS_EN for p in baixas):
        return False
    # várias palavras sem acento nem palavra comum do português ("bobina ativa", "Minha IA"): entra, menos lista de
    # classes CSS e nomes de código ("btn primario", "rx-linha ok")
    tokens = so.split()
    if len(tokens) < 2 or any(re.match(r"^[a-z0-9]+[-_][a-z0-9]", t) for t in tokens):
        return False
    return True


def para_chave(s):
    """Modelo com PH -> chave com {0}, {1}..."""
    n = [0]

    def troca(_):
        n[0] += 1
        return "{%d}" % (n[0] - 1)
    return re.sub(PH, troca, s)


# ---------------------------------------------------------------- coleta
class Coleta:
    def __init__(self):
        self.frases = {}   # texto -> {"arquivos": set, "origem": set}
        self.padroes = {}

    def por(self, texto, arquivo, origem):
        t = normalizar(texto)
        if not e_texto(t):
            return
        # tira variáveis das pontas: "{0} peças" fica padrão; "Total: {0}" também
        if PH in t:
            sem = normalizar(t.replace(PH, " ")).strip(" :·—-–,.()")
            fixas = re.findall(r"[A-Za-zÀ-ÿ]+", sem)
            if not sem or not e_texto(sem) or sum(len(w) for w in fixas) < 4 or all(w.lower() in LIGACAO_PT for w in fixas):
                return
            if re.match(PH + r"[A-ZÀ-Ý]", t):
                t = t[1:]  # ícone em HTML antes do texto ("{0}Importar modelo")
                if PH not in t:
                    return self.por(t, arquivo, origem)
            alvo, chave = self.padroes, para_chave(t)
        else:
            alvo, chave = self.frases, t
        e = alvo.setdefault(chave, {"arquivos": set(), "origem": set()})
        e["arquivos"].add(arquivo)
        e["origem"].add(origem)

    def modelo(self, modelo, arquivo, origem, pedaco_de_cadeia=False):
        if origem == "cpp" and "\n" in modelo.strip():
            # avisos do C++ com parágrafos: Filuna::tr traduz linha a linha para manter as quebras
            for linha in modelo.split("\n"):
                if linha.strip():
                    self.modelo(linha, arquivo, origem, pedaco_de_cadeia)
        for seg in segmentos(modelo):
            if pedaco_de_cadeia and not re.match(r"^\s*[A-ZÀ-Ý0-9✦+•]", seg.replace(PH, "")):
                continue
            # em C++ e nas strings já convertidas, {0} escrito é variável
            seg = re.sub(r"\{\d+\}", PH, seg)
            self.por(seg, arquivo, origem)
            # partes sem variável de um padrão também aparecem sozinhas na tela (texto antes de um <b>{0}</b>)
            if PH in seg:
                for pedaco in seg.split(PH):
                    if re.match(r"^\s*[A-ZÀ-Ý]", pedaco):
                        self.por(pedaco, arquivo, origem)

    def codigo(self, src, arquivo, origem, cpp=False):
        toks = ler_tokens(src, cpp)
        na_cadeia = set()
        for modelo, k, usados in cadeias(toks):
            if contexto_fora(src, toks[k].pos, cpp):
                continue
            na_cadeia |= usados
            self.modelo(modelo, arquivo, origem)
        for t in toks:
            if contexto_fora(src, t.pos, cpp):
                continue
            if t.t == "s":
                self.modelo(t.v, arquivo, origem, t.i in na_cadeia)
            elif t.t == "m":
                txt, exprs = t.v
                self.modelo(PH.join(txt), arquivo, origem)
                for e in exprs:
                    self.codigo(e, arquivo, origem, cpp)


class LeitorHtml(HTMLParser):
    def __init__(self, coleta, arquivo):
        super().__init__(convert_charrefs=True)
        self.c, self.arq, self.pilha, self.script = coleta, arquivo, [], None

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if tag in ("script", "style"):
            self.script = [] if tag == "script" and not a.get("src") else ([] if tag == "style" else None)
            self.estilo = tag == "style"
        for k, v in attrs:
            if v and (k in ATRIBUTOS or (k == "value" and a.get("type") in ("button", "submit", "reset"))):
                self.c.por(v, self.arq, "web")
            # JS em atributos de evento (onclick="...")
            if v and k.startswith("on"):
                self.c.codigo(v, self.arq, "web")

    def handle_endtag(self, tag):
        if tag in ("script", "style") and self.script is not None:
            if tag == "script":
                self.c.codigo("".join(self.script), self.arq, "web")
            self.script = None

    def handle_data(self, data):
        if self.script is not None:
            self.script.append(data)
            return
        self.c.por(data, self.arq, "web")


def arquivos_web():
    for p in sorted(glob.glob(os.path.join(TELAS, "**", "*"), recursive=True)):
        rel = os.path.relpath(p, TELAS).replace(os.sep, "/")
        if rel in FORA_WEB or rel.startswith("farm-py/") or rel.endswith(".test.js") or not re.search(r"\.(html|js)$", rel):
            continue
        yield p, rel
    # Tela Início: as partes da Filuna (o resto é do OrcaSlicer, traduzido pelo i18n.js dele). O programa injeta o
    # mesmo tradutor nela (WebViewDialog.cpp).
    for p in [os.path.join(INICIO, "index.html")] + sorted(glob.glob(os.path.join(INICIO, "js", "filuna-*.js"))):
        if os.path.exists(p):
            yield p, "homepage/" + os.path.relpath(p, INICIO).replace(os.sep, "/")


def coletar():
    c = Coleta()
    for p, rel in arquivos_web():
        src = open(p, encoding="utf-8", errors="replace").read()
        if rel.endswith(".html"):
            LeitorHtml(c, rel).feed(src)
        else:
            c.codigo(src, rel, "web")
    for padrao in CPP:
        for p in sorted(glob.glob(os.path.join(RAIZ, padrao))):
            rel = os.path.relpath(p, RAIZ).replace(os.sep, "/")
            if rel.endswith("FilunaIdioma.cpp"):
                continue
            c.codigo(open(p, encoding="utf-8", errors="replace").read(), rel, "cpp", cpp=True)
    return c


def carregar(p, padrao):
    try:
        return json.load(open(p, encoding="utf-8"))
    except FileNotFoundError:
        return padrao


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--faltando", action="store_true", help="lista as frases sem tradução")
    ap.add_argument("--json", help="grava o que falta traduzir neste arquivo")
    ap.add_argument("--todas", help="grava todas as frases achadas neste arquivo")
    ap.add_argument("--sobrando", action="store_true", help="lista traduções que não aparecem mais no código")
    a = ap.parse_args()

    c = coletar()
    d = carregar(DICIONARIO, {})
    ign = set(carregar(IGNORAR, []))
    fr, pa = d.get("frases", {}), d.get("padroes", {})

    def situacao(chave, tabela):
        if chave in ign:
            return "ignorada"
        return "traduzida" if (tabela.get(chave) or "").strip() else "faltando"

    rel = {}
    falta = {"frases": {}, "padroes": {}}
    for nome, itens, tabela in (("frases", c.frases, fr), ("padroes", c.padroes, pa)):
        for chave, info in itens.items():
            s = situacao(chave, tabela)
            for o in info["origem"]:
                rel.setdefault((o, nome), {"traduzida": 0, "faltando": 0, "ignorada": 0})[s] += 1
            if s == "faltando":
                falta[nome][chave] = sorted(info["arquivos"])

    print("Cobertura da tradução para inglês (en.json):")
    tot = {"traduzida": 0, "faltando": 0, "ignorada": 0}
    for (o, nome), v in sorted(rel.items()):
        conta = v["traduzida"] + v["faltando"]
        pct = 100.0 * v["traduzida"] / conta if conta else 100.0
        print("  %-4s %-8s %5d traduzidas, %4d faltando, %4d ignoradas (não são texto de tela)  -> %5.1f%%"
              % (o, nome, v["traduzida"], v["faltando"], v["ignorada"], pct))
        for k in tot:
            tot[k] += v[k]
    conta = tot["traduzida"] + tot["faltando"]
    print("  total: %d traduzidas, %d faltando -> %.1f%%" % (tot["traduzida"], tot["faltando"], 100.0 * tot["traduzida"] / conta if conta else 100))

    if a.faltando:
        for nome in ("frases", "padroes"):
            for chave, arqs in sorted(falta[nome].items()):
                print("%s\t%s\t%s" % (nome, chave, ", ".join(arqs)))
    if a.json:
        json.dump({k: {ch: "" for ch in sorted(v)} for k, v in falta.items()}, open(a.json, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
        print("faltando gravado em", a.json)
    if a.todas:
        json.dump({"frases": {k: sorted(v["arquivos"]) for k, v in sorted(c.frases.items())},
                   "padroes": {k: sorted(v["arquivos"]) for k, v in sorted(c.padroes.items())}},
                  open(a.todas, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
        print("todas gravadas em", a.todas)
    if a.sobrando:
        for nome, itens, tabela in (("frases", c.frases, fr), ("padroes", c.padroes, pa)):
            for chave in sorted(set(tabela) - set(itens)):
                print("sobrando\t%s\t%s" % (nome, chave))
    return 0


if __name__ == "__main__":
    sys.exit(main())
