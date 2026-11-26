import sys, unittest
from ctypes import (Structure, BigEndianStructure, LittleEndianStructure,
                    c_byte, c_short, c_int, c_long, c_longlong,
                    c_float, c_double,
                    c_ushort, c_uint, c_ulong, c_ulonglong)


structures = []
byteswapped_structures = []


if sys.byteorder == "little":
    SwappedStructure = BigEndianStructure
else:
    SwappedStructure = LittleEndianStructure

for typ in [c_short, c_int, c_long, c_longlong,
            c_float, c_double,
            c_ushort, c_uint, c_ulong, c_ulonglong]:
    class X(Structure):
        _pack_ = 1
        _layout_ = 'ms'
        _fields_ = [("pad", c_byte),
                    ("value", typ)]
    class Y(SwappedStructure):
        _pack_ = 1
        _layout_ = 'ms'
        _fields_ = [("pad", c_byte),
                    ("value", typ)]
    structures.append(X)
    byteswapped_structures.append(Y)


class TestStructures(unittest.TestCase):
    def test_native(self):
        for typ in structures:
            self.assertEqual(typ.value.offset, 1)
            o = typ()
            o.value = 4
            self.assertEqual(o.value, 4)

    def test_swapped(self):
        for typ in byteswapped_structures:
            self.assertEqual(typ.value.offset, 1)
            o = typ()
            o.value = 4
            self.assertEqual(o.value, 4)


if __name__ == '__main__':
    unittest.main()
