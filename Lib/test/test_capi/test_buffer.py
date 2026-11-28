import unittest
from test.support import import_helper

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
_testbuffer = import_helper.import_module('_testbuffer')
from _testbuffer import ndarray, ND_WRITABLE, ND_FORTRAN


class CAPITest(unittest.TestCase):
    def test_object_copydata(self):
        # Test PyObject_CopyData()
        copydata = _testlimitedcapi.object_copydata

        dest = bytearray(b'xyz')
        self.assertEqual(copydata(dest, b'abc'), 0)
        self.assertEqual(dest, b'abc')

        dest = bytearray(5)
        self.assertEqual(copydata(dest, memoryview(b'abc')), 0)
        self.assertEqual(dest, b'abc\0\0')

        dest = bytearray()
        self.assertEqual(copydata(dest, b''), 0)
        self.assertEqual(dest, b'')

        # Non-contiguous source
        dest = bytearray(4)
        self.assertEqual(copydata(dest, memoryview(b'abcdefgh')[::2]), 0)
        self.assertEqual(dest, b'aceg')

        # Non-contiguous destination
        base = bytearray(b'........')
        self.assertEqual(copydata(memoryview(base)[::2], b'wxyz'), 0)
        self.assertEqual(base, b'w.x.y.z.')

        # Destination too small
        dest = bytearray(b'xy')
        self.assertRaises(BufferError, copydata, dest, b'abc')
        self.assertEqual(dest, b'xy')

        # Read-only destination
        self.assertRaises(BufferError, copydata, b'xyz', b'abc')

        self.assertRaises(TypeError, copydata, bytearray(3), 'abc')
        self.assertRaises(TypeError, copydata, [], b'')
        self.assertRaises(TypeError, copydata, 42, b'')

        # CRASHES copydata(NULL, b'abc')
        # CRASHES copydata(bytearray(3), NULL)

    def test_object_copydata_multidim(self):
        # Test PyObject_CopyData() with multi-dimensional buffers
        copydata = _testlimitedcapi.object_copydata
        items = list(range(12))
        c_src = ndarray(items, shape=[3, 4], format='B')
        f_src = ndarray(items, shape=[3, 4], format='B', flags=ND_FORTRAN)

        for src in c_src, f_src:
            for flags in ND_WRITABLE, ND_WRITABLE|ND_FORTRAN:
                with self.subTest(src=src.flags, dest=flags):
                    dest = ndarray([0]*12, shape=[3, 4], format='B',
                                   flags=flags)
                    self.assertEqual(copydata(dest, src), 0)
                    self.assertEqual(dest.tolist(), src.tolist())

        # Multi-byte items
        src = ndarray([1, -2, 3, -4, 5, -6], shape=[2, 3], format='i')
        dest = ndarray([0]*6, shape=[2, 3], format='i',
                       flags=ND_WRITABLE|ND_FORTRAN)
        self.assertEqual(copydata(dest, src), 0)
        self.assertEqual(dest.tolist(), [[1, -2, 3], [-4, 5, -6]])

        # Non-contiguous source
        src = c_src[::2, ::2]
        dest = ndarray([0]*4, shape=[2, 2], format='B', flags=ND_WRITABLE)
        self.assertEqual(copydata(dest, src), 0)
        self.assertEqual(dest.tolist(), [[0, 2], [8, 10]])

    def test_buffer_fillcontiguousstrides(self):
        # Test PyBuffer_FillContiguousStrides()
        fill = _testlimitedcapi.buffer_fillcontiguousstrides

        self.assertEqual(fill((2, 3, 4), 8, 'C'), (96, 32, 8))
        self.assertEqual(fill((2, 3, 4), 8, 'F'), (8, 16, 48))
        self.assertEqual(fill((5,), 2, 'C'), (2,))
        self.assertEqual(fill((5,), 2, 'F'), (2,))
        self.assertEqual(fill((), 4, 'C'), ())
        self.assertEqual(fill((), 4, 'F'), ())
        self.assertEqual(fill((2, 0, 3), 4, 'C'), (0, 12, 4))
        self.assertEqual(fill((2, 0, 3), 4, 'F'), (4, 8, 0))


if __name__ == "__main__":
    unittest.main()
