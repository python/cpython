import unittest
from test import support
import binascii
import random
import sys
from test.support import precisionbigmemtest, _1G, _4G

zlib = support.import_module('zlib')

try:
    import mmap
except ImportError:
    mmap = None


class ChecksumTestCase(unittest.TestCase):
    # checksum test cases
    def test_crc32start(self):
        self.assertEqual(zlib.crc32(b""), zlib.crc32(b"", 0))
        self.assertTrue(zlib.crc32(b"abc", 0xffffffff))

    def test_crc32empty(self):
        self.assertEqual(zlib.crc32(b"", 0), 0)
        self.assertEqual(zlib.crc32(b"", 1), 1)
        self.assertEqual(zlib.crc32(b"", 432), 432)

    def test_adler32start(self):
        self.assertEqual(zlib.adler32(b""), zlib.adler32(b"", 1))
        self.assertTrue(zlib.adler32(b"abc", 0xffffffff))

    def test_adler32empty(self):
        self.assertEqual(zlib.adler32(b"", 0), 0)
        self.assertEqual(zlib.adler32(b"", 1), 1)
        self.assertEqual(zlib.adler32(b"", 432), 432)

    def assertEqual32(self, seen, expected):
        # 32-bit values masked -- checksums on 32- vs 64- bit machines
        # This is important if bit 31 (0x08000000L) is set.
        self.assertEqual(seen & 0x0FFFFFFFF, expected & 0x0FFFFFFFF)

    def test_penguins(self):
        self.assertEqual32(zlib.crc32(b"penguin", 0), 0x0e5c1a120)
        self.assertEqual32(zlib.crc32(b"penguin", 1), 0x43b6aa94)
        self.assertEqual32(zlib.adler32(b"penguin", 0), 0x0bcf02f6)
        self.assertEqual32(zlib.adler32(b"penguin", 1), 0x0bd602f7)

        self.assertEqual(zlib.crc32(b"penguin"), zlib.crc32(b"penguin", 0))
        self.assertEqual(zlib.adler32(b"penguin"),zlib.adler32(b"penguin",1))

    def test_crc32_adler32_unsigned(self):
        foo = b'abcdefghijklmnop'
        # explicitly test signed behavior
        self.assertEqual(zlib.crc32(foo), 2486878355)
        self.assertEqual(zlib.crc32(b'spam'), 1138425661)
        self.assertEqual(zlib.adler32(foo+foo), 3573550353)
        self.assertEqual(zlib.adler32(b'spam'), 72286642)

    def test_same_as_binascii_crc32(self):
        foo = b'abcdefghijklmnop'
        crc = 2486878355
        self.assertEqual(binascii.crc32(foo), crc)
        self.assertEqual(zlib.crc32(foo), crc)
        self.assertEqual(binascii.crc32(b'spam'), zlib.crc32(b'spam'))


# Issue #10276 - check that inputs >=4GB are handled correctly.
class ChecksumBigBufferTestCase(unittest.TestCase):

    def setUp(self):
        with open(support.TESTFN, "wb+") as f:
            f.seek(_4G)
            f.write(b"asdf")
            f.flush()
            self.mapping = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)

    def tearDown(self):
        self.mapping.close()
        support.unlink(support.TESTFN)

    @unittest.skipUnless(mmap, "mmap() is not available.")
    @unittest.skipUnless(sys.maxsize > _4G, "Can't run on a 32-bit system.")
    @unittest.skipUnless(support.is_resource_enabled("largefile"),
                         "May use lots of disk space.")
    def test_big_buffer(self):
        self.assertEqual(zlib.crc32(self.mapping), 3058686908)
        self.assertEqual(zlib.adler32(self.mapping), 82837919)


class ExceptionTestCase(unittest.TestCase):
    # make sure we generate some expected errors
    def test_badlevel(self):
        # specifying compression level out of range causes an error
        # (but -1 is Z_DEFAULT_COMPRESSION and apparently the zlib
        # accepts 0 too)
        self.assertRaises(zlib.error, zlib.compress, b'ERROR', 10)

    def test_badargs(self):
        self.assertRaises(TypeError, zlib.adler32)
        self.assertRaises(TypeError, zlib.crc32)
        self.assertRaises(TypeError, zlib.compress)
        self.assertRaises(TypeError, zlib.decompress)
        for arg in (42, None, '', 'abc', (), []):
            self.assertRaises(TypeError, zlib.adler32, arg)
            self.assertRaises(TypeError, zlib.crc32, arg)
            self.assertRaises(TypeError, zlib.compress, arg)
            self.assertRaises(TypeError, zlib.decompress, arg)

    def test_badcompressobj(self):
        # verify failure on building compress object with bad params
        self.assertRaises(ValueError, zlib.compressobj, 1, zlib.DEFLATED, 0)
        # specifying total bits too large causes an error
        self.assertRaises(ValueError,
                zlib.compressobj, 1, zlib.DEFLATED, zlib.MAX_WBITS + 1)

    def test_baddecompressobj(self):
        # verify failure on building decompress object with bad params
        self.assertRaises(ValueError, zlib.decompressobj, -1)

    def test_decompressobj_badflush(self):
        # verify failure on calling decompressobj.flush with bad params
        self.assertRaises(ValueError, zlib.decompressobj().flush, 0)
        self.assertRaises(ValueError, zlib.decompressobj().flush, -1)


class BaseCompressTestCase(object):
    def check_big_compress_buffer(self, size, compress_func):
        _1M = 1024 * 1024
        fmt = "%%0%dx" % (2 * _1M)
        # Generate 10MB worth of random, and expand it by repeating it.
        # The assumption is that zlib's memory is not big enough to exploit
        # such spread out redundancy.
        data = b''.join([random.getrandbits(8 * _1M).to_bytes(_1M, 'little')
                        for i in range(10)])
        data = data * (size // len(data) + 1)
        try:
            compress_func(data)
        finally:
            # Release memory
            data = None

    def check_big_decompress_buffer(self, size, decompress_func):
        data = b'x' * size
        try:
            compressed = zlib.compress(data, 1)
        finally:
            # Release memory
            data = None
        data = decompress_func(compressed)
        # Sanity check
        try:
            self.assertEqual(len(data), size)
            self.assertEqual(len(data.strip(b'x')), 0)
        finally:
            data = None


class CompressTestCase(BaseCompressTestCase, unittest.TestCase):
    # Test compression in one go (whole message compression)
    def test_speech(self):
        x = zlib.compress(HAMLET_SCENE)
        self.assertEqual(zlib.decompress(x), HAMLET_SCENE)

    def test_speech128(self):
        # compress more data
        data = HAMLET_SCENE * 128
        x = zlib.compress(data)
        self.assertEqual(zlib.compress(bytearray(data)), x)
        for ob in x, bytearray(x):
            self.assertEqual(zlib.decompress(ob), data)

    def test_incomplete_stream(self):
        # An useful error message is given
        x = zlib.compress(HAMLET_SCENE)
        self.assertRaisesRegex(zlib.error,
            "Error -5 while decompressing data: incomplete or truncated stream",
            zlib.decompress, x[:-1])

    # Memory use of the following functions takes into account overallocation

    @precisionbigmemtest(size=_1G + 1024 * 1024, memuse=3)
    def test_big_compress_buffer(self, size):
        compress = lambda s: zlib.compress(s, 1)
        self.check_big_compress_buffer(size, compress)

    @precisionbigmemtest(size=_1G + 1024 * 1024, memuse=2)
    def test_big_decompress_buffer(self, size):
        self.check_big_decompress_buffer(size, zlib.decompress)

    @precisionbigmemtest(size=_4G + 100, memuse=1)
    def test_length_overflow(self, size):
        if size < _4G + 100:
            self.skipTest("not enough free memory, need at least 4 GB")
        data = b'x' * size
        try:
            self.assertRaises(OverflowError, zlib.compress, data, 1)
            self.assertRaises(OverflowError, zlib.decompress, data)
        finally:
            data = None


class CompressObjectTestCase(BaseCompressTestCase, unittest.TestCase):
    # Test compression object
    def test_pair(self):
        # straightforward compress/decompress objects
        datasrc = HAMLET_SCENE * 128
        datazip = zlib.compress(datasrc)
        # should compress both bytes and bytearray data
        for data in (datasrc, bytearray(datasrc)):
            co = zlib.compressobj()
            x1 = co.compress(data)
            x2 = co.flush()
            self.assertRaises(zlib.error, co.flush) # second flush should not work
            self.assertEqual(x1 + x2, datazip)
        for v1, v2 in ((x1, x2), (bytearray(x1), bytearray(x2))):
            dco = zlib.decompressobj()
            y1 = dco.decompress(v1 + v2)
            y2 = dco.flush()
            self.assertEqual(data, y1 + y2)
            self.assertIsInstance(dco.unconsumed_tail, bytes)
            self.assertIsInstance(dco.unused_data, bytes)

    def test_compressoptions(self):
        # specify lots of options to compressobj()
        level = 2
        method = zlib.DEFLATED
        wbits = -12
        memlevel = 9
        strategy = zlib.Z_FILTERED
        co = zlib.compressobj(level, method, wbits, memlevel, strategy)
        x1 = co.compress(HAMLET_SCENE)
        x2 = co.flush()
        dco = zlib.decompressobj(wbits)
        y1 = dco.decompress(x1 + x2)
        y2 = dco.flush()
        self.assertEqual(HAMLET_SCENE, y1 + y2)

    def test_compressincremental(self):
        # compress object in steps, decompress object as one-shot
        data = HAMLET_SCENE * 128
        co = zlib.compressobj()
        bufs = []
        for i in range(0, len(data), 256):
            bufs.append(co.compress(data[i:i+256]))
        bufs.append(co.flush())
        combuf = b''.join(bufs)

        dco = zlib.decompressobj()
        y1 = dco.decompress(b''.join(bufs))
        y2 = dco.flush()
        self.assertEqual(data, y1 + y2)

    def test_decompinc(self, flush=False, source=None, cx=256, dcx=64):
        # compress object in steps, decompress object in steps
        source = source or HAMLET_SCENE
        data = source * 128
        co = zlib.compressobj()
        bufs = []
        for i in range(0, len(data), cx):
            bufs.append(co.compress(data[i:i+cx]))
        bufs.append(co.flush())
        combuf = b''.join(bufs)

        decombuf = zlib.decompress(combuf)
        # Test type of return value
        self.assertIsInstance(decombuf, bytes)

        self.assertEqual(data, decombuf)

        dco = zlib.decompressobj()
        bufs = []
        for i in range(0, len(combuf), dcx):
            bufs.append(dco.decompress(combuf[i:i+dcx]))
            self.assertEqual(b'', dco.unconsumed_tail, ########
                             "(A) uct should be b'': not %d long" %
                                       len(dco.unconsumed_tail))
            self.assertEqual(b'', dco.unused_data)
        if flush:
            bufs.append(dco.flush())
        else:
            while True:
                chunk = dco.decompress(b'')
                if chunk:
                    bufs.append(chunk)
                else:
                    break
        self.assertEqual(b'', dco.unconsumed_tail, ########
                         "(B) uct should be b'': not %d long" %
                                       len(dco.unconsumed_tail))
        self.assertEqual(b'', dco.unused_data)
        self.assertEqual(data, b''.join(bufs))
        # Failure means: "decompressobj with init options failed"

    def test_decompincflush(self):
        self.test_decompinc(flush=True)

    def test_decompimax(self, source=None, cx=256, dcx=64):
        # compress in steps, decompress in length-restricted steps
        source = source or HAMLET_SCENE
        # Check a decompression object with max_length specified
        data = source * 128
        co = zlib.compressobj()
        bufs = []
        for i in range(0, len(data), cx):
            bufs.append(co.compress(data[i:i+cx]))
        bufs.append(co.flush())
        combuf = b''.join(bufs)
        self.assertEqual(data, zlib.decompress(combuf),
                         'compressed data failure')

        dco = zlib.decompressobj()
        bufs = []
        cb = combuf
        while cb:
            #max_length = 1 + len(cb)//10
            chunk = dco.decompress(cb, dcx)
            self.assertFalse(len(chunk) > dcx,
                    'chunk too big (%d>%d)' % (len(chunk), dcx))
            bufs.append(chunk)
            cb = dco.unconsumed_tail
        bufs.append(dco.flush())
        self.assertEqual(data, b''.join(bufs), 'Wrong data retrieved')

    def test_decompressmaxlen(self, flush=False):
        # Check a decompression object with max_length specified
        data = HAMLET_SCENE * 128
        co = zlib.compressobj()
        bufs = []
        for i in range(0, len(data), 256):
            bufs.append(co.compress(data[i:i+256]))
        bufs.append(co.flush())
        combuf = b''.join(bufs)
        self.assertEqual(data, zlib.decompress(combuf),
                         'compressed data failure')

        dco = zlib.decompressobj()
        bufs = []
        cb = combuf
        while cb:
            max_length = 1 + len(cb)//10
            chunk = dco.decompress(cb, max_length)
            self.assertFalse(len(chunk) > max_length,
                        'chunk too big (%d>%d)' % (len(chunk),max_length))
            bufs.append(chunk)
            cb = dco.unconsumed_tail
        if flush:
            bufs.append(dco.flush())
        else:
            while chunk:
                chunk = dco.decompress(b'', max_length)
                self.assertFalse(len(chunk) > max_length,
                            'chunk too big (%d>%d)' % (len(chunk),max_length))
                bufs.append(chunk)
        self.assertEqual(data, b''.join(bufs), 'Wrong data retrieved')

    def test_decompressmaxlenflush(self):
        self.test_decompressmaxlen(flush=True)

    def test_maxlenmisc(self):
        # Misc tests of max_length
        dco = zlib.decompressobj()
        self.assertRaises(ValueError, dco.decompress, b"", -1)
        self.assertEqual(b'', dco.unconsumed_tail)

    def test_clear_unconsumed_tail(self):
        # Issue #12050: calling decompress() without providing max_length
        # should clear the unconsumed_tail attribute.
        cdata = b"x\x9cKLJ\x06\x00\x02M\x01"    # "abc"
        dco = zlib.decompressobj()
        ddata = dco.decompress(cdata, 1)
        ddata += dco.decompress(dco.unconsumed_tail)
        self.assertEqual(dco.unconsumed_tail, b"")

    def test_flushes(self):
        # Test flush() with the various options, using all the
        # different levels in order to provide more variations.
        sync_opt = ['Z_NO_FLUSH', 'Z_SYNC_FLUSH', 'Z_FULL_FLUSH']
        sync_opt = [getattr(zlib, opt) for opt in sync_opt
                    if hasattr(zlib, opt)]
        data = HAMLET_SCENE * 8

        for sync in sync_opt:
            for level in range(10):
                obj = zlib.compressobj( level )
                a = obj.compress( data[:3000] )
                b = obj.flush( sync )
                c = obj.compress( data[3000:] )
                d = obj.flush()
                self.assertEqual(zlib.decompress(b''.join([a,b,c,d])),
                                 data, ("Decompress failed: flush "
                                        "mode=%i, level=%i") % (sync, level))
                del obj

    def test_odd_flush(self):
        # Test for odd flushing bugs noted in 2.0, and hopefully fixed in 2.1
        import random

        if hasattr(zlib, 'Z_SYNC_FLUSH'):
            # Testing on 17K of "random" data

            # Create compressor and decompressor objects
            co = zlib.compressobj(zlib.Z_BEST_COMPRESSION)
            dco = zlib.decompressobj()

            # Try 17K of data
            # generate random data stream
            try:
                # In 2.3 and later, WichmannHill is the RNG of the bug report
                gen = random.WichmannHill()
            except AttributeError:
                try:
                    # 2.2 called it Random
                    gen = random.Random()
                except AttributeError:
                    # others might simply have a single RNG
                    gen = random
            gen.seed(1)
            data = genblock(1, 17 * 1024, generator=gen)

            # compress, sync-flush, and decompress
            first = co.compress(data)
            second = co.flush(zlib.Z_SYNC_FLUSH)
            expanded = dco.decompress(first + second)

            # if decompressed data is different from the input data, choke.
            self.assertEqual(expanded, data, "17K random source doesn't match")

    def test_empty_flush(self):
        # Test that calling .flush() on unused objects works.
        # (Bug #1083110 -- calling .flush() on decompress objects
        # caused a core dump.)

        co = zlib.compressobj(zlib.Z_BEST_COMPRESSION)
        self.assertTrue(co.flush())  # Returns a zlib header
        dco = zlib.decompressobj()
        self.assertEqual(dco.flush(), b"") # Returns nothing

    def test_decompress_incomplete_stream(self):
        # This is 'foo', deflated
        x = b'x\x9cK\xcb\xcf\x07\x00\x02\x82\x01E'
        # For the record
        self.assertEqual(zlib.decompress(x), b'foo')
        self.assertRaises(zlib.error, zlib.decompress, x[:-5])
        # Omitting the stream end works with decompressor objects
        # (see issue #8672).
        dco = zlib.decompressobj()
        y = dco.decompress(x[:-5])
        y += dco.flush()
        self.assertEqual(y, b'foo')

    if hasattr(zlib.compressobj(), "copy"):
        def test_compresscopy(self):
            # Test copying a compression object
            data0 = HAMLET_SCENE
            data1 = bytes(str(HAMLET_SCENE, "ascii").swapcase(), "ascii")
            c0 = zlib.compressobj(zlib.Z_BEST_COMPRESSION)
            bufs0 = []
            bufs0.append(c0.compress(data0))

            c1 = c0.copy()
            bufs1 = bufs0[:]

            bufs0.append(c0.compress(data0))
            bufs0.append(c0.flush())
            s0 = b''.join(bufs0)

            bufs1.append(c1.compress(data1))
            bufs1.append(c1.flush())
            s1 = b''.join(bufs1)

            self.assertEqual(zlib.decompress(s0),data0+data0)
            self.assertEqual(zlib.decompress(s1),data0+data1)

        def test_badcompresscopy(self):
            # Test copying a compression object in an inconsistent state
            c = zlib.compressobj()
            c.compress(HAMLET_SCENE)
            c.flush()
            self.assertRaises(ValueError, c.copy)

    if hasattr(zlib.decompressobj(), "copy"):
        def test_decompresscopy(self):
            # Test copying a decompression object
            data = HAMLET_SCENE
            comp = zlib.compress(data)
            # Test type of return value
            self.assertIsInstance(comp, bytes)

            d0 = zlib.decompressobj()
            bufs0 = []
            bufs0.append(d0.decompress(comp[:32]))

            d1 = d0.copy()
            bufs1 = bufs0[:]

            bufs0.append(d0.decompress(comp[32:]))
            s0 = b''.join(bufs0)

            bufs1.append(d1.decompress(comp[32:]))
            s1 = b''.join(bufs1)

            self.assertEqual(s0,s1)
            self.assertEqual(s0,data)

        def test_baddecompresscopy(self):
            # Test copying a compression object in an inconsistent state
            data = zlib.compress(HAMLET_SCENE)
            d = zlib.decompressobj()
            d.decompress(data)
            d.flush()
            self.assertRaises(ValueError, d.copy)

    # Memory use of the following functions takes into account overallocation

    @precisionbigmemtest(size=_1G + 1024 * 1024, memuse=3)
    def test_big_compress_buffer(self, size):
        c = zlib.compressobj(1)
        compress = lambda s: c.compress(s) + c.flush()
        self.check_big_compress_buffer(size, compress)

    @precisionbigmemtest(size=_1G + 1024 * 1024, memuse=2)
    def test_big_decompress_buffer(self, size):
        d = zlib.decompressobj()
        decompress = lambda s: d.decompress(s) + d.flush()
        self.check_big_decompress_buffer(size, decompress)

    @precisionbigmemtest(size=_4G + 100, memuse=1)
    def test_length_overflow(self, size):
        if size < _4G + 100:
            self.skipTest("not enough free memory, need at least 4 GB")
        data = b'x' * size
        try:
            self.assertRaises(OverflowError, zlib.compress, data, 1)
            self.assertRaises(OverflowError, zlib.decompress, data)
        finally:
            data = None


def genblock(seed, length, step=1024, generator=random):
    """length-byte stream of random data from a seed (in step-byte blocks)."""
    if seed is not None:
        generator.seed(seed)
    randint = generator.randint
    if length < step or step < 2:
        step = length
    blocks = bytes()
    for i in range(0, length, step):
        blocks += bytes(randint(0, 255) for x in range(step))
    return blocks



def choose_lines(source, number, seed=None, generator=random):
    """Return a list of number lines randomly chosen from the source"""
    if seed is not None:
        generator.seed(seed)
    sources = source.split('\n')
    return [generator.choice(sources) for n in range(number)]



HAMLET_SCENE = b"""
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


def test_main():
    support.run_unittest(
        ChecksumTestCase,
        ChecksumBigBufferTestCase,
        ExceptionTestCase,
        CompressTestCase,
        CompressObjectTestCase
    )

if __name__ == "__main__":
    unittest.main() # XXX
    ###test_main()
