# Copyright (C) 2003 Python Software Foundation

import unittest
import os
from test import test_support

aetypes = test_support.import_module('aetypes')
aepack = test_support.import_module('aepack')

class TestAepack(unittest.TestCase):
    OBJECTS = [
        aetypes.Enum('enum'),
        aetypes.Type('type'),
        aetypes.Keyword('kwrd'),
        aetypes.Range(1, 10),
        aetypes.Comparison(1, '<   ', 10),
        aetypes.Logical('not ', 1),
        aetypes.IntlText(0, 0, 'international text'),
        aetypes.IntlWritingCode(0,0),
        aetypes.QDPoint(50,100),
        aetypes.QDRectangle(50,100,150,200),
        aetypes.RGBColor(0x7000, 0x6000, 0x5000),
        aetypes.Unknown('xxxx', 'unknown type data'),
        aetypes.Character(1),
        aetypes.Character(2, aetypes.Line(2)),
    ]

    def test_roundtrip_string(self):
        o = 'a string'
        packed = aepack.pack(o)
        unpacked = aepack.unpack(packed)
        self.assertEqual(o, unpacked)

    def test_roundtrip_int(self):
        o = 12
        packed = aepack.pack(o)
        unpacked = aepack.unpack(packed)
        self.assertEqual(o, unpacked)

    def test_roundtrip_float(self):
        o = 12.1
        packed = aepack.pack(o)
        unpacked = aepack.unpack(packed)
        self.assertEqual(o, unpacked)

    def test_roundtrip_None(self):
        o = None
        packed = aepack.pack(o)
        unpacked = aepack.unpack(packed)
        self.assertEqual(o, unpacked)

    def test_roundtrip_aeobjects(self):
        for o in self.OBJECTS:
            packed = aepack.pack(o)
            unpacked = aepack.unpack(packed)
            self.assertEqual(repr(o), repr(unpacked))

    def test_roundtrip_FSSpec(self):
        try:
            import Carbon.File
        except:
            self.skipTest('Carbon.File not available')

        if not hasattr(Carbon.File, "FSSpec"):
            self.skipTest('Carbon.File.FSSpec not available')
        o = Carbon.File.FSSpec(os.curdir)
        packed = aepack.pack(o)
        unpacked = aepack.unpack(packed)
        self.assertEqual(o.as_pathname(), unpacked.as_pathname())

    def test_roundtrip_Alias(self):
        try:
            import Carbon.File
        except:
            self.skipTest('Carbon.File not available')
        if not hasattr(Carbon.File, "FSSpec"):
            self.skipTest('Carbon.File.FSSpec not available')
        o = Carbon.File.FSSpec(os.curdir).NewAliasMinimal()
        packed = aepack.pack(o)
        unpacked = aepack.unpack(packed)
        self.assertEqual(o.FSResolveAlias(None)[0].as_pathname(),
            unpacked.FSResolveAlias(None)[0].as_pathname())


def test_main():
    test_support.run_unittest(TestAepack)


if __name__ == '__main__':
    test_main()
