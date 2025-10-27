import array
import ctypes
import gc
import sys
import unittest
from ctypes import (CDLL, CFUNCTYPE, Structure,
                    POINTER, pointer, _Pointer,
                    byref, sizeof,
                    c_void_p, c_char_p,
                    c_byte, c_ubyte, c_short, c_ushort, c_int, c_uint,
                    c_long, c_ulong, c_longlong, c_ulonglong,
                    c_float, c_double)
from ctypes import _pointer_type_cache, _pointer_type_cache_fallback
from test.support import import_helper
from weakref import WeakSet
_ctypes_test = import_helper.import_module("_ctypes_test")
from ._support import (_CData, PyCPointerType, Py_TPFLAGS_DISALLOW_INSTANTIATION,
                       Py_TPFLAGS_IMMUTABLETYPE)


ctype_types = [c_byte, c_ubyte, c_short, c_ushort, c_int, c_uint,
                 c_long, c_ulong, c_longlong, c_ulonglong, c_double, c_float]
python_types = [int, int, int, int, int, int,
                int, int, int, int, float, float]


class PointersTestCase(unittest.TestCase):
    def tearDown(self):
        _pointer_type_cache_fallback.clear()

    def test_inheritance_hierarchy(self):
        self.assertEqual(_Pointer.mro(), [_Pointer, _CData, object])

        self.assertEqual(PyCPointerType.__name__, "PyCPointerType")
        self.assertEqual(type(PyCPointerType), type)

    def test_type_flags(self):
        for cls in _Pointer, PyCPointerType:
            with self.subTest(cls=cls):
                self.assertTrue(_Pointer.__flags__ & Py_TPFLAGS_IMMUTABLETYPE)
                self.assertFalse(_Pointer.__flags__ & Py_TPFLAGS_DISALLOW_INSTANTIATION)

    def test_metaclass_details(self):
        # Cannot call the metaclass __init__ more than once
        with self.assertRaisesRegex(SystemError, "already initialized"):
            PyCPointerType.__init__(POINTER(c_byte), 'ptr', (), {})

    def test_pointer_crash(self):

        class A(POINTER(c_ulong)):
            pass

        POINTER(c_ulong)(c_ulong(22))
        # Pointer can't set contents: has no _type_
        self.assertRaises(TypeError, A, c_ulong(33))

    def test_pass_pointers(self):
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_p_p
        if sizeof(c_longlong) == sizeof(c_void_p):
            func.restype = c_longlong
        else:
            func.restype = c_long

        i = c_int(12345678)
        address = func(byref(i))
        self.assertEqual(c_int.from_address(address).value, 12345678)

        func.restype = POINTER(c_int)
        res = func(pointer(i))
        self.assertEqual(res.contents.value, 12345678)
        self.assertEqual(res[0], 12345678)

    def test_change_pointers(self):
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_p_p

        i = c_int(87654)
        func.restype = POINTER(c_int)
        func.argtypes = (POINTER(c_int),)

        res = func(pointer(i))
        self.assertEqual(res[0], 87654)
        self.assertEqual(res.contents.value, 87654)

        # C code: *res = 54345
        res[0] = 54345
        self.assertEqual(i.value, 54345)

        # C code:
        #   int x = 12321;
        #   res = &x
        x = c_int(12321)
        res.contents = x
        self.assertEqual(i.value, 54345)

        x.value = -99
        self.assertEqual(res.contents.value, -99)

    def test_callbacks_with_pointers(self):
        # a function type receiving a pointer
        PROTOTYPE = CFUNCTYPE(c_int, POINTER(c_int))

        self.result = []

        def func(arg):
            for i in range(10):
                self.result.append(arg[i])
            return 0
        callback = PROTOTYPE(func)

        dll = CDLL(_ctypes_test.__file__)
        # This function expects a function pointer,
        # and calls this with an integer pointer as parameter.
        # The int pointer points to a table containing the numbers 1..10
        doit = dll._testfunc_callback_with_pointer

        doit(callback)
        doit(callback)

    def test_basics(self):
        for ct, pt in zip(ctype_types, python_types):
            i = ct(42)
            p = pointer(i)
            self.assertIs(type(p.contents), ct)
            # p.contents is the same as p[0]

            with self.assertRaises(TypeError):
                del p[0]

    def test_from_address(self):
        a = array.array('i', [100, 200, 300, 400, 500])
        addr = a.buffer_info()[0]
        p = POINTER(POINTER(c_int))

    def test_pointer_from_pointer(self):
        p1 = POINTER(c_int)
        p2 = POINTER(p1)

        self.assertIsNot(p1, p2)
        self.assertIs(p1.__pointer_type__, p2)
        self.assertIs(p2._type_, p1)

    def test_other(self):
        class Table(Structure):
            _fields_ = [("a", c_int),
                        ("b", c_int),
                        ("c", c_int)]

        pt = pointer(Table(1, 2, 3))

        self.assertEqual(pt.contents.a, 1)
        self.assertEqual(pt.contents.b, 2)
        self.assertEqual(pt.contents.c, 3)

        pt.contents.c = 33

    def test_basic(self):
        p = pointer(c_int(42))
        # Although a pointer can be indexed, it has no length
        self.assertRaises(TypeError, len, p)
        self.assertEqual(p[0], 42)
        self.assertEqual(p[0:1], [42])
        self.assertEqual(p.contents.value, 42)

    def test_charpp(self):
        """Test that a character pointer-to-pointer is correctly passed"""
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_c_p_p
        func.restype = c_char_p
        argv = (c_char_p * 2)()
        argc = c_int( 2 )
        argv[0] = b'hello'
        argv[1] = b'world'
        result = func( byref(argc), argv )
        self.assertEqual(result, b'world')

    def test_bug_1467852(self):
        # http://sourceforge.net/tracker/?func=detail&atid=532154&aid=1467852&group_id=71702
        x = c_int(5)
        dummy = []
        for i in range(32000):
            dummy.append(c_int(i))
        y = c_int(6)
        p = pointer(x)
        pp = pointer(p)
        q = pointer(y)
        pp[0] = q         # <==
        self.assertEqual(p[0], 6)

    def test_c_void_p(self):
        # http://sourceforge.net/tracker/?func=detail&aid=1518190&group_id=5470&atid=105470
        if sizeof(c_void_p) == 4:
            self.assertEqual(c_void_p(0xFFFFFFFF).value,
                                 c_void_p(-1).value)
            self.assertEqual(c_void_p(0xFFFFFFFFFFFFFFFF).value,
                                 c_void_p(-1).value)
        elif sizeof(c_void_p) == 8:
            self.assertEqual(c_void_p(0xFFFFFFFF).value,
                                 0xFFFFFFFF)
            self.assertEqual(c_void_p(0xFFFFFFFFFFFFFFFF).value,
                                 c_void_p(-1).value)
            self.assertEqual(c_void_p(0xFFFFFFFFFFFFFFFFFFFFFFFF).value,
                                 c_void_p(-1).value)

        self.assertRaises(TypeError, c_void_p, 3.14) # make sure floats are NOT accepted
        self.assertRaises(TypeError, c_void_p, object()) # nor other objects

    def test_read_null_pointer(self):
        null_ptr = POINTER(c_int)()
        with self.assertRaisesRegex(ValueError, "NULL pointer access"):
            null_ptr[0]

    def test_write_null_pointer(self):
        null_ptr = POINTER(c_int)()
        with self.assertRaisesRegex(ValueError, "NULL pointer access"):
            null_ptr[0] = 1

    def test_set_pointer_to_null_and_read(self):
        class Bar(Structure):
            _fields_ = [("values", POINTER(c_int))]

        bar = Bar()
        bar.values = (c_int * 3)(1, 2, 3)

        values = [bar.values[0], bar.values[1], bar.values[2]]
        self.assertEqual(values, [1, 2, 3])

        bar.values = None
        with self.assertRaisesRegex(ValueError, "NULL pointer access"):
            bar.values[0]

    def test_pointers_bool(self):
        # NULL pointers have a boolean False value, non-NULL pointers True.
        self.assertEqual(bool(POINTER(c_int)()), False)
        self.assertEqual(bool(pointer(c_int())), True)

        self.assertEqual(bool(CFUNCTYPE(None)(0)), False)
        self.assertEqual(bool(CFUNCTYPE(None)(42)), True)

        # COM methods are boolean True:
        if sys.platform == "win32":
            mth = ctypes.WINFUNCTYPE(None)(42, "name", (), None)
            self.assertEqual(bool(mth), True)

    def test_pointer_type_name(self):
        LargeNamedType = type('T' * 2 ** 25, (Structure,), {})
        self.assertTrue(POINTER(LargeNamedType))

    def test_pointer_type_str_name(self):
        large_string = 'T' * 2 ** 25
        with self.assertWarns(DeprecationWarning):
            P = POINTER(large_string)
        self.assertTrue(P)

    def test_abstract(self):
        self.assertRaises(TypeError, _Pointer.set_type, 42)

    def test_pointer_types_equal(self):
        t1 = POINTER(c_int)
        t2 = POINTER(c_int)

        self.assertIs(t1, t2)

        p1 = t1(c_int(1))
        p2 = pointer(c_int(1))

        self.assertIsInstance(p1, t1)
        self.assertIsInstance(p2, t1)

        self.assertIs(type(p1), t1)
        self.assertIs(type(p2), t1)

    def test_incomplete_pointer_types_still_equal(self):
        with self.assertWarns(DeprecationWarning):
            t1 = POINTER("LP_C")
        with self.assertWarns(DeprecationWarning):
            t2 = POINTER("LP_C")

        self.assertIs(t1, t2)

    def test_incomplete_pointer_types_cannot_instantiate(self):
        with self.assertWarns(DeprecationWarning):
            t1 = POINTER("LP_C")
        with self.assertRaisesRegex(TypeError, "has no _type_"):
            t1()

    def test_pointer_set_type_twice(self):
        t1 = POINTER(c_int)
        self.assertIs(c_int.__pointer_type__, t1)
        self.assertIs(t1._type_, c_int)

        t1.set_type(c_int)
        self.assertIs(c_int.__pointer_type__, t1)
        self.assertIs(t1._type_, c_int)

    def test_pointer_set_wrong_type(self):
        int_ptr = POINTER(c_int)
        float_ptr = POINTER(c_float)
        try:
            class C(c_int):
                pass

            t1 = POINTER(c_int)
            t2 = POINTER(c_float)
            t1.set_type(c_float)
            self.assertEqual(t1(c_float(1.5))[0], 1.5)
            self.assertIs(t1._type_, c_float)
            self.assertIs(c_int.__pointer_type__, t1)
            self.assertIs(c_float.__pointer_type__, float_ptr)

            t1.set_type(C)
            self.assertEqual(t1(C(123))[0].value, 123)
            self.assertIs(c_int.__pointer_type__, t1)
            self.assertIs(c_float.__pointer_type__, float_ptr)
        finally:
            POINTER(c_int).set_type(c_int)
        self.assertIs(POINTER(c_int), int_ptr)
        self.assertIs(POINTER(c_int)._type_, c_int)
        self.assertIs(c_int.__pointer_type__, int_ptr)

    def test_pointer_not_ctypes_type(self):
        with self.assertRaisesRegex(TypeError, "must have storage info"):
            POINTER(int)

        with self.assertRaisesRegex(TypeError, "must have storage info"):
            pointer(int)

        with self.assertRaisesRegex(TypeError, "must have storage info"):
            pointer(int(1))

    def test_pointer_set_python_type(self):
        p1 = POINTER(c_int)
        with self.assertRaisesRegex(TypeError, "must have storage info"):
            p1.set_type(int)

    def test_pointer_type_attribute_is_none(self):
        class Cls(Structure):
            _fields_ = (
                ('a', c_int),
                ('b', c_float),
            )

        with self.assertRaisesRegex(AttributeError, ".Cls'> has no attribute '__pointer_type__'"):
            Cls.__pointer_type__

        p = POINTER(Cls)
        self.assertIs(Cls.__pointer_type__, p)

    def test_arbitrary_pointer_type_attribute(self):
        class Cls(Structure):
            _fields_ = (
                ('a', c_int),
                ('b', c_float),
            )

        garbage = 'garbage'

        P = POINTER(Cls)
        self.assertIs(Cls.__pointer_type__, P)
        Cls.__pointer_type__ = garbage
        self.assertIs(Cls.__pointer_type__, garbage)
        self.assertIs(POINTER(Cls), garbage)
        self.assertIs(P._type_, Cls)

        instance = Cls(1, 2.0)
        pointer = P(instance)
        self.assertEqual(pointer[0].a, 1)
        self.assertEqual(pointer[0].b, 2)

        del Cls.__pointer_type__

        NewP = POINTER(Cls)
        self.assertIsNot(NewP, P)
        self.assertIs(Cls.__pointer_type__, NewP)
        self.assertIs(P._type_, Cls)

    def test_pointer_types_factory(self):
        """Shouldn't leak"""
        def factory():
            class Cls(Structure):
                _fields_ = (
                    ('a', c_int),
                    ('b', c_float),
                )

            return Cls

        ws_typ = WeakSet()
        ws_ptr = WeakSet()
        for _ in range(10):
            typ = factory()
            ptr = POINTER(typ)

            ws_typ.add(typ)
            ws_ptr.add(ptr)

        typ = None
        ptr = None

        gc.collect()

        self.assertEqual(len(ws_typ), 0, ws_typ)
        self.assertEqual(len(ws_ptr), 0, ws_ptr)


class PointerTypeCacheTestCase(unittest.TestCase):
    # dummy tests to check warnings and base behavior
    def tearDown(self):
        _pointer_type_cache_fallback.clear()

    def test_deprecated_cache_with_not_ctypes_type(self):
        class C:
            pass

        with self.assertWarns(DeprecationWarning):
            P = POINTER("C")

        with self.assertWarns(DeprecationWarning):
            self.assertIs(_pointer_type_cache["C"], P)

        with self.assertWarns(DeprecationWarning):
            _pointer_type_cache[C] = P
        self.assertIs(C.__pointer_type__, P)
        with self.assertWarns(DeprecationWarning):
            self.assertIs(_pointer_type_cache[C], P)

    def test_deprecated_cache_with_ints(self):
        with self.assertWarns(DeprecationWarning):
            _pointer_type_cache[123] = 456

        with self.assertWarns(DeprecationWarning):
            self.assertEqual(_pointer_type_cache[123], 456)

    def test_deprecated_cache_with_ctypes_type(self):
        class C(Structure):
            _fields_ = [("a", c_int),
                        ("b", c_int),
                        ("c", c_int)]

        P1 = POINTER(C)
        with self.assertWarns(DeprecationWarning):
            P2 = POINTER("C")

        with self.assertWarns(DeprecationWarning):
            _pointer_type_cache[C] = P2

        self.assertIs(C.__pointer_type__, P2)
        self.assertIsNot(C.__pointer_type__, P1)

        with self.assertWarns(DeprecationWarning):
            self.assertIs(_pointer_type_cache[C], P2)

        with self.assertWarns(DeprecationWarning):
            self.assertIs(_pointer_type_cache.get(C), P2)

    def test_get_not_registered(self):
        with self.assertWarns(DeprecationWarning):
            self.assertIsNone(_pointer_type_cache.get(str))

        with self.assertWarns(DeprecationWarning):
            self.assertIsNone(_pointer_type_cache.get(str, None))

    def test_repeated_set_type(self):
        # Regression test for gh-133290
        class C(Structure):
            _fields_ = [('a', c_int)]
        ptr = POINTER(C)
        # Read _type_ several times to warm up cache
        for i in range(5):
            self.assertIs(ptr._type_, C)
        ptr.set_type(c_int)
        self.assertIs(ptr._type_, c_int)


if __name__ == '__main__':
    unittest.main()
