#!/usr/bin/env python3
"""Emulador de impressora Klipper/Moonraker para os ensaios de rede do Filuna (só biblioteca padrão).

Serve a API HTTP e o websocket JSON-RPC do Moonraker que os agentes nativos do Filuna usam
(MoonrakerPrinterAgent e derivados: Snapmaker U1, QIDI, Creality K com Moonraker, Elegoo Neptune,
Anycubic com Klipper):
  GET  /server/info  /printer/info  /printer/objects/list  /printer/objects/query?...
  GET  /server/files/directory?path=gcodes   /server/database/item?namespace=..&key=..
  GET  /server/files/config/<arquivo>        (QIDI: officiall_filas_list.cfg)
  POST /server/files/upload (multipart)      /printer/print/start?filename=
  POST /printer/print/pause|resume|cancel    /printer/gcode/script?script=
  WS   /websocket: server.connection.identify, printer.objects.subscribe, notify_status_update
Perfis (--perfil): generico, snapmaker-u1 (4 cabeças), qidi-box (QIDI Box 4 slots),
anycubic-ace (ACE Pro 4 slots), elegoo-neptune. Estado: standby -> printing -> paused -> printing
-> cancelled/complete; PAUSE/RESUME/CANCEL_PRINT e M104/M140 também chegam por G-code.
Registra cada evento em JSON por linha (--log) para a prova. Não é firmware.
"""
from __future__ import annotations

import argparse
import base64
import cgi
import hashlib
import io
import json
import socket
import struct
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, unquote, urlparse

GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
CORES = ["#E60012", "#1565C0", "#FFD600", "#15B857"]


class Estado:
    def __init__(self, a):
        self.a = a
        self.mx = threading.Lock()
        self.ws: list = []
        self.arquivo = ""
        self.estado = "standby"
        self.progresso = 0.0
        n = 4 if a.perfil == "snapmaker-u1" else 1
        self.extrusoras = ["extruder"] + [f"extruder{i}" for i in range(1, n)]
        self.temp = {e: [25.0, 0.0] for e in self.extrusoras}
        self.mesa = [25.0, 0.0]
        self.arquivos: dict[str, int] = {}

    def objetos(self) -> list[str]:
        base = ["print_stats", "virtual_sdcard", "display_status", "heater_bed", "fan", "toolhead", "webhooks", "configfile",
                "gcode_move", "idle_timeout", *self.extrusoras]
        if self.a.perfil == "snapmaker-u1":
            base += ["toolchanger", *[f"tool T{i}" for i in range(4)], "filament_detect", "print_task_config"]
        if self.a.perfil == "qidi-box":
            base += ["save_variables", "multi_color_controller", *[f"box_stepper slot{i}" for i in range(4)]]
        if self.a.perfil == "anycubic-ace":
            base += ["ace"]
        return base

    def status(self, pedidos: dict | None = None) -> dict:
        st = {
            "print_stats": {"state": self.estado, "filename": self.arquivo, "print_duration": 10.0 * self.progresso,
                            "total_duration": 10.0 * self.progresso, "filament_used": 0.0, "message": "",
                            "info": {"current_layer": int(self.progresso * 50), "total_layer": 50}},
            "virtual_sdcard": {"progress": self.progresso, "is_active": self.estado == "printing", "file_path": self.arquivo},
            "display_status": {"progress": self.progresso, "message": ""},
            "heater_bed": {"temperature": self.mesa[0], "target": self.mesa[1], "power": 0.0},
            "fan": {"speed": 0.0},
            "toolhead": {"homed_axes": "xyz", "position": [0, 0, 0, 0], "extruder": "extruder"},
            "webhooks": {"state": "ready", "state_message": "Printer is ready"},
            "idle_timeout": {"state": "Printing" if self.estado == "printing" else "Idle"},
        }
        for e in self.extrusoras:
            st[e] = {"temperature": self.temp[e][0], "target": self.temp[e][1], "power": 0.0, "can_extrude": True}
        if self.a.perfil == "snapmaker-u1":
            st["toolchanger"] = {"status": "ready", "tool": "T0", "tool_number": 0, "tool_numbers": [0, 1, 2, 3],
                                 "tool_names": ["T0", "T1", "T2", "T3"]}
            for i in range(4):
                st[f"tool T{i}"] = {"active": i == 0, "extruder": self.extrusoras[i], "tool_number": i, "mounted": i == 0}
            st["print_task_config"] = {"filament_exist": [True] * 4, "filament_type": ["PLA"] * 4, "filament_sub_type": ["Basic"] * 4,
                                       "filament_color_rgba": [c[1:] + "FF" for c in CORES], "filament_vendor": ["Snapmaker"] * 4}
            st["filament_detect"] = {"info": [{"VENDOR": "Snapmaker", "MAIN_TYPE": "PLA", "RGB_1": int(CORES[i][1:], 16),
                                               "HOTEND_MIN_TEMP": 190, "HOTEND_MAX_TEMP": 230} for i in range(4)]}
        if self.a.perfil == "qidi-box":
            st["save_variables"] = {"variables": {**{f"filament_slot{i}": 1 for i in range(4)},
                                                  **{f"color_slot{i}": i + 1 for i in range(4)}, **{f"vendor_slot{i}": 0 for i in range(4)},
                                                  "box_count": 1}}
            st["multi_color_controller"] = {"box": {}}
            for i in range(4):
                st[f"box_stepper slot{i}"] = {"runout_button": 0}
        if self.a.perfil == "anycubic-ace":
            st["ace"] = {"status": "ready", "slots": [{"index": i, "status": "ready", "type": "PLA", "color": [int(CORES[i][j:j + 2], 16) for j in (1, 3, 5)]}
                                                      for i in range(4)]}
        if pedidos:
            return {k: (v if not pedidos.get(k) else {c: v.get(c) for c in pedidos[k] if c in v}) for k, v in st.items() if k in pedidos}
        return st


def log_evento(a, evento, **dados):
    linha = json.dumps({"t": round(time.time(), 3), "evento": evento, **dados}, ensure_ascii=False)
    print(linha, flush=True)
    if a.log:
        with open(a.log, "a", encoding="utf-8") as f:
            f.write(linha + "\n")


# ------------------------------------------------------------- websocket mínimo (RFC 6455)
def ws_ler(sock) -> tuple[int, bytes] | None:
    def ler(n):
        b = b""
        while len(b) < n:
            p = sock.recv(n - len(b))
            if not p:
                raise ConnectionError
            b += p
        return b
    try:
        h = ler(2)
        op, n = h[0] & 0x0F, h[1] & 0x7F
        if n == 126:
            n = struct.unpack("!H", ler(2))[0]
        elif n == 127:
            n = struct.unpack("!Q", ler(8))[0]
        mask = ler(4) if h[1] & 0x80 else b"\0\0\0\0"
        dado = bytes(b ^ mask[i % 4] for i, b in enumerate(ler(n)))
        return op, dado
    except (ConnectionError, OSError):
        return None


def ws_enviar(sock, texto: str, op: int = 1):
    d = texto.encode()
    n = len(d)
    cab = bytes([0x80 | op]) + (bytes([n]) if n < 126 else bytes([126]) + struct.pack("!H", n) if n < 65536 else bytes([127]) + struct.pack("!Q", n))
    sock.sendall(cab + d)


def avisar(e: Estado):
    msg = json.dumps({"jsonrpc": "2.0", "method": "notify_status_update", "params": [e.status(), time.time()]})
    with e.mx:
        for s in list(e.ws):
            try:
                ws_enviar(s, msg)
            except OSError:
                e.ws.remove(s)


def gcode(e: Estado, script: str):
    a = e.a
    for linha in script.splitlines():
        p = linha.strip().split()
        if not p:
            continue
        c = p[0].upper()
        args = {x[0].upper(): x[1:] for x in p[1:] if len(x) > 1}
        if c == "SDCARD_PRINT_FILE":
            import re as _re
            m = _re.search(r'FILENAME="?([^"]+)"?', linha, _re.I)
            nome = m.group(1) if m else ""
            if nome in e.arquivos:
                e.arquivo, e.estado, e.progresso = nome, "printing", 0.01
                e.temp[e.extrusoras[0]] = [220.0, 220.0]
                e.mesa = [60.0, 60.0]
                log_evento(a, "impressao_iniciada", arquivo=nome, via="SDCARD_PRINT_FILE")
            else:
                log_evento(a, "arquivo_inexistente", arquivo=nome)
        elif c == "PAUSE" and e.estado == "printing":
            e.estado = "paused"
        elif c == "RESUME" and e.estado == "paused":
            e.estado = "printing"
        elif c in ("CANCEL_PRINT", "SDCARD_RESET_FILE") and e.estado in ("printing", "paused"):
            e.estado, e.progresso = "cancelled", 0.0
        elif c in ("M104", "M109") and "S" in args:
            alvo = e.extrusoras[int(args.get("T", "0"))] if args.get("T", "0").isdigit() and int(args.get("T", "0")) < len(e.extrusoras) else "extruder"
            e.temp[alvo] = [float(args["S"]), float(args["S"])]
        elif c in ("M140", "M190") and "S" in args:
            e.mesa = [float(args["S"]), float(args["S"])]
        elif c == "SET_HEATER_TEMPERATURE":
            h = {x.split("=")[0].upper(): x.split("=")[1] for x in p[1:] if "=" in x}
            if h.get("HEATER") == "heater_bed":
                e.mesa = [float(h.get("TARGET", 0))] * 2
            elif h.get("HEATER") in e.temp:
                e.temp[h["HEATER"]] = [float(h.get("TARGET", 0))] * 2
    log_evento(a, "gcode", script=script, estado=e.estado)
    avisar(e)


def fabrica_handler(e: Estado):
    a = e.a

    class H(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *x):
            pass

        def responder(self, obj, code=200):
            d = json.dumps(obj).encode()
            self.send_response(code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(d)))
            self.end_headers()
            self.wfile.write(d)

        def autorizado(self) -> bool:
            if not a.api_key:
                return True
            if self.headers.get("X-Api-Key") == a.api_key:
                return True
            self.responder({"error": {"code": 401, "message": "Unauthorized"}}, 401)
            log_evento(a, "http_recusado", caminho=self.path)
            return False

        def do_GET(self):
            u = urlparse(self.path)
            q = parse_qs(u.query, keep_blank_values=True)
            if u.path == "/websocket" and self.headers.get("Upgrade", "").lower() == "websocket":
                return self.websocket()
            if not self.autorizado():
                return
            log_evento(a, "http", metodo="GET", caminho=u.path)
            if u.path == "/server/info":
                return self.responder({"result": {"klippy_connected": True, "klippy_state": "ready", "components": ["file_manager", "database"],
                                                  "moonraker_version": "v0.9.3-filuna-emulador", "api_version": [1, 5, 0]}})
            if u.path == "/printer/info":
                return self.responder({"result": {"state": "ready", "hostname": a.nome, "software_version": "v0.12.0-emulador",
                                                  "machine": a.modelo, "app": a.perfil}})
            if u.path == "/machine/system_info":
                return self.responder({"result": {"system_info": {"product_info": {"machine_type": a.modelo, "device_name": a.nome}}}})
            if u.path == "/printer/objects/list":
                return self.responder({"result": {"objects": e.objetos()}})
            if u.path == "/printer/objects/query":
                ped = {k: (v[0].split(",") if v and v[0] else None) for k, v in q.items()}
                return self.responder({"result": {"eventtime": time.time(), "status": e.status(ped)}})
            if u.path == "/server/files/directory":
                return self.responder({"result": {"dirs": [], "files": [{"filename": k, "size": v, "modified": time.time()} for k, v in e.arquivos.items()],
                                                  "disk_usage": {"total": 8 << 30, "used": 1 << 30, "free": 7 << 30}, "root_info": {"name": "gcodes", "permissions": "rw"}}})
            if u.path == "/server/database/item":
                ns = q.get("namespace", [""])[0]
                if ns == "lane_data" and a.perfil == "anycubic-ace":
                    # driver comunitário ACEPRO: grava o inventário do ACE em lane_data para o OrcaSlicer
                    val = {f"lane{i + 1}": {"lane": str(i), "color": CORES[i], "material": "PLA", "name": "ACE slot", "vendor": "Anycubic",
                                            "nozzle_temp": 210, "bed_temp": 60} for i in range(4)}
                    log_evento(a, "lane_data")
                    return self.responder({"result": {"namespace": ns, "key": None, "value": val}})
                if ns == "lane_data":
                    return self.responder({"error": {"code": 404, "message": "Namespace 'lane_data' not found"}}, 404)
                return self.responder({"result": {"namespace": ns, "key": q.get("key", [""])[0], "value": {}}})
            if u.path.startswith("/server/files/config/"):
                nome = u.path.rsplit("/", 1)[-1]
                if a.perfil == "qidi-box" and nome == "officiall_filas_list.cfg":
                    corpo = ("[colordict]\n" + "".join(f"{i + 1} = {c}\n" for i, c in enumerate(CORES)) +
                             "[fila1]\nfilament = PLA\nname = QIDI PLA Rapido\n[fila2]\nfilament = PETG\nname = QIDI PETG\n").encode()
                    self.send_response(200)
                    self.send_header("Content-Length", str(len(corpo)))
                    self.end_headers()
                    return self.wfile.write(corpo)
                return self.responder({"error": {"code": 404, "message": "not found"}}, 404)
            if u.path == "/webcam/?action=snapshot" or u.path.startswith("/webcam"):
                corpo = base64.b64decode("/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDAP////////////////////////////////////////////////////////////////////////////////////8AAAsIAAEAAQEBEQD/xAAUAAEAAAAAAAAAAAAAAAAAAAAI/9oACAEBAAA/AFL/2Q==")
                self.send_response(200)
                self.send_header("Content-Type", "image/jpeg")
                self.send_header("Content-Length", str(len(corpo)))
                self.end_headers()
                log_evento(a, "camera_quadro")
                return self.wfile.write(corpo)
            self.responder({"error": {"code": 404, "message": f"{u.path} not found"}}, 404)

        def do_POST(self):
            u = urlparse(self.path)
            q = parse_qs(u.query)
            if not self.autorizado():
                return
            n = int(self.headers.get("Content-Length", "0") or 0)
            corpo = self.rfile.read(n) if n else b""
            log_evento(a, "http", metodo="POST", caminho=u.path)
            if u.path == "/server/files/upload":
                fs = cgi.FieldStorage(fp=io.BytesIO(corpo), headers=self.headers, environ={"REQUEST_METHOD": "POST", "CONTENT_TYPE": self.headers["Content-Type"]})
                item = fs["file"]
                nome = Path(item.filename).name
                dados = item.file.read()
                (Path(a.pasta) / nome).write_bytes(dados)
                e.arquivos[nome] = len(dados)
                log_evento(a, "arquivo_recebido", arquivo=nome, bytes=len(dados), sha256=hashlib.sha256(dados).hexdigest(),
                           imprimir=fs.getfirst("print", ""))
                if fs.getfirst("print", "") == "true":
                    self.iniciar(nome)
                return self.responder({"result": {"item": {"path": nome, "root": "gcodes"}, "print_started": e.estado == "printing", "action": "create_file"}}, 201)
            if u.path == "/printer/print/start":
                nome = unquote(q.get("filename", [""])[0])
                if not nome and corpo:
                    try:
                        nome = json.loads(corpo).get("filename", "")
                    except ValueError:
                        pass
                if nome not in e.arquivos:
                    return self.responder({"error": {"code": 400, "message": f"file {nome} not found"}}, 400)
                self.iniciar(nome)
                return self.responder({"result": "ok"})
            if u.path in ("/printer/print/pause", "/printer/print/resume", "/printer/print/cancel"):
                gcode(e, {"pause": "PAUSE", "resume": "RESUME", "cancel": "CANCEL_PRINT"}[u.path.rsplit("/", 1)[-1]])
                return self.responder({"result": "ok"})
            if u.path == "/printer/gcode/script":
                script = q.get("script", [""])[0]
                if not script and corpo:
                    try:
                        script = json.loads(corpo).get("script", "")
                    except ValueError:
                        pass
                gcode(e, script)
                return self.responder({"result": "ok"})
            if u.path == "/server/database/item":
                return self.responder({"result": {}})
            self.responder({"error": {"code": 404, "message": f"{u.path} not found"}}, 404)

        def iniciar(self, nome):
            e.arquivo, e.estado, e.progresso = nome, "printing", 0.01
            for x in e.extrusoras[:1]:
                e.temp[x] = [220.0, 220.0]
            e.mesa = [60.0, 60.0]
            log_evento(a, "impressao_iniciada", arquivo=nome)
            avisar(e)

        def websocket(self):
            chave = self.headers["Sec-WebSocket-Key"]
            aceite = base64.b64encode(hashlib.sha1((chave + GUID).encode()).digest()).decode()
            self.send_response(101)
            self.send_header("Upgrade", "websocket")
            self.send_header("Connection", "Upgrade")
            self.send_header("Sec-WebSocket-Accept", aceite)
            self.end_headers()
            self.wfile.flush()
            s = self.connection
            log_evento(a, "ws_conectado")
            with e.mx:
                e.ws.append(s)
            try:
                while True:
                    r = ws_ler(s)
                    if r is None:
                        break
                    op, dado = r
                    if op == 8:
                        try:
                            ws_enviar(s, "", 8)
                        except OSError:
                            pass
                        break
                    if op == 9:
                        s.sendall(bytes([0x8A, len(dado)]) + dado)
                        continue
                    if op != 1:
                        continue
                    req = json.loads(dado)
                    m, rid = req.get("method"), req.get("id")
                    log_evento(a, "ws_rpc", metodo=m)
                    if m == "server.connection.identify":
                        res = {"connection_id": 1}
                    elif m == "printer.objects.subscribe":
                        objs = (req.get("params") or {}).get("objects") or {}
                        res = {"eventtime": time.time(), "status": e.status({k: v for k, v in objs.items()})}
                    elif m == "printer.objects.query":
                        res = {"eventtime": time.time(), "status": e.status((req.get("params") or {}).get("objects"))}
                    elif m == "printer.gcode.script":
                        gcode(e, (req.get("params") or {}).get("script", ""))
                        res = "ok"
                    elif m == "server.info":
                        res = {"klippy_state": "ready"}
                    else:
                        res = None
                    ws_enviar(s, json.dumps({"jsonrpc": "2.0", "result": res, "id": rid}))
            finally:
                with e.mx:
                    if s in e.ws:
                        e.ws.remove(s)
                log_evento(a, "ws_desconectado")
                self.close_connection = True
    return H


def creality_extras(e: Estado):
    """K2/K1 com CFS: GET http://ip/info -> {"model"} (porta 80) e websocket 9999 em "/" que responde
    {"method":"get","params":{"boxsInfo":1}} no esquema que o CrealityPrintAgent lê."""
    a = e.a

    class Info(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *x):
            pass

        def do_GET(self):
            if self.path.startswith("/info"):
                d = json.dumps({"model": a.modelo or "F021", "hostname": a.nome, "version": "1.1.2-emulador"}).encode()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(d)))
                self.end_headers()
                log_evento(a, "creality_info")
                return self.wfile.write(d)
            self.send_response(404)
            self.send_header("Content-Length", "0")
            self.end_headers()

    threading.Thread(target=ThreadingHTTPServer((a.ip, a.porta_info), Info).serve_forever, daemon=True).start()
    srv = socket.create_server((a.ip, a.porta_ws))

    def atende(s):
        try:
            pedido = b""
            while b"\r\n\r\n" not in pedido:
                b = s.recv(4096)
                if not b:
                    return
                pedido += b
            chave = [l.split(b":", 1)[1].strip() for l in pedido.split(b"\r\n") if l.lower().startswith(b"sec-websocket-key")][0].decode()
            aceite = base64.b64encode(hashlib.sha1((chave + GUID).encode()).digest()).decode()
            s.sendall(f"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: {aceite}\r\n\r\n".encode())
            ws_enviar(s, json.dumps({"cfsConnect": 1, "state": 0}))
            while True:
                r = ws_ler(s)
                if r is None or r[0] == 8:
                    break
                if r[0] != 1:
                    continue
                txt = r[1].decode(errors="replace")
                if txt.strip() in ("ok", '{"ModeCode":"heart_beat"}'):
                    continue
                req = json.loads(txt)
                log_evento(a, "creality_ws", pedido=req)
                if req.get("method") == "get" and "boxsInfo" in (req.get("params") or {}):
                    mats = [{"id": i, "state": 1, "vendor": "Creality", "type": "PLA", "name": "Hyper PLA", "color": "#0" + CORES[i][1:]} for i in range(4)]
                    ws_enviar(s, json.dumps({"boxsInfo": {"materialBoxs": [{"id": 1, "state": 1, "type": 0, "materials": mats}]}}))
                elif req.get("method") == "set":
                    ws_enviar(s, json.dumps({"result": "ok"}))
        except (OSError, ValueError, IndexError) as x:
            log_evento(a, "creality_ws_erro", erro=str(x))
        finally:
            s.close()

    def laco():
        while True:
            c, _ = srv.accept()
            threading.Thread(target=atende, args=(c,), daemon=True).start()
    threading.Thread(target=laco, daemon=True).start()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ip", default="127.0.0.1")
    ap.add_argument("--porta", type=int, default=7125)
    ap.add_argument("--perfil", default="generico", choices=["generico", "snapmaker-u1", "qidi-box", "anycubic-ace", "elegoo-neptune", "creality-cfs"])
    ap.add_argument("--modelo", default="Klipper")
    ap.add_argument("--nome", default="emulada-filuna")
    ap.add_argument("--api-key", default="")
    ap.add_argument("--pasta", default="/tmp/moonraker-gcodes")
    ap.add_argument("--porta-info", type=int, default=80)
    ap.add_argument("--porta-ws", type=int, default=9999)
    ap.add_argument("--log")
    a = ap.parse_args()
    Path(a.pasta).mkdir(parents=True, exist_ok=True)
    e = Estado(a)
    if a.perfil == "creality-cfs":
        creality_extras(e)
    srv = ThreadingHTTPServer((a.ip, a.porta), fabrica_handler(e))

    def tique():
        while True:
            time.sleep(1)
            if e.estado == "printing":
                e.progresso = min(1.0, e.progresso + 0.05)
                if e.progresso >= 1.0:
                    e.estado = "complete"
                avisar(e)
    threading.Thread(target=tique, daemon=True).start()
    log_evento(a, "pronto", ip=a.ip, porta=a.porta, perfil=a.perfil)
    srv.serve_forever()


if __name__ == "__main__":
    main()
