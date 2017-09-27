import unittest
from test.support import cpython_only
from ctypes import *

class AnonTest(unittest.TestCase):

    def test_anon(self):
        class ANON(Union):
            _fields_ = [("a", c_int),
                        ("b", c_int)]

        class Y(Structure):
            _fields_ = [("x", c_int),
                        ("_", ANON),
                        ("y", c_int)]
            _anonymous_ = ["_"]

        self.assertEqual(Y.a.offset, sizeof(c_int))
        self.assertEqual(Y.b.offset, sizeof(c_int))

        self.assertEqual(ANON.a.offset, 0)
        self.assertEqual(ANON.b.offset, 0)

    def test_anon_nonseq(self):
        # TypeError: _anonymous_ must be a sequence
        self.assertRaises(TypeError,
                              lambda: type(Structure)("Name",
                                                      (Structure,),
                                                      {"_fields_": [], "_anonymous_": 42}))

    def test_anon_nonmember(self):
        # AttributeError: type object 'Name' has no attribute 'x'
        self.assertRaises(AttributeError,
                              lambda: type(Structure)("Name",
                                                      (Structure,),
                                                      {"_fields_": [],
                                                       "_anonymous_": ["x"]}))

    @cpython_only
    def test_issue31490(self):
        # There shouldn't be an assertion failure in case the class has an
        # attribute whose name is specified in _anonymous_ but not in _fields_.

        # AttributeError: 'x' is specified in _anonymous_ but not in _fields_
        with self.assertRaises(AttributeError):
            class Name(Structure):
                _fields_ = []
                _anonymous_ = ["x"]
                x = 42

    def test_nested(self):
        class ANON_S(Structure):
            _fields_ = [("a", c_int)]

        class ANON_U(Union):
            _fields_ = [("_", ANON_S),
                        ("b", c_int)]
            _anonymous_ = ["_"]

        class Y(Structure):
            _fields_ = [("x", c_int),
                        ("_", ANON_U),
                        ("y", c_int)]
            _anonymous_ = ["_"]

        self.assertEqual(Y.x.offset, 0)
        self.assertEqual(Y.a.offset, sizeof(c_int))
        self.assertEqual(Y.b.offset, sizeof(c_int))
        self.assertEqual(Y._.offset, sizeof(c_int))
        self.assertEqual(Y.y.offset, sizeof(c_int) * 2)

if __name__ == "__main__":
    unittest.main()
