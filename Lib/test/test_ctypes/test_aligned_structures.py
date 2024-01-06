from ctypes import Structure, c_char, c_uint32, c_ubyte, alignment
import inspect
import unittest


class TestAlignedStructures(unittest.TestCase):
    def test_aligned_string(self):
        data = bytearray(
            b'\x07\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
            b'\x68\x65\x6c\x6c\x6f\x20\x77\x6f\x72\x6c\x64\x21\x00\x00\x00\x00'
        )

        class Aligned(Structure):
            _align_ = 16
            _fields_ = [
                ('value', c_char * 16),
            ]

        class Main(Structure):
            _fields_ = [
                ('first', c_uint32),
                ('string', Aligned),
            ]

        d = Main.from_buffer(data)
        self.assertEqual(d.first, 7)
        self.assertEqual(d.string.value, b'hello world!')
        self.assertEqual(
            bytes(d.string.__buffer__(inspect.BufferFlags.SIMPLE)),
            b'\x68\x65\x6c\x6c\x6f\x20\x77\x6f\x72\x6c\x64\x21\x00\x00\x00\x00'
        )

    def test_aligned_structures(self):
        data = bytearray(
            b'\x01\x00\x01\x00\x07\x00\x00\x00'
        )

        class SomeBools(Structure):
            _align_ = 4
            _fields_ = [
                ("bool1", c_ubyte),
                ("bool2", c_ubyte),
                ("bool3", c_ubyte),
            ]
        class Main(Structure):
            _fields_ = [
                ("x", SomeBools),
                ("y", c_uint32),
            ]

        class SomeBoolsTooBig(Structure):
            _align_ = 8
            _fields_ = [
                ("bool1", c_ubyte),
                ("bool2", c_ubyte),
                ("bool3", c_ubyte),
            ]
        class MainTooBig(Structure):
            _fields_ = [
                ("x", SomeBoolsTooBig),
                ("y", c_uint32),
            ]
        d = Main.from_buffer(data)
        self.assertEqual(d.y, 7)
        self.assertEqual(alignment(SomeBools), 4)
        self.assertEqual(d.x.bool1, True)
        self.assertEqual(d.x.bool2, False)
        self.assertEqual(d.x.bool3, True)

        with self.assertRaises(ValueError) as ctx:
            MainTooBig.from_buffer(data)
            self.assertEqual(
                ctx.exception.args[0],
                'Buffer size too small (4 instead of at least 8 bytes)'
            )

if __name__ == '__main__':
    unittest.main()
