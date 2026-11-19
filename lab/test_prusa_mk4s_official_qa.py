#!/usr/bin/env python3
"""Offline negative and positive controls for the official Prusa preset resolver."""
import unittest

from prusa_mk4s_official_qa import effective_config, git_blob, resolve, sections


class TestPrusaMK4SResolver(unittest.TestCase):
    def test_multi_parent_precedence_and_chain(self):
        source = """[printer:base]
nozzle_diameter = 0.5
start_gcode = G28
[printer:overlay]
nozzle_diameter = 0.4
[printer:target]
inherits = base; overlay
printer_model = MK4S
"""
        index = sections(source)
        values, chain = resolve(index, "printer", "target")
        self.assertEqual(values["nozzle_diameter"], "0.4")
        self.assertEqual(chain, ["base", "overlay", "target"])
        self.assertEqual(values["start_gcode"], "G28")

    def test_fail_closed_on_missing_parent(self):
        with self.assertRaisesRegex(ValueError, "UNRESOLVED_OFFICIAL_PARENT"):
            resolve(sections("[print:test]\ninherits = absent\n"), "print", "test")

    def test_fail_closed_on_cycle(self):
        index = sections("[filament:a]\ninherits = b\n[filament:b]\ninherits = a\n")
        with self.assertRaisesRegex(ValueError, "INHERITANCE_CYCLE"):
            resolve(index, "filament", "a")

    def test_fail_closed_on_duplicate_key(self):
        with self.assertRaisesRegex(ValueError, "DUPLICATE_OFFICIAL_KEY"):
            sections("[print:test]\nfoo = 1\nfoo = 2\n")

    def test_model_identity_is_not_guessed(self):
        index = sections("""[printer:Original Prusa MK4S 0.4 nozzle]
printer_model = MK4S
printer_variant = 0.6
nozzle_diameter = 0.4
default_print_profile = 0.20mm SPEED @MK4S 0.4
default_filament_profile = "Prusament PLA @MK4S"
start_gcode = G28
bed_shape = 0x0,250x0
[print:0.20mm SPEED @MK4S 0.4]
layer_height = 0.2
[filament:Prusament PLA @MK4S]
temperature = 215
""")
        with self.assertRaisesRegex(ValueError, "WRONG_OFFICIAL_MODEL_OR_NOZZLE"):
            effective_config(index)

    def test_git_blob_hash_respects_bytes(self):
        self.assertEqual(git_blob(b"test\n"), "9daeafb9864cf43055ae93beb0afd6c7d144bfa4")
        self.assertNotEqual(git_blob(b"test\r\n"), git_blob(b"test\n"))


if __name__ == "__main__":
    unittest.main()
