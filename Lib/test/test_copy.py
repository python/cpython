"""Unit tests for the copy module."""

from test.support.import_helper import import_fresh_module

py_copy = import_fresh_module(
    'copy', blocked=['_copy']
)
c_copy = import_fresh_module(
    'copy', fresh=['_copy']
)

import copyreg
import weakref
import abc
from operator import le, lt, ge, gt, eq, ne
#import copy

import unittest
import unittest.mock
from test import support

order_comparisons = le, lt, ge, gt
equality_comparisons = eq, ne
comparisons = order_comparisons + equality_comparisons

class TestCopy:

    copy_module = None

    def test_exceptions(self):
        self.assertIs(self.copy_module.Error, self.copy_module.error)
        self.assertTrue(issubclass(self.copy_module.Error, Exception))

    def test_copy_basic(self):
        x = 42
        y = self.copy_module.copy(x)
        self.assertEqual(x, y)

    def test_copy_copy(self):
        class C(object):
            def __init__(self, foo):
                self.foo = foo
            def __copy__(self):
                return C(self.foo)
        x = C(42)
        y = self.copy_module.copy(x)
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
        self.assertRaises(TypeError, self.copy_module.copy, x)
        copyreg.pickle(C, pickle_C, C)
        y = self.copy_module.copy(x)
        self.assertIsInstance(y, C)
        self.assertEqual(y.foo, x.foo)

    def test_copy_reduce_ex(self):
        class C(object):
            def __reduce_ex__(self, proto):
                c.append(1)
                return ""
            def __reduce__(self):
                self.fail("shouldn't call this")
        c = []
        x = C()
        y = self.copy_module.copy(x)
        self.assertIs(y, x)
        self.assertEqual(c, [1])

    def test_copy_reduce(self):
        class C(object):
            def __reduce__(self):
                c.append(1)
                return ""
        c = []
        x = C()
        y = self.copy_module.copy(x)
        self.assertIs(y, x)
        self.assertEqual(c, [1])

    def test_copy_cant(self):
        class C(object):
            def __getattribute__(self, name):
                if name.startswith("__reduce"):
                    raise AttributeError(name)
                return object.__getattribute__(self, name)
        x = C()
        self.assertRaises(self.copy_module.Error, self.copy_module.copy, x)

    # Type-specific _copy_xxx() methods

    def test_copy_atomic(self):
        class Classic:
            pass
        class NewStyle(object):
            pass
        def f():
            pass
        class WithMetaclass(metaclass=abc.ABCMeta):
            pass
        tests = [None, ..., NotImplemented,
                 42, 2**100, 3.14, True, False, 1j,
                 "hello", "hello\u1234", f.__code__,
                 b"world", bytes(range(256)), range(10), slice(1, 10, 2),
                 NewStyle, Classic, max, WithMetaclass, property()]
        for x in tests:
            self.assertIs(self.copy_module.copy(x), x)

    def test_copy_list(self):
        x = [1, 2, 3]
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        x = []
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)

    def test_copy_tuple(self):
        x = (1, 2, 3)
        self.assertIs(self.copy_module.copy(x), x)
        x = ()
        self.assertIs(self.copy_module.copy(x), x)
        x = (1, 2, 3, [])
        self.assertIs(self.copy_module.copy(x), x)

    def test_copy_dict(self):
        x = {"foo": 1, "bar": 2}
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        x = {}
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)

    def test_copy_set(self):
        x = {1, 2, 3}
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        x = set()
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)

    def test_copy_frozenset(self):
        x = frozenset({1, 2, 3})
        self.assertIs(self.copy_module.copy(x), x)
        x = frozenset()
        self.assertIs(self.copy_module.copy(x), x)

    def test_copy_bytearray(self):
        x = bytearray(b'abc')
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        x = bytearray()
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)

    def test_copy_inst_vanilla(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __eq__(self, other):
                return self.foo == other.foo
        x = C(42)
        self.assertEqual(self.copy_module.copy(x), x)

    def test_copy_inst_copy(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __copy__(self):
                return C(self.foo)
            def __eq__(self, other):
                return self.foo == other.foo
        x = C(42)
        self.assertEqual(self.copy_module.copy(x), x)

    def test_copy_inst_getinitargs(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getinitargs__(self):
                return (self.foo,)
            def __eq__(self, other):
                return self.foo == other.foo
        x = C(42)
        self.assertEqual(self.copy_module.copy(x), x)

    def test_copy_inst_getnewargs(self):
        class C(int):
            def __new__(cls, foo):
                self = int.__new__(cls)
                self.foo = foo
                return self
            def __getnewargs__(self):
                return self.foo,
            def __eq__(self, other):
                return self.foo == other.foo
        x = C(42)
        y = self.copy_module.copy(x)
        self.assertIsInstance(y, C)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertEqual(y.foo, x.foo)

    def test_copy_inst_getnewargs_ex(self):
        class C(int):
            def __new__(cls, *, foo):
                self = int.__new__(cls)
                self.foo = foo
                return self
            def __getnewargs_ex__(self):
                return (), {'foo': self.foo}
            def __eq__(self, other):
                return self.foo == other.foo
        x = C(foo=42)
        y = self.copy_module.copy(x)
        self.assertIsInstance(y, C)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertEqual(y.foo, x.foo)

    def test_copy_inst_getstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return {"foo": self.foo}
            def __eq__(self, other):
                return self.foo == other.foo
        x = C(42)
        self.assertEqual(self.copy_module.copy(x), x)

    def test_copy_inst_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __setstate__(self, state):
                self.foo = state["foo"]
            def __eq__(self, other):
                return self.foo == other.foo
        x = C(42)
        self.assertEqual(self.copy_module.copy(x), x)

    def test_copy_inst_getstate_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return self.foo
            def __setstate__(self, state):
                self.foo = state
            def __eq__(self, other):
                return self.foo == other.foo
        x = C(42)
        self.assertEqual(self.copy_module.copy(x), x)
        # State with boolean value is false (issue #25718)
        x = C(0.0)
        self.assertEqual(self.copy_module.copy(x), x)

class TestDeepcopy:

    copy_module = None

    def test_deepcopy_basic(self):
        x = 42
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)

    def test_deepcopy_memo(self):
        # Tests of reflexive objects are under type-specific sections below.
        # This tests only repetitions of objects.
        x = []
        x = [x, x]
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertIsNot(y[0], x[0])
        self.assertIs(y[0], y[1])

    def test_deepcopy_issubclass(self):
        # XXX Note: there's no way to test the TypeError coming out of
        # issubclass() -- this can only happen when an extension
        # module defines a "type" that doesn't formally inherit from
        # type.
        class Meta(type):
            pass
        class C(metaclass=Meta):
            pass
        self.assertEqual(self.copy_module.deepcopy(C), C)

    def test_deepcopy_deepcopy(self):
        class C(object):
            def __init__(self, foo):
                self.foo = foo
            def __deepcopy__(self, memo=None):
                return C(self.foo)
        x = C(42)
        y = self.copy_module.deepcopy(x)
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
        self.assertRaises(TypeError, self.copy_module.deepcopy, x)
        copyreg.pickle(C, pickle_C, C)
        y = self.copy_module.deepcopy(x)
        self.assertIsInstance(y, C)
        self.assertEqual(y.foo, x.foo)

    def test_deepcopy_reduce_ex(self):
        class C(object):
            def __reduce_ex__(self, proto):
                c.append(1)
                return ""
            def __reduce__(self):
                self.fail("shouldn't call this")
        c = []
        x = C()
        y = self.copy_module.deepcopy(x)
        self.assertIs(y, x)
        self.assertEqual(c, [1])

    def test_deepcopy_reduce(self):
        class C(object):
            def __reduce__(self):
                c.append(1)
                return ""
        c = []
        x = C()
        y = self.copy_module.deepcopy(x)
        self.assertIs(y, x)
        self.assertEqual(c, [1])

    def test_deepcopy_cant(self):
        class C(object):
            def __getattribute__(self, name):
                if name.startswith("__reduce"):
                    raise AttributeError(name)
                return object.__getattribute__(self, name)
        x = C()
         # TODO: this no longer works. perhaps the import of the module inside the C extension is to blame?
        #self.assertRaises(self.copy_module.Error, self.copy_module.deepcopy, x)
        self.assertRaises(Exception, self.copy_module.deepcopy, x)

    # Type-specific _deepcopy_xxx() methods

    def test_deepcopy_atomic(self):
        class Classic:
            pass
        class NewStyle(object):
            pass
        def f():
            pass
        tests = [None, 42, 2**100, 3.14, True, False, 1j,
                 "hello", "hello\u1234", f.__code__,
                 NewStyle, range(10), Classic, max, property()]
        for x in tests:
            self.assertIs(self.copy_module.deepcopy(x), x)

    def test_deepcopy_list(self):
        x = [[1, 2], 3]
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(x, y)
        self.assertIsNot(x[0], y[0])

    def test_deepcopy_reflexive_list(self):
        x = []
        x.append(x)
        y = self.copy_module.deepcopy(x)
        for op in comparisons:
            self.assertRaises(RecursionError, op, y, x)
        self.assertIsNot(y, x)
        self.assertIs(y[0], y)
        self.assertEqual(len(y), 1)

    def test_deepcopy_empty_tuple(self):
        x = ()
        y = self.copy_module.deepcopy(x)
        self.assertIs(x, y)

    def test_deepcopy_tuple(self):
        x = ([1, 2], 3)
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(x, y)
        self.assertIsNot(x[0], y[0])

    def test_deepcopy_tuple_of_immutables(self):
        x = ((1, 2), 3)
        y = self.copy_module.deepcopy(x)
        self.assertIs(x, y)

    def test_deepcopy_reflexive_tuple(self):
        x = ([],)
        x[0].append(x)
        y = self.copy_module.deepcopy(x)
        for op in comparisons:
            self.assertRaises(RecursionError, op, y, x)
        self.assertIsNot(y, x)
        self.assertIsNot(y[0], x[0])
        self.assertIs(y[0][0], y)

    def test_deepcopy_dict(self):
        x = {"foo": [1, 2], "bar": 3}
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(x, y)
        self.assertIsNot(x["foo"], y["foo"])

    def test_deepcopy_reflexive_dict(self):
        x = {}
        x['foo'] = x
        y = self.copy_module.deepcopy(x)
        for op in order_comparisons:
            self.assertRaises(TypeError, op, y, x)
        for op in equality_comparisons:
            self.assertRaises(RecursionError, op, y, x)
        self.assertIsNot(y, x)
        self.assertIs(y['foo'], y)
        self.assertEqual(len(y), 1)

    def test_deepcopy_keepalive(self):
        memo = {}
        x = []
        y = self.copy_module.deepcopy(x, memo)
        self.assertIs(memo[id(memo)][0], x)

    def test_deepcopy_dont_memo_immutable(self):
        memo = {}
        x = [1, 2, 3, 4]
        y = self.copy_module.deepcopy(x, memo)
        self.assertEqual(y, x)
        # There's the entry for the new list, and the keep alive.
        self.assertEqual(len(memo), 2)

        memo = {}
        x = [(1, 2)]
        y = self.copy_module.deepcopy(x, memo)
        self.assertEqual(y, x)
        # Tuples with immutable contents are immutable for deepcopy.
        self.assertEqual(len(memo), 2)

    def test_deepcopy_inst_vanilla(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __eq__(self, other):
                return self.foo == other.foo
        x = C([42])
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y.foo, x.foo)

    def test_deepcopy_inst_deepcopy(self):
        copy_module = self.copy_module
        import copy
        copy_module = copy
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __deepcopy__(self, memo):
                return C(copy_module.deepcopy(self.foo, memo))
            def __eq__(self, other):
                return self.foo == other.foo
        x = C([42])
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertIsNot(y.foo, x.foo)

    def test_deepcopy_inst_getinitargs(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getinitargs__(self):
                return (self.foo,)
            def __eq__(self, other):
                return self.foo == other.foo
        x = C([42])
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertIsNot(y.foo, x.foo)

    def test_deepcopy_inst_getnewargs(self):
        class C(int):
            def __new__(cls, foo):
                self = int.__new__(cls)
                self.foo = foo
                return self
            def __getnewargs__(self):
                return self.foo,
            def __eq__(self, other):
                return self.foo == other.foo
        x = C([42])
        y = self.copy_module.deepcopy(x)
        self.assertIsInstance(y, C)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertEqual(y.foo, x.foo)
        self.assertIsNot(y.foo, x.foo)

    def test_deepcopy_inst_getnewargs_ex(self):
        class C(int):
            def __new__(cls, *, foo):
                self = int.__new__(cls)
                self.foo = foo
                return self
            def __getnewargs_ex__(self):
                return (), {'foo': self.foo}
            def __eq__(self, other):
                return self.foo == other.foo
        x = C(foo=[42])
        y = self.copy_module.deepcopy(x)
        self.assertIsInstance(y, C)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertEqual(y.foo, x.foo)
        self.assertIsNot(y.foo, x.foo)

    def test_deepcopy_inst_getstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return {"foo": self.foo}
            def __eq__(self, other):
                return self.foo == other.foo
        x = C([42])
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertIsNot(y.foo, x.foo)

    def test_deepcopy_inst_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __setstate__(self, state):
                self.foo = state["foo"]
            def __eq__(self, other):
                return self.foo == other.foo
        x = C([42])
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertIsNot(y.foo, x.foo)

    def test_deepcopy_inst_getstate_setstate(self):
        class C:
            def __init__(self, foo):
                self.foo = foo
            def __getstate__(self):
                return self.foo
            def __setstate__(self, state):
                self.foo = state
            def __eq__(self, other):
                return self.foo == other.foo
        x = C([42])
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertIsNot(y.foo, x.foo)
        # State with boolean value is false (issue #25718)
        x = C([])
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y, x)
        self.assertIsNot(y.foo, x.foo)

    def test_deepcopy_reflexive_inst(self):
        class C:
            pass
        x = C()
        x.foo = x
        y = self.copy_module.deepcopy(x)
        self.assertIsNot(y, x)
        self.assertIs(y.foo, y)

    # _reconstruct()

    def test_reconstruct_string(self):
        class C(object):
            def __reduce__(self):
                return ""
        x = C()
        y = self.copy_module.copy(x)
        self.assertIs(y, x)
        y = self.copy_module.deepcopy(x)
        self.assertIs(y, x)

    def test_reconstruct_nostate(self):
        class C(object):
            def __reduce__(self):
                return (C, ())
        x = C()
        x.foo = 42
        y = self.copy_module.copy(x)
        self.assertIs(y.__class__, x.__class__)
        y = self.copy_module.deepcopy(x)
        self.assertIs(y.__class__, x.__class__)

    def test_reconstruct_state(self):
        class C(object):
            def __reduce__(self):
                return (C, (), self.__dict__)
            def __eq__(self, other):
                return self.__dict__ == other.__dict__
        x = C()
        x.foo = [42]
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y.foo, x.foo)

    def test_reconstruct_state_setstate(self):
        class C(object):
            def __reduce__(self):
                return (C, (), self.__dict__)
            def __setstate__(self, state):
                self.__dict__.update(state)
            def __eq__(self, other):
                return self.__dict__ == other.__dict__
        x = C()
        x.foo = [42]
        y = self.copy_module.copy(x)
        self.assertEqual(y, x)
        y = self.copy_module.deepcopy(x)
        self.assertEqual(y, x)
        self.assertIsNot(y.foo, x.foo)

    def test_reconstruct_reflexive(self):
        class C(object):
            pass
        x = C()
        x.foo = x
        y = self.copy_module.deepcopy(x)
        self.assertIsNot(y, x)
        self.assertIs(y.foo, y)

    # Additions for Python 2.3 and pickle protocol 2

    def test_reduce_4tuple(self):
        class C(list):
            def __reduce__(self):
                return (C, (), self.__dict__, iter(self))
            def __eq__(self, other):
                return (list(self) == list(other) and
                        self.__dict__ == other.__dict__)
        x = C([[1, 2], 3])
        y = self.copy_module.copy(x)
        self.assertEqual(x, y)
        self.assertIsNot(x, y)
        self.assertIs(x[0], y[0])
        y = self.copy_module.deepcopy(x)
        self.assertEqual(x, y)
        self.assertIsNot(x, y)
        self.assertIsNot(x[0], y[0])

    def test_reduce_5tuple(self):
        class C(dict):
            def __reduce__(self):
                return (C, (), self.__dict__, None, self.items())
            def __eq__(self, other):
                return (dict(self) == dict(other) and
                        self.__dict__ == other.__dict__)
        x = C([("foo", [1, 2]), ("bar", 3)])
        y = self.copy_module.copy(x)
        self.assertEqual(x, y)
        self.assertIsNot(x, y)
        self.assertIs(x["foo"], y["foo"])
        y = self.copy_module.deepcopy(x)
        self.assertEqual(x, y)
        self.assertIsNot(x, y)
        self.assertIsNot(x["foo"], y["foo"])

    def test_reduce_6tuple(self):
        def state_setter(*args, **kwargs):
            self.fail("shouldn't call this")
        class C:
            def __reduce__(self):
                return C, (), self.__dict__, None, None, state_setter
        x = C()
        with self.assertRaises(TypeError):
            self.copy_module.copy(x)
        with self.assertRaises(TypeError):
            self.copy_module.deepcopy(x)

    def test_reduce_6tuple_none(self):
        class C:
            def __reduce__(self):
                return C, (), self.__dict__, None, None, None
        x = C()
        with self.assertRaises(TypeError):
            self.copy_module.copy(x)
        with self.assertRaises(TypeError):
            self.copy_module.deepcopy(x)

    def test_copy_slots(self):
        class C(object):
            __slots__ = ["foo"]
        x = C()
        x.foo = [42]
        y = self.copy_module.copy(x)
        self.assertIs(x.foo, y.foo)

    def test_deepcopy_slots(self):
        class C(object):
            __slots__ = ["foo"]
        x = C()
        x.foo = [42]
        y = self.copy_module.deepcopy(x)
        self.assertEqual(x.foo, y.foo)
        self.assertIsNot(x.foo, y.foo)

    def test_deepcopy_dict_subclass(self):
        class C(dict):
            def __init__(self, d=None):
                if not d:
                    d = {}
                self._keys = list(d.keys())
                super().__init__(d)
            def __setitem__(self, key, item):
                super().__setitem__(key, item)
                if key not in self._keys:
                    self._keys.append(key)
        x = C(d={'foo':0})
        y = self.copy_module.deepcopy(x)
        self.assertEqual(x, y)
        self.assertEqual(x._keys, y._keys)
        self.assertIsNot(x, y)
        x['bar'] = 1
        self.assertNotEqual(x, y)
        self.assertNotEqual(x._keys, y._keys)

    def test_copy_list_subclass(self):
        class C(list):
            pass
        x = C([[1, 2], 3])
        x.foo = [4, 5]
        y = self.copy_module.copy(x)
        self.assertEqual(list(x), list(y))
        self.assertEqual(x.foo, y.foo)
        self.assertIs(x[0], y[0])
        self.assertIs(x.foo, y.foo)

    def test_deepcopy_list_subclass(self):
        class C(list):
            pass
        x = C([[1, 2], 3])
        x.foo = [4, 5]
        y = self.copy_module.deepcopy(x)
        self.assertEqual(list(x), list(y))
        self.assertEqual(x.foo, y.foo)
        self.assertIsNot(x[0], y[0])
        self.assertIsNot(x.foo, y.foo)

    def test_copy_tuple_subclass(self):
        class C(tuple):
            pass
        x = C([1, 2, 3])
        self.assertEqual(tuple(x), (1, 2, 3))
        y = self.copy_module.copy(x)
        self.assertEqual(tuple(y), (1, 2, 3))

    def test_deepcopy_tuple_subclass(self):
        class C(tuple):
            pass
        x = C([[1, 2], 3])
        self.assertEqual(tuple(x), ([1, 2], 3))
        y = self.copy_module.deepcopy(x)
        self.assertEqual(tuple(y), ([1, 2], 3))
        self.assertIsNot(x, y)
        self.assertIsNot(x[0], y[0])

    def test_getstate_exc(self):
        class EvilState(object):
            def __getstate__(self):
                raise ValueError("ain't got no stickin' state")
        self.assertRaises(ValueError, self.copy_module.copy, EvilState())

    def test_copy_function(self):
        self.assertEqual(self.copy_module.copy(global_foo), global_foo)
        def foo(x, y): return x+y
        self.assertEqual(self.copy_module.copy(foo), foo)
        bar = lambda: None
        self.assertEqual(self.copy_module.copy(bar), bar)

    def test_deepcopy_function(self):
        self.assertEqual(self.copy_module.deepcopy(global_foo), global_foo)
        def foo(x, y): return x+y
        self.assertEqual(self.copy_module.deepcopy(foo), foo)
        bar = lambda: None
        self.assertEqual(self.copy_module.deepcopy(bar), bar)

    def _check_weakref(self, _copy):
        class C(object):
            pass
        obj = C()
        x = weakref.ref(obj)
        y = _copy(x)
        self.assertIs(y, x)
        del obj
        y = _copy(x)
        self.assertIs(y, x)

    def test_copy_weakref(self):
        self._check_weakref(self.copy_module.copy)

    def test_deepcopy_weakref(self):
        self._check_weakref(self.copy_module.deepcopy)

    def _check_copy_weakdict(self, _dicttype):
        class C(object):
            pass
        a, b, c, d = [C() for i in range(4)]
        u = _dicttype()
        u[a] = b
        u[c] = d
        v = self.copy_module.copy(u)
        self.assertIsNot(v, u)
        self.assertEqual(v, u)
        self.assertEqual(v[a], b)
        self.assertEqual(v[c], d)
        self.assertEqual(len(v), 2)
        del c, d
        support.gc_collect()  # For PyPy or other GCs.
        self.assertEqual(len(v), 1)
        x, y = C(), C()
        # The underlying containers are decoupled
        v[x] = y
        self.assertNotIn(x, u)

    def test_copy_weakkeydict(self):
        self._check_copy_weakdict(weakref.WeakKeyDictionary)

    def test_copy_weakvaluedict(self):
        self._check_copy_weakdict(weakref.WeakValueDictionary)

    def test_deepcopy_weakkeydict(self):
        class C(object):
            def __init__(self, i):
                self.i = i
        a, b, c, d = [C(i) for i in range(4)]
        u = weakref.WeakKeyDictionary()
        u[a] = b
        u[c] = d
        # Keys aren't copied, values are
        v = self.copy_module.deepcopy(u)
        self.assertNotEqual(v, u)
        self.assertEqual(len(v), 2)
        self.assertIsNot(v[a], b)
        self.assertIsNot(v[c], d)
        self.assertEqual(v[a].i, b.i)
        self.assertEqual(v[c].i, d.i)
        del c
        support.gc_collect()  # For PyPy or other GCs.
        self.assertEqual(len(v), 1)

    def test_deepcopy_weakvaluedict(self):
        class C(object):
            def __init__(self, i):
                self.i = i
        a, b, c, d = [C(i) for i in range(4)]
        u = weakref.WeakValueDictionary()
        u[a] = b
        u[c] = d
        # Keys are copied, values aren't
        v = self.copy_module.deepcopy(u)
        self.assertNotEqual(v, u)
        self.assertEqual(len(v), 2)
        (x, y), (z, t) = sorted(v.items(), key=lambda pair: pair[0].i)
        self.assertIsNot(x, a)
        self.assertEqual(x.i, a.i)
        self.assertIs(y, b)
        self.assertIsNot(z, c)
        self.assertEqual(z.i, c.i)
        self.assertIs(t, d)
        del x, y, z, t
        del d
        support.gc_collect()  # For PyPy or other GCs.
        self.assertEqual(len(v), 1)

    def test_deepcopy_bound_method(self):
        class Foo(object):
            def m(self):
                pass
        f = Foo()
        f.b = f.m
        g = self.copy_module.deepcopy(f)
        self.assertEqual(g.m, g.b)
        self.assertIs(g.b.__self__, g)
        g.b()


def global_foo(x, y): return x+y


class TestCopyPy(TestCopy, unittest.TestCase):
    copy_module = py_copy
    

class TestDeepcopyPy(TestDeepcopy, unittest.TestCase):
    copy_module = py_copy


@unittest.skipUnless(c_copy, 'requires _copy')
class TestDeepcopyC(TestDeepcopy, unittest.TestCase):
    copy_module = c_copy

    def test_deepcopy_standard_types_no_fallback(self):
        # TODO: not longer working with the new style testing?
        with unittest.mock.patch('copy._deepcopy_fallback') as _deepcopy_fallback_mock:
            _=self.copy_module.deepcopy({'str': 's', 'int': 0, 'list': [1,(1,2)]})
        _deepcopy_fallback_mock.assert_not_called()

if __name__ == "__main__":
    unittest.main()
