#!/usr/bin/env python3
"""Emulador da API PrusaLink v1 (Prusa MK4/MK4S, CORE One, XL, MINI) com autenticação HTTP Digest
(RFC 2617, qop=auth): GET /api/version, GET /api/v1/info, GET /api/v1/status,
PUT /api/v1/files/usb/<arquivo> (Print-After-Upload), PUT /api/v1/job/<id>/pause|resume,
DELETE /api/v1/job/<id>. Não é firmware: não imprime."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import secrets
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote


def log(a, ev, **d):
    linha = json.dumps({"t": round(time.time(), 3), "evento": ev, **d}, ensure_ascii=False)
    print(linha, flush=True)
    if a.log:
        with open(a.log, "a", encoding="utf-8") as f:
            f.write(linha + "\n")


def md5(s):
    return hashlib.md5(s.encode()).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ip", default="127.0.0.1")
    ap.add_argument("--porta", type=int, default=80)
    ap.add_argument("--usuario", default="maker")
    ap.add_argument("--senha", default="Pr0sa1ink")
    ap.add_argument("--pasta", default="/tmp/prusa")
    ap.add_argument("--log")
    a = ap.parse_args()
    Path(a.pasta).mkdir(parents=True, exist_ok=True)
    st = {"state": "IDLE", "job": None, "prog": 0.0, "bico": 25.0, "mesa": 25.0}
    realm, nonce = "Printer API", secrets.token_hex(8)

    class H(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *x):
            pass

        def resp(self, code, obj=None, extra=None):
            d = json.dumps(obj).encode() if obj is not None else b""
            self.send_response(code)
            for k, v in (extra or {}).items():
                self.send_header(k, v)
            if obj is not None:
                self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(d)))
            self.end_headers()
            self.wfile.write(d)

        def autentica(self, metodo):
            h = self.headers.get("Authorization", "")
            if h.startswith("Digest "):
                f = dict(re.findall(r'(\w+)="?([^",]+)"?', h[7:]))
                ha1 = md5(f"{a.usuario}:{realm}:{a.senha}")
                ha2 = md5(f"{metodo}:{f.get('uri', '')}")
                esp = md5(f"{ha1}:{nonce}:{f.get('nc')}:{f.get('cnonce')}:auth:{ha2}") if f.get("qop") == "auth" else md5(f"{ha1}:{nonce}:{ha2}")
                if f.get("username") == a.usuario and f.get("nonce") == nonce and f.get("response") == esp:
                    return True
            log(a, "autenticacao_recusada", caminho=self.path)
            n = int(self.headers.get("Content-Length", 0) or 0)
            if n:
                self.rfile.read(n)
            self.resp(401, {"title": "Unauthorized"}, {"WWW-Authenticate": f'Digest realm="{realm}", nonce="{nonce}", qop="auth"'})
            return False

        def status(self):
            out = {"printer": {"state": st["state"], "temp_nozzle": st["bico"], "target_nozzle": st["bico"], "temp_bed": st["mesa"],
                               "target_bed": st["mesa"], "axis_z": 0.2, "speed": 100, "flow": 100}}
            if st["job"]:
                out["job"] = {"id": st["job"], "progress": st["prog"] * 100, "time_remaining": int((1 - st["prog"]) * 600)}
            return out

        def do_GET(self):
            if not self.autentica("GET"):
                return
            if self.path == "/api/version":
                return self.resp(200, {"api": "2.0.0", "server": "2.1.2", "text": "PrusaLink", "hostname": "prusa-emulada"})
            if self.path == "/api/v1/info":
                return self.resp(200, {"serial": "SN-EMU-MK4S", "hostname": "prusa-emulada", "nozzle_diameter": 0.4, "mmu": False})
            if self.path == "/api/v1/status":
                return self.resp(200, self.status())
            self.resp(404, {"title": "Not Found"})

        def do_PUT(self):
            if not self.autentica("PUT"):
                return
            m = re.match(r"/api/v1/files/usb/(.+)$", self.path)
            if m:
                n = int(self.headers.get("Content-Length", 0))
                dado = self.rfile.read(n)
                nome = unquote(m.group(1))
                (Path(a.pasta) / nome).write_bytes(dado)
                imprimir = self.headers.get("Print-After-Upload", "?0") in ("?1", "1", "true")
                log(a, "arquivo_recebido", arquivo=nome, bytes=len(dado), sha256=hashlib.sha256(dado).hexdigest(), imprimir=imprimir)
                if imprimir:
                    st.update(state="PRINTING", job=st["job"] + 1 if st["job"] else 101, prog=0.01, bico=215.0, mesa=60.0)
                    log(a, "impressao_iniciada", arquivo=nome, job=st["job"])
                return self.resp(201)
            m = re.match(r"/api/v1/job/(\d+)/(pause|resume)$", self.path)
            if m and st["job"] == int(m.group(1)):
                if m.group(2) == "pause" and st["state"] == "PRINTING":
                    st["state"] = "PAUSED"
                elif m.group(2) == "resume" and st["state"] == "PAUSED":
                    st["state"] = "PRINTING"
                else:
                    return self.resp(409, {"title": "Conflict"})
                log(a, "controle", acao=m.group(2))
                return self.resp(204)
            self.resp(404, {"title": "Not Found"})

        def do_DELETE(self):
            if not self.autentica("DELETE"):
                return
            m = re.match(r"/api/v1/job/(\d+)$", self.path)
            if m and st["job"] == int(m.group(1)) and st["state"] in ("PRINTING", "PAUSED"):
                st.update(state="STOPPED", prog=0.0, bico=25.0, mesa=25.0)
                log(a, "controle", acao="stop")
                return self.resp(204)
            self.resp(409, {"title": "Conflict"})

    threading.Thread(target=ThreadingHTTPServer((a.ip, a.porta), H).serve_forever, daemon=True).start()
    log(a, "pronto")
    while True:
        time.sleep(1)
        if st["state"] == "PRINTING":
            st["prog"] = min(1.0, st["prog"] + 0.05)
            if st["prog"] >= 1.0:
                st["state"] = "FINISHED"


if __name__ == "__main__":
    main()
