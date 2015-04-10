import _compression
from io import BytesIO, UnsupportedOperation, DEFAULT_BUFFER_SIZE
import os
import pickle
import random
import unittest

from test.support import (
    _4G, TESTFN, import_module, bigmemtest, run_unittest, unlink
)

lzma = import_module("lzma")
from lzma import LZMACompressor, LZMADecompressor, LZMAError, LZMAFile


class CompressorDecompressorTestCase(unittest.TestCase):

    # Test error cases.

    def test_simple_bad_args(self):
        self.assertRaises(TypeError, LZMACompressor, [])
        self.assertRaises(TypeError, LZMACompressor, format=3.45)
        self.assertRaises(TypeError, LZMACompressor, check="")
        self.assertRaises(TypeError, LZMACompressor, preset="asdf")
        self.assertRaises(TypeError, LZMACompressor, filters=3)
        # Can't specify FORMAT_AUTO when compressing.
        self.assertRaises(ValueError, LZMACompressor, format=lzma.FORMAT_AUTO)
        # Can't specify a preset and a custom filter chain at the same time.
        with self.assertRaises(ValueError):
            LZMACompressor(preset=7, filters=[{"id": lzma.FILTER_LZMA2}])

        self.assertRaises(TypeError, LZMADecompressor, ())
        self.assertRaises(TypeError, LZMADecompressor, memlimit=b"qw")
        with self.assertRaises(TypeError):
            LZMADecompressor(lzma.FORMAT_RAW, filters="zzz")
        # Cannot specify a memory limit with FILTER_RAW.
        with self.assertRaises(ValueError):
            LZMADecompressor(lzma.FORMAT_RAW, memlimit=0x1000000)
        # Can only specify a custom filter chain with FILTER_RAW.
        self.assertRaises(ValueError, LZMADecompressor, filters=FILTERS_RAW_1)
        with self.assertRaises(ValueError):
            LZMADecompressor(format=lzma.FORMAT_XZ, filters=FILTERS_RAW_1)
        with self.assertRaises(ValueError):
            LZMADecompressor(format=lzma.FORMAT_ALONE, filters=FILTERS_RAW_1)

        lzc = LZMACompressor()
        self.assertRaises(TypeError, lzc.compress)
        self.assertRaises(TypeError, lzc.compress, b"foo", b"bar")
        self.assertRaises(TypeError, lzc.flush, b"blah")
        empty = lzc.flush()
        self.assertRaises(ValueError, lzc.compress, b"quux")
        self.assertRaises(ValueError, lzc.flush)

        lzd = LZMADecompressor()
        self.assertRaises(TypeError, lzd.decompress)
        self.assertRaises(TypeError, lzd.decompress, b"foo", b"bar")
        lzd.decompress(empty)
        self.assertRaises(EOFError, lzd.decompress, b"quux")

    def test_bad_filter_spec(self):
        self.assertRaises(TypeError, LZMACompressor, filters=[b"wobsite"])
        self.assertRaises(ValueError, LZMACompressor, filters=[{"xyzzy": 3}])
        self.assertRaises(ValueError, LZMACompressor, filters=[{"id": 98765}])
        with self.assertRaises(ValueError):
            LZMACompressor(filters=[{"id": lzma.FILTER_LZMA2, "foo": 0}])
        with self.assertRaises(ValueError):
            LZMACompressor(filters=[{"id": lzma.FILTER_DELTA, "foo": 0}])
        with self.assertRaises(ValueError):
            LZMACompressor(filters=[{"id": lzma.FILTER_X86, "foo": 0}])

    def test_decompressor_after_eof(self):
        lzd = LZMADecompressor()
        lzd.decompress(COMPRESSED_XZ)
        self.assertRaises(EOFError, lzd.decompress, b"nyan")

    def test_decompressor_memlimit(self):
        lzd = LZMADecompressor(memlimit=1024)
        self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_XZ)

        lzd = LZMADecompressor(lzma.FORMAT_XZ, memlimit=1024)
        self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_XZ)

        lzd = LZMADecompressor(lzma.FORMAT_ALONE, memlimit=1024)
        self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_ALONE)

    # Test LZMADecompressor on known-good input data.

    def _test_decompressor(self, lzd, data, check, unused_data=b""):
        self.assertFalse(lzd.eof)
        out = lzd.decompress(data)
        self.assertEqual(out, INPUT)
        self.assertEqual(lzd.check, check)
        self.assertTrue(lzd.eof)
        self.assertEqual(lzd.unused_data, unused_data)

    def test_decompressor_auto(self):
        lzd = LZMADecompressor()
        self._test_decompressor(lzd, COMPRESSED_XZ, lzma.CHECK_CRC64)

        lzd = LZMADecompressor()
        self._test_decompressor(lzd, COMPRESSED_ALONE, lzma.CHECK_NONE)

    def test_decompressor_xz(self):
        lzd = LZMADecompressor(lzma.FORMAT_XZ)
        self._test_decompressor(lzd, COMPRESSED_XZ, lzma.CHECK_CRC64)

    def test_decompressor_alone(self):
        lzd = LZMADecompressor(lzma.FORMAT_ALONE)
        self._test_decompressor(lzd, COMPRESSED_ALONE, lzma.CHECK_NONE)

    def test_decompressor_raw_1(self):
        lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_1)
        self._test_decompressor(lzd, COMPRESSED_RAW_1, lzma.CHECK_NONE)

    def test_decompressor_raw_2(self):
        lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_2)
        self._test_decompressor(lzd, COMPRESSED_RAW_2, lzma.CHECK_NONE)

    def test_decompressor_raw_3(self):
        lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_3)
        self._test_decompressor(lzd, COMPRESSED_RAW_3, lzma.CHECK_NONE)

    def test_decompressor_raw_4(self):
        lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        self._test_decompressor(lzd, COMPRESSED_RAW_4, lzma.CHECK_NONE)

    def test_decompressor_chunks(self):
        lzd = LZMADecompressor()
        out = []
        for i in range(0, len(COMPRESSED_XZ), 10):
            self.assertFalse(lzd.eof)
            out.append(lzd.decompress(COMPRESSED_XZ[i:i+10]))
        out = b"".join(out)
        self.assertEqual(out, INPUT)
        self.assertEqual(lzd.check, lzma.CHECK_CRC64)
        self.assertTrue(lzd.eof)
        self.assertEqual(lzd.unused_data, b"")

    def test_decompressor_chunks_maxsize(self):
        lzd = LZMADecompressor()
        max_length = 100
        out = []

        # Feed first half the input
        len_ = len(COMPRESSED_XZ) // 2
        out.append(lzd.decompress(COMPRESSED_XZ[:len_],
                                  max_length=max_length))
        self.assertFalse(lzd.needs_input)
        self.assertEqual(len(out[-1]), max_length)

        # Retrieve more data without providing more input
        out.append(lzd.decompress(b'', max_length=max_length))
        self.assertFalse(lzd.needs_input)
        self.assertEqual(len(out[-1]), max_length)

        # Retrieve more data while providing more input
        out.append(lzd.decompress(COMPRESSED_XZ[len_:],
                                  max_length=max_length))
        self.assertLessEqual(len(out[-1]), max_length)

        # Retrieve remaining uncompressed data
        while not lzd.eof:
            out.append(lzd.decompress(b'', max_length=max_length))
            self.assertLessEqual(len(out[-1]), max_length)

        out = b"".join(out)
        self.assertEqual(out, INPUT)
        self.assertEqual(lzd.check, lzma.CHECK_CRC64)
        self.assertEqual(lzd.unused_data, b"")

    def test_decompressor_inputbuf_1(self):
        # Test reusing input buffer after moving existing
        # contents to beginning
        lzd = LZMADecompressor()
        out = []

        # Create input buffer and fill it
        self.assertEqual(lzd.decompress(COMPRESSED_XZ[:100],
                                        max_length=0), b'')

        # Retrieve some results, freeing capacity at beginning
        # of input buffer
        out.append(lzd.decompress(b'', 2))

        # Add more data that fits into input buffer after
        # moving existing data to beginning
        out.append(lzd.decompress(COMPRESSED_XZ[100:105], 15))

        # Decompress rest of data
        out.append(lzd.decompress(COMPRESSED_XZ[105:]))
        self.assertEqual(b''.join(out), INPUT)

    def test_decompressor_inputbuf_2(self):
        # Test reusing input buffer by appending data at the
        # end right away
        lzd = LZMADecompressor()
        out = []

        # Create input buffer and empty it
        self.assertEqual(lzd.decompress(COMPRESSED_XZ[:200],
                                        max_length=0), b'')
        out.append(lzd.decompress(b''))

        # Fill buffer with new data
        out.append(lzd.decompress(COMPRESSED_XZ[200:280], 2))

        # Append some more data, not enough to require resize
        out.append(lzd.decompress(COMPRESSED_XZ[280:300], 2))

        # Decompress rest of data
        out.append(lzd.decompress(COMPRESSED_XZ[300:]))
        self.assertEqual(b''.join(out), INPUT)

    def test_decompressor_inputbuf_3(self):
        # Test reusing input buffer after extending it

        lzd = LZMADecompressor()
        out = []

        # Create almost full input buffer
        out.append(lzd.decompress(COMPRESSED_XZ[:200], 5))

        # Add even more data to it, requiring resize
        out.append(lzd.decompress(COMPRESSED_XZ[200:300], 5))

        # Decompress rest of data
        out.append(lzd.decompress(COMPRESSED_XZ[300:]))
        self.assertEqual(b''.join(out), INPUT)

    def test_decompressor_unused_data(self):
        lzd = LZMADecompressor()
        extra = b"fooblibar"
        self._test_decompressor(lzd, COMPRESSED_XZ + extra, lzma.CHECK_CRC64,
                                unused_data=extra)

    def test_decompressor_bad_input(self):
        lzd = LZMADecompressor()
        self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_RAW_1)

        lzd = LZMADecompressor(lzma.FORMAT_XZ)
        self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_ALONE)

        lzd = LZMADecompressor(lzma.FORMAT_ALONE)
        self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_XZ)

        lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_1)
        self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_XZ)

    # Test that LZMACompressor->LZMADecompressor preserves the input data.

    def test_roundtrip_xz(self):
        lzc = LZMACompressor()
        cdata = lzc.compress(INPUT) + lzc.flush()
        lzd = LZMADecompressor()
        self._test_decompressor(lzd, cdata, lzma.CHECK_CRC64)

    def test_roundtrip_alone(self):
        lzc = LZMACompressor(lzma.FORMAT_ALONE)
        cdata = lzc.compress(INPUT) + lzc.flush()
        lzd = LZMADecompressor()
        self._test_decompressor(lzd, cdata, lzma.CHECK_NONE)

    def test_roundtrip_raw(self):
        lzc = LZMACompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        cdata = lzc.compress(INPUT) + lzc.flush()
        lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        self._test_decompressor(lzd, cdata, lzma.CHECK_NONE)

    def test_roundtrip_chunks(self):
        lzc = LZMACompressor()
        cdata = []
        for i in range(0, len(INPUT), 10):
            cdata.append(lzc.compress(INPUT[i:i+10]))
        cdata.append(lzc.flush())
        cdata = b"".join(cdata)
        lzd = LZMADecompressor()
        self._test_decompressor(lzd, cdata, lzma.CHECK_CRC64)

    # LZMADecompressor intentionally does not handle concatenated streams.

    def test_decompressor_multistream(self):
        lzd = LZMADecompressor()
        self._test_decompressor(lzd, COMPRESSED_XZ + COMPRESSED_ALONE,
                                lzma.CHECK_CRC64, unused_data=COMPRESSED_ALONE)

    # Test with inputs larger than 4GiB.

    @bigmemtest(size=_4G + 100, memuse=2)
    def test_compressor_bigmem(self, size):
        lzc = LZMACompressor()
        cdata = lzc.compress(b"x" * size) + lzc.flush()
        ddata = lzma.decompress(cdata)
        try:
            self.assertEqual(len(ddata), size)
            self.assertEqual(len(ddata.strip(b"x")), 0)
        finally:
            ddata = None

    @bigmemtest(size=_4G + 100, memuse=3)
    def test_decompressor_bigmem(self, size):
        lzd = LZMADecompressor()
        blocksize = 10 * 1024 * 1024
        block = random.getrandbits(blocksize * 8).to_bytes(blocksize, "little")
        try:
            input = block * (size // blocksize + 1)
            cdata = lzma.compress(input)
            ddata = lzd.decompress(cdata)
            self.assertEqual(ddata, input)
        finally:
            input = cdata = ddata = None

    # Pickling raises an exception; there's no way to serialize an lzma_stream.

    def test_pickle(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises(TypeError):
                pickle.dumps(LZMACompressor(), proto)
            with self.assertRaises(TypeError):
                pickle.dumps(LZMADecompressor(), proto)


class CompressDecompressFunctionTestCase(unittest.TestCase):

    # Test error cases:

    def test_bad_args(self):
        self.assertRaises(TypeError, lzma.compress)
        self.assertRaises(TypeError, lzma.compress, [])
        self.assertRaises(TypeError, lzma.compress, b"", format="xz")
        self.assertRaises(TypeError, lzma.compress, b"", check="none")
        self.assertRaises(TypeError, lzma.compress, b"", preset="blah")
        self.assertRaises(TypeError, lzma.compress, b"", filters=1024)
        # Can't specify a preset and a custom filter chain at the same time.
        with self.assertRaises(ValueError):
            lzma.compress(b"", preset=3, filters=[{"id": lzma.FILTER_LZMA2}])

        self.assertRaises(TypeError, lzma.decompress)
        self.assertRaises(TypeError, lzma.decompress, [])
        self.assertRaises(TypeError, lzma.decompress, b"", format="lzma")
        self.assertRaises(TypeError, lzma.decompress, b"", memlimit=7.3e9)
        with self.assertRaises(TypeError):
            lzma.decompress(b"", format=lzma.FORMAT_RAW, filters={})
        # Cannot specify a memory limit with FILTER_RAW.
        with self.assertRaises(ValueError):
            lzma.decompress(b"", format=lzma.FORMAT_RAW, memlimit=0x1000000)
        # Can only specify a custom filter chain with FILTER_RAW.
        with self.assertRaises(ValueError):
            lzma.decompress(b"", filters=FILTERS_RAW_1)
        with self.assertRaises(ValueError):
            lzma.decompress(b"", format=lzma.FORMAT_XZ, filters=FILTERS_RAW_1)
        with self.assertRaises(ValueError):
            lzma.decompress(
                    b"", format=lzma.FORMAT_ALONE, filters=FILTERS_RAW_1)

    def test_decompress_memlimit(self):
        with self.assertRaises(LZMAError):
            lzma.decompress(COMPRESSED_XZ, memlimit=1024)
        with self.assertRaises(LZMAError):
            lzma.decompress(
                    COMPRESSED_XZ, format=lzma.FORMAT_XZ, memlimit=1024)
        with self.assertRaises(LZMAError):
            lzma.decompress(
                    COMPRESSED_ALONE, format=lzma.FORMAT_ALONE, memlimit=1024)

    # Test LZMADecompressor on known-good input data.

    def test_decompress_good_input(self):
        ddata = lzma.decompress(COMPRESSED_XZ)
        self.assertEqual(ddata, INPUT)

        ddata = lzma.decompress(COMPRESSED_ALONE)
        self.assertEqual(ddata, INPUT)

        ddata = lzma.decompress(COMPRESSED_XZ, lzma.FORMAT_XZ)
        self.assertEqual(ddata, INPUT)

        ddata = lzma.decompress(COMPRESSED_ALONE, lzma.FORMAT_ALONE)
        self.assertEqual(ddata, INPUT)

        ddata = lzma.decompress(
                COMPRESSED_RAW_1, lzma.FORMAT_RAW, filters=FILTERS_RAW_1)
        self.assertEqual(ddata, INPUT)

        ddata = lzma.decompress(
                COMPRESSED_RAW_2, lzma.FORMAT_RAW, filters=FILTERS_RAW_2)
        self.assertEqual(ddata, INPUT)

        ddata = lzma.decompress(
                COMPRESSED_RAW_3, lzma.FORMAT_RAW, filters=FILTERS_RAW_3)
        self.assertEqual(ddata, INPUT)

        ddata = lzma.decompress(
                COMPRESSED_RAW_4, lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        self.assertEqual(ddata, INPUT)

    def test_decompress_incomplete_input(self):
        self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_XZ[:128])
        self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_ALONE[:128])
        self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_RAW_1[:128],
                          format=lzma.FORMAT_RAW, filters=FILTERS_RAW_1)
        self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_RAW_2[:128],
                          format=lzma.FORMAT_RAW, filters=FILTERS_RAW_2)
        self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_RAW_3[:128],
                          format=lzma.FORMAT_RAW, filters=FILTERS_RAW_3)
        self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_RAW_4[:128],
                          format=lzma.FORMAT_RAW, filters=FILTERS_RAW_4)

    def test_decompress_bad_input(self):
        with self.assertRaises(LZMAError):
            lzma.decompress(COMPRESSED_BOGUS)
        with self.assertRaises(LZMAError):
            lzma.decompress(COMPRESSED_RAW_1)
        with self.assertRaises(LZMAError):
            lzma.decompress(COMPRESSED_ALONE, format=lzma.FORMAT_XZ)
        with self.assertRaises(LZMAError):
            lzma.decompress(COMPRESSED_XZ, format=lzma.FORMAT_ALONE)
        with self.assertRaises(LZMAError):
            lzma.decompress(COMPRESSED_XZ, format=lzma.FORMAT_RAW,
                            filters=FILTERS_RAW_1)

    # Test that compress()->decompress() preserves the input data.

    def test_roundtrip(self):
        cdata = lzma.compress(INPUT)
        ddata = lzma.decompress(cdata)
        self.assertEqual(ddata, INPUT)

        cdata = lzma.compress(INPUT, lzma.FORMAT_XZ)
        ddata = lzma.decompress(cdata)
        self.assertEqual(ddata, INPUT)

        cdata = lzma.compress(INPUT, lzma.FORMAT_ALONE)
        ddata = lzma.decompress(cdata)
        self.assertEqual(ddata, INPUT)

        cdata = lzma.compress(INPUT, lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        ddata = lzma.decompress(cdata, lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        self.assertEqual(ddata, INPUT)

    # Unlike LZMADecompressor, decompress() *does* handle concatenated streams.

    def test_decompress_multistream(self):
        ddata = lzma.decompress(COMPRESSED_XZ + COMPRESSED_ALONE)
        self.assertEqual(ddata, INPUT * 2)

    # Test robust handling of non-LZMA data following the compressed stream(s).

    def test_decompress_trailing_junk(self):
        ddata = lzma.decompress(COMPRESSED_XZ + COMPRESSED_BOGUS)
        self.assertEqual(ddata, INPUT)

    def test_decompress_multistream_trailing_junk(self):
        ddata = lzma.decompress(COMPRESSED_XZ * 3 + COMPRESSED_BOGUS)
        self.assertEqual(ddata, INPUT * 3)


class TempFile:
    """Context manager - creates a file, and deletes it on __exit__."""

    def __init__(self, filename, data=b""):
        self.filename = filename
        self.data = data

    def __enter__(self):
        with open(self.filename, "wb") as f:
            f.write(self.data)

    def __exit__(self, *args):
        unlink(self.filename)


class FileTestCase(unittest.TestCase):

    def test_init(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            pass
        with LZMAFile(BytesIO(), "w") as f:
            pass
        with LZMAFile(BytesIO(), "x") as f:
            pass
        with LZMAFile(BytesIO(), "a") as f:
            pass

    def test_init_with_filename(self):
        with TempFile(TESTFN, COMPRESSED_XZ):
            with LZMAFile(TESTFN) as f:
                pass
            with LZMAFile(TESTFN, "w") as f:
                pass
            with LZMAFile(TESTFN, "a") as f:
                pass

    def test_init_mode(self):
        with TempFile(TESTFN):
            with LZMAFile(TESTFN, "r"):
                pass
            with LZMAFile(TESTFN, "rb"):
                pass
            with LZMAFile(TESTFN, "w"):
                pass
            with LZMAFile(TESTFN, "wb"):
                pass
            with LZMAFile(TESTFN, "a"):
                pass
            with LZMAFile(TESTFN, "ab"):
                pass

    def test_init_with_x_mode(self):
        self.addCleanup(unlink, TESTFN)
        for mode in ("x", "xb"):
            unlink(TESTFN)
            with LZMAFile(TESTFN, mode):
                pass
            with self.assertRaises(FileExistsError):
                with LZMAFile(TESTFN, mode):
                    pass

    def test_init_bad_mode(self):
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), (3, "x"))
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "")
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "xt")
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "x+")
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "rx")
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "wx")
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "rt")
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "r+")
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "wt")
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "w+")
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), "rw")

    def test_init_bad_check(self):
        with self.assertRaises(TypeError):
            LZMAFile(BytesIO(), "w", check=b"asd")
        # CHECK_UNKNOWN and anything above CHECK_ID_MAX should be invalid.
        with self.assertRaises(LZMAError):
            LZMAFile(BytesIO(), "w", check=lzma.CHECK_UNKNOWN)
        with self.assertRaises(LZMAError):
            LZMAFile(BytesIO(), "w", check=lzma.CHECK_ID_MAX + 3)
        # Cannot specify a check with mode="r".
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_NONE)
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_CRC32)
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_CRC64)
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_SHA256)
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_UNKNOWN)

    def test_init_bad_preset(self):
        with self.assertRaises(TypeError):
            LZMAFile(BytesIO(), "w", preset=4.39)
        with self.assertRaises(LZMAError):
            LZMAFile(BytesIO(), "w", preset=10)
        with self.assertRaises(LZMAError):
            LZMAFile(BytesIO(), "w", preset=23)
        with self.assertRaises(OverflowError):
            LZMAFile(BytesIO(), "w", preset=-1)
        with self.assertRaises(OverflowError):
            LZMAFile(BytesIO(), "w", preset=-7)
        with self.assertRaises(TypeError):
            LZMAFile(BytesIO(), "w", preset="foo")
        # Cannot specify a preset with mode="r".
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(COMPRESSED_XZ), preset=3)

    def test_init_bad_filter_spec(self):
        with self.assertRaises(TypeError):
            LZMAFile(BytesIO(), "w", filters=[b"wobsite"])
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(), "w", filters=[{"xyzzy": 3}])
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(), "w", filters=[{"id": 98765}])
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(), "w",
                     filters=[{"id": lzma.FILTER_LZMA2, "foo": 0}])
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(), "w",
                     filters=[{"id": lzma.FILTER_DELTA, "foo": 0}])
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(), "w",
                     filters=[{"id": lzma.FILTER_X86, "foo": 0}])

    def test_init_with_preset_and_filters(self):
        with self.assertRaises(ValueError):
            LZMAFile(BytesIO(), "w", format=lzma.FORMAT_RAW,
                     preset=6, filters=FILTERS_RAW_1)

    def test_close(self):
        with BytesIO(COMPRESSED_XZ) as src:
            f = LZMAFile(src)
            f.close()
            # LZMAFile.close() should not close the underlying file object.
            self.assertFalse(src.closed)
            # Try closing an already-closed LZMAFile.
            f.close()
            self.assertFalse(src.closed)

        # Test with a real file on disk, opened directly by LZMAFile.
        with TempFile(TESTFN, COMPRESSED_XZ):
            f = LZMAFile(TESTFN)
            fp = f._fp
            f.close()
            # Here, LZMAFile.close() *should* close the underlying file object.
            self.assertTrue(fp.closed)
            # Try closing an already-closed LZMAFile.
            f.close()

    def test_closed(self):
        f = LZMAFile(BytesIO(COMPRESSED_XZ))
        try:
            self.assertFalse(f.closed)
            f.read()
            self.assertFalse(f.closed)
        finally:
            f.close()
        self.assertTrue(f.closed)

        f = LZMAFile(BytesIO(), "w")
        try:
            self.assertFalse(f.closed)
        finally:
            f.close()
        self.assertTrue(f.closed)

    def test_fileno(self):
        f = LZMAFile(BytesIO(COMPRESSED_XZ))
        try:
            self.assertRaises(UnsupportedOperation, f.fileno)
        finally:
            f.close()
        self.assertRaises(ValueError, f.fileno)
        with TempFile(TESTFN, COMPRESSED_XZ):
            f = LZMAFile(TESTFN)
            try:
                self.assertEqual(f.fileno(), f._fp.fileno())
                self.assertIsInstance(f.fileno(), int)
            finally:
                f.close()
        self.assertRaises(ValueError, f.fileno)

    def test_seekable(self):
        f = LZMAFile(BytesIO(COMPRESSED_XZ))
        try:
            self.assertTrue(f.seekable())
            f.read()
            self.assertTrue(f.seekable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.seekable)

        f = LZMAFile(BytesIO(), "w")
        try:
            self.assertFalse(f.seekable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.seekable)

        src = BytesIO(COMPRESSED_XZ)
        src.seekable = lambda: False
        f = LZMAFile(src)
        try:
            self.assertFalse(f.seekable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.seekable)

    def test_readable(self):
        f = LZMAFile(BytesIO(COMPRESSED_XZ))
        try:
            self.assertTrue(f.readable())
            f.read()
            self.assertTrue(f.readable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.readable)

        f = LZMAFile(BytesIO(), "w")
        try:
            self.assertFalse(f.readable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.readable)

    def test_writable(self):
        f = LZMAFile(BytesIO(COMPRESSED_XZ))
        try:
            self.assertFalse(f.writable())
            f.read()
            self.assertFalse(f.writable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.writable)

        f = LZMAFile(BytesIO(), "w")
        try:
            self.assertTrue(f.writable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.writable)

    def test_read(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            self.assertEqual(f.read(), INPUT)
            self.assertEqual(f.read(), b"")
        with LZMAFile(BytesIO(COMPRESSED_ALONE)) as f:
            self.assertEqual(f.read(), INPUT)
        with LZMAFile(BytesIO(COMPRESSED_XZ), format=lzma.FORMAT_XZ) as f:
            self.assertEqual(f.read(), INPUT)
            self.assertEqual(f.read(), b"")
        with LZMAFile(BytesIO(COMPRESSED_ALONE), format=lzma.FORMAT_ALONE) as f:
            self.assertEqual(f.read(), INPUT)
            self.assertEqual(f.read(), b"")
        with LZMAFile(BytesIO(COMPRESSED_RAW_1),
                      format=lzma.FORMAT_RAW, filters=FILTERS_RAW_1) as f:
            self.assertEqual(f.read(), INPUT)
            self.assertEqual(f.read(), b"")
        with LZMAFile(BytesIO(COMPRESSED_RAW_2),
                      format=lzma.FORMAT_RAW, filters=FILTERS_RAW_2) as f:
            self.assertEqual(f.read(), INPUT)
            self.assertEqual(f.read(), b"")
        with LZMAFile(BytesIO(COMPRESSED_RAW_3),
                      format=lzma.FORMAT_RAW, filters=FILTERS_RAW_3) as f:
            self.assertEqual(f.read(), INPUT)
            self.assertEqual(f.read(), b"")
        with LZMAFile(BytesIO(COMPRESSED_RAW_4),
                      format=lzma.FORMAT_RAW, filters=FILTERS_RAW_4) as f:
            self.assertEqual(f.read(), INPUT)
            self.assertEqual(f.read(), b"")

    def test_read_0(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            self.assertEqual(f.read(0), b"")
        with LZMAFile(BytesIO(COMPRESSED_ALONE)) as f:
            self.assertEqual(f.read(0), b"")
        with LZMAFile(BytesIO(COMPRESSED_XZ), format=lzma.FORMAT_XZ) as f:
            self.assertEqual(f.read(0), b"")
        with LZMAFile(BytesIO(COMPRESSED_ALONE), format=lzma.FORMAT_ALONE) as f:
            self.assertEqual(f.read(0), b"")

    def test_read_10(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            chunks = []
            while True:
                result = f.read(10)
                if not result:
                    break
                self.assertLessEqual(len(result), 10)
                chunks.append(result)
            self.assertEqual(b"".join(chunks), INPUT)

    def test_read_multistream(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ * 5)) as f:
            self.assertEqual(f.read(), INPUT * 5)
        with LZMAFile(BytesIO(COMPRESSED_XZ + COMPRESSED_ALONE)) as f:
            self.assertEqual(f.read(), INPUT * 2)
        with LZMAFile(BytesIO(COMPRESSED_RAW_3 * 4),
                      format=lzma.FORMAT_RAW, filters=FILTERS_RAW_3) as f:
            self.assertEqual(f.read(), INPUT * 4)

    def test_read_multistream_buffer_size_aligned(self):
        # Test the case where a stream boundary coincides with the end
        # of the raw read buffer.
        saved_buffer_size = _compression.BUFFER_SIZE
        _compression.BUFFER_SIZE = len(COMPRESSED_XZ)
        try:
            with LZMAFile(BytesIO(COMPRESSED_XZ *  5)) as f:
                self.assertEqual(f.read(), INPUT * 5)
        finally:
            _compression.BUFFER_SIZE = saved_buffer_size

    def test_read_trailing_junk(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ + COMPRESSED_BOGUS)) as f:
            self.assertEqual(f.read(), INPUT)

    def test_read_multistream_trailing_junk(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ * 5 + COMPRESSED_BOGUS)) as f:
            self.assertEqual(f.read(), INPUT * 5)

    def test_read_from_file(self):
        with TempFile(TESTFN, COMPRESSED_XZ):
            with LZMAFile(TESTFN) as f:
                self.assertEqual(f.read(), INPUT)
                self.assertEqual(f.read(), b"")

    def test_read_from_file_with_bytes_filename(self):
        try:
            bytes_filename = TESTFN.encode("ascii")
        except UnicodeEncodeError:
            self.skipTest("Temporary file name needs to be ASCII")
        with TempFile(TESTFN, COMPRESSED_XZ):
            with LZMAFile(bytes_filename) as f:
                self.assertEqual(f.read(), INPUT)
                self.assertEqual(f.read(), b"")

    def test_read_incomplete(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ[:128])) as f:
            self.assertRaises(EOFError, f.read)

    def test_read_truncated(self):
        # Drop stream footer: CRC (4 bytes), index size (4 bytes),
        # flags (2 bytes) and magic number (2 bytes).
        truncated = COMPRESSED_XZ[:-12]
        with LZMAFile(BytesIO(truncated)) as f:
            self.assertRaises(EOFError, f.read)
        with LZMAFile(BytesIO(truncated)) as f:
            self.assertEqual(f.read(len(INPUT)), INPUT)
            self.assertRaises(EOFError, f.read, 1)
        # Incomplete 12-byte header.
        for i in range(12):
            with LZMAFile(BytesIO(truncated[:i])) as f:
                self.assertRaises(EOFError, f.read, 1)

    def test_read_bad_args(self):
        f = LZMAFile(BytesIO(COMPRESSED_XZ))
        f.close()
        self.assertRaises(ValueError, f.read)
        with LZMAFile(BytesIO(), "w") as f:
            self.assertRaises(ValueError, f.read)
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            self.assertRaises(TypeError, f.read, float())

    def test_read_bad_data(self):
        with LZMAFile(BytesIO(COMPRESSED_BOGUS)) as f:
            self.assertRaises(LZMAError, f.read)

    def test_read1(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            blocks = []
            while True:
                result = f.read1()
                if not result:
                    break
                blocks.append(result)
            self.assertEqual(b"".join(blocks), INPUT)
            self.assertEqual(f.read1(), b"")

    def test_read1_0(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            self.assertEqual(f.read1(0), b"")

    def test_read1_10(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            blocks = []
            while True:
                result = f.read1(10)
                if not result:
                    break
                blocks.append(result)
            self.assertEqual(b"".join(blocks), INPUT)
            self.assertEqual(f.read1(), b"")

    def test_read1_multistream(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ * 5)) as f:
            blocks = []
            while True:
                result = f.read1()
                if not result:
                    break
                blocks.append(result)
            self.assertEqual(b"".join(blocks), INPUT * 5)
            self.assertEqual(f.read1(), b"")

    def test_read1_bad_args(self):
        f = LZMAFile(BytesIO(COMPRESSED_XZ))
        f.close()
        self.assertRaises(ValueError, f.read1)
        with LZMAFile(BytesIO(), "w") as f:
            self.assertRaises(ValueError, f.read1)
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            self.assertRaises(TypeError, f.read1, None)

    def test_peek(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            result = f.peek()
            self.assertGreater(len(result), 0)
            self.assertTrue(INPUT.startswith(result))
            self.assertEqual(f.read(), INPUT)
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            result = f.peek(10)
            self.assertGreater(len(result), 0)
            self.assertTrue(INPUT.startswith(result))
            self.assertEqual(f.read(), INPUT)

    def test_peek_bad_args(self):
        with LZMAFile(BytesIO(), "w") as f:
            self.assertRaises(ValueError, f.peek)

    def test_iterator(self):
        with BytesIO(INPUT) as f:
            lines = f.readlines()
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            self.assertListEqual(list(iter(f)), lines)
        with LZMAFile(BytesIO(COMPRESSED_ALONE)) as f:
            self.assertListEqual(list(iter(f)), lines)
        with LZMAFile(BytesIO(COMPRESSED_XZ), format=lzma.FORMAT_XZ) as f:
            self.assertListEqual(list(iter(f)), lines)
        with LZMAFile(BytesIO(COMPRESSED_ALONE), format=lzma.FORMAT_ALONE) as f:
            self.assertListEqual(list(iter(f)), lines)
        with LZMAFile(BytesIO(COMPRESSED_RAW_2),
                      format=lzma.FORMAT_RAW, filters=FILTERS_RAW_2) as f:
            self.assertListEqual(list(iter(f)), lines)

    def test_readline(self):
        with BytesIO(INPUT) as f:
            lines = f.readlines()
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            for line in lines:
                self.assertEqual(f.readline(), line)

    def test_readlines(self):
        with BytesIO(INPUT) as f:
            lines = f.readlines()
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            self.assertListEqual(f.readlines(), lines)

    def test_decompress_limited(self):
        """Decompressed data buffering should be limited"""
        bomb = lzma.compress(bytes(int(2e6)), preset=6)
        self.assertLess(len(bomb), _compression.BUFFER_SIZE)

        decomp = LZMAFile(BytesIO(bomb))
        self.assertEqual(bytes(1), decomp.read(1))
        max_decomp = 1 + DEFAULT_BUFFER_SIZE
        self.assertLessEqual(decomp._buffer.raw.tell(), max_decomp,
            "Excessive amount of data was decompressed")

    def test_write(self):
        with BytesIO() as dst:
            with LZMAFile(dst, "w") as f:
                f.write(INPUT)
            expected = lzma.compress(INPUT)
            self.assertEqual(dst.getvalue(), expected)
        with BytesIO() as dst:
            with LZMAFile(dst, "w", format=lzma.FORMAT_XZ) as f:
                f.write(INPUT)
            expected = lzma.compress(INPUT, format=lzma.FORMAT_XZ)
            self.assertEqual(dst.getvalue(), expected)
        with BytesIO() as dst:
            with LZMAFile(dst, "w", format=lzma.FORMAT_ALONE) as f:
                f.write(INPUT)
            expected = lzma.compress(INPUT, format=lzma.FORMAT_ALONE)
            self.assertEqual(dst.getvalue(), expected)
        with BytesIO() as dst:
            with LZMAFile(dst, "w", format=lzma.FORMAT_RAW,
                          filters=FILTERS_RAW_2) as f:
                f.write(INPUT)
            expected = lzma.compress(INPUT, format=lzma.FORMAT_RAW,
                                     filters=FILTERS_RAW_2)
            self.assertEqual(dst.getvalue(), expected)

    def test_write_10(self):
        with BytesIO() as dst:
            with LZMAFile(dst, "w") as f:
                for start in range(0, len(INPUT), 10):
                    f.write(INPUT[start:start+10])
            expected = lzma.compress(INPUT)
            self.assertEqual(dst.getvalue(), expected)

    def test_write_append(self):
        part1 = INPUT[:1024]
        part2 = INPUT[1024:1536]
        part3 = INPUT[1536:]
        expected = b"".join(lzma.compress(x) for x in (part1, part2, part3))
        with BytesIO() as dst:
            with LZMAFile(dst, "w") as f:
                f.write(part1)
            with LZMAFile(dst, "a") as f:
                f.write(part2)
            with LZMAFile(dst, "a") as f:
                f.write(part3)
            self.assertEqual(dst.getvalue(), expected)

    def test_write_to_file(self):
        try:
            with LZMAFile(TESTFN, "w") as f:
                f.write(INPUT)
            expected = lzma.compress(INPUT)
            with open(TESTFN, "rb") as f:
                self.assertEqual(f.read(), expected)
        finally:
            unlink(TESTFN)

    def test_write_to_file_with_bytes_filename(self):
        try:
            bytes_filename = TESTFN.encode("ascii")
        except UnicodeEncodeError:
            self.skipTest("Temporary file name needs to be ASCII")
        try:
            with LZMAFile(bytes_filename, "w") as f:
                f.write(INPUT)
            expected = lzma.compress(INPUT)
            with open(TESTFN, "rb") as f:
                self.assertEqual(f.read(), expected)
        finally:
            unlink(TESTFN)

    def test_write_append_to_file(self):
        part1 = INPUT[:1024]
        part2 = INPUT[1024:1536]
        part3 = INPUT[1536:]
        expected = b"".join(lzma.compress(x) for x in (part1, part2, part3))
        try:
            with LZMAFile(TESTFN, "w") as f:
                f.write(part1)
            with LZMAFile(TESTFN, "a") as f:
                f.write(part2)
            with LZMAFile(TESTFN, "a") as f:
                f.write(part3)
            with open(TESTFN, "rb") as f:
                self.assertEqual(f.read(), expected)
        finally:
            unlink(TESTFN)

    def test_write_bad_args(self):
        f = LZMAFile(BytesIO(), "w")
        f.close()
        self.assertRaises(ValueError, f.write, b"foo")
        with LZMAFile(BytesIO(COMPRESSED_XZ), "r") as f:
            self.assertRaises(ValueError, f.write, b"bar")
        with LZMAFile(BytesIO(), "w") as f:
            self.assertRaises(TypeError, f.write, None)
            self.assertRaises(TypeError, f.write, "text")
            self.assertRaises(TypeError, f.write, 789)

    def test_writelines(self):
        with BytesIO(INPUT) as f:
            lines = f.readlines()
        with BytesIO() as dst:
            with LZMAFile(dst, "w") as f:
                f.writelines(lines)
            expected = lzma.compress(INPUT)
            self.assertEqual(dst.getvalue(), expected)

    def test_seek_forward(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            f.seek(555)
            self.assertEqual(f.read(), INPUT[555:])

    def test_seek_forward_across_streams(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ * 2)) as f:
            f.seek(len(INPUT) + 123)
            self.assertEqual(f.read(), INPUT[123:])

    def test_seek_forward_relative_to_current(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            f.read(100)
            f.seek(1236, 1)
            self.assertEqual(f.read(), INPUT[1336:])

    def test_seek_forward_relative_to_end(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            f.seek(-555, 2)
            self.assertEqual(f.read(), INPUT[-555:])

    def test_seek_backward(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            f.read(1001)
            f.seek(211)
            self.assertEqual(f.read(), INPUT[211:])

    def test_seek_backward_across_streams(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ * 2)) as f:
            f.read(len(INPUT) + 333)
            f.seek(737)
            self.assertEqual(f.read(), INPUT[737:] + INPUT)

    def test_seek_backward_relative_to_end(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            f.seek(-150, 2)
            self.assertEqual(f.read(), INPUT[-150:])

    def test_seek_past_end(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            f.seek(len(INPUT) + 9001)
            self.assertEqual(f.tell(), len(INPUT))
            self.assertEqual(f.read(), b"")

    def test_seek_past_start(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            f.seek(-88)
            self.assertEqual(f.tell(), 0)
            self.assertEqual(f.read(), INPUT)

    def test_seek_bad_args(self):
        f = LZMAFile(BytesIO(COMPRESSED_XZ))
        f.close()
        self.assertRaises(ValueError, f.seek, 0)
        with LZMAFile(BytesIO(), "w") as f:
            self.assertRaises(ValueError, f.seek, 0)
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            self.assertRaises(ValueError, f.seek, 0, 3)
            # io.BufferedReader raises TypeError instead of ValueError
            self.assertRaises((TypeError, ValueError), f.seek, 9, ())
            self.assertRaises(TypeError, f.seek, None)
            self.assertRaises(TypeError, f.seek, b"derp")

    def test_tell(self):
        with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            pos = 0
            while True:
                self.assertEqual(f.tell(), pos)
                result = f.read(183)
                if not result:
                    break
                pos += len(result)
            self.assertEqual(f.tell(), len(INPUT))
        with LZMAFile(BytesIO(), "w") as f:
            for pos in range(0, len(INPUT), 144):
                self.assertEqual(f.tell(), pos)
                f.write(INPUT[pos:pos+144])
            self.assertEqual(f.tell(), len(INPUT))

    def test_tell_bad_args(self):
        f = LZMAFile(BytesIO(COMPRESSED_XZ))
        f.close()
        self.assertRaises(ValueError, f.tell)


class OpenTestCase(unittest.TestCase):

    def test_binary_modes(self):
        with lzma.open(BytesIO(COMPRESSED_XZ), "rb") as f:
            self.assertEqual(f.read(), INPUT)
        with BytesIO() as bio:
            with lzma.open(bio, "wb") as f:
                f.write(INPUT)
            file_data = lzma.decompress(bio.getvalue())
            self.assertEqual(file_data, INPUT)
            with lzma.open(bio, "ab") as f:
                f.write(INPUT)
            file_data = lzma.decompress(bio.getvalue())
            self.assertEqual(file_data, INPUT * 2)

    def test_text_modes(self):
        uncompressed = INPUT.decode("ascii")
        uncompressed_raw = uncompressed.replace("\n", os.linesep)
        with lzma.open(BytesIO(COMPRESSED_XZ), "rt") as f:
            self.assertEqual(f.read(), uncompressed)
        with BytesIO() as bio:
            with lzma.open(bio, "wt") as f:
                f.write(uncompressed)
            file_data = lzma.decompress(bio.getvalue()).decode("ascii")
            self.assertEqual(file_data, uncompressed_raw)
            with lzma.open(bio, "at") as f:
                f.write(uncompressed)
            file_data = lzma.decompress(bio.getvalue()).decode("ascii")
            self.assertEqual(file_data, uncompressed_raw * 2)

    def test_filename(self):
        with TempFile(TESTFN):
            with lzma.open(TESTFN, "wb") as f:
                f.write(INPUT)
            with open(TESTFN, "rb") as f:
                file_data = lzma.decompress(f.read())
                self.assertEqual(file_data, INPUT)
            with lzma.open(TESTFN, "rb") as f:
                self.assertEqual(f.read(), INPUT)
            with lzma.open(TESTFN, "ab") as f:
                f.write(INPUT)
            with lzma.open(TESTFN, "rb") as f:
                self.assertEqual(f.read(), INPUT * 2)

    def test_bad_params(self):
        # Test invalid parameter combinations.
        with self.assertRaises(ValueError):
            lzma.open(TESTFN, "")
        with self.assertRaises(ValueError):
            lzma.open(TESTFN, "rbt")
        with self.assertRaises(ValueError):
            lzma.open(TESTFN, "rb", encoding="utf-8")
        with self.assertRaises(ValueError):
            lzma.open(TESTFN, "rb", errors="ignore")
        with self.assertRaises(ValueError):
            lzma.open(TESTFN, "rb", newline="\n")

    def test_format_and_filters(self):
        # Test non-default format and filter chain.
        options = {"format": lzma.FORMAT_RAW, "filters": FILTERS_RAW_1}
        with lzma.open(BytesIO(COMPRESSED_RAW_1), "rb", **options) as f:
            self.assertEqual(f.read(), INPUT)
        with BytesIO() as bio:
            with lzma.open(bio, "wb", **options) as f:
                f.write(INPUT)
            file_data = lzma.decompress(bio.getvalue(), **options)
            self.assertEqual(file_data, INPUT)

    def test_encoding(self):
        # Test non-default encoding.
        uncompressed = INPUT.decode("ascii")
        uncompressed_raw = uncompressed.replace("\n", os.linesep)
        with BytesIO() as bio:
            with lzma.open(bio, "wt", encoding="utf-16-le") as f:
                f.write(uncompressed)
            file_data = lzma.decompress(bio.getvalue()).decode("utf-16-le")
            self.assertEqual(file_data, uncompressed_raw)
            bio.seek(0)
            with lzma.open(bio, "rt", encoding="utf-16-le") as f:
                self.assertEqual(f.read(), uncompressed)

    def test_encoding_error_handler(self):
        # Test wih non-default encoding error handler.
        with BytesIO(lzma.compress(b"foo\xffbar")) as bio:
            with lzma.open(bio, "rt", encoding="ascii", errors="ignore") as f:
                self.assertEqual(f.read(), "foobar")

    def test_newline(self):
        # Test with explicit newline (universal newline mode disabled).
        text = INPUT.decode("ascii")
        with BytesIO() as bio:
            with lzma.open(bio, "wt", newline="\n") as f:
                f.write(text)
            bio.seek(0)
            with lzma.open(bio, "rt", newline="\r") as f:
                self.assertEqual(f.readlines(), [text])

    def test_x_mode(self):
        self.addCleanup(unlink, TESTFN)
        for mode in ("x", "xb", "xt"):
            unlink(TESTFN)
            with lzma.open(TESTFN, mode):
                pass
            with self.assertRaises(FileExistsError):
                with lzma.open(TESTFN, mode):
                    pass


class MiscellaneousTestCase(unittest.TestCase):

    def test_is_check_supported(self):
        # CHECK_NONE and CHECK_CRC32 should always be supported,
        # regardless of the options liblzma was compiled with.
        self.assertTrue(lzma.is_check_supported(lzma.CHECK_NONE))
        self.assertTrue(lzma.is_check_supported(lzma.CHECK_CRC32))

        # The .xz format spec cannot store check IDs above this value.
        self.assertFalse(lzma.is_check_supported(lzma.CHECK_ID_MAX + 1))

        # This value should not be a valid check ID.
        self.assertFalse(lzma.is_check_supported(lzma.CHECK_UNKNOWN))

    def test__encode_filter_properties(self):
        with self.assertRaises(TypeError):
            lzma._encode_filter_properties(b"not a dict")
        with self.assertRaises(ValueError):
            lzma._encode_filter_properties({"id": 0x100})
        with self.assertRaises(ValueError):
            lzma._encode_filter_properties({"id": lzma.FILTER_LZMA2, "junk": 12})
        with self.assertRaises(lzma.LZMAError):
            lzma._encode_filter_properties({"id": lzma.FILTER_DELTA,
                                           "dist": 9001})

        # Test with parameters used by zipfile module.
        props = lzma._encode_filter_properties({
                "id": lzma.FILTER_LZMA1,
                "pb": 2,
                "lp": 0,
                "lc": 3,
                "dict_size": 8 << 20,
            })
        self.assertEqual(props, b"]\x00\x00\x80\x00")

    def test__decode_filter_properties(self):
        with self.assertRaises(TypeError):
            lzma._decode_filter_properties(lzma.FILTER_X86, {"should be": bytes})
        with self.assertRaises(lzma.LZMAError):
            lzma._decode_filter_properties(lzma.FILTER_DELTA, b"too long")

        # Test with parameters used by zipfile module.
        filterspec = lzma._decode_filter_properties(
                lzma.FILTER_LZMA1, b"]\x00\x00\x80\x00")
        self.assertEqual(filterspec["id"], lzma.FILTER_LZMA1)
        self.assertEqual(filterspec["pb"], 2)
        self.assertEqual(filterspec["lp"], 0)
        self.assertEqual(filterspec["lc"], 3)
        self.assertEqual(filterspec["dict_size"], 8 << 20)

    def test_filter_properties_roundtrip(self):
        spec1 = lzma._decode_filter_properties(
                lzma.FILTER_LZMA1, b"]\x00\x00\x80\x00")
        reencoded = lzma._encode_filter_properties(spec1)
        spec2 = lzma._decode_filter_properties(lzma.FILTER_LZMA1, reencoded)
        self.assertEqual(spec1, spec2)


# Test data:

INPUT = b"""
LAERTES

       O, fear me not.
       I stay too long: but here my father comes.

       Enter POLONIUS

       A double blessing is a double grace,
       Occasion smiles upon a second leave.

LORD POLONIUS

       Yet here, Laertes! aboard, aboard, for shame!
       The wind sits in the shoulder of your sail,
       And you are stay'd for. There; my blessing with thee!
       And these few precepts in thy memory
       See thou character. Give thy thoughts no tongue,
       Nor any unproportioned thought his act.
       Be thou familiar, but by no means vulgar.
       Those friends thou hast, and their adoption tried,
       Grapple them to thy soul with hoops of steel;
       But do not dull thy palm with entertainment
       Of each new-hatch'd, unfledged comrade. Beware
       Of entrance to a quarrel, but being in,
       Bear't that the opposed may beware of thee.
       Give every man thy ear, but few thy voice;
       Take each man's censure, but reserve thy judgment.
       Costly thy habit as thy purse can buy,
       But not express'd in fancy; rich, not gaudy;
       For the apparel oft proclaims the man,
       And they in France of the best rank and station
       Are of a most select and generous chief in that.
       Neither a borrower nor a lender be;
       For loan oft loses both itself and friend,
       And borrowing dulls the edge of husbandry.
       This above all: to thine ownself be true,
       And it must follow, as the night the day,
       Thou canst not then be false to any man.
       Farewell: my blessing season this in thee!

LAERTES

       Most humbly do I take my leave, my lord.

LORD POLONIUS

       The time invites you; go; your servants tend.

LAERTES

       Farewell, Ophelia; and remember well
       What I have said to you.

OPHELIA

       'Tis in my memory lock'd,
       And you yourself shall keep the key of it.

LAERTES

       Farewell.
"""

COMPRESSED_BOGUS = b"this is not a valid lzma stream"

COMPRESSED_XZ = (
    b"\xfd7zXZ\x00\x00\x04\xe6\xd6\xb4F\x02\x00!\x01\x16\x00\x00\x00t/\xe5\xa3"
    b"\xe0\x07\x80\x03\xdf]\x00\x05\x14\x07bX\x19\xcd\xddn\x98\x15\xe4\xb4\x9d"
    b"o\x1d\xc4\xe5\n\x03\xcc2h\xc7\\\x86\xff\xf8\xe2\xfc\xe7\xd9\xfe6\xb8("
    b"\xa8wd\xc2\"u.n\x1e\xc3\xf2\x8e\x8d\x8f\x02\x17/\xa6=\xf0\xa2\xdf/M\x89"
    b"\xbe\xde\xa7\x1cz\x18-]\xd5\xef\x13\x8frZ\x15\x80\x8c\xf8\x8do\xfa\x12"
    b"\x9b#z/\xef\xf0\xfaF\x01\x82\xa3M\x8e\xa1t\xca6 BF$\xe5Q\xa4\x98\xee\xde"
    b"l\xe8\x7f\xf0\x9d,bn\x0b\x13\xd4\xa8\x81\xe4N\xc8\x86\x153\xf5x2\xa2O"
    b"\x13@Q\xa1\x00/\xa5\xd0O\x97\xdco\xae\xf7z\xc4\xcdS\xb6t<\x16\xf2\x9cI#"
    b"\x89ud\xc66Y\xd9\xee\xe6\xce\x12]\xe5\xf0\xaa\x96-Pe\xade:\x04\t\x1b\xf7"
    b"\xdb7\n\x86\x1fp\xc8J\xba\xf4\xf0V\xa9\xdc\xf0\x02%G\xf9\xdf=?\x15\x1b"
    b"\xe1(\xce\x82=\xd6I\xac3\x12\x0cR\xb7\xae\r\xb1i\x03\x95\x01\xbd\xbe\xfa"
    b"\x02s\x01P\x9d\x96X\xb12j\xc8L\xa8\x84b\xf6\xc3\xd4c-H\x93oJl\xd0iQ\xe4k"
    b"\x84\x0b\xc1\xb7\xbc\xb1\x17\x88\xb1\xca?@\xf6\x07\xea\xe6x\xf1H12P\x0f"
    b"\x8a\xc9\xeauw\xe3\xbe\xaai\xa9W\xd0\x80\xcd#cb5\x99\xd8]\xa9d\x0c\xbd"
    b"\xa2\xdcWl\xedUG\xbf\x89yF\xf77\x81v\xbd5\x98\xbeh8\x18W\x08\xf0\x1b\x99"
    b"5:\x1a?rD\x96\xa1\x04\x0f\xae\xba\x85\xeb\x9d5@\xf5\x83\xd37\x83\x8ac"
    b"\x06\xd4\x97i\xcdt\x16S\x82k\xf6K\x01vy\x88\x91\x9b6T\xdae\r\xfd]:k\xbal"
    b"\xa9\xbba\xc34\xf9r\xeb}r\xdb\xc7\xdb*\x8f\x03z\xdc8h\xcc\xc9\xd3\xbcl"
    b"\xa5-\xcb\xeaK\xa2\xc5\x15\xc0\xe3\xc1\x86Z\xfb\xebL\xe13\xcf\x9c\xe3"
    b"\x1d\xc9\xed\xc2\x06\xcc\xce!\x92\xe5\xfe\x9c^\xa59w \x9bP\xa3PK\x08d"
    b"\xf9\xe2Z}\xa7\xbf\xed\xeb%$\x0c\x82\xb8/\xb0\x01\xa9&,\xf7qh{Q\x96)\xf2"
    b"q\x96\xc3\x80\xb4\x12\xb0\xba\xe6o\xf4!\xb4[\xd4\x8aw\x10\xf7t\x0c\xb3"
    b"\xd9\xd5\xc3`^\x81\x11??\\\xa4\x99\x85R\xd4\x8e\x83\xc9\x1eX\xbfa\xf1"
    b"\xac\xb0\xea\xea\xd7\xd0\xab\x18\xe2\xf2\xed\xe1\xb7\xc9\x18\xcbS\xe4>"
    b"\xc9\x95H\xe8\xcb\t\r%\xeb\xc7$.o\xf1\xf3R\x17\x1db\xbb\xd8U\xa5^\xccS"
    b"\x16\x01\x87\xf3/\x93\xd1\xf0v\xc0r\xd7\xcc\xa2Gkz\xca\x80\x0e\xfd\xd0"
    b"\x8b\xbb\xd2Ix\xb3\x1ey\xca-0\xe3z^\xd6\xd6\x8f_\xf1\x9dP\x9fi\xa7\xd1"
    b"\xe8\x90\x84\xdc\xbf\xcdky\x8e\xdc\x81\x7f\xa3\xb2+\xbf\x04\xef\xd8\\"
    b"\xc4\xdf\xe1\xb0\x01\xe9\x93\xe3Y\xf1\x1dY\xe8h\x81\xcf\xf1w\xcc\xb4\xef"
    b" \x8b|\x04\xea\x83ej\xbe\x1f\xd4z\x9c`\xd3\x1a\x92A\x06\xe5\x8f\xa9\x13"
    b"\t\x9e=\xfa\x1c\xe5_\x9f%v\x1bo\x11ZO\xd8\xf4\t\xddM\x16-\x04\xfc\x18<\""
    b"CM\xddg~b\xf6\xef\x8e\x0c\xd0\xde|\xa0'\x8a\x0c\xd6x\xae!J\xa6F\x88\x15u"
    b"\x008\x17\xbc7y\xb3\xd8u\xac_\x85\x8d\xe7\xc1@\x9c\xecqc\xa3#\xad\xf1"
    b"\x935\xb5)_\r\xec3]\x0fo]5\xd0my\x07\x9b\xee\x81\xb5\x0f\xcfK+\x00\xc0"
    b"\xe4b\x10\xe4\x0c\x1a \x9b\xe0\x97t\xf6\xa1\x9e\x850\xba\x0c\x9a\x8d\xc8"
    b"\x8f\x07\xd7\xae\xc8\xf9+i\xdc\xb9k\xb0>f\x19\xb8\r\xa8\xf8\x1f$\xa5{p"
    b"\xc6\x880\xce\xdb\xcf\xca_\x86\xac\x88h6\x8bZ%'\xd0\n\xbf\x0f\x9c\"\xba"
    b"\xe5\x86\x9f\x0f7X=mNX[\xcc\x19FU\xc9\x860\xbc\x90a+* \xae_$\x03\x1e\xd3"
    b"\xcd_\xa0\x9c\xde\xaf46q\xa5\xc9\x92\xd7\xca\xe3`\x9d\x85}\xb4\xff\xb3"
    b"\x83\xfb\xb6\xca\xae`\x0bw\x7f\xfc\xd8\xacVe\x19\xc8\x17\x0bZ\xad\x88"
    b"\xeb#\x97\x03\x13\xb1d\x0f{\x0c\x04w\x07\r\x97\xbd\xd6\xc1\xc3B:\x95\x08"
    b"^\x10V\xaeaH\x02\xd9\xe3\n\\\x01X\xf6\x9c\x8a\x06u#%\xbe*\xa1\x18v\x85"
    b"\xec!\t4\x00\x00\x00\x00Vj?uLU\xf3\xa6\x00\x01\xfb\x07\x81\x0f\x00\x00tw"
    b"\x99P\xb1\xc4g\xfb\x02\x00\x00\x00\x00\x04YZ"
)

COMPRESSED_ALONE = (
    b"]\x00\x00\x80\x00\xff\xff\xff\xff\xff\xff\xff\xff\x00\x05\x14\x07bX\x19"
    b"\xcd\xddn\x98\x15\xe4\xb4\x9do\x1d\xc4\xe5\n\x03\xcc2h\xc7\\\x86\xff\xf8"
    b"\xe2\xfc\xe7\xd9\xfe6\xb8(\xa8wd\xc2\"u.n\x1e\xc3\xf2\x8e\x8d\x8f\x02"
    b"\x17/\xa6=\xf0\xa2\xdf/M\x89\xbe\xde\xa7\x1cz\x18-]\xd5\xef\x13\x8frZ"
    b"\x15\x80\x8c\xf8\x8do\xfa\x12\x9b#z/\xef\xf0\xfaF\x01\x82\xa3M\x8e\xa1t"
    b"\xca6 BF$\xe5Q\xa4\x98\xee\xdel\xe8\x7f\xf0\x9d,bn\x0b\x13\xd4\xa8\x81"
    b"\xe4N\xc8\x86\x153\xf5x2\xa2O\x13@Q\xa1\x00/\xa5\xd0O\x97\xdco\xae\xf7z"
    b"\xc4\xcdS\xb6t<\x16\xf2\x9cI#\x89ud\xc66Y\xd9\xee\xe6\xce\x12]\xe5\xf0"
    b"\xaa\x96-Pe\xade:\x04\t\x1b\xf7\xdb7\n\x86\x1fp\xc8J\xba\xf4\xf0V\xa9"
    b"\xdc\xf0\x02%G\xf9\xdf=?\x15\x1b\xe1(\xce\x82=\xd6I\xac3\x12\x0cR\xb7"
    b"\xae\r\xb1i\x03\x95\x01\xbd\xbe\xfa\x02s\x01P\x9d\x96X\xb12j\xc8L\xa8"
    b"\x84b\xf8\x1epl\xeajr\xd1=\t\x03\xdd\x13\x1b3!E\xf9vV\xdaF\xf3\xd7\xb4"
    b"\x0c\xa9P~\xec\xdeE\xe37\xf6\x1d\xc6\xbb\xddc%\xb6\x0fI\x07\xf0;\xaf\xe7"
    b"\xa0\x8b\xa7Z\x99(\xe9\xe2\xf0o\x18>`\xe1\xaa\xa8\xd9\xa1\xb2}\xe7\x8d"
    b"\x834T\xb6\xef\xc1\xde\xe3\x98\xbcD\x03MA@\xd8\xed\xdc\xc8\x93\x03\x1a"
    b"\x93\x0b\x7f\x94\x12\x0b\x02Sa\x18\xc9\xc5\x9bTJE}\xf6\xc8g\x17#ZV\x01"
    b"\xc9\x9dc\x83\x0e>0\x16\x90S\xb8/\x03y_\x18\xfa(\xd7\x0br\xa2\xb0\xba?"
    b"\x8c\xe6\x83@\x84\xdf\x02:\xc5z\x9e\xa6\x84\xc9\xf5BeyX\x83\x1a\xf1 :\t"
    b"\xf7\x19\xfexD\\&G\xf3\x85Y\xa2J\xf9\x0bv{\x89\xf6\xe7)A\xaf\x04o\x00"
    b"\x075\xd3\xe0\x7f\x97\x98F\x0f?v\x93\xedVtTf\xb5\x97\x83\xed\x19\xd7\x1a"
    b"'k\xd7\xd9\xc5\\Y\xd1\xdc\x07\x15|w\xbc\xacd\x87\x08d\xec\xa7\xf6\x82"
    b"\xfc\xb3\x93\xeb\xb9 \x8d\xbc ,\xb3X\xb0\xd2s\xd7\xd1\xffv\x05\xdf}\xa2"
    b"\x96\xfb%\n\xdf\xa2\x7f\x08.\xa16\n\xe0\x19\x93\x7fh\n\x1c\x8c\x0f \x11"
    b"\xc6Bl\x95\x19U}\xe4s\xb5\x10H\xea\x86pB\xe88\x95\xbe\x8cZ\xdb\xe4\x94A"
    b"\x92\xb9;z\xaa\xa7{\x1c5!\xc0\xaf\xc1A\xf9\xda\xf0$\xb0\x02qg\xc8\xc7/|"
    b"\xafr\x99^\x91\x88\xbf\x03\xd9=\xd7n\xda6{>8\n\xc7:\xa9'\xba.\x0b\xe2"
    b"\xb5\x1d\x0e\n\x9a\x8e\x06\x8f:\xdd\x82'[\xc3\"wD$\xa7w\xecq\x8c,1\x93"
    b"\xd0,\xae2w\x93\x12$Jd\x19mg\x02\x93\x9cA\x95\x9d&\xca8i\x9c\xb0;\xe7NQ"
    b"\x1frh\x8beL;\xb0m\xee\x07Q\x9b\xc6\xd8\x03\xb5\xdeN\xd4\xfe\x98\xd0\xdc"
    b"\x1a[\x04\xde\x1a\xf6\x91j\xf8EOli\x8eB^\x1d\x82\x07\xb2\xb5R]\xb7\xd7"
    b"\xe9\xa6\xc3.\xfb\xf0-\xb4e\x9b\xde\x03\x88\xc6\xc1iN\x0e\x84wbQ\xdf~"
    b"\xe9\xa4\x884\x96kM\xbc)T\xf3\x89\x97\x0f\x143\xe7)\xa0\xb3B\x00\xa8\xaf"
    b"\x82^\xcb\xc7..\xdb\xc7\t\x9dH\xee5\xe9#\xe6NV\x94\xcb$Kk\xe3\x7f\r\xe3t"
    b"\x12\xcf'\xefR\x8b\xf42\xcf-LH\xac\xe5\x1f0~?SO\xeb\xc1E\x1a\x1c]\xf2"
    b"\xc4<\x11\x02\x10Z0a*?\xe4r\xff\xfb\xff\xf6\x14nG\xead^\xd6\xef8\xb6uEI"
    b"\x99\nV\xe2\xb3\x95\x8e\x83\xf6i!\xb5&1F\xb1DP\xf4 SO3D!w\x99_G\x7f+\x90"
    b".\xab\xbb]\x91>\xc9#h;\x0f5J\x91K\xf4^-[\x9e\x8a\\\x94\xca\xaf\xf6\x19"
    b"\xd4\xa1\x9b\xc4\xb8p\xa1\xae\x15\xe9r\x84\xe0\xcar.l []\x8b\xaf+0\xf2g"
    b"\x01aKY\xdfI\xcf,\n\xe8\xf0\xe7V\x80_#\xb2\xf2\xa9\x06\x8c>w\xe2W,\xf4"
    b"\x8c\r\xf963\xf5J\xcc2\x05=kT\xeaUti\xe5_\xce\x1b\xfa\x8dl\x02h\xef\xa8"
    b"\xfbf\x7f\xff\xf0\x19\xeax"
)

FILTERS_RAW_1 = [{"id": lzma.FILTER_LZMA2, "preset": 3}]
COMPRESSED_RAW_1 = (
    b"\xe0\x07\x80\x03\xfd]\x00\x05\x14\x07bX\x19\xcd\xddn\x96cyq\xa1\xdd\xee"
    b"\xf8\xfam\xe3'\x88\xd3\xff\xe4\x9e \xceQ\x91\xa4\x14I\xf6\xb9\x9dVL8\x15"
    b"_\x0e\x12\xc3\xeb\xbc\xa5\xcd\nW\x1d$=R;\x1d\xf8k8\t\xb1{\xd4\xc5+\x9d"
    b"\x87c\xe5\xef\x98\xb4\xd7S3\xcd\xcc\xd2\xed\xa4\x0em\xe5\xf4\xdd\xd0b"
    b"\xbe4*\xaa\x0b\xc5\x08\x10\x85+\x81.\x17\xaf9\xc9b\xeaZrA\xe20\x7fs\"r"
    b"\xdaG\x81\xde\x90cu\xa5\xdb\xa9.A\x08l\xb0<\xf6\x03\xddOi\xd0\xc5\xb4"
    b"\xec\xecg4t6\"\xa6\xb8o\xb5?\x18^}\xb6}\x03[:\xeb\x03\xa9\n[\x89l\x19g"
    b"\x16\xc82\xed\x0b\xfb\x86n\xa2\x857@\x93\xcd6T\xc3u\xb0\t\xf9\x1b\x918"
    b"\xfc[\x1b\x1e4\xb3\x14\x06PCV\xa8\"\xf5\x81x~\xe9\xb5N\x9cK\x9f\xc6\xc3%"
    b"\xc8k:{6\xe7\xf7\xbd\x05\x02\xb4\xc4\xc3\xd3\xfd\xc3\xa8\\\xfc@\xb1F_"
    b"\xc8\x90\xd9sU\x98\xad8\x05\x07\xde7J\x8bM\xd0\xb3;X\xec\x87\xef\xae\xb3"
    b"eO,\xb1z,d\x11y\xeejlB\x02\x1d\xf28\x1f#\x896\xce\x0b\xf0\xf5\xa9PK\x0f"
    b"\xb3\x13P\xd8\x88\xd2\xa1\x08\x04C?\xdb\x94_\x9a\"\xe9\xe3e\x1d\xde\x9b"
    b"\xa1\xe8>H\x98\x10;\xc5\x03#\xb5\x9d4\x01\xe7\xc5\xba%v\xa49\x97A\xe0\""
    b"\x8c\xc22\xe3i\xc1\x9d\xab3\xdf\xbe\xfdDm7\x1b\x9d\xab\xb5\x15o:J\x92"
    b"\xdb\x816\x17\xc2O\x99\x1b\x0e\x8d\xf3\tQ\xed\x8e\x95S/\x16M\xb2S\x04"
    b"\x0f\xc3J\xc6\xc7\xe4\xcb\xc5\xf4\xe7d\x14\xe4=^B\xfb\xd3E\xd3\x1e\xcd"
    b"\x91\xa5\xd0G\x8f.\xf6\xf9\x0bb&\xd9\x9f\xc2\xfdj\xa2\x9e\xc4\\\x0e\x1dC"
    b"v\xe8\xd2\x8a?^H\xec\xae\xeb>\xfe\xb8\xab\xd4IqY\x8c\xd4K7\x11\xf4D\xd0W"
    b"\xa5\xbe\xeaO\xbf\xd0\x04\xfdl\x10\xae5\xd4U\x19\x06\xf9{\xaa\xe0\x81"
    b"\x0f\xcf\xa3k{\x95\xbd\x19\xa2\xf8\xe4\xa3\x08O*\xf1\xf1B-\xc7(\x0eR\xfd"
    b"@E\x9f\xd3\x1e:\xfdV\xb7\x04Y\x94\xeb]\x83\xc4\xa5\xd7\xc0gX\x98\xcf\x0f"
    b"\xcd3\x00]n\x17\xec\xbd\xa3Y\x86\xc5\xf3u\xf6*\xbdT\xedA$A\xd9A\xe7\x98"
    b"\xef\x14\x02\x9a\xfdiw\xec\xa0\x87\x11\xd9%\xc5\xeb\x8a=\xae\xc0\xc4\xc6"
    b"D\x80\x8f\xa8\xd1\xbbq\xb2\xc0\xa0\xf5Cqp\xeeL\xe3\xe5\xdc \x84\"\xe9"
    b"\x80t\x83\x05\xba\xf1\xc5~\x93\xc9\xf0\x01c\xceix\x9d\xed\xc5)l\x16)\xd1"
    b"\x03@l\x04\x7f\x87\xa5yn\x1b\x01D\xaa:\xd2\x96\xb4\xb3?\xb0\xf9\xce\x07"
    b"\xeb\x81\x00\xe4\xc3\xf5%_\xae\xd4\xf9\xeb\xe2\rh\xb2#\xd67Q\x16D\x82hn"
    b"\xd1\xa3_?q\xf0\xe2\xac\xf317\x9e\xd0_\x83|\xf1\xca\xb7\x95S\xabW\x12"
    b"\xff\xddt\xf69L\x01\xf2|\xdaW\xda\xees\x98L\x18\xb8_\xe8$\x82\xea\xd6"
    b"\xd1F\xd4\x0b\xcdk\x01vf\x88h\xc3\xae\xb91\xc7Q\x9f\xa5G\xd9\xcc\x1f\xe3"
    b"5\xb1\xdcy\x7fI\x8bcw\x8e\x10rIp\x02:\x19p_\xc8v\xcea\"\xc1\xd9\x91\x03"
    b"\xbfe\xbe\xa6\xb3\xa8\x14\x18\xc3\xabH*m}\xc2\xc1\x9a}>l%\xce\x84\x99"
    b"\xb3d\xaf\xd3\x82\x15\xdf\xc1\xfc5fOg\x9b\xfc\x8e^&\t@\xce\x9f\x06J\xb8"
    b"\xb5\x86\x1d\xda{\x9f\xae\xb0\xff\x02\x81r\x92z\x8cM\xb7ho\xc9^\x9c\xb6"
    b"\x9c\xae\xd1\xc9\xf4\xdfU7\xd6\\!\xea\x0b\x94k\xb9Ud~\x98\xe7\x86\x8az"
    b"\x10;\xe3\x1d\xe5PG\xf8\xa4\x12\x05w\x98^\xc4\xb1\xbb\xfb\xcf\xe0\x7f"
    b"\x033Sf\x0c \xb1\xf6@\x94\xe5\xa3\xb2\xa7\x10\x9a\xc0\x14\xc3s\xb5xRD"
    b"\xf4`W\xd9\xe5\xd3\xcf\x91\rTZ-X\xbe\xbf\xb5\xe2\xee|\x1a\xbf\xfb\x08"
    b"\x91\xe1\xfc\x9a\x18\xa3\x8b\xd6^\x89\xf5[\xef\x87\xd1\x06\x1c7\xd6\xa2"
    b"\t\tQ5/@S\xc05\xd2VhAK\x03VC\r\x9b\x93\xd6M\xf1xO\xaaO\xed\xb9<\x0c\xdae"
    b"*\xd0\x07Hk6\x9fG+\xa1)\xcd\x9cl\x87\xdb\xe1\xe7\xefK}\x875\xab\xa0\x19u"
    b"\xf6*F\xb32\x00\x00\x00"
)

FILTERS_RAW_2 = [{"id": lzma.FILTER_DELTA, "dist": 2},
                 {"id": lzma.FILTER_LZMA2,
                  "preset": lzma.PRESET_DEFAULT | lzma.PRESET_EXTREME}]
COMPRESSED_RAW_2 = (
    b"\xe0\x07\x80\x05\x91]\x00\x05\x14\x06-\xd4\xa8d?\xef\xbe\xafH\xee\x042"
    b"\xcb.\xb5g\x8f\xfb\x14\xab\xa5\x9f\x025z\xa4\xdd\xd8\t[}W\xf8\x0c\x1dmH"
    b"\xfa\x05\xfcg\xba\xe5\x01Q\x0b\x83R\xb6A\x885\xc0\xba\xee\n\x1cv~\xde:o"
    b"\x06:J\xa7\x11Cc\xea\xf7\xe5*o\xf7\x83\\l\xbdE\x19\x1f\r\xa8\x10\xb42"
    b"\x0caU{\xd7\xb8w\xdc\xbe\x1b\xfc8\xb4\xcc\xd38\\\xf6\x13\xf6\xe7\x98\xfa"
    b"\xc7[\x17_9\x86%\xa8\xf8\xaa\xb8\x8dfs#\x1e=\xed<\x92\x10\\t\xff\x86\xfb"
    b"=\x9e7\x18\x1dft\\\xb5\x01\x95Q\xc5\x19\xb38\xe0\xd4\xaa\x07\xc3\x7f\xd8"
    b"\xa2\x00>-\xd3\x8e\xa1#\xfa\x83ArAm\xdbJ~\x93\xa3B\x82\xe0\xc7\xcc(\x08`"
    b"WK\xad\x1b\x94kaj\x04 \xde\xfc\xe1\xed\xb0\x82\x91\xefS\x84%\x86\xfbi"
    b"\x99X\xf1B\xe7\x90;E\xfde\x98\xda\xca\xd6T\xb4bg\xa4\n\x9aj\xd1\x83\x9e]"
    b"\"\x7fM\xb5\x0fr\xd2\\\xa5j~P\x10GH\xbfN*Z\x10.\x81\tpE\x8a\x08\xbe1\xbd"
    b"\xcd\xa9\xe1\x8d\x1f\x04\xf9\x0eH\xb9\xae\xd6\xc3\xc1\xa5\xa9\x95P\xdc~"
    b"\xff\x01\x930\xa9\x04\xf6\x03\xfe\xb5JK\xc3]\xdd9\xb1\xd3\xd7F\xf5\xd1"
    b"\x1e\xa0\x1c_\xed[\x0c\xae\xd4\x8b\x946\xeb\xbf\xbb\xe3$kS{\xb5\x80,f:Sj"
    b"\x0f\x08z\x1c\xf5\xe8\xe6\xae\x98\xb0Q~r\x0f\xb0\x05?\xb6\x90\x19\x02&"
    b"\xcb\x80\t\xc4\xea\x9c|x\xce\x10\x9c\xc5|\xcbdhh+\x0c'\xc5\x81\xc33\xb5"
    b"\x14q\xd6\xc5\xe3`Z#\xdc\x8a\xab\xdd\xea\x08\xc2I\xe7\x02l{\xec\x196\x06"
    b"\x91\x8d\xdc\xd5\xb3x\xe1hz%\xd1\xf8\xa5\xdd\x98!\x8c\x1c\xc1\x17RUa\xbb"
    b"\x95\x0f\xe4X\xea1\x0c\xf1=R\xbe\xc60\xe3\xa4\x9a\x90bd\x97$]B\x01\xdd"
    b"\x1f\xe3h2c\x1e\xa0L`4\xc6x\xa3Z\x8a\r\x14]T^\xd8\x89\x1b\x92\r;\xedY"
    b"\x0c\xef\x8d9z\xf3o\xb6)f\xa9]$n\rp\x93\xd0\x10\xa4\x08\xb8\xb2\x8b\xb6"
    b"\x8f\x80\xae;\xdcQ\xf1\xfa\x9a\x06\x8e\xa5\x0e\x8cK\x9c @\xaa:UcX\n!\xc6"
    b"\x02\x12\xcb\x1b\"=\x16.\x1f\x176\xf2g=\xe1Wn\xe9\xe1\xd4\xf1O\xad\x15"
    b"\x86\xe9\xa3T\xaf\xa9\xd7D\xb5\xd1W3pnt\x11\xc7VOj\xb7M\xc4i\xa1\xf1$3"
    b"\xbb\xdc\x8af\xb0\xc5Y\r\xd1\xfb\xf2\xe7K\xe6\xc5hwO\xfe\x8c2^&\x07\xd5"
    b"\x1fV\x19\xfd\r\x14\xd2i=yZ\xe6o\xaf\xc6\xb6\x92\x9d\xc4\r\xb3\xafw\xac%"
    b"\xcfc\x1a\xf1`]\xf2\x1a\x9e\x808\xedm\xedQ\xb2\xfe\xe4h`[q\xae\xe0\x0f"
    b"\xba0g\xb6\"N\xc3\xfb\xcfR\x11\xc5\x18)(\xc40\\\xa3\x02\xd9G!\xce\x1b"
    b"\xc1\x96x\xb5\xc8z\x1f\x01\xb4\xaf\xde\xc2\xcd\x07\xe7H\xb3y\xa8M\n\\A\t"
    b"ar\xddM\x8b\x9a\xea\x84\x9b!\xf1\x8d\xb1\xf1~\x1e\r\xa5H\xba\xf1\x84o"
    b"\xda\x87\x01h\xe9\xa2\xbe\xbeqN\x9d\x84\x0b!WG\xda\xa1\xa5A\xb7\xc7`j"
    b"\x15\xf2\xe9\xdd?\x015B\xd2~E\x06\x11\xe0\x91!\x05^\x80\xdd\xa8y\x15}"
    b"\xa1)\xb1)\x81\x18\xf4\xf4\xf8\xc0\xefD\xe3\xdb2f\x1e\x12\xabu\xc9\x97"
    b"\xcd\x1e\xa7\x0c\x02x4_6\x03\xc4$t\xf39\x94\x1d=\xcb\xbfv\\\xf5\xa3\x1d"
    b"\x9d8jk\x95\x13)ff\xf9n\xc4\xa9\xe3\x01\xb8\xda\xfb\xab\xdfM\x99\xfb\x05"
    b"\xe0\xe9\xb0I\xf4E\xab\xe2\x15\xa3\x035\xe7\xdeT\xee\x82p\xb4\x88\xd3"
    b"\x893\x9c/\xc0\xd6\x8fou;\xf6\x95PR\xa9\xb2\xc1\xefFj\xe2\xa7$\xf7h\xf1"
    b"\xdfK(\xc9c\xba7\xe8\xe3)\xdd\xb2,\x83\xfb\x84\x18.y\x18Qi\x88\xf8`h-"
    b"\xef\xd5\xed\x8c\t\xd8\xc3^\x0f\x00\xb7\xd0[!\xafM\x9b\xd7.\x07\xd8\xfb"
    b"\xd9\xe2-S+\xaa8,\xa0\x03\x1b \xea\xa8\x00\xc3\xab~\xd0$e\xa5\x7f\xf7"
    b"\x95P]\x12\x19i\xd9\x7fo\x0c\xd8g^\rE\xa5\x80\x18\xc5\x01\x80\xaek`\xff~"
    b"\xb6y\xe7+\xe5\x11^D\xa7\x85\x18\"!\xd6\xd2\xa7\xf4\x1eT\xdb\x02\xe15"
    b"\x02Y\xbc\x174Z\xe7\x9cH\x1c\xbf\x0f\xc6\xe9f]\xcf\x8cx\xbc\xe5\x15\x94"
    b"\xfc3\xbc\xa7TUH\xf1\x84\x1b\xf7\xa9y\xc07\x84\xf8X\xd8\xef\xfc \x1c\xd8"
    b"( /\xf2\xb7\xec\xc1\\\x8c\xf6\x95\xa1\x03J\x83vP8\xe1\xe3\xbb~\xc24kA"
    b"\x98y\xa1\xf2P\xe9\x9d\xc9J\xf8N\x99\xb4\xceaO\xde\x16\x1e\xc2\x19\xa7"
    b"\x03\xd2\xe0\x8f:\x15\xf3\x84\x9e\xee\xe6e\xb8\x02q\xc7AC\x1emw\xfd\t"
    b"\x9a\x1eu\xc1\xa9\xcaCwUP\x00\xa5\xf78L4w!\x91L2 \x87\xd0\xf2\x06\x81j"
    b"\x80;\x03V\x06\x87\x92\xcb\x90lv@E\x8d\x8d\xa5\xa6\xe7Z[\xdf\xd6E\x03`>"
    b"\x8f\xde\xa1bZ\x84\xd0\xa9`\x05\x0e{\x80;\xe3\xbef\x8d\x1d\xebk1.\xe3"
    b"\xe9N\x15\xf7\xd4(\xfa\xbb\x15\xbdu\xf7\x7f\x86\xae!\x03L\x1d\xb5\xc1"
    b"\xb9\x11\xdb\xd0\x93\xe4\x02\xe1\xd2\xcbBjc_\xe8}d\xdb\xc3\xa0Y\xbe\xc9/"
    b"\x95\x01\xa3,\xe6bl@\x01\xdbp\xc2\xce\x14\x168\xc2q\xe3uH\x89X\xa4\xa9"
    b"\x19\x1d\xc1}\x7fOX\x19\x9f\xdd\xbe\x85\x83\xff\x96\x1ee\x82O`CF=K\xeb$I"
    b"\x17_\xefX\x8bJ'v\xde\x1f+\xd9.v\xf8Tv\x17\xf2\x9f5\x19\xe1\xb9\x91\xa8S"
    b"\x86\xbd\x1a\"(\xa5x\x8dC\x03X\x81\x91\xa8\x11\xc4pS\x13\xbc\xf2'J\xae!"
    b"\xef\xef\x84G\t\x8d\xc4\x10\x132\x00oS\x9e\xe0\xe4d\x8f\xb8y\xac\xa6\x9f"
    b",\xb8f\x87\r\xdf\x9eE\x0f\xe1\xd0\\L\x00\xb2\xe1h\x84\xef}\x98\xa8\x11"
    b"\xccW#\\\x83\x7fo\xbbz\x8f\x00"
)

FILTERS_RAW_3 = [{"id": lzma.FILTER_IA64, "start_offset": 0x100},
                 {"id": lzma.FILTER_LZMA2}]
COMPRESSED_RAW_3 = (
    b"\xe0\x07\x80\x03\xdf]\x00\x05\x14\x07bX\x19\xcd\xddn\x98\x15\xe4\xb4\x9d"
    b"o\x1d\xc4\xe5\n\x03\xcc2h\xc7\\\x86\xff\xf8\xe2\xfc\xe7\xd9\xfe6\xb8("
    b"\xa8wd\xc2\"u.n\x1e\xc3\xf2\x8e\x8d\x8f\x02\x17/\xa6=\xf0\xa2\xdf/M\x89"
    b"\xbe\xde\xa7\x1cz\x18-]\xd5\xef\x13\x8frZ\x15\x80\x8c\xf8\x8do\xfa\x12"
    b"\x9b#z/\xef\xf0\xfaF\x01\x82\xa3M\x8e\xa1t\xca6 BF$\xe5Q\xa4\x98\xee\xde"
    b"l\xe8\x7f\xf0\x9d,bn\x0b\x13\xd4\xa8\x81\xe4N\xc8\x86\x153\xf5x2\xa2O"
    b"\x13@Q\xa1\x00/\xa5\xd0O\x97\xdco\xae\xf7z\xc4\xcdS\xb6t<\x16\xf2\x9cI#"
    b"\x89ud\xc66Y\xd9\xee\xe6\xce\x12]\xe5\xf0\xaa\x96-Pe\xade:\x04\t\x1b\xf7"
    b"\xdb7\n\x86\x1fp\xc8J\xba\xf4\xf0V\xa9\xdc\xf0\x02%G\xf9\xdf=?\x15\x1b"
    b"\xe1(\xce\x82=\xd6I\xac3\x12\x0cR\xb7\xae\r\xb1i\x03\x95\x01\xbd\xbe\xfa"
    b"\x02s\x01P\x9d\x96X\xb12j\xc8L\xa8\x84b\xf6\xc3\xd4c-H\x93oJl\xd0iQ\xe4k"
    b"\x84\x0b\xc1\xb7\xbc\xb1\x17\x88\xb1\xca?@\xf6\x07\xea\xe6x\xf1H12P\x0f"
    b"\x8a\xc9\xeauw\xe3\xbe\xaai\xa9W\xd0\x80\xcd#cb5\x99\xd8]\xa9d\x0c\xbd"
    b"\xa2\xdcWl\xedUG\xbf\x89yF\xf77\x81v\xbd5\x98\xbeh8\x18W\x08\xf0\x1b\x99"
    b"5:\x1a?rD\x96\xa1\x04\x0f\xae\xba\x85\xeb\x9d5@\xf5\x83\xd37\x83\x8ac"
    b"\x06\xd4\x97i\xcdt\x16S\x82k\xf6K\x01vy\x88\x91\x9b6T\xdae\r\xfd]:k\xbal"
    b"\xa9\xbba\xc34\xf9r\xeb}r\xdb\xc7\xdb*\x8f\x03z\xdc8h\xcc\xc9\xd3\xbcl"
    b"\xa5-\xcb\xeaK\xa2\xc5\x15\xc0\xe3\xc1\x86Z\xfb\xebL\xe13\xcf\x9c\xe3"
    b"\x1d\xc9\xed\xc2\x06\xcc\xce!\x92\xe5\xfe\x9c^\xa59w \x9bP\xa3PK\x08d"
    b"\xf9\xe2Z}\xa7\xbf\xed\xeb%$\x0c\x82\xb8/\xb0\x01\xa9&,\xf7qh{Q\x96)\xf2"
    b"q\x96\xc3\x80\xb4\x12\xb0\xba\xe6o\xf4!\xb4[\xd4\x8aw\x10\xf7t\x0c\xb3"
    b"\xd9\xd5\xc3`^\x81\x11??\\\xa4\x99\x85R\xd4\x8e\x83\xc9\x1eX\xbfa\xf1"
    b"\xac\xb0\xea\xea\xd7\xd0\xab\x18\xe2\xf2\xed\xe1\xb7\xc9\x18\xcbS\xe4>"
    b"\xc9\x95H\xe8\xcb\t\r%\xeb\xc7$.o\xf1\xf3R\x17\x1db\xbb\xd8U\xa5^\xccS"
    b"\x16\x01\x87\xf3/\x93\xd1\xf0v\xc0r\xd7\xcc\xa2Gkz\xca\x80\x0e\xfd\xd0"
    b"\x8b\xbb\xd2Ix\xb3\x1ey\xca-0\xe3z^\xd6\xd6\x8f_\xf1\x9dP\x9fi\xa7\xd1"
    b"\xe8\x90\x84\xdc\xbf\xcdky\x8e\xdc\x81\x7f\xa3\xb2+\xbf\x04\xef\xd8\\"
    b"\xc4\xdf\xe1\xb0\x01\xe9\x93\xe3Y\xf1\x1dY\xe8h\x81\xcf\xf1w\xcc\xb4\xef"
    b" \x8b|\x04\xea\x83ej\xbe\x1f\xd4z\x9c`\xd3\x1a\x92A\x06\xe5\x8f\xa9\x13"
    b"\t\x9e=\xfa\x1c\xe5_\x9f%v\x1bo\x11ZO\xd8\xf4\t\xddM\x16-\x04\xfc\x18<\""
    b"CM\xddg~b\xf6\xef\x8e\x0c\xd0\xde|\xa0'\x8a\x0c\xd6x\xae!J\xa6F\x88\x15u"
    b"\x008\x17\xbc7y\xb3\xd8u\xac_\x85\x8d\xe7\xc1@\x9c\xecqc\xa3#\xad\xf1"
    b"\x935\xb5)_\r\xec3]\x0fo]5\xd0my\x07\x9b\xee\x81\xb5\x0f\xcfK+\x00\xc0"
    b"\xe4b\x10\xe4\x0c\x1a \x9b\xe0\x97t\xf6\xa1\x9e\x850\xba\x0c\x9a\x8d\xc8"
    b"\x8f\x07\xd7\xae\xc8\xf9+i\xdc\xb9k\xb0>f\x19\xb8\r\xa8\xf8\x1f$\xa5{p"
    b"\xc6\x880\xce\xdb\xcf\xca_\x86\xac\x88h6\x8bZ%'\xd0\n\xbf\x0f\x9c\"\xba"
    b"\xe5\x86\x9f\x0f7X=mNX[\xcc\x19FU\xc9\x860\xbc\x90a+* \xae_$\x03\x1e\xd3"
    b"\xcd_\xa0\x9c\xde\xaf46q\xa5\xc9\x92\xd7\xca\xe3`\x9d\x85}\xb4\xff\xb3"
    b"\x83\xfb\xb6\xca\xae`\x0bw\x7f\xfc\xd8\xacVe\x19\xc8\x17\x0bZ\xad\x88"
    b"\xeb#\x97\x03\x13\xb1d\x0f{\x0c\x04w\x07\r\x97\xbd\xd6\xc1\xc3B:\x95\x08"
    b"^\x10V\xaeaH\x02\xd9\xe3\n\\\x01X\xf6\x9c\x8a\x06u#%\xbe*\xa1\x18v\x85"
    b"\xec!\t4\x00\x00\x00"
)

FILTERS_RAW_4 = [{"id": lzma.FILTER_DELTA, "dist": 4},
                 {"id": lzma.FILTER_X86, "start_offset": 0x40},
                 {"id": lzma.FILTER_LZMA2, "preset": 4, "lc": 2}]
COMPRESSED_RAW_4 = (
    b"\xe0\x07\x80\x06\x0e\\\x00\x05\x14\x07bW\xaah\xdd\x10\xdc'\xd6\x90,\xc6v"
    b"Jq \x14l\xb7\x83xB\x0b\x97f=&fx\xba\n>Tn\xbf\x8f\xfb\x1dF\xca\xc3v_\xca?"
    b"\xfbV<\x92#\xd4w\xa6\x8a\xeb\xf6\x03\xc0\x01\x94\xd8\x9e\x13\x12\x98\xd1"
    b"*\xfa]c\xe8\x1e~\xaf\xb5]Eg\xfb\x9e\x01\"8\xb2\x90\x06=~\xe9\x91W\xcd"
    b"\xecD\x12\xc7\xfa\xe1\x91\x06\xc7\x99\xb9\xe3\x901\x87\x19u\x0f\x869\xff"
    b"\xc1\xb0hw|\xb0\xdcl\xcck\xb16o7\x85\xee{Y_b\xbf\xbc$\xf3=\x8d\x8bw\xe5Z"
    b"\x08@\xc4kmE\xad\xfb\xf6*\xd8\xad\xa1\xfb\xc5{\xdej,)\x1emB\x1f<\xaeca"
    b"\x80(\xee\x07 \xdf\xe9\xf8\xeb\x0e-\x97\x86\x90c\xf9\xea'B\xf7`\xd7\xb0"
    b"\x92\xbd\xa0\x82]\xbd\x0e\x0eB\x19\xdc\x96\xc6\x19\xd86D\xf0\xd5\x831"
    b"\x03\xb7\x1c\xf7&5\x1a\x8f PZ&j\xf8\x98\x1bo\xcc\x86\x9bS\xd3\xa5\xcdu"
    b"\xf9$\xcc\x97o\xe5V~\xfb\x97\xb5\x0b\x17\x9c\xfdxW\x10\xfep4\x80\xdaHDY"
    b"\xfa)\xfet\xb5\"\xd4\xd3F\x81\xf4\x13\x1f\xec\xdf\xa5\x13\xfc\"\x91x\xb7"
    b"\x99\xce\xc8\x92\n\xeb[\x10l*Y\xd8\xb1@\x06\xc8o\x8d7r\xebu\xfd5\x0e\x7f"
    b"\xf1$U{\t}\x1fQ\xcfxN\x9d\x9fXX\xe9`\x83\xc1\x06\xf4\x87v-f\x11\xdb/\\"
    b"\x06\xff\xd7)B\xf3g\x06\x88#2\x1eB244\x7f4q\t\xc893?mPX\x95\xa6a\xfb)d"
    b"\x9b\xfc\x98\x9aj\x04\xae\x9b\x9d\x19w\xba\xf92\xfaA\x11\\\x17\x97C3\xa4"
    b"\xbc!\x88\xcdo[\xec:\x030\x91.\x85\xe0@\\4\x16\x12\x9d\xcaJv\x97\xb04"
    b"\xack\xcbkf\xa3ss\xfc\x16^\x8ce\x85a\xa5=&\xecr\xb3p\xd1E\xd5\x80y\xc7"
    b"\xda\xf6\xfek\xbcT\xbfH\xee\x15o\xc5\x8c\x830\xec\x1d\x01\xae\x0c-e\\"
    b"\x91\x90\x94\xb2\xf8\x88\x91\xe8\x0b\xae\xa7>\x98\xf6\x9ck\xd2\xc6\x08"
    b"\xe6\xab\t\x98\xf2!\xa0\x8c^\xacqA\x99<\x1cEG\x97\xc8\xf1\xb6\xb9\x82"
    b"\x8d\xf7\x08s\x98a\xff\xe3\xcc\x92\x0e\xd2\xb6U\xd7\xd9\x86\x7fa\xe5\x1c"
    b"\x8dTG@\t\x1e\x0e7*\xfc\xde\xbc]6N\xf7\xf1\x84\x9e\x9f\xcf\xe9\x1e\xb5'"
    b"\xf4<\xdf\x99sq\xd0\x9d\xbd\x99\x0b\xb4%p4\xbf{\xbb\x8a\xd2\x0b\xbc=M"
    b"\x94H:\xf5\xa8\xd6\xa4\xc90\xc2D\xb9\xd3\xa8\xb0S\x87 `\xa2\xeb\xf3W\xce"
    b" 7\xf9N#\r\xe6\xbe\t\x9d\xe7\x811\xf9\x10\xc1\xc2\x14\xf6\xfc\xcba\xb7"
    b"\xb1\x7f\x95l\xe4\tjA\xec:\x10\xe5\xfe\xc2\\=D\xe2\x0c\x0b3]\xf7\xc1\xf7"
    b"\xbceZ\xb1A\xea\x16\xe5\xfddgFQ\xed\xaf\x04\xa3\xd3\xf8\xa2q\x19B\xd4r"
    b"\xc5\x0c\x9a\x14\x94\xea\x91\xc4o\xe4\xbb\xb4\x99\xf4@\xd1\xe6\x0c\xe3"
    b"\xc6d\xa0Q\n\xf2/\xd8\xb8S5\x8a\x18:\xb5g\xac\x95D\xce\x17\x07\xd4z\xda"
    b"\x90\xe65\x07\x19H!\t\xfdu\x16\x8e\x0eR\x19\xf4\x8cl\x0c\xf9Q\xf1\x80"
    b"\xe3\xbf\xd7O\xf8\x8c\x18\x0b\x9c\xf1\x1fb\xe1\tR\xb2\xf1\xe1A\xea \xcf-"
    b"IGE\xf1\x14\x98$\x83\x15\xc9\xd8j\xbf\x19\x0f\xd5\xd1\xaa\xb3\xf3\xa5I2s"
    b"\x8d\x145\xca\xd5\xd93\x9c\xb8D0\xe6\xaa%\xd0\xc0P}JO^h\x8e\x08\xadlV."
    b"\x18\x88\x13\x05o\xb0\x07\xeaw\xe0\xb6\xa4\xd5*\xe4r\xef\x07G+\xc1\xbei["
    b"w\xe8\xab@_\xef\x15y\xe5\x12\xc9W\x1b.\xad\x85-\xc2\xf7\xe3mU6g\x8eSA"
    b"\x01(\xd3\xdb\x16\x13=\xde\x92\xf9,D\xb8\x8a\xb2\xb4\xc9\xc3\xefnE\xe8\\"
    b"\xa6\xe2Y\xd2\xcf\xcb\x8c\xb6\xd5\xe9\x1d\x1e\x9a\x8b~\xe2\xa6\rE\x84uV"
    b"\xed\xc6\x99\xddm<\x10[\x0fu\x1f\xc1\x1d1\n\xcfw\xb2%!\xf0[\xce\x87\x83B"
    b"\x08\xaa,\x08%d\xcef\x94\"\xd9g.\xc83\xcbXY+4\xec\x85qA\n\x1d=9\xf0*\xb1"
    b"\x1f/\xf3s\xd61b\x7f@\xfb\x9d\xe3FQ\\\xbd\x82\x1e\x00\xf3\xce\xd3\xe1"
    b"\xca,E\xfd7[\xab\xb6\xb7\xac!mA}\xbd\x9d3R5\x9cF\xabH\xeb\x92)cc\x13\xd0"
    b"\xbd\xee\xe9n{\x1dIJB\xa5\xeb\x11\xe8`w&`\x8b}@Oxe\t\x8a\x07\x02\x95\xf2"
    b"\xed\xda|\xb1e\xbe\xaa\xbbg\x19@\xe1Y\x878\x84\x0f\x8c\xe3\xc98\xf2\x9e"
    b"\xd5N\xb5J\xef\xab!\xe2\x8dq\xe1\xe5q\xc5\xee\x11W\xb7\xe4k*\x027\xa0"
    b"\xa3J\xf4\xd8m\xd0q\x94\xcb\x07\n:\xb6`.\xe4\x9c\x15+\xc0)\xde\x80X\xd4"
    b"\xcfQm\x01\xc2cP\x1cA\x85'\xc9\xac\x8b\xe6\xb2)\xe6\x84t\x1c\x92\xe4Z"
    b"\x1cR\xb0\x9e\x96\xd1\xfb\x1c\xa6\x8b\xcb`\x10\x12]\xf2gR\x9bFT\xe0\xc8H"
    b"S\xfb\xac<\x04\xc7\xc1\xe8\xedP\xf4\x16\xdb\xc0\xd7e\xc2\x17J^\x1f\xab"
    b"\xff[\x08\x19\xb4\xf5\xfb\x19\xb4\x04\xe5c~']\xcb\xc2A\xec\x90\xd0\xed"
    b"\x06,\xc5K{\x86\x03\xb1\xcdMx\xdeQ\x8c3\xf9\x8a\xea=\x89\xaba\xd2\xc89a"
    b"\xd72\xf0\xc3\x19\x8a\xdfs\xd4\xfd\xbb\x81b\xeaE\"\xd8\xf4d\x0cD\xf7IJ!"
    b"\xe5d\xbbG\xe9\xcam\xaa\x0f_r\x95\x91NBq\xcaP\xce\xa7\xa9\xb5\x10\x94eP!"
    b"|\x856\xcd\xbfIir\xb8e\x9bjP\x97q\xabwS7\x1a\x0ehM\xe7\xca\x86?\xdeP}y~"
    b"\x0f\x95I\xfc\x13\xe1<Q\x1b\x868\x1d\x11\xdf\x94\xf4\x82>r\xa9k\x88\xcb"
    b"\xfd\xc3v\xe2\xb9\x8a\x02\x8eq\x92I\xf8\xf6\xf1\x03s\x9b\xb8\xe3\"\xe3"
    b"\xa9\xa5>D\xb8\x96;\xe7\x92\xd133\xe8\xdd'e\xc9.\xdc;\x17\x1f\xf5H\x13q"
    b"\xa4W\x0c\xdb~\x98\x01\xeb\xdf\xe32\x13\x0f\xddx\n6\xa0\t\x10\xb6\xbb"
    b"\xb0\xc3\x18\xb6;\x9fj[\xd9\xd5\xc9\x06\x8a\x87\xcd\xe5\xee\xfc\x9c-%@"
    b"\xee\xe0\xeb\xd2\xe3\xe8\xfb\xc0\x122\\\xc7\xaf\xc2\xa1Oth\xb3\x8f\x82"
    b"\xb3\x18\xa8\x07\xd5\xee_\xbe\xe0\x1cA\x1e_\r\x9a\xb0\x17W&\xa2D\x91\x94"
    b"\x1a\xb2\xef\xf2\xdc\x85;X\xb0,\xeb>-7S\xe5\xca\x07)\x1fp\x7f\xcaQBL\xca"
    b"\xf3\xb9d\xfc\xb5su\xb0\xc8\x95\x90\xeb*)\xa0v\xe4\x9a{FW\xf4l\xde\xcdj"
    b"\x00"
)


def test_main():
    run_unittest(
        CompressorDecompressorTestCase,
        CompressDecompressFunctionTestCase,
        FileTestCase,
        OpenTestCase,
        MiscellaneousTestCase,
    )

if __name__ == "__main__":
    test_main()
