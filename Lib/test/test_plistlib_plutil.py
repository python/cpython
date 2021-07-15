import os
import unittest
import plistlib
import subprocess
import json
import sys

@unittest.skipUnless(sys.platform == "darwin", "plutil utility is for Mac os")
class TestWidgetMock(unittest.TestCase):

    file_name = "plutil_test.plist"
    properties = {
            "fname" : "H",
            "lname":"A",
            "marks" : {"a":100, "b":0x10}
        }
    exptected_properties = {
        "fname" : "H",
        "lname": "A",
        "marks" : {"a":100, "b":16}
    }
    pl = {
            "HexType" : 0x0100000c,
            "IntType" : 0o123
        }

    @classmethod
    def setUpClass(cls) -> None:
        ## Generate plist file with plistlib and parse with plutil
        with open(cls.file_name,'wb') as f:
            plistlib.dump(cls.properties, f, fmt=plistlib.FMT_BINARY)

    @classmethod
    def tearDownClass(cls) -> None:
        os.remove(cls.file_name)

    def get_lint_status(self):
        return subprocess.run(['plutil', "-lint", self.file_name], capture_output=True, text=True).stdout

    def convert_bin_to_json(self):
        """Convert binary file to json using plutil
        """
        subprocess.run(['plutil', "-convert", 'json', self.file_name])

    def convert_to_bin(self):
        """Convert file to binary using plutil
        """
        subprocess.run(['plutil', "-convert", 'binary1', self.file_name])

    def write_pl(self):
        """Write Hex properties to file using writePlist
        """
        plistlib.writePlist(self.pl, self.file_name)

    def test_lint_status(self):
        # check lint status of file using plutil
        self.assertEqual(f"{self.file_name}: OK\n", self.get_lint_status())

    def check_content(self):
        # check file content with plutil converting binary to json
        self.convert_bin_to_json()
        with open(self.file_name) as f:
            ff = json.loads(f.read())
            self.assertEqual(ff, self.exptected_properties)
        
    def check_plistlib_parse(self):
        # Generate plist files with plutil and parse with plistlib
        self.convert_to_bin()
        with open(self.file_name, 'rb') as f:
            self.assertEqual(plistlib.load(f), self.exptected_properties)


    def test_octal_and_hex(self):
        self.write_pl()
        self.convert_bin_to_json()
        
        with open(self.file_name, 'r') as f:
            p = json.loads(f.read())
            self.assertEqual(p.get("HexType"), 16777228)
            self.assertEqual(p.get("IntType"), 83)


if __name__ == "__main__":
    unittest.main()

