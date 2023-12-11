import unittest

from test.support import import_helper


# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')


class PyEval_EvalCodeExTests(unittest.TestCase):

    def test_simple(self):
        def f():
            return a

        self.assertEqual(_testcapi.eval_code_ex(f.__code__, dict(a=1)), 1)

    # Need to force the compiler to use LOAD_NAME
    # def test_custom_locals(self):
    #     def f():
    #         return

    def test_with_args(self):
        def f(a, b, c):
            return a

        self.assertEqual(_testcapi.eval_code_ex(f.__code__, {}, {}, (1, 2, 3)), 1)

    def test_with_kwargs(self):
        def f(a, b, c):
            return a

        self.assertEqual(_testcapi.eval_code_ex(f.__code__, {}, {}, (), dict(a=1, b=2, c=3)), 1)

    def test_with_default(self):
        def f(a):
            return a

        self.assertEqual(_testcapi.eval_code_ex(f.__code__, {}, {}, (), {}, (1,)), 1)

    def test_with_kwarg_default(self):
        def f(*, a):
            return a

        self.assertEqual(_testcapi.eval_code_ex(f.__code__, {}, {}, (), {}, (), dict(a=1)), 1)

    def test_with_closure(self):
        a = 1
        def f():
            return a

        self.assertEqual(_testcapi.eval_code_ex(f.__code__, {}, {}, (), {}, (), {}, f.__closure__), 1)


if __name__ == "__main__":
    unittest.main()
