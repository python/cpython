from ctypes import (
    c_char, c_uint32, c_uint16, c_ubyte, c_byte, alignment, sizeof,
    BigEndianStructure, LittleEndianStructure,
    BigEndianUnion, LittleEndianUnion,
)
import struct
import unittest
from ._support import StructCheckMixin

class TestAlignedStructures(unittest.TestCase, StructCheckMixin):
    def test_aligned_string(self):
        for base, e in (
            (LittleEndianStructure, "<"),
            (BigEndianStructure, ">"),
        ):
            data =  bytearray(struct.pack(f"{e}i12x16s", 7, b"hello world!"))
            class Aligned(base):
                _align_ = 16
                _fields_ = [
                    ('value', c_char * 12)
                ]
            self.check_struct(Aligned)

            class Main(base):
                _fields_ = [
                    ('first', c_uint32),
                    ('string', Aligned),
                ]
            self.check_struct(Main)

            main = Main.from_buffer(data)
            self.assertEqual(main.first, 7)
            self.assertEqual(main.string.value, b'hello world!')
            self.assertEqual(bytes(main.string), b'hello world!\0\0\0\0')
            self.assertEqual(Main.string.offset, 16)
            self.assertEqual(Main.string.size, 16)
            self.assertEqual(alignment(main.string), 16)
            self.assertEqual(alignment(main), 16)

    def test_aligned_structures(self):
        for base, data in (
            (LittleEndianStructure, bytearray(b"\1\0\0\0\1\0\0\0\7\0\0\0")),
            (BigEndianStructure, bytearray(b"\1\0\0\0\1\0\0\0\7\0\0\0")),
        ):
            class SomeBools(base):
                _align_ = 4
                _fields_ = [
                    ("bool1", c_ubyte),
                    ("bool2", c_ubyte),
                ]
            self.check_struct(SomeBools)
            class Main(base):
                _fields_ = [
                    ("x", c_ubyte),
                    ("y", SomeBools),
                    ("z", c_ubyte),
                ]
            self.check_struct(Main)

            main = Main.from_buffer(data)
            self.assertEqual(alignment(SomeBools), 4)
            self.assertEqual(alignment(main), 4)
            self.assertEqual(alignment(main.y), 4)
            self.assertEqual(Main.x.size, 1)
            self.assertEqual(Main.y.offset, 4)
            self.assertEqual(Main.y.size, 4)
            self.assertEqual(main.y.bool1, True)
            self.assertEqual(main.y.bool2, False)
            self.assertEqual(Main.z.offset, 8)
            self.assertEqual(main.z, 7)

    def test_oversized_structure(self):
        data = bytearray(b"\0" * 8)
        for base in (LittleEndianStructure, BigEndianStructure):
            class SomeBoolsTooBig(base):
                _align_ = 8
                _fields_ = [
                    ("bool1", c_ubyte),
                    ("bool2", c_ubyte),
                    ("bool3", c_ubyte),
                ]
            self.check_struct(SomeBoolsTooBig)
            class Main(base):
                _fields_ = [
                    ("y", SomeBoolsTooBig),
                    ("z", c_uint32),
                ]
            self.check_struct(Main)
            with self.assertRaises(ValueError) as ctx:
                Main.from_buffer(data)
                self.assertEqual(
                    ctx.exception.args[0],
                    'Buffer size too small (4 instead of at least 8 bytes)'
                )

    def test_aligned_subclasses(self):
        for base, e in (
            (LittleEndianStructure, "<"),
            (BigEndianStructure, ">"),
        ):
            data = bytearray(struct.pack(f"{e}4i", 1, 2, 3, 4))
            class UnalignedSub(base):
                x: c_uint32
                _fields_ = [
                    ("x", c_uint32),
                ]
            self.check_struct(UnalignedSub)

            class AlignedStruct(UnalignedSub):
                _align_ = 8
                _fields_ = [
                    ("y", c_uint32),
                ]
            self.check_struct(AlignedStruct)

            class Main(base):
                _fields_ = [
                    ("a", c_uint32),
                    ("b", AlignedStruct)
                ]
            self.check_struct(Main)

            main = Main.from_buffer(data)
            self.assertEqual(alignment(main.b), 8)
            self.assertEqual(alignment(main), 8)
            self.assertEqual(sizeof(main.b), 8)
            self.assertEqual(sizeof(main), 16)
            self.assertEqual(main.a, 1)
            self.assertEqual(main.b.x, 3)
            self.assertEqual(main.b.y, 4)
            self.assertEqual(Main.b.offset, 8)
            self.assertEqual(Main.b.size, 8)

    def test_aligned_union(self):
        for sbase, ubase, e in (
            (LittleEndianStructure, LittleEndianUnion, "<"),
            (BigEndianStructure, BigEndianUnion, ">"),
        ):
            data = bytearray(struct.pack(f"{e}4i", 1, 2, 3, 4))
            class AlignedUnion(ubase):
                _align_ = 8
                _fields_ = [
                    ("a", c_uint32),
                    ("b", c_ubyte * 7),
                ]
            self.check_union(AlignedUnion)

            class Main(sbase):
                _fields_ = [
                    ("first", c_uint32),
                    ("union", AlignedUnion),
                ]
            self.check_struct(Main)

            main = Main.from_buffer(data)
            self.assertEqual(main.first, 1)
            self.assertEqual(main.union.a, 3)
            self.assertEqual(bytes(main.union.b), data[8:-1])
            self.assertEqual(Main.union.offset, 8)
            self.assertEqual(Main.union.size, 8)
            self.assertEqual(alignment(main.union), 8)
            self.assertEqual(alignment(main), 8)

    def test_aligned_struct_in_union(self):
        for sbase, ubase, e in (
            (LittleEndianStructure, LittleEndianUnion, "<"),
            (BigEndianStructure, BigEndianUnion, ">"),
        ):
            data = bytearray(struct.pack(f"{e}4i", 1, 2, 3, 4))
            class Sub(sbase):
                _align_ = 8
                _fields_ = [
                    ("x", c_uint32),
                    ("y", c_uint32),
                ]
            self.check_struct(Sub)

            class MainUnion(ubase):
                _fields_ = [
                    ("a", c_uint32),
                    ("b", Sub),
                ]
            self.check_union(MainUnion)

            class Main(sbase):
                _fields_ = [
                    ("first", c_uint32),
                    ("union", MainUnion),
                ]
            self.check_struct(Main)

            main = Main.from_buffer(data)
            self.assertEqual(Main.first.size, 4)
            self.assertEqual(alignment(main.union), 8)
            self.assertEqual(alignment(main), 8)
            self.assertEqual(Main.union.offset, 8)
            self.assertEqual(Main.union.size, 8)
            self.assertEqual(main.first, 1)
            self.assertEqual(main.union.a, 3)
            self.assertEqual(main.union.b.x, 3)
            self.assertEqual(main.union.b.y, 4)

    def test_smaller_aligned_subclassed_union(self):
        for sbase, ubase, e in (
            (LittleEndianStructure, LittleEndianUnion, "<"),
            (BigEndianStructure, BigEndianUnion, ">"),
        ):
            data = bytearray(struct.pack(f"{e}H2xI", 1, 0xD60102D7))
            class SubUnion(ubase):
                _align_ = 2
                _fields_ = [
                    ("unsigned", c_ubyte),
                    ("signed", c_byte),
                ]
            self.check_union(SubUnion)

            class MainUnion(SubUnion):
                _fields_ = [
                    ("num", c_uint32)
                ]
            self.check_union(SubUnion)

            class Main(sbase):
                _fields_ = [
                    ("first", c_uint16),
                    ("union", MainUnion),
                ]
            self.check_struct(Main)

            main = Main.from_buffer(data)
            self.assertEqual(main.union.num, 0xD60102D7)
            self.assertEqual(main.union.unsigned, data[4])
            self.assertEqual(main.union.signed, data[4] - 256)
            self.assertEqual(alignment(main), 4)
            self.assertEqual(alignment(main.union), 4)
            self.assertEqual(Main.union.offset, 4)
            self.assertEqual(Main.union.size, 4)
            self.assertEqual(Main.first.size, 2)

    def test_larger_aligned_subclassed_union(self):
        for ubase, e in (
            (LittleEndianUnion, "<"),
            (BigEndianUnion, ">"),
        ):
            data = bytearray(struct.pack(f"{e}I4x", 0xD60102D6))
            class SubUnion(ubase):
                _align_ = 8
                _fields_ = [
                    ("unsigned", c_ubyte),
                    ("signed", c_byte),
                ]
            self.check_union(SubUnion)

            class Main(SubUnion):
                _fields_ = [
                    ("num", c_uint32)
                ]
            self.check_struct(Main)

            main = Main.from_buffer(data)
            self.assertEqual(alignment(main), 8)
            self.assertEqual(sizeof(main), 8)
            self.assertEqual(main.num, 0xD60102D6)
            self.assertEqual(main.unsigned, 0xD6)
            self.assertEqual(main.signed, -42)

    def test_aligned_packed_structures(self):
        for sbase, e in (
            (LittleEndianStructure, "<"),
            (BigEndianStructure, ">"),
        ):
            data = bytearray(struct.pack(f"{e}B2H4xB", 1, 2, 3, 4))

            class Inner(sbase):
                _align_ = 8
                _fields_ = [
                    ("x", c_uint16),
                    ("y", c_uint16),
                ]
            self.check_struct(Inner)

            class Main(sbase):
                _pack_ = 1
                _fields_ = [
                    ("a", c_ubyte),
                    ("b", Inner),
                    ("c", c_ubyte),
                ]
            self.check_struct(Main)

            main = Main.from_buffer(data)
            self.assertEqual(sizeof(main), 10)
            self.assertEqual(Main.b.offset, 1)
            # Alignment == 8 because _pack_ wins out.
            self.assertEqual(alignment(main.b), 8)
            # Size is still 8 though since inside this Structure, it will have
            # effect.
            self.assertEqual(sizeof(main.b), 8)
            self.assertEqual(Main.c.offset, 9)
            self.assertEqual(main.a, 1)
            self.assertEqual(main.b.x, 2)
            self.assertEqual(main.b.y, 3)
            self.assertEqual(main.c, 4)


if __name__ == '__main__':
    unittest.main()
