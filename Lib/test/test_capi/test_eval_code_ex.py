import unittest
import builtins
from collections import UserDict

from test.support import import_helper
from test.support import swap_attr


# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')

NULL = None


class PyEval_EvalCodeExTests(unittest.TestCase):

    def test_simple(self):
        def f():
            return a

        eval_code_ex = _testcapi.eval_code_ex
        code = f.__code__
        self.assertEqual(eval_code_ex(code, dict(a=1)), 1)

        self.assertRaises(NameError, eval_code_ex, code, {})
        self.assertRaises(SystemError, eval_code_ex, code, UserDict(a=1))
        self.assertRaises(SystemError, eval_code_ex, code, [])
        self.assertRaises(SystemError, eval_code_ex, code, 1)
        # CRASHES eval_code_ex(code, NULL)
        # CRASHES eval_code_ex(1, {})
        # CRASHES eval_code_ex(NULL, {})

    def test_custom_locals(self):
        # Monkey-patch __build_class__ to get a class code object.
        code = None
        def build_class(func, name, /, *bases, **kwds):
            nonlocal code
            code = func.__code__

        with swap_attr(builtins, '__build_class__', build_class):
            class A:
                # Uses LOAD_NAME for a
                r[:] = [a]

        eval_code_ex = _testcapi.eval_code_ex
        results = []
        g = dict(a=1, r=results)
        self.assertIsNone(eval_code_ex(code, g))
        self.assertEqual(results, [1])
        self.assertIsNone(eval_code_ex(code, g, dict(a=2)))
        self.assertEqual(results, [2])
        self.assertIsNone(eval_code_ex(code, g, UserDict(a=3)))
        self.assertEqual(results, [3])
        self.assertIsNone(eval_code_ex(code, g, {}))
        self.assertEqual(results, [1])
        self.assertIsNone(eval_code_ex(code, g, NULL))
        self.assertEqual(results, [1])

        self.assertRaises(TypeError, eval_code_ex, code, g, [])
        self.assertRaises(TypeError, eval_code_ex, code, g, 1)
        self.assertRaises(NameError, eval_code_ex, code, dict(r=results), {})
        self.assertRaises(NameError, eval_code_ex, code, dict(r=results), NULL)
        self.assertRaises(TypeError, eval_code_ex, code, dict(r=results), [])
        self.assertRaises(TypeError, eval_code_ex, code, dict(r=results), 1)

    def test_with_args(self):
        def f(a, b, c):
            return a

        eval_code_ex = _testcapi.eval_code_ex
        code = f.__code__
        self.assertEqual(eval_code_ex(code, {}, {}, (1, 2, 3)), 1)
        self.assertRaises(TypeError, eval_code_ex, code, {}, {}, (1, 2))
        self.assertRaises(TypeError, eval_code_ex, code, {}, {}, (1, 2, 3, 4))

    def test_with_kwargs(self):
        def f(a, b, c):
            return a

        eval_code_ex = _testcapi.eval_code_ex
        code = f.__code__
        self.assertEqual(eval_code_ex(code, {}, {}, (), dict(a=1, b=2, c=3)), 1)
        self.assertRaises(TypeError, eval_code_ex, code, {}, {}, (), dict(a=1, b=2))
        self.assertRaises(TypeError, eval_code_ex, code, {}, {}, (), dict(a=1, b=2))
        self.assertRaises(TypeError, eval_code_ex, code, {}, {}, (), dict(a=1, b=2, c=3, d=4))

    def test_with_default(self):
        def f(a):
            return a

        eval_code_ex = _testcapi.eval_code_ex
        code = f.__code__
        self.assertEqual(eval_code_ex(code, {}, {}, (), {}, (1,)), 1)
        self.assertRaises(TypeError, eval_code_ex, code, {}, {}, (), {}, ())

    def test_with_kwarg_default(self):
        def f(*, a):
            return a

        eval_code_ex = _testcapi.eval_code_ex
        code = f.__code__
        self.assertEqual(eval_code_ex(code, {}, {}, (), {}, (), dict(a=1)), 1)
        self.assertRaises(TypeError, eval_code_ex, code, {}, {}, (), {}, (), {})
        self.assertRaises(TypeError, eval_code_ex, code, {}, {}, (), {}, (), NULL)
        self.assertRaises(SystemError, eval_code_ex, code, {}, {}, (), {}, (), UserDict(a=1))
        self.assertRaises(SystemError, eval_code_ex, code, {}, {}, (), {}, (), [])
        self.assertRaises(SystemError, eval_code_ex, code, {}, {}, (), {}, (), 1)

    def test_with_closure(self):
        a = 1
        b = 2
        def f():
            b
            return a

        eval_code_ex = _testcapi.eval_code_ex
        code = f.__code__
        self.assertEqual(eval_code_ex(code, {}, {}, (), {}, (), {}, f.__closure__), 1)
        self.assertEqual(eval_code_ex(code, {}, {}, (), {}, (), {}, f.__closure__[::-1]), 2)

        # CRASHES eval_code_ex(code, {}, {}, (), {}, (), {}, ()), 1)
        # CRASHES eval_code_ex(code, {}, {}, (), {}, (), {}, NULL), 1)


if __name__ == "__main__":
    unittest.main()
