import unittest
from test import test_support
import zlib
import binascii
import random


class ChecksumTestCase(unittest.TestCase):
    # checksum test cases
    def test_crc32start(self):
        self.assertEqual(zlib.crc32(b""), zlib.crc32(b"", 0))
        self.assert_(zlib.crc32(b"abc", 0xffffffff))

    def test_crc32empty(self):
        self.assertEqual(zlib.crc32(b"", 0), 0)
        self.assertEqual(zlib.crc32(b"", 1), 1)
        self.assertEqual(zlib.crc32(b"", 432), 432)

    def test_adler32start(self):
        self.assertEqual(zlib.adler32(b""), zlib.adler32(b"", 1))
        self.assert_(zlib.adler32(b"abc", 0xffffffff))

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
        foo = 'abcdefghijklmnop'
        # explicitly test signed behavior
        self.assertEqual(zlib.crc32(foo), 2486878355)
        self.assertEqual(zlib.crc32('spam'), 1138425661)
        self.assertEqual(zlib.adler32(foo+foo), 3573550353)
        self.assertEqual(zlib.adler32('spam'), 72286642)

    def test_same_as_binascii_crc32(self):
        foo = 'abcdefghijklmnop'
        crc = 2486878355
        self.assertEqual(binascii.crc32(foo), crc)
        self.assertEqual(zlib.crc32(foo), crc)
        self.assertEqual(binascii.crc32('spam'), zlib.crc32('spam'))



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
        self.assertRaises(ValueError, zlib.decompressobj, 0)

    def test_decompressobj_badflush(self):
        # verify failure on calling decompressobj.flush with bad params
        self.assertRaises(ValueError, zlib.decompressobj().flush, 0)
        self.assertRaises(ValueError, zlib.decompressobj().flush, -1)



class CompressTestCase(unittest.TestCase):
    # Test compression in one go (whole message compression)
    def test_speech(self):
        x = zlib.compress(HAMLET_SCENE)
        self.assertEqual(zlib.decompress(x), HAMLET_SCENE)

    def test_speech128(self):
        # compress more data
        data = HAMLET_SCENE * 128
        x = zlib.compress(data)
        self.assertEqual(zlib.decompress(x), data)




class CompressObjectTestCase(unittest.TestCase):
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

        self.assertEqual(data, zlib.decompress(combuf))

        dco = zlib.decompressobj()
        bufs = []
        for i in range(0, len(combuf), dcx):
            bufs.append(dco.decompress(combuf[i:i+dcx]))
            self.assertEqual(b'', dco.unconsumed_tail, ########
                             "(A) uct should be b'': not %d long" %
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
        self.assertEqual(b'', dco.unconsumed_tail, ########
                         "(B) uct should be b'': not %d long" %
                                       len(dco.unconsumed_tail))
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
            self.failIf(len(chunk) > dcx,
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
            self.failIf(len(chunk) > max_length,
                        'chunk too big (%d>%d)' % (len(chunk),max_length))
            bufs.append(chunk)
            cb = dco.unconsumed_tail
        if flush:
            bufs.append(dco.flush())
        else:
            while chunk:
                chunk = dco.decompress('', max_length)
                self.failIf(len(chunk) > max_length,
                            'chunk too big (%d>%d)' % (len(chunk),max_length))
                bufs.append(chunk)
        self.assertEqual(data, b''.join(bufs), 'Wrong data retrieved')

    def test_decompressmaxlenflush(self):
        self.test_decompressmaxlen(flush=True)

    def test_maxlenmisc(self):
        # Misc tests of max_length
        dco = zlib.decompressobj()
        self.assertRaises(ValueError, dco.decompress, "", -1)
        self.assertEqual(b'', dco.unconsumed_tail)

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
        self.failUnless(co.flush())  # Returns a zlib header
        dco = zlib.decompressobj()
        self.assertEqual(dco.flush(), b"") # Returns nothing

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
    test_support.run_unittest(
        ChecksumTestCase,
        ExceptionTestCase,
        CompressTestCase,
        CompressObjectTestCase
    )

if __name__ == "__main__":
    unittest.main() # XXX
    ###test_main()
