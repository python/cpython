import unittest
import pickle
import pickletools
import copy_reg

from test.test_support import TestFailed, TESTFN, run_with_locale

from pickle import bytes_types

# Tests that try a number of pickle protocols should have a
#     for proto in protocols:
# kind of outer loop.
protocols = range(pickle.HIGHEST_PROTOCOL + 1)


# Return True if opcode code appears in the pickle, else False.
def opcode_in_pickle(code, pickle):
    for op, dummy, dummy in pickletools.genops(pickle):
        if op.code == code.decode("latin-1"):
            return True
    return False

# Return the number of times opcode code appears in pickle.
def count_opcode(code, pickle):
    n = 0
    for op, dummy, dummy in pickletools.genops(pickle):
        if op.code == code.decode("latin-1"):
            n += 1
    return n

# We can't very well test the extension registry without putting known stuff
# in it, but we have to be careful to restore its original state.  Code
# should do this:
#
#     e = ExtensionSaver(extension_code)
#     try:
#         fiddle w/ the extension registry's stuff for extension_code
#     finally:
#         e.restore()

class ExtensionSaver:
    # Remember current registration for code (if any), and remove it (if
    # there is one).
    def __init__(self, code):
        self.code = code
        if code in copy_reg._inverted_registry:
            self.pair = copy_reg._inverted_registry[code]
            copy_reg.remove_extension(self.pair[0], self.pair[1], code)
        else:
            self.pair = None

    # Restore previous registration for code.
    def restore(self):
        code = self.code
        curpair = copy_reg._inverted_registry.get(code)
        if curpair is not None:
            copy_reg.remove_extension(curpair[0], curpair[1], code)
        pair = self.pair
        if pair is not None:
            copy_reg.add_extension(pair[0], pair[1], code)

class C:
    def __eq__(self, other):
        return self.__dict__ == other.__dict__

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

class use_metaclass(object, metaclass=metaclass):
    pass

# DATA0 .. DATA2 are the pickles we expect under the various protocols, for
# the object returned by create_data().

DATA0 = (
    b'(lp0\nL0\naL1\naF2.0\nac'
    b'builtins\ncomplex\n'
    b'p1\n(F3.0\nF0.0\ntp2\nRp'
    b'3\naL1\naL-1\naL255\naL-'
    b'255\naL-256\naL65535\na'
    b'L-65535\naL-65536\naL2'
    b'147483647\naL-2147483'
    b'647\naL-2147483648\na('
    b'Vabc\np4\ng4\nccopy_reg'
    b'\n_reconstructor\np5\n('
    b'c__main__\nC\np6\ncbu'
    b'iltins\nobject\np7\nNt'
    b'p8\nRp9\n(dp10\nVfoo\np1'
    b'1\nL1\nsVbar\np12\nL2\nsb'
    b'g9\ntp13\nag13\naL5\na.'
)

# Disassembly of DATA0
DATA0_DIS = """\
    0: (    MARK
    1: l        LIST       (MARK at 0)
    2: p    PUT        0
    5: L    LONG       0
    8: a    APPEND
    9: L    LONG       1
   12: a    APPEND
   13: F    FLOAT      2.0
   18: a    APPEND
   19: c    GLOBAL     'builtins complex'
   40: p    PUT        1
   43: (    MARK
   44: F        FLOAT      3.0
   49: F        FLOAT      0.0
   54: t        TUPLE      (MARK at 43)
   55: p    PUT        2
   58: R    REDUCE
   59: p    PUT        3
   62: a    APPEND
   63: L    LONG       1
   66: a    APPEND
   67: L    LONG       -1
   71: a    APPEND
   72: L    LONG       255
   77: a    APPEND
   78: L    LONG       -255
   84: a    APPEND
   85: L    LONG       -256
   91: a    APPEND
   92: L    LONG       65535
   99: a    APPEND
  100: L    LONG       -65535
  108: a    APPEND
  109: L    LONG       -65536
  117: a    APPEND
  118: L    LONG       2147483647
  130: a    APPEND
  131: L    LONG       -2147483647
  144: a    APPEND
  145: L    LONG       -2147483648
  158: a    APPEND
  159: (    MARK
  160: V        UNICODE    'abc'
  165: p        PUT        4
  168: g        GET        4
  171: c        GLOBAL     'copy_reg _reconstructor'
  196: p        PUT        5
  199: (        MARK
  200: c            GLOBAL     '__main__ C'
  212: p            PUT        6
  215: c            GLOBAL     'builtins object'
  235: p            PUT        7
  238: N            NONE
  239: t            TUPLE      (MARK at 199)
  240: p        PUT        8
  243: R        REDUCE
  244: p        PUT        9
  247: (        MARK
  248: d            DICT       (MARK at 247)
  249: p        PUT        10
  253: V        UNICODE    'foo'
  258: p        PUT        11
  262: L        LONG       1
  265: s        SETITEM
  266: V        UNICODE    'bar'
  271: p        PUT        12
  275: L        LONG       2
  278: s        SETITEM
  279: b        BUILD
  280: g        GET        9
  283: t        TUPLE      (MARK at 159)
  284: p    PUT        13
  288: a    APPEND
  289: g    GET        13
  293: a    APPEND
  294: L    LONG       5
  297: a    APPEND
  298: .    STOP
highest protocol among opcodes = 0
"""

DATA1 = (
    b']q\x00(K\x00K\x01G@\x00\x00\x00\x00\x00\x00\x00c'
    b'builtins\ncomplex\nq\x01'
    b'(G@\x08\x00\x00\x00\x00\x00\x00G\x00\x00\x00\x00\x00\x00\x00\x00t'
    b'q\x02Rq\x03K\x01J\xff\xff\xff\xffK\xffJ\x01\xff\xff\xffJ'
    b'\x00\xff\xff\xffM\xff\xffJ\x01\x00\xff\xffJ\x00\x00\xff\xffJ\xff\xff'
    b'\xff\x7fJ\x01\x00\x00\x80J\x00\x00\x00\x80(X\x03\x00\x00\x00ab'
    b'cq\x04h\x04ccopy_reg\n_reco'
    b'nstructor\nq\x05(c__main'
    b'__\nC\nq\x06cbuiltins\n'
    b'object\nq\x07Ntq\x08Rq\t}q\n('
    b'X\x03\x00\x00\x00fooq\x0bK\x01X\x03\x00\x00\x00bar'
    b'q\x0cK\x02ubh\ttq\rh\rK\x05e.'
)

# Disassembly of DATA1
DATA1_DIS = """\
    0: ]    EMPTY_LIST
    1: q    BINPUT     0
    3: (    MARK
    4: K        BININT1    0
    6: K        BININT1    1
    8: G        BINFLOAT   2.0
   17: c        GLOBAL     'builtins complex'
   38: q        BINPUT     1
   40: (        MARK
   41: G            BINFLOAT   3.0
   50: G            BINFLOAT   0.0
   59: t            TUPLE      (MARK at 40)
   60: q        BINPUT     2
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
  113: X            BINUNICODE 'abc'
  121: q            BINPUT     4
  123: h            BINGET     4
  125: c            GLOBAL     'copy_reg _reconstructor'
  150: q            BINPUT     5
  152: (            MARK
  153: c                GLOBAL     '__main__ C'
  165: q                BINPUT     6
  167: c                GLOBAL     'builtins object'
  187: q                BINPUT     7
  189: N                NONE
  190: t                TUPLE      (MARK at 152)
  191: q            BINPUT     8
  193: R            REDUCE
  194: q            BINPUT     9
  196: }            EMPTY_DICT
  197: q            BINPUT     10
  199: (            MARK
  200: X                BINUNICODE 'foo'
  208: q                BINPUT     11
  210: K                BININT1    1
  212: X                BINUNICODE 'bar'
  220: q                BINPUT     12
  222: K                BININT1    2
  224: u                SETITEMS   (MARK at 199)
  225: b            BUILD
  226: h            BINGET     9
  228: t            TUPLE      (MARK at 112)
  229: q        BINPUT     13
  231: h        BINGET     13
  233: K        BININT1    5
  235: e        APPENDS    (MARK at 3)
  236: .    STOP
highest protocol among opcodes = 1
"""

DATA2 = (
    b'\x80\x02]q\x00(K\x00K\x01G@\x00\x00\x00\x00\x00\x00\x00c'
    b'builtins\ncomplex\n'
    b'q\x01G@\x08\x00\x00\x00\x00\x00\x00G\x00\x00\x00\x00\x00\x00\x00\x00'
    b'\x86q\x02Rq\x03K\x01J\xff\xff\xff\xffK\xffJ\x01\xff\xff\xff'
    b'J\x00\xff\xff\xffM\xff\xffJ\x01\x00\xff\xffJ\x00\x00\xff\xffJ\xff'
    b'\xff\xff\x7fJ\x01\x00\x00\x80J\x00\x00\x00\x80(X\x03\x00\x00\x00a'
    b'bcq\x04h\x04c__main__\nC\nq\x05'
    b')\x81q\x06}q\x07(X\x03\x00\x00\x00fooq\x08K\x01'
    b'X\x03\x00\x00\x00barq\tK\x02ubh\x06tq\nh'
    b'\nK\x05e.'
)

# Disassembly of DATA2
DATA2_DIS = """\
    0: \x80 PROTO      2
    2: ]    EMPTY_LIST
    3: q    BINPUT     0
    5: (    MARK
    6: K        BININT1    0
    8: K        BININT1    1
   10: G        BINFLOAT   2.0
   19: c        GLOBAL     'builtins complex'
   40: q        BINPUT     1
   42: G        BINFLOAT   3.0
   51: G        BINFLOAT   0.0
   60: \x86     TUPLE2
   61: q        BINPUT     2
   63: R        REDUCE
   64: q        BINPUT     3
   66: K        BININT1    1
   68: J        BININT     -1
   73: K        BININT1    255
   75: J        BININT     -255
   80: J        BININT     -256
   85: M        BININT2    65535
   88: J        BININT     -65535
   93: J        BININT     -65536
   98: J        BININT     2147483647
  103: J        BININT     -2147483647
  108: J        BININT     -2147483648
  113: (        MARK
  114: X            BINUNICODE 'abc'
  122: q            BINPUT     4
  124: h            BINGET     4
  126: c            GLOBAL     '__main__ C'
  138: q            BINPUT     5
  140: )            EMPTY_TUPLE
  141: \x81         NEWOBJ
  142: q            BINPUT     6
  144: }            EMPTY_DICT
  145: q            BINPUT     7
  147: (            MARK
  148: X                BINUNICODE 'foo'
  156: q                BINPUT     8
  158: K                BININT1    1
  160: X                BINUNICODE 'bar'
  168: q                BINPUT     9
  170: K                BININT1    2
  172: u                SETITEMS   (MARK at 147)
  173: b            BUILD
  174: h            BINGET     6
  176: t            TUPLE      (MARK at 113)
  177: q        BINPUT     10
  179: h        BINGET     10
  181: K        BININT1    5
  183: e        APPENDS    (MARK at 5)
  184: .    STOP
highest protocol among opcodes = 2
"""

def create_data():
    c = C()
    c.foo = 1
    c.bar = 2
    x = [0, 1, 2.0, 3.0+0j]
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

    def test_load_from_data0(self):
        self.assertEqual(self._testdata, self.loads(DATA0))

    def test_load_from_data1(self):
        self.assertEqual(self._testdata, self.loads(DATA1))

    def test_load_from_data2(self):
        self.assertEqual(self._testdata, self.loads(DATA2))

    # There are gratuitous differences between pickles produced by
    # pickle and cPickle, largely because cPickle starts PUT indices at
    # 1 and pickle starts them at 0.  See XXX comment in cPickle's put2() --
    # there's a comment with an exclamation point there whose meaning
    # is a mystery.  cPickle also suppresses PUT for objects with a refcount
    # of 1.
    def dont_test_disassembly(self):
        from io import StringIO
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
            self.assertEqual(len(x), 1)
            self.assert_(x is x[0])

    def test_recursive_dict(self):
        d = {}
        d[1] = d
        for proto in protocols:
            s = self.dumps(d, proto)
            x = self.loads(s)
            self.assertEqual(list(x.keys()), [1])
            self.assert_(x[1] is x)

    def test_recursive_inst(self):
        i = C()
        i.attr = i
        for proto in protocols:
            s = self.dumps(i, 2)
            x = self.loads(s)
            self.assertEqual(dir(x), dir(i))
            self.assert_(x.attr is x)

    def test_recursive_multi(self):
        l = []
        d = {1:l}
        i = C()
        i.attr = d
        l.append(i)
        for proto in protocols:
            s = self.dumps(l, proto)
            x = self.loads(s)
            self.assertEqual(len(x), 1)
            self.assertEqual(dir(x[0]), dir(i))
            self.assertEqual(list(x[0].attr.keys()), [1])
            self.assert_(x[0].attr[1] is x)

    def test_garyp(self):
        self.assertRaises(self.error, self.loads, b'garyp')

    def test_insecure_strings(self):
        # XXX Some of these tests are temporarily disabled
        insecure = [b"abc", b"2 + 2", # not quoted
                    ## b"'abc' + 'def'", # not a single quoted string
                    b"'abc", # quote is not closed
                    b"'abc\"", # open quote and close quote don't match
                    b"'abc'   ?", # junk after close quote
                    b"'\\'", # trailing backslash
                    # some tests of the quoting rules
                    ## b"'abc\"\''",
                    ## b"'\\\\a\'\'\'\\\'\\\\\''",
                    ]
        for b in insecure:
            buf = b"S" + b + b"\012p0\012."
            self.assertRaises(ValueError, self.loads, buf)

    def test_unicode(self):
        endcases = ['', '<\\u>', '<\\\u1234>', '<\n>',  '<\\>']
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
        maxint64 = (1 << 63) - 1
        data = b'I' + str(maxint64).encode("ascii") + b'\n.'
        got = self.loads(data)
        self.assertEqual(got, maxint64)

        # Try too with a bogus literal.
        data = b'I' + str(maxint64).encode("ascii") + b'JUNK\n.'
        self.assertRaises(ValueError, self.loads, data)

    def test_long(self):
        for proto in protocols:
            # 256 bytes is where LONG4 begins.
            for nbits in 1, 8, 8*254, 8*255, 8*256, 8*257:
                nbase = 1 << nbits
                for npos in nbase-1, nbase, nbase+1:
                    for n in npos, -npos:
                        pickle = self.dumps(n, proto)
                        got = self.loads(pickle)
                        self.assertEqual(n, got)
        # Try a monster.  This is quadratic-time in protos 0 & 1, so don't
        # bother with those.
        nbase = int("deadbeeffeedface", 16)
        nbase += nbase << 1000000
        for n in nbase, -nbase:
            p = self.dumps(n, 2)
            got = self.loads(p)
            self.assertEqual(n, got)

    @run_with_locale('LC_ALL', 'de_DE', 'fr_FR')
    def test_float_format(self):
        # make sure that floats are formatted locale independent
        self.assertEqual(self.dumps(1.2)[0:3], b'F1.')

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
                expected = pickle.PROTO + bytes([proto]) + expected
            p = self.dumps(None, proto)
            self.assertEqual(p, expected)

        oob = protocols[-1] + 1     # a future protocol
        badpickle = pickle.PROTO + bytes([oob]) + build_none
        try:
            self.loads(badpickle)
        except ValueError as detail:
            self.failUnless(str(detail).startswith(
                                            "unsupported pickle protocol"))
        else:
            self.fail("expected bad protocol number to raise ValueError")

    def test_long1(self):
        x = 12345678910111213141516178920
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)
            self.assertEqual(opcode_in_pickle(pickle.LONG1, s), proto >= 2)

    def test_long4(self):
        x = 12345678910111213141516178920 << (256*8)
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)
            self.assertEqual(opcode_in_pickle(pickle.LONG4, s), proto >= 2)

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
                self.assertEqual(opcode_in_pickle(expected, s), True)

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
                self.assertEqual(opcode_in_pickle(expected, s), True)

    def test_newobj_tuple(self):
        x = MyTuple([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(tuple(x), tuple(y))
            self.assertEqual(x.__dict__, y.__dict__)

    def test_newobj_list(self):
        x = MyList([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(list(x), list(y))
            self.assertEqual(x.__dict__, y.__dict__)

    def test_newobj_generic(self):
        for proto in protocols:
            for C in myclasses:
                B = C.__base__
                x = C(C.sample)
                x.foo = 42
                s = self.dumps(x, proto)
                y = self.loads(s)
                detail = (proto, C, B, x, y, type(y))
                self.assertEqual(B(x), B(y), detail)
                self.assertEqual(x.__dict__, y.__dict__, detail)

    # Register a type with copy_reg, with extension code extcode.  Pickle
    # an object of that type.  Check that the resulting pickle uses opcode
    # (EXT[124]) under proto 2, and not in proto 1.

    def produce_global_ext(self, extcode, opcode):
        e = ExtensionSaver(extcode)
        try:
            copy_reg.add_extension(__name__, "MyList", extcode)
            x = MyList([1, 2, 3])
            x.foo = 42
            x.bar = "hello"

            # Dump using protocol 1 for comparison.
            s1 = self.dumps(x, 1)
            self.assert_(__name__.encode("utf-8") in s1)
            self.assert_(b"MyList" in s1)
            self.assertEqual(opcode_in_pickle(opcode, s1), False)

            y = self.loads(s1)
            self.assertEqual(list(x), list(y))
            self.assertEqual(x.__dict__, y.__dict__)

            # Dump using protocol 2 for test.
            s2 = self.dumps(x, 2)
            self.assert_(__name__.encode("utf-8") not in s2)
            self.assert_(b"MyList" not in s2)
            self.assertEqual(opcode_in_pickle(opcode, s2), True, repr(s2))

            y = self.loads(s2)
            self.assertEqual(list(x), list(y))
            self.assertEqual(x.__dict__, y.__dict__)

        finally:
            e.restore()

    def test_global_ext1(self):
        self.produce_global_ext(0x00000001, pickle.EXT1)  # smallest EXT1 code
        self.produce_global_ext(0x000000ff, pickle.EXT1)  # largest EXT1 code

    def test_global_ext2(self):
        self.produce_global_ext(0x00000100, pickle.EXT2)  # smallest EXT2 code
        self.produce_global_ext(0x0000ffff, pickle.EXT2)  # largest EXT2 code
        self.produce_global_ext(0x0000abcd, pickle.EXT2)  # check endianness

    def test_global_ext4(self):
        self.produce_global_ext(0x00010000, pickle.EXT4)  # smallest EXT4 code
        self.produce_global_ext(0x7fffffff, pickle.EXT4)  # largest EXT4 code
        self.produce_global_ext(0x12abcdef, pickle.EXT4)  # check endianness

    def test_list_chunking(self):
        n = 10  # too small to chunk
        x = list(range(n))
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)
            num_appends = count_opcode(pickle.APPENDS, s)
            self.assertEqual(num_appends, proto > 0)

        n = 2500  # expect at least two chunks when proto > 0
        x = list(range(n))
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)
            num_appends = count_opcode(pickle.APPENDS, s)
            if proto == 0:
                self.assertEqual(num_appends, 0)
            else:
                self.failUnless(num_appends >= 2)

    def test_dict_chunking(self):
        n = 10  # too small to chunk
        x = dict.fromkeys(range(n))
        for proto in protocols:
            s = self.dumps(x, proto)
            assert isinstance(s, bytes_types)
            y = self.loads(s)
            self.assertEqual(x, y)
            num_setitems = count_opcode(pickle.SETITEMS, s)
            self.assertEqual(num_setitems, proto > 0)

        n = 2500  # expect at least two chunks when proto > 0
        x = dict.fromkeys(range(n))
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assertEqual(x, y)
            num_setitems = count_opcode(pickle.SETITEMS, s)
            if proto == 0:
                self.assertEqual(num_setitems, 0)
            else:
                self.failUnless(num_setitems >= 2)

    def test_simple_newobj(self):
        x = object.__new__(SimpleNewObj)  # avoid __init__
        x.abc = 666
        for proto in protocols:
            s = self.dumps(x, proto)
            self.assertEqual(opcode_in_pickle(pickle.NEWOBJ, s), proto >= 2)
            y = self.loads(s)   # will raise TypeError if __init__ called
            self.assertEqual(y.abc, 666)
            self.assertEqual(x.__dict__, y.__dict__)

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

    def test_reduce_overrides_default_reduce_ex(self):
        for proto in 0, 1, 2:
            x = REX_one()
            self.assertEqual(x._reduce_called, 0)
            s = self.dumps(x, proto)
            self.assertEqual(x._reduce_called, 1)
            y = self.loads(s)
            self.assertEqual(y._reduce_called, 0)

    def test_reduce_ex_called(self):
        for proto in 0, 1, 2:
            x = REX_two()
            self.assertEqual(x._proto, None)
            s = self.dumps(x, proto)
            self.assertEqual(x._proto, proto)
            y = self.loads(s)
            self.assertEqual(y._proto, None)

    def test_reduce_ex_overrides_reduce(self):
        for proto in 0, 1, 2:
            x = REX_three()
            self.assertEqual(x._proto, None)
            s = self.dumps(x, proto)
            self.assertEqual(x._proto, proto)
            y = self.loads(s)
            self.assertEqual(y._proto, None)

    def test_reduce_ex_calls_base(self):
        for proto in 0, 1, 2:
            x = REX_four()
            self.assertEqual(x._proto, None)
            s = self.dumps(x, proto)
            self.assertEqual(x._proto, proto)
            y = self.loads(s)
            self.assertEqual(y._proto, proto)

    def test_reduce_calls_base(self):
        for proto in 0, 1, 2:
            x = REX_five()
            self.assertEqual(x._reduce_called, 0)
            s = self.dumps(x, proto)
            self.assertEqual(x._reduce_called, 1)
            y = self.loads(s)
            self.assertEqual(y._reduce_called, 1)

# Test classes for reduce_ex

class REX_one(object):
    _reduce_called = 0
    def __reduce__(self):
        self._reduce_called = 1
        return REX_one, ()
    # No __reduce_ex__ here, but inheriting it from object

class REX_two(object):
    _proto = None
    def __reduce_ex__(self, proto):
        self._proto = proto
        return REX_two, ()
    # No __reduce__ here, but inheriting it from object

class REX_three(object):
    _proto = None
    def __reduce_ex__(self, proto):
        self._proto = proto
        return REX_two, ()
    def __reduce__(self):
        raise TestFailed("This __reduce__ shouldn't be called")

class REX_four(object):
    _proto = None
    def __reduce_ex__(self, proto):
        self._proto = proto
        return object.__reduce_ex__(self, proto)
    # Calling base class method should succeed

class REX_five(object):
    _reduce_called = 0
    def __reduce__(self):
        self._reduce_called = 1
        return object.__reduce__(self)
    # This one used to fail with infinite recursion

# Test classes for newobj

class MyInt(int):
    sample = 1

class MyLong(int):
    sample = 1

class MyFloat(float):
    sample = 1.0

class MyComplex(complex):
    sample = 1.0 + 0.0j

class MyStr(str):
    sample = "hello"

class MyUnicode(str):
    sample = "hello \u1234"

class MyTuple(tuple):
    sample = (1, 2, 3)

class MyList(list):
    sample = [1, 2, 3]

class MyDict(dict):
    sample = {"a": 1, "b": 2}

myclasses = [MyInt, MyLong, MyFloat,
             MyComplex,
             MyStr, MyUnicode,
             MyTuple, MyList, MyDict]


class SlotList(MyList):
    __slots__ = ["foo"]

class SimpleNewObj(object):
    def __init__(self, a, b, c):
        # raise an error, to make sure this isn't called
        raise TypeError("SimpleNewObj.__init__() didn't expect to get called")

class AbstractPickleModuleTests(unittest.TestCase):

    def test_dump_closed_file(self):
        import os
        f = open(TESTFN, "wb")
        try:
            f.close()
            self.assertRaises(ValueError, self.module.dump, 123, f)
        finally:
            os.remove(TESTFN)

    def test_load_closed_file(self):
        import os
        f = open(TESTFN, "wb")
        try:
            f.close()
            self.assertRaises(ValueError, self.module.dump, 123, f)
        finally:
            os.remove(TESTFN)

    def test_highest_protocol(self):
        # Of course this needs to be changed when HIGHEST_PROTOCOL changes.
        self.assertEqual(self.module.HIGHEST_PROTOCOL, 2)

    def test_callapi(self):
        from io import BytesIO
        f = BytesIO()
        # With and without keyword arguments
        self.module.dump(123, f, -1)
        self.module.dump(123, file=f, protocol=-1)
        self.module.dumps(123, -1)
        self.module.dumps(123, protocol=-1)
        self.module.Pickler(f, -1)
        self.module.Pickler(f, protocol=-1)

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
        L = list(range(10))
        self.assertEqual(self.loads(self.dumps(L)), L)
        self.assertEqual(self.id_count, 5)
        self.assertEqual(self.load_count, 5)

    def test_bin_persistence(self):
        self.id_count = 0
        self.load_count = 0
        L = list(range(10))
        self.assertEqual(self.loads(self.dumps(L, 1)), L)
        self.assertEqual(self.id_count, 5)
        self.assertEqual(self.load_count, 5)

if __name__ == "__main__":
    # Print some stuff that can be used to rewrite DATA{0,1,2}
    from pickletools import dis
    x = create_data()
    for i in range(3):
        p = pickle.dumps(x, i)
        print("DATA{0} = (".format(i))
        for j in range(0, len(p), 20):
            b = bytes(p[j:j+20])
            print("    {0!r}".format(b))
        print(")")
        print()
        print("# Disassembly of DATA{0}".format(i))
        print("DATA{0}_DIS = \"\"\"\\".format(i))
        dis(p)
        print("\"\"\"")
        print()
