import struct
## import pdb

def simple_err(func, *args):
    try:
	apply(func, args)
    except struct.error:
	pass
    else:
	print 'expected struct.error not caught'
## 	pdb.set_trace()

simple_err(struct.calcsize, 'Q')

sz = struct.calcsize('i')
if sz * 3 <> struct.calcsize('iii'):
    print 'inconsistent sizes'

sz = struct.calcsize('cbhilfd')
if sz * 3 <> struct.calcsize('3c3b3h3i3l3f3d'):
    print 'inconsistent sizes'

simple_err(struct.pack, 'iii', 3)
simple_err(struct.pack, 'i', 3, 3, 3)
simple_err(struct.pack, 'i', 'foo')
simple_err(struct.unpack, 'd', 'flap')
s = struct.pack('ii', 1, 2)
simple_err(struct.unpack, 'iii', s)
simple_err(struct.unpack, 'i', s)

c = 'a'
b = -1
h = 255
i = 65535
l = 65536
f = 3.1415
d = 3.1415

s = struct.pack('xcbhilfd', c, b, h, i, l, f, d)
cp, bp, hp, ip, lp, fp, dp = struct.unpack('xcbhilfd', s)
if cp <> c or bp <> b or hp <> h or ip <> i or lp <> l or \
   int(100 * fp) <> int(100 * f) or int(100 * dp) <> int(100 * d):
   # ^^^ calculate only to two decimal places
    print 'unpack/pack not transitive'
