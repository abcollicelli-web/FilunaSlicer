#!/usr/bin/env python3
"""Structural and physical-topology checks for isolated Snapmaker 3MF CLI fixture.

These tests do not validate OEM fatiamento, motion, macros or printing.
"""
import tempfile
import unittest
from collections import Counter
from pathlib import Path
from xml.etree import ElementTree as ET
import zipfile

from snapmaker_u1_fourtool_original_qa import CORE, build_minimal_core_3mf

class MinimalCore3MF(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.fixture=Path(self.tmp.name)/"qa-minimal-core-cube.3mf"
        self.meta=build_minimal_core_3mf(self.fixture)

    def test_opc_package_contains_complete_rels_and_content_type(self):
        with zipfile.ZipFile(self.fixture) as z:
            self.assertEqual(set(z.namelist()),{
                "[Content_Types].xml","_rels/.rels","3D/3dmodel.model"})
            self.assertIsNone(z.testzip())
            links=ET.fromstring(z.read("_rels/.rels"))
            self.assertEqual(len(links),1)
            self.assertEqual(list(links)[0].attrib["Target"],"/3D/3dmodel.model")
            content=ET.fromstring(z.read("[Content_Types].xml"))
            self.assertEqual(len(content),2)
        self.assertEqual(len(self.meta["sha256"]),64)
        self.assertFalse(self.meta["oem_data_modified"])

    def test_closed_cube_8_vertices_12_triangles_single_build(self):
        n="{"+CORE+"}"
        with zipfile.ZipFile(self.fixture) as z:
            root=ET.fromstring(z.read("3D/3dmodel.model"))
        verts=root.findall("./"+n+"resources/"+n+"object/"+n+"mesh/"+n+"vertices/"+n+"vertex")
        tris=root.findall("./"+n+"resources/"+n+"object/"+n+"mesh/"+n+"triangles/"+n+"triangle")
        items=root.findall("./"+n+"build/"+n+"item")
        self.assertEqual(len(verts),8)
        self.assertEqual(len(tris),12)
        self.assertEqual(len(items),1)
        self.assertEqual(items[0].attrib["objectid"],"1")
        self.assertEqual(len(items[0].attrib["transform"].split()),12)
        self.assertEqual(len(set(tuple(v.attrib.values()) for v in verts)),8)
        edges=Counter()
        for t in tris:
            a,b,c=(int(t.attrib[k]) for k in ("v1","v2","v3"))
            self.assertEqual(len({a,b,c}),3)
            self.assertTrue(all(0<=k<8 for k in (a,b,c)))
            edges.update(tuple(sorted(e)) for e in ((a,b),(b,c),(c,a)))
        self.assertEqual(len(edges),18)
        self.assertTrue(all(count==2 for count in edges.values()))

if __name__=="__main__":
    unittest.main()
