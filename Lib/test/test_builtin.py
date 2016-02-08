# Python test set -- built-in functions

import platform
import unittest
from test.test_support import fcmp, have_unicode, TESTFN, unlink, \
                              run_unittest, check_py3k_warnings
import warnings
from operator import neg

import sys, cStringIO, random, UserDict

# count the number of test runs.
# used to skip running test_execfile() multiple times
# and to create unique strings to intern in test_intern()
numruns = 0

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


class TestFailingBool:
    def __nonzero__(self):
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
        self.assertEqual(abs(0L), 0L)
        self.assertEqual(abs(1234L), 1234L)
        self.assertEqual(abs(-1234L), 1234L)
        # str
        self.assertRaises(TypeError, abs, 'a')
        # bool
        self.assertEqual(abs(True), 1)
        self.assertEqual(abs(False), 0)
        # other
        self.assertRaises(TypeError, abs)
        self.assertRaises(TypeError, abs, None)
        class AbsClass(object):
            def __abs__(self):
                return -5
        self.assertEqual(abs(AbsClass()), -5)

    def test_all(self):
        self.assertEqual(all([2, 4, 6]), True)
        self.assertEqual(all([2, None, 6]), False)
        self.assertRaises(RuntimeError, all, [2, TestFailingBool(), 6])
        self.assertRaises(RuntimeError, all, TestFailingIter())
        self.assertRaises(TypeError, all, 10)               # Non-iterable
        self.assertRaises(TypeError, all)                   # No args
        self.assertRaises(TypeError, all, [2, 4, 6], [])    # Too many args
        self.assertEqual(all([]), True)                     # Empty iterator
        self.assertEqual(all([0, TestFailingBool()]), False)# Short-circuit
        S = [50, 60]
        self.assertEqual(all(x > 42 for x in S), True)
        S = [50, 40, 60]
        self.assertEqual(all(x > 42 for x in S), False)

    def test_any(self):
        self.assertEqual(any([None, None, None]), False)
        self.assertEqual(any([None, 4, None]), True)
        self.assertRaises(RuntimeError, any, [None, TestFailingBool(), 6])
        self.assertRaises(RuntimeError, any, TestFailingIter())
        self.assertRaises(TypeError, any, 10)               # Non-iterable
        self.assertRaises(TypeError, any)                   # No args
        self.assertRaises(TypeError, any, [2, 4, 6], [])    # Too many args
        self.assertEqual(any([]), False)                    # Empty iterator
        self.assertEqual(any([1, TestFailingBool()]), True) # Short-circuit
        S = [40, 60, 30]
        self.assertEqual(any(x > 42 for x in S), True)
        S = [10, 20, 30]
        self.assertEqual(any(x > 42 for x in S), False)

    def test_neg(self):
        x = -sys.maxint-1
        self.assertTrue(isinstance(x, int))
        self.assertEqual(-x, sys.maxint+1)

    def test_apply(self):
        def f0(*args):
            self.assertEqual(args, ())
        def f1(a1):
            self.assertEqual(a1, 1)
        def f2(a1, a2):
            self.assertEqual(a1, 1)
            self.assertEqual(a2, 2)
        def f3(a1, a2, a3):
            self.assertEqual(a1, 1)
            self.assertEqual(a2, 2)
            self.assertEqual(a3, 3)
        apply(f0, ())
        apply(f1, (1,))
        apply(f2, (1, 2))
        apply(f3, (1, 2, 3))

        # A PyCFunction that takes only positional parameters should allow an
        # empty keyword dictionary to pass without a complaint, but raise a
        # TypeError if the dictionary is non-empty.
        apply(id, (1,), {})
        self.assertRaises(TypeError, apply, id, (1,), {"foo": 1})
        self.assertRaises(TypeError, apply)
        self.assertRaises(TypeError, apply, id, 42)
        self.assertRaises(TypeError, apply, id, (42,), 42)

    def test_callable(self):
        self.assertTrue(callable(len))
        self.assertFalse(callable("a"))
        self.assertTrue(callable(callable))
        self.assertTrue(callable(lambda x, y: x + y))
        self.assertFalse(callable(__builtins__))
        def f(): pass
        self.assertTrue(callable(f))

        class Classic:
            def meth(self): pass
        self.assertTrue(callable(Classic))
        c = Classic()
        self.assertTrue(callable(c.meth))
        self.assertFalse(callable(c))

        class NewStyle(object):
            def meth(self): pass
        self.assertTrue(callable(NewStyle))
        n = NewStyle()
        self.assertTrue(callable(n.meth))
        self.assertFalse(callable(n))

        # Classic and new-style classes evaluate __call__() differently
        c.__call__ = None
        self.assertTrue(callable(c))
        del c.__call__
        self.assertFalse(callable(c))
        n.__call__ = None
        self.assertFalse(callable(n))
        del n.__call__
        self.assertFalse(callable(n))

        class N2(object):
            def __call__(self): pass
        n2 = N2()
        self.assertTrue(callable(n2))
        class N3(N2): pass
        n3 = N3()
        self.assertTrue(callable(n3))

    def test_chr(self):
        self.assertEqual(chr(32), ' ')
        self.assertEqual(chr(65), 'A')
        self.assertEqual(chr(97), 'a')
        self.assertEqual(chr(0xff), '\xff')
        self.assertRaises(ValueError, chr, 256)
        self.assertRaises(TypeError, chr)

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

    def test_coerce(self):
        self.assertTrue(not fcmp(coerce(1, 1.1), (1.0, 1.1)))
        self.assertEqual(coerce(1, 1L), (1L, 1L))
        self.assertTrue(not fcmp(coerce(1L, 1.1), (1.0, 1.1)))
        self.assertRaises(TypeError, coerce)
        class BadNumber:
            def __coerce__(self, other):
                raise ValueError
        self.assertRaises(ValueError, coerce, 42, BadNumber())
        self.assertRaises(OverflowError, coerce, 0.5, int("12345" * 1000))

    def test_compile(self):
        compile('print 1\n', '', 'exec')
        bom = '\xef\xbb\xbf'
        compile(bom + 'print 1\n', '', 'exec')
        compile(source='pass', filename='?', mode='exec')
        compile(dont_inherit=0, filename='tmp', source='0', mode='eval')
        compile('pass', '?', dont_inherit=1, mode='exec')
        self.assertRaises(TypeError, compile)
        self.assertRaises(ValueError, compile, 'print 42\n', '<string>', 'badmode')
        self.assertRaises(ValueError, compile, 'print 42\n', '<string>', 'single', 0xff)
        self.assertRaises(TypeError, compile, chr(0), 'f', 'exec')
        self.assertRaises(TypeError, compile, 'pass', '?', 'exec',
                          mode='eval', source='0', filename='tmp')
        if have_unicode:
            compile(unicode('print u"\xc3\xa5"\n', 'utf8'), '', 'exec')
            self.assertRaises(TypeError, compile, unichr(0), 'f', 'exec')
            self.assertRaises(ValueError, compile, unicode('a = 1'), 'f', 'bad')


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
        self.assertIn('local_var', dir())

        # dir(module)
        import sys
        self.assertIn('exit', dir(sys))

        # dir(module_with_invalid__dict__)
        import types
        class Foo(types.ModuleType):
            __dict__ = 8
        f = Foo("foo")
        self.assertRaises(TypeError, dir, f)

        # dir(type)
        self.assertIn("strip", dir(str))
        self.assertNotIn("__mro__", dir(str))

        # dir(obj)
        class Foo(object):
            def __init__(self):
                self.x = 7
                self.y = 8
                self.z = 9
        f = Foo()
        self.assertIn("y", dir(f))

        # dir(obj_no__dict__)
        class Foo(object):
            __slots__ = []
        f = Foo()
        self.assertIn("__repr__", dir(f))

        # dir(obj_no__class__with__dict__)
        # (an ugly trick to cause getattr(f, "__class__") to fail)
        class Foo(object):
            __slots__ = ["__class__", "__dict__"]
            def __init__(self):
                self.bar = "wow"
        f = Foo()
        self.assertNotIn("__repr__", dir(f))
        self.assertIn("bar", dir(f))

        # dir(obj_using __dir__)
        class Foo(object):
            def __dir__(self):
                return ["kan", "ga", "roo"]
        f = Foo()
        self.assertTrue(dir(f) == ["ga", "kan", "roo"])

        # dir(obj__dir__not_list)
        class Foo(object):
            def __dir__(self):
                return 7
        f = Foo()
        self.assertRaises(TypeError, dir, f)

    def test_divmod(self):
        self.assertEqual(divmod(12, 7), (1, 5))
        self.assertEqual(divmod(-12, 7), (-2, 2))
        self.assertEqual(divmod(12, -7), (-2, -2))
        self.assertEqual(divmod(-12, -7), (1, -5))

        self.assertEqual(divmod(12L, 7L), (1L, 5L))
        self.assertEqual(divmod(-12L, 7L), (-2L, 2L))
        self.assertEqual(divmod(12L, -7L), (-2L, -2L))
        self.assertEqual(divmod(-12L, -7L), (1L, -5L))

        self.assertEqual(divmod(12, 7L), (1, 5L))
        self.assertEqual(divmod(-12, 7L), (-2, 2L))
        self.assertEqual(divmod(12L, -7), (-2L, -2))
        self.assertEqual(divmod(-12L, -7), (1L, -5))

        self.assertEqual(divmod(-sys.maxint-1, -1),
                         (sys.maxint+1, 0))

        self.assertTrue(not fcmp(divmod(3.25, 1.0), (3.0, 0.25)))
        self.assertTrue(not fcmp(divmod(-3.25, 1.0), (-4.0, 0.75)))
        self.assertTrue(not fcmp(divmod(3.25, -1.0), (-4.0, -0.75)))
        self.assertTrue(not fcmp(divmod(-3.25, -1.0), (3.0, -0.25)))

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
        if have_unicode:
            self.assertEqual(eval(unicode('1+1')), 2)
            self.assertEqual(eval(unicode(' 1+1\n')), 2)
        globals = {'a': 1, 'b': 2}
        locals = {'b': 200, 'c': 300}
        if have_unicode:
            self.assertEqual(eval(unicode('a'), globals), 1)
            self.assertEqual(eval(unicode('a'), globals, locals), 1)
            self.assertEqual(eval(unicode('b'), globals, locals), 200)
            self.assertEqual(eval(unicode('c'), globals, locals), 300)
            bom = '\xef\xbb\xbf'
            self.assertEqual(eval(bom + 'a', globals, locals), 1)
            self.assertEqual(eval(unicode('u"\xc3\xa5"', 'utf8'), globals),
                             unicode('\xc3\xa5', 'utf8'))
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
                return 'a'
        self.assertRaises(TypeError, eval, 'dir()', globals(), C())

    def test_filter(self):
        self.assertEqual(filter(lambda c: 'a' <= c <= 'z', 'Hello World'), 'elloorld')
        self.assertEqual(filter(None, [1, 'hello', [], [3], '', None, 9, 0]), [1, 'hello', [3], 9])
        self.assertEqual(filter(lambda x: x > 0, [1, -3, 9, 0, 2]), [1, 9, 2])
        self.assertEqual(filter(None, Squares(10)), [1, 4, 9, 16, 25, 36, 49, 64, 81])
        self.assertEqual(filter(lambda x: x%2, Squares(10)), [1, 9, 25, 49, 81])
        def identity(item):
            return 1
        filter(identity, Squares(5))
        self.assertRaises(TypeError, filter)
        class BadSeq(object):
            def __getitem__(self, index):
                if index<4:
                    return 42
                raise ValueError
        self.assertRaises(ValueError, filter, lambda x: x, BadSeq())
        def badfunc():
            pass
        self.assertRaises(TypeError, filter, badfunc, range(5))

        # test bltinmodule.c::filtertuple()
        self.assertEqual(filter(None, (1, 2)), (1, 2))
        self.assertEqual(filter(lambda x: x>=3, (1, 2, 3, 4)), (3, 4))
        self.assertRaises(TypeError, filter, 42, (1, 2))

        # test bltinmodule.c::filterstring()
        self.assertEqual(filter(None, "12"), "12")
        self.assertEqual(filter(lambda x: x>="3", "1234"), "34")
        self.assertRaises(TypeError, filter, 42, "12")
        class badstr(str):
            def __getitem__(self, index):
                raise ValueError
        self.assertRaises(ValueError, filter, lambda x: x >="3", badstr("1234"))

        class badstr2(str):
            def __getitem__(self, index):
                return 42
        self.assertRaises(TypeError, filter, lambda x: x >=42, badstr2("1234"))

        class weirdstr(str):
            def __getitem__(self, index):
                return weirdstr(2*str.__getitem__(self, index))
        self.assertEqual(filter(lambda x: x>="33", weirdstr("1234")), "3344")

        class shiftstr(str):
            def __getitem__(self, index):
                return chr(ord(str.__getitem__(self, index))+1)
        self.assertEqual(filter(lambda x: x>="3", shiftstr("1234")), "345")

        if have_unicode:
            # test bltinmodule.c::filterunicode()
            self.assertEqual(filter(None, unicode("12")), unicode("12"))
            self.assertEqual(filter(lambda x: x>="3", unicode("1234")), unicode("34"))
            self.assertRaises(TypeError, filter, 42, unicode("12"))
            self.assertRaises(ValueError, filter, lambda x: x >="3", badstr(unicode("1234")))

            class badunicode(unicode):
                def __getitem__(self, index):
                    return 42
            self.assertRaises(TypeError, filter, lambda x: x >=42, badunicode("1234"))

            class weirdunicode(unicode):
                def __getitem__(self, index):
                    return weirdunicode(2*unicode.__getitem__(self, index))
            self.assertEqual(
                filter(lambda x: x>=unicode("33"), weirdunicode("1234")), unicode("3344"))

            class shiftunicode(unicode):
                def __getitem__(self, index):
                    return unichr(ord(unicode.__getitem__(self, index))+1)
            self.assertEqual(
                filter(lambda x: x>=unicode("3"), shiftunicode("1234")),
                unicode("345")
            )

    def test_filter_subclasses(self):
        # test that filter() never returns tuple, str or unicode subclasses
        # and that the result always goes through __getitem__
        funcs = (None, bool, lambda x: True)
        class tuple2(tuple):
            def __getitem__(self, index):
                return 2*tuple.__getitem__(self, index)
        class str2(str):
            def __getitem__(self, index):
                return 2*str.__getitem__(self, index)
        inputs = {
            tuple2: {(): (), (1, 2, 3): (2, 4, 6)},
            str2:   {"": "", "123": "112233"}
        }
        if have_unicode:
            class unicode2(unicode):
                def __getitem__(self, index):
                    return 2*unicode.__getitem__(self, index)
            inputs[unicode2] = {
                unicode(): unicode(),
                unicode("123"): unicode("112233")
            }

        for (cls, inps) in inputs.iteritems():
            for (inp, exp) in inps.iteritems():
                # make sure the output goes through __getitem__
                # even if func is None
                self.assertEqual(
                    filter(funcs[0], cls(inp)),
                    filter(funcs[1], cls(inp))
                )
                for func in funcs:
                    outp = filter(func, cls(inp))
                    self.assertEqual(outp, exp)
                    self.assertTrue(not isinstance(outp, cls))

    def test_getattr(self):
        import sys
        self.assertTrue(getattr(sys, 'stdout') is sys.stdout)
        self.assertRaises(TypeError, getattr, sys, 1)
        self.assertRaises(TypeError, getattr, sys, 1, "foo")
        self.assertRaises(TypeError, getattr)
        if have_unicode:
            self.assertRaises(UnicodeError, getattr, sys, unichr(sys.maxunicode))

    def test_hasattr(self):
        import sys
        self.assertTrue(hasattr(sys, 'stdout'))
        self.assertRaises(TypeError, hasattr, sys, 1)
        self.assertRaises(TypeError, hasattr)
        if have_unicode:
            self.assertRaises(UnicodeError, hasattr, sys, unichr(sys.maxunicode))

        # Check that hasattr allows SystemExit and KeyboardInterrupts by
        class A:
            def __getattr__(self, what):
                raise KeyboardInterrupt
        self.assertRaises(KeyboardInterrupt, hasattr, A(), "b")
        class B:
            def __getattr__(self, what):
                raise SystemExit
        self.assertRaises(SystemExit, hasattr, B(), "b")

    def test_hash(self):
        hash(None)
        self.assertEqual(hash(1), hash(1L))
        self.assertEqual(hash(1), hash(1.0))
        hash('spam')
        if have_unicode:
            self.assertEqual(hash('spam'), hash(unicode('spam')))
        hash((0,1,2,3))
        def f(): pass
        self.assertRaises(TypeError, hash, [])
        self.assertRaises(TypeError, hash, {})
        # Bug 1536021: Allow hash to return long objects
        class X:
            def __hash__(self):
                return 2**100
        self.assertEqual(type(hash(X())), int)
        class Y(object):
            def __hash__(self):
                return 2**100
        self.assertEqual(type(hash(Y())), int)
        class Z(long):
            def __hash__(self):
                return self
        self.assertEqual(hash(Z(42)), hash(42L))

    def test_hex(self):
        self.assertEqual(hex(16), '0x10')
        self.assertEqual(hex(16L), '0x10L')
        self.assertEqual(hex(-16), '-0x10')
        self.assertEqual(hex(-16L), '-0x10L')
        self.assertRaises(TypeError, hex, {})

    def test_id(self):
        id(None)
        id(1)
        id(1L)
        id(1.0)
        id('spam')
        id((0,1,2,3))
        id([0,1,2,3])
        id({'spam': 1, 'eggs': 2, 'ham': 3})

    # Test input() later, together with raw_input

    # test_int(): see test_int.py for int() tests.

    def test_intern(self):
        self.assertRaises(TypeError, intern)
        # This fails if the test is run twice with a constant string,
        # therefore append the run counter
        s = "never interned before " + str(numruns)
        self.assertTrue(intern(s) is s)
        s2 = s.swapcase().swapcase()
        self.assertTrue(intern(s2) is s)

        # Subclasses of string can't be interned, because they
        # provide too much opportunity for insane things to happen.
        # We don't want them in the interned dict and if they aren't
        # actually interned, we don't want to create the appearance
        # that they are by allowing intern() to succeed.
        class S(str):
            def __hash__(self):
                return 123

        self.assertRaises(TypeError, intern, S("abc"))

        # It's still safe to pass these strings to routines that
        # call intern internally, e.g. PyObject_SetAttr().
        s = S("abc")
        setattr(s, s, s)
        self.assertEqual(getattr(s, s), s)

    def test_iter(self):
        self.assertRaises(TypeError, iter)
        self.assertRaises(TypeError, iter, 42, 42)
        lists = [("1", "2"), ["1", "2"], "12"]
        if have_unicode:
            lists.append(unicode("12"))
        for l in lists:
            i = iter(l)
            self.assertEqual(i.next(), '1')
            self.assertEqual(i.next(), '2')
            self.assertRaises(StopIteration, i.next)

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
        self.assertTrue(isinstance(c, C))
        self.assertTrue(isinstance(d, C))
        self.assertTrue(not isinstance(e, C))
        self.assertTrue(not isinstance(c, D))
        self.assertTrue(not isinstance('foo', E))
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
        self.assertTrue(issubclass(D, C))
        self.assertTrue(issubclass(C, C))
        self.assertTrue(not issubclass(C, D))
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
        self.assertRaises(TypeError, len, 2)
        class ClassicStyle: pass
        class NewStyle(object): pass
        self.assertRaises(AttributeError, len, ClassicStyle())
        self.assertRaises(TypeError, len, NewStyle())

    def test_map(self):
        self.assertEqual(
            map(None, 'hello world'),
            ['h','e','l','l','o',' ','w','o','r','l','d']
        )
        self.assertEqual(
            map(None, 'abcd', 'efg'),
            [('a', 'e'), ('b', 'f'), ('c', 'g'), ('d', None)]
        )
        self.assertEqual(
            map(None, range(10)),
            [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
        )
        self.assertEqual(
            map(lambda x: x*x, range(1,4)),
            [1, 4, 9]
        )
        try:
            from math import sqrt
        except ImportError:
            def sqrt(x):
                return pow(x, 0.5)
        self.assertEqual(
            map(lambda x: map(sqrt,x), [[16, 4], [81, 9]]),
            [[4.0, 2.0], [9.0, 3.0]]
        )
        self.assertEqual(
            map(lambda x, y: x+y, [1,3,2], [9,1,4]),
            [10, 4, 6]
        )

        def plus(*v):
            accu = 0
            for i in v: accu = accu + i
            return accu
        self.assertEqual(
            map(plus, [1, 3, 7]),
            [1, 3, 7]
        )
        self.assertEqual(
            map(plus, [1, 3, 7], [4, 9, 2]),
            [1+4, 3+9, 7+2]
        )
        self.assertEqual(
            map(plus, [1, 3, 7], [4, 9, 2], [1, 1, 0]),
            [1+4+1, 3+9+1, 7+2+0]
        )
        self.assertEqual(
            map(None, Squares(10)),
            [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]
        )
        self.assertEqual(
            map(int, Squares(10)),
            [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]
        )
        self.assertEqual(
            map(None, Squares(3), Squares(2)),
            [(0,0), (1,1), (4,None)]
        )
        self.assertEqual(
            map(max, Squares(3), Squares(2)),
            [0, 1, 4]
        )
        self.assertRaises(TypeError, map)
        self.assertRaises(TypeError, map, lambda x: x, 42)
        self.assertEqual(map(None, [42]), [42])
        class BadSeq:
            def __getitem__(self, index):
                raise ValueError
        self.assertRaises(ValueError, map, lambda x: x, BadSeq())
        def badfunc(x):
            raise RuntimeError
        self.assertRaises(RuntimeError, map, badfunc, range(5))

    def test_max(self):
        self.assertEqual(max('123123'), '3')
        self.assertEqual(max(1, 2, 3), 3)
        self.assertEqual(max((1, 2, 3, 1, 2, 3)), 3)
        self.assertEqual(max([1, 2, 3, 1, 2, 3]), 3)

        self.assertEqual(max(1, 2L, 3.0), 3.0)
        self.assertEqual(max(1L, 2.0, 3), 3)
        self.assertEqual(max(1.0, 2, 3L), 3L)

        for stmt in (
            "max(key=int)",                 # no args
            "max(1, key=int)",              # single arg not iterable
            "max(1, 2, keystone=int)",      # wrong keyword
            "max(1, 2, key=int, abc=int)",  # two many keywords
            "max(1, 2, key=1)",             # keyfunc is not callable
            ):
            try:
                exec(stmt) in globals()
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

        self.assertEqual(min(1, 2L, 3.0), 1)
        self.assertEqual(min(1L, 2.0, 3), 1L)
        self.assertEqual(min(1.0, 2, 3L), 1.0)

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
        self.assertRaises(ValueError, min, (42, BadNumber()))

        for stmt in (
            "min(key=int)",                 # no args
            "min(1, key=int)",              # single arg not iterable
            "min(1, 2, keystone=int)",      # wrong keyword
            "min(1, 2, key=int, abc=int)",  # two many keywords
            "min(1, 2, key=1)",             # keyfunc is not callable
            ):
            try:
                exec(stmt) in globals()
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
        self.assertEqual(next(it, 42), 42)

        class Iter(object):
            def __iter__(self):
                return self
            def next(self):
                raise StopIteration

        it = iter(Iter())
        self.assertEqual(next(it, 42), 42)
        self.assertRaises(StopIteration, next, it)

        def gen():
            yield 1
            return

        it = gen()
        self.assertEqual(next(it), 1)
        self.assertRaises(StopIteration, next, it)
        self.assertEqual(next(it, 42), 42)

    def test_oct(self):
        self.assertEqual(oct(100), '0144')
        self.assertEqual(oct(100L), '0144L')
        self.assertEqual(oct(-100), '-0144')
        self.assertEqual(oct(-100L), '-0144L')
        self.assertRaises(TypeError, oct, ())

    def write_testfile(self):
        # NB the first 4 lines are also used to test input and raw_input, below
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
        if have_unicode:
            self.assertEqual(ord(unichr(sys.maxunicode)), sys.maxunicode)
        self.assertRaises(TypeError, ord, 42)
        if have_unicode:
            self.assertRaises(TypeError, ord, unicode("12"))

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

        self.assertEqual(pow(0L,0), 1)
        self.assertEqual(pow(0L,1), 0)
        self.assertEqual(pow(1L,0), 1)
        self.assertEqual(pow(1L,1), 1)

        self.assertEqual(pow(2L,0), 1)
        self.assertEqual(pow(2L,10), 1024)
        self.assertEqual(pow(2L,20), 1024*1024)
        self.assertEqual(pow(2L,30), 1024*1024*1024)

        self.assertEqual(pow(-2L,0), 1)
        self.assertEqual(pow(-2L,1), -2)
        self.assertEqual(pow(-2L,2), 4)
        self.assertEqual(pow(-2L,3), -8)

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

        for x in 2, 2L, 2.0:
            for y in 10, 10L, 10.0:
                for z in 1000, 1000L, 1000.0:
                    if isinstance(x, float) or \
                       isinstance(y, float) or \
                       isinstance(z, float):
                        self.assertRaises(TypeError, pow, x, y, z)
                    else:
                        self.assertAlmostEqual(pow(x, y, z), 24.0)

        self.assertRaises(TypeError, pow, -1, -2, 3)
        self.assertRaises(ValueError, pow, 1, 2, 0)
        self.assertRaises(TypeError, pow, -1L, -2L, 3L)
        self.assertRaises(ValueError, pow, 1L, 2L, 0L)
        # Will return complex in 3.0:
        self.assertRaises(ValueError, pow, -342.43, 0.234)

        self.assertRaises(TypeError, pow)

    def test_range(self):
        self.assertEqual(range(3), [0, 1, 2])
        self.assertEqual(range(1, 5), [1, 2, 3, 4])
        self.assertEqual(range(0), [])
        self.assertEqual(range(-3), [])
        self.assertEqual(range(1, 10, 3), [1, 4, 7])
        self.assertEqual(range(5, -5, -3), [5, 2, -1, -4])

        # Now test range() with longs
        self.assertEqual(range(-2**100), [])
        self.assertEqual(range(0, -2**100), [])
        self.assertEqual(range(0, 2**100, -1), [])
        self.assertEqual(range(0, 2**100, -1), [])

        a = long(10 * sys.maxint)
        b = long(100 * sys.maxint)
        c = long(50 * sys.maxint)

        self.assertEqual(range(a, a+2), [a, a+1])
        self.assertEqual(range(a+2, a, -1L), [a+2, a+1])
        self.assertEqual(range(a+4, a, -2), [a+4, a+2])

        seq = range(a, b, c)
        self.assertIn(a, seq)
        self.assertNotIn(b, seq)
        self.assertEqual(len(seq), 2)

        seq = range(b, a, -c)
        self.assertIn(b, seq)
        self.assertNotIn(a, seq)
        self.assertEqual(len(seq), 2)

        seq = range(-a, -b, -c)
        self.assertIn(-a, seq)
        self.assertNotIn(-b, seq)
        self.assertEqual(len(seq), 2)

        self.assertRaises(TypeError, range)
        self.assertRaises(TypeError, range, 1, 2, 3, 4)
        self.assertRaises(ValueError, range, 1, 2, 0)
        self.assertRaises(ValueError, range, a, a + 1, long(0))

        class badzero(int):
            def __cmp__(self, other):
                raise RuntimeError
            __hash__ = None # Invalid cmp makes this unhashable
        self.assertRaises(RuntimeError, range, a, a + 1, badzero(1))

        # Reject floats.
        self.assertRaises(TypeError, range, 1., 1., 1.)
        self.assertRaises(TypeError, range, 1e100, 1e101, 1e101)

        self.assertRaises(TypeError, range, 0, "spam")
        self.assertRaises(TypeError, range, 0, 42, "spam")

        self.assertRaises(OverflowError, range, -sys.maxint, sys.maxint)
        self.assertRaises(OverflowError, range, 0, 2*sys.maxint)

        bignum = 2*sys.maxint
        smallnum = 42
        # Old-style user-defined class with __int__ method
        class I0:
            def __init__(self, n):
                self.n = int(n)
            def __int__(self):
                return self.n
        self.assertEqual(range(I0(bignum), I0(bignum + 1)), [bignum])
        self.assertEqual(range(I0(smallnum), I0(smallnum + 1)), [smallnum])

        # New-style user-defined class with __int__ method
        class I1(object):
            def __init__(self, n):
                self.n = int(n)
            def __int__(self):
                return self.n
        self.assertEqual(range(I1(bignum), I1(bignum + 1)), [bignum])
        self.assertEqual(range(I1(smallnum), I1(smallnum + 1)), [smallnum])

        # New-style user-defined class with failing __int__ method
        class IX(object):
            def __int__(self):
                raise RuntimeError
        self.assertRaises(RuntimeError, range, IX())

        # New-style user-defined class with invalid __int__ method
        class IN(object):
            def __int__(self):
                return "not a number"
        self.assertRaises(TypeError, range, IN())

        # Exercise various combinations of bad arguments, to check
        # refcounting logic
        self.assertRaises(TypeError, range, 0.0)

        self.assertRaises(TypeError, range, 0, 0.0)
        self.assertRaises(TypeError, range, 0.0, 0)
        self.assertRaises(TypeError, range, 0.0, 0.0)

        self.assertRaises(TypeError, range, 0, 0, 1.0)
        self.assertRaises(TypeError, range, 0, 0.0, 1)
        self.assertRaises(TypeError, range, 0, 0.0, 1.0)
        self.assertRaises(TypeError, range, 0.0, 0, 1)
        self.assertRaises(TypeError, range, 0.0, 0, 1.0)
        self.assertRaises(TypeError, range, 0.0, 0.0, 1)
        self.assertRaises(TypeError, range, 0.0, 0.0, 1.0)



    def test_input_and_raw_input(self):
        self.write_testfile()
        fp = open(TESTFN, 'r')
        savestdin = sys.stdin
        savestdout = sys.stdout # Eats the echo
        try:
            sys.stdin = fp
            sys.stdout = BitBucket()
            self.assertEqual(input(), 2)
            self.assertEqual(input('testing\n'), 2)
            self.assertEqual(raw_input(), 'The quick brown fox jumps over the lazy dog.')
            self.assertEqual(raw_input('testing\n'), 'Dear John')

            # SF 1535165: don't segfault on closed stdin
            # sys.stdout must be a regular file for triggering
            sys.stdout = savestdout
            sys.stdin.close()
            self.assertRaises(ValueError, input)

            sys.stdout = BitBucket()
            sys.stdin = cStringIO.StringIO("NULL\0")
            self.assertRaises(TypeError, input, 42, 42)
            sys.stdin = cStringIO.StringIO("    'whitespace'")
            self.assertEqual(input(), 'whitespace')
            sys.stdin = cStringIO.StringIO()
            self.assertRaises(EOFError, input)

            # SF 876178: make sure input() respect future options.
            sys.stdin = cStringIO.StringIO('1/2')
            sys.stdout = cStringIO.StringIO()
            exec compile('print input()', 'test_builtin_tmp', 'exec')
            sys.stdin.seek(0, 0)
            exec compile('from __future__ import division;print input()',
                         'test_builtin_tmp', 'exec')
            sys.stdin.seek(0, 0)
            exec compile('print input()', 'test_builtin_tmp', 'exec')
            # The result we expect depends on whether new division semantics
            # are already in effect.
            if 1/2 == 0:
                # This test was compiled with old semantics.
                expected = ['0', '0.5', '0']
            else:
                # This test was compiled with new semantics (e.g., -Qnew
                # was given on the command line.
                expected = ['0.5', '0.5', '0.5']
            self.assertEqual(sys.stdout.getvalue().splitlines(), expected)

            del sys.stdout
            self.assertRaises(RuntimeError, input, 'prompt')
            del sys.stdin
            self.assertRaises(RuntimeError, input, 'prompt')
        finally:
            sys.stdin = savestdin
            sys.stdout = savestdout
            fp.close()
            unlink(TESTFN)

    def test_reduce(self):
        add = lambda x, y: x+y
        self.assertEqual(reduce(add, ['a', 'b', 'c'], ''), 'abc')
        self.assertEqual(
            reduce(add, [['a', 'c'], [], ['d', 'w']], []),
            ['a','c','d','w']
        )
        self.assertEqual(reduce(lambda x, y: x*y, range(2,8), 1), 5040)
        self.assertEqual(
            reduce(lambda x, y: x*y, range(2,21), 1L),
            2432902008176640000L
        )
        self.assertEqual(reduce(add, Squares(10)), 285)
        self.assertEqual(reduce(add, Squares(10), 0), 285)
        self.assertEqual(reduce(add, Squares(0), 0), 0)
        self.assertRaises(TypeError, reduce)
        self.assertRaises(TypeError, reduce, 42)
        self.assertRaises(TypeError, reduce, 42, 42)
        self.assertRaises(TypeError, reduce, 42, 42, 42)
        self.assertRaises(TypeError, reduce, None, range(5))
        self.assertRaises(TypeError, reduce, add, 42)
        self.assertEqual(reduce(42, "1"), "1") # func is never called with one item
        self.assertEqual(reduce(42, "", "1"), "1") # func is never called with one item
        self.assertRaises(TypeError, reduce, 42, (42, 42))
        self.assertRaises(TypeError, reduce, add, []) # arg 2 must not be empty sequence with no initial value
        self.assertRaises(TypeError, reduce, add, "")
        self.assertRaises(TypeError, reduce, add, ())
        self.assertEqual(reduce(add, [], None), None)
        self.assertEqual(reduce(add, [], 42), 42)

        class BadSeq:
            def __getitem__(self, index):
                raise ValueError
        self.assertRaises(ValueError, reduce, 42, BadSeq())

    def test_reload(self):
        import marshal
        reload(marshal)
        import string
        reload(string)
        ## import sys
        ## self.assertRaises(ImportError, reload, sys)

    def test_repr(self):
        self.assertEqual(repr(''), '\'\'')
        self.assertEqual(repr(0), '0')
        self.assertEqual(repr(0L), '0L')
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
        self.assertEqual(type(round(0.0)), float)  # Will be int in 3.0.
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
        self.assertEqual(type(round(-8.0, -1)), float)

        self.assertEqual(type(round(-8.0, 0)), float)
        self.assertEqual(type(round(-8.0, 1)), float)

        # Check half rounding behaviour.
        self.assertEqual(round(5.5), 6)
        self.assertEqual(round(6.5), 7)
        self.assertEqual(round(-5.5), -6)
        self.assertEqual(round(-6.5), -7)

        # Check behavior on ints
        self.assertEqual(round(0), 0)
        self.assertEqual(round(8), 8)
        self.assertEqual(round(-8), -8)
        self.assertEqual(type(round(0)), float)  # Will be int in 3.0.
        self.assertEqual(type(round(-8, -1)), float)
        self.assertEqual(type(round(-8, 0)), float)
        self.assertEqual(type(round(-8, 1)), float)

        # test new kwargs
        self.assertEqual(round(number=-8.0, ndigits=-1), -10.0)

        self.assertRaises(TypeError, round)

        # test generic rounding delegation for reals
        class TestRound(object):
            def __float__(self):
                return 23.0

        class TestNoRound(object):
            pass

        self.assertEqual(round(TestRound()), 23)

        self.assertRaises(TypeError, round, 1, 2, 3)
        self.assertRaises(TypeError, round, TestNoRound())

        t = TestNoRound()
        t.__float__ = lambda *args: args
        self.assertRaises(TypeError, round, t)
        self.assertRaises(TypeError, round, t, 0)

    # Some versions of glibc for alpha have a bug that affects
    # float -> integer rounding (floor, ceil, rint, round) for
    # values in the range [2**52, 2**53).  See:
    #
    #   http://sources.redhat.com/bugzilla/show_bug.cgi?id=5350
    #
    # We skip this test on Linux/alpha if it would fail.
    linux_alpha = (platform.system().startswith('Linux') and
                   platform.machine().startswith('alpha'))
    system_round_bug = round(5e15+1) != 5e15+1
    @unittest.skipIf(linux_alpha and system_round_bug,
                     "test will fail;  failure is probably due to a "
                     "buggy system round function")
    def test_round_large(self):
        # Issue #1869: integral floats should remain unchanged
        self.assertEqual(round(5e15-1), 5e15-1)
        self.assertEqual(round(5e15), 5e15)
        self.assertEqual(round(5e15+1), 5e15+1)
        self.assertEqual(round(5e15+2), 5e15+2)
        self.assertEqual(round(5e15+3), 5e15+3)

    def test_setattr(self):
        setattr(sys, 'spam', 1)
        self.assertEqual(sys.spam, 1)
        self.assertRaises(TypeError, setattr, sys, 1, 'spam')
        self.assertRaises(TypeError, setattr)

    def test_sum(self):
        self.assertEqual(sum([]), 0)
        self.assertEqual(sum(range(2,8)), 27)
        self.assertEqual(sum(iter(range(2,8))), 27)
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

        empty = []
        sum(([x] for x in range(10)), empty)
        self.assertEqual(empty, [])

    def test_type(self):
        self.assertEqual(type(''),  type('123'))
        self.assertNotEqual(type(''), type(()))

    def test_unichr(self):
        if have_unicode:
            self.assertEqual(unichr(32), unicode(' '))
            self.assertEqual(unichr(65), unicode('A'))
            self.assertEqual(unichr(97), unicode('a'))
            self.assertEqual(
                unichr(sys.maxunicode),
                unicode('\\U%08x' % (sys.maxunicode), 'unicode-escape')
            )
            self.assertRaises(ValueError, unichr, sys.maxunicode+1)
            self.assertRaises(TypeError, unichr)
            self.assertRaises((OverflowError, ValueError), unichr, 2**32)

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

    class C_get_vars(object):
        def getDict(self):
            return {'a':2}
        __dict__ = property(fget=getDict)

    def test_vars(self):
        self.assertEqual(set(vars()), set(dir()))
        import sys
        self.assertEqual(set(vars(sys)), set(dir(sys)))
        self.assertEqual(self.get_vars_f0(), {})
        self.assertEqual(self.get_vars_f2(), {'a': 1, 'b': 2})
        self.assertRaises(TypeError, vars, 42, 42)
        self.assertRaises(TypeError, vars, 42)
        self.assertEqual(vars(self.C_get_vars()), {'a':2})

    def test_zip(self):
        a = (1, 2, 3)
        b = (4, 5, 6)
        t = [(1, 4), (2, 5), (3, 6)]
        self.assertEqual(zip(a, b), t)
        b = [4, 5, 6]
        self.assertEqual(zip(a, b), t)
        b = (4, 5, 6, 7)
        self.assertEqual(zip(a, b), t)
        class I:
            def __getitem__(self, i):
                if i < 0 or i > 2: raise IndexError
                return i + 4
        self.assertEqual(zip(a, I()), t)
        self.assertEqual(zip(), [])
        self.assertEqual(zip(*[]), [])
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
            zip(SequenceWithoutALength(), xrange(2**30)),
            list(enumerate(range(5)))
        )

        class BadSeq:
            def __getitem__(self, i):
                if i == 5:
                    raise ValueError
                else:
                    return i
        self.assertRaises(ValueError, zip, BadSeq(), BadSeq())

    def test_format(self):
        # Test the basic machinery of the format() builtin.  Don't test
        #  the specifics of the various formatters
        self.assertEqual(format(3, ''), '3')

        # Returns some classes to use for various tests.  There's
        #  an old-style version, and a new-style version
        def classes_new():
            class A(object):
                def __init__(self, x):
                    self.x = x
                def __format__(self, format_spec):
                    return str(self.x) + format_spec
            class DerivedFromA(A):
                pass

            class Simple(object): pass
            class DerivedFromSimple(Simple):
                def __init__(self, x):
                    self.x = x
                def __format__(self, format_spec):
                    return str(self.x) + format_spec
            class DerivedFromSimple2(DerivedFromSimple): pass
            return A, DerivedFromA, DerivedFromSimple, DerivedFromSimple2

        # In 3.0, classes_classic has the same meaning as classes_new
        def classes_classic():
            class A:
                def __init__(self, x):
                    self.x = x
                def __format__(self, format_spec):
                    return str(self.x) + format_spec
            class DerivedFromA(A):
                pass

            class Simple: pass
            class DerivedFromSimple(Simple):
                def __init__(self, x):
                    self.x = x
                def __format__(self, format_spec):
                    return str(self.x) + format_spec
            class DerivedFromSimple2(DerivedFromSimple): pass
            return A, DerivedFromA, DerivedFromSimple, DerivedFromSimple2

        def class_test(A, DerivedFromA, DerivedFromSimple, DerivedFromSimple2):
            self.assertEqual(format(A(3), 'spec'), '3spec')
            self.assertEqual(format(DerivedFromA(4), 'spec'), '4spec')
            self.assertEqual(format(DerivedFromSimple(5), 'abc'), '5abc')
            self.assertEqual(format(DerivedFromSimple2(10), 'abcdef'),
                             '10abcdef')

        class_test(*classes_new())
        class_test(*classes_classic())

        def empty_format_spec(value):
            # test that:
            #  format(x, '') == str(x)
            #  format(x) == str(x)
            self.assertEqual(format(value, ""), str(value))
            self.assertEqual(format(value), str(value))

        # for builtin types, format(x, "") == str(x)
        empty_format_spec(17**13)
        empty_format_spec(1.0)
        empty_format_spec(3.1415e104)
        empty_format_spec(-3.1415e104)
        empty_format_spec(3.1415e-104)
        empty_format_spec(-3.1415e-104)
        empty_format_spec(object)
        empty_format_spec(None)

        # TypeError because self.__format__ returns the wrong type
        class BadFormatResult:
            def __format__(self, format_spec):
                return 1.0
        self.assertRaises(TypeError, format, BadFormatResult(), "")

        # TypeError because format_spec is not unicode or str
        self.assertRaises(TypeError, format, object(), 4)
        self.assertRaises(TypeError, format, object(), object())

        # tests for object.__format__ really belong elsewhere, but
        #  there's no good place to put them
        x = object().__format__('')
        self.assertTrue(x.startswith('<object object at'))

        # first argument to object.__format__ must be string
        self.assertRaises(TypeError, object().__format__, 3)
        self.assertRaises(TypeError, object().__format__, object())
        self.assertRaises(TypeError, object().__format__, None)

        # --------------------------------------------------------------------
        # Issue #7994: object.__format__ with a non-empty format string is
        #  pending deprecated
        def test_deprecated_format_string(obj, fmt_str, should_raise_warning):
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter("always", PendingDeprecationWarning)
                format(obj, fmt_str)
            if should_raise_warning:
                self.assertEqual(len(w), 1)
                self.assertIsInstance(w[0].message, PendingDeprecationWarning)
                self.assertIn('object.__format__ with a non-empty format '
                              'string', str(w[0].message))
            else:
                self.assertEqual(len(w), 0)

        fmt_strs = ['', 's', u'', u's']

        class A:
            def __format__(self, fmt_str):
                return format('', fmt_str)

        for fmt_str in fmt_strs:
            test_deprecated_format_string(A(), fmt_str, False)

        class B:
            pass

        class C(object):
            pass

        for cls in [object, B, C]:
            for fmt_str in fmt_strs:
                test_deprecated_format_string(cls(), fmt_str, len(fmt_str) != 0)
        # --------------------------------------------------------------------

        # make sure we can take a subclass of str as a format spec
        class DerivedFromStr(str): pass
        self.assertEqual(format(0, DerivedFromStr('10')), '         0')

    def test_bin(self):
        self.assertEqual(bin(0), '0b0')
        self.assertEqual(bin(1), '0b1')
        self.assertEqual(bin(-1), '-0b1')
        self.assertEqual(bin(2**65), '0b1' + '0' * 65)
        self.assertEqual(bin(2**65-1), '0b' + '1' * 65)
        self.assertEqual(bin(-(2**65)), '-0b1' + '0' * 65)
        self.assertEqual(bin(-(2**65-1)), '-0b' + '1' * 65)

    def test_bytearray_translate(self):
        x = bytearray("abc")
        self.assertRaises(ValueError, x.translate, "1", 1)
        self.assertRaises(TypeError, x.translate, "1"*256, 1)

class TestExecFile(unittest.TestCase):
    # Done outside of the method test_z to get the correct scope
    z = 0
    f = open(TESTFN, 'w')
    f.write('z = z+1\n')
    f.write('z = z*2\n')
    f.close()
    with check_py3k_warnings(("execfile.. not supported in 3.x",
                              DeprecationWarning)):
        execfile(TESTFN)

    def test_execfile(self):
        globals = {'a': 1, 'b': 2}
        locals = {'b': 200, 'c': 300}

        self.assertEqual(self.__class__.z, 2)
        globals['z'] = 0
        execfile(TESTFN, globals)
        self.assertEqual(globals['z'], 2)
        locals['z'] = 0
        execfile(TESTFN, globals, locals)
        self.assertEqual(locals['z'], 2)

        class M:
            "Test mapping interface versus possible calls from execfile()."
            def __init__(self):
                self.z = 10
            def __getitem__(self, key):
                if key == 'z':
                    return self.z
                raise KeyError
            def __setitem__(self, key, value):
                if key == 'z':
                    self.z = value
                    return
                raise KeyError

        locals = M()
        locals['z'] = 0
        execfile(TESTFN, globals, locals)
        self.assertEqual(locals['z'], 2)

        unlink(TESTFN)
        self.assertRaises(TypeError, execfile)
        self.assertRaises(TypeError, execfile, TESTFN, {}, ())
        import os
        self.assertRaises(IOError, execfile, os.curdir)
        self.assertRaises(IOError, execfile, "I_dont_exist")


class TestSorted(unittest.TestCase):

    def test_basic(self):
        data = range(100)
        copy = data[:]
        random.shuffle(copy)
        self.assertEqual(data, sorted(copy))
        self.assertNotEqual(data, copy)

        data.reverse()
        random.shuffle(copy)
        self.assertEqual(data, sorted(copy, cmp=lambda x, y: cmp(y,x)))
        self.assertNotEqual(data, copy)
        random.shuffle(copy)
        self.assertEqual(data, sorted(copy, key=lambda x: -x))
        self.assertNotEqual(data, copy)
        random.shuffle(copy)
        self.assertEqual(data, sorted(copy, reverse=1))
        self.assertNotEqual(data, copy)

    def test_inputtypes(self):
        s = 'abracadabra'
        types = [list, tuple]
        if have_unicode:
            types.insert(0, unicode)
        for T in types:
            self.assertEqual(sorted(s), sorted(T(s)))

        s = ''.join(dict.fromkeys(s).keys())  # unique letters only
        types = [set, frozenset, list, tuple, dict.fromkeys]
        if have_unicode:
            types.insert(0, unicode)
        for T in types:
            self.assertEqual(sorted(s), sorted(T(s)))

    def test_baddecorator(self):
        data = 'The quick Brown fox Jumped over The lazy Dog'.split()
        self.assertRaises(TypeError, sorted, data, None, lambda x,y: 0)


class TestType(unittest.TestCase):
    def test_new_type(self):
        A = type('A', (), {})
        self.assertEqual(A.__name__, 'A')
        self.assertEqual(A.__module__, __name__)
        self.assertEqual(A.__bases__, (object,))
        self.assertIs(A.__base__, object)
        x = A()
        self.assertIs(type(x), A)
        self.assertIs(x.__class__, A)

        class B:
            def ham(self):
                return 'ham%d' % self
        C = type('C', (B, int), {'spam': lambda self: 'spam%s' % self})
        self.assertEqual(C.__name__, 'C')
        self.assertEqual(C.__module__, __name__)
        self.assertEqual(C.__bases__, (B, int))
        self.assertIs(C.__base__, int)
        self.assertIn('spam', C.__dict__)
        self.assertNotIn('ham', C.__dict__)
        x = C(42)
        self.assertEqual(x, 42)
        self.assertIs(type(x), C)
        self.assertIs(x.__class__, C)
        self.assertEqual(x.ham(), 'ham42')
        self.assertEqual(x.spam(), 'spam42')
        self.assertEqual(x.bit_length(), 6)

    def test_type_new_keywords(self):
        class B:
            def ham(self):
                return 'ham%d' % self
        C = type.__new__(type,
                         name='C',
                         bases=(B, int),
                         dict={'spam': lambda self: 'spam%s' % self})
        self.assertEqual(C.__name__, 'C')
        self.assertEqual(C.__module__, __name__)
        self.assertEqual(C.__bases__, (B, int))
        self.assertIs(C.__base__, int)
        self.assertIn('spam', C.__dict__)
        self.assertNotIn('ham', C.__dict__)

    def test_type_name(self):
        for name in 'A', '\xc4', 'B.A', '42', '':
            A = type(name, (), {})
            self.assertEqual(A.__name__, name)
            self.assertEqual(A.__module__, __name__)
        with self.assertRaises(ValueError):
            type('A\x00B', (), {})
        with self.assertRaises(TypeError):
            type(u'A', (), {})

        C = type('C', (), {})
        for name in 'A', '\xc4', 'B.A', '42', '':
            C.__name__ = name
            self.assertEqual(C.__name__, name)
            self.assertEqual(C.__module__, __name__)

        A = type('C', (), {})
        with self.assertRaises(ValueError):
            A.__name__ = 'A\x00B'
        self.assertEqual(A.__name__, 'C')
        with self.assertRaises(TypeError):
            A.__name__ = u'A'
        self.assertEqual(A.__name__, 'C')

    def test_type_doc(self):
        tests = ('x', '\xc4', 'x\x00y', 42, None)
        if have_unicode:
            tests += (u'\xc4', u'x\x00y')
        for doc in tests:
            A = type('A', (), {'__doc__': doc})
            self.assertEqual(A.__doc__, doc)

        A = type('A', (), {})
        self.assertEqual(A.__doc__, None)
        with self.assertRaises(AttributeError):
            A.__doc__ = 'x'

    def test_bad_args(self):
        with self.assertRaises(TypeError):
            type()
        with self.assertRaises(TypeError):
            type('A', ())
        with self.assertRaises(TypeError):
            type('A', (), {}, ())
        with self.assertRaises(TypeError):
            type('A', (), dict={})
        with self.assertRaises(TypeError):
            type('A', [], {})
        with self.assertRaises(TypeError):
            type('A', (), UserDict.UserDict())
        with self.assertRaises(TypeError):
            type('A', (None,), {})
        with self.assertRaises(TypeError):
            type('A', (bool,), {})
        with self.assertRaises(TypeError):
            type('A', (int, str), {})
        class B:
            pass
        with self.assertRaises(TypeError):
            type('A', (B,), {})

    def test_bad_slots(self):
        with self.assertRaises(TypeError):
            type('A', (long,), {'__slots__': 'x'})
        with self.assertRaises(TypeError):
            type('A', (), {'__slots__': ''})
        with self.assertRaises(TypeError):
            type('A', (), {'__slots__': '42'})
        with self.assertRaises(TypeError):
            type('A', (), {'__slots__': 'x\x00y'})
        with self.assertRaises(TypeError):
            type('A', (), {'__slots__': ('__dict__', '__dict__')})
        with self.assertRaises(TypeError):
            type('A', (), {'__slots__': ('__weakref__', '__weakref__')})

        class B(object):
            pass
        with self.assertRaises(TypeError):
            type('A', (B,), {'__slots__': '__dict__'})
        with self.assertRaises(TypeError):
            type('A', (B,), {'__slots__': '__weakref__'})


def _run_unittest(*args):
    with check_py3k_warnings(
            (".+ not supported in 3.x", DeprecationWarning),
            (".+ is renamed to imp.reload", DeprecationWarning),
            ("classic int division", DeprecationWarning)):
        run_unittest(*args)

def test_main(verbose=None):
    global numruns
    if not numruns:
        with check_py3k_warnings(
                (".+ not supported in 3.x", DeprecationWarning)):
            run_unittest(TestExecFile)
    numruns += 1
    test_classes = (BuiltinTest, TestSorted, TestType)

    _run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            _run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts


if __name__ == "__main__":
    test_main(verbose=True)
