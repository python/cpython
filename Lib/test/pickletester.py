import unittest
from test.test_support import TestFailed, have_unicode, TESTFN

# Tests that try a number of pickle protocols should have a
#     for proto in protocols:
# kind of outer loop.  Bump the 3 to 4 if/when protocol 3 is invented.
protocols = range(3)

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

    def __init__(self, a, b):
        self.a = a
        self.b = b

    def __getinitargs__(self):
        return self.a, self.b

class metaclass(type):
    pass

class use_metaclass(object):
    __metaclass__ = metaclass

# DATA and BINDATA are the protocol 0 and protocol 1 pickles of the object
# returned by create_data().

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
                    #"'abc' + 'def'", # not a single quoted string
                    "'abc", # quote is not closed
                    "'abc\"", # open quote and close quote don't match
                    "'abc'   ?", # junk after close quote
                    "'\\'", # trailing backslash
                    # some tests of the quoting rules
                    #"'abc\"\''",
                    #"'\\\\a\'\'\'\\\'\\\\\''",
                    ]
        for s in insecure:
            buf = "S" + s + "\012p0\012."
            self.assertRaises(ValueError, self.loads, buf)

    if have_unicode:
        def test_unicode(self):
            endcases = [unicode(''), unicode('<\\u>'), unicode('<\\\u1234>'),
                        unicode('<\n>'),  unicode('<\\>')]
            for proto in protocols:
                for u in endcases:
                    p = self.dumps(u, proto)
                    u2 = self.loads(p)
                    self.assertEqual(u2, u)

    def test_ints(self):
        import sys
        for proto in protocols:
            n = sys.maxint
            while n:
                for expected in (-n, n):
                    s = self.dumps(expected, proto)
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

    def test_long(self):
        for proto in protocols:
            # 256 bytes is where LONG4 begins.
            for nbits in 1, 8, 8*254, 8*255, 8*256, 8*257:
                nbase = 1L << nbits
                for npos in nbase-1, nbase, nbase+1:
                    for n in npos, -npos:
                        pickle = self.dumps(n, proto)
                        got = self.loads(pickle)
                        self.assertEqual(n, got)
        # Try a monster.  This is quadratic-time in protos 0 & 1, so don't
        # bother with those.
        nbase = long("deadbeeffeedface", 16)
        nbase += nbase << 1000000
        for n in nbase, -nbase:
            p = self.dumps(n, 2)
            got = self.loads(p)
            self.assertEqual(n, got)

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

    # Tests for protocol 2

    def test_long1(self):
        x = 12345678910111213141516178920L
        s = self.dumps(x, 2)
        y = self.loads(s)
        self.assertEqual(x, y)

    def test_long4(self):
        x = 12345678910111213141516178920L << (256*8)
        s = self.dumps(x, 2)
        y = self.loads(s)
        self.assertEqual(x, y)

    def test_short_tuples(self):
        a = ()
        b = (1,)
        c = (1, 2)
        d = (1, 2, 3)
        e = (1, 2, 3, 4)
        for proto in 0, 1, 2:
            for x in a, b, c, d, e:
                s = self.dumps(x, proto)
                y = self.loads(s)
                self.assertEqual(x, y, (proto, x, s, y))

    def test_singletons(self):
        for proto in 0, 1, 2:
            for x in None, False, True:
                s = self.dumps(x, proto)
                y = self.loads(s)
                self.assert_(x is y, (proto, x, s, y))

    def test_newobj_tuple(self):
        x = MyTuple([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        s = self.dumps(x, 2)
        y = self.loads(s)
        self.assertEqual(tuple(x), tuple(y))
        self.assertEqual(x.__dict__, y.__dict__)
##         import pickletools
##         print
##         pickletools.dis(s)

    def test_newobj_list(self):
        x = MyList([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        s = self.dumps(x, 2)
        y = self.loads(s)
        self.assertEqual(list(x), list(y))
        self.assertEqual(x.__dict__, y.__dict__)
##         import pickletools
##         print
##         pickletools.dis(s)

    def test_newobj_generic(self):
        for proto in [0, 1, 2]:
            for C in myclasses:
                B = C.__base__
                x = C(C.sample)
                x.foo = 42
                s = self.dumps(x, proto)
##                import pickletools
##                print
##                pickletools.dis(s)
                y = self.loads(s)
                detail = (proto, C, B, x, y, type(y))
                self.assertEqual(B(x), B(y), detail)
                self.assertEqual(x.__dict__, y.__dict__, detail)

# XXX Temporary hack, so long as the C implementation of pickle protocol
# XXX 2 isn't ready.  When it is, move the methods in TempAbstractPickleTests
# XXX into AbstractPickleTests above, and get rid of TempAbstractPickleTests
# XXX along with the references to it in test_pickle.py.
class TempAbstractPickleTests(unittest.TestCase):

    def test_newobj_list_slots(self):
        x = SlotList([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        s = self.dumps(x, 2)
        y = self.loads(s)
        self.assertEqual(list(x), list(y))
        self.assertEqual(x.__dict__, y.__dict__)
        self.assertEqual(x.foo, y.foo)
        self.assertEqual(x.bar, y.bar)
##         import pickletools
##         print
##         pickletools.dis(s)

    def test_global_ext1(self):
        import copy_reg
        copy_reg.add_extension(__name__, "MyList", 0xf0)
        try:
            x = MyList([1, 2, 3])
            x.foo = 42
            x.bar = "hello"

            # Dump using protocol 1 for comparison
            s1 = self.dumps(x, 1)
            y = self.loads(s1)
            self.assertEqual(list(x), list(y))
            self.assertEqual(x.__dict__, y.__dict__)
            self.assert_(s1.find(__name__) >= 0)
            self.assert_(s1.find("MyList") >= 0)
##            import pickletools
##            print
##            pickletools.dis(s1)

            # Dump using protocol 2 for test
            s2 = self.dumps(x, 2)
            self.assertEqual(s2.find(__name__), -1)
            self.assertEqual(s2.find("MyList"), -1)
            y = self.loads(s2)
            self.assertEqual(list(x), list(y))
            self.assertEqual(x.__dict__, y.__dict__)
##            import pickletools
##            print
##            pickletools.dis(s2)

        finally:
            copy_reg.remove_extension(__name__, "MyList", 0xf0)

    def test_global_ext2(self):
        import copy_reg
        copy_reg.add_extension(__name__, "MyList", 0xfff0)
        try:
            x = MyList()
            s2 = self.dumps(x, 2)
            self.assertEqual(s2.find(__name__), -1)
            self.assertEqual(s2.find("MyList"), -1)
            y = self.loads(s2)
            self.assertEqual(list(x), list(y))
            self.assertEqual(x.__dict__, y.__dict__)
        finally:
            copy_reg.remove_extension(__name__, "MyList", 0xfff0)

    def test_global_ext4(self):
        import copy_reg
        copy_reg.add_extension(__name__, "MyList", 0xfffff0)
        try:
            x = MyList()
            s2 = self.dumps(x, 2)
            self.assertEqual(s2.find(__name__), -1)
            self.assertEqual(s2.find("MyList"), -1)
            y = self.loads(s2)
            self.assertEqual(list(x), list(y))
            self.assertEqual(x.__dict__, y.__dict__)
        finally:
            copy_reg.remove_extension(__name__, "MyList", 0xfffff0)

class MyInt(int):
    sample = 1

class MyLong(long):
    sample = 1L

class MyFloat(float):
    sample = 1.0

class MyComplex(complex):
    sample = 1.0 + 0.0j

class MyStr(str):
    sample = "hello"

class MyUnicode(unicode):
    sample = u"hello \u1234"

class MyTuple(tuple):
    sample = (1, 2, 3)

class MyList(list):
    sample = [1, 2, 3]

class MyDict(dict):
    sample = {"a": 1, "b": 2}

myclasses = [MyInt, MyLong, MyFloat,
             # MyComplex, # XXX complex somehow doesn't work here :-(
             MyStr, MyUnicode,
             MyTuple, MyList, MyDict]


class SlotList(MyList):
    __slots__ = ["foo"]

class AbstractPickleModuleTests(unittest.TestCase):

    def test_dump_closed_file(self):
        import os
        f = open(TESTFN, "w")
        try:
            f.close()
            self.assertRaises(ValueError, self.module.dump, 123, f)
        finally:
            os.remove(TESTFN)

    def test_load_closed_file(self):
        import os
        f = open(TESTFN, "w")
        try:
            f.close()
            self.assertRaises(ValueError, self.module.dump, 123, f)
        finally:
            os.remove(TESTFN)

class AbstractPersistentPicklerTests(unittest.TestCase):

    # This class defines persistent_id() and persistent_load()
    # functions that should be used by the pickler.  All even integers
    # are pickled using persistent ids.

    def persistent_id(self, object):
        if isinstance(object, int) and object % 2 == 0:
            self.id_count += 1
            return str(object)
        else:
            return None

    def persistent_load(self, oid):
        self.load_count += 1
        object = int(oid)
        assert object % 2 == 0
        return object

    def test_persistence(self):
        self.id_count = 0
        self.load_count = 0
        L = range(10)
        self.assertEqual(self.loads(self.dumps(L)), L)
        self.assertEqual(self.id_count, 5)
        self.assertEqual(self.load_count, 5)

    def test_bin_persistence(self):
        self.id_count = 0
        self.load_count = 0
        L = range(10)
        self.assertEqual(self.loads(self.dumps(L, 1)), L)
        self.assertEqual(self.id_count, 5)
        self.assertEqual(self.load_count, 5)
