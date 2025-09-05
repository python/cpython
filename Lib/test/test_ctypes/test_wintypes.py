# See <https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types>
# for reference.
#
# Tests also work on POSIX

import unittest
from ctypes import POINTER, cast, c_int16
from ctypes import wintypes


class WinTypesTest(unittest.TestCase):
    def test_variant_bool(self):
        # reads 16-bits from memory, anything non-zero is True
        for true_value in (1, 32767, 32768, 65535, 65537):
            true = POINTER(c_int16)(c_int16(true_value))
            value = cast(true, POINTER(wintypes.VARIANT_BOOL))
            self.assertEqual(repr(value.contents), 'VARIANT_BOOL(True)')

            vb = wintypes.VARIANT_BOOL()
            self.assertIs(vb.value, False)
            vb.value = True
            self.assertIs(vb.value, True)
            vb.value = true_value
            self.assertIs(vb.value, True)

        for false_value in (0, 65536, 262144, 2**33):
            false = POINTER(c_int16)(c_int16(false_value))
            value = cast(false, POINTER(wintypes.VARIANT_BOOL))
            self.assertEqual(repr(value.contents), 'VARIANT_BOOL(False)')

        # allow any bool conversion on assignment to value
        for set_value in (65536, 262144, 2**33):
            vb = wintypes.VARIANT_BOOL()
            vb.value = set_value
            self.assertIs(vb.value, True)

        vb = wintypes.VARIANT_BOOL()
        vb.value = [2, 3]
        self.assertIs(vb.value, True)
        vb.value = []
        self.assertIs(vb.value, False)

    def assertIsSigned(self, ctype):
        self.assertLess(ctype(-1).value, 0)

    def assertIsUnsigned(self, ctype):
        self.assertGreater(ctype(-1).value, 0)

    def test_signedness(self):
        for ctype in (wintypes.BYTE, wintypes.WORD, wintypes.DWORD,
                     wintypes.BOOLEAN, wintypes.UINT, wintypes.ULONG):
            with self.subTest(ctype=ctype):
                self.assertIsUnsigned(ctype)

        for ctype in (wintypes.BOOL, wintypes.INT, wintypes.LONG):
            with self.subTest(ctype=ctype):
                self.assertIsSigned(ctype)


if __name__ == "__main__":
    unittest.main()
