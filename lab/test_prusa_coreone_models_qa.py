#!/usr/bin/env python3
"""Negative controls prevent crossing CORE One / CORE One L and 0.4 / HF."""
import unittest
from prusa_coreone_models_qa import TARGETS, derive
from prusa_mk4s_official_qa import sections


def example(model="COREONE", nozzle="0.4", high_flow="0",
            printer_name="Prusa CORE One 0.4 nozzle",
            profile="0.20mm SPEED @COREONE 0.4", height="270",
            bed="0x0,250x0,250x220,0x220"):
    return sections("""[printer:%s]
printer_model = %s
printer_variant = 0.4
nozzle_diameter = %s
nozzle_high_flow = %s
bed_shape = %s
max_print_height = %s
default_print_profile = %s
default_filament_profile = "Prusament PLA @COREONE"
start_gcode = G28
binary_gcode = 1
[print:%s]
layer_height = 0.2
[filament:Prusament PLA @COREONE]
temperature = 215
""" % (printer_name, model, nozzle, high_flow, bed, height, profile, profile))


class TestExactCoreOneModels(unittest.TestCase):
    def test_model_ids_are_separate(self):
        self.assertNotEqual(TARGETS["coreone_04"]["printer_model"],
                            TARGETS["coreone_l_04"]["printer_model"])

    def test_correct_coreone_derivation(self):
        cfg, lineage, _ = derive(example(), TARGETS["coreone_04"])
        self.assertEqual(cfg["nozzle_diameter"], "0.4")
        self.assertEqual(cfg["binary_gcode"], "1")

    def test_wrong_model_rejected(self):
        with self.assertRaisesRegex(ValueError,"WRONG_PRESET_printer_model"):
            derive(example(model="COREONEL"), TARGETS["coreone_04"])

    def test_high_flow_not_mistaken_for_normal(self):
        with self.assertRaisesRegex(ValueError,"WRONG_PRESET_nozzle_high_flow"):
            derive(example(high_flow="1"), TARGETS["coreone_04"])

    def test_wrong_bed_rejected(self):
        with self.assertRaisesRegex(ValueError,"WRONG_PRESET_bed_shape"):
            derive(example(bed="0x0,300x0,300x300,0x300"),TARGETS["coreone_04"])

    def test_wrong_default_profile_rejected(self):
        with self.assertRaisesRegex(ValueError,"WRONG_PRESET_default_print_profile"):
            derive(example(profile="0.20mm BALANCED @COREONE 0.4"), TARGETS["coreone_04"])


if __name__=="__main__":
    unittest.main()
