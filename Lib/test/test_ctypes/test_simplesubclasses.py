import unittest
from ctypes import Structure, CFUNCTYPE, c_int, _SimpleCData
from ._support import (_CData, PyCSimpleType, Py_TPFLAGS_DISALLOW_INSTANTIATION,
                       Py_TPFLAGS_IMMUTABLETYPE)


class MyInt(c_int):
    def __eq__(self, other):
        if type(other) != MyInt:
            return NotImplementedError
        return self.value == other.value


class Test(unittest.TestCase):
    def test_inheritance_hierarchy(self):
        self.assertEqual(_SimpleCData.mro(), [_SimpleCData, _CData, object])

        self.assertEqual(PyCSimpleType.__name__, "PyCSimpleType")
        self.assertEqual(type(PyCSimpleType), type)

        self.assertEqual(c_int.mro(), [c_int, _SimpleCData, _CData, object])

    def test_type_flags(self):
        for cls in _SimpleCData, PyCSimpleType:
            with self.subTest(cls=cls):
                self.assertTrue(_SimpleCData.__flags__ & Py_TPFLAGS_IMMUTABLETYPE)
                self.assertFalse(_SimpleCData.__flags__ & Py_TPFLAGS_DISALLOW_INSTANTIATION)

    def test_metaclass_details(self):
        # Abstract classes (whose metaclass __init__ was not called) can't be
        # instantiated directly
        NewT = PyCSimpleType.__new__(PyCSimpleType, 'NewT', (_SimpleCData,), {})
        for cls in _SimpleCData, NewT:
            with self.subTest(cls=cls):
                with self.assertRaisesRegex(TypeError, "abstract class"):
                    obj = cls()

        # Cannot call the metaclass __init__ more than once
        class T(_SimpleCData):
            _type_ = "i"
        with self.assertRaisesRegex(SystemError, "already initialized"):
            PyCSimpleType.__init__(T, 'ptr', (), {})

    def test_swapped_type_creation(self):
        cls = PyCSimpleType.__new__(PyCSimpleType, '', (), {'_type_': 'i'})
        with self.assertRaises(TypeError):
            PyCSimpleType.__init__(cls)
        PyCSimpleType.__init__(cls, '', (), {'_type_': 'i'})
        self.assertEqual(cls.__ctype_le__.__dict__.get('_type_'), 'i')
        self.assertEqual(cls.__ctype_be__.__dict__.get('_type_'), 'i')

    def test_compare(self):
        self.assertEqual(MyInt(3), MyInt(3))
        self.assertNotEqual(MyInt(42), MyInt(43))

    def test_ignore_retval(self):
        # Test if the return value of a callback is ignored
        # if restype is None
        proto = CFUNCTYPE(None)
        def func():
            return (1, "abc", None)

        cb = proto(func)
        self.assertEqual(None, cb())


    def test_int_callback(self):
        args = []
        def func(arg):
            args.append(arg)
            return arg

        cb = CFUNCTYPE(None, MyInt)(func)

        self.assertEqual(None, cb(42))
        self.assertEqual(type(args[-1]), MyInt)

        cb = CFUNCTYPE(c_int, c_int)(func)

        self.assertEqual(42, cb(42))
        self.assertEqual(type(args[-1]), int)

    def test_int_struct(self):
        class X(Structure):
            _fields_ = [("x", MyInt)]

        self.assertEqual(X().x, MyInt())

        s = X()
        s.x = MyInt(42)

        self.assertEqual(s.x, MyInt(42))


if __name__ == "__main__":
    unittest.main()
