#!/usr/bin/env python3
"""Emulador da API local da Flashforge (AD5M/AD5X/Creator 5), porta HTTP 8898, e da descoberta
UDP 48899 (resposta binária: nome em 0x00, série em 0x92, 0xC4 bytes). Rotas: POST /detail,
POST /uploadGcode (multipart + cabeçalhos serialNumber/checkCode/printNow/materialMappings),
POST /control (jobCtl_cmd pause/continue/cancel). Estação de material IFS com 4 slots.
Não é firmware: não imprime."""
from __future__ import annotations

import argparse
import base64
import cgi
import hashlib
import io
import json
import socket
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


def log(a, ev, **d):
    linha = json.dumps({"t": round(time.time(), 3), "evento": ev, **d}, ensure_ascii=False)
    print(linha, flush=True)
    if a.log:
        with open(a.log, "a", encoding="utf-8") as f:
            f.write(linha + "\n")


class FF:
    def __init__(self, a):
        self.a, self.status, self.prog, self.arquivo = a, "ready", 0.0, ""
        self.bico, self.mesa = 25.0, 25.0
        self.arquivos = {}

    def detail(self):
        cores = ["#E60012", "#1565C0", "#FFD600", "#15B857"]
        return {"code": 0, "message": "Success", "detail": {
            "status": self.status, "printProgress": self.prog, "printFileName": self.arquivo, "estimatedTime": (1 - self.prog) * 600,
            "printLayer": int(self.prog * 50), "targetPrintLayer": 50, "rightTemp": self.bico, "rightTargetTemp": self.bico,
            "platTemp": self.mesa, "platTargetTemp": self.mesa, "name": self.a.nome, "firmwareVersion": "3.1.3-emulador",
            "cameraStreamUrl": f"http://{self.a.ip}:8080/?action=stream", "hasMatlStation": True,
            "matlStationInfo": {"slotCnt": 4, "currentSlot": 1, "currentLoadSlot": 0, "stateAction": 0, "stateStep": 0,
                                "slotInfos": [{"slotId": i + 1, "hasFilament": True, "materialName": "PLA", "materialColor": cores[i]} for i in range(4)]}}}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ip", default="127.0.0.1")
    ap.add_argument("--serial", default="SNMQRE9400951")
    ap.add_argument("--codigo", default="aa11bb22")
    ap.add_argument("--nome", default="AD5X emulada Filuna")
    ap.add_argument("--http", type=int, default=8898)
    ap.add_argument("--descoberta", type=int, default=48899)
    ap.add_argument("--pasta", default="/tmp/ff")
    ap.add_argument("--log")
    a = ap.parse_args()
    Path(a.pasta).mkdir(parents=True, exist_ok=True)
    ff = FF(a)

    def descoberta():
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(("0.0.0.0", a.descoberta))
        while True:
            d, de = s.recvfrom(1024)
            if d[:4] != b"www.":
                continue
            r = bytearray(0xC4)
            r[0:len(a.nome)] = a.nome.encode()
            r[0x92:0x92 + len(a.serial)] = a.serial.encode()
            log(a, "descoberta", de=f"{de[0]}:{de[1]}")
            s.sendto(bytes(r), de)

    class H(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *x):
            pass

        def r(self, obj):
            d = json.dumps(obj).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(d)))
            self.end_headers()
            self.wfile.write(d)

        def do_POST(self):
            n = int(self.headers.get("Content-Length", 0))
            corpo = self.rfile.read(n)
            if self.path == "/uploadGcode":
                if self.headers.get("serialNumber") != a.serial or self.headers.get("checkCode") != a.codigo:
                    log(a, "upload_recusado")
                    return self.r({"code": 1, "message": "check code error"})
                fs = cgi.FieldStorage(fp=io.BytesIO(corpo), headers=self.headers, environ={"REQUEST_METHOD": "POST", "CONTENT_TYPE": self.headers["Content-Type"]})
                it = fs["gcodeFile"]
                nome, dados = Path(it.filename).name, it.file.read()
                (Path(a.pasta) / nome).write_bytes(dados)
                maps = json.loads(base64.b64decode(self.headers.get("materialMappings", "") or "W10=") or b"[]")
                log(a, "arquivo_recebido", arquivo=nome, bytes=len(dados), sha256=hashlib.sha256(dados).hexdigest(),
                    printNow=self.headers.get("printNow"), useMatlStation=self.headers.get("useMatlStation"), materialMappings=maps)
                if self.headers.get("printNow") == "true":
                    ff.status, ff.prog, ff.arquivo, ff.bico, ff.mesa = "printing", 0.01, nome, 220.0, 60.0
                    log(a, "impressao_iniciada", arquivo=nome)
                return self.r({"code": 0, "message": "Success"})
            try:
                req = json.loads(corpo or b"{}")
            except ValueError:
                return self.r({"code": 2, "message": "json"})
            if req.get("serialNumber") != a.serial or req.get("checkCode") != a.codigo:
                log(a, "recusado", rota=self.path)
                return self.r({"code": 1, "message": "check code error"})
            if self.path == "/detail":
                return self.r(ff.detail())
            if self.path == "/control":
                p = req.get("payload") or {}
                log(a, "controle", payload=p)
                if p.get("cmd") == "jobCtl_cmd":
                    act = (p.get("args") or {}).get("action")
                    if act == "pause" and ff.status == "printing":
                        ff.status = "paused"
                    elif act == "continue" and ff.status == "paused":
                        ff.status = "printing"
                    elif act == "cancel":
                        ff.status, ff.prog, ff.bico, ff.mesa = "cancel", 0.0, 25.0, 25.0
                    return self.r({"code": 0, "message": "Success"})
                return self.r({"code": 3, "message": "unsupported"})
            self.r({"code": 404, "message": "not found"})

    threading.Thread(target=descoberta, daemon=True).start()
    threading.Thread(target=ThreadingHTTPServer((a.ip, a.http), H).serve_forever, daemon=True).start()
    log(a, "pronto", serial=a.serial)
    while True:
        time.sleep(1)
        if ff.status == "printing":
            ff.prog = min(1.0, ff.prog + 0.05)
            if ff.prog >= 1.0:
                ff.status = "completed"


if __name__ == "__main__":
    main()
