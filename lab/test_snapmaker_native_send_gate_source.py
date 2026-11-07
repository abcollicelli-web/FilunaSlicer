#!/usr/bin/env python3
"""Static seam audit: native PrintHost upload must run read-only OEM gate.

This does not stand in for native compilation or an integration test with
actual connected hardware. Probes source order and fail-closed behavior.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
CPP=(ROOT/"src/slic3r/Utils/PrintHost.cpp").read_text(encoding="utf-8")
HPP=(ROOT/"src/slic3r/Utils/PrintHost.hpp").read_text(encoding="utf-8")
NATIVE=(ROOT/"src/slic3r/Utils/FilunaGCodeSafety.hpp").read_text(encoding="utf-8")
GUI=(ROOT/"src/slic3r/GUI/PrintHostDialogs.cpp").read_text(encoding="utf-8")

class NativeSendGateContract(unittest.TestCase):
    def test_01_effective_profile_captured_at_job_creation(self):
        self.assertIn('config->option<ConfigOptionString>("printer_model")',CPP)
        self.assertIn('filuna_effective_printer_model = model->value',CPP)
        self.assertIn('filuna_effective_printer_model(std::move(other.filuna_effective_printer_model))',HPP)
        self.assertIn('filuna_effective_printer_model = std::move(other.filuna_effective_printer_model)',HPP)

    def test_02_gate_immediately_precedes_actual_upload(self):
        upload=CPP.split('bool PrintHostJobQueue::upload_job(',1)[1]
        self.assertIn('job.filuna_effective_printer_model == "Snapmaker U1"',upload)
        self.assertIn('Filuna::Safety::inspect(original_bytes.str())',upload)
        self.assertLess(upload.index('Filuna::Safety::inspect(original_bytes.str())'),
                        upload.index('job.printhost->upload('))
        self.assertIn('throw std::runtime_error("Filuna U1: geometry cannot be validated',upload)
        self.assertIn('G-code unchanged; upload cancelled',upload)

    def test_03_no_postprocessing_or_external_python(self):
        upload=CPP.split('bool PrintHostJobQueue::upload_job(',1)[1]
        self.assertNotIn('python ',upload)
        self.assertNotIn('system(',upload)
        self.assertNotIn('process_gcode(',upload)
        self.assertNotIn('gcode +=',upload)
        self.assertIn('boost::filesystem::ifstream source(job.upload_data.source_path',upload)

    def test_04_unknown_snapmaker_3mf_blocks(self):
        upload=CPP.split('bool PrintHostJobQueue::upload_job(',1)[1]
        self.assertIn('if (job.upload_data.use_3mf)',upload)
        self.assertIn('3MF upload cannot be cleared',upload)
        self.assertIn('original_bytes << source.rdbuf()',upload)

    def test_05_the_core_is_actual_cpp_not_a_python_hook(self):
        self.assertIn('Report inspect(const std::string& gcode',NATIVE)
        self.assertIn('Report inspect_file(',NATIVE)
        self.assertIn('TRAVEL_CROSSES_PRINTED_GEOMETRY',NATIVE)
        self.assertIn('GCODE_TRAVEL_CROSSES_ALREADY_PRINTED_PART',NATIVE)
        self.assertNotIn('std::system(',NATIVE)

    def test_07_warning_has_real_gui_handler(self):
        self.assertIn('info_fn(wxString::FromUTF8("filuna_u1_safety_warning")',CPP)
        self.assertIn('if (evt.tag == "filuna_u1_safety_warning")', GUI)
        self.assertIn('wxMessageBox(evt.status', GUI)
        self.assertIn('wxICON_WARNING, this)', GUI)

    def test_06_no_unverified_physical_override(self):
        upload=CPP.split('bool PrintHostJobQueue::upload_job(',1)[1]
        self.assertNotIn('USER_ACCEPTED_RISK',upload)
        self.assertNotIn('upload_allowed = true',upload)
        self.assertIn('original_bytes.str()',upload)

    def test_08_warn_requires_ack_before_transfer(self):
        # The UI receives info_fn asynchronously. A WARN may not be cleared
        # merely by queuing wxMessageBox while the worker uploads the file.
        upload=CPP.split('bool PrintHostJobQueue::upload_job(',1)[1]
        warn='if (safety.decision == Filuna::Safety::Report::Decision::Warn)'
        self.assertIn(warn,upload)
        section=upload.split(warn,1)[1].split('success = job.printhost->upload(',1)[0]
        self.assertIn('filuna_u1_safety_warning',section)
        self.assertIn('throw std::runtime_error("Filuna U1: geometry warning requires confirmation before upload',section)
        self.assertIn('authenticated approval is not available',section)
        self.assertIn('G-code unchanged; upload cancelled',section)
        # No success path may be entered inside the WARN branch.
        self.assertNotIn('upload_allowed = true',section)

if __name__=="__main__":
    unittest.main()
