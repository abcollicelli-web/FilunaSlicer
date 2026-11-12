"""Leitor de G-code binário (.bgcode, libbgcode / PrusaSlicer 2.7+), em Python puro.

A régua compara o que a impressora recebe. Com `binary_gcode = 1` (padrão das Prusa MK4/XL/CORE)
o PrusaSlicer grava blocos comprimidos (heatshrink) e empacotados (MeatPack); este módulo os abre
para o texto, sem nenhum componente externo, para a régua normalizar e comparar por camada como
faz com o ASCII. A decodificação é determinística: os mesmos blocos dão sempre o mesmo texto, e
blocos diferentes dão texto diferente. A data de geração ("Produced on", bloco de metadados do
arquivo) vira a linha normalizada da régua; as miniaturas entram pelo SHA-256 dos bytes.
"""
from __future__ import annotations

import hashlib
import struct
import zlib

MAGICO = b"GCDE"
TIPOS = {0: "file_metadata", 1: "gcode", 2: "slicer_metadata", 3: "printer_metadata", 4: "print_metadata", 5: "thumbnail"}


def eh_bgcode(b: bytes) -> bool:
    return b[:4] == MAGICO


def _heatshrink(dados: bytes, w: int, l: int) -> bytes:
    """Descompressor heatshrink (janela 2^w, lookahead 2^l), bits do mais significativo ao menos."""
    saida = bytearray()
    total = len(dados) * 8
    pos = 0

    def bits(n: int) -> int | None:
        nonlocal pos
        if pos + n > total:
            return None
        v = 0
        for _ in range(n):
            v = (v << 1) | ((dados[pos >> 3] >> (7 - (pos & 7))) & 1)
            pos += 1
        return v

    while True:
        tag = bits(1)
        if tag is None:
            break
        if tag:
            c = bits(8)
            if c is None:
                break
            saida.append(c)
        else:
            i = bits(w)
            if i is None:
                break
            n = bits(l)
            if n is None:
                break
            off, n = i + 1, n + 1
            if off > len(saida):
                raise ValueError("bgcode: referência heatshrink fora da janela")
            for _ in range(n):
                saida.append(saida[-off])
    return bytes(saida)


_MP = "0123456789. \nGX"


def _meatpack(dados: bytes) -> bytes:
    """Desempacota MeatPack (com comandos 0xFF 0xFF <cmd>), como o unmeatpack da libbgcode."""
    saida = bytearray()
    empacotando = sem_espaco = cmd_ativo = False
    cmd_n = fila = 0
    guardado = None

    def ch(n: int) -> int:
        if n == 0b1011 and sem_espaco:
            return ord("E")
        return ord(_MP[n])

    def interno(c: int):
        nonlocal fila, guardado
        if not empacotando:
            saida.append(c)
            return
        if fila > 0:
            saida.append(c)
            if guardado is not None:
                saida.append(guardado)
                guardado = None
            fila -= 1
            return
        baixo, alto = c & 0xF, (c >> 4) & 0xF
        if baixo == 0xF:
            fila += 1
            if alto == 0xF:
                fila += 1
            else:
                guardado = ch(alto)
        else:
            a = ch(baixo)
            saida.append(a)
            if a != 0x0A:
                if alto == 0xF:
                    fila += 1
                else:
                    saida.append(ch(alto))

    for c in dados:
        if c == 0xFF:
            if cmd_n > 0:
                cmd_ativo, cmd_n = True, 0
            else:
                cmd_n += 1
            continue
        if cmd_ativo:
            if c == 0xFB:
                empacotando = True
            elif c in (0xFA, 0xF9):
                empacotando = False
            elif c == 0xF7:
                sem_espaco = True
            elif c == 0xF6:
                sem_espaco = False
            cmd_ativo = False
            continue
        if cmd_n > 0:
            interno(0xFF)
            cmd_n = 0
        interno(c)
    return bytes(saida)


def blocos(b: bytes):
    """Gera (tipo, parâmetros, dados descomprimidos) de cada bloco."""
    if not eh_bgcode(b):
        raise ValueError("não é bgcode")
    _, _versao, cks = struct.unpack_from("<4sIH", b, 0)
    p = 10
    tam_cks = 4 if cks == 1 else 0
    while p < len(b):
        tipo, comp, nao_comp = struct.unpack_from("<HHI", b, p)
        p += 8
        if comp:
            (comp_tam,) = struct.unpack_from("<I", b, p)
            p += 4
        else:
            comp_tam = nao_comp
        tam_par = 6 if tipo == 5 else 2
        par = b[p:p + tam_par]
        p += tam_par
        dados = b[p:p + comp_tam]
        p += comp_tam + tam_cks
        if comp == 1:
            dados = zlib.decompress(dados)
        elif comp == 2:
            dados = _heatshrink(dados, 11, 4)
        elif comp == 3:
            dados = _heatshrink(dados, 12, 4)
        elif comp != 0:
            raise ValueError(f"bgcode: compressão {comp} desconhecida")
        if tipo != 5 and len(dados) != nao_comp:
            raise ValueError(f"bgcode: bloco {TIPOS.get(tipo, tipo)} com {len(dados)} bytes, esperado {nao_comp}")
        if tipo == 1:
            (cod,) = struct.unpack("<H", par)
            if cod in (1, 2):
                dados = _meatpack(dados)
        yield tipo, par, dados


def metadados(b: bytes, tipo: int = 2) -> dict[str, str]:
    """Pares chave=valor de um bloco de metadados (2 = slicer: a configuração efetiva inteira)."""
    out: dict[str, str] = {}
    for t, _par, d in blocos(b):
        if t == tipo:
            for ln in d.decode("utf-8", "replace").splitlines():
                if "=" in ln:
                    k, v = ln.split("=", 1)
                    out[k.strip()] = v.strip()
    return out


def texto(b: bytes) -> bytes:
    """Texto canônico do arquivo: metadados como comentários, miniaturas por hash, G-code por extenso.
    Entrada que não é bgcode volta intacta."""
    if not eh_bgcode(b):
        return b
    partes: list[bytes] = []
    for tipo, par, d in blocos(b):
        nome = TIPOS.get(tipo, str(tipo)).encode()
        if tipo == 1:
            partes.append(d.rstrip(b"\n"))
        elif tipo == 5:
            fmt, larg, alt = struct.unpack("<HHH", par)
            partes.append(b"; [thumbnail] %d %dx%d sha256=%s" % (fmt, larg, alt, hashlib.sha256(d).hexdigest().encode()))
        else:
            for ln in d.splitlines():
                if tipo == 0 and ln.startswith(b"Produced on"):
                    partes.append(b"; <cabecalho-normalizado>")
                else:
                    partes.append(b"; [" + nome + b"] " + ln)
    return b"\n".join(partes) + b"\n"
