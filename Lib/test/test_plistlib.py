# Copyright (C) 2003 Python Software Foundation

import unittest
import plistlib
import os
import datetime
from test import test_support


# This test data was generated through Cocoa's NSDictionary class
TESTDATA = b"""<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" \
"http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
        <key>aDate</key>
        <date>2004-10-26T10:33:33Z</date>
        <key>aDict</key>
        <dict>
                <key>aFalseValue</key>
                <false/>
                <key>aTrueValue</key>
                <true/>
                <key>aUnicodeValue</key>
                <string>M\xc3\xa4ssig, Ma\xc3\x9f</string>
                <key>anotherString</key>
                <string>&lt;hello &amp; 'hi' there!&gt;</string>
                <key>deeperDict</key>
                <dict>
                        <key>a</key>
                        <integer>17</integer>
                        <key>b</key>
                        <real>32.5</real>
                        <key>c</key>
                        <array>
                                <integer>1</integer>
                                <integer>2</integer>
                                <string>text</string>
                        </array>
                </dict>
        </dict>
        <key>aFloat</key>
        <real>0.5</real>
        <key>aList</key>
        <array>
                <string>A</string>
                <string>B</string>
                <integer>12</integer>
                <real>32.5</real>
                <array>
                        <integer>1</integer>
                        <integer>2</integer>
                        <integer>3</integer>
                </array>
        </array>
        <key>aString</key>
        <string>Doodah</string>
        <key>anInt</key>
        <integer>728</integer>
        <key>nestedData</key>
        <array>
                <data>
                PGxvdHMgb2YgYmluYXJ5IGd1bms+AAECAzxsb3RzIG9mIGJpbmFyeSBndW5r
                PgABAgM8bG90cyBvZiBiaW5hcnkgZ3Vuaz4AAQIDPGxvdHMgb2YgYmluYXJ5
                IGd1bms+AAECAzxsb3RzIG9mIGJpbmFyeSBndW5rPgABAgM8bG90cyBvZiBi
                aW5hcnkgZ3Vuaz4AAQIDPGxvdHMgb2YgYmluYXJ5IGd1bms+AAECAzxsb3Rz
                IG9mIGJpbmFyeSBndW5rPgABAgM8bG90cyBvZiBiaW5hcnkgZ3Vuaz4AAQID
                PGxvdHMgb2YgYmluYXJ5IGd1bms+AAECAw==
                </data>
        </array>
        <key>someData</key>
        <data>
        PGJpbmFyeSBndW5rPg==
        </data>
        <key>someMoreData</key>
        <data>
        PGxvdHMgb2YgYmluYXJ5IGd1bms+AAECAzxsb3RzIG9mIGJpbmFyeSBndW5rPgABAgM8
        bG90cyBvZiBiaW5hcnkgZ3Vuaz4AAQIDPGxvdHMgb2YgYmluYXJ5IGd1bms+AAECAzxs
        b3RzIG9mIGJpbmFyeSBndW5rPgABAgM8bG90cyBvZiBiaW5hcnkgZ3Vuaz4AAQIDPGxv
        dHMgb2YgYmluYXJ5IGd1bms+AAECAzxsb3RzIG9mIGJpbmFyeSBndW5rPgABAgM8bG90
        cyBvZiBiaW5hcnkgZ3Vuaz4AAQIDPGxvdHMgb2YgYmluYXJ5IGd1bms+AAECAw==
        </data>
        <key>\xc3\x85benraa</key>
        <string>That was a unicode key.</string>
</dict>
</plist>
""".replace(b" " * 8, b"\t")  # Apple as well as plistlib.py output hard tabs


class TestPlistlib(unittest.TestCase):

    def tearDown(self):
        try:
            os.unlink(test_support.TESTFN)
        except:
            pass

    def _create(self):
        pl = dict(
            aString="Doodah",
            aList=["A", "B", 12, 32.5, [1, 2, 3]],
            aFloat = 0.5,
            anInt = 728,
            aDict=dict(
                anotherString="<hello & 'hi' there!>",
                aUnicodeValue='M\xe4ssig, Ma\xdf',
                aTrueValue=True,
                aFalseValue=False,
                deeperDict=dict(a=17, b=32.5, c=[1, 2, "text"]),
            ),
            someData = plistlib.Data(b"<binary gunk>"),
            someMoreData = plistlib.Data(b"<lots of binary gunk>\0\1\2\3" * 10),
            nestedData = [plistlib.Data(b"<lots of binary gunk>\0\1\2\3" * 10)],
            aDate = datetime.datetime(2004, 10, 26, 10, 33, 33),
        )
        pl['\xc5benraa'] = "That was a unicode key."
        return pl

    def test_create(self):
        pl = self._create()
        self.assertEqual(pl["aString"], "Doodah")
        self.assertEqual(pl["aDict"]["aFalseValue"], False)

    def test_io(self):
        pl = self._create()
        plistlib.writePlist(pl, test_support.TESTFN)
        pl2 = plistlib.readPlist(test_support.TESTFN)
        self.assertEqual(dict(pl), dict(pl2))

    def test_bytes(self):
        pl = self._create()
        data = plistlib.writePlistToBytes(pl)
        pl2 = plistlib.readPlistFromBytes(data)
        self.assertEqual(dict(pl), dict(pl2))
        data2 = plistlib.writePlistToBytes(pl2)
        self.assertEqual(data, data2)

    def test_appleformatting(self):
        pl = plistlib.readPlistFromBytes(TESTDATA)
        data = plistlib.writePlistToBytes(pl)
        self.assertEqual(data, TESTDATA,
                         "generated data was not identical to Apple's output")

    def test_appleformattingfromliteral(self):
        pl = self._create()
        pl2 = plistlib.readPlistFromBytes(TESTDATA)
        self.assertEqual(dict(pl), dict(pl2),
                         "generated data was not identical to Apple's output")

    def test_bytesio(self):
        from io import BytesIO
        b = BytesIO()
        pl = self._create()
        plistlib.writePlist(pl, b)
        pl2 = plistlib.readPlist(BytesIO(b.getvalue()))
        self.assertEqual(dict(pl), dict(pl2))

    def test_controlcharacters(self):
        for i in range(128):
            c = chr(i)
            testString = "string containing %s" % c
            if i >= 32 or c in "\r\n\t":
                # \r, \n and \t are the only legal control chars in XML
                plistlib.writePlistToBytes(testString)
            else:
                self.assertRaises(ValueError,
                                  plistlib.writePlistToBytes,
                                  testString)

    def test_nondictroot(self):
        test1 = "abc"
        test2 = [1, 2, 3, "abc"]
        result1 = plistlib.readPlistFromBytes(plistlib.writePlistToBytes(test1))
        result2 = plistlib.readPlistFromBytes(plistlib.writePlistToBytes(test2))
        self.assertEqual(test1, result1)
        self.assertEqual(test2, result2)


def test_main():
    test_support.run_unittest(TestPlistlib)


if __name__ == '__main__':
    test_main()
