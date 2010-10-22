import io
import unittest
import pickle
import pickletools
import copyreg
import weakref
from http.cookies import SimpleCookie

from test.support import TestFailed, TESTFN, run_with_locale

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


class UnseekableIO(io.BytesIO):
    def peek(self, *args):
        raise NotImplementedError

    def seekable(self):
        return False

    def seek(self, *args):
        raise io.UnsupportedOperation

    def tell(self):
        raise io.UnsupportedOperation


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
        if code in copyreg._inverted_registry:
            self.pair = copyreg._inverted_registry[code]
            copyreg.remove_extension(self.pair[0], self.pair[1], code)
        else:
            self.pair = None

    # Restore previous registration for code.
    def restore(self):
        code = self.code
        curpair = copyreg._inverted_registry.get(code)
        if curpair is not None:
            copyreg.remove_extension(curpair[0], curpair[1], code)
        pair = self.pair
        if pair is not None:
            copyreg.add_extension(pair[0], pair[1], code)

class C:
    def __eq__(self, other):
        return self.__dict__ == other.__dict__

class D(C):
    def __init__(self, arg):
        pass

class E(C):
    def __getinitargs__(self):
        return ()

import __main__
__main__.C = C
C.__module__ = "__main__"
__main__.D = D
D.__module__ = "__main__"
__main__.E = E
E.__module__ = "__main__"

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
    b'(lp0\nL0L\naL1L\naF2.0\nac'
    b'builtins\ncomplex\n'
    b'p1\n(F3.0\nF0.0\ntp2\nRp'
    b'3\naL1L\naL-1L\naL255L\naL-'
    b'255L\naL-256L\naL65535L\na'
    b'L-65535L\naL-65536L\naL2'
    b'147483647L\naL-2147483'
    b'647L\naL-2147483648L\na('
    b'Vabc\np4\ng4\nccopyreg'
    b'\n_reconstructor\np5\n('
    b'c__main__\nC\np6\ncbu'
    b'iltins\nobject\np7\nNt'
    b'p8\nRp9\n(dp10\nVfoo\np1'
    b'1\nL1L\nsVbar\np12\nL2L\nsb'
    b'g9\ntp13\nag13\naL5L\na.'
)

# Disassembly of DATA0
DATA0_DIS = """\
    0: (    MARK
    1: l        LIST       (MARK at 0)
    2: p    PUT        0
    5: L    LONG       0
    9: a    APPEND
   10: L    LONG       1
   14: a    APPEND
   15: F    FLOAT      2.0
   20: a    APPEND
   21: c    GLOBAL     'builtins complex'
   39: p    PUT        1
   42: (    MARK
   43: F        FLOAT      3.0
   48: F        FLOAT      0.0
   53: t        TUPLE      (MARK at 42)
   54: p    PUT        2
   57: R    REDUCE
   58: p    PUT        3
   61: a    APPEND
   62: L    LONG       1
   66: a    APPEND
   67: L    LONG       -1
   72: a    APPEND
   73: L    LONG       255
   79: a    APPEND
   80: L    LONG       -255
   87: a    APPEND
   88: L    LONG       -256
   95: a    APPEND
   96: L    LONG       65535
  104: a    APPEND
  105: L    LONG       -65535
  114: a    APPEND
  115: L    LONG       -65536
  124: a    APPEND
  125: L    LONG       2147483647
  138: a    APPEND
  139: L    LONG       -2147483647
  153: a    APPEND
  154: L    LONG       -2147483648
  168: a    APPEND
  169: (    MARK
  170: V        UNICODE    'abc'
  175: p        PUT        4
  178: g        GET        4
  181: c        GLOBAL     'copyreg _reconstructor'
  205: p        PUT        5
  208: (        MARK
  209: c            GLOBAL     '__main__ C'
  221: p            PUT        6
  224: c            GLOBAL     'builtins object'
  241: p            PUT        7
  244: N            NONE
  245: t            TUPLE      (MARK at 208)
  246: p        PUT        8
  249: R        REDUCE
  250: p        PUT        9
  253: (        MARK
  254: d            DICT       (MARK at 253)
  255: p        PUT        10
  259: V        UNICODE    'foo'
  264: p        PUT        11
  268: L        LONG       1
  272: s        SETITEM
  273: V        UNICODE    'bar'
  278: p        PUT        12
  282: L        LONG       2
  286: s        SETITEM
  287: b        BUILD
  288: g        GET        9
  291: t        TUPLE      (MARK at 169)
  292: p    PUT        13
  296: a    APPEND
  297: g    GET        13
  301: a    APPEND
  302: L    LONG       5
  306: a    APPEND
  307: .    STOP
highest protocol among opcodes = 0
"""

DATA1 = (
    b']q\x00(K\x00K\x01G@\x00\x00\x00\x00\x00\x00\x00c'
    b'builtins\ncomplex\nq\x01'
    b'(G@\x08\x00\x00\x00\x00\x00\x00G\x00\x00\x00\x00\x00\x00\x00\x00t'
    b'q\x02Rq\x03K\x01J\xff\xff\xff\xffK\xffJ\x01\xff\xff\xffJ'
    b'\x00\xff\xff\xffM\xff\xffJ\x01\x00\xff\xffJ\x00\x00\xff\xffJ\xff\xff'
    b'\xff\x7fJ\x01\x00\x00\x80J\x00\x00\x00\x80(X\x03\x00\x00\x00ab'
    b'cq\x04h\x04ccopyreg\n_reco'
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
   35: q        BINPUT     1
   37: (        MARK
   38: G            BINFLOAT   3.0
   47: G            BINFLOAT   0.0
   56: t            TUPLE      (MARK at 37)
   57: q        BINPUT     2
   59: R        REDUCE
   60: q        BINPUT     3
   62: K        BININT1    1
   64: J        BININT     -1
   69: K        BININT1    255
   71: J        BININT     -255
   76: J        BININT     -256
   81: M        BININT2    65535
   84: J        BININT     -65535
   89: J        BININT     -65536
   94: J        BININT     2147483647
   99: J        BININT     -2147483647
  104: J        BININT     -2147483648
  109: (        MARK
  110: X            BINUNICODE 'abc'
  118: q            BINPUT     4
  120: h            BINGET     4
  122: c            GLOBAL     'copyreg _reconstructor'
  146: q            BINPUT     5
  148: (            MARK
  149: c                GLOBAL     '__main__ C'
  161: q                BINPUT     6
  163: c                GLOBAL     'builtins object'
  180: q                BINPUT     7
  182: N                NONE
  183: t                TUPLE      (MARK at 148)
  184: q            BINPUT     8
  186: R            REDUCE
  187: q            BINPUT     9
  189: }            EMPTY_DICT
  190: q            BINPUT     10
  192: (            MARK
  193: X                BINUNICODE 'foo'
  201: q                BINPUT     11
  203: K                BININT1    1
  205: X                BINUNICODE 'bar'
  213: q                BINPUT     12
  215: K                BININT1    2
  217: u                SETITEMS   (MARK at 192)
  218: b            BUILD
  219: h            BINGET     9
  221: t            TUPLE      (MARK at 109)
  222: q        BINPUT     13
  224: h        BINGET     13
  226: K        BININT1    5
  228: e        APPENDS    (MARK at 3)
  229: .    STOP
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
   37: q        BINPUT     1
   39: G        BINFLOAT   3.0
   48: G        BINFLOAT   0.0
   57: \x86     TUPLE2
   58: q        BINPUT     2
   60: R        REDUCE
   61: q        BINPUT     3
   63: K        BININT1    1
   65: J        BININT     -1
   70: K        BININT1    255
   72: J        BININT     -255
   77: J        BININT     -256
   82: M        BININT2    65535
   85: J        BININT     -65535
   90: J        BININT     -65536
   95: J        BININT     2147483647
  100: J        BININT     -2147483647
  105: J        BININT     -2147483648
  110: (        MARK
  111: X            BINUNICODE 'abc'
  119: q            BINPUT     4
  121: h            BINGET     4
  123: c            GLOBAL     '__main__ C'
  135: q            BINPUT     5
  137: )            EMPTY_TUPLE
  138: \x81         NEWOBJ
  139: q            BINPUT     6
  141: }            EMPTY_DICT
  142: q            BINPUT     7
  144: (            MARK
  145: X                BINUNICODE 'foo'
  153: q                BINPUT     8
  155: K                BININT1    1
  157: X                BINUNICODE 'bar'
  165: q                BINPUT     9
  167: K                BININT1    2
  169: u                SETITEMS   (MARK at 144)
  170: b            BUILD
  171: h            BINGET     6
  173: t            TUPLE      (MARK at 110)
  174: q        BINPUT     10
  176: h        BINGET     10
  178: K        BININT1    5
  180: e        APPENDS    (MARK at 5)
  181: .    STOP
highest protocol among opcodes = 2
"""

# set([1,2]) pickled from 2.x with protocol 2
DATA3 = b'\x80\x02c__builtin__\nset\nq\x00]q\x01(K\x01K\x02e\x85q\x02Rq\x03.'

# xrange(5) pickled from 2.x with protocol 2
DATA4 = b'\x80\x02c__builtin__\nxrange\nq\x00K\x00K\x05K\x01\x87q\x01Rq\x02.'

# a SimpleCookie() object pickled from 2.x with protocol 2
DATA5 = (b'\x80\x02cCookie\nSimpleCookie\nq\x00)\x81q\x01U\x03key'
         b'q\x02cCookie\nMorsel\nq\x03)\x81q\x04(U\x07commentq\x05U'
         b'\x00q\x06U\x06domainq\x07h\x06U\x06secureq\x08h\x06U\x07'
         b'expiresq\th\x06U\x07max-ageq\nh\x06U\x07versionq\x0bh\x06U'
         b'\x04pathq\x0ch\x06U\x08httponlyq\rh\x06u}q\x0e(U\x0b'
         b'coded_valueq\x0fU\x05valueq\x10h\x10h\x10h\x02h\x02ubs}q\x11b.')

# set([3]) pickled from 2.x with protocol 2
DATA6 = b'\x80\x02c__builtin__\nset\nq\x00]q\x01K\x03a\x85q\x02Rq\x03.'


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
    # Subclass must define self.dumps, self.loads.

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

    def test_load_classic_instance(self):
        # See issue5180.  Test loading 2.x pickles that
        # contain an instance of old style class.
        for X, args in [(C, ()), (D, ('x',)), (E, ())]:
            xname = X.__name__.encode('ascii')
            # Protocol 0 (text mode pickle):
            """
            0: (    MARK
            1: i        INST       '__main__ X' (MARK at 0)
            15: p    PUT        0
            18: (    MARK
            19: d        DICT       (MARK at 18)
            20: p    PUT        1
            23: b    BUILD
            24: .    STOP
            """
            pickle0 = (b"(i__main__\n"
                       b"X\n"
                       b"p0\n"
                       b"(dp1\nb.").replace(b'X', xname)
            self.assertEqual(X(*args), self.loads(pickle0))

            # Protocol 1 (binary mode pickle)
            """
            0: (    MARK
            1: c        GLOBAL     '__main__ X'
            15: q        BINPUT     0
            17: o        OBJ        (MARK at 0)
            18: q    BINPUT     1
            20: }    EMPTY_DICT
            21: q    BINPUT     2
            23: b    BUILD
            24: .    STOP
            """
            pickle1 = (b'(c__main__\n'
                       b'X\n'
                       b'q\x00oq\x01}q\x02b.').replace(b'X', xname)
            self.assertEqual(X(*args), self.loads(pickle1))

            # Protocol 2 (pickle2 = b'\x80\x02' + pickle1)
            """
            0: \x80 PROTO      2
            2: (    MARK
            3: c        GLOBAL     '__main__ X'
            17: q        BINPUT     0
            19: o        OBJ        (MARK at 2)
            20: q    BINPUT     1
            22: }    EMPTY_DICT
            23: q    BINPUT     2
            25: b    BUILD
            26: .    STOP
            """
            pickle2 = (b'\x80\x02(c__main__\n'
                       b'X\n'
                       b'q\x00oq\x01}q\x02b.').replace(b'X', xname)
            self.assertEqual(X(*args), self.loads(pickle2))

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
            self.assertTrue(x is x[0])

    def test_recursive_tuple(self):
        t = ([],)
        t[0].append(t)
        for proto in protocols:
            s = self.dumps(t, proto)
            x = self.loads(s)
            self.assertEqual(len(x), 1)
            self.assertEqual(len(x[0]), 1)
            self.assertTrue(x is x[0][0])

    def test_recursive_dict(self):
        d = {}
        d[1] = d
        for proto in protocols:
            s = self.dumps(d, proto)
            x = self.loads(s)
            self.assertEqual(list(x.keys()), [1])
            self.assertTrue(x[1] is x)

    def test_recursive_inst(self):
        i = C()
        i.attr = i
        for proto in protocols:
            s = self.dumps(i, 2)
            x = self.loads(s)
            self.assertEqual(dir(x), dir(i))
            self.assertTrue(x.attr is x)

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
            self.assertTrue(x[0].attr[1] is x)

    def test_get(self):
        self.assertRaises(KeyError, self.loads, b'g0\np0')
        self.assertEquals(self.loads(b'((Kdtp0\nh\x00l.))'), [(100,), (100,)])

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
        endcases = ['', '<\\u>', '<\\\u1234>', '<\n>',
                    '<\\>', '<\\\U00012345>',
                    # surrogates
                    '<\udc80>']
        for proto in protocols:
            for u in endcases:
                p = self.dumps(u, proto)
                u2 = self.loads(p)
                self.assertEqual(u2, u)

    def test_unicode_high_plane(self):
        t = '\U00012345'
        for proto in protocols:
            p = self.dumps(t, proto)
            t2 = self.loads(p)
            self.assertEqual(t2, t)

    def test_bytes(self):
        for proto in protocols:
            for u in b'', b'xyz', b'xyz'*100:
                p = self.dumps(u)
                self.assertEqual(self.loads(p), u)

    def test_ints(self):
        import sys
        for proto in protocols:
            n = sys.maxsize
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

    def test_float(self):
        test_values = [0.0, 4.94e-324, 1e-310, 7e-308, 6.626e-34, 0.1, 0.5,
                       3.14, 263.44582062374053, 6.022e23, 1e30]
        test_values = test_values + [-x for x in test_values]
        for proto in protocols:
            for value in test_values:
                pickle = self.dumps(value, proto)
                got = self.loads(pickle)
                self.assertEqual(value, got)

    @run_with_locale('LC_ALL', 'de_DE', 'fr_FR')
    def test_float_format(self):
        # make sure that floats are formatted locale independent with proto 0
        self.assertEqual(self.dumps(1.2, 0)[0:3], b'F1.')

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
            self.assertTrue(str(detail).startswith(
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

                           (3, 0): pickle.EMPTY_TUPLE,
                           (3, 1): pickle.TUPLE1,
                           (3, 2): pickle.TUPLE2,
                           (3, 3): pickle.TUPLE3,
                           (3, 4): pickle.TUPLE,
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
                           (3, None): pickle.NONE,

                           (0, True): pickle.INT,
                           (1, True): pickle.INT,
                           (2, True): pickle.NEWTRUE,
                           (3, True): pickle.NEWTRUE,

                           (0, False): pickle.INT,
                           (1, False): pickle.INT,
                           (2, False): pickle.NEWFALSE,
                           (3, False): pickle.NEWFALSE,
                          }
        for proto in protocols:
            for x in None, False, True:
                s = self.dumps(x, proto)
                y = self.loads(s)
                self.assertTrue(x is y, (proto, x, s, y))
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

    def test_newobj_proxies(self):
        # NEWOBJ should use the __class__ rather than the raw type
        classes = myclasses[:]
        # Cannot create weakproxies to these classes
        for c in (MyInt, MyTuple):
            classes.remove(c)
        for proto in protocols:
            for C in classes:
                B = C.__base__
                x = C(C.sample)
                x.foo = 42
                p = weakref.proxy(x)
                s = self.dumps(p, proto)
                y = self.loads(s)
                self.assertEqual(type(y), type(x))  # rather than type(p)
                detail = (proto, C, B, x, y, type(y))
                self.assertEqual(B(x), B(y), detail)
                self.assertEqual(x.__dict__, y.__dict__, detail)

    # Register a type with copyreg, with extension code extcode.  Pickle
    # an object of that type.  Check that the resulting pickle uses opcode
    # (EXT[124]) under proto 2, and not in proto 1.

    def produce_global_ext(self, extcode, opcode):
        e = ExtensionSaver(extcode)
        try:
            copyreg.add_extension(__name__, "MyList", extcode)
            x = MyList([1, 2, 3])
            x.foo = 42
            x.bar = "hello"

            # Dump using protocol 1 for comparison.
            s1 = self.dumps(x, 1)
            self.assertIn(__name__.encode("utf-8"), s1)
            self.assertIn(b"MyList", s1)
            self.assertEqual(opcode_in_pickle(opcode, s1), False)

            y = self.loads(s1)
            self.assertEqual(list(x), list(y))
            self.assertEqual(x.__dict__, y.__dict__)

            # Dump using protocol 2 for test.
            s2 = self.dumps(x, 2)
            self.assertNotIn(__name__.encode("utf-8"), s2)
            self.assertNotIn(b"MyList", s2)
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
                self.assertTrue(num_appends >= 2)

    def test_dict_chunking(self):
        n = 10  # too small to chunk
        x = dict.fromkeys(range(n))
        for proto in protocols:
            s = self.dumps(x, proto)
            self.assertIsInstance(s, bytes_types)
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
                self.assertTrue(num_setitems >= 2)

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
        for proto in protocols:
            x = REX_one()
            self.assertEqual(x._reduce_called, 0)
            s = self.dumps(x, proto)
            self.assertEqual(x._reduce_called, 1)
            y = self.loads(s)
            self.assertEqual(y._reduce_called, 0)

    def test_reduce_ex_called(self):
        for proto in protocols:
            x = REX_two()
            self.assertEqual(x._proto, None)
            s = self.dumps(x, proto)
            self.assertEqual(x._proto, proto)
            y = self.loads(s)
            self.assertEqual(y._proto, None)

    def test_reduce_ex_overrides_reduce(self):
        for proto in protocols:
            x = REX_three()
            self.assertEqual(x._proto, None)
            s = self.dumps(x, proto)
            self.assertEqual(x._proto, proto)
            y = self.loads(s)
            self.assertEqual(y._proto, None)

    def test_reduce_ex_calls_base(self):
        for proto in protocols:
            x = REX_four()
            self.assertEqual(x._proto, None)
            s = self.dumps(x, proto)
            self.assertEqual(x._proto, proto)
            y = self.loads(s)
            self.assertEqual(y._proto, proto)

    def test_reduce_calls_base(self):
        for proto in protocols:
            x = REX_five()
            self.assertEqual(x._reduce_called, 0)
            s = self.dumps(x, proto)
            self.assertEqual(x._reduce_called, 1)
            y = self.loads(s)
            self.assertEqual(y._reduce_called, 1)

    def test_bad_getattr(self):
        x = BadGetattr()
        for proto in 0, 1:
            self.assertRaises(RuntimeError, self.dumps, x, proto)
        # protocol 2 don't raise a RuntimeError.
        d = self.dumps(x, 2)

    def test_reduce_bad_iterator(self):
        # Issue4176: crash when 4th and 5th items of __reduce__()
        # are not iterators
        class C(object):
            def __reduce__(self):
                # 4th item is not an iterator
                return list, (), None, [], None
        class D(object):
            def __reduce__(self):
                # 5th item is not an iterator
                return dict, (), None, None, []

        # Protocol 0 is less strict and also accept iterables.
        for proto in protocols:
            try:
                self.dumps(C(), proto)
            except (pickle.PickleError):
                pass
            try:
                self.dumps(D(), proto)
            except (pickle.PickleError):
                pass

    def test_many_puts_and_gets(self):
        # Test that internal data structures correctly deal with lots of
        # puts/gets.
        keys = ("aaa" + str(i) for i in range(100))
        large_dict = dict((k, [4, 5, 6]) for k in keys)
        obj = [dict(large_dict), dict(large_dict), dict(large_dict)]

        for proto in protocols:
            dumped = self.dumps(obj, proto)
            loaded = self.loads(dumped)
            self.assertEqual(loaded, obj,
                             "Failed protocol %d: %r != %r"
                             % (proto, obj, loaded))

    def test_attribute_name_interning(self):
        # Test that attribute names of pickled objects are interned when
        # unpickling.
        for proto in protocols:
            x = C()
            x.foo = 42
            x.bar = "hello"
            s = self.dumps(x, proto)
            y = self.loads(s)
            x_keys = sorted(x.__dict__)
            y_keys = sorted(y.__dict__)
            for x_key, y_key in zip(x_keys, y_keys):
                self.assertIs(x_key, y_key)

    def test_unpickle_from_2x(self):
        # Unpickle non-trivial data from Python 2.x.
        loaded = self.loads(DATA3)
        self.assertEqual(loaded, set([1, 2]))
        loaded = self.loads(DATA4)
        self.assertEqual(type(loaded), type(range(0)))
        self.assertEqual(list(loaded), list(range(5)))
        loaded = self.loads(DATA5)
        self.assertEqual(type(loaded), SimpleCookie)
        self.assertEqual(list(loaded.keys()), ["key"])
        self.assertEqual(loaded["key"].value, "Set-Cookie: key=value")

    def test_pickle_to_2x(self):
        # Pickle non-trivial data with protocol 2, expecting that it yields
        # the same result as Python 2.x did.
        # NOTE: this test is a bit too strong since we can produce different
        # bytecode that 2.x will still understand.
        dumped = self.dumps(range(5), 2)
        self.assertEqual(dumped, DATA4)
        dumped = self.dumps(set([3]), 2)
        self.assertEqual(dumped, DATA6)

    def test_large_pickles(self):
        # Test the correctness of internal buffering routines when handling
        # large data.
        for proto in protocols:
            data = (1, min, b'xy' * (30 * 1024), len)
            dumped = self.dumps(data, proto)
            loaded = self.loads(dumped)
            self.assertEqual(len(loaded), len(data))
            self.assertEqual(loaded, data)


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

myclasses = [MyInt, MyFloat,
             MyComplex,
             MyStr, MyUnicode,
             MyTuple, MyList, MyDict]


class SlotList(MyList):
    __slots__ = ["foo"]

class SimpleNewObj(object):
    def __init__(self, a, b, c):
        # raise an error, to make sure this isn't called
        raise TypeError("SimpleNewObj.__init__() didn't expect to get called")

class BadGetattr:
    def __getattr__(self, key):
        self.foo


class AbstractPickleModuleTests(unittest.TestCase):

    def test_dump_closed_file(self):
        import os
        f = open(TESTFN, "wb")
        try:
            f.close()
            self.assertRaises(ValueError, pickle.dump, 123, f)
        finally:
            os.remove(TESTFN)

    def test_load_closed_file(self):
        import os
        f = open(TESTFN, "wb")
        try:
            f.close()
            self.assertRaises(ValueError, pickle.dump, 123, f)
        finally:
            os.remove(TESTFN)

    def test_load_from_and_dump_to_file(self):
        stream = io.BytesIO()
        data = [123, {}, 124]
        pickle.dump(data, stream)
        stream.seek(0)
        unpickled = pickle.load(stream)
        self.assertEqual(unpickled, data)

    def test_highest_protocol(self):
        # Of course this needs to be changed when HIGHEST_PROTOCOL changes.
        self.assertEqual(pickle.HIGHEST_PROTOCOL, 3)

    def test_callapi(self):
        f = io.BytesIO()
        # With and without keyword arguments
        pickle.dump(123, f, -1)
        pickle.dump(123, file=f, protocol=-1)
        pickle.dumps(123, -1)
        pickle.dumps(123, protocol=-1)
        pickle.Pickler(f, -1)
        pickle.Pickler(f, protocol=-1)

    def test_bad_init(self):
        # Test issue3664 (pickle can segfault from a badly initialized Pickler).
        # Override initialization without calling __init__() of the superclass.
        class BadPickler(pickle.Pickler):
            def __init__(self): pass

        class BadUnpickler(pickle.Unpickler):
            def __init__(self): pass

        self.assertRaises(pickle.PicklingError, BadPickler().dump, 0)
        self.assertRaises(pickle.UnpicklingError, BadUnpickler().load)

    def test_bad_input(self):
        # Test issue4298
        s = bytes([0x58, 0, 0, 0, 0x54])
        self.assertRaises(EOFError, pickle.loads, s)
        # Test issue7455
        s = b'0'
        self.assertRaises(pickle.UnpicklingError, pickle.loads, s)


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


class AbstractPicklerUnpicklerObjectTests(unittest.TestCase):

    pickler_class = None
    unpickler_class = None

    def setUp(self):
        assert self.pickler_class
        assert self.unpickler_class

    def test_clear_pickler_memo(self):
        # To test whether clear_memo() has any effect, we pickle an object,
        # then pickle it again without clearing the memo; the two serialized
        # forms should be different. If we clear_memo() and then pickle the
        # object again, the third serialized form should be identical to the
        # first one we obtained.
        data = ["abcdefg", "abcdefg", 44]
        f = io.BytesIO()
        pickler = self.pickler_class(f)

        pickler.dump(data)
        first_pickled = f.getvalue()

        # Reset StringIO object.
        f.seek(0)
        f.truncate()

        pickler.dump(data)
        second_pickled = f.getvalue()

        # Reset the Pickler and StringIO objects.
        pickler.clear_memo()
        f.seek(0)
        f.truncate()

        pickler.dump(data)
        third_pickled = f.getvalue()

        self.assertNotEqual(first_pickled, second_pickled)
        self.assertEqual(first_pickled, third_pickled)

    def test_priming_pickler_memo(self):
        # Verify that we can set the Pickler's memo attribute.
        data = ["abcdefg", "abcdefg", 44]
        f = io.BytesIO()
        pickler = self.pickler_class(f)

        pickler.dump(data)
        first_pickled = f.getvalue()

        f = io.BytesIO()
        primed = self.pickler_class(f)
        primed.memo = pickler.memo

        primed.dump(data)
        primed_pickled = f.getvalue()

        self.assertNotEqual(first_pickled, primed_pickled)

    def test_priming_unpickler_memo(self):
        # Verify that we can set the Unpickler's memo attribute.
        data = ["abcdefg", "abcdefg", 44]
        f = io.BytesIO()
        pickler = self.pickler_class(f)

        pickler.dump(data)
        first_pickled = f.getvalue()

        f = io.BytesIO()
        primed = self.pickler_class(f)
        primed.memo = pickler.memo

        primed.dump(data)
        primed_pickled = f.getvalue()

        unpickler = self.unpickler_class(io.BytesIO(first_pickled))
        unpickled_data1 = unpickler.load()

        self.assertEqual(unpickled_data1, data)

        primed = self.unpickler_class(io.BytesIO(primed_pickled))
        primed.memo = unpickler.memo
        unpickled_data2 = primed.load()

        primed.memo.clear()

        self.assertEqual(unpickled_data2, data)
        self.assertTrue(unpickled_data2 is unpickled_data1)

    def test_reusing_unpickler_objects(self):
        data1 = ["abcdefg", "abcdefg", 44]
        f = io.BytesIO()
        pickler = self.pickler_class(f)
        pickler.dump(data1)
        pickled1 = f.getvalue()

        data2 = ["abcdefg", 44, 44]
        f = io.BytesIO()
        pickler = self.pickler_class(f)
        pickler.dump(data2)
        pickled2 = f.getvalue()

        f = io.BytesIO()
        f.write(pickled1)
        f.seek(0)
        unpickler = self.unpickler_class(f)
        self.assertEqual(unpickler.load(), data1)

        f.seek(0)
        f.truncate()
        f.write(pickled2)
        f.seek(0)
        self.assertEqual(unpickler.load(), data2)

    def _check_multiple_unpicklings(self, ioclass):
        for proto in protocols:
            data1 = [(x, str(x)) for x in range(2000)] + [b"abcde", len]
            f = ioclass()
            pickler = self.pickler_class(f, protocol=proto)
            pickler.dump(data1)
            pickled = f.getvalue()

            N = 5
            f = ioclass(pickled * N)
            unpickler = self.unpickler_class(f)
            for i in range(N):
                if f.seekable():
                    pos = f.tell()
                self.assertEqual(unpickler.load(), data1)
                if f.seekable():
                    self.assertEqual(f.tell(), pos + len(pickled))
            self.assertRaises(EOFError, unpickler.load)

    def test_multiple_unpicklings_seekable(self):
        self._check_multiple_unpicklings(io.BytesIO)

    def test_multiple_unpicklings_unseekable(self):
        self._check_multiple_unpicklings(UnseekableIO)


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
