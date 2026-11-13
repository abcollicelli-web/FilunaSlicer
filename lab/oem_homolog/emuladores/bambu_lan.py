#!/usr/bin/env python3
"""Emulador do modo LAN de uma impressora Bambu Lab, para os ensaios de rede do Filuna.

Reproduz o que o protocolo local aberto oferece (o mesmo que o plug-in fechado usa por baixo):
  - SSDP: anúncio NOTIFY na porta UDP 2021 (e 1990), cabeçalhos *.bambu.com;
  - MQTT 3.1.1 sobre TLS (8883), usuário "bblp", senha = código de acesso; tópicos
    device/<serial>/report (impressora -> slicer) e device/<serial>/request (slicer -> impressora);
  - FTPS implícito (990): TLS antes do banner, USER/PASS/PBSZ/PROT/PASV/TYPE/STOR/SIZE/LIST.
Estado simulado: IDLE -> RUNNING (project_file de um arquivo que existe no FTP) -> PAUSE ->
RUNNING -> FINISH/IDLE; AMS com 4 bandejas; temperaturas mudam com os comandos de G-code.
Não é firmware: não imprime nem simula movimento. Serve para provar descoberta, conexão,
autenticação, envio, início, controle e leitura do sistema de cores.

Uso: bambu_lan.py --ip 127.0.0.1 --serial 01P00A123456789 --codigo 12345678 --pasta /tmp/sd
     [--mqtt 8883 --ftps 990 --ssdp 2021 --modelo C11 --nome "P1P emulada"]
Registra cada evento em JSON por linha em --log (para a prova).
"""
from __future__ import annotations

import argparse
import json
import os
import socket
import ssl
import struct
import subprocess
import tempfile
import threading
import time
from pathlib import Path


def certificado(pasta: Path) -> tuple[str, str]:
    crt, key = pasta / "emu.crt", pasta / "emu.key"
    if not crt.exists():
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "30", "-subj", "/CN=BBL emulada",
                        "-keyout", str(key), "-out", str(crt)], check=True, capture_output=True)
    return str(crt), str(key)


class Log:
    def __init__(self, caminho: str | None):
        self.f = open(caminho, "a", encoding="utf-8") if caminho else None
        self.mx = threading.Lock()

    def __call__(self, evento: str, **dados):
        linha = json.dumps({"t": round(time.time(), 3), "evento": evento, **dados}, ensure_ascii=False)
        with self.mx:
            print(linha, flush=True)
            if self.f:
                self.f.write(linha + "\n")
                self.f.flush()


# ---------------------------------------------------------------- MQTT
def rl_encode(n: int) -> bytes:
    out = bytearray()
    while True:
        b = n % 128
        n //= 128
        out.append(b | (0x80 if n else 0))
        if not n:
            return bytes(out)


def pacote(tf: int, corpo: bytes) -> bytes:
    return bytes([tf]) + rl_encode(len(corpo)) + corpo


def mqtt_str(s: str) -> bytes:
    b = s.encode()
    return struct.pack("!H", len(b)) + b


def ler_pacote(sock) -> tuple[int, bytes] | None:
    cab = sock.recv(1)
    if not cab:
        return None
    mult, n = 1, 0
    while True:
        b = sock.recv(1)
        if not b:
            return None
        n += (b[0] & 0x7F) * mult
        mult *= 128
        if not b[0] & 0x80:
            break
    corpo = b""
    while len(corpo) < n:
        parte = sock.recv(n - len(corpo))
        if not parte:
            return None
        corpo += parte
    return cab[0], corpo


class Impressora:
    def __init__(self, a, log: Log):
        self.a, self.log = a, log
        self.estado = "IDLE"
        self.arquivo = ""
        self.progresso = 0
        self.bico, self.mesa = 25, 25
        self.clientes: list = []
        self.mx = threading.Lock()
        cores = ["FF0000FF", "1565C0FF", "FFD600FF", "15B857FF"]
        self.ams = {"ams": [{"id": "0", "humidity": "4", "temp": "26.0",
                             "tray": [{"id": str(i), "tray_type": "PLA", "tray_color": cores[i], "tray_sub_brands": "PLA Basic",
                                       "nozzle_temp_min": "190", "nozzle_temp_max": "230", "remain": 80} for i in range(4)]}],
                    "ams_exist_bits": "1", "tray_exist_bits": "f", "tray_now": "255", "version": 4}

    def status(self) -> dict:
        return {"print": {"command": "push_status", "msg": 0, "sequence_id": "0", "gcode_state": self.estado,
                          "gcode_file": self.arquivo, "subtask_name": Path(self.arquivo).stem if self.arquivo else "",
                          "mc_percent": self.progresso, "mc_remaining_time": 0 if self.estado != "RUNNING" else 30,
                          "nozzle_temper": self.bico, "nozzle_target_temper": self.bico, "bed_temper": self.mesa,
                          "bed_target_temper": self.mesa, "wifi_signal": "-40dBm", "print_type": "local" if self.arquivo else "idle",
                          "ams": self.ams, "lights_report": [{"node": "chamber_light", "mode": "on"}],
                          "ipcam": {"ipcam_dev": "1", "rtsp_url": "disable"}, "hms": []}}

    def publicar(self, msg: dict):
        dado = pacote(0x30, mqtt_str(f"device/{self.a.serial}/report") + json.dumps(msg).encode())
        with self.mx:
            for c in list(self.clientes):
                try:
                    c.sendall(dado)
                except OSError:
                    self.clientes.remove(c)

    def comando(self, req: dict):
        p = req.get("print") or {}
        cmd = p.get("command") or (req.get("pushing") or {}).get("command")
        self.log("comando", comando=cmd, conteudo=req)
        if cmd == "pushall":
            pass
        elif cmd == "project_file":
            url = p.get("url", "")
            nome = url.rsplit("/", 1)[-1]
            if not (Path(self.a.pasta) / nome).is_file():
                self.publicar({"print": {"command": "project_file", "result": "FAIL", "reason": "arquivo nao encontrado",
                                         "sequence_id": p.get("sequence_id", "0")}})
                return
            self.arquivo, self.estado, self.progresso = nome, "RUNNING", 1
            self.bico, self.mesa = 220, 60
            self.log("impressao_iniciada", arquivo=nome, placa=p.get("param"), ams_mapping=p.get("ams_mapping"), use_ams=p.get("use_ams"))
        elif cmd == "pause" and self.estado == "RUNNING":
            self.estado = "PAUSE"
        elif cmd == "resume" and self.estado == "PAUSE":
            self.estado = "RUNNING"
        elif cmd == "stop" and self.estado in ("RUNNING", "PAUSE"):
            self.estado, self.progresso = "FAILED", 0
            self.bico, self.mesa = 25, 25
        elif cmd == "gcode_line":
            linha = p.get("param", "")
            for tok in linha.split():
                if tok.startswith("S") and ("M104" in linha or "M109" in linha):
                    self.bico = int(float(tok[1:]))
                if tok.startswith("S") and ("M140" in linha or "M190" in linha):
                    self.mesa = int(float(tok[1:]))
        else:
            self.publicar({"print": {"command": cmd, "result": "success", "sequence_id": p.get("sequence_id", "0")}})
        self.publicar(self.status())


def mqtt_servidor(imp: Impressora, ctx: ssl.SSLContext | None):
    a, log = imp.a, imp.log
    srv = socket.create_server((a.ip, a.mqtt), reuse_port=False)

    def atende(raw, end):
        s = ctx.wrap_socket(raw, server_side=True) if ctx else raw
        try:
            p = ler_pacote(s)
            if not p or p[0] & 0xF0 != 0x10:
                return
            corpo = p[1]
            i = 2 + struct.unpack("!H", corpo[:2])[0]
            flags = corpo[i + 1]
            i += 4

            def lstr():
                nonlocal i
                n = struct.unpack("!H", corpo[i:i + 2])[0]
                v = corpo[i + 2:i + 2 + n].decode()
                i += 2 + n
                return v
            cid = lstr()
            user = lstr() if flags & 0x80 else ""
            senha = lstr() if flags & 0x40 else ""
            ok = user == "bblp" and senha == a.codigo
            log("mqtt_conexao", cliente=cid, usuario=user, aceito=ok, de=end[0])
            s.sendall(bytes([0x20, 0x02, 0x00, 0x00 if ok else 0x05]))
            if not ok:
                return
            with imp.mx:
                imp.clientes.append(s)
            while True:
                p = ler_pacote(s)
                if not p:
                    break
                tf, corpo = p
                if tf & 0xF0 == 0x80:
                    n = struct.unpack("!H", corpo[2:4])[0]
                    log("mqtt_assinatura", topico=corpo[4:4 + n].decode())
                    s.sendall(bytes([0x90, 0x03]) + corpo[:2] + b"\x00")
                elif tf & 0xF0 == 0x30:
                    n = struct.unpack("!H", corpo[:2])[0]
                    topico = corpo[2:2 + n].decode()
                    if topico == f"device/{a.serial}/request":
                        try:
                            imp.comando(json.loads(corpo[2 + n:]))
                        except ValueError:
                            log("mqtt_json_invalido")
                elif tf & 0xF0 == 0xC0:
                    s.sendall(b"\xd0\x00")
                elif tf & 0xF0 == 0xE0:
                    break
        except (OSError, ssl.SSLError) as e:
            log("mqtt_erro", erro=str(e))
        finally:
            with imp.mx:
                if s in imp.clientes:
                    imp.clientes.remove(s)
            try:
                s.close()
            except OSError:
                pass
            log("mqtt_desconectado")

    while True:
        raw, end = srv.accept()
        threading.Thread(target=atende, args=(raw, end), daemon=True).start()


# ---------------------------------------------------------------- FTPS implícito
def ftps_servidor(a, log: Log, ctx: ssl.SSLContext | None):
    srv = socket.create_server((a.ip, a.ftps))

    def atende(raw, end):
        s = ctx.wrap_socket(raw, server_side=True) if ctx else raw
        f = s.makefile("rwb", buffering=0)

        def resp(t):
            f.write((t + "\r\n").encode())
        resp("220 Bambu FTP emulado")
        usuario, logado, prot, dados_srv = "", False, "C", None
        try:
            for linha in f:
                linha = linha.decode(errors="replace").strip()
                cmd, _, arg = linha.partition(" ")
                cmd = cmd.upper()
                if cmd == "USER":
                    usuario = arg
                    resp("331 senha")
                elif cmd == "PASS":
                    logado = usuario == "bblp" and arg == a.codigo
                    log("ftps_login", usuario=usuario, aceito=logado)
                    resp("230 ok" if logado else "530 login recusado")
                elif not logado:
                    resp("530 entre primeiro")
                elif cmd == "PBSZ":
                    resp("200 PBSZ=0")
                elif cmd == "PROT":
                    prot = arg.upper()
                    resp("200 PROT " + prot)
                elif cmd in ("TYPE", "MODE", "STRU"):
                    resp("200 ok")
                elif cmd == "PWD":
                    resp('257 "/" diretorio atual')
                elif cmd == "CWD":
                    resp("250 ok")
                elif cmd == "PASV":
                    dados_srv = socket.create_server((a.ip, 0))
                    porta = dados_srv.getsockname()[1]
                    h = a.ip.replace(".", ",")
                    resp(f"227 Entering Passive Mode ({h},{porta // 256},{porta % 256})")
                elif cmd == "EPSV":
                    resp("500 use PASV")
                elif cmd == "SIZE":
                    p = Path(a.pasta) / arg.lstrip("/")
                    resp(f"213 {p.stat().st_size}" if p.is_file() else "550 nao existe")
                elif cmd == "STOR" and dados_srv:
                    resp("150 abrindo dados")
                    dc, _ = dados_srv.accept()
                    if prot == "P" and ctx:
                        dc = ctx.wrap_socket(dc, server_side=True)
                    destino = Path(a.pasta) / Path(arg).name
                    total = 0
                    with open(destino, "wb") as out:
                        while True:
                            b = dc.recv(65536)
                            if not b:
                                break
                            out.write(b)
                            total += len(b)
                    try:
                        dc.close()
                    except OSError:
                        pass
                    dados_srv.close()
                    dados_srv = None
                    log("ftps_arquivo_recebido", arquivo=destino.name, bytes=total)
                    resp("226 recebido")
                elif cmd == "QUIT":
                    resp("221 tchau")
                    break
                else:
                    resp("502 nao implementado")
        except (OSError, ssl.SSLError) as e:
            log("ftps_erro", erro=str(e))
        finally:
            try:
                s.close()
            except OSError:
                pass

    while True:
        raw, end = srv.accept()
        threading.Thread(target=atende, args=(raw, end), daemon=True).start()


# ---------------------------------------------------------------- SSDP
def ssdp_anunciante(a, log: Log):
    msg = ("NOTIFY * HTTP/1.1\r\nHOST: 239.255.255.250:1990\r\nServer: UPnP/1.0\r\n"
           f"Location: {a.ip}\r\nNT: urn:bambulab-com:device:3dprinter:1\r\nNTS: ssdp:alive\r\nUSN: {a.serial}\r\n"
           f"Cache-Control: max-age=1800\r\nDevModel.bambu.com: {a.modelo}\r\nDevName.bambu.com: {a.nome}\r\n"
           "DevSignal.bambu.com: -40\r\nDevConnect.bambu.com: lan\r\nDevBind.bambu.com: free\r\n"
           "Devseclink.bambu.com: secure\r\nDevVersion.bambu.com: 01.07.00.00\r\n\r\n").encode()
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    destino = a.ssdp_destino or "255.255.255.255"
    while True:
        try:
            s.sendto(msg, (destino, a.ssdp))
        except OSError as e:
            log("ssdp_erro", erro=str(e))
        time.sleep(a.ssdp_intervalo)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ip", default="127.0.0.1")
    ap.add_argument("--serial", default="01P00A123456789")
    ap.add_argument("--codigo", default="12345678")
    ap.add_argument("--modelo", default="C11")
    ap.add_argument("--nome", default="P1P emulada Filuna")
    ap.add_argument("--pasta", default=tempfile.mkdtemp(prefix="bambu-sd-"))
    ap.add_argument("--mqtt", type=int, default=8883)
    ap.add_argument("--ftps", type=int, default=990)
    ap.add_argument("--ssdp", type=int, default=2021)
    ap.add_argument("--ssdp-destino", default="")
    ap.add_argument("--ssdp-intervalo", type=float, default=2.0)
    ap.add_argument("--sem-tls", action="store_true")
    ap.add_argument("--log")
    a = ap.parse_args()
    os.makedirs(a.pasta, exist_ok=True)
    log = Log(a.log)
    ctx = None
    if not a.sem_tls:
        crt, key = certificado(Path(a.pasta).parent)
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.load_cert_chain(crt, key)
    imp = Impressora(a, log)
    for alvo, args in ((mqtt_servidor, (imp, ctx)), (ftps_servidor, (a, log, ctx)), (ssdp_anunciante, (a, log))):
        threading.Thread(target=alvo, args=args, daemon=True).start()
    log("pronto", ip=a.ip, serial=a.serial, mqtt=a.mqtt, ftps=a.ftps, ssdp=a.ssdp, pasta=a.pasta, tls=not a.sem_tls)

    def tique():  # progresso enquanto imprime
        while True:
            time.sleep(1)
            if imp.estado == "RUNNING":
                imp.progresso = min(100, imp.progresso + 5)
                if imp.progresso >= 100:
                    imp.estado = "FINISH"
                imp.publicar(imp.status())
    threading.Thread(target=tique, daemon=True).start()
    while True:
        time.sleep(3600)


if __name__ == "__main__":
    main()
