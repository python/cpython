import unittest
from test.test_support import TESTFN, run_unittest, import_module, unlink, requires
import binascii
import pickle
import random
from test.test_support import precisionbigmemtest, _1G, _4G
import sys

try:
    import mmap
except ImportError:
    mmap = None

zlib = import_module('zlib')

requires_Compress_copy = unittest.skipUnless(
        hasattr(zlib.compressobj(), "copy"),
        'requires Compress.copy()')
requires_Decompress_copy = unittest.skipUnless(
        hasattr(zlib.decompressobj(), "copy"),
        'requires Decompress.copy()')


class ChecksumTestCase(unittest.TestCase):
    # checksum test cases
    def test_crc32start(self):
        self.assertEqual(zlib.crc32(""), zlib.crc32("", 0))
        self.assertTrue(zlib.crc32("abc", 0xffffffff))

    def test_crc32empty(self):
        self.assertEqual(zlib.crc32("", 0), 0)
        self.assertEqual(zlib.crc32("", 1), 1)
        self.assertEqual(zlib.crc32("", 432), 432)

    def test_adler32start(self):
        self.assertEqual(zlib.adler32(""), zlib.adler32("", 1))
        self.assertTrue(zlib.adler32("abc", 0xffffffff))

    def test_adler32empty(self):
        self.assertEqual(zlib.adler32("", 0), 0)
        self.assertEqual(zlib.adler32("", 1), 1)
        self.assertEqual(zlib.adler32("", 432), 432)

    def assertEqual32(self, seen, expected):
        # 32-bit values masked -- checksums on 32- vs 64- bit machines
        # This is important if bit 31 (0x08000000L) is set.
        self.assertEqual(seen & 0x0FFFFFFFFL, expected & 0x0FFFFFFFFL)

    def test_penguins(self):
        self.assertEqual32(zlib.crc32("penguin", 0), 0x0e5c1a120L)
        self.assertEqual32(zlib.crc32("penguin", 1), 0x43b6aa94)
        self.assertEqual32(zlib.adler32("penguin", 0), 0x0bcf02f6)
        self.assertEqual32(zlib.adler32("penguin", 1), 0x0bd602f7)

        self.assertEqual(zlib.crc32("penguin"), zlib.crc32("penguin", 0))
        self.assertEqual(zlib.adler32("penguin"),zlib.adler32("penguin",1))

    def test_abcdefghijklmnop(self):
        """test issue1202 compliance: signed crc32, adler32 in 2.x"""
        foo = 'abcdefghijklmnop'
        # explicitly test signed behavior
        self.assertEqual(zlib.crc32(foo), -1808088941)
        self.assertEqual(zlib.crc32('spam'), 1138425661)
        self.assertEqual(zlib.adler32(foo+foo), -721416943)
        self.assertEqual(zlib.adler32('spam'), 72286642)

    def test_same_as_binascii_crc32(self):
        foo = 'abcdefghijklmnop'
        self.assertEqual(binascii.crc32(foo), zlib.crc32(foo))
        self.assertEqual(binascii.crc32('spam'), zlib.crc32('spam'))

    def test_negative_crc_iv_input(self):
        # The range of valid input values for the crc state should be
        # -2**31 through 2**32-1 to allow inputs artifically constrained
        # to a signed 32-bit integer.
        self.assertEqual(zlib.crc32('ham', -1), zlib.crc32('ham', 0xffffffffL))
        self.assertEqual(zlib.crc32('spam', -3141593),
                         zlib.crc32('spam',  0xffd01027L))
        self.assertEqual(zlib.crc32('spam', -(2**31)),
                         zlib.crc32('spam',  (2**31)))


class ExceptionTestCase(unittest.TestCase):
    # make sure we generate some expected errors
    def test_badlevel(self):
        # specifying compression level out of range causes an error
        # (but -1 is Z_DEFAULT_COMPRESSION and apparently the zlib
        # accepts 0 too)
        self.assertRaises(zlib.error, zlib.compress, 'ERROR', 10)

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
        data = ''.join([binascii.a2b_hex(fmt % random.getrandbits(8 * _1M))
                        for i in range(10)])
        data = data * (size // len(data) + 1)
        try:
            compress_func(data)
        finally:
            # Release memory
            data = None

    def check_big_decompress_buffer(self, size, decompress_func):
        data = 'x' * size
        try:
            compressed = zlib.compress(data, 1)
        finally:
            # Release memory
            data = None
        data = decompress_func(compressed)
        # Sanity check
        try:
            self.assertEqual(len(data), size)
            self.assertEqual(len(data.strip('x')), 0)
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
        self.assertEqual(zlib.decompress(x), data)

    def test_incomplete_stream(self):
        # A useful error message is given
        x = zlib.compress(HAMLET_SCENE)
        self.assertRaisesRegexp(zlib.error,
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


class CompressObjectTestCase(BaseCompressTestCase, unittest.TestCase):
    # Test compression object
    def test_pair(self):
        # straightforward compress/decompress objects
        data = HAMLET_SCENE * 128
        co = zlib.compressobj()
        x1 = co.compress(data)
        x2 = co.flush()
        self.assertRaises(zlib.error, co.flush) # second flush should not work
        dco = zlib.decompressobj()
        y1 = dco.decompress(x1 + x2)
        y2 = dco.flush()
        self.assertEqual(data, y1 + y2)

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
        combuf = ''.join(bufs)

        dco = zlib.decompressobj()
        y1 = dco.decompress(''.join(bufs))
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
        combuf = ''.join(bufs)

        self.assertEqual(data, zlib.decompress(combuf))

        dco = zlib.decompressobj()
        bufs = []
        for i in range(0, len(combuf), dcx):
            bufs.append(dco.decompress(combuf[i:i+dcx]))
            self.assertEqual('', dco.unconsumed_tail, ########
                             "(A) uct should be '': not %d long" %
                                       len(dco.unconsumed_tail))
        if flush:
            bufs.append(dco.flush())
        else:
            while True:
                chunk = dco.decompress('')
                if chunk:
                    bufs.append(chunk)
                else:
                    break
        self.assertEqual('', dco.unconsumed_tail, ########
                         "(B) uct should be '': not %d long" %
                                       len(dco.unconsumed_tail))
        self.assertEqual(data, ''.join(bufs))
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
        combuf = ''.join(bufs)
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
        self.assertEqual(data, ''.join(bufs), 'Wrong data retrieved')

    def test_decompressmaxlen(self, flush=False):
        # Check a decompression object with max_length specified
        data = HAMLET_SCENE * 128
        co = zlib.compressobj()
        bufs = []
        for i in range(0, len(data), 256):
            bufs.append(co.compress(data[i:i+256]))
        bufs.append(co.flush())
        combuf = ''.join(bufs)
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
                chunk = dco.decompress('', max_length)
                self.assertFalse(len(chunk) > max_length,
                            'chunk too big (%d>%d)' % (len(chunk),max_length))
                bufs.append(chunk)
        self.assertEqual(data, ''.join(bufs), 'Wrong data retrieved')

    def test_decompressmaxlenflush(self):
        self.test_decompressmaxlen(flush=True)

    def test_maxlenmisc(self):
        # Misc tests of max_length
        dco = zlib.decompressobj()
        self.assertRaises(ValueError, dco.decompress, "", -1)
        self.assertEqual('', dco.unconsumed_tail)

    def test_clear_unconsumed_tail(self):
        # Issue #12050: calling decompress() without providing max_length
        # should clear the unconsumed_tail attribute.
        cdata = "x\x9cKLJ\x06\x00\x02M\x01"     # "abc"
        dco = zlib.decompressobj()
        ddata = dco.decompress(cdata, 1)
        ddata += dco.decompress(dco.unconsumed_tail)
        self.assertEqual(dco.unconsumed_tail, "")

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
                self.assertEqual(zlib.decompress(''.join([a,b,c,d])),
                                 data, ("Decompress failed: flush "
                                        "mode=%i, level=%i") % (sync, level))
                del obj

    @unittest.skipUnless(hasattr(zlib, 'Z_SYNC_FLUSH'),
                         'requires zlib.Z_SYNC_FLUSH')
    def test_odd_flush(self):
        # Test for odd flushing bugs noted in 2.0, and hopefully fixed in 2.1
        import random
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
        self.assertEqual(dco.flush(), "") # Returns nothing

    def test_decompress_incomplete_stream(self):
        # This is 'foo', deflated
        x = 'x\x9cK\xcb\xcf\x07\x00\x02\x82\x01E'
        # For the record
        self.assertEqual(zlib.decompress(x), 'foo')
        self.assertRaises(zlib.error, zlib.decompress, x[:-5])
        # Omitting the stream end works with decompressor objects
        # (see issue #8672).
        dco = zlib.decompressobj()
        y = dco.decompress(x[:-5])
        y += dco.flush()
        self.assertEqual(y, 'foo')

    def test_flush_with_freed_input(self):
        # Issue #16411: decompressor accesses input to last decompress() call
        # in flush(), even if this object has been freed in the meanwhile.
        input1 = 'abcdefghijklmnopqrstuvwxyz'
        input2 = 'QWERTYUIOPASDFGHJKLZXCVBNM'
        data = zlib.compress(input1)
        dco = zlib.decompressobj()
        dco.decompress(data, 1)
        del data
        data = zlib.compress(input2)
        self.assertEqual(dco.flush(), input1[1:])

    @requires_Compress_copy
    def test_compresscopy(self):
        # Test copying a compression object
        data0 = HAMLET_SCENE
        data1 = HAMLET_SCENE.swapcase()
        c0 = zlib.compressobj(zlib.Z_BEST_COMPRESSION)
        bufs0 = []
        bufs0.append(c0.compress(data0))

        c1 = c0.copy()
        bufs1 = bufs0[:]

        bufs0.append(c0.compress(data0))
        bufs0.append(c0.flush())
        s0 = ''.join(bufs0)

        bufs1.append(c1.compress(data1))
        bufs1.append(c1.flush())
        s1 = ''.join(bufs1)

        self.assertEqual(zlib.decompress(s0),data0+data0)
        self.assertEqual(zlib.decompress(s1),data0+data1)

    @requires_Compress_copy
    def test_badcompresscopy(self):
        # Test copying a compression object in an inconsistent state
        c = zlib.compressobj()
        c.compress(HAMLET_SCENE)
        c.flush()
        self.assertRaises(ValueError, c.copy)

    def test_decompress_unused_data(self):
        # Repeated calls to decompress() after EOF should accumulate data in
        # dco.unused_data, instead of just storing the arg to the last call.
        source = b'abcdefghijklmnopqrstuvwxyz'
        remainder = b'0123456789'
        y = zlib.compress(source)
        x = y + remainder
        for maxlen in 0, 1000:
            for step in 1, 2, len(y), len(x):
                dco = zlib.decompressobj()
                data = b''
                for i in range(0, len(x), step):
                    if i < len(y):
                        self.assertEqual(dco.unused_data, b'')
                    if maxlen == 0:
                        data += dco.decompress(x[i : i + step])
                        self.assertEqual(dco.unconsumed_tail, b'')
                    else:
                        data += dco.decompress(
                                dco.unconsumed_tail + x[i : i + step], maxlen)
                data += dco.flush()
                self.assertEqual(data, source)
                self.assertEqual(dco.unconsumed_tail, b'')
                self.assertEqual(dco.unused_data, remainder)

    @requires_Decompress_copy
    def test_decompresscopy(self):
        # Test copying a decompression object
        data = HAMLET_SCENE
        comp = zlib.compress(data)

        d0 = zlib.decompressobj()
        bufs0 = []
        bufs0.append(d0.decompress(comp[:32]))

        d1 = d0.copy()
        bufs1 = bufs0[:]

        bufs0.append(d0.decompress(comp[32:]))
        s0 = ''.join(bufs0)

        bufs1.append(d1.decompress(comp[32:]))
        s1 = ''.join(bufs1)

        self.assertEqual(s0,s1)
        self.assertEqual(s0,data)

    @requires_Decompress_copy
    def test_baddecompresscopy(self):
        # Test copying a compression object in an inconsistent state
        data = zlib.compress(HAMLET_SCENE)
        d = zlib.decompressobj()
        d.decompress(data)
        d.flush()
        self.assertRaises(ValueError, d.copy)

    def test_compresspickle(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises((TypeError, pickle.PicklingError)):
                pickle.dumps(zlib.compressobj(zlib.Z_BEST_COMPRESSION), proto)

    def test_decompresspickle(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises((TypeError, pickle.PicklingError)):
                pickle.dumps(zlib.decompressobj(), proto)

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

    def test_wbits(self):
        co = zlib.compressobj(1, zlib.DEFLATED, 15)
        zlib15 = co.compress(HAMLET_SCENE) + co.flush()
        self.assertEqual(zlib.decompress(zlib15, 15), HAMLET_SCENE)
        self.assertEqual(zlib.decompress(zlib15, 0), HAMLET_SCENE)
        self.assertEqual(zlib.decompress(zlib15, 32 + 15), HAMLET_SCENE)
        with self.assertRaisesRegexp(zlib.error, 'invalid window size'):
            zlib.decompress(zlib15, 14)
        dco = zlib.decompressobj(32 + 15)
        self.assertEqual(dco.decompress(zlib15), HAMLET_SCENE)
        dco = zlib.decompressobj(14)
        with self.assertRaisesRegexp(zlib.error, 'invalid window size'):
            dco.decompress(zlib15)

        co = zlib.compressobj(1, zlib.DEFLATED, 9)
        zlib9 = co.compress(HAMLET_SCENE) + co.flush()
        self.assertEqual(zlib.decompress(zlib9, 9), HAMLET_SCENE)
        self.assertEqual(zlib.decompress(zlib9, 15), HAMLET_SCENE)
        self.assertEqual(zlib.decompress(zlib9, 0), HAMLET_SCENE)
        self.assertEqual(zlib.decompress(zlib9, 32 + 9), HAMLET_SCENE)
        dco = zlib.decompressobj(32 + 9)
        self.assertEqual(dco.decompress(zlib9), HAMLET_SCENE)

        co = zlib.compressobj(1, zlib.DEFLATED, -15)
        deflate15 = co.compress(HAMLET_SCENE) + co.flush()
        self.assertEqual(zlib.decompress(deflate15, -15), HAMLET_SCENE)
        dco = zlib.decompressobj(-15)
        self.assertEqual(dco.decompress(deflate15), HAMLET_SCENE)

        co = zlib.compressobj(1, zlib.DEFLATED, -9)
        deflate9 = co.compress(HAMLET_SCENE) + co.flush()
        self.assertEqual(zlib.decompress(deflate9, -9), HAMLET_SCENE)
        self.assertEqual(zlib.decompress(deflate9, -15), HAMLET_SCENE)
        dco = zlib.decompressobj(-9)
        self.assertEqual(dco.decompress(deflate9), HAMLET_SCENE)

        co = zlib.compressobj(1, zlib.DEFLATED, 16 + 15)
        gzip = co.compress(HAMLET_SCENE) + co.flush()
        self.assertEqual(zlib.decompress(gzip, 16 + 15), HAMLET_SCENE)
        self.assertEqual(zlib.decompress(gzip, 32 + 15), HAMLET_SCENE)
        dco = zlib.decompressobj(32 + 15)
        self.assertEqual(dco.decompress(gzip), HAMLET_SCENE)


def genblock(seed, length, step=1024, generator=random):
    """length-byte stream of random data from a seed (in step-byte blocks)."""
    if seed is not None:
        generator.seed(seed)
    randint = generator.randint
    if length < step or step < 2:
        step = length
    blocks = []
    for i in range(0, length, step):
        blocks.append(''.join([chr(randint(0,255))
                               for x in range(step)]))
    return ''.join(blocks)[:length]



def choose_lines(source, number, seed=None, generator=random):
    """Return a list of number lines randomly chosen from the source"""
    if seed is not None:
        generator.seed(seed)
    sources = source.split('\n')
    return [generator.choice(sources) for n in range(number)]



HAMLET_SCENE = """
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
    run_unittest(
        ChecksumTestCase,
        ExceptionTestCase,
        CompressTestCase,
        CompressObjectTestCase
    )

if __name__ == "__main__":
    test_main()
