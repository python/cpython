import datetime
import unittest
from test.support import cpython_only
try:
    import _testcapi
except ImportError:
    _testcapi = None

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

    def test_varargs0_kw(self):
        self.assertRaises(TypeError, {}.__contains__, x=2)

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

    def test_fastcall_dict(self):
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

    def test_fastcall_keywords(self):
        # Test _PyObject_FastCallKeywords()

        for func, args, expected in self.CALLS_POSARGS:
            with self.subTest(func=func, args=args):
                # kwnames=NULL
                result = _testcapi.pyobject_fastcallkeywords(func, args, None)
                self.check_result(result, expected)

                # kwnames=()
                result = _testcapi.pyobject_fastcallkeywords(func, args, ())
                self.check_result(result, expected)

                if not args:
                    # kwnames=NULL
                    result = _testcapi.pyobject_fastcallkeywords(func, None, None)
                    self.check_result(result, expected)

                    # kwnames=()
                    result = _testcapi.pyobject_fastcallkeywords(func, None, ())
                    self.check_result(result, expected)

        for func, args, kwargs, expected in self.CALLS_KWARGS:
            with self.subTest(func=func, args=args, kwargs=kwargs):
                kwnames = tuple(kwargs.keys())
                args = args + tuple(kwargs.values())
                result = _testcapi.pyobject_fastcallkeywords(func, args, kwnames)
                self.check_result(result, expected)


if __name__ == "__main__":
    unittest.main()
