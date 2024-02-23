import unittest
from ctypes import (c_byte, c_short, c_int, c_long, c_longlong,
                    c_ubyte, c_ushort, c_uint, c_ulong, c_ulonglong,
                    c_float, c_double, c_longdouble, c_bool, c_char)


subclasses = []
for base in [c_byte, c_short, c_int, c_long, c_longlong,
        c_ubyte, c_ushort, c_uint, c_ulong, c_ulonglong,
        c_float, c_double, c_longdouble, c_bool]:
    class X(base):
        pass
    subclasses.append(X)


class X(c_char):
    pass


# This test checks if the __repr__ is correct for subclasses of simple types
class ReprTest(unittest.TestCase):
    def test_numbers(self):
        for typ in subclasses:
            base = typ.__bases__[0]
            self.assertTrue(repr(base(42)).startswith(base.__name__))
            self.assertEqual("<X object at", repr(typ(42))[:12])

    def test_char(self):
        self.assertEqual("c_char(b'x')", repr(c_char(b'x')))
        self.assertEqual("<X object at", repr(X(b'x'))[:12])


if __name__ == "__main__":
    unittest.main()
