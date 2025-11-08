import os
import sys
import unittest
from ctypes import (CDLL, Structure, sizeof, POINTER, byref, alignment,
                    LittleEndianStructure, BigEndianStructure,
                    c_byte, c_ubyte, c_char, c_char_p, c_void_p, c_wchar,
                    c_uint8, c_uint16, c_uint32, c_uint64,
                    c_short, c_ushort, c_int, c_uint, c_long, c_ulong,
                    c_longlong, c_ulonglong,
                    Union)
from test import support
from test.support import import_helper
from ._support import StructCheckMixin
_ctypes_test = import_helper.import_module("_ctypes_test")


TEST_FIELDS = (
    ("A", c_int, 1),
    ("B", c_int, 2),
    ("C", c_int, 3),
    ("D", c_int, 4),
    ("E", c_int, 5),
    ("F", c_int, 6),
    ("G", c_int, 7),
    ("H", c_int, 8),
    ("I", c_int, 9),

    ("M", c_short, 1),
    ("N", c_short, 2),
    ("O", c_short, 3),
    ("P", c_short, 4),
    ("Q", c_short, 5),
    ("R", c_short, 6),
    ("S", c_short, 7),
)


class BITS(Structure):
    _fields_ = TEST_FIELDS

func = CDLL(_ctypes_test.__file__).unpack_bitfields
func.argtypes = POINTER(BITS), c_char


class BITS_msvc(Structure):
    _layout_ = "ms"
    _fields_ = TEST_FIELDS


class BITS_gcc(Structure):
    _layout_ = "gcc-sysv"
    _fields_ = TEST_FIELDS


try:
    func_msvc = CDLL(_ctypes_test.__file__).unpack_bitfields_msvc
except AttributeError as err:
    # The MSVC struct must be available on Windows; it's optional elsewhere
    if support.MS_WINDOWS:
        raise err
    func_msvc = None
else:
    func_msvc.argtypes = POINTER(BITS_msvc), c_char


class C_Test(unittest.TestCase):

    def test_ints(self):
        for i in range(512):
            for name in "ABCDEFGHI":
                with self.subTest(i=i, name=name):
                    b = BITS()
                    setattr(b, name, i)
                    self.assertEqual(
                        getattr(b, name),
                        func(byref(b), (name.encode('ascii'))))

    def test_shorts(self):
        b = BITS()
        name = "M"
        # See Modules/_ctypes/_ctypes_test.c for where the magic 999 comes from.
        if func(byref(b), name.encode('ascii')) == 999:
            # unpack_bitfields and unpack_bitfields_msvc in
            # Modules/_ctypes/_ctypes_test.c return 999 to indicate
            # an invalid name. 'M' is only valid, if signed short bitfields
            # are supported by the C compiler.
            self.skipTest("Compiler does not support signed short bitfields")
        for i in range(256):
            for name in "MNOPQRS":
                with self.subTest(i=i, name=name):
                    b = BITS()
                    setattr(b, name, i)
                    self.assertEqual(
                        getattr(b, name),
                        func(byref(b), (name.encode('ascii'))))

    @unittest.skipUnless(func_msvc, "need MSVC or __attribute__((ms_struct))")
    def test_shorts_msvc_mode(self):
        b = BITS_msvc()
        name = "M"
        # See Modules/_ctypes/_ctypes_test.c for where the magic 999 comes from.
        if func_msvc(byref(b), name.encode('ascii')) == 999:
            # unpack_bitfields and unpack_bitfields_msvc in
            # Modules/_ctypes/_ctypes_test.c return 999 to indicate
            # an invalid name. 'M' is only valid, if signed short bitfields
            # are supported by the C compiler.
            self.skipTest("Compiler does not support signed short bitfields")
        for i in range(256):
            for name in "MNOPQRS":
                with self.subTest(i=i, name=name):
                    b = BITS_msvc()
                    setattr(b, name, i)
                    self.assertEqual(
                        getattr(b, name),
                        func_msvc(byref(b), name.encode('ascii')))


signed_int_types = (c_byte, c_short, c_int, c_long, c_longlong)
unsigned_int_types = (c_ubyte, c_ushort, c_uint, c_ulong, c_ulonglong)
int_types = unsigned_int_types + signed_int_types

class BitFieldTest(unittest.TestCase, StructCheckMixin):

    def test_generic_checks(self):
        self.check_struct(BITS)
        self.check_struct(BITS_msvc)
        self.check_struct(BITS_gcc)

    def test_longlong(self):
        class X(Structure):
            _fields_ = [("a", c_longlong, 1),
                        ("b", c_longlong, 62),
                        ("c", c_longlong, 1)]
        self.check_struct(X)

        self.assertEqual(sizeof(X), sizeof(c_longlong))
        x = X()
        x.a, x.b, x.c = -1, 7, -1
        self.assertEqual((x.a, x.b, x.c), (-1, 7, -1))

    def test_ulonglong(self):
        class X(Structure):
            _fields_ = [("a", c_ulonglong, 1),
                        ("b", c_ulonglong, 62),
                        ("c", c_ulonglong, 1)]
        self.check_struct(X)

        self.assertEqual(sizeof(X), sizeof(c_longlong))
        x = X()
        self.assertEqual((x.a, x.b, x.c), (0, 0, 0))
        x.a, x.b, x.c = 7, 7, 7
        self.assertEqual((x.a, x.b, x.c), (1, 7, 1))

    def test_signed(self):
        for c_typ in signed_int_types:
            with self.subTest(c_typ):
                if sizeof(c_typ) != alignment(c_typ):
                     self.skipTest('assumes size=alignment')
                class X(Structure):
                    _fields_ = [("dummy", c_typ),
                                ("a", c_typ, 3),
                                ("b", c_typ, 3),
                                ("c", c_typ, 1)]
                self.check_struct(X)
                self.assertEqual(sizeof(X), sizeof(c_typ)*2)

                x = X()
                self.assertEqual((c_typ, x.a, x.b, x.c), (c_typ, 0, 0, 0))
                x.a = -1
                self.assertEqual((c_typ, x.a, x.b, x.c), (c_typ, -1, 0, 0))
                x.a, x.b = 0, -1
                self.assertEqual((c_typ, x.a, x.b, x.c), (c_typ, 0, -1, 0))


    def test_unsigned(self):
        for c_typ in unsigned_int_types:
            with self.subTest(c_typ):
                if sizeof(c_typ) != alignment(c_typ):
                     self.skipTest('assumes size=alignment')
                class X(Structure):
                    _fields_ = [("a", c_typ, 3),
                                ("b", c_typ, 3),
                                ("c", c_typ, 1)]
                self.check_struct(X)
                self.assertEqual(sizeof(X), sizeof(c_typ))

                x = X()
                self.assertEqual((c_typ, x.a, x.b, x.c), (c_typ, 0, 0, 0))
                x.a = -1
                self.assertEqual((c_typ, x.a, x.b, x.c), (c_typ, 7, 0, 0))
                x.a, x.b = 0, -1
                self.assertEqual((c_typ, x.a, x.b, x.c), (c_typ, 0, 7, 0))

    def fail_fields(self, *fields):
        for layout in "ms", "gcc-sysv":
            with self.subTest(layout=layout):
                return self.get_except(type(Structure), "X", (),
                                       {"_fields_": fields, "layout": layout})

    def test_nonint_types(self):
        # bit fields are not allowed on non-integer types.
        result = self.fail_fields(("a", c_char_p, 1))
        self.assertEqual(result, (TypeError, 'bit fields not allowed for type c_char_p'))

        result = self.fail_fields(("a", c_void_p, 1))
        self.assertEqual(result, (TypeError, 'bit fields not allowed for type c_void_p'))

        if c_int != c_long:
            result = self.fail_fields(("a", POINTER(c_int), 1))
            self.assertEqual(result, (TypeError, 'bit fields not allowed for type LP_c_int'))

        result = self.fail_fields(("a", c_char, 1))
        self.assertEqual(result, (TypeError, 'bit fields not allowed for type c_char'))

        class Empty(Structure):
            _fields_ = []
        self.check_struct(Empty)

        result = self.fail_fields(("a", Empty, 1))
        self.assertEqual(result, (ValueError, "number of bits invalid for bit field 'a'"))

        class Dummy(Structure):
            _fields_ = [("x", c_int)]
        self.check_struct(Dummy)

        result = self.fail_fields(("a", Dummy, 1))
        self.assertEqual(result, (TypeError, 'bit fields not allowed for type Dummy'))

    def test_c_wchar(self):
        result = self.fail_fields(("a", c_wchar, 1))
        self.assertEqual(result,
                (TypeError, 'bit fields not allowed for type c_wchar'))

    def test_single_bitfield_size(self):
        for c_typ in int_types:
            with self.subTest(c_typ):
                if sizeof(c_typ) != alignment(c_typ):
                     self.skipTest('assumes size=alignment')
                result = self.fail_fields(("a", c_typ, -1))
                self.assertEqual(result, (ValueError,
                    "number of bits invalid for bit field 'a'"))

                result = self.fail_fields(("a", c_typ, 0))
                self.assertEqual(result, (ValueError,
                    "number of bits invalid for bit field 'a'"))

                class X(Structure):
                    _fields_ = [("a", c_typ, 1)]
                self.check_struct(X)
                self.assertEqual(sizeof(X), sizeof(c_typ))

                class X(Structure):
                    _fields_ = [("a", c_typ, sizeof(c_typ)*8)]
                self.check_struct(X)
                self.assertEqual(sizeof(X), sizeof(c_typ))

                result = self.fail_fields(("a", c_typ, sizeof(c_typ)*8 + 1))
                self.assertEqual(result, (ValueError,
                    "number of bits invalid for bit field 'a'"))

    def test_multi_bitfields_size(self):
        class X(Structure):
            _fields_ = [("a", c_short, 1),
                        ("b", c_short, 14),
                        ("c", c_short, 1)]
        self.check_struct(X)
        self.assertEqual(sizeof(X), sizeof(c_short))

        class X(Structure):
            _fields_ = [("a", c_short, 1),
                        ("a1", c_short),
                        ("b", c_short, 14),
                        ("c", c_short, 1)]
        self.check_struct(X)
        self.assertEqual(sizeof(X), sizeof(c_short)*3)
        self.assertEqual(X.a.offset, 0)
        self.assertEqual(X.a1.offset, sizeof(c_short))
        self.assertEqual(X.b.offset, sizeof(c_short)*2)
        self.assertEqual(X.c.offset, sizeof(c_short)*2)

        class X(Structure):
            _fields_ = [("a", c_short, 3),
                        ("b", c_short, 14),
                        ("c", c_short, 14)]
        self.check_struct(X)
        self.assertEqual(sizeof(X), sizeof(c_short)*3)
        self.assertEqual(X.a.offset, sizeof(c_short)*0)
        self.assertEqual(X.b.offset, sizeof(c_short)*1)
        self.assertEqual(X.c.offset, sizeof(c_short)*2)

    def get_except(self, func, *args, **kw):
        try:
            func(*args, **kw)
        except Exception as detail:
            return detail.__class__, str(detail)

    def test_mixed_1(self):
        class X(Structure):
            _fields_ = [("a", c_byte, 4),
                        ("b", c_int, 4)]
        self.check_struct(X)
        if os.name == "nt":
            self.assertEqual(sizeof(X), sizeof(c_int)*2)
        else:
            self.assertEqual(sizeof(X), sizeof(c_int))

    def test_mixed_2(self):
        class X(Structure):
            _fields_ = [("a", c_byte, 4),
                        ("b", c_int, 32)]
        self.check_struct(X)
        self.assertEqual(sizeof(X), alignment(c_int)+sizeof(c_int))

    def test_mixed_3(self):
        class X(Structure):
            _fields_ = [("a", c_byte, 4),
                        ("b", c_ubyte, 4)]
        self.check_struct(X)
        self.assertEqual(sizeof(X), sizeof(c_byte))

    def test_mixed_4(self):
        class X(Structure):
            _fields_ = [("a", c_short, 4),
                        ("b", c_short, 4),
                        ("c", c_int, 24),
                        ("d", c_short, 4),
                        ("e", c_short, 4),
                        ("f", c_int, 24)]
        self.check_struct(X)
        # MSVC does NOT combine c_short and c_int into one field, GCC
        # does (unless GCC is run with '-mms-bitfields' which
        # produces code compatible with MSVC).
        if os.name == "nt":
            self.assertEqual(sizeof(X), sizeof(c_int) * 4)
        else:
            self.assertEqual(sizeof(X), sizeof(c_int) * 2)

    def test_mixed_5(self):
        class X(Structure):
            _fields_ = [
                ('A', c_uint, 1),
                ('B', c_ushort, 16)]
        self.check_struct(X)
        a = X()
        a.A = 0
        a.B = 1
        self.assertEqual(1, a.B)

    def test_mixed_6(self):
        class X(Structure):
            _fields_ = [
                ('A', c_ulonglong, 1),
                ('B', c_uint, 32)]
        self.check_struct(X)
        a = X()
        a.A = 0
        a.B = 1
        self.assertEqual(1, a.B)

    @unittest.skipIf(sizeof(c_uint64) != alignment(c_uint64),
                     'assumes size=alignment')
    def test_mixed_7(self):
        class X(Structure):
            _fields_ = [
                ("A", c_uint32),
                ('B', c_uint32, 20),
                ('C', c_uint64, 24)]
        self.check_struct(X)
        self.assertEqual(16, sizeof(X))

    def test_mixed_8(self):
        class Foo(Structure):
            _fields_ = [
                ("A", c_uint32),
                ("B", c_uint32, 32),
                ("C", c_ulonglong, 1),
                ]
        self.check_struct(Foo)

        class Bar(Structure):
            _fields_ = [
                ("A", c_uint32),
                ("B", c_uint32),
                ("C", c_ulonglong, 1),
                ]
        self.check_struct(Bar)
        self.assertEqual(sizeof(Foo), sizeof(Bar))

    def test_mixed_9(self):
        class X(Structure):
            _fields_ = [
                ("A", c_uint8),
                ("B", c_uint32, 1),
                ]
        self.check_struct(X)
        if sys.platform == 'win32':
            self.assertEqual(8, sizeof(X))
        else:
            self.assertEqual(4, sizeof(X))

    @unittest.skipIf(sizeof(c_uint64) != alignment(c_uint64),
                     'assumes size=alignment')
    def test_mixed_10(self):
        class X(Structure):
            _fields_ = [
                ("A", c_uint32, 1),
                ("B", c_uint64, 1),
                ]
        self.check_struct(X)
        if sys.platform == 'win32':
            self.assertEqual(8, alignment(X))
            self.assertEqual(16, sizeof(X))
        else:
            self.assertEqual(8, alignment(X))
            self.assertEqual(8, sizeof(X))

    def test_gh_95496(self):
        for field_width in range(1, 33):
            class TestStruct(Structure):
                _fields_ = [
                    ("Field1", c_uint32, field_width),
                    ("Field2", c_uint8, 8)
                ]
            self.check_struct(TestStruct)

            cmd = TestStruct()
            cmd.Field2 = 1
            self.assertEqual(1, cmd.Field2)

    def test_gh_84039(self):
        class Bad(Structure):
            _pack_ = 1
            _layout_ = "ms"
            _fields_ = [
                ("a0", c_uint8, 1),
                ("a1", c_uint8, 1),
                ("a2", c_uint8, 1),
                ("a3", c_uint8, 1),
                ("a4", c_uint8, 1),
                ("a5", c_uint8, 1),
                ("a6", c_uint8, 1),
                ("a7", c_uint8, 1),
                ("b0", c_uint16, 4),
                ("b1", c_uint16, 12),
            ]

        class GoodA(Structure):
            _pack_ = 1
            _layout_ = "ms"
            _fields_ = [
                ("a0", c_uint8, 1),
                ("a1", c_uint8, 1),
                ("a2", c_uint8, 1),
                ("a3", c_uint8, 1),
                ("a4", c_uint8, 1),
                ("a5", c_uint8, 1),
                ("a6", c_uint8, 1),
                ("a7", c_uint8, 1),
            ]


        class Good(Structure):
            _pack_ = 1
            _layout_ = "ms"
            _fields_ = [
                ("a", GoodA),
                ("b0", c_uint16, 4),
                ("b1", c_uint16, 12),
            ]
        self.check_struct(Bad)
        self.check_struct(GoodA)
        self.check_struct(Good)

        self.assertEqual(3, sizeof(Bad))
        self.assertEqual(3, sizeof(Good))

    def test_gh_73939(self):
        class MyStructure(Structure):
            _pack_      = 1
            _layout_ = "ms"
            _fields_    = [
                            ("P",       c_uint16),
                            ("L",       c_uint16, 9),
                            ("Pro",     c_uint16, 1),
                            ("G",       c_uint16, 1),
                            ("IB",      c_uint16, 1),
                            ("IR",      c_uint16, 1),
                            ("R",       c_uint16, 3),
                            ("T",       c_uint32, 10),
                            ("C",       c_uint32, 20),
                            ("R2",      c_uint32, 2)
                        ]
        self.check_struct(MyStructure)
        self.assertEqual(8, sizeof(MyStructure))

    def test_gh_86098(self):
        class X(Structure):
            _fields_ = [
                ("a", c_uint8, 8),
                ("b", c_uint8, 8),
                ("c", c_uint32, 16)
            ]
        self.check_struct(X)
        if sys.platform == 'win32':
            self.assertEqual(8, sizeof(X))
        else:
            self.assertEqual(4, sizeof(X))

    def test_anon_bitfields(self):
        # anonymous bit-fields gave a strange error message
        class X(Structure):
            _fields_ = [("a", c_byte, 4),
                        ("b", c_ubyte, 4)]
        class Y(Structure):
            _anonymous_ = ["_"]
            _fields_ = [("_", X)]

        self.check_struct(X)
        self.check_struct(Y)

    def test_uint32(self):
        class X(Structure):
            _fields_ = [("a", c_uint32, 32)]
        self.check_struct(X)
        x = X()
        x.a = 10
        self.assertEqual(x.a, 10)
        x.a = 0xFDCBA987
        self.assertEqual(x.a, 0xFDCBA987)

    def test_uint64(self):
        class X(Structure):
            _fields_ = [("a", c_uint64, 64)]
        self.check_struct(X)
        x = X()
        x.a = 10
        self.assertEqual(x.a, 10)
        x.a = 0xFEDCBA9876543211
        self.assertEqual(x.a, 0xFEDCBA9876543211)

    def test_uint32_swap_little_endian(self):
        # Issue #23319
        class Little(LittleEndianStructure):
            _fields_ = [("a", c_uint32, 24),
                        ("b", c_uint32, 4),
                        ("c", c_uint32, 4)]
        self.check_struct(Little)
        b = bytearray(4)
        x = Little.from_buffer(b)
        x.a = 0xabcdef
        x.b = 1
        x.c = 2
        self.assertEqual(b, b'\xef\xcd\xab\x21')

    def test_uint32_swap_big_endian(self):
        # Issue #23319
        class Big(BigEndianStructure):
            _fields_ = [("a", c_uint32, 24),
                        ("b", c_uint32, 4),
                        ("c", c_uint32, 4)]
        self.check_struct(Big)
        b = bytearray(4)
        x = Big.from_buffer(b)
        x.a = 0xabcdef
        x.b = 1
        x.c = 2
        self.assertEqual(b, b'\xab\xcd\xef\x12')

    def test_union_bitfield(self):
        class BitfieldUnion(Union):
            _fields_ = [("a", c_uint32, 1),
                        ("b", c_uint32, 2),
                        ("c", c_uint32, 3)]
        self.check_union(BitfieldUnion)
        self.assertEqual(sizeof(BitfieldUnion), 4)
        b = bytearray(4)
        x = BitfieldUnion.from_buffer(b)
        x.a = 1
        self.assertEqual(int.from_bytes(b).bit_count(), 1)
        x.b = 3
        self.assertEqual(int.from_bytes(b).bit_count(), 2)
        x.c = 7
        self.assertEqual(int.from_bytes(b).bit_count(), 3)


if __name__ == "__main__":
    unittest.main()
