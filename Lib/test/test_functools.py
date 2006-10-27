import functools
import unittest
from test import test_support
from weakref import proxy

@staticmethod
def PythonPartial(func, *args, **keywords):
    'Pure Python approximation of partial()'
    def newfunc(*fargs, **fkeywords):
        newkeywords = keywords.copy()
        newkeywords.update(fkeywords)
        return func(*(args + fargs), **newkeywords)
    newfunc.func = func
    newfunc.args = args
    newfunc.keywords = keywords
    return newfunc

def capture(*args, **kw):
    """capture all positional and keyword arguments"""
    return args, kw

class TestPartial(unittest.TestCase):

    thetype = functools.partial

    def test_basic_examples(self):
        p = self.thetype(capture, 1, 2, a=10, b=20)
        self.assertEqual(p(3, 4, b=30, c=40),
                         ((1, 2, 3, 4), dict(a=10, b=30, c=40)))
        p = self.thetype(map, lambda x: x*10)
        self.assertEqual(p([1,2,3,4]), [10, 20, 30, 40])

    def test_attributes(self):
        p = self.thetype(capture, 1, 2, a=10, b=20)
        # attributes should be readable
        self.assertEqual(p.func, capture)
        self.assertEqual(p.args, (1, 2))
        self.assertEqual(p.keywords, dict(a=10, b=20))
        # attributes should not be writable
        if not isinstance(self.thetype, type):
            return
        self.assertRaises(TypeError, setattr, p, 'func', map)
        self.assertRaises(TypeError, setattr, p, 'args', (1, 2))
        self.assertRaises(TypeError, setattr, p, 'keywords', dict(a=1, b=2))

    def test_argument_checking(self):
        self.assertRaises(TypeError, self.thetype)     # need at least a func arg
        try:
            self.thetype(2)()
        except TypeError:
            pass
        else:
            self.fail('First arg not checked for callability')

    def test_protection_of_callers_dict_argument(self):
        # a caller's dictionary should not be altered by partial
        def func(a=10, b=20):
            return a
        d = {'a':3}
        p = self.thetype(func, a=5)
        self.assertEqual(p(**d), 3)
        self.assertEqual(d, {'a':3})
        p(b=7)
        self.assertEqual(d, {'a':3})

    def test_arg_combinations(self):
        # exercise special code paths for zero args in either partial
        # object or the caller
        p = self.thetype(capture)
        self.assertEqual(p(), ((), {}))
        self.assertEqual(p(1,2), ((1,2), {}))
        p = self.thetype(capture, 1, 2)
        self.assertEqual(p(), ((1,2), {}))
        self.assertEqual(p(3,4), ((1,2,3,4), {}))

    def test_kw_combinations(self):
        # exercise special code paths for no keyword args in
        # either the partial object or the caller
        p = self.thetype(capture)
        self.assertEqual(p(), ((), {}))
        self.assertEqual(p(a=1), ((), {'a':1}))
        p = self.thetype(capture, a=1)
        self.assertEqual(p(), ((), {'a':1}))
        self.assertEqual(p(b=2), ((), {'a':1, 'b':2}))
        # keyword args in the call override those in the partial object
        self.assertEqual(p(a=3, b=2), ((), {'a':3, 'b':2}))

    def test_positional(self):
        # make sure positional arguments are captured correctly
        for args in [(), (0,), (0,1), (0,1,2), (0,1,2,3)]:
            p = self.thetype(capture, *args)
            expected = args + ('x',)
            got, empty = p('x')
            self.failUnless(expected == got and empty == {})

    def test_keyword(self):
        # make sure keyword arguments are captured correctly
        for a in ['a', 0, None, 3.5]:
            p = self.thetype(capture, a=a)
            expected = {'a':a,'x':None}
            empty, got = p(x=None)
            self.failUnless(expected == got and empty == ())

    def test_no_side_effects(self):
        # make sure there are no side effects that affect subsequent calls
        p = self.thetype(capture, 0, a=1)
        args1, kw1 = p(1, b=2)
        self.failUnless(args1 == (0,1) and kw1 == {'a':1,'b':2})
        args2, kw2 = p()
        self.failUnless(args2 == (0,) and kw2 == {'a':1})

    def test_error_propagation(self):
        def f(x, y):
            x / y
        self.assertRaises(ZeroDivisionError, self.thetype(f, 1, 0))
        self.assertRaises(ZeroDivisionError, self.thetype(f, 1), 0)
        self.assertRaises(ZeroDivisionError, self.thetype(f), 1, 0)
        self.assertRaises(ZeroDivisionError, self.thetype(f, y=0), 1)

    def test_attributes(self):
        p = self.thetype(hex)
        try:
            del p.__dict__
        except TypeError:
            pass
        else:
            self.fail('partial object allowed __dict__ to be deleted')

    def test_weakref(self):
        f = self.thetype(int, base=16)
        p = proxy(f)
        self.assertEqual(f.func, p.func)
        f = None
        self.assertRaises(ReferenceError, getattr, p, 'func')

    def test_with_bound_and_unbound_methods(self):
        data = map(str, range(10))
        join = self.thetype(str.join, '')
        self.assertEqual(join(data), '0123456789')
        join = self.thetype(''.join)
        self.assertEqual(join(data), '0123456789')

class PartialSubclass(functools.partial):
    pass

class TestPartialSubclass(TestPartial):

    thetype = PartialSubclass


class TestPythonPartial(TestPartial):

    thetype = PythonPartial

class TestUpdateWrapper(unittest.TestCase):

    def check_wrapper(self, wrapper, wrapped,
                      assigned=functools.WRAPPER_ASSIGNMENTS,
                      updated=functools.WRAPPER_UPDATES):
        # Check attributes were assigned
        for name in assigned:
            self.failUnless(getattr(wrapper, name) is getattr(wrapped, name))
        # Check attributes were updated
        for name in updated:
            wrapper_attr = getattr(wrapper, name)
            wrapped_attr = getattr(wrapped, name)
            for key in wrapped_attr:
                self.failUnless(wrapped_attr[key] is wrapper_attr[key])

    def test_default_update(self):
        def f():
            """This is a test"""
            pass
        f.attr = 'This is also a test'
        def wrapper():
            pass
        functools.update_wrapper(wrapper, f)
        self.check_wrapper(wrapper, f)
        self.assertEqual(wrapper.__name__, 'f')
        self.assertEqual(wrapper.__doc__, 'This is a test')
        self.assertEqual(wrapper.attr, 'This is also a test')

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
        self.assertEqual(wrapper.__doc__, None)
        self.failIf(hasattr(wrapper, 'attr'))

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
        self.assertEqual(wrapper.__doc__, None)
        self.assertEqual(wrapper.attr, 'This is a different test')
        self.assertEqual(wrapper.dict_attr, f.dict_attr)

    def test_builtin_update(self):
        # Test for bug #1576241
        def wrapper():
            pass
        functools.update_wrapper(wrapper, max)
        self.assertEqual(wrapper.__name__, 'max')
        self.assert_(wrapper.__doc__.startswith('max('))

class TestWraps(TestUpdateWrapper):

    def test_default_update(self):
        def f():
            """This is a test"""
            pass
        f.attr = 'This is also a test'
        @functools.wraps(f)
        def wrapper():
            pass
        self.check_wrapper(wrapper, f)
        self.assertEqual(wrapper.__name__, 'f')
        self.assertEqual(wrapper.__doc__, 'This is a test')
        self.assertEqual(wrapper.attr, 'This is also a test')

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
        self.assertEqual(wrapper.__doc__, None)
        self.failIf(hasattr(wrapper, 'attr'))

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
        self.assertEqual(wrapper.__doc__, None)
        self.assertEqual(wrapper.attr, 'This is a different test')
        self.assertEqual(wrapper.dict_attr, f.dict_attr)



def test_main(verbose=None):
    import sys
    test_classes = (
        TestPartial,
        TestPartialSubclass,
        TestPythonPartial,
        TestUpdateWrapper,
        TestWraps
    )
    test_support.run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts

if __name__ == '__main__':
    test_main(verbose=True)
