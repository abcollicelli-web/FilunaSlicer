#!/usr/bin/env python3
"""Emulador SDCP v3 (Elegoo Centauri Carbon): UDP 3000 "M99999" -> dados da placa; porta 3030 com
WebSocket /websocket (sdcp/request -> sdcp/response + sdcp/status) e HTTP POST /uploadFile/upload
(multipart em pedaços: S-File-MD5, Check, Offset, Uuid, TotalSize, File). Comandos 0,1,128-131.
Não é firmware: não imprime."""
from __future__ import annotations

import argparse
import cgi
import hashlib
import io
import json
import socket
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from moonraker import GUID, ws_enviar, ws_ler  # noqa: E402

import base64  # noqa: E402


def log(a, ev, **d):
    linha = json.dumps({"t": round(time.time(), 3), "evento": ev, **d}, ensure_ascii=False)
    print(linha, flush=True)
    if a.log:
        with open(a.log, "a", encoding="utf-8") as f:
            f.write(linha + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ip", default="127.0.0.1")
    ap.add_argument("--porta", type=int, default=3030)
    ap.add_argument("--descoberta", type=int, default=3000)
    ap.add_argument("--mainboard", default="000000000001d354")
    ap.add_argument("--pasta", default="/tmp/sdcp")
    ap.add_argument("--log")
    a = ap.parse_args()
    Path(a.pasta).mkdir(parents=True, exist_ok=True)
    st = {"ps": 0, "prog": 0, "arquivo": "", "bico": 25.0, "mesa": 25.0}
    arquivos, uploads, conexoes = {}, {}, []
    mx = threading.Lock()

    def status():
        return {"Status": {"CurrentStatus": [1 if st["ps"] not in (0, 8, 9) else 0], "PrintInfo": {
            "Status": st["ps"], "CurrentLayer": st["prog"] // 2, "TotalLayer": 50, "CurrentTicks": st["prog"] * 6000, "TotalTicks": 600000,
            "Filename": st["arquivo"], "Progress": st["prog"]}, "TempOfNozzle": st["bico"], "TempTargetNozzle": st["bico"],
            "TempOfHotbed": st["mesa"], "TempTargetHotbed": st["mesa"]}, "MainboardID": a.mainboard, "TimeStamp": int(time.time()),
            "Topic": f"sdcp/status/{a.mainboard}"}

    def empurrar():
        m = json.dumps(status())
        with mx:
            for s in list(conexoes):
                try:
                    ws_enviar(s, m)
                except OSError:
                    conexoes.remove(s)

    def descoberta():
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(("0.0.0.0", a.descoberta))
        while True:
            d, de = s.recvfrom(1024)
            if d.strip() == b"M99999":
                log(a, "descoberta")
                s.sendto(json.dumps({"Id": "emu", "Data": {"Name": "CC emulada Filuna", "MachineName": "Centauri Carbon", "BrandName": "ELEGOO",
                                     "MainboardIP": a.ip, "MainboardID": a.mainboard, "ProtocolVersion": "V3.0.0", "FirmwareVersion": "V1.1.0"}}).encode(), de)

    class H(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *x):
            pass

        def do_GET(self):
            if self.path == "/websocket" and self.headers.get("Upgrade", "").lower() == "websocket":
                aceite = base64.b64encode(hashlib.sha1((self.headers["Sec-WebSocket-Key"] + GUID).encode()).digest()).decode()
                self.send_response(101)
                self.send_header("Upgrade", "websocket")
                self.send_header("Connection", "Upgrade")
                self.send_header("Sec-WebSocket-Accept", aceite)
                self.end_headers()
                self.wfile.flush()
                s = self.connection
                with mx:
                    conexoes.append(s)
                log(a, "ws_conectado")
                try:
                    while True:
                        r = ws_ler(s)
                        if r is None or r[0] == 8:
                            break
                        if r[0] == 9:
                            s.sendall(bytes([0x8A, len(r[1])]) + r[1])
                            continue
                        if r[0] != 1:
                            continue
                        req = json.loads(r[1])
                        d = req.get("Data") or {}
                        cmd, dados = d.get("Cmd"), d.get("Data") or {}
                        log(a, "comando", cmd=cmd, dados=dados)
                        ack = 0
                        if cmd == 128:
                            nome = str(dados.get("Filename", "")).rsplit("/", 1)[-1]
                            if nome in arquivos:
                                st.update(ps=13, prog=1, arquivo=nome, bico=220.0, mesa=60.0)
                                log(a, "impressao_iniciada", arquivo=nome)
                            else:
                                ack = 1
                        elif cmd == 129 and st["ps"] == 13:
                            st["ps"] = 6
                        elif cmd == 131 and st["ps"] == 6:
                            st["ps"] = 13
                        elif cmd == 130 and st["ps"] in (13, 6):
                            st.update(ps=8, prog=0, bico=25.0, mesa=25.0)
                        ws_enviar(s, json.dumps({"Id": req.get("Id"), "Data": {"Cmd": cmd, "Data": {"Ack": ack}, "RequestID": d.get("RequestID"),
                                                 "MainboardID": a.mainboard, "TimeStamp": int(time.time())}, "Topic": f"sdcp/response/{a.mainboard}"}))
                        empurrar()
                finally:
                    with mx:
                        if s in conexoes:
                            conexoes.remove(s)
                    self.close_connection = True
                return
            self.send_response(404)
            self.send_header("Content-Length", "0")
            self.end_headers()

        def do_POST(self):
            n = int(self.headers.get("Content-Length", 0))
            corpo = self.rfile.read(n)
            if self.path != "/uploadFile/upload":
                self.send_response(404)
                self.send_header("Content-Length", "0")
                return self.end_headers()
            fs = cgi.FieldStorage(fp=io.BytesIO(corpo), headers=self.headers, environ={"REQUEST_METHOD": "POST", "CONTENT_TYPE": self.headers["Content-Type"]})
            f = fs["File"]
            nome, parte = Path(f.filename).name, f.file.read()
            uid, off, total, md5 = fs.getfirst("Uuid"), int(fs.getfirst("Offset")), int(fs.getfirst("TotalSize")), fs.getfirst("S-File-MD5")
            buf = uploads.setdefault(uid, bytearray())
            ok = off == len(buf)
            if ok:
                buf += parte
                if len(buf) >= total:
                    ok = hashlib.md5(buf).hexdigest() == md5
                    if ok:
                        (Path(a.pasta) / nome).write_bytes(bytes(buf))
                        arquivos[nome] = len(buf)
                    log(a, "arquivo_recebido", arquivo=nome, bytes=len(buf), md5_ok=ok)
                    del uploads[uid]
            d = json.dumps({"code": "000000" if ok else "000001", "messages": None, "data": None, "success": ok}).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(d)))
            self.end_headers()
            self.wfile.write(d)

    threading.Thread(target=descoberta, daemon=True).start()
    threading.Thread(target=ThreadingHTTPServer((a.ip, a.porta), H).serve_forever, daemon=True).start()
    log(a, "pronto")
    while True:
        time.sleep(1)
        if st["ps"] == 13:
            st["prog"] = min(100, st["prog"] + 5)
            if st["prog"] >= 100:
                st["ps"] = 9
            empurrar()


if __name__ == "__main__":
    main()
