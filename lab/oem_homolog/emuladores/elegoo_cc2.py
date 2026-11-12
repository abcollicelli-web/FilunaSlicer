#!/usr/bin/env python3
"""Emulador LAN da Elegoo Centauri Carbon 2, pelo protocolo do SDK aberto Elegoo-Link (Apache-2.0,
adaptador elegoo_fdm_cc2):
  - descoberta: UDP 52700, pedido {"id":0,"method":7000} -> {"id":0,"result":{host_name, machine_model, sn, ...}}
  - MQTT 1883 (usuário "elegoo", senha = código de acesso): elegoo/<sn>/api_register ->
    elegoo/<sn>/<request_id>/register_response; elegoo/<sn>/<cliente>/api_request -> .../api_response;
    estado empurrado em elegoo/<sn>/api_status (método 6000)
  - HTTP PUT /upload em pedaços com Content-Range, X-File-Name, X-File-MD5 e X-Token
Métodos: 1001 atributos, 1002 estado, 1020 iniciar, 1021 pausar, 1022 parar, 1023 retomar,
2005 canvas (4 bandejas). Não é firmware: não imprime.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import socket
import struct
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


def log(a, evento, **d):
    linha = json.dumps({"t": round(time.time(), 3), "evento": evento, **d}, ensure_ascii=False)
    print(linha, flush=True)
    if a.log:
        with open(a.log, "a", encoding="utf-8") as f:
            f.write(linha + "\n")


def rl(n):
    out = bytearray()
    while True:
        b, n = n % 128, n // 128
        out.append(b | (0x80 if n else 0))
        if not n:
            return bytes(out)


def mstr(s):
    b = s.encode()
    return struct.pack("!H", len(b)) + b


def ler(sock):
    c = sock.recv(1)
    if not c:
        return None
    m, n = 1, 0
    while True:
        b = sock.recv(1)
        if not b:
            return None
        n += (b[0] & 127) * m
        m *= 128
        if not b[0] & 128:
            break
    d = b""
    while len(d) < n:
        p = sock.recv(n - len(d))
        if not p:
            return None
        d += p
    return c[0], d


class CC2:
    def __init__(self, a):
        self.a = a
        self.mx = threading.Lock()
        self.clientes = []
        self.estado, self.sub, self.prog, self.arquivo = 1, 0, 0, ""
        self.bico, self.mesa = 25, 25
        self.arquivos: dict[str, dict] = {}
        self.uploads: dict[str, bytearray] = {}

    def status(self):
        return {"error_code": 0, "machine_status": {"status": self.estado, "sub_status": self.sub},
                "print_status": {"progress": self.prog, "filename": self.arquivo, "current_layer": self.prog // 2, "total_layer": 50,
                                 "remaining_time_sec": (100 - self.prog) * 6},
                "extruder": {"temperature": self.bico, "target": self.bico}, "heater_bed": {"temperature": self.mesa, "target": self.mesa}}

    def canvas(self):
        cores = ["E60012", "1565C0", "FFD600", "15B857"]
        return {"error_code": 0, "canvas_info": {"active_tray_id": 0, "canvas_list": [{"canvas_id": 0, "connected": 1,
                "tray_list": [{"tray_id": i, "status": 1, "filament_type": "PLA", "filament_color": cores[i], "brand": "ELEGOO"} for i in range(4)]}]}}

    def pub(self, topico, obj):
        d = bytes([0x30]) + rl(2 + len(topico.encode()) + len(json.dumps(obj).encode())) + mstr(topico) + json.dumps(obj).encode()
        with self.mx:
            for c in list(self.clientes):
                try:
                    c.sendall(d)
                except OSError:
                    self.clientes.remove(c)

    def empurrar(self):
        self.pub(f"elegoo/{self.a.serial}/api_status", {"id": 0, "method": 6000, "result": self.status()})

    def metodo(self, cid, req):
        m, p = int(req.get("method", 0)), req.get("params") or {}
        log(self.a, "metodo", metodo=m, params=p)
        if m == 1001:
            r = {"error_code": 0, "machine_model": "Centauri Carbon 2", "sn": self.a.serial, "firmware_version": "1.0.0-emulador",
                 "protocol_version": "1.0"}
        elif m == 1002:
            r = self.status()
        elif m == 2005:
            r = self.canvas()
        elif m == 1020:
            nome = p.get("filename", "")
            if nome not in self.arquivos:
                r = {"error_code": 1003, "error_message": "file not found"}
            else:
                self.estado, self.sub, self.prog, self.arquivo = 2, 0, 1, nome
                self.bico, self.mesa = 220, 60
                log(self.a, "impressao_iniciada", arquivo=nome, slot_map=(p.get("config") or {}).get("slot_map"))
                r = {"error_code": 0}
        elif m == 1021 and self.estado == 2:
            self.sub, r = 2501, {"error_code": 0}
        elif m == 1023 and self.estado == 2 and self.sub == 2501:
            self.sub, r = 0, {"error_code": 0}
        elif m == 1022 and self.estado == 2:
            self.estado, self.sub, self.prog = 1, 0, 0
            self.bico, self.mesa = 25, 25
            r = {"error_code": 0}
        else:
            r = {"error_code": 1001, "error_message": "unsupported"}
        self.pub(f"elegoo/{self.a.serial}/{cid}/api_response", {"id": req.get("id"), "method": m, "result": r})
        if m in (1020, 1021, 1022, 1023):
            self.empurrar()


def mqtt(cc: CC2):
    a = cc.a
    srv = socket.create_server((a.ip, a.mqtt))

    def atende(s):
        try:
            p = ler(s)
            if not p or p[0] >> 4 != 1:
                return
            d = p[1]
            i = 2 + struct.unpack("!H", d[:2])[0]
            fl = d[i + 1]
            i += 4

            def ls():
                nonlocal i
                n = struct.unpack("!H", d[i:i + 2])[0]
                v = d[i + 2:i + 2 + n].decode()
                i += 2 + n
                return v
            cid, u = ls(), ""
            u = ls() if fl & 0x80 else ""
            pw = ls() if fl & 0x40 else ""
            ok = u == "elegoo" and pw == a.codigo
            log(a, "mqtt_conexao", cliente=cid, usuario=u, aceito=ok)
            s.sendall(bytes([0x20, 2, 0, 0 if ok else 5]))
            if not ok:
                return
            with cc.mx:
                cc.clientes.append(s)
            while True:
                p = ler(s)
                if not p:
                    break
                t, d = p
                if t >> 4 == 8:
                    s.sendall(bytes([0x90, 3]) + d[:2] + b"\0")
                elif t >> 4 == 3:
                    n = struct.unpack("!H", d[:2])[0]
                    top = d[2:2 + n].decode()
                    req = json.loads(d[2 + n:])
                    base = f"elegoo/{a.serial}/"
                    if top == base + "api_register":
                        log(a, "registro", req=req)
                        cc.pub(base + f"{req.get('request_id')}/register_response", {"client_id": req.get("client_id"), "error": "ok"})
                    elif top.startswith(base) and top.endswith("/api_request"):
                        cc.metodo(top[len(base):].split("/")[0], req)
                elif t >> 4 == 12:
                    s.sendall(b"\xd0\0")
                elif t >> 4 == 14:
                    break
        except OSError:
            pass
        finally:
            with cc.mx:
                if s in cc.clientes:
                    cc.clientes.remove(s)
            s.close()

    while True:
        s, _ = srv.accept()
        threading.Thread(target=atende, args=(s,), daemon=True).start()


def descoberta(cc: CC2):
    a = cc.a
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("0.0.0.0", a.descoberta))
    while True:
        d, de = s.recvfrom(2048)
        try:
            r = json.loads(d)
        except ValueError:
            continue
        if r.get("method") == 7000:
            log(a, "descoberta", de=de[0])
            s.sendto(json.dumps({"id": r.get("id", 0), "result": {"host_name": a.nome, "machine_model": "Centauri Carbon 2",
                                                                  "sn": a.serial, "token_status": 1, "lan_status": 1}}).encode(), de)


def http(cc: CC2):
    a = cc.a

    class H(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *x):
            pass

        def r(self, obj, code=200):
            d = json.dumps(obj).encode()
            self.send_response(code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(d)))
            self.end_headers()
            self.wfile.write(d)

        def do_PUT(self):
            if self.path.split("?")[0] != "/upload":
                return self.r({"error_code": 404}, 404)
            if self.headers.get("X-Token") != a.codigo:
                log(a, "upload_recusado")
                return self.r({"error_code": 401, "error_message": "token"}, 401)
            n = int(self.headers.get("Content-Length", 0))
            dado = self.rfile.read(n)
            nome, md5 = self.headers.get("X-File-Name", ""), self.headers.get("X-File-MD5", "")
            ini, resto = self.headers.get("Content-Range", "bytes 0-0/0")[6:].split("-")
            fim, total = resto.split("/")
            buf = cc.uploads.setdefault(nome, bytearray())
            if int(ini) != len(buf):
                return self.r({"error_code": 1, "error_message": "offset"}, 400)
            buf += dado
            if len(buf) >= int(total):
                ok = hashlib.md5(buf).hexdigest() == md5
                if ok:
                    (Path(a.pasta) / nome).write_bytes(bytes(buf))
                    cc.arquivos[nome] = {"bytes": len(buf)}
                log(a, "arquivo_recebido", arquivo=nome, bytes=len(buf), md5_ok=ok)
                del cc.uploads[nome]
                if not ok:
                    return self.r({"error_code": 2, "error_message": "md5"}, 400)
            self.r({"error_code": 0})
    ThreadingHTTPServer((a.ip, a.http), H).serve_forever()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ip", default="127.0.0.1")
    ap.add_argument("--serial", default="F01ELEGOOCC2EMU01")
    ap.add_argument("--codigo", default="123456")
    ap.add_argument("--nome", default="CC2 emulada Filuna")
    ap.add_argument("--mqtt", type=int, default=1883)
    ap.add_argument("--http", type=int, default=80)
    ap.add_argument("--descoberta", type=int, default=52700)
    ap.add_argument("--pasta", default="/tmp/cc2")
    ap.add_argument("--log")
    a = ap.parse_args()
    Path(a.pasta).mkdir(parents=True, exist_ok=True)
    cc = CC2(a)
    for f in (mqtt, descoberta, http):
        threading.Thread(target=f, args=(cc,), daemon=True).start()
    log(a, "pronto", serial=a.serial)
    while True:
        time.sleep(1)
        if cc.estado == 2 and cc.sub == 0:
            cc.prog = min(100, cc.prog + 5)
            if cc.prog >= 100:
                cc.estado, cc.prog = 1, 100
            cc.empurrar()


if __name__ == "__main__":
    main()
