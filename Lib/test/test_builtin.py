# Python test set -- built-in functions

import test.test_support, unittest
from test.test_support import fcmp, TESTFN, unlink,  run_unittest, \
                              run_with_locale
from operator import neg

import sys, warnings, random, UserDict, io
warnings.filterwarnings("ignore", "hex../oct.. of negative int",
                        FutureWarning, __name__)
warnings.filterwarnings("ignore", "integer argument expected",
                        DeprecationWarning, "unittest")

class Squares:

    def __init__(self, max):
        self.max = max
        self.sofar = []

    def __len__(self): return len(self.sofar)

    def __getitem__(self, i):
        if not 0 <= i < self.max: raise IndexError
        n = len(self.sofar)
        while n <= i:
            self.sofar.append(n*n)
            n += 1
        return self.sofar[i]

class StrSquares:

    def __init__(self, max):
        self.max = max
        self.sofar = []

    def __len__(self):
        return len(self.sofar)

    def __getitem__(self, i):
        if not 0 <= i < self.max:
            raise IndexError
        n = len(self.sofar)
        while n <= i:
            self.sofar.append(str(n*n))
            n += 1
        return self.sofar[i]

class BitBucket:
    def write(self, line):
        pass

L = [
        ('0', 0),
        ('1', 1),
        ('9', 9),
        ('10', 10),
        ('99', 99),
        ('100', 100),
        ('314', 314),
        (' 314', 314),
        ('314 ', 314),
        ('  \t\t  314  \t\t  ', 314),
        (repr(sys.maxint), sys.maxint),
        ('  1x', ValueError),
        ('  1  ', 1),
        ('  1\02  ', ValueError),
        ('', ValueError),
        (' ', ValueError),
        ('  \t\t  ', ValueError),
        (str(b'\u0663\u0661\u0664 ','raw-unicode-escape'), 314),
        (chr(0x200), ValueError),
]

class TestFailingBool:
    def __bool__(self):
        raise RuntimeError

class TestFailingIter:
    def __iter__(self):
        raise RuntimeError

class BuiltinTest(unittest.TestCase):

    def test_import(self):
        __import__('sys')
        __import__('time')
        __import__('string')
        __import__(name='sys')
        __import__(name='time', level=0)
        self.assertRaises(ImportError, __import__, 'spamspam')
        self.assertRaises(TypeError, __import__, 1, 2, 3, 4)
        self.assertRaises(ValueError, __import__, '')
        self.assertRaises(TypeError, __import__, 'sys', name='sys')

    def test_abs(self):
        # int
        self.assertEqual(abs(0), 0)
        self.assertEqual(abs(1234), 1234)
        self.assertEqual(abs(-1234), 1234)
        self.assertTrue(abs(-sys.maxint-1) > 0)
        # float
        self.assertEqual(abs(0.0), 0.0)
        self.assertEqual(abs(3.14), 3.14)
        self.assertEqual(abs(-3.14), 3.14)
        # long
        self.assertEqual(abs(0), 0)
        self.assertEqual(abs(1234), 1234)
        self.assertEqual(abs(-1234), 1234)
        # str
        self.assertRaises(TypeError, abs, 'a')

    def test_all(self):
        self.assertEqual(all([2, 4, 6]), True)
        self.assertEqual(all([2, None, 6]), False)
        self.assertRaises(RuntimeError, all, [2, TestFailingBool(), 6])
        self.assertRaises(RuntimeError, all, TestFailingIter())
        self.assertRaises(TypeError, all, 10)               # Non-iterable
        self.assertRaises(TypeError, all)                   # No args
        self.assertRaises(TypeError, all, [2, 4, 6], [])    # Too many args
        self.assertEqual(all([]), True)                     # Empty iterator
        S = [50, 60]
        self.assertEqual(all(x > 42 for x in S), True)
        S = [50, 40, 60]
        self.assertEqual(all(x > 42 for x in S), False)

    def test_any(self):
        self.assertEqual(any([None, None, None]), False)
        self.assertEqual(any([None, 4, None]), True)
        self.assertRaises(RuntimeError, any, [None, TestFailingBool(), 6])
        self.assertRaises(RuntimeError, all, TestFailingIter())
        self.assertRaises(TypeError, any, 10)               # Non-iterable
        self.assertRaises(TypeError, any)                   # No args
        self.assertRaises(TypeError, any, [2, 4, 6], [])    # Too many args
        self.assertEqual(any([]), False)                    # Empty iterator
        S = [40, 60, 30]
        self.assertEqual(any(x > 42 for x in S), True)
        S = [10, 20, 30]
        self.assertEqual(any(x > 42 for x in S), False)

    def test_neg(self):
        x = -sys.maxint-1
        self.assert_(isinstance(x, int))
        self.assertEqual(-x, sys.maxint+1)

    # XXX(nnorwitz): This test case for callable should probably be removed.
    def test_callable(self):
        self.assert_(hasattr(len, '__call__'))
        def f(): pass
        self.assert_(hasattr(f, '__call__'))
        class C:
            def meth(self): pass
        self.assert_(hasattr(C, '__call__'))
        x = C()
        self.assert_(hasattr(x.meth, '__call__'))
        self.assert_(not hasattr(x, '__call__'))
        class D(C):
            def __call__(self): pass
        y = D()
        self.assert_(hasattr(y, '__call__'))
        y()

    def test_chr(self):
        self.assertEqual(chr(32), ' ')
        self.assertEqual(chr(65), 'A')
        self.assertEqual(chr(97), 'a')
        self.assertEqual(chr(0xff), '\xff')
        self.assertRaises(ValueError, chr, 1<<24)
        self.assertEqual(chr(sys.maxunicode),
                         str(('\\U%08x' % (sys.maxunicode)).encode("ascii"),
                             'unicode-escape'))
        self.assertRaises(TypeError, chr)
        self.assertEqual(chr(0x0000FFFF), "\U0000FFFF")
        self.assertEqual(chr(0x00010000), "\U00010000")
        self.assertEqual(chr(0x00010001), "\U00010001")
        self.assertEqual(chr(0x000FFFFE), "\U000FFFFE")
        self.assertEqual(chr(0x000FFFFF), "\U000FFFFF")
        self.assertEqual(chr(0x00100000), "\U00100000")
        self.assertEqual(chr(0x00100001), "\U00100001")
        self.assertEqual(chr(0x0010FFFE), "\U0010FFFE")
        self.assertEqual(chr(0x0010FFFF), "\U0010FFFF")
        self.assertRaises(ValueError, chr, -1)
        self.assertRaises(ValueError, chr, 0x00110000)

    def test_cmp(self):
        self.assertEqual(cmp(-1, 1), -1)
        self.assertEqual(cmp(1, -1), 1)
        self.assertEqual(cmp(1, 1), 0)
        # verify that circular objects are not handled
        a = []; a.append(a)
        b = []; b.append(b)
        from UserList import UserList
        c = UserList(); c.append(c)
        self.assertRaises(RuntimeError, cmp, a, b)
        self.assertRaises(RuntimeError, cmp, b, c)
        self.assertRaises(RuntimeError, cmp, c, a)
        self.assertRaises(RuntimeError, cmp, a, c)
       # okay, now break the cycles
        a.pop(); b.pop(); c.pop()
        self.assertRaises(TypeError, cmp)

    def test_compile(self):
        compile('print(1)\n', '', 'exec')
##         bom = b'\xef\xbb\xbf'
##         compile(bom + b'print(1)\n', '', 'exec')
        compile(source='pass', filename='?', mode='exec')
        compile(dont_inherit=0, filename='tmp', source='0', mode='eval')
        compile('pass', '?', dont_inherit=1, mode='exec')
        self.assertRaises(TypeError, compile)
        self.assertRaises(ValueError, compile, 'print(42)\n', '<string>', 'badmode')
        self.assertRaises(ValueError, compile, 'print(42)\n', '<string>', 'single', 0xff)
        self.assertRaises(TypeError, compile, chr(0), 'f', 'exec')
        self.assertRaises(TypeError, compile, 'pass', '?', 'exec',
                          mode='eval', source='0', filename='tmp')
        compile('print("\xe5")\n', '', 'exec')
        self.assertRaises(TypeError, compile, chr(0), 'f', 'exec')
        self.assertRaises(ValueError, compile, str('a = 1'), 'f', 'bad')

    def test_delattr(self):
        import sys
        sys.spam = 1
        delattr(sys, 'spam')
        self.assertRaises(TypeError, delattr)

    def test_dir(self):
        # dir(wrong number of arguments)
        self.assertRaises(TypeError, dir, 42, 42)

        # dir() - local scope
        local_var = 1
        self.assert_('local_var' in dir())

        # dir(module)
        import sys
        self.assert_('exit' in dir(sys))

        # dir(module_with_invalid__dict__)
        import types
        class Foo(types.ModuleType):
            __dict__ = 8
        f = Foo("foo")
        self.assertRaises(TypeError, dir, f)

        # dir(type)
        self.assert_("strip" in dir(str))
        self.assert_("__mro__" not in dir(str))

        # dir(obj)
        class Foo(object):
            def __init__(self):
                self.x = 7
                self.y = 8
                self.z = 9
        f = Foo()
        self.assert_("y" in dir(f))

        # dir(obj_no__dict__)
        class Foo(object):
            __slots__ = []
        f = Foo()
        self.assert_("__repr__" in dir(f))

        # dir(obj_no__class__with__dict__)
        # (an ugly trick to cause getattr(f, "__class__") to fail)
        class Foo(object):
            __slots__ = ["__class__", "__dict__"]
            def __init__(self):
                self.bar = "wow"
        f = Foo()
        self.assert_("__repr__" not in dir(f))
        self.assert_("bar" in dir(f))

        # dir(obj_using __dir__)
        class Foo(object):
            def __dir__(self):
                return ["kan", "ga", "roo"]
        f = Foo()
        self.assert_(dir(f) == ["ga", "kan", "roo"])

        # dir(obj__dir__not_list)
        class Foo(object):
            def __dir__(self):
                return 7
        f = Foo()
        self.assertRaises(TypeError, dir, f)

        # dir(traceback)
        try:
            raise IndexError
        except:
            self.assertEqual(len(dir(sys.exc_info()[2])), 4)


    def test_divmod(self):
        self.assertEqual(divmod(12, 7), (1, 5))
        self.assertEqual(divmod(-12, 7), (-2, 2))
        self.assertEqual(divmod(12, -7), (-2, -2))
        self.assertEqual(divmod(-12, -7), (1, -5))

        self.assertEqual(divmod(12, 7), (1, 5))
        self.assertEqual(divmod(-12, 7), (-2, 2))
        self.assertEqual(divmod(12, -7), (-2, -2))
        self.assertEqual(divmod(-12, -7), (1, -5))

        self.assertEqual(divmod(12, 7), (1, 5))
        self.assertEqual(divmod(-12, 7), (-2, 2))
        self.assertEqual(divmod(12, -7), (-2, -2))
        self.assertEqual(divmod(-12, -7), (1, -5))

        self.assertEqual(divmod(-sys.maxint-1, -1),
                         (sys.maxint+1, 0))

        self.assert_(not fcmp(divmod(3.25, 1.0), (3.0, 0.25)))
        self.assert_(not fcmp(divmod(-3.25, 1.0), (-4.0, 0.75)))
        self.assert_(not fcmp(divmod(3.25, -1.0), (-4.0, -0.75)))
        self.assert_(not fcmp(divmod(-3.25, -1.0), (3.0, -0.25)))

        self.assertRaises(TypeError, divmod)

    def test_eval(self):
        self.assertEqual(eval('1+1'), 2)
        self.assertEqual(eval(' 1+1\n'), 2)
        globals = {'a': 1, 'b': 2}
        locals = {'b': 200, 'c': 300}
        self.assertEqual(eval('a', globals) , 1)
        self.assertEqual(eval('a', globals, locals), 1)
        self.assertEqual(eval('b', globals, locals), 200)
        self.assertEqual(eval('c', globals, locals), 300)
        globals = {'a': 1, 'b': 2}
        locals = {'b': 200, 'c': 300}
##         bom = b'\xef\xbb\xbf'
##         self.assertEqual(eval(bom + b'a', globals, locals), 1)
        self.assertEqual(eval('"\xe5"', globals), "\xe5")
        self.assertRaises(TypeError, eval)
        self.assertRaises(TypeError, eval, ())

    def test_general_eval(self):
        # Tests that general mappings can be used for the locals argument

        class M:
            "Test mapping interface versus possible calls from eval()."
            def __getitem__(self, key):
                if key == 'a':
                    return 12
                raise KeyError
            def keys(self):
                return list('xyz')

        m = M()
        g = globals()
        self.assertEqual(eval('a', g, m), 12)
        self.assertRaises(NameError, eval, 'b', g, m)
        self.assertEqual(eval('dir()', g, m), list('xyz'))
        self.assertEqual(eval('globals()', g, m), g)
        self.assertEqual(eval('locals()', g, m), m)
        self.assertRaises(TypeError, eval, 'a', m)
        class A:
            "Non-mapping"
            pass
        m = A()
        self.assertRaises(TypeError, eval, 'a', g, m)

        # Verify that dict subclasses work as well
        class D(dict):
            def __getitem__(self, key):
                if key == 'a':
                    return 12
                return dict.__getitem__(self, key)
            def keys(self):
                return list('xyz')

        d = D()
        self.assertEqual(eval('a', g, d), 12)
        self.assertRaises(NameError, eval, 'b', g, d)
        self.assertEqual(eval('dir()', g, d), list('xyz'))
        self.assertEqual(eval('globals()', g, d), g)
        self.assertEqual(eval('locals()', g, d), d)

        # Verify locals stores (used by list comps)
        eval('[locals() for i in (2,3)]', g, d)
        eval('[locals() for i in (2,3)]', g, UserDict.UserDict())

        class SpreadSheet:
            "Sample application showing nested, calculated lookups."
            _cells = {}
            def __setitem__(self, key, formula):
                self._cells[key] = formula
            def __getitem__(self, key):
                return eval(self._cells[key], globals(), self)

        ss = SpreadSheet()
        ss['a1'] = '5'
        ss['a2'] = 'a1*6'
        ss['a3'] = 'a2*7'
        self.assertEqual(ss['a3'], 210)

        # Verify that dir() catches a non-list returned by eval
        # SF bug #1004669
        class C:
            def __getitem__(self, item):
                raise KeyError(item)
            def keys(self):
                return 1 # used to be 'a' but that's no longer an error
        self.assertRaises(TypeError, eval, 'dir()', globals(), C())

    def test_exec(self):
        g = {}
        exec('z = 1', g)
        if '__builtins__' in g:
            del g['__builtins__']
        self.assertEqual(g, {'z': 1})

        exec('z = 1+1', g)
        if '__builtins__' in g:
            del g['__builtins__']
        self.assertEqual(g, {'z': 2})
        g = {}
        l = {}

        import warnings
        warnings.filterwarnings("ignore", "global statement", module="<string>")
        exec('global a; a = 1; b = 2', g, l)
        if '__builtins__' in g:
            del g['__builtins__']
        if '__builtins__' in l:
            del l['__builtins__']
        self.assertEqual((g, l), ({'a': 1}, {'b': 2}))

    def test_filter(self):
        self.assertEqual(list(filter(lambda c: 'a' <= c <= 'z', 'Hello World')), list('elloorld'))
        self.assertEqual(list(filter(None, [1, 'hello', [], [3], '', None, 9, 0])), [1, 'hello', [3], 9])
        self.assertEqual(list(filter(lambda x: x > 0, [1, -3, 9, 0, 2])), [1, 9, 2])
        self.assertEqual(list(filter(None, Squares(10))), [1, 4, 9, 16, 25, 36, 49, 64, 81])
        self.assertEqual(list(filter(lambda x: x%2, Squares(10))), [1, 9, 25, 49, 81])
        def identity(item):
            return 1
        filter(identity, Squares(5))
        self.assertRaises(TypeError, filter)
        class BadSeq(object):
            def __getitem__(self, index):
                if index<4:
                    return 42
                raise ValueError
        self.assertRaises(ValueError, list, filter(lambda x: x, BadSeq()))
        def badfunc():
            pass
        self.assertRaises(TypeError, list, filter(badfunc, range(5)))

        # test bltinmodule.c::filtertuple()
        self.assertEqual(list(filter(None, (1, 2))), [1, 2])
        self.assertEqual(list(filter(lambda x: x>=3, (1, 2, 3, 4))), [3, 4])
        self.assertRaises(TypeError, list, filter(42, (1, 2)))

    def test_float(self):
        self.assertEqual(float(3.14), 3.14)
        self.assertEqual(float(314), 314.0)
        self.assertEqual(float(314), 314.0)
        self.assertEqual(float("  3.14  "), 3.14)
        self.assertRaises(ValueError, float, "  0x3.1  ")
        self.assertRaises(ValueError, float, "  -0x3.p-1  ")
        self.assertEqual(float(str(b"  \u0663.\u0661\u0664  ",'raw-unicode-escape')), 3.14)
        self.assertEqual(float("1"*10000), 1e10000) # Inf on both sides

    @run_with_locale('LC_NUMERIC', 'fr_FR', 'de_DE')
    def test_float_with_comma(self):
        # set locale to something that doesn't use '.' for the decimal point
        # float must not accept the locale specific decimal point but
        # it still has to accept the normal python syntac
        import locale
        if not locale.localeconv()['decimal_point'] == ',':
            return

        self.assertEqual(float("  3.14  "), 3.14)
        self.assertEqual(float("+3.14  "), 3.14)
        self.assertEqual(float("-3.14  "), -3.14)
        self.assertEqual(float(".14  "), .14)
        self.assertEqual(float("3.  "), 3.0)
        self.assertEqual(float("3.e3  "), 3000.0)
        self.assertEqual(float("3.2e3  "), 3200.0)
        self.assertEqual(float("2.5e-1  "), 0.25)
        self.assertEqual(float("5e-1"), 0.5)
        self.assertRaises(ValueError, float, "  3,14  ")
        self.assertRaises(ValueError, float, "  +3,14  ")
        self.assertRaises(ValueError, float, "  -3,14  ")
        self.assertRaises(ValueError, float, "  0x3.1  ")
        self.assertRaises(ValueError, float, "  -0x3.p-1  ")
        self.assertEqual(float("  25.e-1  "), 2.5)
        self.assertEqual(fcmp(float("  .25e-1  "), .025), 0)

    def test_floatconversion(self):
        # Make sure that calls to __float__() work properly
        class Foo0:
            def __float__(self):
                return 42.

        class Foo1(object):
            def __float__(self):
                return 42.

        class Foo2(float):
            def __float__(self):
                return 42.

        class Foo3(float):
            def __new__(cls, value=0.):
                return float.__new__(cls, 2*value)

            def __float__(self):
                return self

        class Foo4(float):
            def __float__(self):
                return 42

        self.assertAlmostEqual(float(Foo0()), 42.)
        self.assertAlmostEqual(float(Foo1()), 42.)
        self.assertAlmostEqual(float(Foo2()), 42.)
        self.assertAlmostEqual(float(Foo3(21)), 42.)
        self.assertRaises(TypeError, float, Foo4(42))

    def test_getattr(self):
        import sys
        self.assert_(getattr(sys, 'stdout') is sys.stdout)
        self.assertRaises(TypeError, getattr, sys, 1)
        self.assertRaises(TypeError, getattr, sys, 1, "foo")
        self.assertRaises(TypeError, getattr)
        self.assertRaises(AttributeError, getattr, sys, chr(sys.maxunicode))

    def test_hasattr(self):
        import sys
        self.assert_(hasattr(sys, 'stdout'))
        self.assertRaises(TypeError, hasattr, sys, 1)
        self.assertRaises(TypeError, hasattr)
        self.assertEqual(False, hasattr(sys, chr(sys.maxunicode)))

    def test_hash(self):
        hash(None)
        self.assertEqual(hash(1), hash(1))
        self.assertEqual(hash(1), hash(1.0))
        hash('spam')
        self.assertEqual(hash('spam'), hash(str8('spam')))
        hash((0,1,2,3))
        def f(): pass
        self.assertRaises(TypeError, hash, [])
        self.assertRaises(TypeError, hash, {})
        # Bug 1536021: Allow hash to return long objects
        class X:
            def __hash__(self):
                return 2**100
        self.assertEquals(type(hash(X())), int)
        class Y(object):
            def __hash__(self):
                return 2**100
        self.assertEquals(type(hash(Y())), int)
        class Z(int):
            def __hash__(self):
                return self
        self.assertEquals(hash(Z(42)), hash(42))

    def test_hex(self):
        self.assertEqual(hex(16), '0x10')
        self.assertEqual(hex(16), '0x10')
        self.assertEqual(hex(-16), '-0x10')
        self.assertEqual(hex(-16), '-0x10')
        self.assertRaises(TypeError, hex, {})

    def test_id(self):
        id(None)
        id(1)
        id(1)
        id(1.0)
        id('spam')
        id((0,1,2,3))
        id([0,1,2,3])
        id({'spam': 1, 'eggs': 2, 'ham': 3})

    # Test input() later, alphabetized as if it were raw_input

    def test_int(self):
        self.assertEqual(int(314), 314)
        self.assertEqual(int(3.14), 3)
        self.assertEqual(int(314), 314)
        # Check that conversion from float truncates towards zero
        self.assertEqual(int(-3.14), -3)
        self.assertEqual(int(3.9), 3)
        self.assertEqual(int(-3.9), -3)
        self.assertEqual(int(3.5), 3)
        self.assertEqual(int(-3.5), -3)
        # Different base:
        self.assertEqual(int("10",16), 16)
        # Test conversion from strings and various anomalies
        for s, v in L:
            for sign in "", "+", "-":
                for prefix in "", " ", "\t", "  \t\t  ":
                    ss = prefix + sign + s
                    vv = v
                    if sign == "-" and v is not ValueError:
                        vv = -v
                    try:
                        self.assertEqual(int(ss), vv)
                    except v:
                        pass

        s = repr(-1-sys.maxint)
        x = int(s)
        self.assertEqual(x+1, -sys.maxint)
        self.assert_(isinstance(x, int))
        # should return long
        self.assertEqual(int(s[1:]), sys.maxint+1)

        # should return long
        x = int(1e100)
        self.assert_(isinstance(x, int))
        x = int(-1e100)
        self.assert_(isinstance(x, int))


        # SF bug 434186:  0x80000000/2 != 0x80000000>>1.
        # Worked by accident in Windows release build, but failed in debug build.
        # Failed in all Linux builds.
        x = -1-sys.maxint
        self.assertEqual(x >> 1, x//2)

        self.assertRaises(ValueError, int, '123\0')
        self.assertRaises(ValueError, int, '53', 40)

        # SF bug 1545497: embedded NULs were not detected with
        # explicit base
        self.assertRaises(ValueError, int, '123\0', 10)
        self.assertRaises(ValueError, int, '123\x00 245', 20)

        x = int('1' * 600)
        self.assert_(isinstance(x, int))

        x = int(chr(0x661) * 600)
        self.assert_(isinstance(x, int))

        self.assertRaises(TypeError, int, 1, 12)

        # tests with base 0
        self.assertRaises(ValueError, int, ' 0123  ', 0) # old octal syntax
        self.assertEqual(int('000', 0), 0)
        self.assertEqual(int('0o123', 0), 83)
        self.assertEqual(int('0x123', 0), 291)
        self.assertEqual(int('0b100', 0), 4)
        self.assertEqual(int(' 0O123   ', 0), 83)
        self.assertEqual(int(' 0X123  ', 0), 291)
        self.assertEqual(int(' 0B100 ', 0), 4)

        # without base still base 10
        self.assertEqual(int('0123'), 123)
        self.assertEqual(int('0123', 10), 123)

        # tests with prefix and base != 0
        self.assertEqual(int('0x123', 16), 291)
        self.assertEqual(int('0o123', 8), 83)
        self.assertEqual(int('0b100', 2), 4)
        self.assertEqual(int('0X123', 16), 291)
        self.assertEqual(int('0O123', 8), 83)
        self.assertEqual(int('0B100', 2), 4)

        # SF bug 1334662: int(string, base) wrong answers
        # Various representations of 2**32 evaluated to 0
        # rather than 2**32 in previous versions

        self.assertEqual(int('100000000000000000000000000000000', 2), 4294967296)
        self.assertEqual(int('102002022201221111211', 3), 4294967296)
        self.assertEqual(int('10000000000000000', 4), 4294967296)
        self.assertEqual(int('32244002423141', 5), 4294967296)
        self.assertEqual(int('1550104015504', 6), 4294967296)
        self.assertEqual(int('211301422354', 7), 4294967296)
        self.assertEqual(int('40000000000', 8), 4294967296)
        self.assertEqual(int('12068657454', 9), 4294967296)
        self.assertEqual(int('4294967296', 10), 4294967296)
        self.assertEqual(int('1904440554', 11), 4294967296)
        self.assertEqual(int('9ba461594', 12), 4294967296)
        self.assertEqual(int('535a79889', 13), 4294967296)
        self.assertEqual(int('2ca5b7464', 14), 4294967296)
        self.assertEqual(int('1a20dcd81', 15), 4294967296)
        self.assertEqual(int('100000000', 16), 4294967296)
        self.assertEqual(int('a7ffda91', 17), 4294967296)
        self.assertEqual(int('704he7g4', 18), 4294967296)
        self.assertEqual(int('4f5aff66', 19), 4294967296)
        self.assertEqual(int('3723ai4g', 20), 4294967296)
        self.assertEqual(int('281d55i4', 21), 4294967296)
        self.assertEqual(int('1fj8b184', 22), 4294967296)
        self.assertEqual(int('1606k7ic', 23), 4294967296)
        self.assertEqual(int('mb994ag', 24), 4294967296)
        self.assertEqual(int('hek2mgl', 25), 4294967296)
        self.assertEqual(int('dnchbnm', 26), 4294967296)
        self.assertEqual(int('b28jpdm', 27), 4294967296)
        self.assertEqual(int('8pfgih4', 28), 4294967296)
        self.assertEqual(int('76beigg', 29), 4294967296)
        self.assertEqual(int('5qmcpqg', 30), 4294967296)
        self.assertEqual(int('4q0jto4', 31), 4294967296)
        self.assertEqual(int('4000000', 32), 4294967296)
        self.assertEqual(int('3aokq94', 33), 4294967296)
        self.assertEqual(int('2qhxjli', 34), 4294967296)
        self.assertEqual(int('2br45qb', 35), 4294967296)
        self.assertEqual(int('1z141z4', 36), 4294967296)

        # SF bug 1334662: int(string, base) wrong answers
        # Checks for proper evaluation of 2**32 + 1
        self.assertEqual(int('100000000000000000000000000000001', 2), 4294967297)
        self.assertEqual(int('102002022201221111212', 3), 4294967297)
        self.assertEqual(int('10000000000000001', 4), 4294967297)
        self.assertEqual(int('32244002423142', 5), 4294967297)
        self.assertEqual(int('1550104015505', 6), 4294967297)
        self.assertEqual(int('211301422355', 7), 4294967297)
        self.assertEqual(int('40000000001', 8), 4294967297)
        self.assertEqual(int('12068657455', 9), 4294967297)
        self.assertEqual(int('4294967297', 10), 4294967297)
        self.assertEqual(int('1904440555', 11), 4294967297)
        self.assertEqual(int('9ba461595', 12), 4294967297)
        self.assertEqual(int('535a7988a', 13), 4294967297)
        self.assertEqual(int('2ca5b7465', 14), 4294967297)
        self.assertEqual(int('1a20dcd82', 15), 4294967297)
        self.assertEqual(int('100000001', 16), 4294967297)
        self.assertEqual(int('a7ffda92', 17), 4294967297)
        self.assertEqual(int('704he7g5', 18), 4294967297)
        self.assertEqual(int('4f5aff67', 19), 4294967297)
        self.assertEqual(int('3723ai4h', 20), 4294967297)
        self.assertEqual(int('281d55i5', 21), 4294967297)
        self.assertEqual(int('1fj8b185', 22), 4294967297)
        self.assertEqual(int('1606k7id', 23), 4294967297)
        self.assertEqual(int('mb994ah', 24), 4294967297)
        self.assertEqual(int('hek2mgm', 25), 4294967297)
        self.assertEqual(int('dnchbnn', 26), 4294967297)
        self.assertEqual(int('b28jpdn', 27), 4294967297)
        self.assertEqual(int('8pfgih5', 28), 4294967297)
        self.assertEqual(int('76beigh', 29), 4294967297)
        self.assertEqual(int('5qmcpqh', 30), 4294967297)
        self.assertEqual(int('4q0jto5', 31), 4294967297)
        self.assertEqual(int('4000001', 32), 4294967297)
        self.assertEqual(int('3aokq95', 33), 4294967297)
        self.assertEqual(int('2qhxjlj', 34), 4294967297)
        self.assertEqual(int('2br45qc', 35), 4294967297)
        self.assertEqual(int('1z141z5', 36), 4294967297)

    def test_intconversion(self):
        # Test __int__()
        class Foo0:
            def __int__(self):
                return 42

        class Foo1(object):
            def __int__(self):
                return 42

        class Foo2(int):
            def __int__(self):
                return 42

        class Foo3(int):
            def __int__(self):
                return self

        class Foo4(int):
            def __int__(self):
                return 42

        class Foo5(int):
            def __int__(self):
                return 42.

        self.assertEqual(int(Foo0()), 42)
        self.assertEqual(int(Foo1()), 42)
        self.assertEqual(int(Foo2()), 42)
        self.assertEqual(int(Foo3()), 0)
        self.assertEqual(int(Foo4()), 42)
        self.assertRaises(TypeError, int, Foo5())

    def test_iter(self):
        self.assertRaises(TypeError, iter)
        self.assertRaises(TypeError, iter, 42, 42)
        lists = [("1", "2"), ["1", "2"], "12"]
        for l in lists:
            i = iter(l)
            self.assertEqual(next(i), '1')
            self.assertEqual(next(i), '2')
            self.assertRaises(StopIteration, next, i)

    def test_isinstance(self):
        class C:
            pass
        class D(C):
            pass
        class E:
            pass
        c = C()
        d = D()
        e = E()
        self.assert_(isinstance(c, C))
        self.assert_(isinstance(d, C))
        self.assert_(not isinstance(e, C))
        self.assert_(not isinstance(c, D))
        self.assert_(not isinstance('foo', E))
        self.assertRaises(TypeError, isinstance, E, 'foo')
        self.assertRaises(TypeError, isinstance)

    def test_issubclass(self):
        class C:
            pass
        class D(C):
            pass
        class E:
            pass
        c = C()
        d = D()
        e = E()
        self.assert_(issubclass(D, C))
        self.assert_(issubclass(C, C))
        self.assert_(not issubclass(C, D))
        self.assertRaises(TypeError, issubclass, 'foo', E)
        self.assertRaises(TypeError, issubclass, E, 'foo')
        self.assertRaises(TypeError, issubclass)

    def test_len(self):
        self.assertEqual(len('123'), 3)
        self.assertEqual(len(()), 0)
        self.assertEqual(len((1, 2, 3, 4)), 4)
        self.assertEqual(len([1, 2, 3, 4]), 4)
        self.assertEqual(len({}), 0)
        self.assertEqual(len({'a':1, 'b': 2}), 2)
        class BadSeq:
            def __len__(self):
                raise ValueError
        self.assertRaises(ValueError, len, BadSeq())

    def test_list(self):
        self.assertEqual(list([]), [])
        l0_3 = [0, 1, 2, 3]
        l0_3_bis = list(l0_3)
        self.assertEqual(l0_3, l0_3_bis)
        self.assert_(l0_3 is not l0_3_bis)
        self.assertEqual(list(()), [])
        self.assertEqual(list((0, 1, 2, 3)), [0, 1, 2, 3])
        self.assertEqual(list(''), [])
        self.assertEqual(list('spam'), ['s', 'p', 'a', 'm'])

        if sys.maxint == 0x7fffffff:
            # This test can currently only work on 32-bit machines.
            # XXX If/when PySequence_Length() returns a ssize_t, it should be
            # XXX re-enabled.
            # Verify clearing of bug #556025.
            # This assumes that the max data size (sys.maxint) == max
            # address size this also assumes that the address size is at
            # least 4 bytes with 8 byte addresses, the bug is not well
            # tested
            #
            # Note: This test is expected to SEGV under Cygwin 1.3.12 or
            # earlier due to a newlib bug.  See the following mailing list
            # thread for the details:

            #     http://sources.redhat.com/ml/newlib/2002/msg00369.html
            self.assertRaises(MemoryError, list, range(sys.maxint // 2))

        # This code used to segfault in Py2.4a3
        x = []
        x.extend(-y for y in x)
        self.assertEqual(x, [])

    def test_long(self):
        self.assertEqual(int(314), 314)
        self.assertEqual(int(3.14), 3)
        self.assertEqual(int(314), 314)
        # Check that conversion from float truncates towards zero
        self.assertEqual(int(-3.14), -3)
        self.assertEqual(int(3.9), 3)
        self.assertEqual(int(-3.9), -3)
        self.assertEqual(int(3.5), 3)
        self.assertEqual(int(-3.5), -3)
        self.assertEqual(int("-3"), -3)
        # Different base:
        self.assertEqual(int("10",16), 16)
        # Check conversions from string (same test set as for int(), and then some)
        LL = [
                ('1' + '0'*20, 10**20),
                ('1' + '0'*100, 10**100)
        ]
        for s, v in LL:
            for sign in "", "+", "-":
                for prefix in "", " ", "\t", "  \t\t  ":
                    ss = prefix + sign + s
                    vv = v
                    if sign == "-" and v is not ValueError:
                        vv = -v
                    try:
                        self.assertEqual(int(ss), int(vv))
                    except v:
                        pass

        self.assertRaises(ValueError, int, '123\0')
        self.assertRaises(ValueError, int, '53', 40)
        self.assertRaises(TypeError, int, 1, 12)

        # SF patch #1638879: embedded NULs were not detected with
        # explicit base
        self.assertRaises(ValueError, int, '123\0', 10)
        self.assertRaises(ValueError, int, '123\x00 245', 20)

        self.assertEqual(int('100000000000000000000000000000000', 2),
                         4294967296)
        self.assertEqual(int('102002022201221111211', 3), 4294967296)
        self.assertEqual(int('10000000000000000', 4), 4294967296)
        self.assertEqual(int('32244002423141', 5), 4294967296)
        self.assertEqual(int('1550104015504', 6), 4294967296)
        self.assertEqual(int('211301422354', 7), 4294967296)
        self.assertEqual(int('40000000000', 8), 4294967296)
        self.assertEqual(int('12068657454', 9), 4294967296)
        self.assertEqual(int('4294967296', 10), 4294967296)
        self.assertEqual(int('1904440554', 11), 4294967296)
        self.assertEqual(int('9ba461594', 12), 4294967296)
        self.assertEqual(int('535a79889', 13), 4294967296)
        self.assertEqual(int('2ca5b7464', 14), 4294967296)
        self.assertEqual(int('1a20dcd81', 15), 4294967296)
        self.assertEqual(int('100000000', 16), 4294967296)
        self.assertEqual(int('a7ffda91', 17), 4294967296)
        self.assertEqual(int('704he7g4', 18), 4294967296)
        self.assertEqual(int('4f5aff66', 19), 4294967296)
        self.assertEqual(int('3723ai4g', 20), 4294967296)
        self.assertEqual(int('281d55i4', 21), 4294967296)
        self.assertEqual(int('1fj8b184', 22), 4294967296)
        self.assertEqual(int('1606k7ic', 23), 4294967296)
        self.assertEqual(int('mb994ag', 24), 4294967296)
        self.assertEqual(int('hek2mgl', 25), 4294967296)
        self.assertEqual(int('dnchbnm', 26), 4294967296)
        self.assertEqual(int('b28jpdm', 27), 4294967296)
        self.assertEqual(int('8pfgih4', 28), 4294967296)
        self.assertEqual(int('76beigg', 29), 4294967296)
        self.assertEqual(int('5qmcpqg', 30), 4294967296)
        self.assertEqual(int('4q0jto4', 31), 4294967296)
        self.assertEqual(int('4000000', 32), 4294967296)
        self.assertEqual(int('3aokq94', 33), 4294967296)
        self.assertEqual(int('2qhxjli', 34), 4294967296)
        self.assertEqual(int('2br45qb', 35), 4294967296)
        self.assertEqual(int('1z141z4', 36), 4294967296)

        self.assertEqual(int('100000000000000000000000000000001', 2),
                         4294967297)
        self.assertEqual(int('102002022201221111212', 3), 4294967297)
        self.assertEqual(int('10000000000000001', 4), 4294967297)
        self.assertEqual(int('32244002423142', 5), 4294967297)
        self.assertEqual(int('1550104015505', 6), 4294967297)
        self.assertEqual(int('211301422355', 7), 4294967297)
        self.assertEqual(int('40000000001', 8), 4294967297)
        self.assertEqual(int('12068657455', 9), 4294967297)
        self.assertEqual(int('4294967297', 10), 4294967297)
        self.assertEqual(int('1904440555', 11), 4294967297)
        self.assertEqual(int('9ba461595', 12), 4294967297)
        self.assertEqual(int('535a7988a', 13), 4294967297)
        self.assertEqual(int('2ca5b7465', 14), 4294967297)
        self.assertEqual(int('1a20dcd82', 15), 4294967297)
        self.assertEqual(int('100000001', 16), 4294967297)
        self.assertEqual(int('a7ffda92', 17), 4294967297)
        self.assertEqual(int('704he7g5', 18), 4294967297)
        self.assertEqual(int('4f5aff67', 19), 4294967297)
        self.assertEqual(int('3723ai4h', 20), 4294967297)
        self.assertEqual(int('281d55i5', 21), 4294967297)
        self.assertEqual(int('1fj8b185', 22), 4294967297)
        self.assertEqual(int('1606k7id', 23), 4294967297)
        self.assertEqual(int('mb994ah', 24), 4294967297)
        self.assertEqual(int('hek2mgm', 25), 4294967297)
        self.assertEqual(int('dnchbnn', 26), 4294967297)
        self.assertEqual(int('b28jpdn', 27), 4294967297)
        self.assertEqual(int('8pfgih5', 28), 4294967297)
        self.assertEqual(int('76beigh', 29), 4294967297)
        self.assertEqual(int('5qmcpqh', 30), 4294967297)
        self.assertEqual(int('4q0jto5', 31), 4294967297)
        self.assertEqual(int('4000001', 32), 4294967297)
        self.assertEqual(int('3aokq95', 33), 4294967297)
        self.assertEqual(int('2qhxjlj', 34), 4294967297)
        self.assertEqual(int('2br45qc', 35), 4294967297)
        self.assertEqual(int('1z141z5', 36), 4294967297)


    def test_longconversion(self):
        # Test __long__()
        class Foo0:
            def __long__(self):
                return 42

        class Foo1(object):
            def __long__(self):
                return 42

        class Foo2(int):
            def __long__(self):
                return 42

        class Foo3(int):
            def __long__(self):
                return self

        class Foo4(int):
            def __long__(self):
                return 42

        class Foo5(int):
            def __long__(self):
                return 42.

        self.assertEqual(int(Foo0()), 42)
        self.assertEqual(int(Foo1()), 42)
        # XXX invokes __int__ now
        # self.assertEqual(long(Foo2()), 42L)
        self.assertEqual(int(Foo3()), 0)
        # XXX likewise
        # self.assertEqual(long(Foo4()), 42)
        # self.assertRaises(TypeError, long, Foo5())

    def test_map(self):
        self.assertEqual(
            list(map(None, 'hello')),
            [('h',), ('e',), ('l',), ('l',), ('o',)]
        )
        self.assertEqual(
            list(map(None, 'abcd', 'efg')),
            [('a', 'e'), ('b', 'f'), ('c', 'g')]
        )
        self.assertEqual(
            list(map(None, range(3))),
            [(0,), (1,), (2,)]
        )
        self.assertEqual(
            list(map(lambda x: x*x, range(1,4))),
            [1, 4, 9]
        )
        try:
            from math import sqrt
        except ImportError:
            def sqrt(x):
                return pow(x, 0.5)
        self.assertEqual(
            list(map(lambda x: list(map(sqrt, x)), [[16, 4], [81, 9]])),
            [[4.0, 2.0], [9.0, 3.0]]
        )
        self.assertEqual(
            list(map(lambda x, y: x+y, [1,3,2], [9,1,4])),
            [10, 4, 6]
        )

        def plus(*v):
            accu = 0
            for i in v: accu = accu + i
            return accu
        self.assertEqual(
            list(map(plus, [1, 3, 7])),
            [1, 3, 7]
        )
        self.assertEqual(
            list(map(plus, [1, 3, 7], [4, 9, 2])),
            [1+4, 3+9, 7+2]
        )
        self.assertEqual(
            list(map(plus, [1, 3, 7], [4, 9, 2], [1, 1, 0])),
            [1+4+1, 3+9+1, 7+2+0]
        )
        self.assertEqual(
            list(map(None, Squares(10))),
            [(0,), (1,), (4,), (9,), (16,), (25,), (36,), (49,), (64,), (81,)]
        )
        self.assertEqual(
            list(map(int, Squares(10))),
            [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]
        )
        self.assertEqual(
            list(map(None, Squares(3), Squares(2))),
            [(0,0), (1,1)]
        )
        def Max(a, b):
            if a is None:
                return b
            if b is None:
                return a
            return max(a, b)
        self.assertEqual(
            list(map(Max, Squares(3), Squares(2))),
            [0, 1]
        )
        self.assertRaises(TypeError, map)
        self.assertRaises(TypeError, map, lambda x: x, 42)
        self.assertEqual(list(map(None, [42])), [(42,)])
        class BadSeq:
            def __iter__(self):
                raise ValueError
                yield None
        self.assertRaises(ValueError, list, map(lambda x: x, BadSeq()))
        def badfunc(x):
            raise RuntimeError
        self.assertRaises(RuntimeError, list, map(badfunc, range(5)))

    def test_max(self):
        self.assertEqual(max('123123'), '3')
        self.assertEqual(max(1, 2, 3), 3)
        self.assertEqual(max((1, 2, 3, 1, 2, 3)), 3)
        self.assertEqual(max([1, 2, 3, 1, 2, 3]), 3)

        self.assertEqual(max(1, 2, 3.0), 3.0)
        self.assertEqual(max(1, 2.0, 3), 3)
        self.assertEqual(max(1.0, 2, 3), 3)

        for stmt in (
            "max(key=int)",                 # no args
            "max(1, key=int)",              # single arg not iterable
            "max(1, 2, keystone=int)",      # wrong keyword
            "max(1, 2, key=int, abc=int)",  # two many keywords
            "max(1, 2, key=1)",             # keyfunc is not callable
            ):
            try:
                exec(stmt, globals())
            except TypeError:
                pass
            else:
                self.fail(stmt)

        self.assertEqual(max((1,), key=neg), 1)     # one elem iterable
        self.assertEqual(max((1,2), key=neg), 1)    # two elem iterable
        self.assertEqual(max(1, 2, key=neg), 1)     # two elems

        data = [random.randrange(200) for i in range(100)]
        keys = dict((elem, random.randrange(50)) for elem in data)
        f = keys.__getitem__
        self.assertEqual(max(data, key=f),
                         sorted(reversed(data), key=f)[-1])

    def test_min(self):
        self.assertEqual(min('123123'), '1')
        self.assertEqual(min(1, 2, 3), 1)
        self.assertEqual(min((1, 2, 3, 1, 2, 3)), 1)
        self.assertEqual(min([1, 2, 3, 1, 2, 3]), 1)

        self.assertEqual(min(1, 2, 3.0), 1)
        self.assertEqual(min(1, 2.0, 3), 1)
        self.assertEqual(min(1.0, 2, 3), 1.0)

        self.assertRaises(TypeError, min)
        self.assertRaises(TypeError, min, 42)
        self.assertRaises(ValueError, min, ())
        class BadSeq:
            def __getitem__(self, index):
                raise ValueError
        self.assertRaises(ValueError, min, BadSeq())
        class BadNumber:
            def __cmp__(self, other):
                raise ValueError
        self.assertRaises(TypeError, min, (42, BadNumber()))

        for stmt in (
            "min(key=int)",                 # no args
            "min(1, key=int)",              # single arg not iterable
            "min(1, 2, keystone=int)",      # wrong keyword
            "min(1, 2, key=int, abc=int)",  # two many keywords
            "min(1, 2, key=1)",             # keyfunc is not callable
            ):
            try:
                exec(stmt, globals())
            except TypeError:
                pass
            else:
                self.fail(stmt)

        self.assertEqual(min((1,), key=neg), 1)     # one elem iterable
        self.assertEqual(min((1,2), key=neg), 2)    # two elem iterable
        self.assertEqual(min(1, 2, key=neg), 2)     # two elems

        data = [random.randrange(200) for i in range(100)]
        keys = dict((elem, random.randrange(50)) for elem in data)
        f = keys.__getitem__
        self.assertEqual(min(data, key=f),
                         sorted(data, key=f)[0])

    def test_next(self):
        it = iter(range(2))
        self.assertEqual(next(it), 0)
        self.assertEqual(next(it), 1)
        self.assertRaises(StopIteration, next, it)
        self.assertRaises(StopIteration, next, it)
        self.assertEquals(next(it, 42), 42)

        class Iter(object):
            def __iter__(self):
                return self
            def __next__(self):
                raise StopIteration

        it = iter(Iter())
        self.assertEquals(next(it, 42), 42)
        self.assertRaises(StopIteration, next, it)

        def gen():
            yield 1
            return

        it = gen()
        self.assertEquals(next(it), 1)
        self.assertRaises(StopIteration, next, it)
        self.assertEquals(next(it, 42), 42)

    def test_oct(self):
        self.assertEqual(oct(100), '0o144')
        self.assertEqual(oct(100), '0o144')
        self.assertEqual(oct(-100), '-0o144')
        self.assertEqual(oct(-100), '-0o144')
        self.assertRaises(TypeError, oct, ())

    def write_testfile(self):
        # NB the first 4 lines are also used to test input, below
        fp = open(TESTFN, 'w')
        try:
            fp.write('1+1\n')
            fp.write('1+1\n')
            fp.write('The quick brown fox jumps over the lazy dog')
            fp.write('.\n')
            fp.write('Dear John\n')
            fp.write('XXX'*100)
            fp.write('YYY'*100)
        finally:
            fp.close()

    def test_open(self):
        self.write_testfile()
        fp = open(TESTFN, 'r')
        try:
            self.assertEqual(fp.readline(4), '1+1\n')
            self.assertEqual(fp.readline(4), '1+1\n')
            self.assertEqual(fp.readline(), 'The quick brown fox jumps over the lazy dog.\n')
            self.assertEqual(fp.readline(4), 'Dear')
            self.assertEqual(fp.readline(100), ' John\n')
            self.assertEqual(fp.read(300), 'XXX'*100)
            self.assertEqual(fp.read(1000), 'YYY'*100)
        finally:
            fp.close()
        unlink(TESTFN)

    def test_ord(self):
        self.assertEqual(ord(' '), 32)
        self.assertEqual(ord('A'), 65)
        self.assertEqual(ord('a'), 97)
        self.assertEqual(ord('\x80'), 128)
        self.assertEqual(ord('\xff'), 255)

        self.assertEqual(ord(b' '), 32)
        self.assertEqual(ord(b'A'), 65)
        self.assertEqual(ord(b'a'), 97)
        self.assertEqual(ord(b'\x80'), 128)
        self.assertEqual(ord(b'\xff'), 255)

        self.assertEqual(ord(chr(sys.maxunicode)), sys.maxunicode)
        self.assertRaises(TypeError, ord, 42)

        self.assertEqual(ord(chr(0x10FFFF)), 0x10FFFF)
        self.assertEqual(ord("\U0000FFFF"), 0x0000FFFF)
        self.assertEqual(ord("\U00010000"), 0x00010000)
        self.assertEqual(ord("\U00010001"), 0x00010001)
        self.assertEqual(ord("\U000FFFFE"), 0x000FFFFE)
        self.assertEqual(ord("\U000FFFFF"), 0x000FFFFF)
        self.assertEqual(ord("\U00100000"), 0x00100000)
        self.assertEqual(ord("\U00100001"), 0x00100001)
        self.assertEqual(ord("\U0010FFFE"), 0x0010FFFE)
        self.assertEqual(ord("\U0010FFFF"), 0x0010FFFF)

    def test_pow(self):
        self.assertEqual(pow(0,0), 1)
        self.assertEqual(pow(0,1), 0)
        self.assertEqual(pow(1,0), 1)
        self.assertEqual(pow(1,1), 1)

        self.assertEqual(pow(2,0), 1)
        self.assertEqual(pow(2,10), 1024)
        self.assertEqual(pow(2,20), 1024*1024)
        self.assertEqual(pow(2,30), 1024*1024*1024)

        self.assertEqual(pow(-2,0), 1)
        self.assertEqual(pow(-2,1), -2)
        self.assertEqual(pow(-2,2), 4)
        self.assertEqual(pow(-2,3), -8)

        self.assertEqual(pow(0,0), 1)
        self.assertEqual(pow(0,1), 0)
        self.assertEqual(pow(1,0), 1)
        self.assertEqual(pow(1,1), 1)

        self.assertEqual(pow(2,0), 1)
        self.assertEqual(pow(2,10), 1024)
        self.assertEqual(pow(2,20), 1024*1024)
        self.assertEqual(pow(2,30), 1024*1024*1024)

        self.assertEqual(pow(-2,0), 1)
        self.assertEqual(pow(-2,1), -2)
        self.assertEqual(pow(-2,2), 4)
        self.assertEqual(pow(-2,3), -8)

        self.assertAlmostEqual(pow(0.,0), 1.)
        self.assertAlmostEqual(pow(0.,1), 0.)
        self.assertAlmostEqual(pow(1.,0), 1.)
        self.assertAlmostEqual(pow(1.,1), 1.)

        self.assertAlmostEqual(pow(2.,0), 1.)
        self.assertAlmostEqual(pow(2.,10), 1024.)
        self.assertAlmostEqual(pow(2.,20), 1024.*1024.)
        self.assertAlmostEqual(pow(2.,30), 1024.*1024.*1024.)

        self.assertAlmostEqual(pow(-2.,0), 1.)
        self.assertAlmostEqual(pow(-2.,1), -2.)
        self.assertAlmostEqual(pow(-2.,2), 4.)
        self.assertAlmostEqual(pow(-2.,3), -8.)

        for x in 2, 2, 2.0:
            for y in 10, 10, 10.0:
                for z in 1000, 1000, 1000.0:
                    if isinstance(x, float) or \
                       isinstance(y, float) or \
                       isinstance(z, float):
                        self.assertRaises(TypeError, pow, x, y, z)
                    else:
                        self.assertAlmostEqual(pow(x, y, z), 24.0)

        self.assertRaises(TypeError, pow, -1, -2, 3)
        self.assertRaises(ValueError, pow, 1, 2, 0)
        self.assertRaises(TypeError, pow, -1, -2, 3)
        self.assertRaises(ValueError, pow, 1, 2, 0)
        self.assertRaises(ValueError, pow, -342.43, 0.234)

        self.assertRaises(TypeError, pow)

    def test_range(self):
        self.assertEqual(list(range(3)), [0, 1, 2])
        self.assertEqual(list(range(1, 5)), [1, 2, 3, 4])
        self.assertEqual(list(range(0)), [])
        self.assertEqual(list(range(-3)), [])
        self.assertEqual(list(range(1, 10, 3)), [1, 4, 7])
        #self.assertEqual(list(range(5, -5, -3)), [5, 2, -1, -4])

        """ XXX(nnorwitz):
        # Now test range() with longs
        self.assertEqual(list(range(-2**100)), [])
        self.assertEqual(list(range(0, -2**100)), [])
        self.assertEqual(list(range(0, 2**100, -1)), [])
        self.assertEqual(list(range(0, 2**100, -1)), [])

        a = int(10 * sys.maxint)
        b = int(100 * sys.maxint)
        c = int(50 * sys.maxint)

        self.assertEqual(list(range(a, a+2)), [a, a+1])
        self.assertEqual(list(range(a+2, a, -1)), [a+2, a+1])
        self.assertEqual(list(range(a+4, a, -2)), [a+4, a+2])

        seq = list(range(a, b, c))
        self.assert_(a in seq)
        self.assert_(b not in seq)
        self.assertEqual(len(seq), 2)

        seq = list(range(b, a, -c))
        self.assert_(b in seq)
        self.assert_(a not in seq)
        self.assertEqual(len(seq), 2)

        seq = list(range(-a, -b, -c))
        self.assert_(-a in seq)
        self.assert_(-b not in seq)
        self.assertEqual(len(seq), 2)

        self.assertRaises(TypeError, range)
        self.assertRaises(TypeError, range, 1, 2, 3, 4)
        self.assertRaises(ValueError, range, 1, 2, 0)
        self.assertRaises(ValueError, range, a, a + 1, int(0))

        class badzero(int):
            def __eq__(self, other):
                raise RuntimeError
            __ne__ = __lt__ = __gt__ = __le__ = __ge__ = __eq__

        # XXX This won't (but should!) raise RuntimeError if a is an int...
        self.assertRaises(RuntimeError, range, a, a + 1, badzero(1))
        """

        # Reject floats when it would require PyLongs to represent.
        # (smaller floats still accepted, but deprecated)
        self.assertRaises(TypeError, range, 1e100, 1e101, 1e101)

        self.assertRaises(TypeError, range, 0, "spam")
        self.assertRaises(TypeError, range, 0, 42, "spam")

        #NEAL self.assertRaises(OverflowError, range, -sys.maxint, sys.maxint)
        #NEAL self.assertRaises(OverflowError, range, 0, 2*sys.maxint)

        self.assertRaises(OverflowError, len, range(0, sys.maxint**10))

    def test_input(self):
        self.write_testfile()
        fp = open(TESTFN, 'r')
        savestdin = sys.stdin
        savestdout = sys.stdout # Eats the echo
        try:
            sys.stdin = fp
            sys.stdout = BitBucket()
            self.assertEqual(input(), "1+1")
            self.assertEqual(input('testing\n'), "1+1")
            self.assertEqual(input(), 'The quick brown fox jumps over the lazy dog.')
            self.assertEqual(input('testing\n'), 'Dear John')

            # SF 1535165: don't segfault on closed stdin
            # sys.stdout must be a regular file for triggering
            sys.stdout = savestdout
            sys.stdin.close()
            self.assertRaises(ValueError, input)

            sys.stdout = BitBucket()
            sys.stdin = io.StringIO("NULL\0")
            self.assertRaises(TypeError, input, 42, 42)
            sys.stdin = io.StringIO("    'whitespace'")
            self.assertEqual(input(), "    'whitespace'")
            sys.stdin = io.StringIO()
            self.assertRaises(EOFError, input)

            del sys.stdout
            self.assertRaises(RuntimeError, input, 'prompt')
            del sys.stdin
            self.assertRaises(RuntimeError, input, 'prompt')
        finally:
            sys.stdin = savestdin
            sys.stdout = savestdout
            fp.close()
            unlink(TESTFN)

    def test_repr(self):
        self.assertEqual(repr(''), '\'\'')
        self.assertEqual(repr(0), '0')
        self.assertEqual(repr(0), '0')
        self.assertEqual(repr(()), '()')
        self.assertEqual(repr([]), '[]')
        self.assertEqual(repr({}), '{}')
        a = []
        a.append(a)
        self.assertEqual(repr(a), '[[...]]')
        a = {}
        a[0] = a
        self.assertEqual(repr(a), '{0: {...}}')

    def test_round(self):
        self.assertEqual(round(0.0), 0.0)
        self.assertEqual(round(1.0), 1.0)
        self.assertEqual(round(10.0), 10.0)
        self.assertEqual(round(1000000000.0), 1000000000.0)
        self.assertEqual(round(1e20), 1e20)

        self.assertEqual(round(-1.0), -1.0)
        self.assertEqual(round(-10.0), -10.0)
        self.assertEqual(round(-1000000000.0), -1000000000.0)
        self.assertEqual(round(-1e20), -1e20)

        self.assertEqual(round(0.1), 0.0)
        self.assertEqual(round(1.1), 1.0)
        self.assertEqual(round(10.1), 10.0)
        self.assertEqual(round(1000000000.1), 1000000000.0)

        self.assertEqual(round(-1.1), -1.0)
        self.assertEqual(round(-10.1), -10.0)
        self.assertEqual(round(-1000000000.1), -1000000000.0)

        self.assertEqual(round(0.9), 1.0)
        self.assertEqual(round(9.9), 10.0)
        self.assertEqual(round(999999999.9), 1000000000.0)

        self.assertEqual(round(-0.9), -1.0)
        self.assertEqual(round(-9.9), -10.0)
        self.assertEqual(round(-999999999.9), -1000000000.0)

        self.assertEqual(round(-8.0, -1), -10.0)

        # test new kwargs
        self.assertEqual(round(number=-8.0, ndigits=-1), -10.0)

        self.assertRaises(TypeError, round)

    def test_setattr(self):
        setattr(sys, 'spam', 1)
        self.assertEqual(sys.spam, 1)
        self.assertRaises(TypeError, setattr, sys, 1, 'spam')
        self.assertRaises(TypeError, setattr)

    def test_str(self):
        self.assertEqual(str(''), '')
        self.assertEqual(str(0), '0')
        self.assertEqual(str(0), '0')
        self.assertEqual(str(()), '()')
        self.assertEqual(str([]), '[]')
        self.assertEqual(str({}), '{}')
        a = []
        a.append(a)
        self.assertEqual(str(a), '[[...]]')
        a = {}
        a[0] = a
        self.assertEqual(str(a), '{0: {...}}')

    def test_sum(self):
        self.assertEqual(sum([]), 0)
        self.assertEqual(sum(list(range(2,8))), 27)
        self.assertEqual(sum(iter(list(range(2,8)))), 27)
        self.assertEqual(sum(Squares(10)), 285)
        self.assertEqual(sum(iter(Squares(10))), 285)
        self.assertEqual(sum([[1], [2], [3]], []), [1, 2, 3])

        self.assertRaises(TypeError, sum)
        self.assertRaises(TypeError, sum, 42)
        self.assertRaises(TypeError, sum, ['a', 'b', 'c'])
        self.assertRaises(TypeError, sum, ['a', 'b', 'c'], '')
        self.assertRaises(TypeError, sum, [[1], [2], [3]])
        self.assertRaises(TypeError, sum, [{2:3}])
        self.assertRaises(TypeError, sum, [{2:3}]*2, {2:3})

        class BadSeq:
            def __getitem__(self, index):
                raise ValueError
        self.assertRaises(ValueError, sum, BadSeq())

    def test_tuple(self):
        self.assertEqual(tuple(()), ())
        t0_3 = (0, 1, 2, 3)
        t0_3_bis = tuple(t0_3)
        self.assert_(t0_3 is t0_3_bis)
        self.assertEqual(tuple([]), ())
        self.assertEqual(tuple([0, 1, 2, 3]), (0, 1, 2, 3))
        self.assertEqual(tuple(''), ())
        self.assertEqual(tuple('spam'), ('s', 'p', 'a', 'm'))

    def test_type(self):
        self.assertEqual(type(''),  type('123'))
        self.assertNotEqual(type(''), type(()))

    # We don't want self in vars(), so these are static methods

    @staticmethod
    def get_vars_f0():
        return vars()

    @staticmethod
    def get_vars_f2():
        BuiltinTest.get_vars_f0()
        a = 1
        b = 2
        return vars()

    def test_vars(self):
        self.assertEqual(set(vars()), set(dir()))
        import sys
        self.assertEqual(set(vars(sys)), set(dir(sys)))
        self.assertEqual(self.get_vars_f0(), {})
        self.assertEqual(self.get_vars_f2(), {'a': 1, 'b': 2})
        self.assertRaises(TypeError, vars, 42, 42)
        self.assertRaises(TypeError, vars, 42)

    def test_zip(self):
        a = (1, 2, 3)
        b = (4, 5, 6)
        t = [(1, 4), (2, 5), (3, 6)]
        self.assertEqual(list(zip(a, b)), t)
        b = [4, 5, 6]
        self.assertEqual(list(zip(a, b)), t)
        b = (4, 5, 6, 7)
        self.assertEqual(list(zip(a, b)), t)
        class I:
            def __getitem__(self, i):
                if i < 0 or i > 2: raise IndexError
                return i + 4
        self.assertEqual(list(zip(a, I())), t)
        self.assertEqual(list(zip()), [])
        self.assertEqual(list(zip(*[])), [])
        self.assertRaises(TypeError, zip, None)
        class G:
            pass
        self.assertRaises(TypeError, zip, a, G())

        # Make sure zip doesn't try to allocate a billion elements for the
        # result list when one of its arguments doesn't say how long it is.
        # A MemoryError is the most likely failure mode.
        class SequenceWithoutALength:
            def __getitem__(self, i):
                if i == 5:
                    raise IndexError
                else:
                    return i
        self.assertEqual(
            list(zip(SequenceWithoutALength(), range(2**30))),
            list(enumerate(range(5)))
        )

        class BadSeq:
            def __getitem__(self, i):
                if i == 5:
                    raise ValueError
                else:
                    return i
        self.assertRaises(ValueError, list, zip(BadSeq(), BadSeq()))

class TestSorted(unittest.TestCase):

    def test_basic(self):
        data = list(range(100))
        copy = data[:]
        random.shuffle(copy)
        self.assertEqual(data, sorted(copy))
        self.assertNotEqual(data, copy)

        data.reverse()
        random.shuffle(copy)
        self.assertEqual(data, sorted(copy, cmp=lambda x, y: (x < y) - (x > y)))
        self.assertNotEqual(data, copy)
        random.shuffle(copy)
        self.assertEqual(data, sorted(copy, key=lambda x: -x))
        self.assertNotEqual(data, copy)
        random.shuffle(copy)
        self.assertEqual(data, sorted(copy, reverse=1))
        self.assertNotEqual(data, copy)

    def test_inputtypes(self):
        s = 'abracadabra'
        types = [list, tuple, str]
        for T in types:
            self.assertEqual(sorted(s), sorted(T(s)))

        s = ''.join(set(s))  # unique letters only
        types = [str, set, frozenset, list, tuple, dict.fromkeys]
        for T in types:
            self.assertEqual(sorted(s), sorted(T(s)))

    def test_baddecorator(self):
        data = 'The quick Brown fox Jumped over The lazy Dog'.split()
        self.assertRaises(TypeError, sorted, data, None, lambda x,y: 0)

def test_main(verbose=None):
    test_classes = (BuiltinTest, TestSorted)

    run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in range(len(counts)):
            run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print(counts)


if __name__ == "__main__":
    test_main(verbose=True)
