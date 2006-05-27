from ctypes import *
import unittest
import sys

class Test(unittest.TestCase):

    def test_array2pointer(self):
        array = (c_int * 3)(42, 17, 2)

        # casting an array to a pointer works.
        ptr = cast(array, POINTER(c_int))
        self.failUnlessEqual([ptr[i] for i in range(3)], [42, 17, 2])

        if 2*sizeof(c_short) == sizeof(c_int):
            ptr = cast(array, POINTER(c_short))
            if sys.byteorder == "little":
                self.failUnlessEqual([ptr[i] for i in range(6)],
                                     [42, 0, 17, 0, 2, 0])
            else:
                self.failUnlessEqual([ptr[i] for i in range(6)],
                                     [0, 42, 0, 17, 0, 2])

    def test_address2pointer(self):
        array = (c_int * 3)(42, 17, 2)

        address = addressof(array)
        ptr = cast(c_void_p(address), POINTER(c_int))
        self.failUnlessEqual([ptr[i] for i in range(3)], [42, 17, 2])

        ptr = cast(address, POINTER(c_int))
        self.failUnlessEqual([ptr[i] for i in range(3)], [42, 17, 2])


    def test_ptr2array(self):
        array = (c_int * 3)(42, 17, 2)

        from sys import getrefcount

        before = getrefcount(array)
        ptr = cast(array, POINTER(c_int))
        self.failUnlessEqual(getrefcount(array), before + 1)
        del ptr
        self.failUnlessEqual(getrefcount(array), before)

if __name__ == "__main__":
    unittest.main()
