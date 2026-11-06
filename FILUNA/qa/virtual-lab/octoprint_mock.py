#!/usr/bin/env python3
"""Local-only OctoPrint API emulator for protocol contract testing.

No actual printer, no external listening, no real authentication or printing.
"""
import argparse
import json
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

class Handler(BaseHTTPRequestHandler):
    state={"state":"Operational","file":None}
    api_key="FILUNA_LAB_ONLY_KEY"
    def log_message(self,*args): pass
    def respond(self,code,payload):
        raw=json.dumps(payload).encode()
        self.send_response(code)
        self.send_header("Content-Type","application/json")
        self.send_header("Content-Length",str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)
    def check_auth(self):
        if self.headers.get("X-Api-Key")!=self.api_key:
            self.respond(401,{"error":"Unauthorized LAB API key"})
            return False
        return True
    def do_GET(self):
        if not self.check_auth():return
        if self.path=="/api/version":
            return self.respond(200,{"server":"1.11.0-lab","api":"0.1","text":"OctoPrint LAB"})
        if self.path=="/api/printer":
            return self.respond(200,{"state":{"text":self.state["state"],
                "flags":{"operational":True,"printing":self.state["state"]=="Printing"}},
                "temperature":{"tool0":{"actual":24.0,"target":0.0},"bed":{"actual":24.0,"target":0.0}}})
        if self.path=="/api/job":
            return self.respond(200,{"state":self.state["state"],
                "job":{"file":{"name":self.state["file"]}},"progress":{"completion":0.0}})
        self.respond(404,{"error":"Unknown endpoint"})
    def do_POST(self):
        if not self.check_auth():return
        size=int(self.headers.get("Content-Length","0"))
        if size>1024*1024:return self.respond(413,{"error":"LAB file too large"})
        try:payload=json.loads(self.rfile.read(size) or b"{}")
        except ValueError:return self.respond(400,{"error":"Malformed JSON"})
        if self.path=="/api/job":
            if payload.get("command") not in ("start","pause","cancel"):
                return self.respond(400,{"error":"Unsupported command"})
            command=payload["command"]
            self.state["state"]={"start":"Printing","pause":"Paused","cancel":"Operational"}[command]
            return self.respond(204,{})
        self.respond(404,{"error":"Unknown endpoint"})
def main():
    p=argparse.ArgumentParser()
    p.add_argument("--bind",default="127.0.0.1")
    p.add_argument("--port",type=int,default=15000)
    a=p.parse_args()
    if a.bind!="127.0.0.1":raise SystemExit("Lab only permits 127.0.0.1")
    ThreadingHTTPServer((a.bind,a.port),Handler).serve_forever()
if __name__=="__main__":main()
