import sys
import unittest
from test.support import import_helper

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')


class Tests(unittest.TestCase):
    def test_eval_get_func_name(self):
        eval_get_func_name = _testlimitedcapi.eval_get_func_name

        def function_example(): ...

        class A:
            def method_example(self): ...

        self.assertEqual(eval_get_func_name(function_example),
                         "function_example")
        self.assertEqual(eval_get_func_name(A.method_example),
                         "method_example")
        self.assertEqual(eval_get_func_name(A().method_example),
                         "method_example")
        self.assertEqual(eval_get_func_name(sum), "sum")  # c function
        self.assertEqual(eval_get_func_name(A), "type")

    def test_eval_get_func_desc(self):
        eval_get_func_desc = _testlimitedcapi.eval_get_func_desc

        def function_example(): ...

        class A:
            def method_example(self): ...

        self.assertEqual(eval_get_func_desc(function_example),
                         "()")
        self.assertEqual(eval_get_func_desc(A.method_example),
                         "()")
        self.assertEqual(eval_get_func_desc(A().method_example),
                         "()")
        self.assertEqual(eval_get_func_desc(sum), "()")  # c function
        self.assertEqual(eval_get_func_desc(A), " object")

    def test_eval_getlocals(self):
        # Test PyEval_GetLocals()
        x = 1
        self.assertEqual(_testlimitedcapi.eval_getlocals(),
            {'self': self,
             'x': 1})

        y = 2
        self.assertEqual(_testlimitedcapi.eval_getlocals(),
            {'self': self,
             'x': 1,
             'y': 2})

    def test_eval_getglobals(self):
        # Test PyEval_GetGlobals()
        self.assertEqual(_testlimitedcapi.eval_getglobals(),
                         globals())

    def test_eval_getbuiltins(self):
        # Test PyEval_GetBuiltins()
        self.assertEqual(_testlimitedcapi.eval_getbuiltins(),
                         globals()['__builtins__'])

    def test_eval_getframe(self):
        # Test PyEval_GetFrame()
        self.assertEqual(_testlimitedcapi.eval_getframe(),
                         sys._getframe())

    def test_eval_getframe_builtins(self):
        # Test PyEval_GetFrameBuiltins()
        self.assertEqual(_testlimitedcapi.eval_getframe_builtins(),
                         sys._getframe().f_builtins)

    def test_eval_getframe_globals(self):
        # Test PyEval_GetFrameGlobals()
        self.assertEqual(_testlimitedcapi.eval_getframe_globals(),
                         sys._getframe().f_globals)

    def test_eval_getframe_locals(self):
        # Test PyEval_GetFrameLocals()
        self.assertEqual(_testlimitedcapi.eval_getframe_locals(),
                         sys._getframe().f_locals)

    def test_eval_get_recursion_limit(self):
        # Test Py_GetRecursionLimit()
        self.assertEqual(_testlimitedcapi.eval_get_recursion_limit(),
                         sys.getrecursionlimit())

    def test_eval_set_recursion_limit(self):
        # Test Py_SetRecursionLimit()
        old_limit = sys.getrecursionlimit()
        try:
            limit = old_limit + 123
            _testlimitedcapi.eval_set_recursion_limit(limit)
            self.assertEqual(sys.getrecursionlimit(), limit)
        finally:
            sys.setrecursionlimit(old_limit)


if __name__ == "__main__":
    unittest.main()
