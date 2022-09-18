import unittest
from test.support import cpython_only, requires_limited_api
try:
    import _testcapi
except ImportError:
    _testcapi = None
import struct
import collections
import itertools
import gc
import contextlib
import sys


class BadStr(str):
    def __eq__(self, other):
        return True
    def __hash__(self):
        # Guaranteed different hash
        return str.__hash__(self) ^ 3


class FunctionCalls(unittest.TestCase):

    def test_kwargs_order(self):
        # bpo-34320:  **kwargs should preserve order of passed OrderedDict
        od = collections.OrderedDict([('a', 1), ('b', 2)])
        od.move_to_end('a')
        expected = list(od.items())

        def fn(**kw):
            return kw

        res = fn(**od)
        self.assertIsInstance(res, dict)
        self.assertEqual(list(res.items()), expected)

    def test_frames_are_popped_after_failed_calls(self):
        # GH-93252: stuff blows up if we don't pop the new frame after
        # recovering from failed calls:
        def f():
            pass
        for _ in range(1000):
            try:
                f(None)
            except TypeError:
                pass
        # BOOM!


@cpython_only
class CFunctionCallsErrorMessages(unittest.TestCase):

    def test_varargs0(self):
        msg = r"__contains__\(\) takes exactly one argument \(0 given\)"
        self.assertRaisesRegex(TypeError, msg, {}.__contains__)

    def test_varargs2(self):
        msg = r"__contains__\(\) takes exactly one argument \(2 given\)"
        self.assertRaisesRegex(TypeError, msg, {}.__contains__, 0, 1)

    def test_varargs3(self):
        msg = r"^from_bytes\(\) takes at most 2 positional arguments \(3 given\)"
        self.assertRaisesRegex(TypeError, msg, int.from_bytes, b'a', 'little', False)

    def test_varargs1min(self):
        msg = r"get expected at least 1 argument, got 0"
        self.assertRaisesRegex(TypeError, msg, {}.get)

        msg = r"expected 1 argument, got 0"
        self.assertRaisesRegex(TypeError, msg, {}.__delattr__)

    def test_varargs2min(self):
        msg = r"getattr expected at least 2 arguments, got 0"
        self.assertRaisesRegex(TypeError, msg, getattr)

    def test_varargs1max(self):
        msg = r"input expected at most 1 argument, got 2"
        self.assertRaisesRegex(TypeError, msg, input, 1, 2)

    def test_varargs2max(self):
        msg = r"get expected at most 2 arguments, got 3"
        self.assertRaisesRegex(TypeError, msg, {}.get, 1, 2, 3)

    def test_varargs1_kw(self):
        msg = r"__contains__\(\) takes no keyword arguments"
        self.assertRaisesRegex(TypeError, msg, {}.__contains__, x=2)

    def test_varargs2_kw(self):
        msg = r"__contains__\(\) takes no keyword arguments"
        self.assertRaisesRegex(TypeError, msg, {}.__contains__, x=2, y=2)

    def test_varargs3_kw(self):
        msg = r"bool\(\) takes no keyword arguments"
        self.assertRaisesRegex(TypeError, msg, bool, x=2)

    def test_varargs4_kw(self):
        msg = r"^list[.]index\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, [].index, x=2)

    def test_varargs5_kw(self):
        msg = r"^hasattr\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, hasattr, x=2)

    def test_varargs6_kw(self):
        msg = r"^getattr\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, getattr, x=2)

    def test_varargs7_kw(self):
        msg = r"^next\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, next, x=2)

    def test_varargs8_kw(self):
        msg = r"^_struct[.]pack\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, struct.pack, x=2)

    def test_varargs9_kw(self):
        msg = r"^_struct[.]pack_into\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, struct.pack_into, x=2)

    def test_varargs10_kw(self):
        msg = r"^deque[.]index\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, collections.deque().index, x=2)

    def test_varargs11_kw(self):
        msg = r"^Struct[.]pack\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, struct.Struct.pack, struct.Struct(""), x=2)

    def test_varargs12_kw(self):
        msg = r"^staticmethod\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, staticmethod, func=id)

    def test_varargs13_kw(self):
        msg = r"^classmethod\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, classmethod, func=id)

    def test_varargs14_kw(self):
        msg = r"^product\(\) takes at most 1 keyword argument \(2 given\)$"
        self.assertRaisesRegex(TypeError, msg,
                               itertools.product, 0, repeat=1, foo=2)

    def test_varargs15_kw(self):
        msg = r"^ImportError\(\) takes at most 2 keyword arguments \(3 given\)$"
        self.assertRaisesRegex(TypeError, msg,
                               ImportError, 0, name=1, path=2, foo=3)

    def test_varargs16_kw(self):
        msg = r"^min\(\) takes at most 2 keyword arguments \(3 given\)$"
        self.assertRaisesRegex(TypeError, msg,
                               min, 0, default=1, key=2, foo=3)

    def test_varargs17_kw(self):
        msg = r"'foo' is an invalid keyword argument for print\(\)$"
        self.assertRaisesRegex(TypeError, msg,
                               print, 0, sep=1, end=2, file=3, flush=4, foo=5)

    def test_varargs18_kw(self):
        # _PyArg_UnpackKeywordsWithVararg()
        msg = r"invalid keyword argument for print\(\)$"
        with self.assertRaisesRegex(TypeError, msg):
            print(0, 1, **{BadStr('foo'): ','})

    def test_varargs19_kw(self):
        # _PyArg_UnpackKeywords()
        msg = r"invalid keyword argument for round\(\)$"
        with self.assertRaisesRegex(TypeError, msg):
            round(1.75, **{BadStr('foo'): 1})

    def test_oldargs0_1(self):
        msg = r"keys\(\) takes no arguments \(1 given\)"
        self.assertRaisesRegex(TypeError, msg, {}.keys, 0)

    def test_oldargs0_2(self):
        msg = r"keys\(\) takes no arguments \(2 given\)"
        self.assertRaisesRegex(TypeError, msg, {}.keys, 0, 1)

    def test_oldargs0_1_kw(self):
        msg = r"keys\(\) takes no keyword arguments"
        self.assertRaisesRegex(TypeError, msg, {}.keys, x=2)

    def test_oldargs0_2_kw(self):
        msg = r"keys\(\) takes no keyword arguments"
        self.assertRaisesRegex(TypeError, msg, {}.keys, x=2, y=2)

    def test_oldargs1_0(self):
        msg = r"count\(\) takes exactly one argument \(0 given\)"
        self.assertRaisesRegex(TypeError, msg, [].count)

    def test_oldargs1_2(self):
        msg = r"count\(\) takes exactly one argument \(2 given\)"
        self.assertRaisesRegex(TypeError, msg, [].count, 1, 2)

    def test_oldargs1_0_kw(self):
        msg = r"count\(\) takes no keyword arguments"
        self.assertRaisesRegex(TypeError, msg, [].count, x=2)

    def test_oldargs1_1_kw(self):
        msg = r"count\(\) takes no keyword arguments"
        self.assertRaisesRegex(TypeError, msg, [].count, {}, x=2)

    def test_oldargs1_2_kw(self):
        msg = r"count\(\) takes no keyword arguments"
        self.assertRaisesRegex(TypeError, msg, [].count, x=2, y=2)



class TestCallingConventions(unittest.TestCase):
    """Test calling using various C calling conventions (METH_*) from Python

    Subclasses test several kinds of functions (module-level, methods,
    class methods static methods) using these attributes:
      obj: the object that contains tested functions (as attributes)
      expected_self: expected "self" argument to the C function

    The base class tests module-level functions.
    """

    def setUp(self):
        self.obj = self.expected_self = _testcapi

    def test_varargs(self):
        self.assertEqual(
            self.obj.meth_varargs(1, 2, 3),
            (self.expected_self, (1, 2, 3)),
        )

    def test_varargs_ext(self):
        self.assertEqual(
            self.obj.meth_varargs(*(1, 2, 3)),
            (self.expected_self, (1, 2, 3)),
        )

    def test_varargs_error_kw(self):
        msg = r"meth_varargs\(\) takes no keyword arguments"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_varargs(k=1),
        )

    def test_varargs_keywords(self):
        self.assertEqual(
            self.obj.meth_varargs_keywords(1, 2, a=3, b=4),
            (self.expected_self, (1, 2), {'a': 3, 'b': 4})
        )

    def test_varargs_keywords_ext(self):
        self.assertEqual(
            self.obj.meth_varargs_keywords(*[1, 2], **{'a': 3, 'b': 4}),
            (self.expected_self, (1, 2), {'a': 3, 'b': 4})
        )

    def test_o(self):
        self.assertEqual(self.obj.meth_o(1), (self.expected_self, 1))

    def test_o_ext(self):
        self.assertEqual(self.obj.meth_o(*[1]), (self.expected_self, 1))

    def test_o_error_no_arg(self):
        msg = r"meth_o\(\) takes exactly one argument \(0 given\)"
        self.assertRaisesRegex(TypeError, msg, self.obj.meth_o)

    def test_o_error_two_args(self):
        msg = r"meth_o\(\) takes exactly one argument \(2 given\)"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_o(1, 2),
        )

    def test_o_error_ext(self):
        msg = r"meth_o\(\) takes exactly one argument \(3 given\)"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_o(*(1, 2, 3)),
        )

    def test_o_error_kw(self):
        msg = r"meth_o\(\) takes no keyword arguments"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_o(k=1),
        )

    def test_o_error_arg_kw(self):
        msg = r"meth_o\(\) takes no keyword arguments"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_o(k=1),
        )

    def test_noargs(self):
        self.assertEqual(self.obj.meth_noargs(), self.expected_self)

    def test_noargs_ext(self):
        self.assertEqual(self.obj.meth_noargs(*[]), self.expected_self)

    def test_noargs_error_arg(self):
        msg = r"meth_noargs\(\) takes no arguments \(1 given\)"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_noargs(1),
        )

    def test_noargs_error_arg2(self):
        msg = r"meth_noargs\(\) takes no arguments \(2 given\)"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_noargs(1, 2),
        )

    def test_noargs_error_ext(self):
        msg = r"meth_noargs\(\) takes no arguments \(3 given\)"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_noargs(*(1, 2, 3)),
        )

    def test_noargs_error_kw(self):
        msg = r"meth_noargs\(\) takes no keyword arguments"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_noargs(k=1),
        )

    def test_fastcall(self):
        self.assertEqual(
            self.obj.meth_fastcall(1, 2, 3),
            (self.expected_self, (1, 2, 3)),
        )

    def test_fastcall_ext(self):
        self.assertEqual(
            self.obj.meth_fastcall(*(1, 2, 3)),
            (self.expected_self, (1, 2, 3)),
        )

    def test_fastcall_error_kw(self):
        msg = r"meth_fastcall\(\) takes no keyword arguments"
        self.assertRaisesRegex(
            TypeError, msg, lambda: self.obj.meth_fastcall(k=1),
        )

    def test_fastcall_keywords(self):
        self.assertEqual(
            self.obj.meth_fastcall_keywords(1, 2, a=3, b=4),
            (self.expected_self, (1, 2), {'a': 3, 'b': 4})
        )

    def test_fastcall_keywords_ext(self):
        self.assertEqual(
            self.obj.meth_fastcall_keywords(*(1, 2), **{'a': 3, 'b': 4}),
            (self.expected_self, (1, 2), {'a': 3, 'b': 4})
        )


class TestCallingConventionsInstance(TestCallingConventions):
    """Test calling instance methods using various calling conventions"""

    def setUp(self):
        self.obj = self.expected_self = _testcapi.MethInstance()


class TestCallingConventionsClass(TestCallingConventions):
    """Test calling class methods using various calling conventions"""

    def setUp(self):
        self.obj = self.expected_self = _testcapi.MethClass


class TestCallingConventionsClassInstance(TestCallingConventions):
    """Test calling class methods on instance"""

    def setUp(self):
        self.obj = _testcapi.MethClass()
        self.expected_self = _testcapi.MethClass


class TestCallingConventionsStatic(TestCallingConventions):
    """Test calling static methods using various calling conventions"""

    def setUp(self):
        self.obj = _testcapi.MethStatic()
        self.expected_self = None


def pyfunc(arg1, arg2):
    return [arg1, arg2]


def pyfunc_noarg():
    return "noarg"


class PythonClass:
    def method(self, arg1, arg2):
        return [arg1, arg2]

    def method_noarg(self):
        return "noarg"

    @classmethod
    def class_method(cls):
        return "classmethod"

    @staticmethod
    def static_method():
        return "staticmethod"


PYTHON_INSTANCE = PythonClass()

NULL_OR_EMPTY = object()

class FastCallTests(unittest.TestCase):
    """Test calling using various callables from C
    """

    # Test calls with positional arguments
    CALLS_POSARGS = [
        # (func, args: tuple, result)

        # Python function with 2 arguments
        (pyfunc, (1, 2), [1, 2]),

        # Python function without argument
        (pyfunc_noarg, (), "noarg"),

        # Python class methods
        (PythonClass.class_method, (), "classmethod"),
        (PythonClass.static_method, (), "staticmethod"),

        # Python instance methods
        (PYTHON_INSTANCE.method, (1, 2), [1, 2]),
        (PYTHON_INSTANCE.method_noarg, (), "noarg"),
        (PYTHON_INSTANCE.class_method, (), "classmethod"),
        (PYTHON_INSTANCE.static_method, (), "staticmethod"),

        # C callables are added later
    ]

    # Test calls with positional and keyword arguments
    CALLS_KWARGS = [
        # (func, args: tuple, kwargs: dict, result)

        # Python function with 2 arguments
        (pyfunc, (1,), {'arg2': 2}, [1, 2]),
        (pyfunc, (), {'arg1': 1, 'arg2': 2}, [1, 2]),

        # Python instance methods
        (PYTHON_INSTANCE.method, (1,), {'arg2': 2}, [1, 2]),
        (PYTHON_INSTANCE.method, (), {'arg1': 1, 'arg2': 2}, [1, 2]),

        # C callables are added later
    ]

    # Add all the calling conventions and variants of C callables
    _instance = _testcapi.MethInstance()
    for obj, expected_self in (
        (_testcapi, _testcapi),  # module-level function
        (_instance, _instance),  # bound method
        (_testcapi.MethClass, _testcapi.MethClass),  # class method on class
        (_testcapi.MethClass(), _testcapi.MethClass),  # class method on inst.
        (_testcapi.MethStatic, None),  # static method
    ):
        CALLS_POSARGS.extend([
            (obj.meth_varargs, (1, 2), (expected_self, (1, 2))),
            (obj.meth_varargs_keywords,
                (1, 2), (expected_self, (1, 2), NULL_OR_EMPTY)),
            (obj.meth_fastcall, (1, 2), (expected_self, (1, 2))),
            (obj.meth_fastcall, (), (expected_self, ())),
            (obj.meth_fastcall_keywords,
                (1, 2), (expected_self, (1, 2), NULL_OR_EMPTY)),
            (obj.meth_fastcall_keywords,
                (), (expected_self, (), NULL_OR_EMPTY)),
            (obj.meth_noargs, (), expected_self),
            (obj.meth_o, (123, ), (expected_self, 123)),
        ])

        CALLS_KWARGS.extend([
            (obj.meth_varargs_keywords,
                (1, 2), {'x': 'y'}, (expected_self, (1, 2), {'x': 'y'})),
            (obj.meth_varargs_keywords,
                (), {'x': 'y'}, (expected_self, (), {'x': 'y'})),
            (obj.meth_varargs_keywords,
                (1, 2), {}, (expected_self, (1, 2), NULL_OR_EMPTY)),
            (obj.meth_fastcall_keywords,
                (1, 2), {'x': 'y'}, (expected_self, (1, 2), {'x': 'y'})),
            (obj.meth_fastcall_keywords,
                (), {'x': 'y'}, (expected_self, (), {'x': 'y'})),
            (obj.meth_fastcall_keywords,
                (1, 2), {}, (expected_self, (1, 2), NULL_OR_EMPTY)),
        ])

    def check_result(self, result, expected):
        if isinstance(expected, tuple) and expected[-1] is NULL_OR_EMPTY:
            if result[-1] in ({}, None):
                expected = (*expected[:-1], result[-1])
        self.assertEqual(result, expected)

    def test_fastcall(self):
        # Test _PyObject_FastCall()

        for func, args, expected in self.CALLS_POSARGS:
            with self.subTest(func=func, args=args):
                result = _testcapi.pyobject_fastcall(func, args)
                self.check_result(result, expected)

                if not args:
                    # args=NULL, nargs=0
                    result = _testcapi.pyobject_fastcall(func, None)
                    self.check_result(result, expected)

    def test_vectorcall_dict(self):
        # Test PyObject_VectorcallDict()

        for func, args, expected in self.CALLS_POSARGS:
            with self.subTest(func=func, args=args):
                # kwargs=NULL
                result = _testcapi.pyobject_fastcalldict(func, args, None)
                self.check_result(result, expected)

                if not args:
                    # args=NULL, nargs=0, kwargs=NULL
                    result = _testcapi.pyobject_fastcalldict(func, None, None)
                    self.check_result(result, expected)

        for func, args, kwargs, expected in self.CALLS_KWARGS:
            with self.subTest(func=func, args=args, kwargs=kwargs):
                result = _testcapi.pyobject_fastcalldict(func, args, kwargs)
                self.check_result(result, expected)

    def test_vectorcall(self):
        # Test PyObject_Vectorcall()

        for func, args, expected in self.CALLS_POSARGS:
            with self.subTest(func=func, args=args):
                # kwnames=NULL
                result = _testcapi.pyobject_vectorcall(func, args, None)
                self.check_result(result, expected)

                # kwnames=()
                result = _testcapi.pyobject_vectorcall(func, args, ())
                self.check_result(result, expected)

                if not args:
                    # kwnames=NULL
                    result = _testcapi.pyobject_vectorcall(func, None, None)
                    self.check_result(result, expected)

                    # kwnames=()
                    result = _testcapi.pyobject_vectorcall(func, None, ())
                    self.check_result(result, expected)

        for func, args, kwargs, expected in self.CALLS_KWARGS:
            with self.subTest(func=func, args=args, kwargs=kwargs):
                kwnames = tuple(kwargs.keys())
                args = args + tuple(kwargs.values())
                result = _testcapi.pyobject_vectorcall(func, args, kwnames)
                self.check_result(result, expected)

    def test_fastcall_clearing_dict(self):
        # Test bpo-36907: the point of the test is just checking that this
        # does not crash.
        class IntWithDict:
            __slots__ = ["kwargs"]
            def __init__(self, **kwargs):
                self.kwargs = kwargs
            def __index__(self):
                self.kwargs.clear()
                gc.collect()
                return 0
        x = IntWithDict(dont_inherit=IntWithDict())
        # We test the argument handling of "compile" here, the compilation
        # itself is not relevant. When we pass flags=x below, x.__index__() is
        # called, which changes the keywords dict.
        compile("pass", "", "exec", x, **x.kwargs)


Py_TPFLAGS_HAVE_VECTORCALL = 1 << 11
Py_TPFLAGS_METHOD_DESCRIPTOR = 1 << 17


def testfunction(self):
    """some doc"""
    return self


def testfunction_kw(self, *, kw):
    """some doc"""
    return self


QUICKENING_WARMUP_DELAY = 8


class TestPEP590(unittest.TestCase):

    def test_method_descriptor_flag(self):
        import functools
        cached = functools.lru_cache(1)(testfunction)

        self.assertFalse(type(repr).__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)
        self.assertTrue(type(list.append).__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)
        self.assertTrue(type(list.__add__).__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)
        self.assertTrue(type(testfunction).__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)
        self.assertTrue(type(cached).__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)

        self.assertTrue(_testcapi.MethodDescriptorBase.__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)
        self.assertTrue(_testcapi.MethodDescriptorDerived.__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)
        self.assertFalse(_testcapi.MethodDescriptorNopGet.__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)

        # Mutable heap types should not inherit Py_TPFLAGS_METHOD_DESCRIPTOR
        class MethodDescriptorHeap(_testcapi.MethodDescriptorBase):
            pass
        self.assertFalse(MethodDescriptorHeap.__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)

    def test_vectorcall_flag(self):
        self.assertTrue(_testcapi.MethodDescriptorBase.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)
        self.assertTrue(_testcapi.MethodDescriptorDerived.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)
        self.assertFalse(_testcapi.MethodDescriptorNopGet.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)
        self.assertTrue(_testcapi.MethodDescriptor2.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)

        # Mutable heap types should inherit Py_TPFLAGS_HAVE_VECTORCALL,
        # but should lose it when __call__ is overridden
        class MethodDescriptorHeap(_testcapi.MethodDescriptorBase):
            pass
        self.assertTrue(MethodDescriptorHeap.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)
        MethodDescriptorHeap.__call__ = print
        self.assertFalse(MethodDescriptorHeap.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)

        # Mutable heap types should not inherit Py_TPFLAGS_HAVE_VECTORCALL if
        # they define __call__ directly
        class MethodDescriptorHeap(_testcapi.MethodDescriptorBase):
            def __call__(self):
                pass
        self.assertFalse(MethodDescriptorHeap.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)

    def test_vectorcall_override(self):
        # Check that tp_call can correctly override vectorcall.
        # MethodDescriptorNopGet implements tp_call but it inherits from
        # MethodDescriptorBase, which implements vectorcall. Since
        # MethodDescriptorNopGet returns the args tuple when called, we check
        # additionally that no new tuple is created for this call.
        args = tuple(range(5))
        f = _testcapi.MethodDescriptorNopGet()
        self.assertIs(f(*args), args)

    def test_vectorcall_override_on_mutable_class(self):
        """Setting __call__ should disable vectorcall"""
        TestType = _testcapi.make_vectorcall_class()
        instance = TestType()
        self.assertEqual(instance(), "tp_call")
        instance.set_vectorcall(TestType)
        self.assertEqual(instance(), "vectorcall")  # assume vectorcall is used
        TestType.__call__ = lambda self: "custom"
        self.assertEqual(instance(), "custom")

    def test_vectorcall_override_with_subclass(self):
        """Setting __call__ on a superclass should disable vectorcall"""
        SuperType = _testcapi.make_vectorcall_class()
        class DerivedType(SuperType):
            pass

        instance = DerivedType()

        # Derived types with its own vectorcall should be unaffected
        UnaffectedType1 = _testcapi.make_vectorcall_class(DerivedType)
        UnaffectedType2 = _testcapi.make_vectorcall_class(SuperType)

        # Aside: Quickly check that the C helper actually made derived types
        self.assertTrue(issubclass(UnaffectedType1, DerivedType))
        self.assertTrue(issubclass(UnaffectedType2, SuperType))

        # Initial state: tp_call
        self.assertEqual(instance(), "tp_call")
        self.assertEqual(_testcapi.has_vectorcall_flag(SuperType), True)
        self.assertEqual(_testcapi.has_vectorcall_flag(DerivedType), True)
        self.assertEqual(_testcapi.has_vectorcall_flag(UnaffectedType1), True)
        self.assertEqual(_testcapi.has_vectorcall_flag(UnaffectedType2), True)

        # Setting the vectorcall function
        instance.set_vectorcall(SuperType)

        self.assertEqual(instance(), "vectorcall")
        self.assertEqual(_testcapi.has_vectorcall_flag(SuperType), True)
        self.assertEqual(_testcapi.has_vectorcall_flag(DerivedType), True)
        self.assertEqual(_testcapi.has_vectorcall_flag(UnaffectedType1), True)
        self.assertEqual(_testcapi.has_vectorcall_flag(UnaffectedType2), True)

        # Setting __call__ should remove vectorcall from all subclasses
        SuperType.__call__ = lambda self: "custom"

        self.assertEqual(instance(), "custom")
        self.assertEqual(_testcapi.has_vectorcall_flag(SuperType), False)
        self.assertEqual(_testcapi.has_vectorcall_flag(DerivedType), False)
        self.assertEqual(_testcapi.has_vectorcall_flag(UnaffectedType1), True)
        self.assertEqual(_testcapi.has_vectorcall_flag(UnaffectedType2), True)


    def test_vectorcall(self):
        # Test a bunch of different ways to call objects:
        # 1. vectorcall using PyVectorcall_Call()
        #   (only for objects that support vectorcall directly)
        # 2. normal call
        # 3. vectorcall using PyObject_Vectorcall()
        # 4. call as bound method
        # 5. call using functools.partial

        # A list of (function, args, kwargs, result) calls to test
        calls = [(len, (range(42),), {}, 42),
                 (list.append, ([], 0), {}, None),
                 ([].append, (0,), {}, None),
                 (sum, ([36],), {"start":6}, 42),
                 (testfunction, (42,), {}, 42),
                 (testfunction_kw, (42,), {"kw":None}, 42),
                 (_testcapi.MethodDescriptorBase(), (0,), {}, True),
                 (_testcapi.MethodDescriptorDerived(), (0,), {}, True),
                 (_testcapi.MethodDescriptor2(), (0,), {}, False)]

        from _testcapi import pyobject_vectorcall, pyvectorcall_call
        from types import MethodType
        from functools import partial

        def vectorcall(func, args, kwargs):
            args = *args, *kwargs.values()
            kwnames = tuple(kwargs)
            return pyobject_vectorcall(func, args, kwnames)

        for (func, args, kwargs, expected) in calls:
            with self.subTest(str(func)):
                if not kwargs:
                    self.assertEqual(expected, pyvectorcall_call(func, args))
                self.assertEqual(expected, pyvectorcall_call(func, args, kwargs))

        # Add derived classes (which do not support vectorcall directly,
        # but do support all other ways of calling).

        class MethodDescriptorHeap(_testcapi.MethodDescriptorBase):
            pass

        class MethodDescriptorOverridden(_testcapi.MethodDescriptorBase):
            def __call__(self, n):
                return 'new'

        class SuperBase:
            def __call__(self, *args):
                return super().__call__(*args)

        class MethodDescriptorSuper(SuperBase, _testcapi.MethodDescriptorBase):
            def __call__(self, *args):
                return super().__call__(*args)

        calls += [
            (dict.update, ({},), {"key":True}, None),
            ({}.update, ({},), {"key":True}, None),
            (MethodDescriptorHeap(), (0,), {}, True),
            (MethodDescriptorOverridden(), (0,), {}, 'new'),
            (MethodDescriptorSuper(), (0,), {}, True),
        ]

        for (func, args, kwargs, expected) in calls:
            with self.subTest(str(func)):
                args1 = args[1:]
                meth = MethodType(func, args[0])
                wrapped = partial(func)
                if not kwargs:
                    self.assertEqual(expected, func(*args))
                    self.assertEqual(expected, pyobject_vectorcall(func, args, None))
                    self.assertEqual(expected, meth(*args1))
                    self.assertEqual(expected, wrapped(*args))
                self.assertEqual(expected, func(*args, **kwargs))
                self.assertEqual(expected, vectorcall(func, args, kwargs))
                self.assertEqual(expected, meth(*args1, **kwargs))
                self.assertEqual(expected, wrapped(*args, **kwargs))

    def test_setvectorcall(self):
        from _testcapi import function_setvectorcall
        def f(num): return num + 1
        assert_equal = self.assertEqual
        num = 10
        assert_equal(11, f(num))
        function_setvectorcall(f)
        # make sure specializer is triggered by running > 50 times
        for _ in range(10 * QUICKENING_WARMUP_DELAY):
            assert_equal("overridden", f(num))

    def test_setvectorcall_load_attr_specialization_skip(self):
        from _testcapi import function_setvectorcall

        class X:
            def __getattribute__(self, attr):
                return attr

        assert_equal = self.assertEqual
        x = X()
        assert_equal("a", x.a)
        function_setvectorcall(X.__getattribute__)
        # make sure specialization doesn't trigger
        # when vectorcall is overridden
        for _ in range(QUICKENING_WARMUP_DELAY):
            assert_equal("overridden", x.a)

    def test_setvectorcall_load_attr_specialization_deopt(self):
        from _testcapi import function_setvectorcall

        class X:
            def __getattribute__(self, attr):
                return attr

        def get_a(x):
            return x.a

        assert_equal = self.assertEqual
        x = X()
        # trigger LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN specialization
        for _ in range(QUICKENING_WARMUP_DELAY):
            assert_equal("a", get_a(x))
        function_setvectorcall(X.__getattribute__)
        # make sure specialized LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN
        # gets deopted due to overridden vectorcall
        for _ in range(QUICKENING_WARMUP_DELAY):
            assert_equal("overridden", get_a(x))

    @requires_limited_api
    def test_vectorcall_limited(self):
        from _testcapi import pyobject_vectorcall
        obj = _testcapi.LimitedVectorCallClass()
        self.assertEqual(pyobject_vectorcall(obj, (), ()), "vectorcall called")


class A:
    def method_two_args(self, x, y):
        pass

    @staticmethod
    def static_no_args():
        pass

    @staticmethod
    def positional_only(arg, /):
        pass

@cpython_only
class TestErrorMessagesUseQualifiedName(unittest.TestCase):

    @contextlib.contextmanager
    def check_raises_type_error(self, message):
        with self.assertRaises(TypeError) as cm:
            yield
        self.assertEqual(str(cm.exception), message)

    def test_missing_arguments(self):
        msg = "A.method_two_args() missing 1 required positional argument: 'y'"
        with self.check_raises_type_error(msg):
            A().method_two_args("x")

    def test_too_many_positional(self):
        msg = "A.static_no_args() takes 0 positional arguments but 1 was given"
        with self.check_raises_type_error(msg):
            A.static_no_args("oops it's an arg")

    def test_positional_only_passed_as_keyword(self):
        msg = "A.positional_only() got some positional-only arguments passed as keyword arguments: 'arg'"
        with self.check_raises_type_error(msg):
            A.positional_only(arg="x")

    def test_unexpected_keyword(self):
        msg = "A.method_two_args() got an unexpected keyword argument 'bad'"
        with self.check_raises_type_error(msg):
            A().method_two_args(bad="x")

    def test_multiple_values(self):
        msg = "A.method_two_args() got multiple values for argument 'x'"
        with self.check_raises_type_error(msg):
            A().method_two_args("x", "y", x="oops")


if __name__ == "__main__":
    unittest.main()
