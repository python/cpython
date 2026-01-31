"""Tests for ctypes.Structure

Features common with Union should go in test_structunion.py instead.
"""

from platform import architecture as _architecture
import struct
import sys
import unittest
from ctypes import (CDLL, Structure, Union, POINTER, sizeof, byref,
                    c_void_p, c_char, c_wchar, c_byte, c_ubyte,
                    c_uint8, c_uint16, c_uint32, c_int, c_uint,
                    c_long, c_ulong, c_longlong, c_float, c_double)
from ctypes.util import find_library
from collections import namedtuple
from test import support
from test.support import import_helper
from ._support import StructCheckMixin
_ctypes_test = import_helper.import_module("_ctypes_test")


class StructureTestCase(unittest.TestCase, StructCheckMixin):
    def test_packed(self):
        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 1
            _layout_ = 'ms'
        self.check_struct(X)

        self.assertEqual(sizeof(X), 9)
        self.assertEqual(X.b.offset, 1)

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 2
            _layout_ = 'ms'
        self.check_struct(X)
        self.assertEqual(sizeof(X), 10)
        self.assertEqual(X.b.offset, 2)

        longlong_size = struct.calcsize("q")
        longlong_align = struct.calcsize("bq") - longlong_size

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 4
            _layout_ = 'ms'
        self.check_struct(X)
        self.assertEqual(sizeof(X), min(4, longlong_align) + longlong_size)
        self.assertEqual(X.b.offset, min(4, longlong_align))

        class X(Structure):
            _fields_ = [("a", c_byte),
                        ("b", c_longlong)]
            _pack_ = 8
            _layout_ = 'ms'
        self.check_struct(X)

        self.assertEqual(sizeof(X), min(8, longlong_align) + longlong_size)
        self.assertEqual(X.b.offset, min(8, longlong_align))

        with self.assertRaises(ValueError):
            class X(Structure):
                _fields_ = [("a", "b"), ("b", "q")]
                _pack_ = -1
                _layout_ = "ms"

    @support.cpython_only
    def test_packed_c_limits(self):
        # Issue 15989
        import _testcapi
        with self.assertRaises(ValueError):
            class X(Structure):
                _fields_ = [("a", c_byte)]
                _pack_ = _testcapi.INT_MAX + 1
                _layout_ = "ms"

        with self.assertRaises(ValueError):
            class X(Structure):
                _fields_ = [("a", c_byte)]
                _pack_ = _testcapi.UINT_MAX + 2
                _layout_ = "ms"

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
        self.check_struct(POINT)
        # conflicting positional and keyword args
        self.assertRaisesRegex(TypeError, "phi", POINT, 2, 3, phi=4)
        self.assertRaisesRegex(TypeError, "rho", POINT, 2, 3, rho=4)

        # too many initializers
        self.assertRaises(TypeError, POINT, 2, 3, 4)

    def test_keyword_initializers(self):
        class POINT(Structure):
            _fields_ = [("x", c_int), ("y", c_int)]
        self.check_struct(POINT)
        pt = POINT(1, 2)
        self.assertEqual((pt.x, pt.y), (1, 2))

        pt = POINT(y=2, x=1)
        self.assertEqual((pt.x, pt.y), (1, 2))

    def test_nested_initializers(self):
        # test initializing nested structures
        class Phone(Structure):
            _fields_ = [("areacode", c_char*6),
                        ("number", c_char*12)]
        self.check_struct(Phone)

        class Person(Structure):
            _fields_ = [("name", c_char * 12),
                        ("phone", Phone),
                        ("age", c_int)]
        self.check_struct(Person)

        p = Person(b"Someone", (b"1234", b"5678"), 5)

        self.assertEqual(p.name, b"Someone")
        self.assertEqual(p.phone.areacode, b"1234")
        self.assertEqual(p.phone.number, b"5678")
        self.assertEqual(p.age, 5)

    def test_structures_with_wchar(self):
        class PersonW(Structure):
            _fields_ = [("name", c_wchar * 12),
                        ("age", c_int)]
        self.check_struct(PersonW)

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
        self.check_struct(Phone)

        class Person(Structure):
            _fields_ = [("name", c_char * 12),
                        ("phone", Phone),
                        ("age", c_int)]
        self.check_struct(Person)

        cls, msg = self.get_except(Person, b"Someone", (1, 2))
        self.assertEqual(cls, RuntimeError)
        self.assertEqual(msg,
                             "(Phone) TypeError: "
                             "expected bytes, int found")

        cls, msg = self.get_except(Person, b"Someone", (b"a", b"b", b"c"))
        self.assertEqual(cls, RuntimeError)
        self.assertEqual(msg,
                             "(Phone) TypeError: too many initializers")

    def get_except(self, func, *args):
        try:
            func(*args)
        except Exception as detail:
            return detail.__class__, str(detail)

    def test_positional_args(self):
        # see also http://bugs.python.org/issue5042
        class W(Structure):
            _fields_ = [("a", c_int), ("b", c_int)]
        self.check_struct(W)

        class X(W):
            _fields_ = [("c", c_int)]
        self.check_struct(X)

        class Y(X):
            pass
        self.check_struct(Y)

        class Z(Y):
            _fields_ = [("d", c_int), ("e", c_int), ("f", c_int)]
        self.check_struct(Z)

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
        self.check_struct(Test)

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
        self.check_struct(Test)

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
        self.check_struct(X)

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
        dll.get_last_tfrsuv_arg.argtypes = ()
        dll.get_last_tfrsuv_arg.restype = X
        got = dll.get_last_tfrsuv_arg()
        self.assertEqual(s.first, got.first)
        self.assertEqual(s.second, got.second)

    def _test_issue18060(self, Vector):
        # Regression tests for gh-62260

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
        # PyCStructUnionType_update_stginfo() for each
        # _fields_ assignment, and PyCStgInfo_clone()
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
        # PyCStructUnionType_update_stginfo() for each
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
        # PyCStructUnionType_update_stginfo() for each
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
        self.check_struct(Test2)

        class Test3AParent(Structure):
            _fields_ = [
                ('data', c_float * 2),
            ]
        self.check_struct(Test3AParent)

        class Test3A(Test3AParent):
            _fields_ = [
                ('more_data', c_float * 2),
            ]
        self.check_struct(Test3A)

        class Test3B(Structure):
            _fields_ = [
                ('data', c_double * 2),
            ]
        self.check_struct(Test3B)

        class Test3C(Structure):
            _fields_ = [
                ("data", c_double * 4)
            ]
        self.check_struct(Test3C)

        class Test3D(Structure):
            _fields_ = [
                ("data", c_double * 8)
            ]
        self.check_struct(Test3D)

        class Test3E(Structure):
            _fields_ = [
                ("data", c_double * 9)
            ]
        self.check_struct(Test3E)


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
        # Regression test for gh-82549
        class U(Union):
            _fields_ = [
                ('f1', c_uint8 * 16),
                ('f2', c_uint16 * 8),
                ('f3', c_uint32 * 4),
            ]
        self.check_union(U)

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

    @unittest.skipIf(True, 'Test disabled for now - see gh-60779/gh-60780')
    def test_union_by_value(self):
        # See gh-60779

        # These should mirror the structures in Modules/_ctypes/_ctypes_test.c

        class Nested1(Structure):
            _fields_ = [
                ('an_int', c_int),
                ('another_int', c_int),
            ]
        self.check_struct(Nested1)

        class Test4(Union):
            _fields_ = [
                ('a_long', c_long),
                ('a_struct', Nested1),
            ]
        self.check_struct(Test4)

        class Nested2(Structure):
            _fields_ = [
                ('an_int', c_int),
                ('a_union', Test4),
            ]
        self.check_struct(Nested2)

        class Test5(Structure):
            _fields_ = [
                ('an_int', c_int),
                ('nested', Nested2),
                ('another_int', c_int),
            ]
        self.check_struct(Test5)

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

    @unittest.skipIf(True, 'Test disabled for now - see gh-60779/gh-60780')
    def test_bitfield_by_value(self):
        # See gh-60780

        # These should mirror the structures in Modules/_ctypes/_ctypes_test.c

        class Test6(Structure):
            _fields_ = [
                ('A', c_int, 1),
                ('B', c_int, 2),
                ('C', c_int, 3),
                ('D', c_int, 2),
            ]
        self.check_struct(Test6)

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
        self.check_struct(Test7)

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
        self.check_union(Test8)

        test8 = Test8()
        with self.assertRaises(TypeError) as ctx:
            func = dll._testfunc_bitfield_by_value2
            func.restype = c_long
            func.argtypes = (Test8,)
            result = func(test8)
        self.assertEqual(ctx.exception.args[0], 'item 1 in _argtypes_ passes '
                         'a union by value, which is unsupported.')

    def test_do_not_share_pointer_type_cache_via_stginfo_clone(self):
        # This test case calls PyCStgInfo_clone()
        # for the Mid and Vector class definitions
        # and checks that pointer_type cache not shared
        # between subclasses.
        class Base(Structure):
            _fields_ = [('y', c_double),
                        ('x', c_double)]
        base_ptr = POINTER(Base)

        class Mid(Base):
            pass
        Mid._fields_ = []
        mid_ptr = POINTER(Mid)

        class Vector(Mid):
            pass

        vector_ptr = POINTER(Vector)

        self.assertIsNot(base_ptr, mid_ptr)
        self.assertIsNot(base_ptr, vector_ptr)
        self.assertIsNot(mid_ptr, vector_ptr)


if __name__ == '__main__':
    unittest.main()
