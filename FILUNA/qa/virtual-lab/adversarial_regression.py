#!/usr/bin/env python3
"""Adversarial regression for the ISOLATED Filuna virtual-printer LAN.

Only targets 10.77.0.11/13 and local ephemeral sockets. Tests the mock,
NOT the Filuna DeviceBridge, physical printers, or a release gate.
"""
import json
import socket
import struct
import sys
import urllib.error
import urllib.request
from pathlib import Path

ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
import network_regression as base

ROWS=[]
def check(name, fn):
    try:
        good, note=fn()
    except Exception as exc:
        good=False
        note=f"{type(exc).__name__}: {exc}"
    ROWS.append({"case":name,"status":"PASS" if good else "FAIL","detail":note})
    print(f"{'PASS' if good else 'FAIL'} {name}: {note}")

def invalid_mqtt():
    sock, code=base.mqtt_connect("deliberately-wrong-lab-code")
    sock.settimeout(2)
    try:
        data=sock.recv(1)
        return code==4 and data==b"", "CONNACK=4 then connection closed by mock"
    finally:
        sock.close()

def mqtt_disconnect():
    s,code=base.mqtt_connect("123456")
    if code: return False, "unexpected authentication rejection"
    s.sendall(b"\xe0\x00")
    s.settimeout(2)
    try:
        reply=s.recv(1)
        return reply==b"", "MQTT DISCONNECT caused EOF"
    finally:
        s.close()

def malformed_mqtt():
    # Isolated emulator must drop malformed/oversized remaining-length framing.
    s=socket.create_connection(("10.77.0.11",1883),timeout=3)
    s.settimeout(3)
    try:
        s.sendall(b"\x10\xff\xff\xff\xff\x01")
        reply=s.recv(1)
        return reply==b"", "Malformed MQTT length rejected by emulator"
    finally:
        s.close()

def oversized_upload_cc2():
    # Content-Length is rejected before body is read. Never send a 32MB payload.
    req=urllib.request.Request("http://10.77.0.11/upload",data=b"x",
                               method="PUT",headers={"Content-Length":str(33*1024*1024)})
    try:
        urllib.request.urlopen(req,timeout=4)
        return False,"server accepted oversized upload"
    except urllib.error.HTTPError as exc:
        return exc.code==413, f"CC2 upload limit returned HTTP {exc.code}"

def oversized_upload_moonraker():
    req=urllib.request.Request("http://10.77.0.13:7125/server/files/upload",
                               data=b"x",method="POST",
                               headers={"Content-Length":str(33*1024*1024)})
    try:
        urllib.request.urlopen(req,timeout=4)
        return False,"server accepted oversized upload"
    except urllib.error.HTTPError as exc:
        return exc.code==413, f"Moonraker upload limit returned HTTP {exc.code}"

def unknown_method():
    s,code=base.mqtt_connect("123456")
    if code: return False,"authentication failed"
    try:
        result=base.mqtt_method(s,987654)
        # The supplied emulator currently responds success for unknown methods.
        # We record this as a *known failure*, never treat it as approved behavior.
        err=result.get("result",{}).get("error_code")
        return err not in (None,0),f"unknown MQTT method error_code={err!r}"
    finally:
        s.close()

def unreachable_host():
    # Local ephemeral port guaranteed to have no listener after closing bound socket.
    with socket.socket() as tmp:
        tmp.bind(("127.0.0.1",0))
        port=tmp.getsockname()[1]
    try:
        with socket.create_connection(("127.0.0.1",port),timeout=1):
            return False,"unexpected connection"
    except (ConnectionRefusedError,OSError) as exc:
        return True,f"disconnected local endpoint refused ({type(exc).__name__})"

def main():
    check("cc2-wrong-password-rejected",invalid_mqtt)
    check("cc2-clean-mqtt-disconnect",mqtt_disconnect)
    check("cc2-malformed-mqtt-framing-rejected",malformed_mqtt)
    check("cc2-upload-size-limit",oversized_upload_cc2)
    check("moonraker-upload-size-limit",oversized_upload_moonraker)
    check("disconnected-local-endpoint",unreachable_host)
    check("cc2-unknown-command-rejected",unknown_method)
    report={"scope":"virtual-lab-adversarial-not-filuna-not-hardware",
            "results":ROWS,"pass":sum(x["status"]=="PASS" for x in ROWS),
            "fail":sum(x["status"]=="FAIL" for x in ROWS),
            "is_release_evidence":False}
    out=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"adversarial-results.json"
    out.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print(f"ADVERSARIAL: {report['pass']} PASS, {report['fail']} FAIL")
    return int(report["fail"]!=0)
if __name__=="__main__":
    raise SystemExit(main())
