import copyreg
import io
import pickle
import pickletools
import random
import struct
import sys
import unittest
import weakref
from http.cookies import SimpleCookie

from test.support import (
    TestFailed, TESTFN, run_with_locale, no_tracing,
    _2G, _4G, bigmemtest,
    )

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

class H(object):
    pass

import __main__
__main__.C = C
C.__module__ = "__main__"
__main__.D = D
D.__module__ = "__main__"
__main__.E = E
E.__module__ = "__main__"
__main__.H = H
H.__module__ = "__main__"

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

class pickling_metaclass(type):
    def __eq__(self, other):
        return (type(self) == type(other) and
                self.reduce_args == other.reduce_args)

    def __reduce__(self):
        return (create_dynamic_class, self.reduce_args)

def create_dynamic_class(name, bases):
    result = pickling_metaclass(name, bases, dict())
    result.reduce_args = (name, bases)
    return result

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

python2_exceptions_without_args = (
    ArithmeticError,
    AssertionError,
    AttributeError,
    BaseException,
    BufferError,
    BytesWarning,
    DeprecationWarning,
    EOFError,
    EnvironmentError,
    Exception,
    FloatingPointError,
    FutureWarning,
    GeneratorExit,
    IOError,
    ImportError,
    ImportWarning,
    IndentationError,
    IndexError,
    KeyError,
    KeyboardInterrupt,
    LookupError,
    MemoryError,
    NameError,
    NotImplementedError,
    OSError,
    OverflowError,
    PendingDeprecationWarning,
    ReferenceError,
    RuntimeError,
    RuntimeWarning,
    # StandardError is gone in Python 3, we map it to Exception
    StopIteration,
    SyntaxError,
    SyntaxWarning,
    SystemError,
    SystemExit,
    TabError,
    TypeError,
    UnboundLocalError,
    UnicodeError,
    UnicodeWarning,
    UserWarning,
    ValueError,
    Warning,
    ZeroDivisionError,
)

exception_pickle = b'\x80\x02cexceptions\n?\nq\x00)Rq\x01.'

# Exception objects without arguments pickled from 2.x with protocol 2
DATA7 = {
    exception :
    exception_pickle.replace(b'?', exception.__name__.encode("ascii"))
    for exception in python2_exceptions_without_args
}

# StandardError is mapped to Exception, test that separately
DATA8 = exception_pickle.replace(b'?', b'StandardError')

# UnicodeEncodeError object pickled from 2.x with protocol 2
DATA9 = (b'\x80\x02cexceptions\nUnicodeEncodeError\n'
         b'q\x00(U\x05asciiq\x01X\x03\x00\x00\x00fooq\x02K\x00K\x01'
         b'U\x03badq\x03tq\x04Rq\x05.')


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

    optimized = False

    _testdata = create_data()

    def setUp(self):
        pass

    def assert_is_copy(self, obj, objcopy, msg=None):
        """Utility method to verify if two objects are copies of each others.
        """
        if msg is None:
            msg = "{!r} is not a copy of {!r}".format(obj, objcopy)
        self.assertEqual(obj, objcopy, msg=msg)
        self.assertIs(type(obj), type(objcopy), msg=msg)
        if hasattr(obj, '__dict__'):
            self.assertDictEqual(obj.__dict__, objcopy.__dict__, msg=msg)
            self.assertIsNot(obj.__dict__, objcopy.__dict__, msg=msg)
        if hasattr(obj, '__slots__'):
            self.assertListEqual(obj.__slots__, objcopy.__slots__, msg=msg)
            for slot in obj.__slots__:
                self.assertEqual(
                    hasattr(obj, slot), hasattr(objcopy, slot), msg=msg)
                self.assertEqual(getattr(obj, slot, None),
                                 getattr(objcopy, slot, None), msg=msg)

    def test_misc(self):
        # test various datatypes not tested by testdata
        for proto in protocols:
            x = myint(4)
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)

            x = (1, ())
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)

            x = initarg(1, x)
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)

        # XXX test __reduce__ protocol?

    def test_roundtrip_equality(self):
        expected = self._testdata
        for proto in protocols:
            s = self.dumps(expected, proto)
            got = self.loads(s)
            self.assert_is_copy(expected, got)

    def test_load_from_data0(self):
        self.assert_is_copy(self._testdata, self.loads(DATA0))

    def test_load_from_data1(self):
        self.assert_is_copy(self._testdata, self.loads(DATA1))

    def test_load_from_data2(self):
        self.assert_is_copy(self._testdata, self.loads(DATA2))

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
            self.assert_is_copy(X(*args), self.loads(pickle0))

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
            self.assert_is_copy(X(*args), self.loads(pickle1))

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
            self.assert_is_copy(X(*args), self.loads(pickle2))

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
            self.assertIsInstance(x, list)
            self.assertEqual(len(x), 1)
            self.assertTrue(x is x[0])

    def test_recursive_tuple(self):
        t = ([],)
        t[0].append(t)
        for proto in protocols:
            s = self.dumps(t, proto)
            x = self.loads(s)
            self.assertIsInstance(x, tuple)
            self.assertEqual(len(x), 1)
            self.assertEqual(len(x[0]), 1)
            self.assertTrue(x is x[0][0])

    def test_recursive_dict(self):
        d = {}
        d[1] = d
        for proto in protocols:
            s = self.dumps(d, proto)
            x = self.loads(s)
            self.assertIsInstance(x, dict)
            self.assertEqual(list(x.keys()), [1])
            self.assertTrue(x[1] is x)

    def test_recursive_set(self):
        h = H()
        y = set({h})
        h.attr = y
        for proto in protocols:
            s = self.dumps(y, proto)
            x = self.loads(s)
            self.assertIsInstance(x, set)
            self.assertIs(list(x)[0].attr, x)
            self.assertEqual(len(x), 1)

    def test_recursive_frozenset(self):
        h = H()
        y = frozenset({h})
        h.attr = y
        for proto in protocols:
            s = self.dumps(y, proto)
            x = self.loads(s)
            self.assertIsInstance(x, frozenset)
            self.assertIs(list(x)[0].attr, x)
            self.assertEqual(len(x), 1)

    def test_recursive_inst(self):
        i = C()
        i.attr = i
        for proto in protocols:
            s = self.dumps(i, proto)
            x = self.loads(s)
            self.assertIsInstance(x, C)
            self.assertEqual(dir(x), dir(i))
            self.assertIs(x.attr, x)

    def test_recursive_multi(self):
        l = []
        d = {1:l}
        i = C()
        i.attr = d
        l.append(i)
        for proto in protocols:
            s = self.dumps(l, proto)
            x = self.loads(s)
            self.assertIsInstance(x, list)
            self.assertEqual(len(x), 1)
            self.assertEqual(dir(x[0]), dir(i))
            self.assertEqual(list(x[0].attr.keys()), [1])
            self.assertTrue(x[0].attr[1] is x)

    def test_get(self):
        self.assertRaises(KeyError, self.loads, b'g0\np0')
        self.assert_is_copy([(100,), (100,)],
                            self.loads(b'((Kdtp0\nh\x00l.))'))

    def test_unicode(self):
        endcases = ['', '<\\u>', '<\\\u1234>', '<\n>',
                    '<\\>', '<\\\U00012345>',
                    # surrogates
                    '<\udc80>']
        for proto in protocols:
            for u in endcases:
                p = self.dumps(u, proto)
                u2 = self.loads(p)
                self.assert_is_copy(u, u2)

    def test_unicode_high_plane(self):
        t = '\U00012345'
        for proto in protocols:
            p = self.dumps(t, proto)
            t2 = self.loads(p)
            self.assert_is_copy(t, t2)

    def test_bytes(self):
        for proto in protocols:
            for s in b'', b'xyz', b'xyz'*100:
                p = self.dumps(s, proto)
                self.assert_is_copy(s, self.loads(p))
            for s in [bytes([i]) for i in range(256)]:
                p = self.dumps(s, proto)
                self.assert_is_copy(s, self.loads(p))
            for s in [bytes([i, i]) for i in range(256)]:
                p = self.dumps(s, proto)
                self.assert_is_copy(s, self.loads(p))

    def test_ints(self):
        import sys
        for proto in protocols:
            n = sys.maxsize
            while n:
                for expected in (-n, n):
                    s = self.dumps(expected, proto)
                    n2 = self.loads(s)
                    self.assert_is_copy(expected, n2)
                n = n >> 1

    def test_maxint64(self):
        maxint64 = (1 << 63) - 1
        data = b'I' + str(maxint64).encode("ascii") + b'\n.'
        got = self.loads(data)
        self.assert_is_copy(maxint64, got)

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
                        self.assert_is_copy(n, got)
        # Try a monster.  This is quadratic-time in protos 0 & 1, so don't
        # bother with those.
        nbase = int("deadbeeffeedface", 16)
        nbase += nbase << 1000000
        for n in nbase, -nbase:
            p = self.dumps(n, 2)
            got = self.loads(p)
            # assert_is_copy is very expensive here as it precomputes
            # a failure message by computing the repr() of n and got,
            # we just do the check ourselves.
            self.assertIs(type(got), int)
            self.assertEqual(n, got)

    def test_float(self):
        test_values = [0.0, 4.94e-324, 1e-310, 7e-308, 6.626e-34, 0.1, 0.5,
                       3.14, 263.44582062374053, 6.022e23, 1e30]
        test_values = test_values + [-x for x in test_values]
        for proto in protocols:
            for value in test_values:
                pickle = self.dumps(value, proto)
                got = self.loads(pickle)
                self.assert_is_copy(value, got)

    @run_with_locale('LC_ALL', 'de_DE', 'fr_FR')
    def test_float_format(self):
        # make sure that floats are formatted locale independent with proto 0
        self.assertEqual(self.dumps(1.2, 0)[0:3], b'F1.')

    def test_reduce(self):
        for proto in protocols:
            inst = AAA()
            dumped = self.dumps(inst, proto)
            loaded = self.loads(dumped)
            self.assertEqual(loaded, REDUCE_A)

    def test_getinitargs(self):
        for proto in protocols:
            inst = initarg(1, 2)
            dumped = self.dumps(inst, proto)
            loaded = self.loads(dumped)
            self.assert_is_copy(inst, loaded)

    def test_pop_empty_stack(self):
        # Test issue7455
        s = b'0'
        self.assertRaises((pickle.UnpicklingError, IndexError), self.loads, s)

    def test_metaclass(self):
        a = use_metaclass()
        for proto in protocols:
            s = self.dumps(a, proto)
            b = self.loads(s)
            self.assertEqual(a.__class__, b.__class__)

    def test_dynamic_class(self):
        a = create_dynamic_class("my_dynamic_class", (object,))
        copyreg.pickle(pickling_metaclass, pickling_metaclass.__reduce__)
        for proto in protocols:
            s = self.dumps(a, proto)
            b = self.loads(s)
            self.assertEqual(a, b)
            self.assertIs(type(a), type(b))

    def test_structseq(self):
        import time
        import os

        t = time.localtime()
        for proto in protocols:
            s = self.dumps(t, proto)
            u = self.loads(s)
            self.assert_is_copy(t, u)
            if hasattr(os, "stat"):
                t = os.stat(os.curdir)
                s = self.dumps(t, proto)
                u = self.loads(s)
                self.assert_is_copy(t, u)
            if hasattr(os, "statvfs"):
                t = os.statvfs(os.curdir)
                s = self.dumps(t, proto)
                u = self.loads(s)
                self.assert_is_copy(t, u)

    def test_ellipsis(self):
        for proto in protocols:
            s = self.dumps(..., proto)
            u = self.loads(s)
            self.assertIs(..., u)

    def test_notimplemented(self):
        for proto in protocols:
            s = self.dumps(NotImplemented, proto)
            u = self.loads(s)
            self.assertIs(NotImplemented, u)

    def test_singleton_types(self):
        # Issue #6477: Test that types of built-in singletons can be pickled.
        singletons = [None, ..., NotImplemented]
        for singleton in singletons:
            for proto in protocols:
                s = self.dumps(type(singleton), proto)
                u = self.loads(s)
                self.assertIs(type(singleton), u)

    # Tests for protocol 2

    def test_proto(self):
        for proto in protocols:
            pickled = self.dumps(None, proto)
            if proto >= 2:
                proto_header = pickle.PROTO + bytes([proto])
                self.assertTrue(pickled.startswith(proto_header))
            else:
                self.assertEqual(count_opcode(pickle.PROTO, pickled), 0)

        oob = protocols[-1] + 1     # a future protocol
        build_none = pickle.NONE + pickle.STOP
        badpickle = pickle.PROTO + bytes([oob]) + build_none
        try:
            self.loads(badpickle)
        except ValueError as err:
            self.assertIn("unsupported pickle protocol", str(err))
        else:
            self.fail("expected bad protocol number to raise ValueError")

    def test_long1(self):
        x = 12345678910111213141516178920
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)
            self.assertEqual(opcode_in_pickle(pickle.LONG1, s), proto >= 2)

    def test_long4(self):
        x = 12345678910111213141516178920 << (256*8)
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)
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
                self.assert_is_copy(x, y)
                expected = expected_opcode[min(proto, 3), len(x)]
                self.assertTrue(opcode_in_pickle(expected, s))

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
                expected = expected_opcode[min(proto, 3), x]
                self.assertTrue(opcode_in_pickle(expected, s))

    def test_newobj_tuple(self):
        x = MyTuple([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)

    def test_newobj_list(self):
        x = MyList([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)

    def test_newobj_generic(self):
        for proto in protocols:
            for C in myclasses:
                B = C.__base__
                x = C(C.sample)
                x.foo = 42
                s = self.dumps(x, proto)
                y = self.loads(s)
                detail = (proto, C, B, x, y, type(y))
                self.assert_is_copy(x, y) # XXX revisit
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
            self.assertFalse(opcode_in_pickle(opcode, s1))

            y = self.loads(s1)
            self.assert_is_copy(x, y)

            # Dump using protocol 2 for test.
            s2 = self.dumps(x, 2)
            self.assertNotIn(__name__.encode("utf-8"), s2)
            self.assertNotIn(b"MyList", s2)
            self.assertEqual(opcode_in_pickle(opcode, s2), True, repr(s2))

            y = self.loads(s2)
            self.assert_is_copy(x, y)
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
            self.assert_is_copy(x, y)
            num_appends = count_opcode(pickle.APPENDS, s)
            self.assertEqual(num_appends, proto > 0)

        n = 2500  # expect at least two chunks when proto > 0
        x = list(range(n))
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)
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
            self.assert_is_copy(x, y)
            num_setitems = count_opcode(pickle.SETITEMS, s)
            self.assertEqual(num_setitems, proto > 0)

        n = 2500  # expect at least two chunks when proto > 0
        x = dict.fromkeys(range(n))
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)
            num_setitems = count_opcode(pickle.SETITEMS, s)
            if proto == 0:
                self.assertEqual(num_setitems, 0)
            else:
                self.assertTrue(num_setitems >= 2)

    def test_set_chunking(self):
        n = 10  # too small to chunk
        x = set(range(n))
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)
            num_additems = count_opcode(pickle.ADDITEMS, s)
            if proto < 4:
                self.assertEqual(num_additems, 0)
            else:
                self.assertEqual(num_additems, 1)

        n = 2500  # expect at least two chunks when proto >= 4
        x = set(range(n))
        for proto in protocols:
            s = self.dumps(x, proto)
            y = self.loads(s)
            self.assert_is_copy(x, y)
            num_additems = count_opcode(pickle.ADDITEMS, s)
            if proto < 4:
                self.assertEqual(num_additems, 0)
            else:
                self.assertGreaterEqual(num_additems, 2)

    def test_simple_newobj(self):
        x = object.__new__(SimpleNewObj)  # avoid __init__
        x.abc = 666
        for proto in protocols:
            s = self.dumps(x, proto)
            self.assertEqual(opcode_in_pickle(pickle.NEWOBJ, s),
                             2 <= proto < 4)
            self.assertEqual(opcode_in_pickle(pickle.NEWOBJ_EX, s),
                             proto >= 4)
            y = self.loads(s)   # will raise TypeError if __init__ called
            self.assert_is_copy(x, y)

    def test_newobj_list_slots(self):
        x = SlotList([1, 2, 3])
        x.foo = 42
        x.bar = "hello"
        s = self.dumps(x, 2)
        y = self.loads(s)
        self.assert_is_copy(x, y)

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

    @no_tracing
    def test_bad_getattr(self):
        # Issue #3514: crash when there is an infinite loop in __getattr__
        x = BadGetattr()
        for proto in protocols:
            self.assertRaises(RuntimeError, self.dumps, x, proto)

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
            with self.subTest(proto=proto):
                dumped = self.dumps(obj, proto)
                loaded = self.loads(dumped)
                self.assert_is_copy(obj, loaded)

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
        self.assertEqual(loaded["key"].value, "value")

        for (exc, data) in DATA7.items():
            loaded = self.loads(data)
            self.assertIs(type(loaded), exc)

        loaded = self.loads(DATA8)
        self.assertIs(type(loaded), Exception)

        loaded = self.loads(DATA9)
        self.assertIs(type(loaded), UnicodeEncodeError)
        self.assertEqual(loaded.object, "foo")
        self.assertEqual(loaded.encoding, "ascii")
        self.assertEqual(loaded.start, 0)
        self.assertEqual(loaded.end, 1)
        self.assertEqual(loaded.reason, "bad")

    def test_pickle_to_2x(self):
        # Pickle non-trivial data with protocol 2, expecting that it yields
        # the same result as Python 2.x did.
        # NOTE: this test is a bit too strong since we can produce different
        # bytecode that 2.x will still understand.
        dumped = self.dumps(range(5), 2)
        self.assertEqual(dumped, DATA4)
        dumped = self.dumps(set([3]), 2)
        self.assertEqual(dumped, DATA6)

    def test_load_python2_str_as_bytes(self):
        # From Python 2: pickle.dumps('a\x00\xa0', protocol=0)
        self.assertEqual(self.loads(b"S'a\\x00\\xa0'\n.",
                                    encoding="bytes"), b'a\x00\xa0')
        # From Python 2: pickle.dumps('a\x00\xa0', protocol=1)
        self.assertEqual(self.loads(b'U\x03a\x00\xa0.',
                                    encoding="bytes"), b'a\x00\xa0')
        # From Python 2: pickle.dumps('a\x00\xa0', protocol=2)
        self.assertEqual(self.loads(b'\x80\x02U\x03a\x00\xa0.',
                                    encoding="bytes"), b'a\x00\xa0')

    def test_load_python2_unicode_as_str(self):
        # From Python 2: pickle.dumps(u'π', protocol=0)
        self.assertEqual(self.loads(b'V\\u03c0\n.',
                                    encoding='bytes'), 'π')
        # From Python 2: pickle.dumps(u'π', protocol=1)
        self.assertEqual(self.loads(b'X\x02\x00\x00\x00\xcf\x80.',
                                    encoding="bytes"), 'π')
        # From Python 2: pickle.dumps(u'π', protocol=2)
        self.assertEqual(self.loads(b'\x80\x02X\x02\x00\x00\x00\xcf\x80.',
                                    encoding="bytes"), 'π')

    def test_load_long_python2_str_as_bytes(self):
        # From Python 2: pickle.dumps('x' * 300, protocol=1)
        self.assertEqual(self.loads(pickle.BINSTRING +
                                    struct.pack("<I", 300) +
                                    b'x' * 300 + pickle.STOP,
                                    encoding='bytes'), b'x' * 300)

    def test_large_pickles(self):
        # Test the correctness of internal buffering routines when handling
        # large data.
        for proto in protocols:
            data = (1, min, b'xy' * (30 * 1024), len)
            dumped = self.dumps(data, proto)
            loaded = self.loads(dumped)
            self.assertEqual(len(loaded), len(data))
            self.assertEqual(loaded, data)

    def test_empty_bytestring(self):
        # issue 11286
        empty = self.loads(b'\x80\x03U\x00q\x00.', encoding='koi8-r')
        self.assertEqual(empty, '')

    def test_int_pickling_efficiency(self):
        # Test compacity of int representation (see issue #12744)
        for proto in protocols:
            with self.subTest(proto=proto):
                pickles = [self.dumps(2**n, proto) for n in range(70)]
                sizes = list(map(len, pickles))
                # the size function is monotonic
                self.assertEqual(sorted(sizes), sizes)
                if proto >= 2:
                    for p in pickles:
                        self.assertFalse(opcode_in_pickle(pickle.LONG, p))

    def check_negative_32b_binXXX(self, dumped):
        if sys.maxsize > 2**32:
            self.skipTest("test is only meaningful on 32-bit builds")
        # XXX Pure Python pickle reads lengths as signed and passes
        # them directly to read() (hence the EOFError)
        with self.assertRaises((pickle.UnpicklingError, EOFError,
                                ValueError, OverflowError)):
            self.loads(dumped)

    def test_negative_32b_binbytes(self):
        # On 32-bit builds, a BINBYTES of 2**31 or more is refused
        self.check_negative_32b_binXXX(b'\x80\x03B\xff\xff\xff\xffxyzq\x00.')

    def test_negative_32b_binunicode(self):
        # On 32-bit builds, a BINUNICODE of 2**31 or more is refused
        self.check_negative_32b_binXXX(b'\x80\x03X\xff\xff\xff\xffxyzq\x00.')

    def test_negative_put(self):
        # Issue #12847
        dumped = b'Va\np-1\n.'
        self.assertRaises(ValueError, self.loads, dumped)

    def test_negative_32b_binput(self):
        # Issue #12847
        if sys.maxsize > 2**32:
            self.skipTest("test is only meaningful on 32-bit builds")
        dumped = b'\x80\x03X\x01\x00\x00\x00ar\xff\xff\xff\xff.'
        self.assertRaises(ValueError, self.loads, dumped)

    def test_badly_escaped_string(self):
        self.assertRaises(ValueError, self.loads, b"S'\\'\n.")

    def test_badly_quoted_string(self):
        # Issue #17710
        badpickles = [b"S'\n.",
                      b'S"\n.',
                      b'S\' \n.',
                      b'S" \n.',
                      b'S\'"\n.',
                      b'S"\'\n.',
                      b"S' ' \n.",
                      b'S" " \n.',
                      b"S ''\n.",
                      b'S ""\n.',
                      b'S \n.',
                      b'S\n.',
                      b'S.']
        for p in badpickles:
            self.assertRaises(pickle.UnpicklingError, self.loads, p)

    def test_correctly_quoted_string(self):
        goodpickles = [(b"S''\n.", ''),
                       (b'S""\n.', ''),
                       (b'S"\\n"\n.', '\n'),
                       (b"S'\\n'\n.", '\n')]
        for p, expected in goodpickles:
            self.assertEqual(self.loads(p), expected)

    def _check_pickling_with_opcode(self, obj, opcode, proto):
        pickled = self.dumps(obj, proto)
        self.assertTrue(opcode_in_pickle(opcode, pickled))
        unpickled = self.loads(pickled)
        self.assertEqual(obj, unpickled)

    def test_appends_on_non_lists(self):
        # Issue #17720
        obj = REX_six([1, 2, 3])
        for proto in protocols:
            if proto == 0:
                self._check_pickling_with_opcode(obj, pickle.APPEND, proto)
            else:
                self._check_pickling_with_opcode(obj, pickle.APPENDS, proto)

    def test_setitems_on_non_dicts(self):
        obj = REX_seven({1: -1, 2: -2, 3: -3})
        for proto in protocols:
            if proto == 0:
                self._check_pickling_with_opcode(obj, pickle.SETITEM, proto)
            else:
                self._check_pickling_with_opcode(obj, pickle.SETITEMS, proto)

    # Exercise framing (proto >= 4) for significant workloads

    FRAME_SIZE_TARGET = 64 * 1024

    def check_frame_opcodes(self, pickled):
        """
        Check the arguments of FRAME opcodes in a protocol 4+ pickle.
        """
        frame_opcode_size = 9
        last_arg = last_pos = None
        for op, arg, pos in pickletools.genops(pickled):
            if op.name != 'FRAME':
                continue
            if last_pos is not None:
                # The previous frame's size should be equal to the number
                # of bytes up to the current frame.
                frame_size = pos - last_pos - frame_opcode_size
                self.assertEqual(frame_size, last_arg)
            last_arg, last_pos = arg, pos
        # The last frame's size should be equal to the number of bytes up
        # to the pickle's end.
        frame_size = len(pickled) - last_pos - frame_opcode_size
        self.assertEqual(frame_size, last_arg)

    def test_framing_many_objects(self):
        obj = list(range(10**5))
        for proto in range(4, pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                pickled = self.dumps(obj, proto)
                unpickled = self.loads(pickled)
                self.assertEqual(obj, unpickled)
                bytes_per_frame = (len(pickled) /
                                   count_opcode(pickle.FRAME, pickled))
                self.assertGreater(bytes_per_frame,
                                   self.FRAME_SIZE_TARGET / 2)
                self.assertLessEqual(bytes_per_frame,
                                     self.FRAME_SIZE_TARGET * 1)
                self.check_frame_opcodes(pickled)

    def test_framing_large_objects(self):
        N = 1024 * 1024
        obj = [b'x' * N, b'y' * N, b'z' * N]
        for proto in range(4, pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                pickled = self.dumps(obj, proto)
                unpickled = self.loads(pickled)
                self.assertEqual(obj, unpickled)
                n_frames = count_opcode(pickle.FRAME, pickled)
                self.assertGreaterEqual(n_frames, len(obj))
                self.check_frame_opcodes(pickled)

    def test_optional_frames(self):
        if pickle.HIGHEST_PROTOCOL < 4:
            return

        def remove_frames(pickled, keep_frame=None):
            """Remove frame opcodes from the given pickle."""
            frame_starts = []
            # 1 byte for the opcode and 8 for the argument
            frame_opcode_size = 9
            for opcode, _, pos in pickletools.genops(pickled):
                if opcode.name == 'FRAME':
                    frame_starts.append(pos)

            newpickle = bytearray()
            last_frame_end = 0
            for i, pos in enumerate(frame_starts):
                if keep_frame and keep_frame(i):
                    continue
                newpickle += pickled[last_frame_end:pos]
                last_frame_end = pos + frame_opcode_size
            newpickle += pickled[last_frame_end:]
            return newpickle

        frame_size = self.FRAME_SIZE_TARGET
        num_frames = 20
        obj = [bytes([i]) * frame_size for i in range(num_frames)]

        for proto in range(4, pickle.HIGHEST_PROTOCOL + 1):
            pickled = self.dumps(obj, proto)

            frameless_pickle = remove_frames(pickled)
            self.assertEqual(count_opcode(pickle.FRAME, frameless_pickle), 0)
            self.assertEqual(obj, self.loads(frameless_pickle))

            some_frames_pickle = remove_frames(pickled, lambda i: i % 2)
            self.assertLess(count_opcode(pickle.FRAME, some_frames_pickle),
                            count_opcode(pickle.FRAME, pickled))
            self.assertEqual(obj, self.loads(some_frames_pickle))

    def test_frame_readline(self):
        pickled = b'\x80\x04\x95\x05\x00\x00\x00\x00\x00\x00\x00I42\n.'
        #    0: \x80 PROTO      4
        #    2: \x95 FRAME      5
        #   11: I    INT        42
        #   15: .    STOP
        self.assertEqual(self.loads(pickled), 42)

    def test_nested_names(self):
        global Nested
        class Nested:
            class A:
                class B:
                    class C:
                        pass

        for proto in range(4, pickle.HIGHEST_PROTOCOL + 1):
            for obj in [Nested.A, Nested.A.B, Nested.A.B.C]:
                with self.subTest(proto=proto, obj=obj):
                    unpickled = self.loads(self.dumps(obj, proto))
                    self.assertIs(obj, unpickled)

    def test_py_methods(self):
        global PyMethodsTest
        class PyMethodsTest:
            @staticmethod
            def cheese():
                return "cheese"
            @classmethod
            def wine(cls):
                assert cls is PyMethodsTest
                return "wine"
            def biscuits(self):
                assert isinstance(self, PyMethodsTest)
                return "biscuits"
            class Nested:
                "Nested class"
                @staticmethod
                def ketchup():
                    return "ketchup"
                @classmethod
                def maple(cls):
                    assert cls is PyMethodsTest.Nested
                    return "maple"
                def pie(self):
                    assert isinstance(self, PyMethodsTest.Nested)
                    return "pie"

        py_methods = (
            PyMethodsTest.cheese,
            PyMethodsTest.wine,
            PyMethodsTest().biscuits,
            PyMethodsTest.Nested.ketchup,
            PyMethodsTest.Nested.maple,
            PyMethodsTest.Nested().pie
        )
        py_unbound_methods = (
            (PyMethodsTest.biscuits, PyMethodsTest),
            (PyMethodsTest.Nested.pie, PyMethodsTest.Nested)
        )
        for proto in range(4, pickle.HIGHEST_PROTOCOL + 1):
            for method in py_methods:
                with self.subTest(proto=proto, method=method):
                    unpickled = self.loads(self.dumps(method, proto))
                    self.assertEqual(method(), unpickled())
            for method, cls in py_unbound_methods:
                obj = cls()
                with self.subTest(proto=proto, method=method):
                    unpickled = self.loads(self.dumps(method, proto))
                    self.assertEqual(method(obj), unpickled(obj))

    def test_c_methods(self):
        global Subclass
        class Subclass(tuple):
            class Nested(str):
                pass

        c_methods = (
            # bound built-in method
            ("abcd".index, ("c",)),
            # unbound built-in method
            (str.index, ("abcd", "c")),
            # bound "slot" method
            ([1, 2, 3].__len__, ()),
            # unbound "slot" method
            (list.__len__, ([1, 2, 3],)),
            # bound "coexist" method
            ({1, 2}.__contains__, (2,)),
            # unbound "coexist" method
            (set.__contains__, ({1, 2}, 2)),
            # built-in class method
            (dict.fromkeys, (("a", 1), ("b", 2))),
            # built-in static method
            (bytearray.maketrans, (b"abc", b"xyz")),
            # subclass methods
            (Subclass([1,2,2]).count, (2,)),
            (Subclass.count, (Subclass([1,2,2]), 2)),
            (Subclass.Nested("sweet").count, ("e",)),
            (Subclass.Nested.count, (Subclass.Nested("sweet"), "e")),
        )
        for proto in range(4, pickle.HIGHEST_PROTOCOL + 1):
            for method, args in c_methods:
                with self.subTest(proto=proto, method=method):
                    unpickled = self.loads(self.dumps(method, proto))
                    self.assertEqual(method(*args), unpickled(*args))


class BigmemPickleTests(unittest.TestCase):

    # Binary protocols can serialize longs of up to 2GB-1

    @bigmemtest(size=_2G, memuse=3.6, dry_run=False)
    def test_huge_long_32b(self, size):
        data = 1 << (8 * size)
        try:
            for proto in protocols:
                if proto < 2:
                    continue
                with self.subTest(proto=proto):
                    with self.assertRaises((ValueError, OverflowError)):
                        self.dumps(data, protocol=proto)
        finally:
            data = None

    # Protocol 3 can serialize up to 4GB-1 as a bytes object
    # (older protocols don't have a dedicated opcode for bytes and are
    # too inefficient)

    @bigmemtest(size=_2G, memuse=2.5, dry_run=False)
    def test_huge_bytes_32b(self, size):
        data = b"abcd" * (size // 4)
        try:
            for proto in protocols:
                if proto < 3:
                    continue
                with self.subTest(proto=proto):
                    try:
                        pickled = self.dumps(data, protocol=proto)
                        header = (pickle.BINBYTES +
                                  struct.pack("<I", len(data)))
                        data_start = pickled.index(data)
                        self.assertEqual(
                            header,
                            pickled[data_start-len(header):data_start])
                    finally:
                        pickled = None
        finally:
            data = None

    @bigmemtest(size=_4G, memuse=2.5, dry_run=False)
    def test_huge_bytes_64b(self, size):
        data = b"acbd" * (size // 4)
        try:
            for proto in protocols:
                if proto < 3:
                    continue
                with self.subTest(proto=proto):
                    if proto == 3:
                        # Protocol 3 does not support large bytes objects.
                        # Verify that we do not crash when processing one.
                        with self.assertRaises((ValueError, OverflowError)):
                            self.dumps(data, protocol=proto)
                        continue
                    try:
                        pickled = self.dumps(data, protocol=proto)
                        header = (pickle.BINBYTES8 +
                                  struct.pack("<Q", len(data)))
                        data_start = pickled.index(data)
                        self.assertEqual(
                            header,
                            pickled[data_start-len(header):data_start])
                    finally:
                        pickled = None
        finally:
            data = None

    # All protocols use 1-byte per printable ASCII character; we add another
    # byte because the encoded form has to be copied into the internal buffer.

    @bigmemtest(size=_2G, memuse=8, dry_run=False)
    def test_huge_str_32b(self, size):
        data = "abcd" * (size // 4)
        try:
            for proto in protocols:
                if proto == 0:
                    continue
                with self.subTest(proto=proto):
                    try:
                        pickled = self.dumps(data, protocol=proto)
                        header = (pickle.BINUNICODE +
                                  struct.pack("<I", len(data)))
                        data_start = pickled.index(b'abcd')
                        self.assertEqual(
                            header,
                            pickled[data_start-len(header):data_start])
                        self.assertEqual((pickled.rindex(b"abcd") + len(b"abcd") -
                                          pickled.index(b"abcd")), len(data))
                    finally:
                        pickled = None
        finally:
            data = None

    # BINUNICODE (protocols 1, 2 and 3) cannot carry more than 2**32 - 1 bytes
    # of utf-8 encoded unicode. BINUNICODE8 (protocol 4) supports these huge
    # unicode strings however.

    @bigmemtest(size=_4G, memuse=8, dry_run=False)
    def test_huge_str_64b(self, size):
        data = "abcd" * (size // 4)
        try:
            for proto in protocols:
                if proto == 0:
                    continue
                with self.subTest(proto=proto):
                    if proto < 4:
                        with self.assertRaises((ValueError, OverflowError)):
                            self.dumps(data, protocol=proto)
                        continue
                    try:
                        pickled = self.dumps(data, protocol=proto)
                        header = (pickle.BINUNICODE8 +
                                  struct.pack("<Q", len(data)))
                        data_start = pickled.index(b'abcd')
                        self.assertEqual(
                            header,
                            pickled[data_start-len(header):data_start])
                        self.assertEqual((pickled.rindex(b"abcd") + len(b"abcd") -
                                          pickled.index(b"abcd")), len(data))
                    finally:
                        pickled = None
        finally:
            data = None


# Test classes for reduce_ex

class REX_one(object):
    """No __reduce_ex__ here, but inheriting it from object"""
    _reduce_called = 0
    def __reduce__(self):
        self._reduce_called = 1
        return REX_one, ()

class REX_two(object):
    """No __reduce__ here, but inheriting it from object"""
    _proto = None
    def __reduce_ex__(self, proto):
        self._proto = proto
        return REX_two, ()

class REX_three(object):
    _proto = None
    def __reduce_ex__(self, proto):
        self._proto = proto
        return REX_two, ()
    def __reduce__(self):
        raise TestFailed("This __reduce__ shouldn't be called")

class REX_four(object):
    """Calling base class method should succeed"""
    _proto = None
    def __reduce_ex__(self, proto):
        self._proto = proto
        return object.__reduce_ex__(self, proto)

class REX_five(object):
    """This one used to fail with infinite recursion"""
    _reduce_called = 0
    def __reduce__(self):
        self._reduce_called = 1
        return object.__reduce__(self)

class REX_six(object):
    """This class is used to check the 4th argument (list iterator) of
    the reduce protocol.
    """
    def __init__(self, items=None):
        self.items = items if items is not None else []
    def __eq__(self, other):
        return type(self) is type(other) and self.items == self.items
    def append(self, item):
        self.items.append(item)
    def __reduce__(self):
        return type(self), (), None, iter(self.items), None

class REX_seven(object):
    """This class is used to check the 5th argument (dict iterator) of
    the reduce protocol.
    """
    def __init__(self, table=None):
        self.table = table if table is not None else {}
    def __eq__(self, other):
        return type(self) is type(other) and self.table == self.table
    def __setitem__(self, key, value):
        self.table[key] = value
    def __reduce__(self):
        return type(self), (), None, None, iter(self.table.items())


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

class MySet(set):
    sample = {"a", "b"}

class MyFrozenSet(frozenset):
    sample = frozenset({"a", "b"})

myclasses = [MyInt, MyFloat,
             MyComplex,
             MyStr, MyUnicode,
             MyTuple, MyList, MyDict, MySet, MyFrozenSet]


class SlotList(MyList):
    __slots__ = ["foo"]

class SimpleNewObj(object):
    def __init__(self, a, b, c):
        # raise an error, to make sure this isn't called
        raise TypeError("SimpleNewObj.__init__() didn't expect to get called")
    def __eq__(self, other):
        return self.__dict__ == other.__dict__

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
        self.assertEqual(pickle.HIGHEST_PROTOCOL, 4)

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


class AbstractPersistentPicklerTests(unittest.TestCase):

    # This class defines persistent_id() and persistent_load()
    # functions that should be used by the pickler.  All even integers
    # are pickled using persistent ids.

    def persistent_id(self, object):
        if isinstance(object, int) and object % 2 == 0:
            self.id_count += 1
            return str(object)
        elif object == "test_false_value":
            self.false_count += 1
            return ""
        else:
            return None

    def persistent_load(self, oid):
        if not oid:
            self.load_false_count += 1
            return "test_false_value"
        else:
            self.load_count += 1
            object = int(oid)
            assert object % 2 == 0
            return object

    def test_persistence(self):
        L = list(range(10)) + ["test_false_value"]
        for proto in protocols:
            self.id_count = 0
            self.false_count = 0
            self.load_false_count = 0
            self.load_count = 0
            self.assertEqual(self.loads(self.dumps(L, proto)), L)
            self.assertEqual(self.id_count, 5)
            self.assertEqual(self.false_count, 1)
            self.assertEqual(self.load_count, 5)
            self.assertEqual(self.load_false_count, 1)


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

        # Reset BytesIO object.
        f.seek(0)
        f.truncate()

        pickler.dump(data)
        second_pickled = f.getvalue()

        # Reset the Pickler and BytesIO objects.
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
            with self.subTest(proto=proto):
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

    def test_unpickling_buffering_readline(self):
        # Issue #12687: the unpickler's buffering logic could fail with
        # text mode opcodes.
        data = list(range(10))
        for proto in protocols:
            for buf_size in range(1, 11):
                f = io.BufferedRandom(io.BytesIO(), buffer_size=buf_size)
                pickler = self.pickler_class(f, protocol=proto)
                pickler.dump(data)
                f.seek(0)
                unpickler = self.unpickler_class(f)
                self.assertEqual(unpickler.load(), data)


# Tests for dispatch_table attribute

REDUCE_A = 'reduce_A'

class AAA(object):
    def __reduce__(self):
        return str, (REDUCE_A,)

class BBB(object):
    pass

class AbstractDispatchTableTests(unittest.TestCase):

    def test_default_dispatch_table(self):
        # No dispatch_table attribute by default
        f = io.BytesIO()
        p = self.pickler_class(f, 0)
        with self.assertRaises(AttributeError):
            p.dispatch_table
        self.assertFalse(hasattr(p, 'dispatch_table'))

    def test_class_dispatch_table(self):
        # A dispatch_table attribute can be specified class-wide
        dt = self.get_dispatch_table()

        class MyPickler(self.pickler_class):
            dispatch_table = dt

        def dumps(obj, protocol=None):
            f = io.BytesIO()
            p = MyPickler(f, protocol)
            self.assertEqual(p.dispatch_table, dt)
            p.dump(obj)
            return f.getvalue()

        self._test_dispatch_table(dumps, dt)

    def test_instance_dispatch_table(self):
        # A dispatch_table attribute can also be specified instance-wide
        dt = self.get_dispatch_table()

        def dumps(obj, protocol=None):
            f = io.BytesIO()
            p = self.pickler_class(f, protocol)
            p.dispatch_table = dt
            self.assertEqual(p.dispatch_table, dt)
            p.dump(obj)
            return f.getvalue()

        self._test_dispatch_table(dumps, dt)

    def _test_dispatch_table(self, dumps, dispatch_table):
        def custom_load_dump(obj):
            return pickle.loads(dumps(obj, 0))

        def default_load_dump(obj):
            return pickle.loads(pickle.dumps(obj, 0))

        # pickling complex numbers using protocol 0 relies on copyreg
        # so check pickling a complex number still works
        z = 1 + 2j
        self.assertEqual(custom_load_dump(z), z)
        self.assertEqual(default_load_dump(z), z)

        # modify pickling of complex
        REDUCE_1 = 'reduce_1'
        def reduce_1(obj):
            return str, (REDUCE_1,)
        dispatch_table[complex] = reduce_1
        self.assertEqual(custom_load_dump(z), REDUCE_1)
        self.assertEqual(default_load_dump(z), z)

        # check picklability of AAA and BBB
        a = AAA()
        b = BBB()
        self.assertEqual(custom_load_dump(a), REDUCE_A)
        self.assertIsInstance(custom_load_dump(b), BBB)
        self.assertEqual(default_load_dump(a), REDUCE_A)
        self.assertIsInstance(default_load_dump(b), BBB)

        # modify pickling of BBB
        dispatch_table[BBB] = reduce_1
        self.assertEqual(custom_load_dump(a), REDUCE_A)
        self.assertEqual(custom_load_dump(b), REDUCE_1)
        self.assertEqual(default_load_dump(a), REDUCE_A)
        self.assertIsInstance(default_load_dump(b), BBB)

        # revert pickling of BBB and modify pickling of AAA
        REDUCE_2 = 'reduce_2'
        def reduce_2(obj):
            return str, (REDUCE_2,)
        dispatch_table[AAA] = reduce_2
        del dispatch_table[BBB]
        self.assertEqual(custom_load_dump(a), REDUCE_2)
        self.assertIsInstance(custom_load_dump(b), BBB)
        self.assertEqual(default_load_dump(a), REDUCE_A)
        self.assertIsInstance(default_load_dump(b), BBB)


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
