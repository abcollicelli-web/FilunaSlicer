#!/usr/bin/env python3
"""Filuna Virtual LAN lab regression against OWNER-PROVIDED emulators.

Important: these tests exercise SIMULATED printers, not the Filuna application
or any physical firmware. Only the fixed 10.77.0.0/24 isolated lab is touched.
Never change the lab IPs to physical or public addresses.
"""
from __future__ import annotations

import datetime as dt
import json
import socket
import struct
import sys
import urllib.error
import urllib.request
from pathlib import Path

LAB = Path(__file__).resolve().parent
IPS = {"cc2":"10.77.0.11", "bambu":"10.77.0.12",
       "moonraker":"10.77.0.13", "sdcp":"10.77.0.14"}
OUT = Path(sys.argv[1]) if len(sys.argv)>1 else LAB / "results-lan.json"
ROWS: list[dict] = []

def record(device: str, stage: str, condition: bool, detail: str) -> None:
    state = "PASS" if condition else "FAIL"
    ROWS.append({"device":device,"stage":stage,"status":state,"detail":detail})
    print(f"{state} {device} {stage}: {detail}")

def not_run(device: str, stage: str, reason: str) -> None:
    ROWS.append({"device":device,"stage":stage,"status":"NOT_RUN","detail":reason})
    print(f"NOT_RUN {device} {stage}: {reason}")

def udp(host: str, port: int, payload: bytes) -> bytes:
    s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    try:
        s.settimeout(4)
        s.sendto(payload,(host,port))
        return s.recvfrom(8192)[0]
    finally:
        s.close()

def http(host: str, port: int, path: str, method: str="GET",
         data: bytes | None=None) -> tuple[int, dict]:
    req=urllib.request.Request(
        f"http://{host}:{port}{path}",data=data,method=method)
    try:
        with urllib.request.urlopen(req,timeout=5) as resp:
            return resp.status, json.load(resp)
    except urllib.error.HTTPError as exc:
        return exc.code, json.load(exc)

def mqtt_string(s: str) -> bytes:
    v=s.encode()
    return struct.pack("!H",len(v))+v

def mqtt_packet(tag: int, payload: bytes) -> bytes:
    n=len(payload);length=bytearray()
    while True:
        x=n%128;n//=128;length.append(x+(128 if n else 0))
        if not n: break
    return bytes([tag])+bytes(length)+payload

def mqtt_read(s: socket.socket) -> tuple[int,bytes]:
    def get(n:int)->bytes:
        b=bytearray()
        while len(b)<n:
            q=s.recv(n-len(b))
            if not q: raise EOFError("MQTT closed before reply")
            b.extend(q)
        return bytes(b)
    tag=get(1)[0];n=0;factor=1
    for i in range(4):
        byte=get(1)[0]
        n+=(byte&127)*factor
        if byte<128:break
        factor*=128
    if n>65536: raise ValueError("oversized reply")
    return tag,get(n)

def mqtt_connect(password: str)->tuple[socket.socket,int]:
    s=socket.create_connection((IPS["cc2"],1883),timeout=5)
    s.settimeout(5)
    body=(mqtt_string("MQTT")+bytes([4,0xC2,0,60])
          +mqtt_string("filuna_qa")+mqtt_string("elegoo")+mqtt_string(password))
    s.sendall(mqtt_packet(0x10,body))
    tag,msg=mqtt_read(s)
    if tag!=0x20 or len(msg)!=2:raise ValueError("missing CONNACK")
    return s,msg[1]

def mqtt_method(s:socket.socket,method:int)->dict:
    topic="elegoo/CC2TEST00001/filuna_qa/api_request"
    payload=json.dumps({"id":"lab-regression","method":method,"params":{"filename":"lab-test-only.gcode"}}).encode()
    s.sendall(mqtt_packet(0x30,mqtt_string(topic)+payload))
    tag,msg=mqtt_read(s)
    if tag>>4!=3:raise ValueError(f"expected MQTT publish, got {tag}")
    length=int.from_bytes(msg[:2],"big");body=json.loads(msg[2+length:])
    return body

def expect_case(label:str,device:str,stage:str,fn):
    try:
        ok,detail=fn()
        record(device,stage,bool(ok),detail)
    except Exception as exc:
        record(device,stage,False,f"{label}: {type(exc).__name__}: {exc}")

def test_cc2_discovery():
    b=udp(IPS["cc2"],52700,json.dumps({"id":51,"method":7000}).encode())
    j=json.loads(b)
    result=j.get("result",{})
    return (result.get("machine_model")=="Centauri Carbon 2"
            and result.get("sn")=="CC2TEST00001",
            "UDP 52700 returned CC2TEST00001; identity from emulator")

def test_cc2_auth():
    s,code=mqtt_connect("123456")
    s.close()
    return code==0,"MQTT 1883 CONNECT accepted lab key only"

def test_cc2_bad_auth():
    s,code=mqtt_connect("incorrect")
    s.close()
    return code==4,"MQTT 1883 rejected incorrect key (CONNACK=4)"

def test_cc2_status():
    s,code=mqtt_connect("123456")
    if code: raise ValueError("lab auth failed")
    res=mqtt_method(s,1002)
    s.close()
    d=res.get("result",{})
    return ("machine_status" in d and "extruder" in d
            and "print_status" in d,
            "MQTT status response includes machine/extruder/progress")

def test_cc2_commands():
    s,code=mqtt_connect("123456")
    if code:raise ValueError("lab auth failed")
    codes=[1020,1021,1023,1022]
    outs=[mqtt_method(s,i) for i in codes]
    s.close()
    return all(v.get("result",{}).get("error_code")==0 for v in outs), \
           "Lab MQTT start/pause/resume/cancel methods acknowledged (NOT Filuna)"

def test_cc2_upload():
    code,res=http(IPS["cc2"],80,"/upload","PUT",b"FILUNA_TEST_ONLY")
    return code==200 and res.get("error_code")==0,"Lab PUT /upload accepted 16 test bytes"

def test_cc2_invalid_endpoint():
    code,res=http(IPS["cc2"],80,"/not-supported","GET")
    return code==404,"Unknown endpoint correctly rejected HTTP 404"

def test_moon_info():
    code,r=http(IPS["moonraker"],7125,"/printer/info")
    return code==200 and r.get("result",{}).get("hostname")=="filuna-moonraker-test", \
           "Moonraker mocked API identified"

def test_moon_status():
    code,r=http(IPS["moonraker"],7125,"/printer/objects/query?print_stats&extruder")
    st=r.get("result",{}).get("status",{})
    return code==200 and "print_stats" in st and "extruder" in st, \
           "Moonraker telemetry simulated"

def test_moon_actions():
    assert http(IPS["moonraker"],7125,"/__lab/state")[0]==200, "not a lab target"
    # No physical device: exact virtual namespace and emulated endpoint only.
    for action in ("start?filename=filuna_lab.gcode","pause","resume","cancel"):
        code,result=http(IPS["moonraker"],7125,"/printer/print/"+action,"POST",b"")
        if code!=200 or result.get("result")!="ok":
            return False, "Moonraker emulator action failed: "+action
    _,st=http(IPS["moonraker"],7125,"/__lab/state")
    return st.get("state")=="ready","Moonraker start/pause/resume/cancel mocked"

def test_moon_upload():
    code,r=http(IPS["moonraker"],7125,"/server/files/upload","POST",b"TEST_GCODE")
    return code==200 and r.get("result",{}).get("item",{}).get("size")==10,\
           "Moonraker mocked upload accepted test payload"

def test_moon_bad_endpoint():
    code,r=http(IPS["moonraker"],7125,"/bad_endpoint")
    return code==404,"Unknown Moonraker endpoint rejected"

def test_sdcp_discovery():
    j=json.loads(udp(IPS["sdcp"],3000,b"M99999"))
    return j.get("Data",{}).get("MachineName")=="Centauri Carbon",\
           "Original Carbon answered SDCP M99999 UDP 3000"

def test_bambu_discovery():
    # The supplied Bambu mock replies to UDP 2021, not the client's ephemeral port.
    listen=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    req=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    try:
        listen.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        listen.bind(("10.77.0.1",2021));listen.settimeout(4)
        req.sendto(b"M-SEARCH * HTTP/1.1\r\nST: urn:bambulab-com:device:3dprinter:1\r\n\r\n",
                   (IPS["bambu"],1990))
        resp=listen.recvfrom(8192)[0]
        return b"DevName.bambu.com: Filuna LAB Bambu A1" in resp and \
               b"DevModel.bambu.com:" in resp,\
               "Bambu A1 mock replied to SSDP 2021"
    finally:
        req.close();listen.close()

def main()->int:
    checks=[
        ("UDP","elegoo-cc2","discovered+identified",test_cc2_discovery),
        ("MQTT","elegoo-cc2","connected-authorized",test_cc2_auth),
        ("MQTT","elegoo-cc2","auth-invalid-rejected",test_cc2_bad_auth),
        ("MQTT","elegoo-cc2","monitored",test_cc2_status),
        ("MQTT","elegoo-cc2","commands-simulated",test_cc2_commands),
        ("HTTP","elegoo-cc2","file-transfer-simulated",test_cc2_upload),
        ("HTTP","elegoo-cc2","unknown-endpoint-rejected",test_cc2_invalid_endpoint),
        ("HTTP","moonraker","identified",test_moon_info),
        ("HTTP","moonraker","monitored",test_moon_status),
        ("HTTP","moonraker","commands-simulated",test_moon_actions),
        ("HTTP","moonraker","file-transfer-simulated",test_moon_upload),
        ("HTTP","moonraker","unknown-endpoint-rejected",test_moon_bad_endpoint),
        ("UDP","elegoo-carbon-original","discovered+identified",test_sdcp_discovery),
        ("SSDP","bambu-a1","discovered+identified",test_bambu_discovery),
    ]
    for label,device,stage,fn in checks:
        expect_case(label,device,stage,fn)
    not_run("bambu-a1","mqtt-tls-8883","not implemented in supplied lab")
    not_run("bambu-a1","ftps-990","not implemented in supplied lab")
    not_run("elegoo-carbon-original","sdcp-ws-3030","not implemented in supplied lab")
    not_run("moonraker","websocket-7125","not implemented in supplied lab")
    not_run("all","filuna-wine-lan-adapter","no UI/DeviceBridge integration tested yet")
    not_run("all","real-printer-firmware","no physical printer connected")
    now=dt.datetime.now(dt.timezone.utc).isoformat()
    report={
      "kind":"emulator-protocol-wire-regression-not-hardware",
      "timestamp_utc":now,
      "network":"10.77.0.0/24 isolated bridge, no NAT",
      "limitations":["no Filuna adapter operated","no real printer or firmware",
                     "no Bambu TLS/FTPS; no SDCP/Moonraker WebSocket"],
      "results":ROWS,"pass":sum(x["status"]=="PASS" for x in ROWS),
      "fail":sum(x["status"]=="FAIL" for x in ROWS),
      "not_run":sum(x["status"]=="NOT_RUN" for x in ROWS),
      "is_release_evidence":False
    }
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(report,indent=2,ensure_ascii=False)+"\n")
    print("RESULT",report["pass"],"PASS",report["fail"],"FAIL",
          report["not_run"],"NOT_RUN; result at",OUT)
    return 1 if report["fail"] else 0

if __name__=="__main__":
    raise SystemExit(main())
