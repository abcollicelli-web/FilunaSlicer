#!/usr/bin/env python3
"""Filuna LAN Lab: printer-like wire-protocol emulators (not firmware emulators).
Python 3.10+, standard library only. Run in isolated Linux network namespaces.
"""
import argparse
import json
import socket
import socketserver
import struct
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

STATE = {"state": "ready", "progress": 0.0, "filename": "", "uploaded": 0}
LOCK = threading.Lock()


def frame(packet_type, body):
    n = len(body)
    encoded = bytearray([packet_type])
    while True:
        b = n % 128
        n //= 128
        encoded.append(b | (128 if n else 0))
        if not n:
            break
    return bytes(encoded) + body


def mqtt_str(s):
    b = s.encode("utf-8")
    return struct.pack("!H", len(b)) + b


def mqtt_field(body, pos):
    if pos + 2 > len(body):
        raise ValueError("truncated MQTT string length")
    n = int.from_bytes(body[pos:pos+2], "big")
    if pos + 2 + n > len(body):
        raise ValueError("truncated MQTT string")
    return body[pos+2:pos+2+n].decode("utf-8"), pos + 2 + n


def read_exact(reader, n):
    b = reader.read(n)
    if b is None or len(b) != n:
        raise EOFError
    return b


def read_mqtt(reader):
    first = reader.read(1)
    if not first:
        raise EOFError
    multi = 1
    n = 0
    for _ in range(4):
        d = read_exact(reader, 1)[0]
        n += (d & 127) * multi
        if d & 128 == 0:
            break
        multi *= 128
    else:
        raise ValueError("invalid remaining length")
    if n > 1024 * 1024:
        raise ValueError("packet too large")
    return first[0], read_exact(reader, n)


def cc2_result(method):
    with LOCK:
        if method == 1020:
            STATE.update(state="printing", progress=1)
        elif method == 1021:
            STATE["state"] = "paused"
        elif method == 1022:
            STATE["state"] = "ready"
        elif method == 1023:
            STATE["state"] = "printing"
        elif method == 1002 and STATE["state"] == "printing":
            STATE["progress"] = min(100, STATE["progress"] + 1)
        state = dict(STATE)
    sub_status = 2077 if state["state"] == "ready" else 2501 if state["state"] == "paused" else 2000
    result = {
        "error_code": 0,
        "machine_status": {"status": 14 if state["state"] == "error" else 2, "sub_status": sub_status},
        "print_status": {"progress": state["progress"], "filename": state["filename"],
                         "current_layer": int(state["progress"]), "total_layer": 100,
                         "remaining_time_sec": int((100-state["progress"])*15)},
        "extruder": {"temperature": 215 if state["state"] == "printing" else 25, "target": 215 if state["state"] == "printing" else 0},
        "heater_bed": {"temperature": 60 if state["state"] == "printing" else 25, "target": 60 if state["state"] == "printing" else 0},
    }
    if method == 2005:
        result["canvas_info"] = {"active_tray_id": 0, "canvas_list": [{"connected": 1, "canvas_id": 0,
            "tray_list": [{"tray_id": 0, "status": 1, "filament_type": "PLA", "filament_color": "FF0000", "brand": "Filuna LAB"}]}]}
    return result


class ThreadedTCP(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True


class TinyCc2Mqtt(socketserver.StreamRequestHandler):
    """Enough MQTT 3.1.1 for Filuna's current read-only status/pairing client.
    Is NOT a general MQTT broker; use Mosquitto for broker conformance tests.
    """
    def handle(self):
        serial = self.server.serial
        try:
            while True:
                code, payload = read_mqtt(self.rfile)
                ptype = code >> 4
                if ptype == 1:
                    proto, p = mqtt_field(payload, 0)
                    if proto != "MQTT" or payload[p] != 4:
                        return
                    flags = payload[p+1]
                    p += 4
                    cid, p = mqtt_field(payload, p)
                    username = password = ""
                    if flags & 0x80:
                        username, p = mqtt_field(payload, p)
                    if flags & 0x40:
                        password, p = mqtt_field(payload, p)
                    ack = 0 if username == "elegoo" and password == self.server.access_code else 4
                    self.wfile.write(bytes([0x20, 2, 0, ack]))
                    self.wfile.flush()
                    if ack:
                        return
                    print("[CC2 MQTT] client authenticated:", cid, flush=True)
                elif ptype == 8:
                    pid = payload[:2]
                    pos = 2
                    grants = []
                    while pos < len(payload):
                        _, pos = mqtt_field(payload, pos)
                        grants.append(0)
                        pos += 1
                    self.wfile.write(frame(0x90, pid + bytes(grants)))
                    self.wfile.flush()
                elif ptype == 3:
                    topic, pos = mqtt_field(payload, 0)
                    qos = (code >> 1) & 3
                    if qos:
                        packet_id = payload[pos:pos+2]
                        pos += 2
                        if qos == 1:
                            self.wfile.write(frame(0x40, packet_id))
                    request = json.loads(payload[pos:])
                    prefix = f"elegoo/{serial}/"
                    if topic == prefix + "api_register":
                        cid = request.get("client_id", "")
                        self.publish(prefix + cid + "/register_response", {"error": "ok"})
                    elif topic.startswith(prefix) and topic.endswith("/api_request"):
                        cid = topic[len(prefix):].split("/")[0]
                        method = int(request.get("method", 0))
                        if method == 1020:
                            with LOCK:
                                STATE["filename"] = str(request.get("params", {}).get("filename", ""))
                        self.publish(prefix + cid + "/api_response", {"id": request.get("id"), "method": method, "result": cc2_result(method)})
                    print("[CC2 MQTT]", topic, request, flush=True)
                elif ptype == 12:
                    self.wfile.write(b"\xd0\x00")
                    self.wfile.flush()
                elif ptype == 14:
                    return
        except (EOFError, BrokenPipeError, ConnectionResetError):
            return
        except Exception as exc:
            print("[CC2 MQTT] client error:", repr(exc), flush=True)

    def publish(self, topic, payload):
        self.wfile.write(frame(0x30, mqtt_str(topic) + json.dumps(payload).encode()))
        self.wfile.flush()


class PrinterHTTP(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        print(f"[HTTP {self.server.server_port}] " + format % args, flush=True)

    def reply(self, obj, status=200):
        data = json.dumps(obj, separators=(",", ":")).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        path = urlparse(self.path)
        with LOCK:
            state = dict(STATE)
        if path.path == "/__lab/state":
            self.reply(state)
        elif path.path == "/printer/info":
            self.reply({"result": {"state": "ready", "state_message": "Printer is ready", "hostname": "filuna-moonraker-test", "software_version": "v0.9.0"}})
        elif path.path == "/server/info":
            self.reply({"result": {"klippy_connected": True, "klippy_state": "ready", "components": ["file_manager"]}})
        elif path.path == "/printer/objects/query":
            mr_state = "printing" if state["state"] == "printing" else "paused" if state["state"] == "paused" else "standby"
            self.reply({"result": {"eventtime": 42, "status": {
                "print_stats": {"state": mr_state, "filename": state["filename"], "print_duration": 22.4},
                "display_status": {"progress": state["progress"] / 100.0},
                "extruder": {"temperature": 215 if mr_state == "printing" else 24, "target": 215 if mr_state == "printing" else 0},
                "heater_bed": {"temperature": 60 if mr_state == "printing" else 25, "target": 60 if mr_state == "printing" else 0},
                "exclude_object": {"objects": [], "excluded_objects": []}}}})
        elif path.path == "/server/files/list":
            self.reply({"result": []})
        else:
            self.reply({"error": {"message": "Unknown Moonraker test endpoint"}}, 404)

    def do_PUT(self):
        path = urlparse(self.path).path
        length = int(self.headers.get("Content-Length", 0))
        if length > 32 * 1024 * 1024:
            self.reply({"error_code": 901}, 413)
            return
        self.rfile.read(length)
        if path == "/upload":
            with LOCK:
                STATE["uploaded"] += length
            self.reply({"error_code": 0})
        else:
            self.reply({"error": "not found"}, 404)

    def do_POST(self):
        path = urlparse(self.path)
        length = int(self.headers.get("Content-Length", 0))
        if length > 32 * 1024 * 1024:
            self.reply({"error": "file too large"}, 413)
            return
        self.rfile.read(length)
        if path.path == "/printer/print/start":
            with LOCK:
                STATE["state"] = "printing"
                STATE["filename"] = parse_qs(path.query).get("filename", ["test.gcode"])[0]
                STATE["progress"] = 2
            self.reply({"result": "ok"})
        elif path.path == "/printer/print/pause":
            with LOCK: STATE["state"] = "paused"
            self.reply({"result": "ok"})
        elif path.path == "/printer/print/resume":
            with LOCK: STATE["state"] = "printing"
            self.reply({"result": "ok"})
        elif path.path == "/printer/print/cancel":
            with LOCK: STATE["state"] = "ready"
            self.reply({"result": "ok"})
        elif path.path == "/server/files/upload":
            self.reply({"result": {"item": {"path": "test.gcode", "size": length}, "action": "create"}})
        else:
            self.reply({"error": "not found"}, 404)


def serve_http(bind, port):
    server = ThreadingHTTPServer((bind, port), PrinterHTTP)
    print(f"[HTTP] listening on {bind}:{port}", flush=True)
    server.serve_forever()


def serve_cc2_udp(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.bind(("0.0.0.0", port))
    print(f"[CC2] discovery listening UDP {port}", flush=True)
    while True:
        raw, remote = sock.recvfrom(4096)
        try:
            req = json.loads(raw)
        except (ValueError, UnicodeDecodeError):
            continue
        if req.get("method") != 7000:
            continue
        reply = {"id": req.get("id", 0), "result": {"host_name": "Filuna LAB Elegoo CC2", "machine_model": "Centauri Carbon 2", "sn": "CC2TEST00001", "ip_address": ip}}
        sock.sendto(json.dumps(reply).encode(), remote)
        print("[CC2] discovery replied to", remote, flush=True)


def serve_sdcp_udp(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.bind(("0.0.0.0", port))
    print(f"[SDCP] discovery listening UDP {port}", flush=True)
    while True:
        data, remote = sock.recvfrom(4096)
        if data != b"M99999":
            continue
        resp = {"Id": "aabbccddeeff", "Data": {"Name": "Centauri Carbon", "MachineName": "Centauri Carbon", "MainboardID": "aabbccdd00112233445566778899aabb", "FirmwareVersion": "LAB-TEST"}}
        sock.sendto(json.dumps(resp).encode(), remote)
        print("[SDCP] discovery replied to", remote, flush=True)


def serve_bambu_udp(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.bind(("0.0.0.0", port))
    payload = ("HTTP/1.1 200 OK\r\nST: urn:bambulab-com:device:3dprinter:1\r\n"
               "USN: 039LAB0000001\r\nLocation: " + ip + "\r\n"
               "DevModel.bambu.com: N2S\r\nDevName.bambu.com: Filuna LAB Bambu A1\r\n"
               "DevConnect.bambu.com: lan\r\nCache-Control: max-age=120\r\n\r\n").encode()
    print(f"[Bambu] discovery listening UDP {port}", flush=True)
    while True:
        raw, remote = sock.recvfrom(8192)
        if b"M-SEARCH" not in raw.upper():
            continue
        for destport in (1990, 2021):
            sock.sendto(payload, (remote[0], destport))
        print("[Bambu] SSDP response to", remote[0], flush=True)


def spawn(target, *args):
    thread = threading.Thread(target=target, args=args, daemon=True)
    thread.start()


def main():
    parser = argparse.ArgumentParser(description="Real-LAN protocol lab for Filuna Slicer. Not a hardware emulator.")
    parser.add_argument("protocol", choices=["moonraker", "cc2", "bambu", "sdcp"])
    parser.add_argument("--ip", default="127.0.0.1", help="IP inside dedicated namespace")
    parser.add_argument("--udp-port", type=int, default=None)
    parser.add_argument("--mqtt-port", type=int, default=1883)
    parser.add_argument("--http-port", type=int, default=None)
    parser.add_argument("--access-code", default="123456", help="CC2 lab-only test secret")
    args = parser.parse_args()
    if args.protocol == "moonraker":
        serve_http("0.0.0.0", args.http_port or 7125)
    if args.protocol == "cc2":
        server = ThreadedTCP(("0.0.0.0", args.mqtt_port), TinyCc2Mqtt)
        server.serial = "CC2TEST00001"
        server.access_code = args.access_code
        spawn(server.serve_forever)
        spawn(serve_http, "0.0.0.0", args.http_port or 80)
        serve_cc2_udp(args.ip, args.udp_port or 52700)
    if args.protocol == "bambu":
        print("[Bambu] SSDP discovery ONLY. MQTT/TLS 8883 and FTPS 990 not emulated.", flush=True)
        serve_bambu_udp(args.ip, args.udp_port or 1990)
    if args.protocol == "sdcp":
        print("[SDCP] UDP discovery ONLY. WebSocket 3030 not emulated.", flush=True)
        serve_sdcp_udp(args.udp_port or 3000)


if __name__ == "__main__":
    main()