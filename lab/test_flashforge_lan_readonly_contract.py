#!/usr/bin/env python3
"""Loopback Flashforge AD5X/5M firmware endpoint contract tests; no hardware."""
import json
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
import sys
import threading
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parent))
from flashforge_lan_readonly_contract import (
    FlashforgeLoopbackContractProbe,ProtocolError,MAX_REPLY
)

class LoopbackPrinter(BaseHTTPRequestHandler):
    def log_message(self,fmt,*args):
        return  # Never print credential/body data.
    def do_POST(self):
        body=self.rfile.read(min(4096,int(self.headers.get("Content-Length",0))))
        try:cred=json.loads(body)
        except json.JSONDecodeError:cred={}
        self.server.paths.append(self.path)
        if self.path not in ("/checkCode","/detail","/product"):
            self.send_response(405);self.end_headers();return
        if self.server.mode=="invalid_json":
            payload=b"INVALID{"
        elif self.server.mode=="oversized":
            payload=b"{" + b"x"*MAX_REPLY
        elif self.server.mode=="schema":
            payload=b'{"code":"0","message":"wrong type"}'
        elif cred.get("serialNumber")!="TEST-VENDOR-UNIT" or cred.get("checkCode")!="TEST-CODE-ONLY":
            payload=b'{"code":401,"message":"Denied"}'
        elif self.server.mode=="status_missing" and self.path=="/detail":
            payload=b'{"code":0,"message":"ok"}'
        elif self.server.mode=="product_missing" and self.path=="/product":
            payload=b'{"code":0,"message":"ok"}'
        elif self.path=="/checkCode":
            payload=b'{"code":0,"message":"ok"}'
        elif self.path=="/detail":
            payload=b'{"code":0,"detail":{"printStatus":"idle","firmware":"test-fixture"}}'
        else:
            payload=b'{"code":0,"product":{"nozzleTempCtrlState":1}}'
        self.send_response(200)
        self.send_header("Content-Type","application/json")
        self.send_header("Content-Length",str(len(payload)))
        self.end_headers()
        try:self.wfile.write(payload)
        except (BrokenPipeError,ConnectionResetError):pass


class ReadOnlyFlashforgeNetworkTests(unittest.TestCase):
    def setUp(self):
        self.srv=ThreadingHTTPServer(("127.0.0.1",0),LoopbackPrinter)
        self.srv.paths=[]
        self.srv.mode="pass"
        self.runner=threading.Thread(target=self.srv.serve_forever,daemon=True)
        self.runner.start()
        self.addCleanup(self.srv.server_close)
        self.addCleanup(self.srv.shutdown)
        self.client=FlashforgeLoopbackContractProbe(
            "127.0.0.1",self.srv.server_port,"TEST-VENDOR-UNIT","TEST-CODE-ONLY")

    def test_readonly_pairing_status_product(self):
        result=self.client.run()
        self.assertEqual(self.srv.paths,["/checkCode","/detail","/product"])
        self.assertEqual(result["network_opened"],"PASS_SIMULATED")
        self.assertEqual(result["hardware_print"],"NOT_RUN")
        self.assertEqual(result["native_filuna_cpp_adapter"],"NOT_RUN")
        self.assertNotIn("TEST-CODE-ONLY",json.dumps(result))
        self.assertNotIn("TEST-VENDOR-UNIT",json.dumps(result))

    def test_control_and_start_endpoint_rejected(self):
        for name in ("/control","/printGcode","/uploadGcode","/gcodeList"):
            with self.subTest(endpoint=name):
                with self.assertRaisesRegex(ProtocolError,"ENDPOINT_NOT_READONLY_ALLOWED"):
                    self.client._post(name)
        self.assertEqual(self.srv.paths,[])

    def test_foreign_host_prohibited_even_if_private(self):
        for host in ("192.168.1.50","8.8.8.8"):
            with self.assertRaisesRegex(ProtocolError,"QA_REQUIRES_LOOPBACK_ONLY"):
                FlashforgeLoopbackContractProbe(host,8898,"TEST","ABC")

    def test_wrong_credentials_do_not_unlock_status(self):
        bad=FlashforgeLoopbackContractProbe(
            "127.0.0.1",self.srv.server_port,"TEST-VENDOR-UNIT","WRONG")
        with self.assertRaisesRegex(ProtocolError,"DEVICE_AUTH_OR_STATUS_DENIED"):
            bad.run()
        self.assertEqual(self.srv.paths,["/checkCode"])

    def test_invalid_device_json_is_hard_failure(self):
        self.srv.mode="invalid_json"
        with self.assertRaisesRegex(ProtocolError,"DEVICE_JSON_INVALID"):
            self.client.run()

    def test_too_large_response_is_hard_failure(self):
        self.srv.mode="oversized"
        with self.assertRaisesRegex(ProtocolError,"DEVICE_REPLY_TOO_LARGE"):
            self.client.run()

    def test_schema_must_contain_integer_status(self):
        self.srv.mode="schema"
        with self.assertRaisesRegex(ProtocolError,"DEVICE_RESPONSE_SCHEMA_INVALID"):
            self.client.run()

    def test_missing_status_object_must_not_be_passed(self):
        self.srv.mode="status_missing"
        with self.assertRaisesRegex(ProtocolError,"DETAIL_OBJECT_REQUIRED"):
            self.client.run()

    def test_missing_product_capabilities_must_not_be_passed(self):
        self.srv.mode="product_missing"
        with self.assertRaisesRegex(ProtocolError,"PRODUCT_CAPABILITIES_OBJECT_REQUIRED"):
            self.client.run()

    def test_no_credentials_not_accepted(self):
        with self.assertRaisesRegex(ProtocolError,"INVALID_CHECK_CODE"):
            FlashforgeLoopbackContractProbe(
                "127.0.0.1",self.srv.server_port,"TEST-VENDOR-UNIT","")

if __name__=="__main__":
    unittest.main()
