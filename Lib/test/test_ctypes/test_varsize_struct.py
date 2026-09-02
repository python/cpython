import unittest
from ctypes import Structure, sizeof, resize, c_int


class VarSizeTest(unittest.TestCase):
    def test_resize(self):
        class X(Structure):
            _fields_ = [("item", c_int),
                        ("array", c_int * 1)]

        self.assertEqual(sizeof(X), sizeof(c_int) * 2)
        x = X()
        x.item = 42
        x.array[0] = 100
        self.assertEqual(sizeof(x), sizeof(c_int) * 2)

        # make room for one additional item
        new_size = sizeof(X) + sizeof(c_int) * 1
        resize(x, new_size)
        self.assertEqual(sizeof(x), new_size)
        self.assertEqual((x.item, x.array[0]), (42, 100))

        # make room for 10 additional items
        new_size = sizeof(X) + sizeof(c_int) * 9
        resize(x, new_size)
        self.assertEqual(sizeof(x), new_size)
        self.assertEqual((x.item, x.array[0]), (42, 100))

        # make room for one additional item
        new_size = sizeof(X) + sizeof(c_int) * 1
        resize(x, new_size)
        self.assertEqual(sizeof(x), new_size)
        self.assertEqual((x.item, x.array[0]), (42, 100))

    def test_array_invalid_length(self):
        # cannot create arrays with non-positive size
        self.assertRaises(ValueError, lambda: c_int * -1)
        self.assertRaises(ValueError, lambda: c_int * -3)

    def test_zerosized_array(self):
        array = (c_int * 0)()
        # accessing elements of zero-sized arrays raise IndexError
        self.assertRaises(IndexError, array.__setitem__, 0, None)
        self.assertRaises(IndexError, array.__getitem__, 0)
        self.assertRaises(IndexError, array.__setitem__, 1, None)
        self.assertRaises(IndexError, array.__getitem__, 1)
        self.assertRaises(IndexError, array.__setitem__, -1, None)
        self.assertRaises(IndexError, array.__getitem__, -1)


if __name__ == "__main__":
    unittest.main()
