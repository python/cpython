import zlib
from test_support import TestFailed
import sys
import imp

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
combuf = ''.join(bufs)

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
decomp2 = ''.join(bufs)
if decomp2 != buf:
    print "decompressobj with init options failed"
else:
    print "decompressobj with init options succeeded"

print "should be '':", `deco.unconsumed_tail`

# Check a decompression object with max_length specified
deco = zlib.decompressobj(-12)
cb = combuf
bufs = []
while cb:
    max_length = 1 + len(cb)/10
    chunk = deco.decompress(cb, max_length)
    if len(chunk) > max_length:
        print 'chunk too big (%d>%d)' % (len(chunk),max_length)
    bufs.append(chunk)
    cb = deco.unconsumed_tail
bufs.append(deco.flush())
decomp2 = ''.join(buf)
if decomp2 != buf:
    print "max_length decompressobj failed"
else:
    print "max_length decompressobj succeeded"

# Misc tests of max_length
deco = zlib.decompressobj(-12)
try:
    deco.decompress("", -1)
except ValueError:
    pass
else:
    print "failed to raise value error on bad max_length"
print "unconsumed_tail should be '':", `deco.unconsumed_tail`

# Test flush() with the various options, using all the different levels
# in order to provide more variations.
sync_opt = ['Z_NO_FLUSH', 'Z_SYNC_FLUSH', 'Z_FULL_FLUSH']
sync_opt = [getattr(zlib, opt) for opt in sync_opt if hasattr(zlib, opt)]

for sync in sync_opt:
    for level in range(10):
        obj = zlib.compressobj( level )
        d = obj.compress( buf[:3000] )
        d = d + obj.flush( sync )
        d = d + obj.compress( buf[3000:] )
        d = d + obj.flush()
        if zlib.decompress(d) != buf:
            print "Decompress failed: flush mode=%i, level=%i" % (sync,level)
        del obj

# Test for the odd flushing bugs noted in 2.0, and hopefully fixed in 2.1

import random
random.seed(1)

print 'Testing on 17K of random data'

if hasattr(zlib, 'Z_SYNC_FLUSH'):

    # Create compressor and decompressor objects
    c=zlib.compressobj(9)
    d=zlib.decompressobj()

    # Try 17K of data
    # generate random data stream
    a=""
    for i in range(17*1024):
        a=a+chr(random.randint(0,255))

    # compress, sync-flush, and decompress
    t = d.decompress( c.compress(a)+c.flush(zlib.Z_SYNC_FLUSH) )

    # if decompressed data is different from the input data, choke.
    if len(t) != len(a):
        print len(a),len(t),len(d.unused_data)
        raise TestFailed, "output of 17K doesn't match"

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
