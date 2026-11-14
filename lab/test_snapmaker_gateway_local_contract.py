#!/usr/bin/env python3
"""Only mock official Snapmaker application gateway, not U1 printer LAN."""
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
import json
import sys
import threading
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parent))
from snapmaker_gateway_local_contract import (
  GatewayNotProven,probe_local_gateway,_get_local,parse_health,parse_device_cache
)

def health():
    return {"status":"ok","ok":True,
            "components":{"ipc_server":"ok","web_server":"ok"},
            "server_url":{"base_url":"http://127.0.0.1:9000"},
            "device_connected":True,"sn":"SANITIZED_TEST"}

def cache():
    return {"data":{"info":{"product":{"machine_type":"U1",
                                       "nozzle_diameter":[0.4,0.4,0.4,0.4]}},
                    "identity":{"productName":"Snapmaker U1","name":"TEST-ONLY"},
                    "objects":[]}}

class LocalGatewayFixture(BaseHTTPRequestHandler):
    def log_message(self,*args):return
    def do_GET(self):
        self.server.visited.append(self.path)
        if self.path not in ("/api/health","/api/cache/all"):
            self.send_response(404);self.end_headers();return
        record=(self.server.mock_health if self.path=="/api/health" else self.server.mock_cache)
        data=json.dumps(record).encode("utf-8") if record!="INVALID_JSON" else b"{UNPARSABLE"
        self.send_response(200);self.send_header("Content-Type","application/json")
        self.send_header("Content-Length",str(len(data)));self.end_headers()
        self.wfile.write(data)

class GatewayContractTests(unittest.TestCase):
    def setUp(self):
        self.s=ThreadingHTTPServer(("127.0.0.1",0),LocalGatewayFixture)
        self.s.mock_health=health();self.s.mock_cache=cache();self.s.visited=[]
        t=threading.Thread(target=self.s.serve_forever,daemon=True);t.start()
        self.addCleanup(self.s.server_close)
        self.addCleanup(self.s.shutdown)

    def runprobe(self):
        return probe_local_gateway("127.0.0.1",self.s.server_port)

    def test_valid_gateway_returns_simulator_not_hardware(self):
        result=self.runprobe()
        self.assertEqual(result["model"],"U1")
        self.assertEqual(result["nozzle_mm"],[0.4]*4)
        self.assertTrue(result["no_real_printer"])
        self.assertTrue(result["no_official_filuna_adapter"])
        self.assertEqual(self.s.visited,["/api/health","/api/cache/all"])

    def test_cache_fallback_product_name(self):
        del self.s.mock_cache["data"]["info"]["product"]["machine_type"]
        self.assertEqual(self.runprobe()["model"],"Snapmaker U1")

    def test_gateway_disconnected_rejected_before_cache(self):
        self.s.mock_health["device_connected"]=False
        with self.assertRaisesRegex(GatewayNotProven,"NO_DEVICE_CONNECTED"):
            self.runprobe()
        self.assertEqual(self.s.visited,["/api/health"])

    def test_invalid_health_rejected(self):
        self.s.mock_health["status"]="error";self.s.mock_health["ok"]=False
        with self.assertRaisesRegex(GatewayNotProven,"GATEWAY_HEALTH_FAILED"):
            self.runprobe()

    def test_unready_ipc_and_web_rejected(self):
        for name in ("ipc_server","web_server"):
            with self.subTest(name=name):
                h=health();h["components"][name]="offline"
                with self.assertRaisesRegex(GatewayNotProven,"GATEWAY_COMPONENT_NOT_READY"):
                    parse_health(h)

    def test_fake_external_gateway_base_url_rejected(self):
        self.s.mock_health["server_url"]["base_url"]="http://198.51.100.1:80"
        with self.assertRaisesRegex(GatewayNotProven,"GATEWAY_BASE_URL_NOT_LOCAL"):
            self.runprobe()

    def test_unidentified_model_cannot_pass(self):
        self.s.mock_cache["data"]["info"]["product"]["machine_type"]=""
        self.s.mock_cache["data"]["identity"]["productName"]=""
        with self.assertRaisesRegex(GatewayNotProven,"GATEWAY_MODEL_NOT_IDENTIFIED"):
            self.runprobe()

    def test_invalid_nozzle_cannot_pass(self):
        self.s.mock_cache["data"]["info"]["product"]["nozzle_diameter"]=["unknown"]
        with self.assertRaisesRegex(GatewayNotProven,"GATEWAY_NOZZLE_INVALID"):
            self.runprobe()

    def test_bogus_nozzle_size_rejected(self):
        self.s.mock_cache["data"]["info"]["product"]["nozzle_diameter"]=[35]
        with self.assertRaisesRegex(GatewayNotProven,"GATEWAY_NOZZLE_OUT_OF_VALID_RANGE"):
            self.runprobe()

    def test_gateway_service_does_not_equate_to_printer_lan(self):
        result=self.runprobe()
        self.assertIn("APP_LOCAL",result["mode"])
        self.assertTrue(result["no_hardware_print"])
        self.assertTrue(result["no_websocket_rpc_test"])

    def test_remote_host_not_contacted(self):
        with self.assertRaisesRegex(GatewayNotProven,"MUST_STAY_LOOPBACK"):
            _get_local(("192.168.0.32",9999),"/api/health")

    def test_command_endpoint_not_allowed(self):
        with self.assertRaisesRegex(GatewayNotProven,"PROHIBITED_NONREADONLY"):
            _get_local(("127.0.0.1",self.s.server_port),"/api/account")
        self.assertEqual(self.s.visited,[])

    def test_unparseable_response_rejected(self):
        self.s.mock_health="INVALID_JSON"
        with self.assertRaisesRegex(GatewayNotProven,"GATEWAY_JSON_INVALID"):
            self.runprobe()

    def test_serial_field_wrong_type_rejected(self):
        self.s.mock_health["sn"]={"raw_serial":"X"}
        with self.assertRaisesRegex(GatewayNotProven,"GATEWAY_SERIAL_TYPE_INVALID"):
            self.runprobe()

if __name__=="__main__":
    unittest.main()
