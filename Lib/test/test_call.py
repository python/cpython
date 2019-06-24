import datetime
import unittest
from test.support import cpython_only
try:
    import _testcapi
except ImportError:
    _testcapi = None
import struct
import collections
import itertools
import gc


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


# The test cases here cover several paths through the function calling
# code.  They depend on the METH_XXX flag that is used to define a C
# function, which can't be verified from Python.  If the METH_XXX decl
# for a C function changes, these tests may not cover the right paths.

class CFunctionCalls(unittest.TestCase):

    def test_varargs0(self):
        self.assertRaises(TypeError, {}.__contains__)

    def test_varargs1(self):
        {}.__contains__(0)

    def test_varargs2(self):
        self.assertRaises(TypeError, {}.__contains__, 0, 1)

    def test_varargs0_ext(self):
        try:
            {}.__contains__(*())
        except TypeError:
            pass

    def test_varargs1_ext(self):
        {}.__contains__(*(0,))

    def test_varargs2_ext(self):
        try:
            {}.__contains__(*(1, 2))
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_varargs1_kw(self):
        self.assertRaises(TypeError, {}.__contains__, x=2)

    def test_varargs2_kw(self):
        self.assertRaises(TypeError, {}.__contains__, x=2, y=2)

    def test_oldargs0_0(self):
        {}.keys()

    def test_oldargs0_1(self):
        self.assertRaises(TypeError, {}.keys, 0)

    def test_oldargs0_2(self):
        self.assertRaises(TypeError, {}.keys, 0, 1)

    def test_oldargs0_0_ext(self):
        {}.keys(*())

    def test_oldargs0_1_ext(self):
        try:
            {}.keys(*(0,))
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs0_2_ext(self):
        try:
            {}.keys(*(1, 2))
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs0_0_kw(self):
        try:
            {}.keys(x=2)
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs0_1_kw(self):
        self.assertRaises(TypeError, {}.keys, x=2)

    def test_oldargs0_2_kw(self):
        self.assertRaises(TypeError, {}.keys, x=2, y=2)

    def test_oldargs1_0(self):
        self.assertRaises(TypeError, [].count)

    def test_oldargs1_1(self):
        [].count(1)

    def test_oldargs1_2(self):
        self.assertRaises(TypeError, [].count, 1, 2)

    def test_oldargs1_0_ext(self):
        try:
            [].count(*())
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs1_1_ext(self):
        [].count(*(1,))

    def test_oldargs1_2_ext(self):
        try:
            [].count(*(1, 2))
        except TypeError:
            pass
        else:
            raise RuntimeError

    def test_oldargs1_0_kw(self):
        self.assertRaises(TypeError, [].count, x=2)

    def test_oldargs1_1_kw(self):
        self.assertRaises(TypeError, [].count, {}, x=2)

    def test_oldargs1_2_kw(self):
        self.assertRaises(TypeError, [].count, x=2, y=2)


@cpython_only
class CFunctionCallsErrorMessages(unittest.TestCase):

    def test_varargs0(self):
        msg = r"__contains__\(\) takes exactly one argument \(0 given\)"
        self.assertRaisesRegex(TypeError, msg, {}.__contains__)

    def test_varargs2(self):
        msg = r"__contains__\(\) takes exactly one argument \(2 given\)"
        self.assertRaisesRegex(TypeError, msg, {}.__contains__, 0, 1)

    def test_varargs3(self):
        msg = r"^from_bytes\(\) takes exactly 2 positional arguments \(3 given\)"
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
        msg = r"^index\(\) takes no keyword arguments$"
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
        msg = r"^pack\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, struct.pack, x=2)

    def test_varargs9_kw(self):
        msg = r"^pack_into\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, struct.pack_into, x=2)

    def test_varargs10_kw(self):
        msg = r"^index\(\) takes no keyword arguments$"
        self.assertRaisesRegex(TypeError, msg, collections.deque().index, x=2)

    def test_varargs11_kw(self):
        msg = r"^pack\(\) takes no keyword arguments$"
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
        msg = r"^print\(\) takes at most 4 keyword arguments \(5 given\)$"
        self.assertRaisesRegex(TypeError, msg,
                               print, 0, sep=1, end=2, file=3, flush=4, foo=5)

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


IGNORE_RESULT = object()


@cpython_only
class FastCallTests(unittest.TestCase):
    # Test calls with positional arguments
    CALLS_POSARGS = (
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

        # C function: METH_NOARGS
        (globals, (), IGNORE_RESULT),

        # C function: METH_O
        (id, ("hello",), IGNORE_RESULT),

        # C function: METH_VARARGS
        (dir, (1,), IGNORE_RESULT),

        # C function: METH_VARARGS | METH_KEYWORDS
        (min, (5, 9), 5),

        # C function: METH_FASTCALL
        (divmod, (1000, 33), (30, 10)),

        # C type static method: METH_FASTCALL | METH_CLASS
        (int.from_bytes, (b'\x01\x00', 'little'), 1),

        # bpo-30524: Test that calling a C type static method with no argument
        # doesn't crash (ignore the result): METH_FASTCALL | METH_CLASS
        (datetime.datetime.now, (), IGNORE_RESULT),
    )

    # Test calls with positional and keyword arguments
    CALLS_KWARGS = (
        # (func, args: tuple, kwargs: dict, result)

        # Python function with 2 arguments
        (pyfunc, (1,), {'arg2': 2}, [1, 2]),
        (pyfunc, (), {'arg1': 1, 'arg2': 2}, [1, 2]),

        # Python instance methods
        (PYTHON_INSTANCE.method, (1,), {'arg2': 2}, [1, 2]),
        (PYTHON_INSTANCE.method, (), {'arg1': 1, 'arg2': 2}, [1, 2]),

        # C function: METH_VARARGS | METH_KEYWORDS
        (max, ([],), {'default': 9}, 9),

        # C type static method: METH_FASTCALL | METH_CLASS
        (int.from_bytes, (b'\x01\x00',), {'byteorder': 'little'}, 1),
        (int.from_bytes, (), {'bytes': b'\x01\x00', 'byteorder': 'little'}, 1),
    )

    def check_result(self, result, expected):
        if expected is IGNORE_RESULT:
            return
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
        # Test _PyObject_FastCallDict()

        for func, args, expected in self.CALLS_POSARGS:
            with self.subTest(func=func, args=args):
                # kwargs=NULL
                result = _testcapi.pyobject_fastcalldict(func, args, None)
                self.check_result(result, expected)

                # kwargs={}
                result = _testcapi.pyobject_fastcalldict(func, args, {})
                self.check_result(result, expected)

                if not args:
                    # args=NULL, nargs=0, kwargs=NULL
                    result = _testcapi.pyobject_fastcalldict(func, None, None)
                    self.check_result(result, expected)

                    # args=NULL, nargs=0, kwargs={}
                    result = _testcapi.pyobject_fastcalldict(func, None, {})
                    self.check_result(result, expected)

        for func, args, kwargs, expected in self.CALLS_KWARGS:
            with self.subTest(func=func, args=args, kwargs=kwargs):
                result = _testcapi.pyobject_fastcalldict(func, args, kwargs)
                self.check_result(result, expected)

    def test_vectorcall(self):
        # Test _PyObject_Vectorcall()

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

        # Heap type should not inherit Py_TPFLAGS_METHOD_DESCRIPTOR
        class MethodDescriptorHeap(_testcapi.MethodDescriptorBase):
            pass
        self.assertFalse(MethodDescriptorHeap.__flags__ & Py_TPFLAGS_METHOD_DESCRIPTOR)

    def test_vectorcall_flag(self):
        self.assertTrue(_testcapi.MethodDescriptorBase.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)
        self.assertTrue(_testcapi.MethodDescriptorDerived.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)
        self.assertFalse(_testcapi.MethodDescriptorNopGet.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)
        self.assertTrue(_testcapi.MethodDescriptor2.__flags__ & Py_TPFLAGS_HAVE_VECTORCALL)

        # Heap type should not inherit Py_TPFLAGS_HAVE_VECTORCALL
        class MethodDescriptorHeap(_testcapi.MethodDescriptorBase):
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

    def test_vectorcall(self):
        # Test a bunch of different ways to call objects:
        # 1. vectorcall using PyVectorcall_Call()
        #   (only for objects that support vectorcall directly)
        # 2. normal call
        # 3. vectorcall using _PyObject_Vectorcall()
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


if __name__ == "__main__":
    unittest.main()
