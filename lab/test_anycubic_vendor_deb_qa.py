#!/usr/bin/env python3
"""Vendor APT provenance negative tests: never trust repackaged third-party assets."""
import unittest
from anycubic_vendor_deb_qa import find_exact_package, apt_records, FILENAME, SHA256, VERSION, BYTES


def valid():
    return {"Package":"anycubicslicernext","Version":VERSION,"Filename":FILENAME,
            "SHA256":SHA256,"Size":str(BYTES)}


class TestOriginalAnycubicAPT(unittest.TestCase):
    def test_vendor_version_size_and_sha_exact(self):
        self.assertEqual(find_exact_package([valid()])["SHA256"], SHA256)

    def test_bad_sha_rejected(self):
        x=valid();x["SHA256"]="0"*64
        with self.assertRaisesRegex(ValueError,"SHA256"):
            find_exact_package([x])

    def test_unofficial_file_rejected(self):
        x=valid();x["Filename"]="untrusted-repacked.AppImage"
        with self.assertRaisesRegex(ValueError,"Filename"):
            find_exact_package([x])

    def test_bad_size_rejected(self):
        x=valid();x["Size"]="10"
        with self.assertRaisesRegex(ValueError,"Size"):
            find_exact_package([x])

    def test_ambiguous_packages_rejected(self):
        with self.assertRaisesRegex(ValueError,"EXPECTED_ONE_VENDOR_PACKAGE_FOUND_2"):
            find_exact_package([valid(),valid()])

    def test_duplicate_field_rejected(self):
        with self.assertRaisesRegex(ValueError,"DUPLICATED_APT_FIELD"):
            apt_records("Package: anycubicslicernext\nPackage: anycubicslicernext\n")


if __name__=="__main__":
    unittest.main()
