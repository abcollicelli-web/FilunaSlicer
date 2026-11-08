#!/usr/bin/env python3
"""Source-level regression contracts for Snapmaker U1 parity toggles.

These guard option spelling, defaults, and native emission branches.
Actual M104/M109 and Klipper G-code behavior must also be checked in the
fresh Windows executable's native proof before parity approval.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class SnapmakerParitySourceContracts(unittest.TestCase):
    def test_toolchange_wait_default_and_native_branch(self):
        h = (ROOT / "src/libslic3r/PrintConfig.hpp").read_text()
        cpp = (ROOT / "src/libslic3r/PrintConfig.cpp").read_text()
        g = (ROOT / "src/libslic3r/GCode.cpp").read_text()
        self.assertIn("((ConfigOptionBool,               tool_change_temprature_wait))", h)
        section = cpp.split('this->add("tool_change_temprature_wait", coBool)',1)[1].split('this->add(',1)[0]
        self.assertIn("ConfigOptionBool(true)",section)
        self.assertIn("gcodegen.config().tool_change_temprature_wait.value",g)
        self.assertIn("set_temperature(this->_get_temp(gcodegen), gcodegen.config().tool_change_temprature_wait.value",g)

    def test_klipper_ratio_default_and_conditional(self):
        h = (ROOT / "src/libslic3r/PrintConfig.hpp").read_text()
        cpp = (ROOT / "src/libslic3r/PrintConfig.cpp").read_text()
        writer = (ROOT / "src/libslic3r/GCodeWriter.cpp").read_text()
        self.assertIn("((ConfigOptionBool,                klipper_emit_minimum_cruise_ratio))",h)
        section = cpp.split('this->add("klipper_emit_minimum_cruise_ratio", coBool)',1)[1].split('this->add(',1)[0]
        self.assertIn("ConfigOptionBool(true)",section)
        self.assertIn("if (emit_minimum_cruise_ratio)",writer)
        self.assertIn("this->config.klipper_emit_minimum_cruise_ratio.value);",writer)
        tower = (ROOT / "src/libslic3r/GCode/WipeTower.cpp").read_text()
        self.assertIn("m_klipper_emit_minimum_cruise_ratio",tower)
        self.assertIn('gcode += " MINIMUM_CRUISE_RATIO=";',writer)

if __name__ == "__main__":
    unittest.main()
