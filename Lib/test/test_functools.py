import collections
import sys
import unittest
from test import support
from weakref import proxy
import pickle
from random import choice

import functools

py_functools = support.import_fresh_module('functools', blocked=['_functools'])
c_functools = support.import_fresh_module('functools', fresh=['_functools'])

def capture(*args, **kw):
    """capture all positional and keyword arguments"""
    return args, kw

def signature(part):
    """ return the signature of a partial object """
    return (part.func, part.args, part.keywords, part.__dict__)

class TestPartial:

    def test_basic_examples(self):
        p = self.partial(capture, 1, 2, a=10, b=20)
        self.assertTrue(callable(p))
        self.assertEqual(p(3, 4, b=30, c=40),
                         ((1, 2, 3, 4), dict(a=10, b=30, c=40)))
        p = self.partial(map, lambda x: x*10)
        self.assertEqual(list(p([1,2,3,4])), [10, 20, 30, 40])

    def test_attributes(self):
        p = self.partial(capture, 1, 2, a=10, b=20)
        # attributes should be readable
        self.assertEqual(p.func, capture)
        self.assertEqual(p.args, (1, 2))
        self.assertEqual(p.keywords, dict(a=10, b=20))
        # attributes should not be writable
        if not isinstance(self.partial, type):
            return
        self.assertRaises(AttributeError, setattr, p, 'func', map)
        self.assertRaises(AttributeError, setattr, p, 'args', (1, 2))
        self.assertRaises(AttributeError, setattr, p, 'keywords', dict(a=1, b=2))

        p = self.partial(hex)
        try:
            del p.__dict__
        except TypeError:
            pass
        else:
            self.fail('partial object allowed __dict__ to be deleted')

    def test_argument_checking(self):
        self.assertRaises(TypeError, self.partial)     # need at least a func arg
        try:
            self.partial(2)()
        except TypeError:
            pass
        else:
            self.fail('First arg not checked for callability')

    def test_protection_of_callers_dict_argument(self):
        # a caller's dictionary should not be altered by partial
        def func(a=10, b=20):
            return a
        d = {'a':3}
        p = self.partial(func, a=5)
        self.assertEqual(p(**d), 3)
        self.assertEqual(d, {'a':3})
        p(b=7)
        self.assertEqual(d, {'a':3})

    def test_arg_combinations(self):
        # exercise special code paths for zero args in either partial
        # object or the caller
        p = self.partial(capture)
        self.assertEqual(p(), ((), {}))
        self.assertEqual(p(1,2), ((1,2), {}))
        p = self.partial(capture, 1, 2)
        self.assertEqual(p(), ((1,2), {}))
        self.assertEqual(p(3,4), ((1,2,3,4), {}))

    def test_kw_combinations(self):
        # exercise special code paths for no keyword args in
        # either the partial object or the caller
        p = self.partial(capture)
        self.assertEqual(p(), ((), {}))
        self.assertEqual(p(a=1), ((), {'a':1}))
        p = self.partial(capture, a=1)
        self.assertEqual(p(), ((), {'a':1}))
        self.assertEqual(p(b=2), ((), {'a':1, 'b':2}))
        # keyword args in the call override those in the partial object
        self.assertEqual(p(a=3, b=2), ((), {'a':3, 'b':2}))

    def test_positional(self):
        # make sure positional arguments are captured correctly
        for args in [(), (0,), (0,1), (0,1,2), (0,1,2,3)]:
            p = self.partial(capture, *args)
            expected = args + ('x',)
            got, empty = p('x')
            self.assertTrue(expected == got and empty == {})

    def test_keyword(self):
        # make sure keyword arguments are captured correctly
        for a in ['a', 0, None, 3.5]:
            p = self.partial(capture, a=a)
            expected = {'a':a,'x':None}
            empty, got = p(x=None)
            self.assertTrue(expected == got and empty == ())

    def test_no_side_effects(self):
        # make sure there are no side effects that affect subsequent calls
        p = self.partial(capture, 0, a=1)
        args1, kw1 = p(1, b=2)
        self.assertTrue(args1 == (0,1) and kw1 == {'a':1,'b':2})
        args2, kw2 = p()
        self.assertTrue(args2 == (0,) and kw2 == {'a':1})

    def test_error_propagation(self):
        def f(x, y):
            x / y
        self.assertRaises(ZeroDivisionError, self.partial(f, 1, 0))
        self.assertRaises(ZeroDivisionError, self.partial(f, 1), 0)
        self.assertRaises(ZeroDivisionError, self.partial(f), 1, 0)
        self.assertRaises(ZeroDivisionError, self.partial(f, y=0), 1)

    def test_weakref(self):
        f = self.partial(int, base=16)
        p = proxy(f)
        self.assertEqual(f.func, p.func)
        f = None
        self.assertRaises(ReferenceError, getattr, p, 'func')

    def test_with_bound_and_unbound_methods(self):
        data = list(map(str, range(10)))
        join = self.partial(str.join, '')
        self.assertEqual(join(data), '0123456789')
        join = self.partial(''.join)
        self.assertEqual(join(data), '0123456789')

@unittest.skipUnless(c_functools, 'requires the C _functools module')
class TestPartialC(TestPartial, unittest.TestCase):
    if c_functools:
        partial = c_functools.partial

    def test_repr(self):
        args = (object(), object())
        args_repr = ', '.join(repr(a) for a in args)
        kwargs = {'a': object(), 'b': object()}
        kwargs_repr = ', '.join("%s=%r" % (k, v) for k, v in kwargs.items())
        if self.partial is c_functools.partial:
            name = 'functools.partial'
        else:
            name = self.partial.__name__

        f = self.partial(capture)
        self.assertEqual('{}({!r})'.format(name, capture),
                         repr(f))

        f = self.partial(capture, *args)
        self.assertEqual('{}({!r}, {})'.format(name, capture, args_repr),
                         repr(f))

        f = self.partial(capture, **kwargs)
        self.assertEqual('{}({!r}, {})'.format(name, capture, kwargs_repr),
                         repr(f))

        f = self.partial(capture, *args, **kwargs)
        self.assertEqual('{}({!r}, {}, {})'.format(name, capture, args_repr, kwargs_repr),
                         repr(f))

    def test_pickle(self):
        f = self.partial(signature, 'asdf', bar=True)
        f.add_something_to__dict__ = True
        f_copy = pickle.loads(pickle.dumps(f))
        self.assertEqual(signature(f), signature(f_copy))

    # Issue 6083: Reference counting bug
    def test_setstate_refcount(self):
        class BadSequence:
            def __len__(self):
                return 4
            def __getitem__(self, key):
                if key == 0:
                    return max
                elif key == 1:
                    return tuple(range(1000000))
                elif key in (2, 3):
                    return {}
                raise IndexError

        f = self.partial(object)
        self.assertRaisesRegex(SystemError,
                "new style getargs format but argument is not a tuple",
                f.__setstate__, BadSequence())

class TestPartialPy(TestPartial, unittest.TestCase):
    partial = staticmethod(py_functools.partial)

if c_functools:
    class PartialSubclass(c_functools.partial):
        pass

@unittest.skipUnless(c_functools, 'requires the C _functools module')
class TestPartialCSubclass(TestPartialC):
    if c_functools:
        partial = PartialSubclass

class TestUpdateWrapper(unittest.TestCase):

    def check_wrapper(self, wrapper, wrapped,
                      assigned=functools.WRAPPER_ASSIGNMENTS,
                      updated=functools.WRAPPER_UPDATES):
        # Check attributes were assigned
        for name in assigned:
            self.assertTrue(getattr(wrapper, name) is getattr(wrapped, name))
        # Check attributes were updated
        for name in updated:
            wrapper_attr = getattr(wrapper, name)
            wrapped_attr = getattr(wrapped, name)
            for key in wrapped_attr:
                self.assertTrue(wrapped_attr[key] is wrapper_attr[key])

    def _default_update(self):
        def f(a:'This is a new annotation'):
            """This is a test"""
            pass
        f.attr = 'This is also a test'
        def wrapper(b:'This is the prior annotation'):
            pass
        functools.update_wrapper(wrapper, f)
        return wrapper, f

    def test_default_update(self):
        wrapper, f = self._default_update()
        self.check_wrapper(wrapper, f)
        self.assertIs(wrapper.__wrapped__, f)
        self.assertEqual(wrapper.__name__, 'f')
        self.assertEqual(wrapper.__qualname__, f.__qualname__)
        self.assertEqual(wrapper.attr, 'This is also a test')
        self.assertEqual(wrapper.__annotations__['a'], 'This is a new annotation')
        self.assertNotIn('b', wrapper.__annotations__)

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_default_update_doc(self):
        wrapper, f = self._default_update()
        self.assertEqual(wrapper.__doc__, 'This is a test')

    def test_no_update(self):
        def f():
            """This is a test"""
            pass
        f.attr = 'This is also a test'
        def wrapper():
            pass
        functools.update_wrapper(wrapper, f, (), ())
        self.check_wrapper(wrapper, f, (), ())
        self.assertEqual(wrapper.__name__, 'wrapper')
        self.assertNotEqual(wrapper.__qualname__, f.__qualname__)
        self.assertEqual(wrapper.__doc__, None)
        self.assertEqual(wrapper.__annotations__, {})
        self.assertFalse(hasattr(wrapper, 'attr'))

    def test_selective_update(self):
        def f():
            pass
        f.attr = 'This is a different test'
        f.dict_attr = dict(a=1, b=2, c=3)
        def wrapper():
            pass
        wrapper.dict_attr = {}
        assign = ('attr',)
        update = ('dict_attr',)
        functools.update_wrapper(wrapper, f, assign, update)
        self.check_wrapper(wrapper, f, assign, update)
        self.assertEqual(wrapper.__name__, 'wrapper')
        self.assertNotEqual(wrapper.__qualname__, f.__qualname__)
        self.assertEqual(wrapper.__doc__, None)
        self.assertEqual(wrapper.attr, 'This is a different test')
        self.assertEqual(wrapper.dict_attr, f.dict_attr)

    def test_missing_attributes(self):
        def f():
            pass
        def wrapper():
            pass
        wrapper.dict_attr = {}
        assign = ('attr',)
        update = ('dict_attr',)
        # Missing attributes on wrapped object are ignored
        functools.update_wrapper(wrapper, f, assign, update)
        self.assertNotIn('attr', wrapper.__dict__)
        self.assertEqual(wrapper.dict_attr, {})
        # Wrapper must have expected attributes for updating
        del wrapper.dict_attr
        with self.assertRaises(AttributeError):
            functools.update_wrapper(wrapper, f, assign, update)
        wrapper.dict_attr = 1
        with self.assertRaises(AttributeError):
            functools.update_wrapper(wrapper, f, assign, update)

    @support.requires_docstrings
    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_builtin_update(self):
        # Test for bug #1576241
        def wrapper():
            pass
        functools.update_wrapper(wrapper, max)
        self.assertEqual(wrapper.__name__, 'max')
        self.assertTrue(wrapper.__doc__.startswith('max('))
        self.assertEqual(wrapper.__annotations__, {})

class TestWraps(TestUpdateWrapper):

    def _default_update(self):
        def f():
            """This is a test"""
            pass
        f.attr = 'This is also a test'
        @functools.wraps(f)
        def wrapper():
            pass
        self.check_wrapper(wrapper, f)
        return wrapper, f

    def test_default_update(self):
        wrapper, f = self._default_update()
        self.assertEqual(wrapper.__name__, 'f')
        self.assertEqual(wrapper.__qualname__, f.__qualname__)
        self.assertEqual(wrapper.attr, 'This is also a test')

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_default_update_doc(self):
        wrapper, _ = self._default_update()
        self.assertEqual(wrapper.__doc__, 'This is a test')

    def test_no_update(self):
        def f():
            """This is a test"""
            pass
        f.attr = 'This is also a test'
        @functools.wraps(f, (), ())
        def wrapper():
            pass
        self.check_wrapper(wrapper, f, (), ())
        self.assertEqual(wrapper.__name__, 'wrapper')
        self.assertNotEqual(wrapper.__qualname__, f.__qualname__)
        self.assertEqual(wrapper.__doc__, None)
        self.assertFalse(hasattr(wrapper, 'attr'))

    def test_selective_update(self):
        def f():
            pass
        f.attr = 'This is a different test'
        f.dict_attr = dict(a=1, b=2, c=3)
        def add_dict_attr(f):
            f.dict_attr = {}
            return f
        assign = ('attr',)
        update = ('dict_attr',)
        @functools.wraps(f, assign, update)
        @add_dict_attr
        def wrapper():
            pass
        self.check_wrapper(wrapper, f, assign, update)
        self.assertEqual(wrapper.__name__, 'wrapper')
        self.assertNotEqual(wrapper.__qualname__, f.__qualname__)
        self.assertEqual(wrapper.__doc__, None)
        self.assertEqual(wrapper.attr, 'This is a different test')
        self.assertEqual(wrapper.dict_attr, f.dict_attr)

class TestReduce(unittest.TestCase):
    func = functools.reduce

    def test_reduce(self):
        class Squares:
            def __init__(self, max):
                self.max = max
                self.sofar = []

            def __len__(self):
                return len(self.sofar)

            def __getitem__(self, i):
                if not 0 <= i < self.max: raise IndexError
                n = len(self.sofar)
                while n <= i:
                    self.sofar.append(n*n)
                    n += 1
                return self.sofar[i]
        def add(x, y):
            return x + y
        self.assertEqual(self.func(add, ['a', 'b', 'c'], ''), 'abc')
        self.assertEqual(
            self.func(add, [['a', 'c'], [], ['d', 'w']], []),
            ['a','c','d','w']
        )
        self.assertEqual(self.func(lambda x, y: x*y, range(2,8), 1), 5040)
        self.assertEqual(
            self.func(lambda x, y: x*y, range(2,21), 1),
            2432902008176640000
        )
        self.assertEqual(self.func(add, Squares(10)), 285)
        self.assertEqual(self.func(add, Squares(10), 0), 285)
        self.assertEqual(self.func(add, Squares(0), 0), 0)
        self.assertRaises(TypeError, self.func)
        self.assertRaises(TypeError, self.func, 42, 42)
        self.assertRaises(TypeError, self.func, 42, 42, 42)
        self.assertEqual(self.func(42, "1"), "1") # func is never called with one item
        self.assertEqual(self.func(42, "", "1"), "1") # func is never called with one item
        self.assertRaises(TypeError, self.func, 42, (42, 42))
        self.assertRaises(TypeError, self.func, add, []) # arg 2 must not be empty sequence with no initial value
        self.assertRaises(TypeError, self.func, add, "")
        self.assertRaises(TypeError, self.func, add, ())
        self.assertRaises(TypeError, self.func, add, object())

        class TestFailingIter:
            def __iter__(self):
                raise RuntimeError
        self.assertRaises(RuntimeError, self.func, add, TestFailingIter())

        self.assertEqual(self.func(add, [], None), None)
        self.assertEqual(self.func(add, [], 42), 42)

        class BadSeq:
            def __getitem__(self, index):
                raise ValueError
        self.assertRaises(ValueError, self.func, 42, BadSeq())

    # Test reduce()'s use of iterators.
    def test_iterator_usage(self):
        class SequenceClass:
            def __init__(self, n):
                self.n = n
            def __getitem__(self, i):
                if 0 <= i < self.n:
                    return i
                else:
                    raise IndexError

        from operator import add
        self.assertEqual(self.func(add, SequenceClass(5)), 10)
        self.assertEqual(self.func(add, SequenceClass(5), 42), 52)
        self.assertRaises(TypeError, self.func, add, SequenceClass(0))
        self.assertEqual(self.func(add, SequenceClass(0), 42), 42)
        self.assertEqual(self.func(add, SequenceClass(1)), 0)
        self.assertEqual(self.func(add, SequenceClass(1), 42), 42)

        d = {"one": 1, "two": 2, "three": 3}
        self.assertEqual(self.func(add, d), "".join(d.keys()))

class TestCmpToKey:

    def test_cmp_to_key(self):
        def cmp1(x, y):
            return (x > y) - (x < y)
        key = self.cmp_to_key(cmp1)
        self.assertEqual(key(3), key(3))
        self.assertGreater(key(3), key(1))
        self.assertGreaterEqual(key(3), key(3))

        def cmp2(x, y):
            return int(x) - int(y)
        key = self.cmp_to_key(cmp2)
        self.assertEqual(key(4.0), key('4'))
        self.assertLess(key(2), key('35'))
        self.assertLessEqual(key(2), key('35'))
        self.assertNotEqual(key(2), key('35'))

    def test_cmp_to_key_arguments(self):
        def cmp1(x, y):
            return (x > y) - (x < y)
        key = self.cmp_to_key(mycmp=cmp1)
        self.assertEqual(key(obj=3), key(obj=3))
        self.assertGreater(key(obj=3), key(obj=1))
        with self.assertRaises((TypeError, AttributeError)):
            key(3) > 1    # rhs is not a K object
        with self.assertRaises((TypeError, AttributeError)):
            1 < key(3)    # lhs is not a K object
        with self.assertRaises(TypeError):
            key = self.cmp_to_key()             # too few args
        with self.assertRaises(TypeError):
            key = self.cmp_to_key(cmp1, None)   # too many args
        key = self.cmp_to_key(cmp1)
        with self.assertRaises(TypeError):
            key()                                    # too few args
        with self.assertRaises(TypeError):
            key(None, None)                          # too many args

    def test_bad_cmp(self):
        def cmp1(x, y):
            raise ZeroDivisionError
        key = self.cmp_to_key(cmp1)
        with self.assertRaises(ZeroDivisionError):
            key(3) > key(1)

        class BadCmp:
            def __lt__(self, other):
                raise ZeroDivisionError
        def cmp1(x, y):
            return BadCmp()
        with self.assertRaises(ZeroDivisionError):
            key(3) > key(1)

    def test_obj_field(self):
        def cmp1(x, y):
            return (x > y) - (x < y)
        key = self.cmp_to_key(mycmp=cmp1)
        self.assertEqual(key(50).obj, 50)

    def test_sort_int(self):
        def mycmp(x, y):
            return y - x
        self.assertEqual(sorted(range(5), key=self.cmp_to_key(mycmp)),
                         [4, 3, 2, 1, 0])

    def test_sort_int_str(self):
        def mycmp(x, y):
            x, y = int(x), int(y)
            return (x > y) - (x < y)
        values = [5, '3', 7, 2, '0', '1', 4, '10', 1]
        values = sorted(values, key=self.cmp_to_key(mycmp))
        self.assertEqual([int(value) for value in values],
                         [0, 1, 1, 2, 3, 4, 5, 7, 10])

    def test_hash(self):
        def mycmp(x, y):
            return y - x
        key = self.cmp_to_key(mycmp)
        k = key(10)
        self.assertRaises(TypeError, hash, k)
        self.assertNotIsInstance(k, collections.Hashable)

@unittest.skipUnless(c_functools, 'requires the C _functools module')
class TestCmpToKeyC(TestCmpToKey, unittest.TestCase):
    if c_functools:
        cmp_to_key = c_functools.cmp_to_key

class TestCmpToKeyPy(TestCmpToKey, unittest.TestCase):
    cmp_to_key = staticmethod(py_functools.cmp_to_key)

class TestTotalOrdering(unittest.TestCase):

    def test_total_ordering_lt(self):
        @functools.total_ordering
        class A:
            def __init__(self, value):
                self.value = value
            def __lt__(self, other):
                return self.value < other.value
            def __eq__(self, other):
                return self.value == other.value
        self.assertTrue(A(1) < A(2))
        self.assertTrue(A(2) > A(1))
        self.assertTrue(A(1) <= A(2))
        self.assertTrue(A(2) >= A(1))
        self.assertTrue(A(2) <= A(2))
        self.assertTrue(A(2) >= A(2))

    def test_total_ordering_le(self):
        @functools.total_ordering
        class A:
            def __init__(self, value):
                self.value = value
            def __le__(self, other):
                return self.value <= other.value
            def __eq__(self, other):
                return self.value == other.value
        self.assertTrue(A(1) < A(2))
        self.assertTrue(A(2) > A(1))
        self.assertTrue(A(1) <= A(2))
        self.assertTrue(A(2) >= A(1))
        self.assertTrue(A(2) <= A(2))
        self.assertTrue(A(2) >= A(2))

    def test_total_ordering_gt(self):
        @functools.total_ordering
        class A:
            def __init__(self, value):
                self.value = value
            def __gt__(self, other):
                return self.value > other.value
            def __eq__(self, other):
                return self.value == other.value
        self.assertTrue(A(1) < A(2))
        self.assertTrue(A(2) > A(1))
        self.assertTrue(A(1) <= A(2))
        self.assertTrue(A(2) >= A(1))
        self.assertTrue(A(2) <= A(2))
        self.assertTrue(A(2) >= A(2))

    def test_total_ordering_ge(self):
        @functools.total_ordering
        class A:
            def __init__(self, value):
                self.value = value
            def __ge__(self, other):
                return self.value >= other.value
            def __eq__(self, other):
                return self.value == other.value
        self.assertTrue(A(1) < A(2))
        self.assertTrue(A(2) > A(1))
        self.assertTrue(A(1) <= A(2))
        self.assertTrue(A(2) >= A(1))
        self.assertTrue(A(2) <= A(2))
        self.assertTrue(A(2) >= A(2))

    def test_total_ordering_no_overwrite(self):
        # new methods should not overwrite existing
        @functools.total_ordering
        class A(int):
            pass
        self.assertTrue(A(1) < A(2))
        self.assertTrue(A(2) > A(1))
        self.assertTrue(A(1) <= A(2))
        self.assertTrue(A(2) >= A(1))
        self.assertTrue(A(2) <= A(2))
        self.assertTrue(A(2) >= A(2))

    def test_no_operations_defined(self):
        with self.assertRaises(ValueError):
            @functools.total_ordering
            class A:
                pass

    def test_bug_10042(self):
        @functools.total_ordering
        class TestTO:
            def __init__(self, value):
                self.value = value
            def __eq__(self, other):
                if isinstance(other, TestTO):
                    return self.value == other.value
                return False
            def __lt__(self, other):
                if isinstance(other, TestTO):
                    return self.value < other.value
                raise TypeError
        with self.assertRaises(TypeError):
            TestTO(8) <= ()

class TestLRU(unittest.TestCase):

    def test_lru(self):
        def orig(x, y):
            return 3 * x + y
        f = functools.lru_cache(maxsize=20)(orig)
        hits, misses, maxsize, currsize = f.cache_info()
        self.assertEqual(maxsize, 20)
        self.assertEqual(currsize, 0)
        self.assertEqual(hits, 0)
        self.assertEqual(misses, 0)

        domain = range(5)
        for i in range(1000):
            x, y = choice(domain), choice(domain)
            actual = f(x, y)
            expected = orig(x, y)
            self.assertEqual(actual, expected)
        hits, misses, maxsize, currsize = f.cache_info()
        self.assertTrue(hits > misses)
        self.assertEqual(hits + misses, 1000)
        self.assertEqual(currsize, 20)

        f.cache_clear()   # test clearing
        hits, misses, maxsize, currsize = f.cache_info()
        self.assertEqual(hits, 0)
        self.assertEqual(misses, 0)
        self.assertEqual(currsize, 0)
        f(x, y)
        hits, misses, maxsize, currsize = f.cache_info()
        self.assertEqual(hits, 0)
        self.assertEqual(misses, 1)
        self.assertEqual(currsize, 1)

        # Test bypassing the cache
        self.assertIs(f.__wrapped__, orig)
        f.__wrapped__(x, y)
        hits, misses, maxsize, currsize = f.cache_info()
        self.assertEqual(hits, 0)
        self.assertEqual(misses, 1)
        self.assertEqual(currsize, 1)

        # test size zero (which means "never-cache")
        @functools.lru_cache(0)
        def f():
            nonlocal f_cnt
            f_cnt += 1
            return 20
        self.assertEqual(f.cache_info().maxsize, 0)
        f_cnt = 0
        for i in range(5):
            self.assertEqual(f(), 20)
        self.assertEqual(f_cnt, 5)
        hits, misses, maxsize, currsize = f.cache_info()
        self.assertEqual(hits, 0)
        self.assertEqual(misses, 5)
        self.assertEqual(currsize, 0)

        # test size one
        @functools.lru_cache(1)
        def f():
            nonlocal f_cnt
            f_cnt += 1
            return 20
        self.assertEqual(f.cache_info().maxsize, 1)
        f_cnt = 0
        for i in range(5):
            self.assertEqual(f(), 20)
        self.assertEqual(f_cnt, 1)
        hits, misses, maxsize, currsize = f.cache_info()
        self.assertEqual(hits, 4)
        self.assertEqual(misses, 1)
        self.assertEqual(currsize, 1)

        # test size two
        @functools.lru_cache(2)
        def f(x):
            nonlocal f_cnt
            f_cnt += 1
            return x*10
        self.assertEqual(f.cache_info().maxsize, 2)
        f_cnt = 0
        for x in 7, 9, 7, 9, 7, 9, 8, 8, 8, 9, 9, 9, 8, 8, 8, 7:
            #    *  *              *                          *
            self.assertEqual(f(x), x*10)
        self.assertEqual(f_cnt, 4)
        hits, misses, maxsize, currsize = f.cache_info()
        self.assertEqual(hits, 12)
        self.assertEqual(misses, 4)
        self.assertEqual(currsize, 2)

    def test_lru_with_maxsize_none(self):
        @functools.lru_cache(maxsize=None)
        def fib(n):
            if n < 2:
                return n
            return fib(n-1) + fib(n-2)
        self.assertEqual([fib(n) for n in range(16)],
            [0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610])
        self.assertEqual(fib.cache_info(),
            functools._CacheInfo(hits=28, misses=16, maxsize=None, currsize=16))
        fib.cache_clear()
        self.assertEqual(fib.cache_info(),
            functools._CacheInfo(hits=0, misses=0, maxsize=None, currsize=0))

    def test_lru_with_exceptions(self):
        # Verify that user_function exceptions get passed through without
        # creating a hard-to-read chained exception.
        # http://bugs.python.org/issue13177
        for maxsize in (None, 128):
            @functools.lru_cache(maxsize)
            def func(i):
                return 'abc'[i]
            self.assertEqual(func(0), 'a')
            with self.assertRaises(IndexError) as cm:
                func(15)
            self.assertIsNone(cm.exception.__context__)
            # Verify that the previous exception did not result in a cached entry
            with self.assertRaises(IndexError):
                func(15)

    def test_lru_with_types(self):
        for maxsize in (None, 128):
            @functools.lru_cache(maxsize=maxsize, typed=True)
            def square(x):
                return x * x
            self.assertEqual(square(3), 9)
            self.assertEqual(type(square(3)), type(9))
            self.assertEqual(square(3.0), 9.0)
            self.assertEqual(type(square(3.0)), type(9.0))
            self.assertEqual(square(x=3), 9)
            self.assertEqual(type(square(x=3)), type(9))
            self.assertEqual(square(x=3.0), 9.0)
            self.assertEqual(type(square(x=3.0)), type(9.0))
            self.assertEqual(square.cache_info().hits, 4)
            self.assertEqual(square.cache_info().misses, 4)

    def test_lru_with_keyword_args(self):
        @functools.lru_cache()
        def fib(n):
            if n < 2:
                return n
            return fib(n=n-1) + fib(n=n-2)
        self.assertEqual(
            [fib(n=number) for number in range(16)],
            [0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610]
        )
        self.assertEqual(fib.cache_info(),
            functools._CacheInfo(hits=28, misses=16, maxsize=128, currsize=16))
        fib.cache_clear()
        self.assertEqual(fib.cache_info(),
            functools._CacheInfo(hits=0, misses=0, maxsize=128, currsize=0))

    def test_lru_with_keyword_args_maxsize_none(self):
        @functools.lru_cache(maxsize=None)
        def fib(n):
            if n < 2:
                return n
            return fib(n=n-1) + fib(n=n-2)
        self.assertEqual([fib(n=number) for number in range(16)],
            [0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610])
        self.assertEqual(fib.cache_info(),
            functools._CacheInfo(hits=28, misses=16, maxsize=None, currsize=16))
        fib.cache_clear()
        self.assertEqual(fib.cache_info(),
            functools._CacheInfo(hits=0, misses=0, maxsize=None, currsize=0))

    def test_need_for_rlock(self):
        # This will deadlock on an LRU cache that uses a regular lock

        @functools.lru_cache(maxsize=10)
        def test_func(x):
            'Used to demonstrate a reentrant lru_cache call within a single thread'
            return x

        class DoubleEq:
            'Demonstrate a reentrant lru_cache call within a single thread'
            def __init__(self, x):
                self.x = x
            def __hash__(self):
                return self.x
            def __eq__(self, other):
                if self.x == 2:
                    test_func(DoubleEq(1))
                return self.x == other.x

        test_func(DoubleEq(1))                      # Load the cache
        test_func(DoubleEq(2))                      # Load the cache
        self.assertEqual(test_func(DoubleEq(2)),    # Trigger a re-entrant __eq__ call
                         DoubleEq(2))               # Verify the correct return value


def test_main(verbose=None):
    test_classes = (
        TestPartialC,
        TestPartialPy,
        TestPartialCSubclass,
        TestUpdateWrapper,
        TestTotalOrdering,
        TestCmpToKeyC,
        TestCmpToKeyPy,
        TestWraps,
        TestReduce,
        TestLRU,
    )
    support.run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in range(len(counts)):
            support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print(counts)

if __name__ == '__main__':
    test_main(verbose=True)
