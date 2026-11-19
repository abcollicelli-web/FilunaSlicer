#!/usr/bin/env python3
"""Snapmaker official v2.4.1 application LOCAL gateway read-only contract QA.

Source mapping:
Snapmaker/OrcaSlicer@bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c
src/slic3r/Utils/GatewayService.{cpp,hpp}
src/slic3r/Utils/GatewayDevice.{cpp,hpp}
This is a loopback-only MOCK of an app-local gateway, NOT a connection to the
U1 printer's actual LAN, NO websocket command, no login and no printing.
"""
from __future__ import annotations

import hashlib
import ipaddress
import json
from urllib import request,error

class GatewayNotProven(RuntimeError):pass

READ_PATHS=("/api/health","/api/cache/all")


def _get_local(base,endpoint):
    if endpoint not in READ_PATHS:
        raise GatewayNotProven("PROHIBITED_NONREADONLY_GATEWAY_PATH")
    host,port=base
    if not ipaddress.ip_address(host).is_loopback or not 0<port<65536:
        raise GatewayNotProven("GATEWAY_SIMULATOR_MUST_STAY_LOOPBACK")
    url=f"http://{host}:{port}{endpoint}"
    try:
        with request.urlopen(request.Request(url,method="GET",
                headers={"Accept":"application/json"}),timeout=3) as res:
            if res.status!=200:
                raise GatewayNotProven("GATEWAY_HTTP_FAILED")
            data=res.read(256*1024+1)
            if len(data)>256*1024:
                raise GatewayNotProven("GATEWAY_RESPONSE_TOO_LARGE")
    except (error.HTTPError,error.URLError,TimeoutError) as exc:
        raise GatewayNotProven("GATEWAY_OFFLINE_OR_UNREACHABLE") from exc
    try:
        payload=json.loads(data)
    except (UnicodeError,json.JSONDecodeError) as exc:
        raise GatewayNotProven("GATEWAY_JSON_INVALID") from exc
    if not isinstance(payload,dict):
        raise GatewayNotProven("GATEWAY_RESPONSE_NOT_OBJECT")
    return payload


def parse_health(payload):
    if (payload.get("status")!="ok" and payload.get("ok") is not True):
        raise GatewayNotProven("GATEWAY_HEALTH_FAILED")
    components=payload.get("components")
    if not isinstance(components,dict):
        raise GatewayNotProven("GATEWAY_COMPONENTS_MISSING")
    for name in ("ipc_server","web_server"):
        if components.get(name)!="ok":
            raise GatewayNotProven("GATEWAY_COMPONENT_NOT_READY_"+name)
    url=payload.get("server_url",{})
    if not isinstance(url,dict) or not isinstance(url.get("base_url"),str):
        raise GatewayNotProven("GATEWAY_SERVER_URL_UNAVAILABLE")
    if not url["base_url"].startswith("http://127.0.0.1"):
        raise GatewayNotProven("GATEWAY_BASE_URL_NOT_LOCAL")
    if "device_connected" in payload and not isinstance(payload["device_connected"],bool):
        raise GatewayNotProven("GATEWAY_DEVICE_CONNECTED_NOT_BOOL")
    if "sn" in payload and not isinstance(payload["sn"],str):
        raise GatewayNotProven("GATEWAY_SERIAL_TYPE_INVALID")
    return {"gateway_healthy":True,
            "device_connected":payload.get("device_connected") is True}


def parse_device_cache(payload):
    data=payload.get("data")
    if not isinstance(data,dict):
        raise GatewayNotProven("GATEWAY_CACHE_MISSING_DATA")
    info=data.get("info",{})
    product=info.get("product",{}) if isinstance(info,dict) else {}
    identity=data.get("identity",{})
    if not isinstance(identity,dict) or not isinstance(product,dict):
        raise GatewayNotProven("GATEWAY_CACHE_IDENTITY_INVALID")
    model=product.get("machine_type") or identity.get("productName")
    if not isinstance(model,str) or not model.strip() or len(model)>120:
        raise GatewayNotProven("GATEWAY_MODEL_NOT_IDENTIFIED")
    nozzle=product.get("nozzle_diameter")
    if isinstance(nozzle,(int,float,str)) and not isinstance(nozzle,bool):
        nozzle=[nozzle]
    if not isinstance(nozzle,list) or not nozzle:
        raise GatewayNotProven("GATEWAY_NOZZLE_NOT_IDENTIFIED")
    try:
        values=[float(x) for x in nozzle]
    except (TypeError,ValueError) as exc:
        raise GatewayNotProven("GATEWAY_NOZZLE_INVALID") from exc
    if any(not 0.1<=v<=2.4 for v in values):
        raise GatewayNotProven("GATEWAY_NOZZLE_OUT_OF_VALID_RANGE")
    return {"identified_model":model.strip(),"nozzle_diameters_mm":values,
            "firmware_verified":"NOT_IN_GATEWAY_CACHE_CONTRACT",
            "real_device_verified":False}


def probe_local_gateway(host,port):
    health=parse_health(_get_local((host,port),"/api/health"))
    if not health["device_connected"]:
        raise GatewayNotProven("NO_DEVICE_CONNECTED_TO_LOCAL_GATEWAY")
    device=parse_device_cache(_get_local((host,port),"/api/cache/all"))
    return {"mode":"APP_LOCAL_SNAPMAKER_GATEWAY_SIMULATOR_ONLY",
            "transport":"HTTP_LOCALHOST_READONLY",
            "health":"PASS_SIMULATED",
            "identity":"PASS_SIMULATED",
            "model":device["identified_model"],
            "nozzle_mm":device["nozzle_diameters_mm"],
            "no_real_printer":True,
            "no_official_filuna_adapter":True,
            "no_websocket_rpc_test":True,
            "no_hardware_print":True,
            "further_firmware_and_oem_auth_required":True}
