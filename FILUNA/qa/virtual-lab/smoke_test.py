#!/usr/bin/env python3
"""Simple localhost protocol smoke test; no namespace privileges required."""
import json
import socket
import struct
import threading
import time
import urllib.request
import lab


def start(target, *args):
    t = threading.Thread(target=target, args=args, daemon=True)
    t.start()
    return t


def udp_probe(port, payload):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(3)
    s.sendto(payload, ("127.0.0.1", port))
    data = s.recvfrom(4096)[0]
    s.close()
    return data


def mqtt_field(s):
    v = s.encode()
    return len(v).to_bytes(2, "big") + v


def test():
    server = lab.ThreadedTCP(("127.0.0.1", 11883), lab.TinyCc2Mqtt)
    server.serial = "CC2TEST00001"
    server.access_code = "123456"
    start(server.serve_forever)
    start(lab.serve_http, "127.0.0.1", 17125)
    start(lab.serve_cc2_udp, "127.0.0.1", 15270)
    start(lab.serve_sdcp_udp, 13000)
    start(lab.serve_bambu_udp, "127.0.0.1", 11990)
    time.sleep(0.35)

    response = json.loads(udp_probe(15270, b'{"id":0,"method":7000}'))
    assert response["result"]["sn"] == "CC2TEST00001"
    print("OK: Elegoo CC2 UDP 52700-equivalent")

    c = socket.create_connection(("127.0.0.1", 11883), timeout=3)
    proto = mqtt_field("MQTT") + bytes([4, 0xC2, 0, 60])
    body = proto + mqtt_field("filuna_lab") + mqtt_field("elegoo") + mqtt_field("123456")
    c.sendall(bytes([0x10, len(body)]) + body)
    assert c.recv(4) == bytes([0x20, 0x02, 0x00, 0x00])
    c.close()
    print("OK: Elegoo CC2 MQTT 3.1.1 authentication")

    info = json.load(urllib.request.urlopen("http://127.0.0.1:17125/printer/info", timeout=3))
    assert "hostname" in info["result"]
    print("OK: Moonraker HTTP /printer/info")

    info = json.loads(udp_probe(13000, b"M99999"))
    assert info["Data"]["MachineName"] == "Centauri Carbon"
    print("OK: Elegoo CC1 SDCP discovery")

    listen = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listen.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listen.bind(("127.0.0.1", 2021))
    listen.settimeout(3)
    req = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    req.sendto(b"M-SEARCH * HTTP/1.1\r\nST: urn:bambulab-com:device:3dprinter:1\r\n\r\n", ("127.0.0.1", 11990))
    resp = listen.recvfrom(4096)[0]
    assert b"DevModel.bambu.com: N2S" in resp
    print("OK: Bambu A1 SSDP discovery")
    req.close()
    listen.close()
    server.shutdown()
    print("All 5 local protocol smoke checks passed.")


if __name__ == "__main__":
    test()