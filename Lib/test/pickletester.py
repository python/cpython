import unittest
from test_support import TestFailed, have_unicode

class C:
    def __cmp__(self, other):
        return cmp(self.__dict__, other.__dict__)

import __main__
__main__.C = C
C.__module__ = "__main__"

class myint(int):
    def __init__(self, x):
        self.str = str(x)

class initarg(C):

    __safe_for_unpickling__ = 1

    def __init__(self, a, b):
        self.a = a
        self.b = b

    def __getinitargs__(self):
        return self.a, self.b

class metaclass(type):
    pass

class use_metaclass(object):
    __metaclass__ = metaclass

# break into multiple strings to avoid confusing font-lock-mode
DATA = """(lp1
I0
aL1L
aF2
ac__builtin__
complex
p2
""" + \
"""(F3
F0
tRp3
aI1
aI-1
aI255
aI-255
aI-256
aI65535
aI-65535
aI-65536
aI2147483647
aI-2147483647
aI-2147483648
a""" + \
"""(S'abc'
p4
g4
""" + \
"""(i__main__
C
p5
""" + \
"""(dp6
S'foo'
p7
I1
sS'bar'
p8
I2
sbg5
tp9
ag9
aI5
a.
"""

BINDATA = ']q\x01(K\x00L1L\nG@\x00\x00\x00\x00\x00\x00\x00' + \
          'c__builtin__\ncomplex\nq\x02(G@\x08\x00\x00\x00\x00\x00' + \
          '\x00G\x00\x00\x00\x00\x00\x00\x00\x00tRq\x03K\x01J\xff\xff' + \
          '\xff\xffK\xffJ\x01\xff\xff\xffJ\x00\xff\xff\xffM\xff\xff' + \
          'J\x01\x00\xff\xffJ\x00\x00\xff\xffJ\xff\xff\xff\x7fJ\x01\x00' + \
          '\x00\x80J\x00\x00\x00\x80(U\x03abcq\x04h\x04(c__main__\n' + \
          'C\nq\x05oq\x06}q\x07(U\x03fooq\x08K\x01U\x03barq\tK\x02ubh' + \
          '\x06tq\nh\nK\x05e.'

def create_data():
    c = C()
    c.foo = 1
    c.bar = 2
    x = [0, 1L, 2.0, 3.0+0j]
    # Append some integer test cases at cPickle.c's internal size
    # cutoffs.
    uint1max = 0xff
    uint2max = 0xffff
    int4max = 0x7fffffff
    x.extend([1, -1,
              uint1max, -uint1max, -uint1max-1,
              uint2max, -uint2max, -uint2max-1,
               int4max,  -int4max,  -int4max-1])
    y = ('abc', 'abc', c, c)
    x.append(y)
    x.append(y)
    x.append(5)
    return x

class AbstractPickleTests(unittest.TestCase):

    _testdata = create_data()

    def setUp(self):
        # subclass must define self.dumps, self.loads, self.error
        pass

    def test_misc(self):
        # test various datatypes not tested by testdata
        x = myint(4)
        s = self.dumps(x)
        y = self.loads(s)
        self.assertEqual(x, y)

        x = (1, ())
        s = self.dumps(x)
        y = self.loads(s)
        self.assertEqual(x, y)

        x = initarg(1, x)
        s = self.dumps(x)
        y = self.loads(s)
        self.assertEqual(x, y)

        # XXX test __reduce__ protocol?

    def test_identity(self):
        s = self.dumps(self._testdata)
        x = self.loads(s)
        self.assertEqual(x, self._testdata)

    def test_constant(self):
        x = self.loads(DATA)
        self.assertEqual(x, self._testdata)
        x = self.loads(BINDATA)
        self.assertEqual(x, self._testdata)

    def test_binary(self):
        s = self.dumps(self._testdata, 1)
        x = self.loads(s)
        self.assertEqual(x, self._testdata)

    def test_recursive_list(self):
        l = []
        l.append(l)
        s = self.dumps(l)
        x = self.loads(s)
        self.assertEqual(x, l)
        self.assertEqual(x, x[0])
        self.assertEqual(id(x), id(x[0]))

    def test_recursive_dict(self):
        d = {}
        d[1] = d
        s = self.dumps(d)
        x = self.loads(s)
        self.assertEqual(x, d)
        self.assertEqual(x[1], x)
        self.assertEqual(id(x[1]), id(x))

    def test_recursive_inst(self):
        i = C()
        i.attr = i
        s = self.dumps(i)
        x = self.loads(s)
        self.assertEqual(x, i)
        self.assertEqual(x.attr, x)
        self.assertEqual(id(x.attr), id(x))

    def test_recursive_multi(self):
        l = []
        d = {1:l}
        i = C()
        i.attr = d
        l.append(i)
        s = self.dumps(l)
        x = self.loads(s)
        self.assertEqual(x, l)
        self.assertEqual(x[0], i)
        self.assertEqual(x[0].attr, d)
        self.assertEqual(x[0].attr[1], x)
        self.assertEqual(x[0].attr[1][0], i)
        self.assertEqual(x[0].attr[1][0].attr, d)

    def test_garyp(self):
        self.assertRaises(self.error, self.loads, 'garyp')

    def test_insecure_strings(self):
        insecure = ["abc", "2 + 2", # not quoted
                    "'abc' + 'def'", # not a single quoted string
                    "'abc", # quote is not closed
                    "'abc\"", # open quote and close quote don't match
                    "'abc'   ?", # junk after close quote
                    # some tests of the quoting rules
                    "'abc\"\''",
                    "'\\\\a\'\'\'\\\'\\\\\''",
                    ]
        for s in insecure:
            buf = "S" + s + "\012p0\012."
            self.assertRaises(ValueError, self.loads, buf)

    if have_unicode:
        def test_unicode(self):
            endcases = [unicode(''), unicode('<\\u>'), unicode('<\\\u1234>'),
                        unicode('<\n>'),  unicode('<\\>')]
            for u in endcases:
                p = self.dumps(u)
                u2 = self.loads(p)
                self.assertEqual(u2, u)

    def test_ints(self):
        import sys
        n = sys.maxint
        while n:
            for expected in (-n, n):
                s = self.dumps(expected)
                n2 = self.loads(s)
                self.assertEqual(expected, n2)
            n = n >> 1

    def test_maxint64(self):
        maxint64 = (1L << 63) - 1
        data = 'I' + str(maxint64) + '\n.'
        got = self.loads(data)
        self.assertEqual(got, maxint64)

        # Try too with a bogus literal.
        data = 'I' + str(maxint64) + 'JUNK\n.'
        self.assertRaises(ValueError, self.loads, data)

    def test_reduce(self):
        pass

    def test_getinitargs(self):
        pass

    def test_metaclass(self):
        a = use_metaclass()
        s = self.dumps(a)
        b = self.loads(s)
        self.assertEqual(a.__class__, b.__class__)

    def test_structseq(self):
        import time
        t = time.localtime()
        s = self.dumps(t)
        u = self.loads(s)
        self.assertEqual(t, u)
        import os
        if hasattr(os, "stat"):
            t = os.stat(os.curdir)
            s = self.dumps(t)
            u = self.loads(s)
            self.assertEqual(t, u)
        if hasattr(os, "statvfs"):
            t = os.statvfs(os.curdir)
            s = self.dumps(t)
            u = self.loads(s)
            self.assertEqual(t, u)

class AbstractPickleModuleTests(unittest.TestCase):

    def test_dump_closed_file(self):
        import tempfile, os
        fn = tempfile.mktemp()
        f = open(fn, "w")
        f.close()
        self.assertRaises(ValueError, self.module.dump, 123, f)
        os.remove(fn)

    def test_load_closed_file(self):
        import tempfile, os
        fn = tempfile.mktemp()
        f = open(fn, "w")
        f.close()
        self.assertRaises(ValueError, self.module.dump, 123, f)
        os.remove(fn)
