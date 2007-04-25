# To fully test this module, we would need a copy of the stringprep tables.
# Since we don't have them, this test checks only a few codepoints.

import unittest
from test import test_support

from stringprep import *

class StringprepTests(unittest.TestCase):
    def test(self):
        self.failUnless(in_table_a1(u"\u0221"))
        self.failIf(in_table_a1(u"\u0222"))

        self.failUnless(in_table_b1(u"\u00ad"))
        self.failIf(in_table_b1(u"\u00ae"))

        self.failUnless(map_table_b2(u"\u0041"), u"\u0061")
        self.failUnless(map_table_b2(u"\u0061"), u"\u0061")

        self.failUnless(map_table_b3(u"\u0041"), u"\u0061")
        self.failUnless(map_table_b3(u"\u0061"), u"\u0061")

        self.failUnless(in_table_c11(u"\u0020"))
        self.failIf(in_table_c11(u"\u0021"))

        self.failUnless(in_table_c12(u"\u00a0"))
        self.failIf(in_table_c12(u"\u00a1"))

        self.failUnless(in_table_c12(u"\u00a0"))
        self.failIf(in_table_c12(u"\u00a1"))

        self.failUnless(in_table_c11_c12(u"\u00a0"))
        self.failIf(in_table_c11_c12(u"\u00a1"))

        self.failUnless(in_table_c21(u"\u001f"))
        self.failIf(in_table_c21(u"\u0020"))

        self.failUnless(in_table_c22(u"\u009f"))
        self.failIf(in_table_c22(u"\u00a0"))

        self.failUnless(in_table_c21_c22(u"\u009f"))
        self.failIf(in_table_c21_c22(u"\u00a0"))

        self.failUnless(in_table_c3(u"\ue000"))
        self.failIf(in_table_c3(u"\uf900"))

        self.failUnless(in_table_c4(u"\uffff"))
        self.failIf(in_table_c4(u"\u0000"))

        self.failUnless(in_table_c5(u"\ud800"))
        self.failIf(in_table_c5(u"\ud7ff"))

        self.failUnless(in_table_c6(u"\ufff9"))
        self.failIf(in_table_c6(u"\ufffe"))

        self.failUnless(in_table_c7(u"\u2ff0"))
        self.failIf(in_table_c7(u"\u2ffc"))

        self.failUnless(in_table_c8(u"\u0340"))
        self.failIf(in_table_c8(u"\u0342"))

        # C.9 is not in the bmp
        # self.failUnless(in_table_c9(u"\U000E0001"))
        # self.failIf(in_table_c8(u"\U000E0002"))

        self.failUnless(in_table_d1(u"\u05be"))
        self.failIf(in_table_d1(u"\u05bf"))

        self.failUnless(in_table_d2(u"\u0041"))
        self.failIf(in_table_d2(u"\u0040"))

        # This would generate a hash of all predicates. However, running
        # it is quite expensive, and only serves to detect changes in the
        # unicode database. Instead, stringprep.py asserts the version of
        # the database.

        # import hashlib
        # predicates = [k for k in dir(stringprep) if k.startswith("in_table")]
        # predicates.sort()
        # for p in predicates:
        #     f = getattr(stringprep, p)
        #     # Collect all BMP code points
        #     data = ["0"] * 0x10000
        #     for i in range(0x10000):
        #         if f(unichr(i)):
        #             data[i] = "1"
        #     data = "".join(data)
        #     h = hashlib.sha1()
        #     h.update(data)
        #     print p, h.hexdigest()

def test_main():
    test_support.run_unittest(StringprepTests)

if __name__ == '__main__':
    test_main()
