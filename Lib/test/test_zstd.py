import array
import gc
import io
import pathlib
import random
import re
import os
import unittest
import tempfile
import threading

from test.support.import_helper import import_module
from test.support import threading_helper
from test.support import _1M
from test.support import Py_GIL_DISABLED

_zstd = import_module("_zstd")
zstd = import_module("compression.zstd")

from compression.zstd import (
    open,
    compress,
    decompress,
    ZstdCompressor,
    ZstdDecompressor,
    ZstdDict,
    ZstdError,
    zstd_version,
    zstd_version_info,
    COMPRESSION_LEVEL_DEFAULT,
    get_frame_info,
    get_frame_size,
    finalize_dict,
    train_dict,
    CompressionParameter,
    DecompressionParameter,
    Strategy,
    ZstdFile,
)

_1K = 1024
_130_1K = 130 * _1K
DICT_SIZE1 = 3*_1K

DAT_130K_D = None
DAT_130K_C = None

DECOMPRESSED_DAT = None
COMPRESSED_DAT = None

DECOMPRESSED_100_PLUS_32KB = None
COMPRESSED_100_PLUS_32KB = None

SKIPPABLE_FRAME = None

THIS_FILE_BYTES = None
THIS_FILE_STR = None
COMPRESSED_THIS_FILE = None

COMPRESSED_BOGUS = None

SAMPLES = None

TRAINED_DICT = None

SUPPORT_MULTITHREADING = False

def setUpModule():
    global SUPPORT_MULTITHREADING
    SUPPORT_MULTITHREADING = CompressionParameter.nb_workers.bounds() != (0, 0)
    # uncompressed size 130KB, more than a zstd block.
    # with a frame epilogue, 4 bytes checksum.
    global DAT_130K_D
    DAT_130K_D = bytes([random.randint(0, 127) for _ in range(130*_1K)])

    global DAT_130K_C
    DAT_130K_C = compress(DAT_130K_D, options={CompressionParameter.checksum_flag:1})

    global DECOMPRESSED_DAT
    DECOMPRESSED_DAT = b'abcdefg123456' * 1000

    global COMPRESSED_DAT
    COMPRESSED_DAT = compress(DECOMPRESSED_DAT)

    global DECOMPRESSED_100_PLUS_32KB
    DECOMPRESSED_100_PLUS_32KB = b'a' * (100 + 32*_1K)

    global COMPRESSED_100_PLUS_32KB
    COMPRESSED_100_PLUS_32KB = compress(DECOMPRESSED_100_PLUS_32KB)

    global SKIPPABLE_FRAME
    SKIPPABLE_FRAME = (0x184D2A50).to_bytes(4, byteorder='little') + \
                      (32*_1K).to_bytes(4, byteorder='little') + \
                      b'a' * (32*_1K)

    global THIS_FILE_BYTES, THIS_FILE_STR
    with io.open(os.path.abspath(__file__), 'rb') as f:
        THIS_FILE_BYTES = f.read()
        THIS_FILE_BYTES = re.sub(rb'\r?\n', rb'\n', THIS_FILE_BYTES)
        THIS_FILE_STR = THIS_FILE_BYTES.decode('utf-8')

    global COMPRESSED_THIS_FILE
    COMPRESSED_THIS_FILE = compress(THIS_FILE_BYTES)

    global COMPRESSED_BOGUS
    COMPRESSED_BOGUS = DECOMPRESSED_DAT

    # dict data
    words = [b'red', b'green', b'yellow', b'black', b'withe', b'blue',
             b'lilac', b'purple', b'navy', b'glod', b'silver', b'olive',
             b'dog', b'cat', b'tiger', b'lion', b'fish', b'bird']
    lst = []
    for i in range(300):
        sample = [b'%s = %d' % (random.choice(words), random.randrange(100))
                  for j in range(20)]
        sample = b'\n'.join(sample)

        lst.append(sample)
    global SAMPLES
    SAMPLES = lst
    assert len(SAMPLES) > 10

    global TRAINED_DICT
    TRAINED_DICT = train_dict(SAMPLES, 3*_1K)
    assert len(TRAINED_DICT.dict_content) <= 3*_1K


class FunctionsTestCase(unittest.TestCase):

    def test_version(self):
        s = ".".join((str(i) for i in zstd_version_info))
        self.assertEqual(s, zstd_version)

    def test_compressionLevel_values(self):
        min, max = CompressionParameter.compression_level.bounds()
        self.assertIs(type(COMPRESSION_LEVEL_DEFAULT), int)
        self.assertIs(type(min), int)
        self.assertIs(type(max), int)
        self.assertLess(min, max)

    def test_roundtrip_default(self):
        raw_dat = THIS_FILE_BYTES[: len(THIS_FILE_BYTES) // 6]
        dat1 = compress(raw_dat)
        dat2 = decompress(dat1)
        self.assertEqual(dat2, raw_dat)

    def test_roundtrip_level(self):
        raw_dat = THIS_FILE_BYTES[: len(THIS_FILE_BYTES) // 6]
        level_min, level_max = CompressionParameter.compression_level.bounds()

        for level in range(max(-20, level_min), level_max + 1):
            dat1 = compress(raw_dat, level)
            dat2 = decompress(dat1)
            self.assertEqual(dat2, raw_dat)

    def test_get_frame_info(self):
        # no dict
        info = get_frame_info(COMPRESSED_100_PLUS_32KB[:20])
        self.assertEqual(info.decompressed_size, 32 * _1K + 100)
        self.assertEqual(info.dictionary_id, 0)

        # use dict
        dat = compress(b"a" * 345, zstd_dict=TRAINED_DICT)
        info = get_frame_info(dat)
        self.assertEqual(info.decompressed_size, 345)
        self.assertEqual(info.dictionary_id, TRAINED_DICT.dict_id)

        with self.assertRaisesRegex(ZstdError, "not less than the frame header"):
            get_frame_info(b"aaaaaaaaaaaaaa")

    def test_get_frame_size(self):
        size = get_frame_size(COMPRESSED_100_PLUS_32KB)
        self.assertEqual(size, len(COMPRESSED_100_PLUS_32KB))

        with self.assertRaisesRegex(ZstdError, "not less than this complete frame"):
            get_frame_size(b"aaaaaaaaaaaaaa")

    def test_decompress_2x130_1K(self):
        decompressed_size = get_frame_info(DAT_130K_C).decompressed_size
        self.assertEqual(decompressed_size, _130_1K)

        dat = decompress(DAT_130K_C + DAT_130K_C)
        self.assertEqual(len(dat), 2 * _130_1K)


class CompressorTestCase(unittest.TestCase):

    def test_simple_compress_bad_args(self):
        # ZstdCompressor
        self.assertRaises(TypeError, ZstdCompressor, [])
        self.assertRaises(TypeError, ZstdCompressor, level=3.14)
        self.assertRaises(TypeError, ZstdCompressor, level="abc")
        self.assertRaises(TypeError, ZstdCompressor, options=b"abc")

        self.assertRaises(TypeError, ZstdCompressor, zstd_dict=123)
        self.assertRaises(TypeError, ZstdCompressor, zstd_dict=b"abcd1234")
        self.assertRaises(TypeError, ZstdCompressor, zstd_dict={1: 2, 3: 4})

        with self.assertRaises(ValueError):
            ZstdCompressor(2**31)
        with self.assertRaises(ValueError):
            ZstdCompressor(options={2**31: 100})

        with self.assertRaises(ZstdError):
            ZstdCompressor(options={CompressionParameter.window_log: 100})
        with self.assertRaises(ZstdError):
            ZstdCompressor(options={3333: 100})

        # Method bad arguments
        zc = ZstdCompressor()
        self.assertRaises(TypeError, zc.compress)
        self.assertRaises((TypeError, ValueError), zc.compress, b"foo", b"bar")
        self.assertRaises(TypeError, zc.compress, "str")
        self.assertRaises((TypeError, ValueError), zc.flush, b"foo")
        self.assertRaises(TypeError, zc.flush, b"blah", 1)

        self.assertRaises(ValueError, zc.compress, b'', -1)
        self.assertRaises(ValueError, zc.compress, b'', 3)
        self.assertRaises(ValueError, zc.flush, zc.CONTINUE) # 0
        self.assertRaises(ValueError, zc.flush, 3)

        zc.compress(b'')
        zc.compress(b'', zc.CONTINUE)
        zc.compress(b'', zc.FLUSH_BLOCK)
        zc.compress(b'', zc.FLUSH_FRAME)
        empty = zc.flush()
        zc.flush(zc.FLUSH_BLOCK)
        zc.flush(zc.FLUSH_FRAME)

    def test_compress_parameters(self):
        d = {CompressionParameter.compression_level : 10,

             CompressionParameter.window_log : 12,
             CompressionParameter.hash_log : 10,
             CompressionParameter.chain_log : 12,
             CompressionParameter.search_log : 12,
             CompressionParameter.min_match : 4,
             CompressionParameter.target_length : 12,
             CompressionParameter.strategy : Strategy.lazy,

             CompressionParameter.enable_long_distance_matching : 1,
             CompressionParameter.ldm_hash_log : 12,
             CompressionParameter.ldm_min_match : 11,
             CompressionParameter.ldm_bucket_size_log : 5,
             CompressionParameter.ldm_hash_rate_log : 12,

             CompressionParameter.content_size_flag : 1,
             CompressionParameter.checksum_flag : 1,
             CompressionParameter.dict_id_flag : 0,

             CompressionParameter.nb_workers : 2 if SUPPORT_MULTITHREADING else 0,
             CompressionParameter.job_size : 5*_1M if SUPPORT_MULTITHREADING else 0,
             CompressionParameter.overlap_log : 9 if SUPPORT_MULTITHREADING else 0,
             }
        ZstdCompressor(options=d)

        # larger than signed int, ValueError
        d1 = d.copy()
        d1[CompressionParameter.ldm_bucket_size_log] = 2**31
        self.assertRaises(ValueError, ZstdCompressor, options=d1)

        # clamp compressionLevel
        level_min, level_max = CompressionParameter.compression_level.bounds()
        compress(b'', level_max+1)
        compress(b'', level_min-1)

        compress(b'', options={CompressionParameter.compression_level:level_max+1})
        compress(b'', options={CompressionParameter.compression_level:level_min-1})

        # zstd lib doesn't support MT compression
        if not SUPPORT_MULTITHREADING:
            with self.assertRaises(ZstdError):
                ZstdCompressor(options={CompressionParameter.nb_workers:4})
            with self.assertRaises(ZstdError):
                ZstdCompressor(options={CompressionParameter.job_size:4})
            with self.assertRaises(ZstdError):
                ZstdCompressor(options={CompressionParameter.overlap_log:4})

        # out of bounds error msg
        option = {CompressionParameter.window_log:100}
        with self.assertRaisesRegex(ZstdError,
                (r'Error when setting zstd compression parameter "window_log", '
                 r'it should \d+ <= value <= \d+, provided value is 100\. '
                 r'\((?:32|64)-bit build\)')):
            compress(b'', options=option)

    def test_unknown_compression_parameter(self):
        KEY = 100001234
        option = {CompressionParameter.compression_level: 10,
                  KEY: 200000000}
        pattern = (r'Invalid zstd compression parameter.*?'
                   fr'"unknown parameter \(key {KEY}\)"')
        with self.assertRaisesRegex(ZstdError, pattern):
            ZstdCompressor(options=option)

    @unittest.skipIf(not SUPPORT_MULTITHREADING,
                     "zstd build doesn't support multi-threaded compression")
    def test_zstd_multithread_compress(self):
        size = 40*_1M
        b = THIS_FILE_BYTES * (size // len(THIS_FILE_BYTES))

        options = {CompressionParameter.compression_level : 4,
                   CompressionParameter.nb_workers : 2}

        # compress()
        dat1 = compress(b, options=options)
        dat2 = decompress(dat1)
        self.assertEqual(dat2, b)

        # ZstdCompressor
        c = ZstdCompressor(options=options)
        dat1 = c.compress(b, c.CONTINUE)
        dat2 = c.compress(b, c.FLUSH_BLOCK)
        dat3 = c.compress(b, c.FLUSH_FRAME)
        dat4 = decompress(dat1+dat2+dat3)
        self.assertEqual(dat4, b * 3)

        # ZstdFile
        with ZstdFile(io.BytesIO(), 'w', options=options) as f:
            f.write(b)

    def test_compress_flushblock(self):
        point = len(THIS_FILE_BYTES) // 2

        c = ZstdCompressor()
        self.assertEqual(c.last_mode, c.FLUSH_FRAME)
        dat1 = c.compress(THIS_FILE_BYTES[:point])
        self.assertEqual(c.last_mode, c.CONTINUE)
        dat1 += c.compress(THIS_FILE_BYTES[point:], c.FLUSH_BLOCK)
        self.assertEqual(c.last_mode, c.FLUSH_BLOCK)
        dat2 = c.flush()
        pattern = "Compressed data ended before the end-of-stream marker"
        with self.assertRaisesRegex(ZstdError, pattern):
            decompress(dat1)

        dat3 = decompress(dat1 + dat2)

        self.assertEqual(dat3, THIS_FILE_BYTES)

    def test_compress_flushframe(self):
        # test compress & decompress
        point = len(THIS_FILE_BYTES) // 2

        c = ZstdCompressor()

        dat1 = c.compress(THIS_FILE_BYTES[:point])
        self.assertEqual(c.last_mode, c.CONTINUE)

        dat1 += c.compress(THIS_FILE_BYTES[point:], c.FLUSH_FRAME)
        self.assertEqual(c.last_mode, c.FLUSH_FRAME)

        nt = get_frame_info(dat1)
        self.assertEqual(nt.decompressed_size, None) # no content size

        dat2 = decompress(dat1)

        self.assertEqual(dat2, THIS_FILE_BYTES)

        # single .FLUSH_FRAME mode has content size
        c = ZstdCompressor()
        dat = c.compress(THIS_FILE_BYTES, mode=c.FLUSH_FRAME)
        self.assertEqual(c.last_mode, c.FLUSH_FRAME)

        nt = get_frame_info(dat)
        self.assertEqual(nt.decompressed_size, len(THIS_FILE_BYTES))

    def test_compress_empty(self):
        # output empty content frame
        self.assertNotEqual(compress(b''), b'')

        c = ZstdCompressor()
        self.assertNotEqual(c.compress(b'', c.FLUSH_FRAME), b'')

class DecompressorTestCase(unittest.TestCase):

    def test_simple_decompress_bad_args(self):
        # ZstdDecompressor
        self.assertRaises(TypeError, ZstdDecompressor, ())
        self.assertRaises(TypeError, ZstdDecompressor, zstd_dict=123)
        self.assertRaises(TypeError, ZstdDecompressor, zstd_dict=b'abc')
        self.assertRaises(TypeError, ZstdDecompressor, zstd_dict={1:2, 3:4})

        self.assertRaises(TypeError, ZstdDecompressor, options=123)
        self.assertRaises(TypeError, ZstdDecompressor, options='abc')
        self.assertRaises(TypeError, ZstdDecompressor, options=b'abc')

        with self.assertRaises(ValueError):
            ZstdDecompressor(options={2**31 : 100})

        with self.assertRaises(ZstdError):
            ZstdDecompressor(options={DecompressionParameter.window_log_max:100})
        with self.assertRaises(ZstdError):
            ZstdDecompressor(options={3333 : 100})

        empty = compress(b'')
        lzd = ZstdDecompressor()
        self.assertRaises(TypeError, lzd.decompress)
        self.assertRaises(TypeError, lzd.decompress, b"foo", b"bar")
        self.assertRaises(TypeError, lzd.decompress, "str")
        lzd.decompress(empty)

    def test_decompress_parameters(self):
        d = {DecompressionParameter.window_log_max : 15}
        ZstdDecompressor(options=d)

        # larger than signed int, ValueError
        d1 = d.copy()
        d1[DecompressionParameter.window_log_max] = 2**31
        self.assertRaises(ValueError, ZstdDecompressor, None, d1)

        # out of bounds error msg
        options = {DecompressionParameter.window_log_max:100}
        with self.assertRaisesRegex(ZstdError,
                (r'Error when setting zstd decompression parameter "window_log_max", '
                 r'it should \d+ <= value <= \d+, provided value is 100\. '
                 r'\((?:32|64)-bit build\)')):
            decompress(b'', options=options)

    def test_unknown_decompression_parameter(self):
        KEY = 100001234
        options = {DecompressionParameter.window_log_max: DecompressionParameter.window_log_max.bounds()[1],
                  KEY: 200000000}
        pattern = (r'Invalid zstd decompression parameter.*?'
                   fr'"unknown parameter \(key {KEY}\)"')
        with self.assertRaisesRegex(ZstdError, pattern):
            ZstdDecompressor(options=options)

    def test_decompress_epilogue_flags(self):
        # DAT_130K_C has a 4 bytes checksum at frame epilogue

        # full unlimited
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C)
        self.assertEqual(len(dat), _130_1K)
        self.assertFalse(d.needs_input)

        with self.assertRaises(EOFError):
            dat = d.decompress(b'')

        # full limited
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C, _130_1K)
        self.assertEqual(len(dat), _130_1K)
        self.assertFalse(d.needs_input)

        with self.assertRaises(EOFError):
            dat = d.decompress(b'', 0)

        # [:-4] unlimited
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C[:-4])
        self.assertEqual(len(dat), _130_1K)
        self.assertTrue(d.needs_input)

        dat = d.decompress(b'')
        self.assertEqual(len(dat), 0)
        self.assertTrue(d.needs_input)

        # [:-4] limited
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C[:-4], _130_1K)
        self.assertEqual(len(dat), _130_1K)
        self.assertFalse(d.needs_input)

        dat = d.decompress(b'', 0)
        self.assertEqual(len(dat), 0)
        self.assertFalse(d.needs_input)

        # [:-3] unlimited
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C[:-3])
        self.assertEqual(len(dat), _130_1K)
        self.assertTrue(d.needs_input)

        dat = d.decompress(b'')
        self.assertEqual(len(dat), 0)
        self.assertTrue(d.needs_input)

        # [:-3] limited
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C[:-3], _130_1K)
        self.assertEqual(len(dat), _130_1K)
        self.assertFalse(d.needs_input)

        dat = d.decompress(b'', 0)
        self.assertEqual(len(dat), 0)
        self.assertFalse(d.needs_input)

        # [:-1] unlimited
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C[:-1])
        self.assertEqual(len(dat), _130_1K)
        self.assertTrue(d.needs_input)

        dat = d.decompress(b'')
        self.assertEqual(len(dat), 0)
        self.assertTrue(d.needs_input)

        # [:-1] limited
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C[:-1], _130_1K)
        self.assertEqual(len(dat), _130_1K)
        self.assertFalse(d.needs_input)

        dat = d.decompress(b'', 0)
        self.assertEqual(len(dat), 0)
        self.assertFalse(d.needs_input)

    def test_decompressor_arg(self):
        zd = ZstdDict(b'12345678', is_raw=True)

        with self.assertRaises(TypeError):
            d = ZstdDecompressor(zstd_dict={})

        with self.assertRaises(TypeError):
            d = ZstdDecompressor(options=zd)

        ZstdDecompressor()
        ZstdDecompressor(zd, {})
        ZstdDecompressor(zstd_dict=zd, options={DecompressionParameter.window_log_max:25})

    def test_decompressor_1(self):
        # empty
        d = ZstdDecompressor()
        dat = d.decompress(b'')

        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)

        # 130_1K full
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C)

        self.assertEqual(len(dat), _130_1K)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)

        # 130_1K full, limit output
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C, _130_1K)

        self.assertEqual(len(dat), _130_1K)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)

        # 130_1K, without 4 bytes checksum
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C[:-4])

        self.assertEqual(len(dat), _130_1K)
        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)

        # above, limit output
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C[:-4], _130_1K)

        self.assertEqual(len(dat), _130_1K)
        self.assertFalse(d.eof)
        self.assertFalse(d.needs_input)

        # full, unused_data
        TRAIL = b'89234893abcd'
        d = ZstdDecompressor()
        dat = d.decompress(DAT_130K_C + TRAIL, _130_1K)

        self.assertEqual(len(dat), _130_1K)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, TRAIL)

    def test_decompressor_chunks_read_300(self):
        TRAIL = b'89234893abcd'
        DAT = DAT_130K_C + TRAIL
        d = ZstdDecompressor()

        bi = io.BytesIO(DAT)
        lst = []
        while True:
            if d.needs_input:
                dat = bi.read(300)
                if not dat:
                    break
            else:
                raise Exception('should not get here')

            ret = d.decompress(dat)
            lst.append(ret)
            if d.eof:
                break

        ret = b''.join(lst)

        self.assertEqual(len(ret), _130_1K)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data + bi.read(), TRAIL)

    def test_decompressor_chunks_read_3(self):
        TRAIL = b'89234893'
        DAT = DAT_130K_C + TRAIL
        d = ZstdDecompressor()

        bi = io.BytesIO(DAT)
        lst = []
        while True:
            if d.needs_input:
                dat = bi.read(3)
                if not dat:
                    break
            else:
                dat = b''

            ret = d.decompress(dat, 1)
            lst.append(ret)
            if d.eof:
                break

        ret = b''.join(lst)

        self.assertEqual(len(ret), _130_1K)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data + bi.read(), TRAIL)


    def test_decompress_empty(self):
        with self.assertRaises(ZstdError):
            decompress(b'')

        d = ZstdDecompressor()
        self.assertEqual(d.decompress(b''), b'')
        self.assertFalse(d.eof)

    def test_decompress_empty_content_frame(self):
        DAT = compress(b'')
        # decompress
        self.assertGreaterEqual(len(DAT), 4)
        self.assertEqual(decompress(DAT), b'')

        with self.assertRaises(ZstdError):
            decompress(DAT[:-1])

        # ZstdDecompressor
        d = ZstdDecompressor()
        dat = d.decompress(DAT)
        self.assertEqual(dat, b'')
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        d = ZstdDecompressor()
        dat = d.decompress(DAT[:-1])
        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

class DecompressorFlagsTestCase(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        options = {CompressionParameter.checksum_flag:1}
        c = ZstdCompressor(options=options)

        cls.DECOMPRESSED_42 = b'a'*42
        cls.FRAME_42 = c.compress(cls.DECOMPRESSED_42, c.FLUSH_FRAME)

        cls.DECOMPRESSED_60 = b'a'*60
        cls.FRAME_60 = c.compress(cls.DECOMPRESSED_60, c.FLUSH_FRAME)

        cls.FRAME_42_60 = cls.FRAME_42 + cls.FRAME_60
        cls.DECOMPRESSED_42_60 = cls.DECOMPRESSED_42 + cls.DECOMPRESSED_60

        cls._130_1K = 130*_1K

        c = ZstdCompressor()
        cls.UNKNOWN_FRAME_42 = c.compress(cls.DECOMPRESSED_42) + c.flush()
        cls.UNKNOWN_FRAME_60 = c.compress(cls.DECOMPRESSED_60) + c.flush()
        cls.UNKNOWN_FRAME_42_60 = cls.UNKNOWN_FRAME_42 + cls.UNKNOWN_FRAME_60

        cls.TRAIL = b'12345678abcdefg!@#$%^&*()_+|'

    def test_function_decompress(self):

        self.assertEqual(len(decompress(COMPRESSED_100_PLUS_32KB)), 100+32*_1K)

        # 1 frame
        self.assertEqual(decompress(self.FRAME_42), self.DECOMPRESSED_42)

        self.assertEqual(decompress(self.UNKNOWN_FRAME_42), self.DECOMPRESSED_42)

        pattern = r"Compressed data ended before the end-of-stream marker"
        with self.assertRaisesRegex(ZstdError, pattern):
            decompress(self.FRAME_42[:1])

        with self.assertRaisesRegex(ZstdError, pattern):
            decompress(self.FRAME_42[:-4])

        with self.assertRaisesRegex(ZstdError, pattern):
            decompress(self.FRAME_42[:-1])

        # 2 frames
        self.assertEqual(decompress(self.FRAME_42_60), self.DECOMPRESSED_42_60)

        self.assertEqual(decompress(self.UNKNOWN_FRAME_42_60), self.DECOMPRESSED_42_60)

        self.assertEqual(decompress(self.FRAME_42 + self.UNKNOWN_FRAME_60),
                         self.DECOMPRESSED_42_60)

        self.assertEqual(decompress(self.UNKNOWN_FRAME_42 + self.FRAME_60),
                         self.DECOMPRESSED_42_60)

        with self.assertRaisesRegex(ZstdError, pattern):
            decompress(self.FRAME_42_60[:-4])

        with self.assertRaisesRegex(ZstdError, pattern):
            decompress(self.UNKNOWN_FRAME_42_60[:-1])

        # 130_1K
        self.assertEqual(decompress(DAT_130K_C), DAT_130K_D)

        with self.assertRaisesRegex(ZstdError, pattern):
            decompress(DAT_130K_C[:-4])

        with self.assertRaisesRegex(ZstdError, pattern):
            decompress(DAT_130K_C[:-1])

        # Unknown frame descriptor
        with self.assertRaisesRegex(ZstdError, "Unknown frame descriptor"):
            decompress(b'aaaaaaaaa')

        with self.assertRaisesRegex(ZstdError, "Unknown frame descriptor"):
            decompress(self.FRAME_42 + b'aaaaaaaaa')

        with self.assertRaisesRegex(ZstdError, "Unknown frame descriptor"):
            decompress(self.UNKNOWN_FRAME_42_60 + b'aaaaaaaaa')

        # doesn't match checksum
        checksum = DAT_130K_C[-4:]
        if checksum[0] == 255:
            wrong_checksum = bytes([254]) + checksum[1:]
        else:
            wrong_checksum = bytes([checksum[0]+1]) + checksum[1:]

        dat = DAT_130K_C[:-4] + wrong_checksum

        with self.assertRaisesRegex(ZstdError, "doesn't match checksum"):
            decompress(dat)

    def test_function_skippable(self):
        self.assertEqual(decompress(SKIPPABLE_FRAME), b'')
        self.assertEqual(decompress(SKIPPABLE_FRAME + SKIPPABLE_FRAME), b'')

        # 1 frame + 2 skippable
        self.assertEqual(len(decompress(SKIPPABLE_FRAME + SKIPPABLE_FRAME + DAT_130K_C)),
                         self._130_1K)

        self.assertEqual(len(decompress(DAT_130K_C + SKIPPABLE_FRAME + SKIPPABLE_FRAME)),
                         self._130_1K)

        self.assertEqual(len(decompress(SKIPPABLE_FRAME + DAT_130K_C + SKIPPABLE_FRAME)),
                         self._130_1K)

        # unknown size
        self.assertEqual(decompress(SKIPPABLE_FRAME + self.UNKNOWN_FRAME_60),
                         self.DECOMPRESSED_60)

        self.assertEqual(decompress(self.UNKNOWN_FRAME_60 + SKIPPABLE_FRAME),
                         self.DECOMPRESSED_60)

        # 2 frames + 1 skippable
        self.assertEqual(decompress(self.FRAME_42 + SKIPPABLE_FRAME + self.FRAME_60),
                         self.DECOMPRESSED_42_60)

        self.assertEqual(decompress(SKIPPABLE_FRAME + self.FRAME_42_60),
                         self.DECOMPRESSED_42_60)

        self.assertEqual(decompress(self.UNKNOWN_FRAME_42_60 + SKIPPABLE_FRAME),
                         self.DECOMPRESSED_42_60)

        # incomplete
        with self.assertRaises(ZstdError):
            decompress(SKIPPABLE_FRAME[:1])

        with self.assertRaises(ZstdError):
            decompress(SKIPPABLE_FRAME[:-1])

        with self.assertRaises(ZstdError):
            decompress(self.FRAME_42 + SKIPPABLE_FRAME[:-1])

        # Unknown frame descriptor
        with self.assertRaisesRegex(ZstdError, "Unknown frame descriptor"):
            decompress(b'aaaaaaaaa' + SKIPPABLE_FRAME)

        with self.assertRaisesRegex(ZstdError, "Unknown frame descriptor"):
            decompress(SKIPPABLE_FRAME + b'aaaaaaaaa')

        with self.assertRaisesRegex(ZstdError, "Unknown frame descriptor"):
            decompress(SKIPPABLE_FRAME + SKIPPABLE_FRAME + b'aaaaaaaaa')

    def test_decompressor_1(self):
        # empty 1
        d = ZstdDecompressor()

        dat = d.decompress(b'')
        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        dat = d.decompress(b'', 0)
        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        dat = d.decompress(COMPRESSED_100_PLUS_32KB + b'a')
        self.assertEqual(dat, DECOMPRESSED_100_PLUS_32KB)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'a')
        self.assertEqual(d.unused_data, b'a') # twice

        # empty 2
        d = ZstdDecompressor()

        dat = d.decompress(b'', 0)
        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        dat = d.decompress(b'')
        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        dat = d.decompress(COMPRESSED_100_PLUS_32KB + b'a')
        self.assertEqual(dat, DECOMPRESSED_100_PLUS_32KB)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'a')
        self.assertEqual(d.unused_data, b'a') # twice

        # 1 frame
        d = ZstdDecompressor()
        dat = d.decompress(self.FRAME_42)

        self.assertEqual(dat, self.DECOMPRESSED_42)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        with self.assertRaises(EOFError):
            d.decompress(b'')

        # 1 frame, trail
        d = ZstdDecompressor()
        dat = d.decompress(self.FRAME_42 + self.TRAIL)

        self.assertEqual(dat, self.DECOMPRESSED_42)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, self.TRAIL)
        self.assertEqual(d.unused_data, self.TRAIL) # twice

        # 1 frame, 32_1K
        temp = compress(b'a'*(32*_1K))
        d = ZstdDecompressor()
        dat = d.decompress(temp, 32*_1K)

        self.assertEqual(dat, b'a'*(32*_1K))
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        with self.assertRaises(EOFError):
            d.decompress(b'')

        # 1 frame, 32_1K+100, trail
        d = ZstdDecompressor()
        dat = d.decompress(COMPRESSED_100_PLUS_32KB+self.TRAIL, 100) # 100 bytes

        self.assertEqual(len(dat), 100)
        self.assertFalse(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')

        dat = d.decompress(b'') # 32_1K

        self.assertEqual(len(dat), 32*_1K)
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, self.TRAIL)
        self.assertEqual(d.unused_data, self.TRAIL) # twice

        with self.assertRaises(EOFError):
            d.decompress(b'')

        # incomplete 1
        d = ZstdDecompressor()
        dat = d.decompress(self.FRAME_60[:1])

        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        # incomplete 2
        d = ZstdDecompressor()

        dat = d.decompress(self.FRAME_60[:-4])
        self.assertEqual(dat, self.DECOMPRESSED_60)
        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        # incomplete 3
        d = ZstdDecompressor()

        dat = d.decompress(self.FRAME_60[:-1])
        self.assertEqual(dat, self.DECOMPRESSED_60)
        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)
        self.assertEqual(d.unused_data, b'')

        # incomplete 4
        d = ZstdDecompressor()

        dat = d.decompress(self.FRAME_60[:-4], 60)
        self.assertEqual(dat, self.DECOMPRESSED_60)
        self.assertFalse(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        dat = d.decompress(b'')
        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        # Unknown frame descriptor
        d = ZstdDecompressor()
        with self.assertRaisesRegex(ZstdError, "Unknown frame descriptor"):
            d.decompress(b'aaaaaaaaa')

    def test_decompressor_skippable(self):
        # 1 skippable
        d = ZstdDecompressor()
        dat = d.decompress(SKIPPABLE_FRAME)

        self.assertEqual(dat, b'')
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        # 1 skippable, max_length=0
        d = ZstdDecompressor()
        dat = d.decompress(SKIPPABLE_FRAME, 0)

        self.assertEqual(dat, b'')
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        # 1 skippable, trail
        d = ZstdDecompressor()
        dat = d.decompress(SKIPPABLE_FRAME + self.TRAIL)

        self.assertEqual(dat, b'')
        self.assertTrue(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, self.TRAIL)
        self.assertEqual(d.unused_data, self.TRAIL) # twice

        # incomplete
        d = ZstdDecompressor()
        dat = d.decompress(SKIPPABLE_FRAME[:-1])

        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        # incomplete
        d = ZstdDecompressor()
        dat = d.decompress(SKIPPABLE_FRAME[:-1], 0)

        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)
        self.assertFalse(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice

        dat = d.decompress(b'')

        self.assertEqual(dat, b'')
        self.assertFalse(d.eof)
        self.assertTrue(d.needs_input)
        self.assertEqual(d.unused_data, b'')
        self.assertEqual(d.unused_data, b'') # twice



class ZstdDictTestCase(unittest.TestCase):

    def test_is_raw(self):
        # must be passed as a keyword argument
        with self.assertRaises(TypeError):
            ZstdDict(bytes(8), True)

        # content < 8
        b = b'1234567'
        with self.assertRaises(ValueError):
            ZstdDict(b)

        # content == 8
        b = b'12345678'
        zd = ZstdDict(b, is_raw=True)
        self.assertEqual(zd.dict_id, 0)

        temp = compress(b'aaa12345678', level=3, zstd_dict=zd)
        self.assertEqual(b'aaa12345678', decompress(temp, zd))

        # is_raw == False
        b = b'12345678abcd'
        with self.assertRaises(ValueError):
            ZstdDict(b)

        # read only attributes
        with self.assertRaises(AttributeError):
            zd.dict_content = b

        with self.assertRaises(AttributeError):
            zd.dict_id = 10000

        # ZstdDict arguments
        zd = ZstdDict(TRAINED_DICT.dict_content, is_raw=False)
        self.assertNotEqual(zd.dict_id, 0)

        zd = ZstdDict(TRAINED_DICT.dict_content, is_raw=True)
        self.assertNotEqual(zd.dict_id, 0) # note this assertion

        with self.assertRaises(TypeError):
            ZstdDict("12345678abcdef", is_raw=True)
        with self.assertRaises(TypeError):
            ZstdDict(TRAINED_DICT)

        # invalid parameter
        with self.assertRaises(TypeError):
            ZstdDict(desk333=345)

    def test_invalid_dict(self):
        DICT_MAGIC = 0xEC30A437.to_bytes(4, byteorder='little')
        dict_content = DICT_MAGIC + b'abcdefghighlmnopqrstuvwxyz'

        # corrupted
        zd = ZstdDict(dict_content, is_raw=False)
        with self.assertRaisesRegex(ZstdError, r'ZSTD_CDict.*?content\.$'):
            ZstdCompressor(zstd_dict=zd.as_digested_dict)
        with self.assertRaisesRegex(ZstdError, r'ZSTD_DDict.*?content\.$'):
            ZstdDecompressor(zd)

        # wrong type
        with self.assertRaisesRegex(TypeError, r'should be ZstdDict object'):
            ZstdCompressor(zstd_dict=(zd, b'123'))
        with self.assertRaisesRegex(TypeError, r'should be ZstdDict object'):
            ZstdCompressor(zstd_dict=(zd, 1, 2))
        with self.assertRaisesRegex(TypeError, r'should be ZstdDict object'):
            ZstdCompressor(zstd_dict=(zd, -1))
        with self.assertRaisesRegex(TypeError, r'should be ZstdDict object'):
            ZstdCompressor(zstd_dict=(zd, 3))

        with self.assertRaisesRegex(TypeError, r'should be ZstdDict object'):
            ZstdDecompressor(zstd_dict=(zd, b'123'))
        with self.assertRaisesRegex(TypeError, r'should be ZstdDict object'):
            ZstdDecompressor((zd, 1, 2))
        with self.assertRaisesRegex(TypeError, r'should be ZstdDict object'):
            ZstdDecompressor((zd, -1))
        with self.assertRaisesRegex(TypeError, r'should be ZstdDict object'):
            ZstdDecompressor((zd, 3))

    def test_train_dict(self):


        TRAINED_DICT = train_dict(SAMPLES, DICT_SIZE1)
        ZstdDict(TRAINED_DICT.dict_content, is_raw=False)

        self.assertNotEqual(TRAINED_DICT.dict_id, 0)
        self.assertGreater(len(TRAINED_DICT.dict_content), 0)
        self.assertLessEqual(len(TRAINED_DICT.dict_content), DICT_SIZE1)
        self.assertTrue(re.match(r'^<ZstdDict dict_id=\d+ dict_size=\d+>$', str(TRAINED_DICT)))

        # compress/decompress
        c = ZstdCompressor(zstd_dict=TRAINED_DICT)
        for sample in SAMPLES:
            dat1 = compress(sample, zstd_dict=TRAINED_DICT)
            dat2 = decompress(dat1, TRAINED_DICT)
            self.assertEqual(sample, dat2)

            dat1 = c.compress(sample)
            dat1 += c.flush()
            dat2 = decompress(dat1, TRAINED_DICT)
            self.assertEqual(sample, dat2)

    def test_finalize_dict(self):
        DICT_SIZE2 = 200*_1K
        C_LEVEL = 6

        try:
            dic2 = finalize_dict(TRAINED_DICT, SAMPLES, DICT_SIZE2, C_LEVEL)
        except NotImplementedError:
            # < v1.4.5 at compile-time, >= v.1.4.5 at run-time
            return

        self.assertNotEqual(dic2.dict_id, 0)
        self.assertGreater(len(dic2.dict_content), 0)
        self.assertLessEqual(len(dic2.dict_content), DICT_SIZE2)

        # compress/decompress
        c = ZstdCompressor(C_LEVEL, zstd_dict=dic2)
        for sample in SAMPLES:
            dat1 = compress(sample, C_LEVEL, zstd_dict=dic2)
            dat2 = decompress(dat1, dic2)
            self.assertEqual(sample, dat2)

            dat1 = c.compress(sample)
            dat1 += c.flush()
            dat2 = decompress(dat1, dic2)
            self.assertEqual(sample, dat2)

        # dict mismatch
        self.assertNotEqual(TRAINED_DICT.dict_id, dic2.dict_id)

        dat1 = compress(SAMPLES[0], zstd_dict=TRAINED_DICT)
        with self.assertRaises(ZstdError):
            decompress(dat1, dic2)

    def test_train_dict_arguments(self):
        with self.assertRaises(ValueError):
            train_dict([], 100*_1K)

        with self.assertRaises(ValueError):
            train_dict(SAMPLES, -100)

        with self.assertRaises(ValueError):
            train_dict(SAMPLES, 0)

    def test_finalize_dict_arguments(self):
        with self.assertRaises(TypeError):
            finalize_dict({1:2}, (b'aaa', b'bbb'), 100*_1K, 2)

        with self.assertRaises(ValueError):
            finalize_dict(TRAINED_DICT, [], 100*_1K, 2)

        with self.assertRaises(ValueError):
            finalize_dict(TRAINED_DICT, SAMPLES, -100, 2)

        with self.assertRaises(ValueError):
            finalize_dict(TRAINED_DICT, SAMPLES, 0, 2)

    def test_train_dict_c(self):
        # argument wrong type
        with self.assertRaises(TypeError):
            _zstd.train_dict({}, (), 100)
        with self.assertRaises(TypeError):
            _zstd.train_dict(b'', 99, 100)
        with self.assertRaises(TypeError):
            _zstd.train_dict(b'', (), 100.1)

        # size > size_t
        with self.assertRaises(ValueError):
            _zstd.train_dict(b'', (2**64+1,), 100)

        # dict_size <= 0
        with self.assertRaises(ValueError):
            _zstd.train_dict(b'', (), 0)

    def test_finalize_dict_c(self):
        with self.assertRaises(TypeError):
            _zstd.finalize_dict(1, 2, 3, 4, 5)

        # argument wrong type
        with self.assertRaises(TypeError):
            _zstd.finalize_dict({}, b'', (), 100, 5)
        with self.assertRaises(TypeError):
            _zstd.finalize_dict(TRAINED_DICT.dict_content, {}, (), 100, 5)
        with self.assertRaises(TypeError):
            _zstd.finalize_dict(TRAINED_DICT.dict_content, b'', 99, 100, 5)
        with self.assertRaises(TypeError):
            _zstd.finalize_dict(TRAINED_DICT.dict_content, b'', (), 100.1, 5)
        with self.assertRaises(TypeError):
            _zstd.finalize_dict(TRAINED_DICT.dict_content, b'', (), 100, 5.1)

        # size > size_t
        with self.assertRaises(ValueError):
            _zstd.finalize_dict(TRAINED_DICT.dict_content, b'', (2**64+1,), 100, 5)

        # dict_size <= 0
        with self.assertRaises(ValueError):
            _zstd.finalize_dict(TRAINED_DICT.dict_content, b'', (), 0, 5)

    def test_train_buffer_protocol_samples(self):
        def _nbytes(dat):
            if isinstance(dat, (bytes, bytearray)):
                return len(dat)
            return memoryview(dat).nbytes

        # prepare samples
        chunk_lst = []
        wrong_size_lst = []
        correct_size_lst = []
        for _ in range(300):
            arr = array.array('Q', [random.randint(0, 20) for i in range(20)])
            chunk_lst.append(arr)
            correct_size_lst.append(_nbytes(arr))
            wrong_size_lst.append(len(arr))
        concatenation = b''.join(chunk_lst)

        # wrong size list
        with self.assertRaisesRegex(ValueError,
                "The samples size tuple doesn't match the concatenation's size"):
            _zstd.train_dict(concatenation, tuple(wrong_size_lst), 100*_1K)

        # correct size list
        _zstd.train_dict(concatenation, tuple(correct_size_lst), 3*_1K)

        # wrong size list
        with self.assertRaisesRegex(ValueError,
                "The samples size tuple doesn't match the concatenation's size"):
            _zstd.finalize_dict(TRAINED_DICT.dict_content,
                                  concatenation, tuple(wrong_size_lst), 300*_1K, 5)

        # correct size list
        _zstd.finalize_dict(TRAINED_DICT.dict_content,
                              concatenation, tuple(correct_size_lst), 300*_1K, 5)

    def test_as_prefix(self):
        # V1
        V1 = THIS_FILE_BYTES
        zd = ZstdDict(V1, is_raw=True)

        # V2
        mid = len(V1) // 2
        V2 = V1[:mid] + \
             (b'a' if V1[mid] != int.from_bytes(b'a') else b'b') + \
             V1[mid+1:]

        # compress
        dat = compress(V2, zstd_dict=zd.as_prefix)
        self.assertEqual(get_frame_info(dat).dictionary_id, 0)

        # decompress
        self.assertEqual(decompress(dat, zd.as_prefix), V2)

        # use wrong prefix
        zd2 = ZstdDict(SAMPLES[0], is_raw=True)
        try:
            decompressed = decompress(dat, zd2.as_prefix)
        except ZstdError: # expected
            pass
        else:
            self.assertNotEqual(decompressed, V2)

        # read only attribute
        with self.assertRaises(AttributeError):
            zd.as_prefix = b'1234'

    def test_as_digested_dict(self):
        zd = TRAINED_DICT

        # test .as_digested_dict
        dat = compress(SAMPLES[0], zstd_dict=zd.as_digested_dict)
        self.assertEqual(decompress(dat, zd.as_digested_dict), SAMPLES[0])
        with self.assertRaises(AttributeError):
            zd.as_digested_dict = b'1234'

        # test .as_undigested_dict
        dat = compress(SAMPLES[0], zstd_dict=zd.as_undigested_dict)
        self.assertEqual(decompress(dat, zd.as_undigested_dict), SAMPLES[0])
        with self.assertRaises(AttributeError):
            zd.as_undigested_dict = b'1234'

    def test_advanced_compression_parameters(self):
        options = {CompressionParameter.compression_level: 6,
                  CompressionParameter.window_log: 20,
                  CompressionParameter.enable_long_distance_matching: 1}

        # automatically select
        dat = compress(SAMPLES[0], options=options, zstd_dict=TRAINED_DICT)
        self.assertEqual(decompress(dat, TRAINED_DICT), SAMPLES[0])

        # explicitly select
        dat = compress(SAMPLES[0], options=options, zstd_dict=TRAINED_DICT.as_digested_dict)
        self.assertEqual(decompress(dat, TRAINED_DICT), SAMPLES[0])

    def test_len(self):
        self.assertEqual(len(TRAINED_DICT), len(TRAINED_DICT.dict_content))
        self.assertIn(str(len(TRAINED_DICT)), str(TRAINED_DICT))

class FileTestCase(unittest.TestCase):
    def setUp(self):
        self.DECOMPRESSED_42 = b'a'*42
        self.FRAME_42 = compress(self.DECOMPRESSED_42)

    def test_init(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            pass
        with ZstdFile(io.BytesIO(), "w") as f:
            pass
        with ZstdFile(io.BytesIO(), "x") as f:
            pass
        with ZstdFile(io.BytesIO(), "a") as f:
            pass

        with ZstdFile(io.BytesIO(), "w", level=12) as f:
            pass
        with ZstdFile(io.BytesIO(), "w", options={CompressionParameter.checksum_flag:1}) as f:
            pass
        with ZstdFile(io.BytesIO(), "w", options={}) as f:
            pass
        with ZstdFile(io.BytesIO(), "w", level=20, zstd_dict=TRAINED_DICT) as f:
            pass

        with ZstdFile(io.BytesIO(), "r", options={DecompressionParameter.window_log_max:25}) as f:
            pass
        with ZstdFile(io.BytesIO(), "r", options={}, zstd_dict=TRAINED_DICT) as f:
            pass

    def test_init_with_PathLike_filename(self):
        with tempfile.NamedTemporaryFile(delete=False) as tmp_f:
            filename = pathlib.Path(tmp_f.name)

        with ZstdFile(filename, "a") as f:
            f.write(DECOMPRESSED_100_PLUS_32KB)
        with ZstdFile(filename) as f:
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB)

        with ZstdFile(filename, "a") as f:
            f.write(DECOMPRESSED_100_PLUS_32KB)
        with ZstdFile(filename) as f:
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB * 2)

        os.remove(filename)

    def test_init_with_filename(self):
        with tempfile.NamedTemporaryFile(delete=False) as tmp_f:
            filename = pathlib.Path(tmp_f.name)

        with ZstdFile(filename) as f:
            pass
        with ZstdFile(filename, "w") as f:
            pass
        with ZstdFile(filename, "a") as f:
            pass

        os.remove(filename)

    def test_init_mode(self):
        bi = io.BytesIO()

        with ZstdFile(bi, "r"):
            pass
        with ZstdFile(bi, "rb"):
            pass
        with ZstdFile(bi, "w"):
            pass
        with ZstdFile(bi, "wb"):
            pass
        with ZstdFile(bi, "a"):
            pass
        with ZstdFile(bi, "ab"):
            pass

    def test_init_with_x_mode(self):
        with tempfile.NamedTemporaryFile() as tmp_f:
            filename = pathlib.Path(tmp_f.name)

        for mode in ("x", "xb"):
            with ZstdFile(filename, mode):
                pass
            with self.assertRaises(FileExistsError):
                with ZstdFile(filename, mode):
                    pass
            os.remove(filename)

    def test_init_bad_mode(self):
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), (3, "x"))
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "")
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "xt")
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "x+")
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "rx")
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "wx")
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "rt")
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "r+")
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "wt")
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "w+")
        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "rw")

        with self.assertRaisesRegex(TypeError, r"NOT be CompressionParameter"):
            ZstdFile(io.BytesIO(), 'rb',
                     options={CompressionParameter.compression_level:5})
        with self.assertRaisesRegex(TypeError,
                                    r"NOT be DecompressionParameter"):
            ZstdFile(io.BytesIO(), 'wb',
                     options={DecompressionParameter.window_log_max:21})

        with self.assertRaises(TypeError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "r", level=12)

    def test_init_bad_check(self):
        with self.assertRaises(TypeError):
            ZstdFile(io.BytesIO(), "w", level='asd')
        # CHECK_UNKNOWN and anything above CHECK_ID_MAX should be invalid.
        with self.assertRaises(ZstdError):
            ZstdFile(io.BytesIO(), "w", options={999:9999})
        with self.assertRaises(ZstdError):
            ZstdFile(io.BytesIO(), "w", options={CompressionParameter.window_log:99})

        with self.assertRaises(TypeError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "r", options=33)

        with self.assertRaises(ValueError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB),
                             options={DecompressionParameter.window_log_max:2**31})

        with self.assertRaises(ZstdError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB),
                             options={444:333})

        with self.assertRaises(TypeError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), zstd_dict={1:2})

        with self.assertRaises(TypeError):
            ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), zstd_dict=b'dict123456')

    def test_init_close_fp(self):
        # get a temp file name
        with tempfile.NamedTemporaryFile(delete=False) as tmp_f:
            tmp_f.write(DAT_130K_C)
            filename = tmp_f.name

        with self.assertRaises(ValueError):
            ZstdFile(filename, options={'a':'b'})

        # for PyPy
        gc.collect()

        os.remove(filename)

    def test_close(self):
        with io.BytesIO(COMPRESSED_100_PLUS_32KB) as src:
            f = ZstdFile(src)
            f.close()
            # ZstdFile.close() should not close the underlying file object.
            self.assertFalse(src.closed)
            # Try closing an already-closed ZstdFile.
            f.close()
            self.assertFalse(src.closed)

        # Test with a real file on disk, opened directly by ZstdFile.
        with tempfile.NamedTemporaryFile(delete=False) as tmp_f:
            filename = pathlib.Path(tmp_f.name)

        f = ZstdFile(filename)
        fp = f._fp
        f.close()
        # Here, ZstdFile.close() *should* close the underlying file object.
        self.assertTrue(fp.closed)
        # Try closing an already-closed ZstdFile.
        f.close()

        os.remove(filename)

    def test_closed(self):
        f = ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB))
        try:
            self.assertFalse(f.closed)
            f.read()
            self.assertFalse(f.closed)
        finally:
            f.close()
        self.assertTrue(f.closed)

        f = ZstdFile(io.BytesIO(), "w")
        try:
            self.assertFalse(f.closed)
        finally:
            f.close()
        self.assertTrue(f.closed)

    def test_fileno(self):
        # 1
        f = ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB))
        try:
            self.assertRaises(io.UnsupportedOperation, f.fileno)
        finally:
            f.close()
        self.assertRaises(ValueError, f.fileno)

        # 2
        with tempfile.NamedTemporaryFile(delete=False) as tmp_f:
            filename = pathlib.Path(tmp_f.name)

        f = ZstdFile(filename)
        try:
            self.assertEqual(f.fileno(), f._fp.fileno())
            self.assertIsInstance(f.fileno(), int)
        finally:
            f.close()
        self.assertRaises(ValueError, f.fileno)

        os.remove(filename)

        # 3, no .fileno() method
        class C:
            def read(self, size=-1):
                return b'123'
        with ZstdFile(C(), 'rb') as f:
            with self.assertRaisesRegex(AttributeError, r'fileno'):
                f.fileno()

    def test_name(self):
        # 1
        f = ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB))
        try:
            with self.assertRaises(AttributeError):
                f.name
        finally:
            f.close()
        with self.assertRaises(ValueError):
            f.name

        # 2
        with tempfile.NamedTemporaryFile(delete=False) as tmp_f:
            filename = pathlib.Path(tmp_f.name)

        f = ZstdFile(filename)
        try:
            self.assertEqual(f.name, f._fp.name)
            self.assertIsInstance(f.name, str)
        finally:
            f.close()
        with self.assertRaises(ValueError):
            f.name

        os.remove(filename)

        # 3, no .filename property
        class C:
            def read(self, size=-1):
                return b'123'
        with ZstdFile(C(), 'rb') as f:
            with self.assertRaisesRegex(AttributeError, r'name'):
                f.name

    def test_seekable(self):
        f = ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB))
        try:
            self.assertTrue(f.seekable())
            f.read()
            self.assertTrue(f.seekable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.seekable)

        f = ZstdFile(io.BytesIO(), "w")
        try:
            self.assertFalse(f.seekable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.seekable)

        src = io.BytesIO(COMPRESSED_100_PLUS_32KB)
        src.seekable = lambda: False
        f = ZstdFile(src)
        try:
            self.assertFalse(f.seekable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.seekable)

    def test_readable(self):
        f = ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB))
        try:
            self.assertTrue(f.readable())
            f.read()
            self.assertTrue(f.readable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.readable)

        f = ZstdFile(io.BytesIO(), "w")
        try:
            self.assertFalse(f.readable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.readable)

    def test_writable(self):
        f = ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB))
        try:
            self.assertFalse(f.writable())
            f.read()
            self.assertFalse(f.writable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.writable)

        f = ZstdFile(io.BytesIO(), "w")
        try:
            self.assertTrue(f.writable())
        finally:
            f.close()
        self.assertRaises(ValueError, f.writable)

    def test_read_0(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            self.assertEqual(f.read(0), b"")
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB)
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB),
                              options={DecompressionParameter.window_log_max:20}) as f:
            self.assertEqual(f.read(0), b"")

        # empty file
        with ZstdFile(io.BytesIO(b'')) as f:
            self.assertEqual(f.read(0), b"")
            with self.assertRaises(EOFError):
                f.read(10)

        with ZstdFile(io.BytesIO(b'')) as f:
            with self.assertRaises(EOFError):
                f.read(10)

    def test_read_10(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            chunks = []
            while True:
                result = f.read(10)
                if not result:
                    break
                self.assertLessEqual(len(result), 10)
                chunks.append(result)
            self.assertEqual(b"".join(chunks), DECOMPRESSED_100_PLUS_32KB)

    def test_read_multistream(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB * 5)) as f:
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB * 5)

        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB + SKIPPABLE_FRAME)) as f:
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB)

        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB + COMPRESSED_DAT)) as f:
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB + DECOMPRESSED_DAT)

    def test_read_incomplete(self):
        with ZstdFile(io.BytesIO(DAT_130K_C[:-200])) as f:
            self.assertRaises(EOFError, f.read)

        # Trailing data isn't a valid compressed stream
        with ZstdFile(io.BytesIO(self.FRAME_42 + b'12345')) as f:
            self.assertRaises(ZstdError, f.read)

        with ZstdFile(io.BytesIO(SKIPPABLE_FRAME + b'12345')) as f:
            self.assertRaises(ZstdError, f.read)

    def test_read_truncated(self):
        # Drop stream epilogue: 4 bytes checksum
        truncated = DAT_130K_C[:-4]
        with ZstdFile(io.BytesIO(truncated)) as f:
            self.assertRaises(EOFError, f.read)

        with ZstdFile(io.BytesIO(truncated)) as f:
            # this is an important test, make sure it doesn't raise EOFError.
            self.assertEqual(f.read(130*_1K), DAT_130K_D)
            with self.assertRaises(EOFError):
                f.read(1)

        # Incomplete header
        for i in range(1, 20):
            with ZstdFile(io.BytesIO(truncated[:i])) as f:
                self.assertRaises(EOFError, f.read, 1)

    def test_read_bad_args(self):
        f = ZstdFile(io.BytesIO(COMPRESSED_DAT))
        f.close()
        self.assertRaises(ValueError, f.read)
        with ZstdFile(io.BytesIO(), "w") as f:
            self.assertRaises(ValueError, f.read)
        with ZstdFile(io.BytesIO(COMPRESSED_DAT)) as f:
            self.assertRaises(TypeError, f.read, float())

    def test_read_bad_data(self):
        with ZstdFile(io.BytesIO(COMPRESSED_BOGUS)) as f:
            self.assertRaises(ZstdError, f.read)

    def test_read_exception(self):
        class C:
            def read(self, size=-1):
                raise OSError
        with ZstdFile(C()) as f:
            with self.assertRaises(OSError):
                f.read(10)

    def test_read1(self):
        with ZstdFile(io.BytesIO(DAT_130K_C)) as f:
            blocks = []
            while True:
                result = f.read1()
                if not result:
                    break
                blocks.append(result)
            self.assertEqual(b"".join(blocks), DAT_130K_D)
            self.assertEqual(f.read1(), b"")

    def test_read1_0(self):
        with ZstdFile(io.BytesIO(COMPRESSED_DAT)) as f:
            self.assertEqual(f.read1(0), b"")

    def test_read1_10(self):
        with ZstdFile(io.BytesIO(COMPRESSED_DAT)) as f:
            blocks = []
            while True:
                result = f.read1(10)
                if not result:
                    break
                blocks.append(result)
            self.assertEqual(b"".join(blocks), DECOMPRESSED_DAT)
            self.assertEqual(f.read1(), b"")

    def test_read1_multistream(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB * 5)) as f:
            blocks = []
            while True:
                result = f.read1()
                if not result:
                    break
                blocks.append(result)
            self.assertEqual(b"".join(blocks), DECOMPRESSED_100_PLUS_32KB * 5)
            self.assertEqual(f.read1(), b"")

    def test_read1_bad_args(self):
        f = ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB))
        f.close()
        self.assertRaises(ValueError, f.read1)
        with ZstdFile(io.BytesIO(), "w") as f:
            self.assertRaises(ValueError, f.read1)
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            self.assertRaises(TypeError, f.read1, None)

    def test_readinto(self):
        arr = array.array("I", range(100))
        self.assertEqual(len(arr), 100)
        self.assertEqual(len(arr) * arr.itemsize, 400)
        ba = bytearray(300)
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            # 0 length output buffer
            self.assertEqual(f.readinto(ba[0:0]), 0)

            # use correct length for buffer protocol object
            self.assertEqual(f.readinto(arr), 400)
            self.assertEqual(arr.tobytes(), DECOMPRESSED_100_PLUS_32KB[:400])

            # normal readinto
            self.assertEqual(f.readinto(ba), 300)
            self.assertEqual(ba, DECOMPRESSED_100_PLUS_32KB[400:700])

    def test_peek(self):
        with ZstdFile(io.BytesIO(DAT_130K_C)) as f:
            result = f.peek()
            self.assertGreater(len(result), 0)
            self.assertTrue(DAT_130K_D.startswith(result))
            self.assertEqual(f.read(), DAT_130K_D)
        with ZstdFile(io.BytesIO(DAT_130K_C)) as f:
            result = f.peek(10)
            self.assertGreater(len(result), 0)
            self.assertTrue(DAT_130K_D.startswith(result))
            self.assertEqual(f.read(), DAT_130K_D)

    def test_peek_bad_args(self):
        with ZstdFile(io.BytesIO(), "w") as f:
            self.assertRaises(ValueError, f.peek)

    def test_iterator(self):
        with io.BytesIO(THIS_FILE_BYTES) as f:
            lines = f.readlines()
        compressed = compress(THIS_FILE_BYTES)

        # iter
        with ZstdFile(io.BytesIO(compressed)) as f:
            self.assertListEqual(list(iter(f)), lines)

        # readline
        with ZstdFile(io.BytesIO(compressed)) as f:
            for line in lines:
                self.assertEqual(f.readline(), line)
            self.assertEqual(f.readline(), b'')
            self.assertEqual(f.readline(), b'')

        # readlines
        with ZstdFile(io.BytesIO(compressed)) as f:
            self.assertListEqual(f.readlines(), lines)

    def test_decompress_limited(self):
        _ZSTD_DStreamInSize = 128*_1K + 3

        bomb = compress(b'\0' * int(2e6), level=10)
        self.assertLess(len(bomb), _ZSTD_DStreamInSize)

        decomp = ZstdFile(io.BytesIO(bomb))
        self.assertEqual(decomp.read(1), b'\0')

        # BufferedReader uses 128 KiB buffer in __init__.py
        max_decomp = 128*_1K
        self.assertLessEqual(decomp._buffer.raw.tell(), max_decomp,
            "Excessive amount of data was decompressed")

    def test_write(self):
        raw_data = THIS_FILE_BYTES[: len(THIS_FILE_BYTES) // 6]
        with io.BytesIO() as dst:
            with ZstdFile(dst, "w") as f:
                f.write(raw_data)

            comp = ZstdCompressor()
            expected = comp.compress(raw_data) + comp.flush()
            self.assertEqual(dst.getvalue(), expected)

        with io.BytesIO() as dst:
            with ZstdFile(dst, "w", level=12) as f:
                f.write(raw_data)

            comp = ZstdCompressor(12)
            expected = comp.compress(raw_data) + comp.flush()
            self.assertEqual(dst.getvalue(), expected)

        with io.BytesIO() as dst:
            with ZstdFile(dst, "w", options={CompressionParameter.checksum_flag:1}) as f:
                f.write(raw_data)

            comp = ZstdCompressor(options={CompressionParameter.checksum_flag:1})
            expected = comp.compress(raw_data) + comp.flush()
            self.assertEqual(dst.getvalue(), expected)

        with io.BytesIO() as dst:
            options = {CompressionParameter.compression_level:-5,
                      CompressionParameter.checksum_flag:1}
            with ZstdFile(dst, "w",
                          options=options) as f:
                f.write(raw_data)

            comp = ZstdCompressor(options=options)
            expected = comp.compress(raw_data) + comp.flush()
            self.assertEqual(dst.getvalue(), expected)

    def test_write_empty_frame(self):
        # .FLUSH_FRAME generates an empty content frame
        c = ZstdCompressor()
        self.assertNotEqual(c.flush(c.FLUSH_FRAME), b'')
        self.assertNotEqual(c.flush(c.FLUSH_FRAME), b'')

        # don't generate empty content frame
        bo = io.BytesIO()
        with ZstdFile(bo, 'w') as f:
            pass
        self.assertEqual(bo.getvalue(), b'')

        bo = io.BytesIO()
        with ZstdFile(bo, 'w') as f:
            f.flush(f.FLUSH_FRAME)
        self.assertEqual(bo.getvalue(), b'')

        # if .write(b''), generate empty content frame
        bo = io.BytesIO()
        with ZstdFile(bo, 'w') as f:
            f.write(b'')
        self.assertNotEqual(bo.getvalue(), b'')

        # has an empty content frame
        bo = io.BytesIO()
        with ZstdFile(bo, 'w') as f:
            f.flush(f.FLUSH_BLOCK)
        self.assertNotEqual(bo.getvalue(), b'')

    def test_write_empty_block(self):
        # If no internal data, .FLUSH_BLOCK return b''.
        c = ZstdCompressor()
        self.assertEqual(c.flush(c.FLUSH_BLOCK), b'')
        self.assertNotEqual(c.compress(b'123', c.FLUSH_BLOCK),
                            b'')
        self.assertEqual(c.flush(c.FLUSH_BLOCK), b'')
        self.assertEqual(c.compress(b''), b'')
        self.assertEqual(c.compress(b''), b'')
        self.assertEqual(c.flush(c.FLUSH_BLOCK), b'')

        # mode = .last_mode
        bo = io.BytesIO()
        with ZstdFile(bo, 'w') as f:
            f.write(b'123')
            f.flush(f.FLUSH_BLOCK)
            fp_pos = f._fp.tell()
            self.assertNotEqual(fp_pos, 0)
            f.flush(f.FLUSH_BLOCK)
            self.assertEqual(f._fp.tell(), fp_pos)

        # mode != .last_mode
        bo = io.BytesIO()
        with ZstdFile(bo, 'w') as f:
            f.flush(f.FLUSH_BLOCK)
            self.assertEqual(f._fp.tell(), 0)
            f.write(b'')
            f.flush(f.FLUSH_BLOCK)
            self.assertEqual(f._fp.tell(), 0)

    def test_write_101(self):
        with io.BytesIO() as dst:
            with ZstdFile(dst, "w") as f:
                for start in range(0, len(THIS_FILE_BYTES), 101):
                    f.write(THIS_FILE_BYTES[start:start+101])

            comp = ZstdCompressor()
            expected = comp.compress(THIS_FILE_BYTES) + comp.flush()
            self.assertEqual(dst.getvalue(), expected)

    def test_write_append(self):
        def comp(data):
            comp = ZstdCompressor()
            return comp.compress(data) + comp.flush()

        part1 = THIS_FILE_BYTES[:_1K]
        part2 = THIS_FILE_BYTES[_1K:1536]
        part3 = THIS_FILE_BYTES[1536:]
        expected = b"".join(comp(x) for x in (part1, part2, part3))
        with io.BytesIO() as dst:
            with ZstdFile(dst, "w") as f:
                f.write(part1)
            with ZstdFile(dst, "a") as f:
                f.write(part2)
            with ZstdFile(dst, "a") as f:
                f.write(part3)
            self.assertEqual(dst.getvalue(), expected)

    def test_write_bad_args(self):
        f = ZstdFile(io.BytesIO(), "w")
        f.close()
        self.assertRaises(ValueError, f.write, b"foo")
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB), "r") as f:
            self.assertRaises(ValueError, f.write, b"bar")
        with ZstdFile(io.BytesIO(), "w") as f:
            self.assertRaises(TypeError, f.write, None)
            self.assertRaises(TypeError, f.write, "text")
            self.assertRaises(TypeError, f.write, 789)

    def test_writelines(self):
        def comp(data):
            comp = ZstdCompressor()
            return comp.compress(data) + comp.flush()

        with io.BytesIO(THIS_FILE_BYTES) as f:
            lines = f.readlines()
        with io.BytesIO() as dst:
            with ZstdFile(dst, "w") as f:
                f.writelines(lines)
            expected = comp(THIS_FILE_BYTES)
            self.assertEqual(dst.getvalue(), expected)

    def test_seek_forward(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            f.seek(555)
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB[555:])

    def test_seek_forward_across_streams(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB * 2)) as f:
            f.seek(len(DECOMPRESSED_100_PLUS_32KB) + 123)
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB[123:])

    def test_seek_forward_relative_to_current(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            f.read(100)
            f.seek(1236, 1)
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB[1336:])

    def test_seek_forward_relative_to_end(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            f.seek(-555, 2)
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB[-555:])

    def test_seek_backward(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            f.read(1001)
            f.seek(211)
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB[211:])

    def test_seek_backward_across_streams(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB * 2)) as f:
            f.read(len(DECOMPRESSED_100_PLUS_32KB) + 333)
            f.seek(737)
            self.assertEqual(f.read(),
              DECOMPRESSED_100_PLUS_32KB[737:] + DECOMPRESSED_100_PLUS_32KB)

    def test_seek_backward_relative_to_end(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            f.seek(-150, 2)
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB[-150:])

    def test_seek_past_end(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            f.seek(len(DECOMPRESSED_100_PLUS_32KB) + 9001)
            self.assertEqual(f.tell(), len(DECOMPRESSED_100_PLUS_32KB))
            self.assertEqual(f.read(), b"")

    def test_seek_past_start(self):
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            f.seek(-88)
            self.assertEqual(f.tell(), 0)
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB)

    def test_seek_bad_args(self):
        f = ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB))
        f.close()
        self.assertRaises(ValueError, f.seek, 0)
        with ZstdFile(io.BytesIO(), "w") as f:
            self.assertRaises(ValueError, f.seek, 0)
        with ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB)) as f:
            self.assertRaises(ValueError, f.seek, 0, 3)
            # io.BufferedReader raises TypeError instead of ValueError
            self.assertRaises((TypeError, ValueError), f.seek, 9, ())
            self.assertRaises(TypeError, f.seek, None)
            self.assertRaises(TypeError, f.seek, b"derp")

    def test_seek_not_seekable(self):
        class C(io.BytesIO):
            def seekable(self):
                return False
        obj = C(COMPRESSED_100_PLUS_32KB)
        with ZstdFile(obj, 'r') as f:
            d = f.read(1)
            self.assertFalse(f.seekable())
            with self.assertRaisesRegex(io.UnsupportedOperation,
                                        'File or stream is not seekable'):
                f.seek(0)
            d += f.read()
            self.assertEqual(d, DECOMPRESSED_100_PLUS_32KB)

    def test_tell(self):
        with ZstdFile(io.BytesIO(DAT_130K_C)) as f:
            pos = 0
            while True:
                self.assertEqual(f.tell(), pos)
                result = f.read(random.randint(171, 189))
                if not result:
                    break
                pos += len(result)
            self.assertEqual(f.tell(), len(DAT_130K_D))
        with ZstdFile(io.BytesIO(), "w") as f:
            for pos in range(0, len(DAT_130K_D), 143):
                self.assertEqual(f.tell(), pos)
                f.write(DAT_130K_D[pos:pos+143])
            self.assertEqual(f.tell(), len(DAT_130K_D))

    def test_tell_bad_args(self):
        f = ZstdFile(io.BytesIO(COMPRESSED_100_PLUS_32KB))
        f.close()
        self.assertRaises(ValueError, f.tell)

    def test_file_dict(self):
        # default
        bi = io.BytesIO()
        with ZstdFile(bi, 'w', zstd_dict=TRAINED_DICT) as f:
            f.write(SAMPLES[0])
        bi.seek(0)
        with ZstdFile(bi, zstd_dict=TRAINED_DICT) as f:
            dat = f.read()
        self.assertEqual(dat, SAMPLES[0])

        # .as_(un)digested_dict
        bi = io.BytesIO()
        with ZstdFile(bi, 'w', zstd_dict=TRAINED_DICT.as_digested_dict) as f:
            f.write(SAMPLES[0])
        bi.seek(0)
        with ZstdFile(bi, zstd_dict=TRAINED_DICT.as_undigested_dict) as f:
            dat = f.read()
        self.assertEqual(dat, SAMPLES[0])

    def test_file_prefix(self):
        bi = io.BytesIO()
        with ZstdFile(bi, 'w', zstd_dict=TRAINED_DICT.as_prefix) as f:
            f.write(SAMPLES[0])
        bi.seek(0)
        with ZstdFile(bi, zstd_dict=TRAINED_DICT.as_prefix) as f:
            dat = f.read()
        self.assertEqual(dat, SAMPLES[0])

    def test_UnsupportedOperation(self):
        # 1
        with ZstdFile(io.BytesIO(), 'r') as f:
            with self.assertRaises(io.UnsupportedOperation):
                f.write(b'1234')

        # 2
        class T:
            def read(self, size):
                return b'a' * size

        with self.assertRaises(TypeError): # on creation
            with ZstdFile(T(), 'w') as f:
                pass

        # 3
        with ZstdFile(io.BytesIO(), 'w') as f:
            with self.assertRaises(io.UnsupportedOperation):
                f.read(100)
            with self.assertRaises(io.UnsupportedOperation):
                f.seek(100)
        self.assertEqual(f.closed, True)
        with self.assertRaises(ValueError):
            f.readable()
        with self.assertRaises(ValueError):
            f.tell()
        with self.assertRaises(ValueError):
            f.read(100)

    def test_read_readinto_readinto1(self):
        lst = []
        with ZstdFile(io.BytesIO(COMPRESSED_THIS_FILE*5)) as f:
            while True:
                method = random.randint(0, 2)
                size = random.randint(0, 300)

                if method == 0:
                    dat = f.read(size)
                    if not dat and size:
                        break
                    lst.append(dat)
                elif method == 1:
                    ba = bytearray(size)
                    read_size = f.readinto(ba)
                    if read_size == 0 and size:
                        break
                    lst.append(bytes(ba[:read_size]))
                elif method == 2:
                    ba = bytearray(size)
                    read_size = f.readinto1(ba)
                    if read_size == 0 and size:
                        break
                    lst.append(bytes(ba[:read_size]))
        self.assertEqual(b''.join(lst), THIS_FILE_BYTES*5)

    def test_zstdfile_flush(self):
        # closed
        f = ZstdFile(io.BytesIO(), 'w')
        f.close()
        with self.assertRaises(ValueError):
            f.flush()

        # read
        with ZstdFile(io.BytesIO(), 'r') as f:
            # does nothing for read-only stream
            f.flush()

        # write
        DAT = b'abcd'
        bi = io.BytesIO()
        with ZstdFile(bi, 'w') as f:
            self.assertEqual(f.write(DAT), len(DAT))
            self.assertEqual(f.tell(), len(DAT))
            self.assertEqual(bi.tell(), 0) # not enough for a block

            self.assertEqual(f.flush(), None)
            self.assertEqual(f.tell(), len(DAT))
            self.assertGreater(bi.tell(), 0) # flushed

        # write, no .flush() method
        class C:
            def write(self, b):
                return len(b)
        with ZstdFile(C(), 'w') as f:
            self.assertEqual(f.write(DAT), len(DAT))
            self.assertEqual(f.tell(), len(DAT))

            self.assertEqual(f.flush(), None)
            self.assertEqual(f.tell(), len(DAT))

    def test_zstdfile_flush_mode(self):
        self.assertEqual(ZstdFile.FLUSH_BLOCK, ZstdCompressor.FLUSH_BLOCK)
        self.assertEqual(ZstdFile.FLUSH_FRAME, ZstdCompressor.FLUSH_FRAME)
        with self.assertRaises(AttributeError):
            ZstdFile.CONTINUE

        bo = io.BytesIO()
        with ZstdFile(bo, 'w') as f:
            # flush block
            self.assertEqual(f.write(b'123'), 3)
            self.assertIsNone(f.flush(f.FLUSH_BLOCK))
            p1 = bo.tell()
            # mode == .last_mode, should return
            self.assertIsNone(f.flush())
            p2 = bo.tell()
            self.assertEqual(p1, p2)
            # flush frame
            self.assertEqual(f.write(b'456'), 3)
            self.assertIsNone(f.flush(mode=f.FLUSH_FRAME))
            # flush frame
            self.assertEqual(f.write(b'789'), 3)
            self.assertIsNone(f.flush(f.FLUSH_FRAME))
            p1 = bo.tell()
            # mode == .last_mode, should return
            self.assertIsNone(f.flush(f.FLUSH_FRAME))
            p2 = bo.tell()
            self.assertEqual(p1, p2)
        self.assertEqual(decompress(bo.getvalue()), b'123456789')

        bo = io.BytesIO()
        with ZstdFile(bo, 'w') as f:
            f.write(b'123')
            with self.assertRaisesRegex(ValueError, r'\.FLUSH_.*?\.FLUSH_'):
                f.flush(ZstdCompressor.CONTINUE)
            with self.assertRaises(ValueError):
                f.flush(-1)
            with self.assertRaises(ValueError):
                f.flush(123456)
            with self.assertRaises(TypeError):
                f.flush(node=ZstdCompressor.CONTINUE)
            with self.assertRaises((TypeError, ValueError)):
                f.flush('FLUSH_FRAME')
            with self.assertRaises(TypeError):
                f.flush(b'456', f.FLUSH_BLOCK)

    def test_zstdfile_truncate(self):
        with ZstdFile(io.BytesIO(), 'w') as f:
            with self.assertRaises(io.UnsupportedOperation):
                f.truncate(200)

    def test_zstdfile_iter_issue45475(self):
        lines = [l for l in ZstdFile(io.BytesIO(COMPRESSED_THIS_FILE))]
        self.assertGreater(len(lines), 0)

    def test_append_new_file(self):
        with tempfile.NamedTemporaryFile(delete=True) as tmp_f:
            filename = tmp_f.name

        with ZstdFile(filename, 'a') as f:
            pass
        self.assertTrue(os.path.isfile(filename))

        os.remove(filename)

class OpenTestCase(unittest.TestCase):

    def test_binary_modes(self):
        with open(io.BytesIO(COMPRESSED_100_PLUS_32KB), "rb") as f:
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB)
        with io.BytesIO() as bio:
            with open(bio, "wb") as f:
                f.write(DECOMPRESSED_100_PLUS_32KB)
            file_data = decompress(bio.getvalue())
            self.assertEqual(file_data, DECOMPRESSED_100_PLUS_32KB)
            with open(bio, "ab") as f:
                f.write(DECOMPRESSED_100_PLUS_32KB)
            file_data = decompress(bio.getvalue())
            self.assertEqual(file_data, DECOMPRESSED_100_PLUS_32KB * 2)

    def test_text_modes(self):
        # empty input
        with self.assertRaises(EOFError):
            with open(io.BytesIO(b''), "rt", encoding="utf-8", newline='\n') as reader:
                for _ in reader:
                    pass

        # read
        uncompressed = THIS_FILE_STR.replace(os.linesep, "\n")
        with open(io.BytesIO(COMPRESSED_THIS_FILE), "rt", encoding="utf-8") as f:
            self.assertEqual(f.read(), uncompressed)

        with io.BytesIO() as bio:
            # write
            with open(bio, "wt", encoding="utf-8") as f:
                f.write(uncompressed)
            file_data = decompress(bio.getvalue()).decode("utf-8")
            self.assertEqual(file_data.replace(os.linesep, "\n"), uncompressed)
            # append
            with open(bio, "at", encoding="utf-8") as f:
                f.write(uncompressed)
            file_data = decompress(bio.getvalue()).decode("utf-8")
            self.assertEqual(file_data.replace(os.linesep, "\n"), uncompressed * 2)

    def test_bad_params(self):
        with tempfile.NamedTemporaryFile(delete=False) as tmp_f:
            TESTFN = pathlib.Path(tmp_f.name)

        with self.assertRaises(ValueError):
            open(TESTFN, "")
        with self.assertRaises(ValueError):
            open(TESTFN, "rbt")
        with self.assertRaises(ValueError):
            open(TESTFN, "rb", encoding="utf-8")
        with self.assertRaises(ValueError):
            open(TESTFN, "rb", errors="ignore")
        with self.assertRaises(ValueError):
            open(TESTFN, "rb", newline="\n")

        os.remove(TESTFN)

    def test_option(self):
        options = {DecompressionParameter.window_log_max:25}
        with open(io.BytesIO(COMPRESSED_100_PLUS_32KB), "rb", options=options) as f:
            self.assertEqual(f.read(), DECOMPRESSED_100_PLUS_32KB)

        options = {CompressionParameter.compression_level:12}
        with io.BytesIO() as bio:
            with open(bio, "wb", options=options) as f:
                f.write(DECOMPRESSED_100_PLUS_32KB)
            file_data = decompress(bio.getvalue())
            self.assertEqual(file_data, DECOMPRESSED_100_PLUS_32KB)

    def test_encoding(self):
        uncompressed = THIS_FILE_STR.replace(os.linesep, "\n")

        with io.BytesIO() as bio:
            with open(bio, "wt", encoding="utf-16-le") as f:
                f.write(uncompressed)
            file_data = decompress(bio.getvalue()).decode("utf-16-le")
            self.assertEqual(file_data.replace(os.linesep, "\n"), uncompressed)
            bio.seek(0)
            with open(bio, "rt", encoding="utf-16-le") as f:
                self.assertEqual(f.read().replace(os.linesep, "\n"), uncompressed)

    def test_encoding_error_handler(self):
        with io.BytesIO(compress(b"foo\xffbar")) as bio:
            with open(bio, "rt", encoding="ascii", errors="ignore") as f:
                self.assertEqual(f.read(), "foobar")

    def test_newline(self):
        # Test with explicit newline (universal newline mode disabled).
        text = THIS_FILE_STR.replace(os.linesep, "\n")
        with io.BytesIO() as bio:
            with open(bio, "wt", encoding="utf-8", newline="\n") as f:
                f.write(text)
            bio.seek(0)
            with open(bio, "rt", encoding="utf-8", newline="\r") as f:
                self.assertEqual(f.readlines(), [text])

    def test_x_mode(self):
        with tempfile.NamedTemporaryFile(delete=False) as tmp_f:
            TESTFN = pathlib.Path(tmp_f.name)

        for mode in ("x", "xb", "xt"):
            os.remove(TESTFN)

            if mode == "xt":
                encoding = "utf-8"
            else:
                encoding = None
            with open(TESTFN, mode, encoding=encoding):
                pass
            with self.assertRaises(FileExistsError):
                with open(TESTFN, mode):
                    pass

        os.remove(TESTFN)

    def test_open_dict(self):
        # default
        bi = io.BytesIO()
        with open(bi, 'w', zstd_dict=TRAINED_DICT) as f:
            f.write(SAMPLES[0])
        bi.seek(0)
        with open(bi, zstd_dict=TRAINED_DICT) as f:
            dat = f.read()
        self.assertEqual(dat, SAMPLES[0])

        # .as_(un)digested_dict
        bi = io.BytesIO()
        with open(bi, 'w', zstd_dict=TRAINED_DICT.as_digested_dict) as f:
            f.write(SAMPLES[0])
        bi.seek(0)
        with open(bi, zstd_dict=TRAINED_DICT.as_undigested_dict) as f:
            dat = f.read()
        self.assertEqual(dat, SAMPLES[0])

        # invalid dictionary
        bi = io.BytesIO()
        with self.assertRaisesRegex(TypeError, 'zstd_dict'):
            open(bi, 'w', zstd_dict={1:2, 2:3})

        with self.assertRaisesRegex(TypeError, 'zstd_dict'):
            open(bi, 'w', zstd_dict=b'1234567890')

    def test_open_prefix(self):
        bi = io.BytesIO()
        with open(bi, 'w', zstd_dict=TRAINED_DICT.as_prefix) as f:
            f.write(SAMPLES[0])
        bi.seek(0)
        with open(bi, zstd_dict=TRAINED_DICT.as_prefix) as f:
            dat = f.read()
        self.assertEqual(dat, SAMPLES[0])

    def test_buffer_protocol(self):
        # don't use len() for buffer protocol objects
        arr = array.array("i", range(1000))
        LENGTH = len(arr) * arr.itemsize

        with open(io.BytesIO(), "wb") as f:
            self.assertEqual(f.write(arr), LENGTH)
            self.assertEqual(f.tell(), LENGTH)

@unittest.skip("it fails for now, see gh-133885")
class FreeThreadingMethodTests(unittest.TestCase):

    @unittest.skipUnless(Py_GIL_DISABLED, 'this test can only possibly fail with GIL disabled')
    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_compress_locking(self):
        input = b'a'* (16*_1K)
        num_threads = 8

        comp = ZstdCompressor()
        parts = []
        for _ in range(num_threads):
            res = comp.compress(input, ZstdCompressor.FLUSH_BLOCK)
            if res:
                parts.append(res)
        rest1 = comp.flush()
        expected = b''.join(parts) + rest1

        comp = ZstdCompressor()
        output = []
        def run_method(method, input_data, output_data):
            res = method(input_data, ZstdCompressor.FLUSH_BLOCK)
            if res:
                output_data.append(res)
        threads = []

        for i in range(num_threads):
            thread = threading.Thread(target=run_method, args=(comp.compress, input, output))

            threads.append(thread)

        with threading_helper.start_threads(threads):
            pass

        rest2 = comp.flush()
        self.assertEqual(rest1, rest2)
        actual = b''.join(output) + rest2
        self.assertEqual(expected, actual)

    @unittest.skipUnless(Py_GIL_DISABLED, 'this test can only possibly fail with GIL disabled')
    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_decompress_locking(self):
        input = compress(b'a'* (16*_1K))
        num_threads = 8
        # to ensure we decompress over multiple calls, set maxsize
        window_size = _1K * 16//num_threads

        decomp = ZstdDecompressor()
        parts = []
        for _ in range(num_threads):
            res = decomp.decompress(input, window_size)
            if res:
                parts.append(res)
        expected = b''.join(parts)

        comp = ZstdDecompressor()
        output = []
        def run_method(method, input_data, output_data):
            res = method(input_data, window_size)
            if res:
                output_data.append(res)
        threads = []

        for i in range(num_threads):
            thread = threading.Thread(target=run_method, args=(comp.decompress, input, output))

            threads.append(thread)

        with threading_helper.start_threads(threads):
            pass

        actual = b''.join(output)
        self.assertEqual(expected, actual)



if __name__ == "__main__":
    unittest.main()
