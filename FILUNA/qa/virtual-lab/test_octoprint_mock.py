#!/usr/bin/env python3
"""OctoPrint mock contract tests, localhost only, no real hardware."""
import json
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

ROOT=Path(__file__).resolve().parent
HOST="http://127.0.0.1:15077"
KEY="FILUNA_LAB_ONLY_KEY"
def req(path,key=None,method="GET",body=None):
    headers={} if key is None else {"X-Api-Key":key}
    if body is not None:headers["Content-Type"]="application/json"
    data=None if body is None else json.dumps(body).encode()
    r=urllib.request.Request(HOST+path,method=method,headers=headers,data=data)
    try:
        with urllib.request.urlopen(r,timeout=2) as res:
            return res.status,json.loads(res.read() or b"{}")
    except urllib.error.HTTPError as exc:
        return exc.code,json.loads(exc.read() or b"{}")
def main():
    p=subprocess.Popen([sys.executable,str(ROOT/"octoprint_mock.py"),
                        "--port","15077"],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    results=[]
    def check(name,ok):
        results.append((name,bool(ok)))
        print(("PASS" if ok else "FAIL"),name)
    try:
        for _ in range(30):
            if p.poll() is not None:raise RuntimeError("mock failed to start")
            try:
                req("/api/version")
                break
            except OSError:time.sleep(.1)
        else:raise RuntimeError("mock not reachable")
        check("missing-auth-rejected",req("/api/version")[0]==401)
        check("wrong-auth-rejected",req("/api/version","wrong")[0]==401)
        check("identified",req("/api/version",KEY)[1].get("api")=="0.1")
        check("monitored",req("/api/printer",KEY)[1]["state"]["text"]=="Operational")
        check("job-status",req("/api/job",KEY)[0]==200)
        check("unsupported-command-rejected",req("/api/job",KEY,"POST",{"command":"unsafe"})[0]==400)
        check("start",req("/api/job",KEY,"POST",{"command":"start"})[0]==204)
        check("printing",req("/api/printer",KEY)[1]["state"]["flags"]["printing"])
        check("pause",req("/api/job",KEY,"POST",{"command":"pause"})[0]==204)
        check("cancel",req("/api/job",KEY,"POST",{"command":"cancel"})[0]==204)
        check("unknown-endpoint-rejected",req("/api/not-real",KEY)[0]==404)
    finally:
        p.terminate()
        try:p.wait(timeout=3)
        except subprocess.TimeoutExpired:p.kill()
    report={"scope":"OctoPrint local mock only, not Filuna, not physical hardware",
            "pass":sum(x[1] for x in results),"fail":sum(not x[1] for x in results),
            "results":[{"case":n,"status":"PASS" if ok else "FAIL"} for n,ok in results],
            "is_release_evidence":False}
    if len(sys.argv)>1:Path(sys.argv[1]).write_text(json.dumps(report,indent=2)+"\n")
    print("OCTOPRINT MOCK",report["pass"],"PASS",report["fail"],"FAIL")
    return 1 if report["fail"] else 0
if __name__=="__main__":raise SystemExit(main())
