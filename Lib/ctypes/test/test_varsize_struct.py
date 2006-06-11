from ctypes import *
import unittest

class VarSizeTest(unittest.TestCase):
    def test_resize(self):
        class X(Structure):
            _fields_ = [("item", c_int),
                        ("array", c_int * 1)]

        self.failUnlessEqual(sizeof(X), sizeof(c_int) * 2)
        x = X()
        x.item = 42
        x.array[0] = 100
        self.failUnlessEqual(sizeof(x), sizeof(c_int) * 2)

        # make room for one additional item
        new_size = sizeof(X) + sizeof(c_int) * 1
        resize(x, new_size)
        self.failUnlessEqual(sizeof(x), new_size)
        self.failUnlessEqual((x.item, x.array[0]), (42, 100))

        # make room for 10 additional items
        new_size = sizeof(X) + sizeof(c_int) * 9
        resize(x, new_size)
        self.failUnlessEqual(sizeof(x), new_size)
        self.failUnlessEqual((x.item, x.array[0]), (42, 100))

        # make room for one additional item
        new_size = sizeof(X) + sizeof(c_int) * 1
        resize(x, new_size)
        self.failUnlessEqual(sizeof(x), new_size)
        self.failUnlessEqual((x.item, x.array[0]), (42, 100))

    def test_array_invalid_length(self):
        # cannot create arrays with non-positive size
        self.failUnlessRaises(ValueError, lambda: c_int * -1)
        self.failUnlessRaises(ValueError, lambda: c_int * -3)

    def test_zerosized_array(self):
        array = (c_int * 0)()
        # accessing elements of zero-sized arrays raise IndexError
        self.failUnlessRaises(IndexError, array.__setitem__, 0, None)
        self.failUnlessRaises(IndexError, array.__getitem__, 0)
        self.failUnlessRaises(IndexError, array.__setitem__, 1, None)
        self.failUnlessRaises(IndexError, array.__getitem__, 1)
        self.failUnlessRaises(IndexError, array.__setitem__, -1, None)
        self.failUnlessRaises(IndexError, array.__getitem__, -1)

    def test_varsized_array(self):
        array = (c_int * 20)(20, 21, 22, 23, 24, 25, 26, 27, 28, 29)

        # no range checking is done on arrays with size == 1
        varsize_array = (c_int * 1).from_address(addressof(array))

        # __getitem__
        self.failUnlessEqual(varsize_array[0], 20)
        self.failUnlessEqual(varsize_array[1], 21)
        self.failUnlessEqual(varsize_array[2], 22)
        self.failUnlessEqual(varsize_array[3], 23)
        self.failUnlessEqual(varsize_array[4], 24)
        self.failUnlessEqual(varsize_array[5], 25)
        self.failUnlessEqual(varsize_array[6], 26)
        self.failUnlessEqual(varsize_array[7], 27)
        self.failUnlessEqual(varsize_array[8], 28)
        self.failUnlessEqual(varsize_array[9], 29)

        # still, normal sequence of length one behaviour:
        self.failUnlessEqual(varsize_array[-1], 20)
        self.failUnlessRaises(IndexError, lambda: varsize_array[-2])
        # except for this one, which will raise MemoryError
        self.failUnlessRaises(MemoryError, lambda: varsize_array[:])

        # __setitem__
        varsize_array[0] = 100
        varsize_array[1] = 101
        varsize_array[2] = 102
        varsize_array[3] = 103
        varsize_array[4] = 104
        varsize_array[5] = 105
        varsize_array[6] = 106
        varsize_array[7] = 107
        varsize_array[8] = 108
        varsize_array[9] = 109

        for i in range(10):
            self.failUnlessEqual(varsize_array[i], i + 100)
            self.failUnlessEqual(array[i], i + 100)

        # __getslice__
        self.failUnlessEqual(varsize_array[0:10], range(100, 110))
        self.failUnlessEqual(varsize_array[1:9], range(101, 109))
        self.failUnlessEqual(varsize_array[1:-1], [])

        # __setslice__
        varsize_array[0:10] = range(1000, 1010)
        self.failUnlessEqual(varsize_array[0:10], range(1000, 1010))

        varsize_array[1:9] = range(1001, 1009)
        self.failUnlessEqual(varsize_array[1:9], range(1001, 1009))

    def test_vararray_is_sane(self):
        array = (c_int * 15)(20, 21, 22, 23, 24, 25, 26, 27, 28, 29)

        varsize_array = (c_int * 1).from_address(addressof(array))
        varsize_array[:] = [1, 2, 3, 4, 5]

        self.failUnlessEqual(array[:], [1, 2, 3, 4, 5, 25, 26, 27, 28, 29, 0, 0, 0, 0, 0])
        self.failUnlessEqual(varsize_array[0:10], [1, 2, 3, 4, 5, 25, 26, 27, 28, 29])

        array[:5] = [10, 11, 12, 13, 14]
        self.failUnlessEqual(array[:], [10, 11, 12, 13, 14, 25, 26, 27, 28, 29, 0, 0, 0, 0, 0])
        self.failUnlessEqual(varsize_array[0:10], [10, 11, 12, 13, 14, 25, 26, 27, 28, 29])

if __name__ == "__main__":
    unittest.main()
