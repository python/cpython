import unittest
import pickle
import pickletools

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

# DATA0 .. DATA2 are the pickles we expect under the various protocols, for
# the object returned by create_data().
# XXX DATA2 doesn't exist yet, as it's not fully implemented in cPickle.

# break into multiple strings to avoid confusing font-lock-mode
DATA0 = """(lp1
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

# Disassembly of DATA0.
DATA0_DIS = """\
    0: (    MARK
    1: l        LIST       (MARK at 0)
    2: p    PUT        1
    5: I    INT        0
    8: a    APPEND
    9: L    LONG       1L
   13: a    APPEND
   14: F    FLOAT      2.0
   17: a    APPEND
   18: c    GLOBAL     '__builtin__ complex'
   39: p    PUT        2
   42: (    MARK
   43: F        FLOAT      3.0
   46: F        FLOAT      0.0
   49: t        TUPLE      (MARK at 42)
   50: R    REDUCE
   51: p    PUT        3
   54: a    APPEND
   55: I    INT        1
   58: a    APPEND
   59: I    INT        -1
   63: a    APPEND
   64: I    INT        255
   69: a    APPEND
   70: I    INT        -255
   76: a    APPEND
   77: I    INT        -256
   83: a    APPEND
   84: I    INT        65535
   91: a    APPEND
   92: I    INT        -65535
  100: a    APPEND
  101: I    INT        -65536
  109: a    APPEND
  110: I    INT        2147483647
  122: a    APPEND
  123: I    INT        -2147483647
  136: a    APPEND
  137: I    INT        -2147483648
  150: a    APPEND
  151: (    MARK
  152: S        STRING     'abc'
  159: p        PUT        4
  162: g        GET        4
  165: (        MARK
  166: i            INST       '__main__ C' (MARK at 165)
  178: p        PUT        5
  181: (        MARK
  182: d            DICT       (MARK at 181)
  183: p        PUT        6
  186: S        STRING     'foo'
  193: p        PUT        7
  196: I        INT        1
  199: s        SETITEM
  200: S        STRING     'bar'
  207: p        PUT        8
  210: I        INT        2
  213: s        SETITEM
  214: b        BUILD
  215: g        GET        5
  218: t        TUPLE      (MARK at 151)
  219: p    PUT        9
  222: a    APPEND
  223: g    GET        9
  226: a    APPEND
  227: I    INT        5
  230: a    APPEND
  231: .    STOP
highest protocol among opcodes = 0
"""

DATA1 = (']q\x01(K\x00L1L\nG@\x00\x00\x00\x00\x00\x00\x00'
         'c__builtin__\ncomplex\nq\x02(G@\x08\x00\x00\x00\x00\x00'
         '\x00G\x00\x00\x00\x00\x00\x00\x00\x00tRq\x03K\x01J\xff\xff'
         '\xff\xffK\xffJ\x01\xff\xff\xffJ\x00\xff\xff\xffM\xff\xff'
         'J\x01\x00\xff\xffJ\x00\x00\xff\xffJ\xff\xff\xff\x7fJ\x01\x00'
         '\x00\x80J\x00\x00\x00\x80(U\x03abcq\x04h\x04(c__main__\n'
         'C\nq\x05oq\x06}q\x07(U\x03fooq\x08K\x01U\x03barq\tK\x02ubh'
         '\x06tq\nh\nK\x05e.'
        )

# Disassembly of DATA1.
DATA1_DIS = """\
    0: ]    EMPTY_LIST
    1: q    BINPUT     1
    3: (    MARK
    4: K        BININT1    0
    6: L        LONG       1L
   10: G        BINFLOAT   2.0
   19: c        GLOBAL     '__builtin__ complex'
   40: q        BINPUT     2
   42: (        MARK
   43: G            BINFLOAT   3.0
   52: G            BINFLOAT   0.0
   61: t            TUPLE      (MARK at 42)
   62: R        REDUCE
   63: q        BINPUT     3
   65: K        BININT1    1
   67: J        BININT     -1
   72: K        BININT1    255
   74: J        BININT     -255
   79: J        BININT     -256
   84: M        BININT2    65535
   87: J        BININT     -65535
   92: J        BININT     -65536
   97: J        BININT     2147483647
  102: J        BININT     -2147483647
  107: J        BININT     -2147483648
  112: (        MARK
  113: U            SHORT_BINSTRING 'abc'
  118: q            BINPUT     4
  120: h            BINGET     4
  122: (            MARK
  123: c                GLOBAL     '__main__ C'
  135: q                BINPUT     5
  137: o                OBJ        (MARK at 122)
  138: q            BINPUT     6
  140: }            EMPTY_DICT
  141: q            BINPUT     7
  143: (            MARK
  144: U                SHORT_BINSTRING 'foo'
  149: q                BINPUT     8
  151: K                BININT1    1
  153: U                SHORT_BINSTRING 'bar'
  158: q                BINPUT     9
  160: K                BININT1    2
  162: u                SETITEMS   (MARK at 143)
  163: b            BUILD
  164: h            BINGET     6
  166: t            TUPLE      (MARK at 112)
  167: q        BINPUT     10
  169: h        BINGET     10
  171: K        BININT1    5
  173: e        APPENDS    (MARK at 3)
  174: .    STOP
highest protocol among opcodes = 1
"""

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
    # Subclass must define self.dumps, self.loads, self.error.

    _testdata = create_data()

    def setUp(self):
        pass

    # Return True if opcode code appears in the pickle, else False.
    def opcode_in_pickle(self, code, pickle):
        for op, dummy, dummy in pickletools.genops(pickle):
            if op.code == code:
                return True
        return False

    def test_misc(self):
        # test various datatypes not tested by testdata
        for proto in protocols:
            x = myint(4)
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)

            x = (1, ())
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)

            x = initarg(1, x)
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)

        # XXX test __reduce__ protocol?

    def test_roundtrip_equality(self):
        expected = self._testdata
        for proto in protocols:
            s = self.dumps(expected, proto)
            got = self.loads(s)
            self.assertEqual(expected, got)

    def test_load_from_canned_string(self):
        expected = self._testdata
        for canned in DATA0, DATA1:
            got = self.loads(canned)
            self.assertEqual(expected, got)

    # There are gratuitous differences between pickles produced by
    # pickle and cPickle, largely because cPickle starts PUT indices at
    # 1 and pickle starts them at 0.  See XXX comment in cPickle's put2() --
    # there's a comment with an exclamation point there whose meaning
    # is a mystery.  cPickle also suppresses PUT for objects with a refcount
    # of 1.
    def dont_test_disassembly(self):
        from cStringIO import StringIO
        from pickletools import dis

        for proto, expected in (0, DATA0_DIS), (1, DATA1_DIS):
            s = self.dumps(self._testdata, proto)
            filelike = StringIO()
            dis(s, out=filelike)
            got = filelike.getvalue()
            self.assertEqual(expected, got)

    def test_recursive_list(self):
        l = []
        l.append(l)
        for proto in protocols:
            s = self.dumps(l, proto)
            x = self.loads(s)
            self.assertEqual(x, l)
            self.assertEqual(x, x[0])
            self.assertEqual(id(x), id(x[0]))

    def test_recursive_dict(self):
        d = {}
        d[1] = d
        for proto in protocols:
            s = self.dumps(d, proto)
            x = self.loads(s)
            self.assertEqual(x, d)
            self.assertEqual(x[1], x)
            self.assertEqual(id(x[1]), id(x))

    def test_recursive_inst(self):
        i = C()
        i.attr = i
        for proto in protocols:
            s = self.dumps(i, 2)
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
        for proto in protocols:
            s = self.dumps(l, proto)
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
        for proto in protocols:
            s = self.dumps(a, proto)
            b = self.loads(s)
            self.assertEqual(a.__class__, b.__class__)

    def test_structseq(self):
        import time
        import os

        t = time.localtime()
        for proto in protocols:
            s = self.dumps(t, proto)
            u = self.loads(s)
            self.assertEqual(t, u)
            if hasattr(os, "stat"):
                t = os.stat(os.curdir)
                s = self.dumps(t, proto)
                u = self.loads(s)
                self.assertEqual(t, u)
            if hasattr(os, "statvfs"):
                t = os.statvfs(os.curdir)
                s = self.dumps(t, proto)
                u = self.loads(s)
                self.assertEqual(t, u)

    # Tests for protocol 2

    def test_proto(self):
        build_none = pickle.NONE + pickle.STOP
        for proto in protocols:
            expected = build_none
            if proto >= 2:
                expected = pickle.PROTO + chr(proto) + expected
            p = self.dumps(None, proto)
            self.assertEqual(p, expected)

        oob = protocols[-1] + 1     # a future protocol
        badpickle = pickle.PROTO + chr(oob) + build_none
        try:
            self.loads(badpickle)
        except ValueError, detail:
            self.failUnless(str(detail).startswith(
                                            "unsupported pickle protocol"))
        else:
            self.fail("expected bad protocol number to raise ValueError")

    def test_long1(self):
        x = 12345678910111213141516178920L
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)
            self.assertEqual(self.opcode_in_pickle(pickle.LONG1, s),
                             proto >= 2)

    def test_long4(self):
        x = 12345678910111213141516178920L << (256*8)
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)
            self.assertEqual(self.opcode_in_pickle(pickle.LONG4, s),
                             proto >= 2)

    def test_short_tuples(self):
        # Map (proto, len(tuple)) to expected opcode.
        expected_opcode = {(0, 0): pickle.TUPLE,
                           (0, 1): pickle.TUPLE,
                           (0, 2): pickle.TUPLE,
                           (0, 3): pickle.TUPLE,
                           (0, 4): pickle.TUPLE,

                           (1, 0): pickle.EMPTY_TUPLE,
                           (1, 1): pickle.TUPLE,
                           (1, 2): pickle.TUPLE,
                           (1, 3): pickle.TUPLE,
                           (1, 4): pickle.TUPLE,

                           (2, 0): pickle.EMPTY_TUPLE,
                           (2, 1): pickle.TUPLE1,
                           (2, 2): pickle.TUPLE2,
                           (2, 3): pickle.TUPLE3,
                           (2, 4): pickle.TUPLE,
                          }
        a = ()
        b = (1,)
        c = (1, 2)
        d = (1, 2, 3)
        e = (1, 2, 3, 4)
        for proto in protocols:
            for x in a, b, c, d, e:
                s = self.dumps(x, proto)
                y = self.loads(s)
                self.assertEqual(x, y, (proto, x, s, y))
                expected = expected_opcode[proto, len(x)]
                self.assertEqual(self.opcode_in_pickle(expected, s), True)

    def test_singletons(self):
        # Map (proto, singleton) to expected opcode.
        expected_opcode = {(0, None): pickle.NONE,
                           (1, None): pickle.NONE,
                           (2, None): pickle.NONE,

                           (0, True): pickle.INT,
                           (1, True): pickle.INT,
                           (2, True): pickle.NEWTRUE,

                           (0, False): pickle.INT,
                           (1, False): pickle.INT,
                           (2, False): pickle.NEWFALSE,
                          }
        for proto in protocols:
            for x in None, False, True:
                s = self.dumps(x, proto)
                y = self.loads(s)
                self.assert_(x is y, (proto, x, s, y))
                expected = expected_opcode[proto, x]
                self.assertEqual(self.opcode_in_pickle(expected, s), True)

    def test_newobj_tuple(self):
        x = MyTuple([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        s = self.dumps(x, 2)
        y = self.loads(s)
        self.assertEqual(tuple(x), tuple(y))
        self.assertEqual(x.__dict__, y.__dict__)

    def test_newobj_list(self):
        x = MyList([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        s = self.dumps(x, 2)
        y = self.loads(s)
        self.assertEqual(list(x), list(y))
        self.assertEqual(x.__dict__, y.__dict__)

    def test_newobj_generic(self):
        for proto in [0, 1, 2]:
            for C in myclasses:
                B = C.__base__
                x = C(C.sample)
                x.foo = 42
                s = self.dumps(x, proto)
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
