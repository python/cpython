"""Unit tests for the copy module."""

import sys
import copy
import copy_reg

import unittest
from test import test_support

class TestCopy(unittest.TestCase):

    # Attempt full line coverage of copy.py from top to bottom

    def test_exceptions(self):
        self.assert_(copy.Error is copy.error)
        self.assert_(issubclass(copy.Error, Exception))

    # The copy() method

    def test_copy_basic(self):
        x = 42
        y = copy.copy(x)
        self.assertEqual(x, y)

    def test_copy_copy(self):
        class C(object):
            def __init__(self, foo):
                self.foo = foo
            def __copy__(self):
                return C(self.foo)
        x = C(42)
        y = copy.copy(x)
        self.assertEqual(y.__class__, x.__class__)
        self.assertEqual(y.foo, x.foo)

    def test_copy_registry(self):
        class C(object):
            def __new__(cls, foo):
                obj = object.__new__(cls)
                obj.foo = foo
                return obj
        def pickle_C(obj):
            return (C, (obj.foo,))
        x = C(42)
        self.assertRaises(TypeError, copy.copy, x)
        copy_reg.pickle(C, pickle_C, C)
        y = copy.copy(x)

    def test_copy_reduce_ex(self):
        class C(object):
            def __reduce_ex__(self, proto):
                return ""
            def __reduce__(self):
                raise test_support.TestFailed, "shouldn't call this"
        x = C()
        y = copy.copy(x)
        self.assert_(y is x)

    def test_copy_reduce(self):
        class C(object):
            def __reduce__(self):
                return ""
        x = C()
        y = copy.copy(x)
        self.assert_(y is x)

    def test_copy_cant(self):
        class C(object):
            def __getattribute__(self, name):
                if name.startswith("__reduce"):
                    raise AttributeError, name
                return object.__getattribute__(self, name)
        x = C()
        self.assertRaises(copy.Error, copy.copy, x)

    # Type-specific _copy_xxx() methods

    def test_copy_atomic(self):
        class Classic:
            pass
        class NewStyle(object):
            pass
        def f():
            pass
        tests = [None, 42, 2L**100, 3.14, True, False, 1j,
                 "hello", u"hello\u1234", f.func_code,
                 NewStyle, xrange(10), Classic, max]
        for x in tests:
            self.assert_(copy.copy(x) is x, repr(x))

    def test_copy_list(self):
        x = [1, 2, 3]
        self.assertEqual(copy.copy(x), x)

    def test_copy_tuple(self):
        x = (1, 2, 3)
        self.assertEqual(copy.copy(x), x)

    def test_copy_dict(self):
        x = {"foo": 1, "bar": 2}
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_vanilla(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_copy(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __copy__(self):
                return C(self.foo)
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_getinitargs(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getinitargs__(self):
                return (self.foo,)
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_getstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return {"foo": self.foo}
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __setstate__(self, state):
                self.foo = state["foo"]
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    def test_copy_inst_getstate_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return self.foo
            def __setstate__(self, state):
                self.foo = state
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C(42)
        self.assertEqual(copy.copy(x), x)

    # tests for copying extension types, iff module trycopy is installed
    def test_copy_classictype(self):
        from _testcapi import make_copyable
        x = make_copyable([23])
        y = copy.copy(x)
        self.assertEqual(x, y)
        self.assertEqual(x.tag, y.tag)
        self.assert_(x is not y)
        self.assert_(x.tag is y.tag)

    def test_deepcopy_classictype(self):
        from _testcapi import make_copyable
        x = make_copyable([23])
        y = copy.deepcopy(x)
        self.assertEqual(x, y)
        self.assertEqual(x.tag, y.tag)
        self.assert_(x is not y)
        self.assert_(x.tag is not y.tag)

    # regression tests for class-vs-instance and metaclass-confusion
    def test_copy_classoverinstance(self):
        class C(object):
            def __init__(self, v):
                self.v = v
            def __cmp__(self, other):
                return -cmp(other, self.v)
            def __copy__(self):
                return self.__class__(self.v)
        x = C(23)
        self.assertEqual(copy.copy(x), x)
        x.__copy__ = lambda: 42
        self.assertEqual(copy.copy(x), x)

    def test_deepcopy_classoverinstance(self):
        class C(object):
            def __init__(self, v):
                self.v = v
            def __cmp__(self, other):
                return -cmp(other, self.v)
            def __deepcopy__(self, memo):
                return self.__class__(copy.deepcopy(self.v, memo))
        x = C(23)
        self.assertEqual(copy.deepcopy(x), x)
        x.__deepcopy__ = lambda memo: 42
        self.assertEqual(copy.deepcopy(x), x)


    def test_copy_metaclassconfusion(self):
        class MyOwnError(copy.Error):
            pass
        class Meta(type):
            def __copy__(cls):
                raise MyOwnError("can't copy classes w/this metaclass")
        class C:
            __metaclass__ = Meta
            def __init__(self, tag):
                self.tag = tag
            def __cmp__(self, other):
                return -cmp(other, self.tag)
        # the metaclass can forbid shallow copying of its classes
        self.assertRaises(MyOwnError, copy.copy, C)
        # check that there is no interference with instances
        x = C(23)
        self.assertEqual(copy.copy(x), x)

    def test_deepcopy_metaclassconfusion(self):
        class MyOwnError(copy.Error):
            pass
        class Meta(type):
            def __deepcopy__(cls, memo):
                raise MyOwnError("can't deepcopy classes w/this metaclass")
        class C:
            __metaclass__ = Meta
            def __init__(self, tag):
                self.tag = tag
            def __cmp__(self, other):
                return -cmp(other, self.tag)
        # types are ALWAYS deepcopied atomically, no matter what
        self.assertEqual(copy.deepcopy(C), C)
        # check that there is no interference with instances
        x = C(23)
        self.assertEqual(copy.deepcopy(x), x)

    def _nomro(self):
        class C(type):
            def __getattribute__(self, attr):
                if attr == '__mro__':
                    raise AttributeError, "What, *me*, a __mro__? Nevah!"
                return super(C, self).__getattribute__(attr)
        class D(object):
            __metaclass__ = C
        return D()

    def test_copy_mro(self):
        x = self._nomro()
        y = copy.copy(x)

    def test_deepcopy_mro(self):
        x = self._nomro()
        y = copy.deepcopy(x)

    # The deepcopy() method

    def test_deepcopy_basic(self):
        x = 42
        y = copy.deepcopy(x)
        self.assertEqual(y, x)

    def test_deepcopy_memo(self):
        # Tests of reflexive objects are under type-specific sections below.
        # This tests only repetitions of objects.
        x = []
        x = [x, x]
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(y is not x)
        self.assert_(y[0] is not x[0])
        self.assert_(y[0] is y[1])

    def test_deepcopy_issubclass(self):
        # XXX Note: there's no way to test the TypeError coming out of
        # issubclass() -- this can only happen when an extension
        # module defines a "type" that doesn't formally inherit from
        # type.
        class Meta(type):
            pass
        class C:
            __metaclass__ = Meta
        self.assertEqual(copy.deepcopy(C), C)

    def test_deepcopy_deepcopy(self):
        class C(object):
            def __init__(self, foo):
                self.foo = foo
            def __deepcopy__(self, memo=None):
                return C(self.foo)
        x = C(42)
        y = copy.deepcopy(x)
        self.assertEqual(y.__class__, x.__class__)
        self.assertEqual(y.foo, x.foo)

    def test_deepcopy_registry(self):
        class C(object):
            def __new__(cls, foo):
                obj = object.__new__(cls)
                obj.foo = foo
                return obj
        def pickle_C(obj):
            return (C, (obj.foo,))
        x = C(42)
        self.assertRaises(TypeError, copy.deepcopy, x)
        copy_reg.pickle(C, pickle_C, C)
        y = copy.deepcopy(x)

    def test_deepcopy_reduce_ex(self):
        class C(object):
            def __reduce_ex__(self, proto):
                return ""
            def __reduce__(self):
                raise test_support.TestFailed, "shouldn't call this"
        x = C()
        y = copy.deepcopy(x)
        self.assert_(y is x)

    def test_deepcopy_reduce(self):
        class C(object):
            def __reduce__(self):
                return ""
        x = C()
        y = copy.deepcopy(x)
        self.assert_(y is x)

    def test_deepcopy_cant(self):
        class C(object):
            def __getattribute__(self, name):
                if name.startswith("__reduce"):
                    raise AttributeError, name
                return object.__getattribute__(self, name)
        x = C()
        self.assertRaises(copy.Error, copy.deepcopy, x)

    # Type-specific _deepcopy_xxx() methods

    def test_deepcopy_atomic(self):
        class Classic:
            pass
        class NewStyle(object):
            pass
        def f():
            pass
        tests = [None, 42, 2L**100, 3.14, True, False, 1j,
                 "hello", u"hello\u1234", f.func_code,
                 NewStyle, xrange(10), Classic, max]
        for x in tests:
            self.assert_(copy.deepcopy(x) is x, repr(x))

    def test_deepcopy_list(self):
        x = [[1, 2], 3]
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(x is not y)
        self.assert_(x[0] is not y[0])

    def test_deepcopy_reflexive_list(self):
        x = []
        x.append(x)
        y = copy.deepcopy(x)
        self.assertRaises(RuntimeError, cmp, y, x)
        self.assert_(y is not x)
        self.assert_(y[0] is y)
        self.assertEqual(len(y), 1)

    def test_deepcopy_tuple(self):
        x = ([1, 2], 3)
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(x is not y)
        self.assert_(x[0] is not y[0])

    def test_deepcopy_reflexive_tuple(self):
        x = ([],)
        x[0].append(x)
        y = copy.deepcopy(x)
        self.assertRaises(RuntimeError, cmp, y, x)
        self.assert_(y is not x)
        self.assert_(y[0] is not x[0])
        self.assert_(y[0][0] is y)

    def test_deepcopy_dict(self):
        x = {"foo": [1, 2], "bar": 3}
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(x is not y)
        self.assert_(x["foo"] is not y["foo"])

    def test_deepcopy_reflexive_dict(self):
        x = {}
        x['foo'] = x
        y = copy.deepcopy(x)
        self.assertRaises(RuntimeError, cmp, y, x)
        self.assert_(y is not x)
        self.assert_(y['foo'] is y)
        self.assertEqual(len(y), 1)

    def test_deepcopy_keepalive(self):
        memo = {}
        x = 42
        y = copy.deepcopy(x, memo)
        self.assert_(memo[id(x)] is x)

    def test_deepcopy_inst_vanilla(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(y.foo is not x.foo)

    def test_deepcopy_inst_deepcopy(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __deepcopy__(self, memo):
                return C(copy.deepcopy(self.foo, memo))
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(y is not x)
        self.assert_(y.foo is not x.foo)

    def test_deepcopy_inst_getinitargs(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getinitargs__(self):
                return (self.foo,)
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(y is not x)
        self.assert_(y.foo is not x.foo)

    def test_deepcopy_inst_getstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return {"foo": self.foo}
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(y is not x)
        self.assert_(y.foo is not x.foo)

    def test_deepcopy_inst_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __setstate__(self, state):
                self.foo = state["foo"]
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(y is not x)
        self.assert_(y.foo is not x.foo)

    def test_deepcopy_inst_getstate_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return self.foo
            def __setstate__(self, state):
                self.foo = state
            def __cmp__(self, other):
                return cmp(self.foo, other.foo)
        x = C([42])
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(y is not x)
        self.assert_(y.foo is not x.foo)

    def test_deepcopy_reflexive_inst(self):
        class C:
            pass
        x = C()
        x.foo = x
        y = copy.deepcopy(x)
        self.assert_(y is not x)
        self.assert_(y.foo is y)

    # _reconstruct()

    def test_reconstruct_string(self):
        class C(object):
            def __reduce__(self):
                return ""
        x = C()
        y = copy.copy(x)
        self.assert_(y is x)
        y = copy.deepcopy(x)
        self.assert_(y is x)

    def test_reconstruct_nostate(self):
        class C(object):
            def __reduce__(self):
                return (C, ())
        x = C()
        x.foo = 42
        y = copy.copy(x)
        self.assert_(y.__class__ is x.__class__)
        y = copy.deepcopy(x)
        self.assert_(y.__class__ is x.__class__)

    def test_reconstruct_state(self):
        class C(object):
            def __reduce__(self):
                return (C, (), self.__dict__)
            def __cmp__(self, other):
                return cmp(self.__dict__, other.__dict__)
        x = C()
        x.foo = [42]
        y = copy.copy(x)
        self.assertEqual(y, x)
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(y.foo is not x.foo)

    def test_reconstruct_state_setstate(self):
        class C(object):
            def __reduce__(self):
                return (C, (), self.__dict__)
            def __setstate__(self, state):
                self.__dict__.update(state)
            def __cmp__(self, other):
                return cmp(self.__dict__, other.__dict__)
        x = C()
        x.foo = [42]
        y = copy.copy(x)
        self.assertEqual(y, x)
        y = copy.deepcopy(x)
        self.assertEqual(y, x)
        self.assert_(y.foo is not x.foo)

    def test_reconstruct_reflexive(self):
        class C(object):
            pass
        x = C()
        x.foo = x
        y = copy.deepcopy(x)
        self.assert_(y is not x)
        self.assert_(y.foo is y)

    # Additions for Python 2.3 and pickle protocol 2

    def test_reduce_4tuple(self):
        class C(list):
            def __reduce__(self):
                return (C, (), self.__dict__, iter(self))
            def __cmp__(self, other):
                return (cmp(list(self), list(other)) or
                        cmp(self.__dict__, other.__dict__))
        x = C([[1, 2], 3])
        y = copy.copy(x)
        self.assertEqual(x, y)
        self.assert_(x is not y)
        self.assert_(x[0] is y[0])
        y = copy.deepcopy(x)
        self.assertEqual(x, y)
        self.assert_(x is not y)
        self.assert_(x[0] is not y[0])

    def test_reduce_5tuple(self):
        class C(dict):
            def __reduce__(self):
                return (C, (), self.__dict__, None, self.iteritems())
            def __cmp__(self, other):
                return (cmp(dict(self), list(dict)) or
                        cmp(self.__dict__, other.__dict__))
        x = C([("foo", [1, 2]), ("bar", 3)])
        y = copy.copy(x)
        self.assertEqual(x, y)
        self.assert_(x is not y)
        self.assert_(x["foo"] is y["foo"])
        y = copy.deepcopy(x)
        self.assertEqual(x, y)
        self.assert_(x is not y)
        self.assert_(x["foo"] is not y["foo"])

    def test_copy_slots(self):
        class C(object):
            __slots__ = ["foo"]
        x = C()
        x.foo = [42]
        y = copy.copy(x)
        self.assert_(x.foo is y.foo)

    def test_deepcopy_slots(self):
        class C(object):
            __slots__ = ["foo"]
        x = C()
        x.foo = [42]
        y = copy.deepcopy(x)
        self.assertEqual(x.foo, y.foo)
        self.assert_(x.foo is not y.foo)

    def test_copy_list_subclass(self):
        class C(list):
            pass
        x = C([[1, 2], 3])
        x.foo = [4, 5]
        y = copy.copy(x)
        self.assertEqual(list(x), list(y))
        self.assertEqual(x.foo, y.foo)
        self.assert_(x[0] is y[0])
        self.assert_(x.foo is y.foo)

    def test_deepcopy_list_subclass(self):
        class C(list):
            pass
        x = C([[1, 2], 3])
        x.foo = [4, 5]
        y = copy.deepcopy(x)
        self.assertEqual(list(x), list(y))
        self.assertEqual(x.foo, y.foo)
        self.assert_(x[0] is not y[0])
        self.assert_(x.foo is not y.foo)

    def test_copy_tuple_subclass(self):
        class C(tuple):
            pass
        x = C([1, 2, 3])
        self.assertEqual(tuple(x), (1, 2, 3))
        y = copy.copy(x)
        self.assertEqual(tuple(y), (1, 2, 3))

    def test_deepcopy_tuple_subclass(self):
        class C(tuple):
            pass
        x = C([[1, 2], 3])
        self.assertEqual(tuple(x), ([1, 2], 3))
        y = copy.deepcopy(x)
        self.assertEqual(tuple(y), ([1, 2], 3))
        self.assert_(x is not y)
        self.assert_(x[0] is not y[0])

    def test_getstate_exc(self):
        class EvilState(object):
            def __getstate__(self):
                raise ValueError, "ain't got no stickin' state"
        self.assertRaises(ValueError, copy.copy, EvilState())

def test_main():
    test_support.run_unittest(TestCopy)

if __name__ == "__main__":
    test_main()
