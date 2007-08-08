import unittest
from ctypes import *

formats = "bBhHiIlLqQfd"

formats = c_byte, c_ubyte, c_short, c_ushort, c_int, c_uint, \
          c_long, c_ulonglong, c_float, c_double

class ArrayTestCase(unittest.TestCase):
    def test_simple(self):
        # create classes holding simple numeric types, and check
        # various properties.

        init = list(range(15, 25))

        for fmt in formats:
            alen = len(init)
            int_array = ARRAY(fmt, alen)

            ia = int_array(*init)
            # length of instance ok?
            self.failUnlessEqual(len(ia), alen)

            # slot values ok?
            values = [ia[i] for i in range(len(init))]
            self.failUnlessEqual(values, init)

            # change the items
            from operator import setitem
            new_values = list(range(42, 42+alen))
            [setitem(ia, n, new_values[n]) for n in range(alen)]
            values = [ia[i] for i in range(len(init))]
            self.failUnlessEqual(values, new_values)

            # are the items initialized to 0?
            ia = int_array()
            values = [ia[i] for i in range(len(init))]
            self.failUnlessEqual(values, [0] * len(init))

            # Too many in itializers should be caught
            self.assertRaises(IndexError, int_array, *range(alen*2))

        CharArray = ARRAY(c_char, 3)

        ca = CharArray("a", "b", "c")

        # Should this work? It doesn't:
        # CharArray("abc")
        self.assertRaises(TypeError, CharArray, "abc")

        self.failUnlessEqual(ca[0], b"a")
        self.failUnlessEqual(ca[1], b"b")
        self.failUnlessEqual(ca[2], b"c")
        self.failUnlessEqual(ca[-3], b"a")
        self.failUnlessEqual(ca[-2], b"b")
        self.failUnlessEqual(ca[-1], b"c")

        self.failUnlessEqual(len(ca), 3)

        # slicing is now supported, but not extended slicing (3-argument)!
        from operator import getslice, delitem
        self.assertRaises(TypeError, getslice, ca, 0, 1, -1)

        # cannot delete items
        self.assertRaises(TypeError, delitem, ca, 0)

    def test_numeric_arrays(self):

        alen = 5

        numarray = ARRAY(c_int, alen)

        na = numarray()
        values = [na[i] for i in range(alen)]
        self.failUnlessEqual(values, [0] * alen)

        na = numarray(*[c_int()] * alen)
        values = [na[i] for i in range(alen)]
        self.failUnlessEqual(values, [0]*alen)

        na = numarray(1, 2, 3, 4, 5)
        values = [i for i in na]
        self.failUnlessEqual(values, [1, 2, 3, 4, 5])

        na = numarray(*map(c_int, (1, 2, 3, 4, 5)))
        values = [i for i in na]
        self.failUnlessEqual(values, [1, 2, 3, 4, 5])

    def test_classcache(self):
        self.failUnless(not ARRAY(c_int, 3) is ARRAY(c_int, 4))
        self.failUnless(ARRAY(c_int, 3) is ARRAY(c_int, 3))

    def test_from_address(self):
        # Failed with 0.9.8, reported by JUrner
        p = create_string_buffer("foo")
        sz = (c_char * 3).from_address(addressof(p))
        self.failUnlessEqual(sz[:], "foo")
        self.failUnlessEqual(sz.value, "foo")

    try:
        create_unicode_buffer
    except NameError:
        pass
    else:
        def test_from_addressW(self):
            p = create_unicode_buffer("foo")
            sz = (c_wchar * 3).from_address(addressof(p))
            self.failUnlessEqual(sz[:], "foo")
            self.failUnlessEqual(sz.value, "foo")

if __name__ == '__main__':
    unittest.main()
