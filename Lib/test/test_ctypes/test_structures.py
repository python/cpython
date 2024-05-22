import _ctypes_test
from platform import architecture as _architecture
import struct
import sys
import unittest
from ctypes import (CDLL, Array, Structure, Union, POINTER, sizeof, byref, alignment,
                    c_void_p, c_char, c_wchar, c_byte, c_ubyte,
                    c_uint8, c_uint16, c_uint32,
                    c_short, c_ushort, c_int, c_uint,
                    c_long, c_ulong, c_longlong, c_ulonglong, c_float, c_double)
from ctypes.util import find_library
from struct import calcsize
from collections import namedtuple
from test import support
from ._support import (_CData, PyCStructType, Py_TPFLAGS_DISALLOW_INSTANTIATION,
                       Py_TPFLAGS_IMMUTABLETYPE)


class SubclassesTest(unittest.TestCase):
    def test_subclass(self):
        class X(Structure):
            _fields_ = [("a", c_int)]

        class Y(X):
            _fields_ = [("b", c_int)]

        class Z(X):
            pass

        self.assertEqual(sizeof(X), sizeof(c_int))
        self.assertEqual(sizeof(Y), sizeof(c_int)*2)
        self.assertEqual(sizeof(Z), sizeof(c_int))
        self.assertEqual(X._fields_, [("a", c_int)])
        self.assertEqual(Y._fields_, [("b", c_int)])
        self.assertEqual(Z._fields_, [("a", c_int)])

    def test_subclass_delayed(self):
        class X(Structure):
            pass
        self.assertEqual(sizeof(X), 0)
        X._fields_ = [("a", c_int)]

        class Y(X):
            pass
        self.assertEqual(sizeof(Y), sizeof(X))
        Y._fields_ = [("b", c_int)]

        class Z(X):
            pass

        self.assertEqual(sizeof(X), sizeof(c_int))
        self.assertEqual(sizeof(Y), sizeof(c_int)*2)
        self.assertEqual(sizeof(Z), sizeof(c_int))
        self.assertEqual(X._fields_, [("a", c_int)])
        self.assertEqual(Y._fields_, [("b", c_int)])
        self.assertEqual(Z._fields_, [("a", c_int)])


class StructureTestCase(unittest.TestCase):
    formats = {"c": c_char,
               "b": c_byte,
               "B": c_ubyte,
               "h": c_short,
               "H": c_ushort,
               "i": c_int,
               "I": c_uint,
               "l": c_long,
               "L": c_ulong,
               "q": c_longlong,
               "Q": c_ulonglong,
               "f": c_float,
               "d": c_double,
               }

    def test_inheritance_hierarchy(self):
        self.assertEqual(Structure.mro(), [Structure, _CData, object])

        self.assertEqual(PyCStructType.__name__, "PyCStructType")
        self.assertEqual(type(PyCStructType), type)


    def test_type_flags(self):
        for cls in Structure, PyCStructType:
            with self.subTest(cls=cls):
                self.assertTrue(Structure.__flags__ & Py_TPFLAGS_IMMUTABLETYPE)
                self.assertFalse(Structure.__flags__ & Py_TPFLAGS_DISALLOW_INSTANTIATION)

    def test_simple_structs(self):
        for code, tp in self.formats.items():
            class X(Structure):
                _fields_ = [("x", c_char),
                            ("y", tp)]
            self.assertEqual((sizeof(X), code),
                                 (calcsize("c%c0%c" % (code, code)), code))

    def test_unions(self):
        for code, tp in self.formats.items():
            class X(Union):
                _fields_ = [("x", c_char),
                            ("y", tp)]
            self.assertEqual((sizeof(X), code),
                                 (calcsize("%c" % (code)), code))

    def test_struct_alignment(self):
        class X(Structure):
            _fields_ = [("x", c_char * 3)]
        self.assertEqual(alignment(X), calcsize("s"))
        self.assertEqual(sizeof(X), calcsize("3s"))

        class Y(Structure):
            _fields_ = [("x", c_char * 3),
                        ("y", c_int)]
        self.assertEqual(alignment(Y), alignment(c_int))
        self.assertEqual(sizeof(Y), calcsize("3si"))

        class SI(Structure):
            _fields_ = [("a", X),
                        ("b", Y)]
        self.assertEqual(alignment(SI), max(alignment(Y), alignment(X)))
        self.assertEqual(sizeof(SI), calcsize("3s0i 3si 0i"))

        class IS(Structure):
            _fields_ = [("b", Y),
                        ("a", X)]

        self.assertEqual(alignment(SI), max(alignment(X), alignment(Y)))
        self.assertEqual(sizeof(IS), calcsize("3si 3s 0i"))

        class XX(Structure):
            _fields_ = [("a", X),
                        ("b", X)]
        self.assertEqual(alignment(XX), alignment(X))
        self.assertEqual(sizeof(XX), calcsize("3s 3s 0s"))

    def test_empty(self):
        # I had problems with these
        #
        # Although these are pathological cases: Empty Structures!
        class X(Structure):
            _fields_ = []

        class Y(Union):
            _fields_ = []

        # Is this really the correct alignment, or should it be 0?
        self.assertTrue(alignment(X) == alignment(Y) == 1)
        self.assertTrue(sizeof(X) == sizeof(Y) == 0)

        class XX(Structure):
            _fields_ = [("a", X),
                        ("b", X)]

        self.assertEqual(alignment(XX), 1)
        self.assertEqual(sizeof(XX), 0)

    def test_fields(self):
        # test the offset and size attributes of Structure/Union fields.
        class X(Structure):
            _fields_ = [("x", c_int),
                        ("y", c_char)]

        self.assertEqual(X.x.offset, 0)
        self.assertEqual(X.x.size, sizeof(c_int))

        self.assertEqual(X.y.offset, sizeof(c_int))
        self.assertEqual(X.y.size, sizeof(c_char))

        # readonly
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "offset", 92)
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "size", 92)

        class X(Union):
            _fields_ = [("x", c_int),
                        ("y", c_char)]

        self.assertEqual(X.x.offset, 0)
        self.assertEqual(X.x.size, sizeof(c_int))

        self.assertEqual(X.y.offset, 0)
        self.assertEqual(X.y.size, sizeof(c_char))

        # readonly
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "offset", 92)
        self.assertRaises((TypeError, AttributeError), setattr, X.x, "size", 92)

        # XXX Should we check nested data types also?
        # offset is always relative to the class...

    def test_packed(self):
        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 1

        self.assertEqual(sizeof(X), 9)
        self.assertEqual(X.b.offset, 1)

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 2
        self.assertEqual(sizeof(X), 10)
        self.assertEqual(X.b.offset, 2)

        longlong_size = struct.calcsize("q")
        longlong_align = struct.calcsize("bq") - longlong_size

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 4
        self.assertEqual(sizeof(X), min(4, longlong_align) + longlong_size)
        self.assertEqual(X.b.offset, min(4, longlong_align))

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 8

        self.assertEqual(sizeof(X), min(8, longlong_align) + longlong_size)
        self.assertEqual(X.b.offset, min(8, longlong_align))


        d = {"_fields_": [("a", "b"),
                          ("b", "q")],
             "_pack_": -1}
        self.assertRaises(ValueError, type(Structure), "X", (Structure,), d)

    @support.cpython_only
    def test_packed_c_limits(self):
        # Issue 15989
        import _testcapi
        d = {"_fields_": [("a", c_byte)],
             "_pack_": _testcapi.INT_MAX + 1}
        self.assertRaises(ValueError, type(Structure), "X", (Structure,), d)
        d = {"_fields_": [("a", c_byte)],
             "_pack_": _testcapi.UINT_MAX + 2}
        self.assertRaises(ValueError, type(Structure), "X", (Structure,), d)

    def test_initializers(self):
        class Person(Structure):
            _fields_ = [("name", c_char*6),
                        ("age", c_int)]

        self.assertRaises(TypeError, Person, 42)
        self.assertRaises(ValueError, Person, b"asldkjaslkdjaslkdj")
        self.assertRaises(TypeError, Person, "Name", "HI")

        # short enough
        self.assertEqual(Person(b"12345", 5).name, b"12345")
        # exact fit
        self.assertEqual(Person(b"123456", 5).name, b"123456")
        # too long
        self.assertRaises(ValueError, Person, b"1234567", 5)

    def test_conflicting_initializers(self):
        class POINT(Structure):
            _fields_ = [("phi", c_float), ("rho", c_float)]
        # conflicting positional and keyword args
        self.assertRaisesRegex(TypeError, "phi", POINT, 2, 3, phi=4)
        self.assertRaisesRegex(TypeError, "rho", POINT, 2, 3, rho=4)

        # too many initializers
        self.assertRaises(TypeError, POINT, 2, 3, 4)

    def test_keyword_initializers(self):
        class POINT(Structure):
            _fields_ = [("x", c_int), ("y", c_int)]
        pt = POINT(1, 2)
        self.assertEqual((pt.x, pt.y), (1, 2))

        pt = POINT(y=2, x=1)
        self.assertEqual((pt.x, pt.y), (1, 2))

    def test_invalid_field_types(self):
        class POINT(Structure):
            pass
        self.assertRaises(TypeError, setattr, POINT, "_fields_", [("x", 1), ("y", 2)])

    def test_invalid_name(self):
        # field name must be string
        def declare_with_name(name):
            class S(Structure):
                _fields_ = [(name, c_int)]

        self.assertRaises(TypeError, declare_with_name, b"x")

    def test_intarray_fields(self):
        class SomeInts(Structure):
            _fields_ = [("a", c_int * 4)]

        # can use tuple to initialize array (but not list!)
        self.assertEqual(SomeInts((1, 2)).a[:], [1, 2, 0, 0])
        self.assertEqual(SomeInts((1, 2)).a[::], [1, 2, 0, 0])
        self.assertEqual(SomeInts((1, 2)).a[::-1], [0, 0, 2, 1])
        self.assertEqual(SomeInts((1, 2)).a[::2], [1, 0])
        self.assertEqual(SomeInts((1, 2)).a[1:5:6], [2])
        self.assertEqual(SomeInts((1, 2)).a[6:4:-1], [])
        self.assertEqual(SomeInts((1, 2, 3, 4)).a[:], [1, 2, 3, 4])
        self.assertEqual(SomeInts((1, 2, 3, 4)).a[::], [1, 2, 3, 4])
        # too long
        # XXX Should raise ValueError?, not RuntimeError
        self.assertRaises(RuntimeError, SomeInts, (1, 2, 3, 4, 5))

    def test_nested_initializers(self):
        # test initializing nested structures
        class Phone(Structure):
            _fields_ = [("areacode", c_char*6),
                        ("number", c_char*12)]

        class Person(Structure):
            _fields_ = [("name", c_char * 12),
                        ("phone", Phone),
                        ("age", c_int)]

        p = Person(b"Someone", (b"1234", b"5678"), 5)

        self.assertEqual(p.name, b"Someone")
        self.assertEqual(p.phone.areacode, b"1234")
        self.assertEqual(p.phone.number, b"5678")
        self.assertEqual(p.age, 5)

    def test_structures_with_wchar(self):
        class PersonW(Structure):
            _fields_ = [("name", c_wchar * 12),
                        ("age", c_int)]

        p = PersonW("Someone \xe9")
        self.assertEqual(p.name, "Someone \xe9")

        self.assertEqual(PersonW("1234567890").name, "1234567890")
        self.assertEqual(PersonW("12345678901").name, "12345678901")
        # exact fit
        self.assertEqual(PersonW("123456789012").name, "123456789012")
        #too long
        self.assertRaises(ValueError, PersonW, "1234567890123")

    def test_init_errors(self):
        class Phone(Structure):
            _fields_ = [("areacode", c_char*6),
                        ("number", c_char*12)]

        class Person(Structure):
            _fields_ = [("name", c_char * 12),
                        ("phone", Phone),
                        ("age", c_int)]

        cls, msg = self.get_except(Person, b"Someone", (1, 2))
        self.assertEqual(cls, RuntimeError)
        self.assertEqual(msg,
                             "(Phone) TypeError: "
                             "expected bytes, int found")

        cls, msg = self.get_except(Person, b"Someone", (b"a", b"b", b"c"))
        self.assertEqual(cls, RuntimeError)
        self.assertEqual(msg,
                             "(Phone) TypeError: too many initializers")

    def test_huge_field_name(self):
        # issue12881: segfault with large structure field names
        def create_class(length):
            class S(Structure):
                _fields_ = [('x' * length, c_int)]

        for length in [10 ** i for i in range(0, 8)]:
            try:
                create_class(length)
            except MemoryError:
                # MemoryErrors are OK, we just don't want to segfault
                pass

    def get_except(self, func, *args):
        try:
            func(*args)
        except Exception as detail:
            return detail.__class__, str(detail)

    def test_abstract_class(self):
        class X(Structure):
            _abstract_ = "something"
        # try 'X()'
        cls, msg = self.get_except(eval, "X()", locals())
        self.assertEqual((cls, msg), (TypeError, "abstract class"))

    def test_methods(self):
        self.assertIn("in_dll", dir(type(Structure)))
        self.assertIn("from_address", dir(type(Structure)))
        self.assertIn("in_dll", dir(type(Structure)))

    def test_positional_args(self):
        # see also http://bugs.python.org/issue5042
        class W(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]
        class X(W):
            _fields_ = [("c", c_int)]
        class Y(X):
            pass
        class Z(Y):
            _fields_ = [("d", c_int), ("e", c_int), ("f", c_int)]

        z = Z(1, 2, 3, 4, 5, 6)
        self.assertEqual((z.a, z.b, z.c, z.d, z.e, z.f),
                         (1, 2, 3, 4, 5, 6))
        z = Z(1)
        self.assertEqual((z.a, z.b, z.c, z.d, z.e, z.f),
                         (1, 0, 0, 0, 0, 0))
        self.assertRaises(TypeError, lambda: Z(1, 2, 3, 4, 5, 6, 7))

    def test_pass_by_value(self):
        # This should mirror the Test structure
        # in Modules/_ctypes/_ctypes_test.c
        class Test(Structure):
            _fields_ = [
                ('first', c_ulong),
                ('second', c_ulong),
                ('third', c_ulong),
            ]

        s = Test()
        s.first = 0xdeadbeef
        s.second = 0xcafebabe
        s.third = 0x0bad1dea
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_large_struct_update_value
        func.argtypes = (Test,)
        func.restype = None
        func(s)
        self.assertEqual(s.first, 0xdeadbeef)
        self.assertEqual(s.second, 0xcafebabe)
        self.assertEqual(s.third, 0x0bad1dea)

    def test_pass_by_value_finalizer(self):
        # bpo-37140: Similar to test_pass_by_value(), but the Python structure
        # has a finalizer (__del__() method): the finalizer must only be called
        # once.

        finalizer_calls = []

        class Test(Structure):
            _fields_ = [
                ('first', c_ulong),
                ('second', c_ulong),
                ('third', c_ulong),
            ]
            def __del__(self):
                finalizer_calls.append("called")

        s = Test(1, 2, 3)
        # Test the StructUnionType_paramfunc() code path which copies the
        # structure: if the structure is larger than sizeof(void*).
        self.assertGreater(sizeof(s), sizeof(c_void_p))

        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_large_struct_update_value
        func.argtypes = (Test,)
        func.restype = None
        func(s)
        # bpo-37140: Passing the structure by reference must not call
        # its finalizer!
        self.assertEqual(finalizer_calls, [])
        self.assertEqual(s.first, 1)
        self.assertEqual(s.second, 2)
        self.assertEqual(s.third, 3)

        # The finalizer must be called exactly once
        s = None
        support.gc_collect()
        self.assertEqual(finalizer_calls, ["called"])

    def test_pass_by_value_in_register(self):
        class X(Structure):
            _fields_ = [
                ('first', c_uint),
                ('second', c_uint)
            ]

        s = X()
        s.first = 0xdeadbeef
        s.second = 0xcafebabe
        dll = CDLL(_ctypes_test.__file__)
        func = dll._testfunc_reg_struct_update_value
        func.argtypes = (X,)
        func.restype = None
        func(s)
        self.assertEqual(s.first, 0xdeadbeef)
        self.assertEqual(s.second, 0xcafebabe)
        got = X.in_dll(dll, "last_tfrsuv_arg")
        self.assertEqual(s.first, got.first)
        self.assertEqual(s.second, got.second)

    def _test_issue18060(self, Vector):
        # The call to atan2() should succeed if the
        # class fields were correctly cloned in the
        # subclasses. Otherwise, it will segfault.
        if sys.platform == 'win32':
            libm = CDLL(find_library('msvcrt.dll'))
        else:
            libm = CDLL(find_library('m'))

        libm.atan2.argtypes = [Vector]
        libm.atan2.restype = c_double

        arg = Vector(y=0.0, x=-1.0)
        self.assertAlmostEqual(libm.atan2(arg), 3.141592653589793)

    @unittest.skipIf(_architecture() == ('64bit', 'WindowsPE'), "can't test Windows x64 build")
    @unittest.skipUnless(sys.byteorder == 'little', "can't test on this platform")
    def test_issue18060_a(self):
        # This test case calls
        # PyCStructUnionType_update_stgdict() for each
        # _fields_ assignment, and PyCStgDict_clone()
        # for the Mid and Vector class definitions.
        class Base(Structure):
            _fields_ = [('y', c_double),
                        ('x', c_double)]
        class Mid(Base):
            pass
        Mid._fields_ = []
        class Vector(Mid): pass
        self._test_issue18060(Vector)

    @unittest.skipIf(_architecture() == ('64bit', 'WindowsPE'), "can't test Windows x64 build")
    @unittest.skipUnless(sys.byteorder == 'little', "can't test on this platform")
    def test_issue18060_b(self):
        # This test case calls
        # PyCStructUnionType_update_stgdict() for each
        # _fields_ assignment.
        class Base(Structure):
            _fields_ = [('y', c_double),
                        ('x', c_double)]
        class Mid(Base):
            _fields_ = []
        class Vector(Mid):
            _fields_ = []
        self._test_issue18060(Vector)

    @unittest.skipIf(_architecture() == ('64bit', 'WindowsPE'), "can't test Windows x64 build")
    @unittest.skipUnless(sys.byteorder == 'little', "can't test on this platform")
    def test_issue18060_c(self):
        # This test case calls
        # PyCStructUnionType_update_stgdict() for each
        # _fields_ assignment.
        class Base(Structure):
            _fields_ = [('y', c_double)]
        class Mid(Base):
            _fields_ = []
        class Vector(Mid):
            _fields_ = [('x', c_double)]
        self._test_issue18060(Vector)

    def test_array_in_struct(self):
        # See bpo-22273

        # Load the shared library
        dll = CDLL(_ctypes_test.__file__)

        # These should mirror the structures in Modules/_ctypes/_ctypes_test.c
        class Test2(Structure):
            _fields_ = [
                ('data', c_ubyte * 16),
            ]

        class Test3AParent(Structure):
            _fields_ = [
                ('data', c_float * 2),
            ]

        class Test3A(Test3AParent):
            _fields_ = [
                ('more_data', c_float * 2),
            ]

        class Test3B(Structure):
            _fields_ = [
                ('data', c_double * 2),
            ]

        class Test3C(Structure):
            _fields_ = [
                ("data", c_double * 4)
            ]

        class Test3D(Structure):
            _fields_ = [
                ("data", c_double * 8)
            ]

        class Test3E(Structure):
            _fields_ = [
                ("data", c_double * 9)
            ]


        # Tests for struct Test2
        s = Test2()
        expected = 0
        for i in range(16):
            s.data[i] = i
            expected += i
        func = dll._testfunc_array_in_struct2
        func.restype = c_int
        func.argtypes = (Test2,)
        result = func(s)
        self.assertEqual(result, expected)
        # check the passed-in struct hasn't changed
        for i in range(16):
            self.assertEqual(s.data[i], i)

        # Tests for struct Test3A
        s = Test3A()
        s.data[0] = 3.14159
        s.data[1] = 2.71828
        s.more_data[0] = -3.0
        s.more_data[1] = -2.0
        expected = 3.14159 + 2.71828 - 3.0 - 2.0
        func = dll._testfunc_array_in_struct3A
        func.restype = c_double
        func.argtypes = (Test3A,)
        result = func(s)
        self.assertAlmostEqual(result, expected, places=6)
        # check the passed-in struct hasn't changed
        self.assertAlmostEqual(s.data[0], 3.14159, places=6)
        self.assertAlmostEqual(s.data[1], 2.71828, places=6)
        self.assertAlmostEqual(s.more_data[0], -3.0, places=6)
        self.assertAlmostEqual(s.more_data[1], -2.0, places=6)

        # Test3B, Test3C, Test3D, Test3E have the same logic with different
        # sizes hence putting them in a loop.
        StructCtype = namedtuple(
            "StructCtype",
            ["cls", "cfunc1", "cfunc2", "items"]
        )
        structs_to_test = [
            StructCtype(
                Test3B,
                dll._testfunc_array_in_struct3B,
                dll._testfunc_array_in_struct3B_set_defaults,
                2),
            StructCtype(
                Test3C,
                dll._testfunc_array_in_struct3C,
                dll._testfunc_array_in_struct3C_set_defaults,
                4),
            StructCtype(
                Test3D,
                dll._testfunc_array_in_struct3D,
                dll._testfunc_array_in_struct3D_set_defaults,
                8),
            StructCtype(
                Test3E,
                dll._testfunc_array_in_struct3E,
                dll._testfunc_array_in_struct3E_set_defaults,
                9),
        ]

        for sut in structs_to_test:
            s = sut.cls()

            # Test for cfunc1
            expected = 0
            for i in range(sut.items):
                float_i = float(i)
                s.data[i] = float_i
                expected += float_i
            func = sut.cfunc1
            func.restype = c_double
            func.argtypes = (sut.cls,)
            result = func(s)
            self.assertEqual(result, expected)
            # check the passed-in struct hasn't changed
            for i in range(sut.items):
                self.assertEqual(s.data[i], float(i))

            # Test for cfunc2
            func = sut.cfunc2
            func.restype = sut.cls
            result = func()
            # check if the default values have been set correctly
            for i in range(sut.items):
                self.assertEqual(result.data[i], float(i+1))

    def test_38368(self):
        class U(Union):
            _fields_ = [
                ('f1', c_uint8 * 16),
                ('f2', c_uint16 * 8),
                ('f3', c_uint32 * 4),
            ]
        u = U()
        u.f3[0] = 0x01234567
        u.f3[1] = 0x89ABCDEF
        u.f3[2] = 0x76543210
        u.f3[3] = 0xFEDCBA98
        f1 = [u.f1[i] for i in range(16)]
        f2 = [u.f2[i] for i in range(8)]
        if sys.byteorder == 'little':
            self.assertEqual(f1, [0x67, 0x45, 0x23, 0x01,
                                  0xef, 0xcd, 0xab, 0x89,
                                  0x10, 0x32, 0x54, 0x76,
                                  0x98, 0xba, 0xdc, 0xfe])
            self.assertEqual(f2, [0x4567, 0x0123, 0xcdef, 0x89ab,
                                  0x3210, 0x7654, 0xba98, 0xfedc])

    @unittest.skipIf(True, 'Test disabled for now - see bpo-16575/bpo-16576')
    def test_union_by_value(self):
        # See bpo-16575

        # These should mirror the structures in Modules/_ctypes/_ctypes_test.c

        class Nested1(Structure):
            _fields_ = [
                ('an_int', c_int),
                ('another_int', c_int),
            ]

        class Test4(Union):
            _fields_ = [
                ('a_long', c_long),
                ('a_struct', Nested1),
            ]

        class Nested2(Structure):
            _fields_ = [
                ('an_int', c_int),
                ('a_union', Test4),
            ]

        class Test5(Structure):
            _fields_ = [
                ('an_int', c_int),
                ('nested', Nested2),
                ('another_int', c_int),
            ]

        test4 = Test4()
        dll = CDLL(_ctypes_test.__file__)
        with self.assertRaises(TypeError) as ctx:
            func = dll._testfunc_union_by_value1
            func.restype = c_long
            func.argtypes = (Test4,)
            result = func(test4)
        self.assertEqual(ctx.exception.args[0], 'item 1 in _argtypes_ passes '
                         'a union by value, which is unsupported.')
        test5 = Test5()
        with self.assertRaises(TypeError) as ctx:
            func = dll._testfunc_union_by_value2
            func.restype = c_long
            func.argtypes = (Test5,)
            result = func(test5)
        self.assertEqual(ctx.exception.args[0], 'item 1 in _argtypes_ passes '
                         'a union by value, which is unsupported.')

        # passing by reference should be OK
        test4.a_long = 12345;
        func = dll._testfunc_union_by_reference1
        func.restype = c_long
        func.argtypes = (POINTER(Test4),)
        result = func(byref(test4))
        self.assertEqual(result, 12345)
        self.assertEqual(test4.a_long, 0)
        self.assertEqual(test4.a_struct.an_int, 0)
        self.assertEqual(test4.a_struct.another_int, 0)
        test4.a_struct.an_int = 0x12340000
        test4.a_struct.another_int = 0x5678
        func = dll._testfunc_union_by_reference2
        func.restype = c_long
        func.argtypes = (POINTER(Test4),)
        result = func(byref(test4))
        self.assertEqual(result, 0x12345678)
        self.assertEqual(test4.a_long, 0)
        self.assertEqual(test4.a_struct.an_int, 0)
        self.assertEqual(test4.a_struct.another_int, 0)
        test5.an_int = 0x12000000
        test5.nested.an_int = 0x345600
        test5.another_int = 0x78
        func = dll._testfunc_union_by_reference3
        func.restype = c_long
        func.argtypes = (POINTER(Test5),)
        result = func(byref(test5))
        self.assertEqual(result, 0x12345678)
        self.assertEqual(test5.an_int, 0)
        self.assertEqual(test5.nested.an_int, 0)
        self.assertEqual(test5.another_int, 0)

    @unittest.skipIf(True, 'Test disabled for now - see bpo-16575/bpo-16576')
    def test_bitfield_by_value(self):
        # See bpo-16576

        # These should mirror the structures in Modules/_ctypes/_ctypes_test.c

        class Test6(Structure):
            _fields_ = [
                ('A', c_int, 1),
                ('B', c_int, 2),
                ('C', c_int, 3),
                ('D', c_int, 2),
            ]

        test6 = Test6()
        # As these are signed int fields, all are logically -1 due to sign
        # extension.
        test6.A = 1
        test6.B = 3
        test6.C = 7
        test6.D = 3
        dll = CDLL(_ctypes_test.__file__)
        with self.assertRaises(TypeError) as ctx:
            func = dll._testfunc_bitfield_by_value1
            func.restype = c_long
            func.argtypes = (Test6,)
            result = func(test6)
        self.assertEqual(ctx.exception.args[0], 'item 1 in _argtypes_ passes '
                         'a struct/union with a bitfield by value, which is '
                         'unsupported.')
        # passing by reference should be OK
        func = dll._testfunc_bitfield_by_reference1
        func.restype = c_long
        func.argtypes = (POINTER(Test6),)
        result = func(byref(test6))
        self.assertEqual(result, -4)
        self.assertEqual(test6.A, 0)
        self.assertEqual(test6.B, 0)
        self.assertEqual(test6.C, 0)
        self.assertEqual(test6.D, 0)

        class Test7(Structure):
            _fields_ = [
                ('A', c_uint, 1),
                ('B', c_uint, 2),
                ('C', c_uint, 3),
                ('D', c_uint, 2),
            ]
        test7 = Test7()
        test7.A = 1
        test7.B = 3
        test7.C = 7
        test7.D = 3
        func = dll._testfunc_bitfield_by_reference2
        func.restype = c_long
        func.argtypes = (POINTER(Test7),)
        result = func(byref(test7))
        self.assertEqual(result, 14)
        self.assertEqual(test7.A, 0)
        self.assertEqual(test7.B, 0)
        self.assertEqual(test7.C, 0)
        self.assertEqual(test7.D, 0)

        # for a union with bitfields, the union check happens first
        class Test8(Union):
            _fields_ = [
                ('A', c_int, 1),
                ('B', c_int, 2),
                ('C', c_int, 3),
                ('D', c_int, 2),
            ]

        test8 = Test8()
        with self.assertRaises(TypeError) as ctx:
            func = dll._testfunc_bitfield_by_value2
            func.restype = c_long
            func.argtypes = (Test8,)
            result = func(test8)
        self.assertEqual(ctx.exception.args[0], 'item 1 in _argtypes_ passes '
                         'a union by value, which is unsupported.')


class PointerMemberTestCase(unittest.TestCase):

    def test(self):
        # a Structure with a POINTER field
        class S(Structure):
            _fields_ = [("array", POINTER(c_int))]

        s = S()
        # We can assign arrays of the correct type
        s.array = (c_int * 3)(1, 2, 3)
        items = [s.array[i] for i in range(3)]
        self.assertEqual(items, [1, 2, 3])

        # The following are bugs, but are included here because the unittests
        # also describe the current behaviour.
        #
        # This fails with SystemError: bad arg to internal function
        # or with IndexError (with a patch I have)

        s.array[0] = 42

        items = [s.array[i] for i in range(3)]
        self.assertEqual(items, [42, 2, 3])

        s.array[0] = 1

        items = [s.array[i] for i in range(3)]
        self.assertEqual(items, [1, 2, 3])

    def test_none_to_pointer_fields(self):
        class S(Structure):
            _fields_ = [("x", c_int),
                        ("p", POINTER(c_int))]

        s = S()
        s.x = 12345678
        s.p = None
        self.assertEqual(s.x, 12345678)


class TestRecursiveStructure(unittest.TestCase):
    def test_contains_itself(self):
        class Recursive(Structure):
            pass

        try:
            Recursive._fields_ = [("next", Recursive)]
        except AttributeError as details:
            self.assertIn("Structure or union cannot contain itself",
                          str(details))
        else:
            self.fail("Structure or union cannot contain itself")


    def test_vice_versa(self):
        class First(Structure):
            pass
        class Second(Structure):
            pass

        First._fields_ = [("second", Second)]

        try:
            Second._fields_ = [("first", First)]
        except AttributeError as details:
            self.assertIn("_fields_ is final", str(details))
        else:
            self.fail("AttributeError not raised")


if __name__ == '__main__':
    unittest.main()
