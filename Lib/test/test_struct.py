from test_support import TestFailed, verbose
import struct
## import pdb

def simple_err(func, *args):
    try:
        apply(func, args)
    except struct.error:
        pass
    else:
        raise TestFailed, "%s%s did not raise struct.error" % (
            func.__name__, args)
##      pdb.set_trace()

simple_err(struct.calcsize, 'Q')

sz = struct.calcsize('i')
if sz * 3 <> struct.calcsize('iii'):
    raise TestFailed, 'inconsistent sizes'

fmt = 'cbxxxxxxhhhhiillffd'
fmt3 = '3c3b18x12h6i6l6f3d'
sz = struct.calcsize(fmt)
sz3 = struct.calcsize(fmt3)
if sz * 3 <> sz3:
    raise TestFailed, 'inconsistent sizes (3*%s -> 3*%d = %d, %s -> %d)' % (
        `fmt`, sz, 3*sz, `fmt3`, sz3)

simple_err(struct.pack, 'iii', 3)
simple_err(struct.pack, 'i', 3, 3, 3)
simple_err(struct.pack, 'i', 'foo')
simple_err(struct.unpack, 'd', 'flap')
s = struct.pack('ii', 1, 2)
simple_err(struct.unpack, 'iii', s)
simple_err(struct.unpack, 'i', s)

c = 'a'
b = 1
h = 255
i = 65535
l = 65536
f = 3.1415
d = 3.1415

for prefix in ('', '@', '<', '>', '=', '!'):
    for format in ('xcbhilfd', 'xcBHILfd'):
        format = prefix + format
        if verbose:
            print "trying:", format
        s = struct.pack(format, c, b, h, i, l, f, d)
        cp, bp, hp, ip, lp, fp, dp = struct.unpack(format, s)
        if (cp <> c or bp <> b or hp <> h or ip <> i or lp <> l or
            int(100 * fp) <> int(100 * f) or int(100 * dp) <> int(100 * d)):
            # ^^^ calculate only to two decimal places
            raise TestFailed, "unpack/pack not transitive (%s, %s)" % (
                str(format), str((cp, bp, hp, ip, lp, fp, dp)))

# Test some of the new features in detail

# (format, argument, big-endian result, little-endian result, asymmetric)
tests = [
    ('c', 'a', 'a', 'a', 0),
    ('xc', 'a', '\0a', '\0a', 0),
    ('cx', 'a', 'a\0', 'a\0', 0),
    ('s', 'a', 'a', 'a', 0),
    ('0s', 'helloworld', '', '', 1),
    ('1s', 'helloworld', 'h', 'h', 1),
    ('9s', 'helloworld', 'helloworl', 'helloworl', 1),
    ('10s', 'helloworld', 'helloworld', 'helloworld', 0),
    ('11s', 'helloworld', 'helloworld\0', 'helloworld\0', 1),
    ('20s', 'helloworld', 'helloworld'+10*'\0', 'helloworld'+10*'\0', 1),
    ('b', 7, '\7', '\7', 0),
    ('b', -7, '\371', '\371', 0),
    ('B', 7, '\7', '\7', 0),
    ('B', 249, '\371', '\371', 0),
    ('h', 700, '\002\274', '\274\002', 0),
    ('h', -700, '\375D', 'D\375', 0),
    ('H', 700, '\002\274', '\274\002', 0),
    ('H', 0x10000-700, '\375D', 'D\375', 0),
    ('i', 70000000, '\004,\035\200', '\200\035,\004', 0),
    ('i', -70000000, '\373\323\342\200', '\200\342\323\373', 0),
    ('I', 70000000L, '\004,\035\200', '\200\035,\004', 0),
    ('I', 0x100000000L-70000000, '\373\323\342\200', '\200\342\323\373', 0),
    ('l', 70000000, '\004,\035\200', '\200\035,\004', 0),
    ('l', -70000000, '\373\323\342\200', '\200\342\323\373', 0),
    ('L', 70000000L, '\004,\035\200', '\200\035,\004', 0),
    ('L', 0x100000000L-70000000, '\373\323\342\200', '\200\342\323\373', 0),
    ('f', 2.0, '@\000\000\000', '\000\000\000@', 0),
    ('d', 2.0, '@\000\000\000\000\000\000\000',
               '\000\000\000\000\000\000\000@', 0),
    ('f', -2.0, '\300\000\000\000', '\000\000\000\300', 0),
    ('d', -2.0, '\300\000\000\000\000\000\000\000',
               '\000\000\000\000\000\000\000\300', 0),
]

def badpack(fmt, arg, got, exp):
    return 

def badunpack(fmt, arg, got, exp):
    return "unpack(%s, %s) -> (%s,) # expected (%s,)" % (
        `fmt`, `arg`, `got`, `exp`)

isbigendian = struct.pack('=h', 1) == '\0\1'

for fmt, arg, big, lil, asy in tests:
    if verbose:
        print `fmt`, `arg`, `big`, `lil`
    for (xfmt, exp) in [('>'+fmt, big), ('!'+fmt, big), ('<'+fmt, lil),
                        ('='+fmt, isbigendian and big or lil)]:
        res = struct.pack(xfmt, arg)
        if res != exp:
            raise TestFailed, "pack(%s, %s) -> %s # expected %s" % (
                `fmt`, `arg`, `res`, `exp`)
        n = struct.calcsize(xfmt)
        if n != len(res):
            raise TestFailed, "calcsize(%s) -> %d # expected %d" % (
                `xfmt`, n, len(res))
        rev = struct.unpack(xfmt, res)[0]
        if rev != arg and not asy:
            raise TestFailed, "unpack(%s, %s) -> (%s,) # expected (%s,)" % (
                `fmt`, `res`, `rev`, `arg`)
