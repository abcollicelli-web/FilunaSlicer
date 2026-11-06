#!/usr/bin/env python3
"""Evidence gate: simulated results may be PARTIAL, never physical PASS.

This script consumes emulator reports and emits a final-hardware checklist.
It never sends commands to real devices. Safe to run on the QA VPS or CI.
"""
from __future__ import annotations
import argparse
import datetime as dt
import json
from pathlib import Path

DEVICES = {
    "bambu-a1": {
        "name":"Bambu Lab A1 / A1 Mini",
        "protocols":["SSDP UDP 1990/2021","MQTT/TLS 8883","FTPS 990"],
        "physical":["Descoberta em rede LAN real", "Identificação de modelo e serial reais",
                    "Autenticação TLS com certificado e chave reais",
                    "Monitoramento de temperatura, progresso e erros",
                    "Comandos permitidos com confirmação do operador",
                    "Transferência FTPS e integridade do arquivo"],
    },
    "elegoo-carbon-original": {
        "name":"Elegoo Centauri Carbon original",
        "protocols":["SDCP UDP 3000","SDCP WebSocket 3030"],
        "physical":["Descoberta SDCP na LAN real","Identificação e firmware real",
                    "Handshake e autenticação WebSocket SDCP",
                    "Estado/temperaturas/progresso em tempo real",
                    "Comandos permitidos com confirmação do operador",
                    "Transferência e integridade do arquivo"],
    },
    "elegoo-cc2": {
        "name":"Elegoo Centauri Carbon 2",
        "protocols":["UDP 52700","MQTT 1883 / WS 9001"],
        "physical":["Descoberta UDP em rede real","Identificação de modelo e serial reais",
                    "Pareamento/autenticação e rejeição de chave incorreta",
                    "Telemetria MQTT real","Comandos reais com confirmação do operador",
                    "Upload e conferência de integridade"],
    },
    "moonraker": {
        "name":"Klipper / Moonraker",
        "protocols":["HTTP/WS 7125"],
        "physical":["Conexão ao IP real autorizado","Identificação do Klipper/Moonraker",
                    "Autenticação se configurada","Monitoramento WebSocket real",
                    "Comandos permitidos com confirmação do operador",
                    "Upload e verificação de arquivo"],
    },
    "octoprint": {
        "name":"OctoPrint",
        "protocols":["HTTP API 80/5000"],
        "physical":["Conexão ao IP real autorizado","Identificação e versão reais",
                    "Autenticação com chave API","Monitoramento real",
                    "Comandos permitidos com confirmação do operador",
                    "Upload e verificação de arquivo"],
    },
}
STAGES = ("discovered","identified","connected","monitored","commands","transfer")
def build(protocol:dict, adversarial:dict)->dict:
    assert protocol["is_release_evidence"] is False
    assert adversarial["is_release_evidence"] is False
    rows = protocol["results"]
    assert all(x["status"] in ("PASS","FAIL","NOT_RUN") for x in rows)
    assert protocol["pass"]==sum(x["status"]=="PASS" for x in rows)
    assert protocol["fail"]==sum(x["status"]=="FAIL" for x in rows)
    assert adversarial["pass"]==sum(x["status"]=="PASS" for x in adversarial["results"])
    assert adversarial["fail"]==sum(x["status"]=="FAIL" for x in adversarial["results"])
    results={}
    for slug,dev in DEVICES.items():
        applicable=[r for r in rows if r["device"]==slug]
        mock_pass=[r["stage"] for r in applicable if r["status"]=="PASS"]
        mock_fail=[r["stage"] for r in applicable if r["status"]=="FAIL"]
        results[slug]={
            "name":dev["name"],"protocols":dev["protocols"],
            "emulator_status":"PARTIAL_SIMULATED" if mock_pass and not mock_fail else
                              ("FAIL_SIMULATED" if mock_fail else "NOT_RUN"),
            "mock_pass_cases":mock_pass,"mock_fail_cases":mock_fail,
            "filuna_client_status":"NOT_RUN",
            "real_hardware_status":"PENDING_FINAL_HOMOLOGATION",
            "physical_checklist":[{"id":f"{slug.upper()}-{i+1:02d}",
                "stage":STAGES[i],"procedure":task,
                "status":"NOT_RUN","evidence":None,"operator":None}
                for i,task in enumerate(dev["physical"])],
        }
    return {
        "schema":"filuna-hardware-final-checklist-v1",
        "generated_at_utc":dt.datetime.now(dt.timezone.utc).isoformat(),
        "policy":"PARTIAL_SIMULATED does not count as hardware PASS or release approval",
        "source":{"protocol_pass":protocol["pass"],"protocol_fail":protocol["fail"],
                  "protocol_not_run":protocol["not_run"],
                  "adversarial_pass":adversarial["pass"],
                  "adversarial_fail":adversarial["fail"]},
        "devices":results,"physical_checks_total":sum(len(x["physical_checklist"]) for x in results.values()),
        "physical_checks_pass":0,"filuna_devicebridge_tested":False,
        "release_g3":"BLOCKED","release_g4":"BLOCKED",
        "hardware_homologation":"PENDING_FINAL_HOMOLOGATION",
    }
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--protocol",type=Path,required=True)
    ap.add_argument("--adversarial",type=Path,required=True)
    ap.add_argument("--output",type=Path,required=True)
    a=ap.parse_args()
    report=build(json.loads(a.protocol.read_text()),json.loads(a.adversarial.read_text()))
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n")
    print("PARTIAL_SIMULATED",sum(x["emulator_status"]=="PARTIAL_SIMULATED" for x in report["devices"].values()))
    print("HARDWARE CHECKS",report["physical_checks_total"],"PENDING",report["physical_checks_total"])
    print("RELEASE",report["release_g3"],report["release_g4"])
if __name__=="__main__": main()
