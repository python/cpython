import zlib
import sys
import imp
import string

try:
    t = imp.find_module('test_zlib')
    file = t[0]
except ImportError:
    file = open(__file__)
buf = file.read() * 8
file.close()

# test the checksums (hex so the test doesn't break on 64-bit machines)
print hex(zlib.crc32('penguin')), hex(zlib.crc32('penguin', 1))
print hex(zlib.adler32('penguin')), hex(zlib.adler32('penguin', 1))

# make sure we generate some expected errors
try:
    zlib.compress('ERROR', zlib.MAX_WBITS + 1)
except zlib.error, msg:
    print "expecting", msg
try:
    zlib.compressobj(1, 8, 0)
except ValueError, msg:
    print "expecting", msg
try:
    zlib.decompressobj(0)
except ValueError, msg:
    print "expecting", msg

x = zlib.compress(buf)
y = zlib.decompress(x)
if buf != y:
    print "normal compression/decompression failed"
else:
    print "normal compression/decompression succeeded"

buf = buf * 16

co = zlib.compressobj(8, 8, -15)
x1 = co.compress(buf)
x2 = co.flush()
x = x1 + x2

dc = zlib.decompressobj(-15)
y1 = dc.decompress(x)
y2 = dc.flush()
y = y1 + y2
if buf != y:
    print "compress/decompression obj failed"
else:
    print "compress/decompression obj succeeded"

co = zlib.compressobj(2, 8, -12, 9, 1)
bufs = []
for i in range(0, len(buf), 256):
    bufs.append(co.compress(buf[i:i+256]))
bufs.append(co.flush())
combuf = string.join(bufs, '')

decomp1 = zlib.decompress(combuf, -12, -5)
if decomp1 != buf:
    print "decompress with init options failed"
else:
    print "decompress with init options succeeded"

deco = zlib.decompressobj(-12)
bufs = []
for i in range(0, len(combuf), 128):
    bufs.append(deco.decompress(combuf[i:i+128]))
bufs.append(deco.flush())
decomp2 = string.join(buf, '')
if decomp2 != buf:
    print "decompressobj with init options failed"
else:
    print "decompressobj with init options succeeded"

# Test flush() with the various options, using all the different levels
# in order to provide more variations.
for sync in [zlib.Z_NO_FLUSH, zlib.Z_SYNC_FLUSH, zlib.Z_FULL_FLUSH]:
    for level in range(10):
        obj = zlib.compressobj( level )
        d = obj.compress( buf[:3000] )
        d = d + obj.flush( sync )
        d = d + obj.compress( buf[3000:] )
        d = d + obj.flush()
        if zlib.decompress(d) != buf:
            print "Decompress failed: flush mode=%i, level=%i" % (sync,level)
        del obj

def ignore():
    """An empty function with a big string.

    Make the compression algorithm work a little harder.
    """

    """
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

