#!/usr/bin/env python3
"""Flashforge modern LAN read-only protocol probe, restricted to QA loopback.

Reference from public independent firmware endpoint inventories:
https://github.com/Parallel-7/flashforge-api-docs
AD5X firmware 1.2.1 and Adventurer 5M firmware 3.2.7 endpoints:
POST /checkCode, /detail, /product with serialNumber+checkCode.
This is NOT the native C++ Filuna adapter, not an official manufacturer
certification, not evidence of a physical printer, and not a production
credential store. Only local 127.0.0.1 simulation may be probed here.
No /control, upload, print, camera or private customer network access.
"""
from __future__ import annotations

import hashlib
import ipaddress
import json
from urllib import request, error

READ_ONLY=("/checkCode","/detail","/product")
MAX_REPLY=256*1024

class ProtocolError(RuntimeError):
    pass


def _parse_target(host:str,port:int):
    ip=ipaddress.ip_address(host)
    if not ip.is_loopback or not 0<port<65536:
        raise ProtocolError("QA_REQUIRES_LOOPBACK_ONLY_NOT_CLIENT_PRINTER")
    return f"http://{ip.compressed}:{port}"


class FlashforgeLoopbackContractProbe:
    def __init__(self,host:str,port:int,serial_number:str,check_code:str):
        self.base=_parse_target(host,port)
        if not isinstance(serial_number,str) or not serial_number.strip() or len(serial_number)>100:
            raise ProtocolError("INVALID_SERIAL")
        if not isinstance(check_code,str) or not check_code.strip() or len(check_code)>100:
            raise ProtocolError("INVALID_CHECK_CODE")
        self._serial=serial_number
        self._code=check_code

    def _post(self,endpoint:str)->dict:
        if endpoint not in READ_ONLY:
            raise ProtocolError("ENDPOINT_NOT_READONLY_ALLOWED")
        body=json.dumps({"serialNumber":self._serial,
                         "checkCode":self._code}).encode("utf-8")
        req=request.Request(self.base+endpoint,data=body,
            headers={"Content-Type":"application/json",
                     "Accept":"application/json",
                     "User-Agent":"Filuna-OEM-LAN-Simulator/1"},
            method="POST")
        try:
            with request.urlopen(req,timeout=3) as res:
                if res.status!=200:
                    raise ProtocolError("HTTP_STATUS_NOT_OK")
                payload=res.read(MAX_REPLY+1)
                if len(payload)>MAX_REPLY:
                    raise ProtocolError("DEVICE_REPLY_TOO_LARGE")
        except (error.HTTPError,error.URLError,TimeoutError) as exc:
            raise ProtocolError("LAN_SIMULATOR_TRANSPORT_FAILED") from exc
        try:
            obj=json.loads(payload)
        except (UnicodeError,json.JSONDecodeError) as exc:
            raise ProtocolError("DEVICE_JSON_INVALID") from exc
        if not isinstance(obj,dict) or type(obj.get("code")) is not int:
            raise ProtocolError("DEVICE_RESPONSE_SCHEMA_INVALID")
        if obj["code"]!=0:
            raise ProtocolError("DEVICE_AUTH_OR_STATUS_DENIED")
        return obj

    def run(self):
        # Always prove pairing BEFORE status/capabilities, just as device
        # onboarding should. Neither credentials nor full payload are stored.
        check=self._post("/checkCode")
        detail=self._post("/detail")
        product=self._post("/product")
        if not isinstance(detail.get("detail"),dict):
            raise ProtocolError("DETAIL_OBJECT_REQUIRED")
        if not isinstance(product.get("product"),dict):
            raise ProtocolError("PRODUCT_CAPABILITIES_OBJECT_REQUIRED")
        return {
            "protocol":"FLASHFORGE_LAN_READONLY_HTTP_8898_SIMULATOR",
            "transport":"LOOPBACK_ONLY",
            "network_opened":"PASS_SIMULATED",
            "pairing":"PASS_SIMULATED",
            "status":"PASS_SIMULATED",
            "product":"PASS_SIMULATED",
            "printer_capabilities_verification":"NOT_REAL_DEVICE",
            "filament_station_confirmed":"NOT_RUN",
            "camera_preview_confirmed":"NOT_RUN",
            "gcode_send":"NOT_RUN",
            "hardware_print":"NOT_RUN",
            "native_filuna_cpp_adapter":"NOT_RUN",
            "response_proof_sha256":hashlib.sha256(
              json.dumps({"check":check,"detail":detail,"product":product},
                         sort_keys=True,separators=(",",":")).encode("utf-8")).hexdigest()
        }
