#!/usr/bin/env python3
"""U1 four-extruder OEM 2.4.1 fixture and verified portable QA.

Uses original pinned OEM slicer + actual 4 distinct extruder object assignments.
No vendor binaries/profiles are changed. CLI runtime guard modifies *only* a
temporary process copy's nozzle_diameter, sourced from the same OEM machine.
A/B non-repeatability is reported, never concealed. No physical prints.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import zipfile
from xml.etree import ElementTree as ET

from portable_first_qa import OFFICIAL, obtain, extract_executable, digest
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_runtime_nozzle_guard_qa import guarded_runtime_process
from snapmaker_u1_guard_probe import run_cli
from snapmaker_spatial_gcode_validator import policy_from_yaml, validate_gcode
from snapmaker_u1_temporal_ab_qa import compare_temporally_separated

ROOT=Path(__file__).resolve().parents[1]
CORE="http://schemas.microsoft.com/3dmanufacturing/core/2015/02"
PROD="http://schemas.microsoft.com/3dmanufacturing/production/2015/06"
REL="http://schemas.openxmlformats.org/package/2006/relationships"
TYPES="http://schemas.openxmlformats.org/package/2006/content-types"
COLOR=("#E60012","#1565C0","#FFD600","#15B857")


def xml(node)->bytes:
    return b'<?xml version="1.0" encoding="UTF-8"?>\n'+ET.tostring(
        node, encoding="UTF-8", xml_declaration=False)


def build_four_colour_3mf(out:Path)->dict:
    """Standard 3MF with 4 disjoint cubes; Orca object metadata extruder 1..4."""
    ET.register_namespace("",CORE)
    ET.register_namespace("p",PROD)
    objroot=ET.Element(f"{{{CORE}}}model",{"unit":"millimeter"})
    objects=ET.SubElement(objroot,f"{{{CORE}}}resources")
    meshobj=ET.SubElement(objects,f"{{{CORE}}}object",{"id":"1","type":"model"})
    mesh=ET.SubElement(meshobj,f"{{{CORE}}}mesh")
    verts=ET.SubElement(mesh,f"{{{CORE}}}vertices")
    # Closed manifold cube centered at origin, Z -10..10.
    coords=[(-10,-10,-10),(10,10,-10),(10,-10,-10),(-10,10,-10),
            (-10,-10,10),(10,-10,10),(10,10,10),(-10,10,10)]
    for x,y,z in coords:
        ET.SubElement(verts,f"{{{CORE}}}vertex",{"x":str(x),"y":str(y),"z":str(z)})
    faces=[(0,1,2),(0,3,1),(4,5,6),(4,6,7),(0,2,5),(0,5,4),
           (2,1,6),(2,6,5),(1,3,7),(1,7,6),(3,0,4),(3,4,7)]
    tr=ET.SubElement(mesh,f"{{{CORE}}}triangles")
    for a,b,c in faces:
        ET.SubElement(tr,f"{{{CORE}}}triangle",{"v1":str(a),"v2":str(b),"v3":str(c)})
    # Bambu/Orca extension: wrapper object (id >=2) references a model mesh
    # in the same OPC 3MF archive; each has an independent extruder assignment.
    main=ET.Element(f"{{{CORE}}}model",{"unit":"millimeter","requiredextensions":"p"})
    ET.SubElement(main,f"{{{CORE}}}metadata",{"name":"Application"}).text="Filuna QA 4-extruder fixture"
    rs=ET.SubElement(main,f"{{{CORE}}}resources")
    build=ET.SubElement(main,f"{{{CORE}}}build")
    settings=ET.Element("config")
    plate=ET.SubElement(settings,"plate")
    ET.SubElement(plate,"metadata",{"key":"plater_id","value":"1"})
    ET.SubElement(plate,"metadata",{"key":"filament_map_mode","value":"Auto For Flush"})
    origin=[45,100,155,210]
    for i in range(4):
        objid=i+2
        obj=ET.SubElement(rs,f"{{{CORE}}}object",{"id":str(objid),"type":"model"})
        components=ET.SubElement(obj,f"{{{CORE}}}components")
        ET.SubElement(components,f"{{{CORE}}}component",{
            "objectid":"1",f"{{{PROD}}}path":"/3D/Objects/object_1.model",
            "transform":"1 0 0 0 1 0 0 0 1 0 0 10"})
        ET.SubElement(build,f"{{{CORE}}}item",{
            "objectid":str(objid),
            "transform":f"1 0 0 0 1 0 0 0 1 {origin[i]} 135 0",
            "printable":"1"})
        item=ET.SubElement(settings,"object",{"id":str(objid)})
        ET.SubElement(item,"metadata",{"key":"name","value":f"U1 QA extruder {i+1}"})
        ET.SubElement(item,"metadata",{"key":"extruder","value":str(i+1)})
        inst=ET.SubElement(plate,"model_instance")
        ET.SubElement(inst,"metadata",{"key":"object_id","value":str(objid)})
        ET.SubElement(inst,"metadata",{"key":"instance_id","value":"0"})
        ET.SubElement(inst,"metadata",{"key":"identify_id","value":str(i+101)})
    rel=ET.Element(f"{{{REL}}}Relationships")
    ET.SubElement(rel,f"{{{REL}}}Relationship",{"Target":"/3D/3dmodel.model",
       "Id":"rel-1","Type":"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"})
    model_rel=ET.Element(f"{{{REL}}}Relationships")
    ET.SubElement(model_rel,f"{{{REL}}}Relationship",{
        "Target":"/3D/Objects/object_1.model","Id":"rel-model-1",
        "Type":"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"})
    content=ET.Element(f"{{{TYPES}}}Types")
    ET.SubElement(content,f"{{{TYPES}}}Default",{"Extension":"rels",
       "ContentType":"application/vnd.openxmlformats-package.relationships+xml"})
    ET.SubElement(content,f"{{{TYPES}}}Default",{"Extension":"model",
       "ContentType":"application/vnd.ms-package.3dmanufacturing-3dmodel+xml"})
    with zipfile.ZipFile(out,"w",compression=zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml",xml(content))
        z.writestr("_rels/.rels",xml(rel))
        z.writestr("3D/3dmodel.model",xml(main))
        z.writestr("3D/Objects/object_1.model",xml(objroot))
        z.writestr("3D/_rels/3dmodel.model.rels",xml(model_rel))
        z.writestr("Metadata/model_settings.config",xml(settings))
    # Check file structure, four separate instances, and distinct extruders.
    with zipfile.ZipFile(out) as z:
        a=ET.fromstring(z.read("Metadata/model_settings.config"))
        ids=[(int(o.attrib["id"]),int(o.find("./metadata[@key='extruder']").attrib["value"]))
             for o in a.findall("object")]
        if ids != [(2,1),(3,2),(4,3),(5,4)]:
            raise AssertionError("FOUR_OBJECT_EXTRUDER_ASSIGNMENT_NOT_PRESERVED")
    return {"fixture_sha256":digest(out),"object_extruders":ids,
            "origins_xyz":[[x,135,10] for x in origin],
            "source":"generated 3MF test geometry, NOT OEM 3MF, OEM engine original"}



def build_from_official_3mf(template:Path,out:Path)->dict:
    """Clone an OEM-provided working 3MF's existing model/OPC structure.

    Only changes the temporary QA model instances and extruder labels;
    keeps the source archive byte-identical. No vendor engine/preset changes.
    """
    import copy
    import uuid
    # Preserve the original 3MF *default* XML namespace. The Snapmaker BBS
    # importer did not register any objects when ElementTree rewrote it as
    # ns0:model/ns0:object; plate mappings then had no obj_map entries.
    ET.register_namespace("",CORE)
    ET.register_namespace("p",PROD)
    with zipfile.ZipFile(template) as zin:
        payload={name:zin.read(name) for name in zin.namelist()}
    model_name="3D/3dmodel.model"
    if model_name not in payload:
        raise ValueError("OEM_TEMPLATE_MISSING_MASTER_MODEL")
    master=ET.fromstring(payload[model_name])
    ns=master.tag.partition("}")[0] + "}"
    resources=master.find(ns+"resources")
    build=master.find(ns+"build")
    if resources is None or build is None or not list(build):
        raise ValueError("OEM_TEMPLATE_MISSING_OBJECTS_OR_BUILD")
    initial_item=list(build)[0]
    original_id=initial_item.attrib.get("objectid")
    original_object=next((obj for obj in resources
                          if obj.attrib.get("id")==original_id),None)
    if original_object is None:
        raise ValueError("OEM_TEMPLATE_MODEL_ID_NOT_REFERENCED")
    # The OEM handy model's build item points to a COMPONENTS wrapper.
    # Snapmaker BBS importer's object map does not accept duplicated wrapper
    # IDs as new top-level print objects. Clone the referenced *mesh* itself.
    mesh_object=next((obj for obj in resources if obj.find(ns+"mesh") is not None),None)
    if mesh_object is None:
        raise ValueError("OEM_TEMPLATE_MISSING_DIRECT_MESH")
    original_object=mesh_object
    settings_name="Metadata/model_settings.config"
    setting_root=ET.fromstring(payload[settings_name]) if settings_name in payload else ET.Element("config")
    old_entry=next((e for e in setting_root.findall("object")
                    if e.attrib.get("id")==original_id),None)
    old_plate=setting_root.find("plate")
    if old_plate is None:
        old_plate=ET.SubElement(setting_root,"plate")
    # A handy-model 3MF is not a full slicer project and may not contain
    # any plate metadata. Snapmaker 2.4.1's BBS importer nevertheless
    # consumes model_settings.config and rejects a plate with index absent.
    plate_id=old_plate.find("./metadata[@key='plater_id']")
    if plate_id is None:
        ET.SubElement(old_plate,"metadata",{"key":"plater_id","value":"1"})
    else:
        plate_id.attrib["value"]="1"
    existing_ids=[int(x.attrib["id"]) for x in resources if x.attrib.get("id","").isdigit()]
    base_id=max(existing_ids+[1])+1
    for item in list(build):
        build.remove(item)
    for item in list(setting_root.findall("object")):
        setting_root.remove(item)
    for item in list(old_plate.findall("model_instance")):
        old_plate.remove(item)
    identifiers=[]
    # Keep only original model resources as dependencies, but no original
    # item is built: precisely four new copies will be on the build plate.
    for idx in range(4):
        object_id=str(base_id+idx)
        clone=copy.deepcopy(original_object)
        clone.attrib["id"]=object_id
        for zidx,el in enumerate(clone.iter()):
            for attr in list(el.attrib):
                if attr.endswith("}UUID"):
                    el.attrib[attr]=str(uuid.uuid5(uuid.NAMESPACE_URL,
                        f"filuna-fourtool-{idx}-{zidx}-{el.tag}"))
        resources.append(clone)
        item=copy.deepcopy(initial_item)
        item.attrib["objectid"]=object_id
        item.attrib["transform"]=f"1 0 0 0 1 0 0 0 1 {45+idx*55} 135 15"
        for attr in list(item.attrib):
            if attr.endswith("}UUID"):
                item.attrib[attr]=str(uuid.uuid5(uuid.NAMESPACE_URL,f"filuna-fourtool-build-{idx}"))
        build.append(item)
        setting=copy.deepcopy(old_entry) if old_entry is not None else ET.Element("object")
        setting.attrib["id"]=object_id
        for key,value in (("extruder",str(idx+1)),("name",f"U1 official QA tool {idx+1}")):
            meta=setting.find(f"./metadata[@key='{key}']")
            if meta is None:
                meta=ET.SubElement(setting,"metadata",{"key":key})
            meta.attrib["value"]=value
        setting_root.append(setting)
        instance=ET.SubElement(old_plate,"model_instance")
        ET.SubElement(instance,"metadata",{"key":"object_id","value":object_id})
        ET.SubElement(instance,"metadata",{"key":"instance_id","value":"0"})
        ET.SubElement(instance,"metadata",{"key":"identify_id","value":str(100+idx)})
        identifiers.append((int(object_id),idx+1))
    payload[model_name]=xml(master)
    payload[settings_name]=xml(setting_root)
    with zipfile.ZipFile(out,"w",zipfile.ZIP_DEFLATED) as zout:
        for name,data in payload.items():
            zout.writestr(name,data)
    with zipfile.ZipFile(out) as z:
        check=ET.fromstring(z.read(settings_name))
        actual=sorted((int(e.attrib["id"]),
                       int(e.find("./metadata[@key='extruder']").attrib["value"]))
                      for e in check.findall("object"))
        if actual!=sorted(identifiers):
            raise AssertionError("FOUR_INDEPENDENT_OBJECT_EXTRUDERS_LOST")
    return {"model_source":"OFFICIAL_PORTABLE_HANDBOOK_3MF",
            "official_template_path":str(template),"official_template_sha256":digest(template),
            "qa_fourtool_sha256":digest(out),
            "qa_assignments":identifiers,
            "official_template_was_modified":False,
            "payload_files":len(payload)}


def build_minimal_core_3mf(out:Path)->dict:
    """Minimal 3MF Core 1.3 OPC cube, without BBS project metadata.

    This tests whether Snapmaker's *unchanged* official Windows CLI can
    slice any standards-only 3MF. It does not impersonate an OEM project,
    nor test four extruders, nor alter official G-code or vendor files.
    """
    ET.register_namespace("",CORE)
    root=ET.Element(f"{{{CORE}}}model",{"unit":"millimeter"})
    ET.SubElement(root,f"{{{CORE}}}metadata",{"name":"Application"}).text="Filuna QA standards-only 3MF"
    resources=ET.SubElement(root,f"{{{CORE}}}resources")
    obj=ET.SubElement(resources,f"{{{CORE}}}object",{"id":"1","type":"model"})
    mesh=ET.SubElement(obj,f"{{{CORE}}}mesh")
    vertices=ET.SubElement(mesh,f"{{{CORE}}}vertices")
    coords=[(-10,-10,-10),(10,-10,-10),(10,10,-10),(-10,10,-10),
            (-10,-10,10),(10,-10,10),(10,10,10),(-10,10,10)]
    for x,y,z in coords:
        ET.SubElement(vertices,f"{{{CORE}}}vertex",
                      {"x":str(x),"y":str(y),"z":str(z)})
    faces=[(0,2,1),(0,3,2),(4,5,6),(4,6,7),
           (0,1,5),(0,5,4),(1,2,6),(1,6,5),
           (2,3,7),(2,7,6),(3,0,4),(3,4,7)]
    triangles=ET.SubElement(mesh,f"{{{CORE}}}triangles")
    for a,b,c in faces:
        ET.SubElement(triangles,f"{{{CORE}}}triangle",
                      {"v1":str(a),"v2":str(b),"v3":str(c)})
    build=ET.SubElement(root,f"{{{CORE}}}build")
    ET.SubElement(build,f"{{{CORE}}}item",
                  {"objectid":"1","transform":"1 0 0 0 1 0 0 0 1 135 135 10"})
    relationships=ET.Element(f"{{{REL}}}Relationships")
    ET.SubElement(relationships,f"{{{REL}}}Relationship",
                  {"Target":"/3D/3dmodel.model","Id":"rel-1",
                   "Type":"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"})
    types=ET.Element(f"{{{TYPES}}}Types")
    ET.SubElement(types,f"{{{TYPES}}}Default",
                  {"Extension":"rels",
                   "ContentType":"application/vnd.openxmlformats-package.relationships+xml"})
    ET.SubElement(types,f"{{{TYPES}}}Default",
                  {"Extension":"model",
                   "ContentType":"application/vnd.ms-package.3dmanufacturing-3dmodel+xml"})
    # OPC relationships and content-type XML must use the same default
    # namespace serialization as the manufacturer's working package.
    # ElementTree otherwise writes ns0:Relationships/ns0:Types; XML-valid,
    # but vendor parsers may incorrectly assume unprefixed root elements.
    ET.register_namespace("",TYPES)
    types_xml=xml(types)
    ET.register_namespace("",REL)
    relationships_xml=xml(relationships)
    ET.register_namespace("",CORE)
    model_xml=xml(root)
    with zipfile.ZipFile(out,"w",compression=zipfile.ZIP_DEFLATED) as package:
        package.writestr("[Content_Types].xml",types_xml)
        package.writestr("_rels/.rels",relationships_xml)
        package.writestr("3D/3dmodel.model",model_xml)
    with zipfile.ZipFile(out) as package:
        check=ET.fromstring(package.read("3D/3dmodel.model"))
        ns=f"{{{CORE}}}"
        if len(check.findall("./"+ns+"resources/"+ns+"object/"+ns+"mesh/"+ns+"vertices/"+ns+"vertex"))!=8:
            raise AssertionError("STANDARD_3MF_8_VERTICES_MISSING")
        if len(check.findall("./"+ns+"resources/"+ns+"object/"+ns+"mesh/"+ns+"triangles/"+ns+"triangle"))!=12:
            raise AssertionError("STANDARD_3MF_12_TRIANGLES_MISSING")
        if len(check.findall("./"+ns+"build/"+ns+"item"))!=1:
            raise AssertionError("STANDARD_3MF_PLATE_BUILD_ITEM_MISSING")
    return {"type":"STANDARD_3MF_CORE_SINGLE_CUBE","sha256":digest(out),
            "filename":out.name,"plate_center":[135,135],"one_extruder_only":True,
            "oem_data_modified":False}


def build_four_from_core_3mf(seed:Path,out:Path)->dict:
    """Produce four independent cubes + OEM-style extruder metadata from a
    successfully sliced standards-only 3MF. Vendor code/presets stay unchanged.
    This is a *new synthetic QA 3MF*, never an authentic OEM 4-color project.
    """
    import copy
    with zipfile.ZipFile(seed) as z:
        payload={name:z.read(name) for name in z.namelist()}
    root=ET.fromstring(payload["3D/3dmodel.model"])
    ns=f"{{{CORE}}}"
    resources=root.find(ns+"resources")
    build=root.find(ns+"build")
    if resources is None or build is None or len(resources)!=1:
        raise ValueError("FOUR_CORE_SEED_NOT_SINGLE_MODEL")
    template=resources.find(ns+"object")
    if template is None or template.find(ns+"mesh") is None:
        raise ValueError("FOUR_CORE_SEED_MESH_MISSING")
    for element in list(resources):
        resources.remove(element)
    for element in list(build):
        build.remove(element)
    config=ET.Element("config")
    plate=ET.SubElement(config,"plate")
    ET.SubElement(plate,"metadata",{"key":"plater_id","value":"1"})
    ET.SubElement(plate,"metadata",{"key":"filament_map_mode","value":"Auto For Flush"})
    # OEM A 2x2 placement, keyed by the original tool/instance order.
    locations=[(146.5,147),(146.5,125),(124.5,147),(124.5,125)]
    for i,(x,y) in enumerate(locations):
        identifier=str(i+1)
        item=copy.deepcopy(template)
        item.attrib["id"]=identifier
        resources.append(item)
        ET.SubElement(build,ns+"item",{
            "objectid":identifier,
            "transform":f"1 0 0 0 1 0 0 0 1 {x} {y} 10",
            "printable":"1"})
        obj_config=ET.SubElement(config,"object",{"id":identifier})
        ET.SubElement(obj_config,"metadata",{"key":"name","value":f"Core QA tool {i+1}"})
        ET.SubElement(obj_config,"metadata",{"key":"extruder","value":str(i+1)})
        instance=ET.SubElement(plate,"model_instance")
        for key,value in (("object_id",identifier),("instance_id","0"),("identify_id",str(100+i))):
            ET.SubElement(instance,"metadata",{"key":key,"value":value})
    ET.register_namespace("",CORE)
    payload["3D/3dmodel.model"]=xml(root)
    payload["Metadata/model_settings.config"]=xml(config)
    with zipfile.ZipFile(out,"w",compression=zipfile.ZIP_DEFLATED) as z:
        for name,data in payload.items():
            z.writestr(name,data)
    with zipfile.ZipFile(out) as z:
        model=ET.fromstring(z.read("3D/3dmodel.model"))
        metadata=ET.fromstring(z.read("Metadata/model_settings.config"))
    objects=model.findall("./"+ns+"resources/"+ns+"object")
    items=model.findall("./"+ns+"build/"+ns+"item")
    extruders=[int(o.find("./metadata[@key='extruder']").attrib["value"])
               for o in metadata.findall("object")]
    if len(objects)!=4 or len(items)!=4 or extruders!=[1,2,3,4]:
        raise AssertionError("FOUR_CORE_EXTRUDER_CONTRACT_BROKEN")
    return {"type":"QA_3MF_CORE_PLUS_BBS_METADATA_FOUR_CUBES",
            "sha256":digest(out),"from_seed_sha256":digest(seed),
            "object_count":4,"extruder_values":extruders,
            "oem_code_or_presets_modified":False,"not_official_oem_project":True}


def export_oem_profiled_3mf(executable:Path,profiles:dict,process:Path,cube:Path,out:Path)->dict:
    """Unmodified OEM executable exports its own 3MF from known-good STL.

    This isolates a generic 3MF CLI import failure from a bundled handy
    model's metadata. Never edits vendor code, process or generated G-code.
    """
    destination=out/"oem-cli-exported-cube-control.3mf"
    log=out/"oem-cli-exported-cube-control.log"
    command=[str(executable),"--debug","3",
             "--load-settings",str(profiles["machine"])+";"+str(process),
             "--load-filaments",";".join([str(profiles["filament"])]*4),
             "--export-3mf",str(destination),str(cube.resolve())]
    with log.open("w",encoding="utf-8",errors="replace") as fp:
        fp.write("COMMAND_JSON="+json.dumps(command,ensure_ascii=False)+"\\n")
        fp.flush()
        try:
            code=subprocess.run(command,cwd=executable.parent,stdout=fp,
                                stderr=subprocess.STDOUT,timeout=220,
                                check=False).returncode
        except subprocess.TimeoutExpired:
            code=124
            fp.write("TIMEOUT\\n")
    valid=destination.is_file() and zipfile.is_zipfile(destination)
    return {"exit_code":code,"valid_3mf":valid,"sha256":digest(destination) if valid else None,
            "output_path":str(destination) if valid else None,"log":log.name}


def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    ap.add_argument("--out",type=Path,default=Path("qa-snapmaker-fourtool"))
    a=ap.parse_args()
    out=a.out.resolve();cache=a.cache.resolve()
    out.mkdir(parents=True,exist_ok=True)
    report={"status":"NOT_RUN","scope":"Snapmaker Orca 2.4.1 official CLI four tools only",
            "filuna_native_parity":"NOT_RUN","device_network":"NOT_RUN",
            "hardware_physical":"NOT_RUN","original_vendor_data_modified":False,
            "qa_runtime_nozzle_guard_expected":True}
    try:
        package,source_url=obtain(OFFICIAL["snapmaker"],cache)
        if digest(package)!="1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d":
            raise ValueError("OEM_PORTABLE_ZIP_SHA_MISMATCH")
        executable=extract_executable(package,OFFICIAL["snapmaker"]["exe"],
            cache/"snapmaker-portable-extracted").resolve()
        profiles=build_effective_original_presets(out)
        adjusted=out/"qa-process-runtime-guard.json"
        report["temporary_process_guard"]=guarded_runtime_process(
            profiles["machine"],profiles["process"],adjusted)
        model=out/"four-extruder-object-fixture.3mf"
        template=executable.parent/"resources/handy_models/OrcaCube_v2.3mf"
        if not template.is_file():
            raise ValueError("OFFICIAL_OEM_3MF_TEMPLATE_NOT_PRESENT")
        report["model"]=build_from_official_3mf(template,model)
        # Essential causality control: does original unmodified OEM handy 3MF
        # itself work through the official CLI? A crash shared with the
        # 4-tool copy cannot be attributed to our extruder assignments.
        original_control=run_cli(executable,profiles["machine"],adjusted,
            profiles["filament"],template,out,"official-unmodified-3mf-control",timeout=220)
        report["original_unmodified_oem_template_control"]=original_control
        # Causality controls using the exact same official binary and settings:
        # (1) prove raw STL can be sliced; (2) OEM-export a project 3MF from it;
        # (3) try to slice that untouched OEM-produced 3MF. These do not
        # promote a four-tool PASS and are preserved when the latter fails.
        cube=(ROOT/"fixtures/cube20.stl").resolve()
        if not cube.is_file():
            raise ValueError("ORIGINAL_STL_CONTROL_MISSING")
        report["raw_stl_control"]=run_cli(executable,profiles["machine"],adjusted,
            profiles["filament"],cube,out,"oem-raw-stl-control",timeout=220)
        # Distinguish general 3MF CLI handling from proprietary bundled
        # BBS project metadata: standards-only model with no plate metadata.
        minimal_3mf=out/"qa-minimal-core-cube.3mf"
        report["minimal_core_3mf_fixture"]=build_minimal_core_3mf(minimal_3mf)
        report["minimal_core_3mf_slice_control"]=run_cli(
            executable,profiles["machine"],adjusted,profiles["filament"],
            minimal_3mf,out,"minimal-core-3mf-control",timeout=220)
        four_core=out/"qa-core-four-distinct-tools.3mf"
        report["four_core_3mf_fixture"]=build_four_from_core_3mf(minimal_3mf,four_core)
        diagnostic=run_cli(executable,profiles["machine"],adjusted,
            profiles["filament"],four_core,out,"four-core-3mf-control",timeout=220)
        if diagnostic["gcodes"]:
            path=out/diagnostic["gcodes"][0]["path"]
            data=path.read_bytes()
            diagnostic["tool_numbers"]=sorted({int(m.group(1)) for m in
                re.finditer(rb"(?m)^\s*T([0-3])(?:\s|;|$)",data)})
            diagnostic["toolchange_blocks"]=len(re.findall(
                rb"(?m)^;\s*CP TOOLCHANGE START",data))
        report["four_core_3mf_slice_control"]=diagnostic
        if diagnostic["exit_code"]==0 and len(diagnostic["gcodes"])==1:
            gcode_a=(out/diagnostic["gcodes"][0]["path"]).read_bytes()
            # Run in different wall-clock seconds. Only a manufacturer-proven
            # HEADER_BLOCK timestamp may be excluded from the comparison.
            time.sleep(2.5)
            second=run_cli(executable,profiles["machine"],adjusted,
                profiles["filament"],four_core,out,"four-core-3mf-repeated-B",
                timeout=220)
            report["four_core_3mf_repeat_B"]=second
            if second["exit_code"]==0 and len(second["gcodes"])==1:
                gcode_b=(out/second["gcodes"][0]["path"]).read_bytes()
                try:
                    report["four_core_strict_A_B"]=compare_temporally_separated(
                        gcode_a,gcode_b)
                except ValueError as exc:
                    report["four_core_strict_A_B"]={"status":"FAIL",
                        "reason":str(exc),"both_gcodes_original":True,
                        "manufacturer_toolpaths_unchanged":True}
            else:
                report["four_core_strict_A_B"]={"status":"NOT_PROVEN",
                    "reason":"SECOND_OEM_CLI_NOT_SUCCESSFUL"}
            geometry=validate_gcode(gcode_a,policy_from_yaml(
                ROOT/"FILUNA/fabricantes/snapmaker/vendor.yaml"),source="official")
            report["four_core_spatial"]={
                "status":geometry["status"],"issues":geometry["issues"][:12],
                "warnings":geometry["warnings"][:12],
                "gcode_sha256":geometry["gcode_sha256"]}
            if diagnostic.get("tool_numbers")==[0,1,2,3] and diagnostic.get("toolchange_blocks",0)>=3:
                report["four_core_tool_motion_proof"]="PASS_SYNTHETIC_4_TOOL_OEM_CLI_ONLY"
            else:
                report["four_core_tool_motion_proof"]="FAIL_TOOL_ASSIGNMENTS"
        else:
            report["four_core_tool_motion_proof"]="NOT_PROVEN"
        exported=export_oem_profiled_3mf(executable,profiles,adjusted,cube,out)
        report["oem_exported_3mf_control"]=exported
        if exported["valid_3mf"]:
            report["oem_exported_3mf_slice_control"]=run_cli(
                executable,profiles["machine"],adjusted,profiles["filament"],
                Path(exported["output_path"]),out,"oem-exported-3mf-slice-control",timeout=220)
        else:
            report["oem_exported_3mf_slice_control"]={"status":"NOT_RUN",
                "reason":"OFFICIAL_OEM_EXPORT_3MF_FAILED"}
        report["vendor_original_zip_sha256"]=digest(package)
        report["vendor_url"]=source_url
        gcode_hashes=[]
        for idx in range(2):
            label=f"four-tool-{idx+1}"
            result=run_cli(executable,profiles["machine"],adjusted,
                profiles["filament"],model,out,label,timeout=220)
            report[label]=result
            if result["exit_code"]!=0 or len(result["gcodes"])!=1:
                report["status"]="FAIL_VENDOR_FOUR_TOOL_CLI"
                return 2
            path=out/result["gcodes"][0]["path"]
            raw=path.read_bytes()
            tools=sorted({int(m.group(1)) for m in re.finditer(
                rb"(?m)^\s*T([0-3])(?:\s|;|$)",raw)})
            # A command may be present as startup initialization as well.
            changes=len(re.findall(rb"(?m)^;\s*CP TOOLCHANGE START",raw))
            report[label]["tools_in_gcode"]=tools
            report[label]["toolchange_blocks"]=changes
            geometry=validate_gcode(raw,policy_from_yaml(
                ROOT/"FILUNA/fabricantes/snapmaker/vendor.yaml"),source="official")
            report[label]["spatial"]={
                "status":geometry["status"],"issues":geometry["issues"][:4],
                "warnings":geometry["warnings"][:4], "gcode_sha256":geometry["gcode_sha256"]}
            gcode_hashes.append(digest(path))
            if tools!=[0,1,2,3] or changes<3:
                report["status"]="FAIL_FOUR_TOOL_ASSIGNMENTS_NOT_PROVED"
                return 2
        report["hashes_across_two_runs"]=gcode_hashes
        report["status"]="OFFICIAL_FOUR_TOOL_CLI_PASS_SPATIAL_RECORDED"
        return 0
    except Exception as exc:
        report["status"]="FAIL_"+type(exc).__name__
        report["error"]=str(exc)
        return 2
    finally:
        (out/"fourtool-proof.json").write_text(
            json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        print("U1_OFFICIAL_FOUR_TOOL",report["status"],flush=True)


if __name__=="__main__":
    raise SystemExit(main())
