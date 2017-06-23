"""
Test the implementation of the PEP 510: Function specialization.
"""

import unittest
from test import support

# this test is written for CPython: it must be skipped if _testcapi is missing
_testcapi = support.import_module('_testcapi')


class MyGuard(_testcapi.PyGuard):
    def __init__(self):
        self.init_call = None
        self.init_result = 0

        self.check_call = None
        self.check_result = 0

    def check(self, *args, **kw):
        self.check_call = (args, kw)
        return self.check_result

    def init(self, func):
        self.init_call = func
        return self.init_result


class GuardError(Exception):
    pass


class GuardCheck(unittest.TestCase):
    def test_check_result(self):
        guard = MyGuard()
        self.assertEqual(guard(), 0)

        guard.check_result = 1
        self.assertEqual(guard(), 1)

        guard.check_result = 2
        self.assertEqual(guard(), 2)

        with self.assertRaises(RuntimeError):
            guard.check_result = 3
            guard()

    def test_check_args(self):
        guard = MyGuard()
        guard(1, 2, 3, key1='value1', key2='value2')

        expected = ((1, 2, 3), {'key1': 'value1', 'key2': 'value2'})
        self.assertEqual(guard.check_call, expected)

    def test_check_error(self):
        class GuardCheckError(MyGuard):
            def check(self, *args, **kw):
                raise GuardError

        guard = GuardCheckError()
        with self.assertRaises(GuardError):
            guard()

    def test_init_ok(self):
        def func():
            pass

        def func2():
            pass

        guard = MyGuard()
        _testcapi.func_specialize(func, func2.__code__, [guard])
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 1)

        self.assertIs(guard.init_call, func)

    def test_init_fail(self):
        def func():
            pass

        def func2():
            pass

        guard = MyGuard()
        guard.init_result = 1
        _testcapi.func_specialize(func, func2.__code__, [guard])
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 0)

        self.assertIs(guard.init_call, func)

    def test_init_error(self):
        def func():
            pass

        def func2():
            pass

        class GuardInitError(MyGuard):
            def init(self, func):
                raise GuardError

        guard = GuardInitError()
        with self.assertRaises(GuardError):
            _testcapi.func_specialize(func, func2.__code__, [guard])
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 0)

    def test_init_bug(self):
        def func():
            pass

        def func2():
            pass

        guard = MyGuard()
        guard.init_result = 2
        with self.assertRaises(RuntimeError):
            _testcapi.func_specialize(func, func2.__code__, [guard])
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 0)


class SpecializeTests(unittest.TestCase):
    def setUp(self):
        self.guard = MyGuard()

    def test_get_specialize(self):
        def func():
            return "slow"

        class FastFunc:
            def __call__(self):
                return "fast"

        fast_func = FastFunc()
        _testcapi.func_specialize(func, fast_func, [self.guard])
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 1)

        specialized_code, guards = _testcapi.func_get_specialized(func)[0]
        self.assertIs(specialized_code, fast_func)
        self.assertEqual(len(guards), 1)
        self.assertIs(guards[0], self.guard)

    def test_specialize_bytecode(self):
        def func():
            return "slow"

        def fast_func():
            return "fast"

        self.assertEqual(func(), "slow")

        func_code = func.__code__
        fast_code = fast_func.__code__

        # pass the fast_func code object
        _testcapi.func_specialize(func, fast_code, [self.guard])
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 1)

        specialized_code, guards = _testcapi.func_get_specialized(func)[0]
        self.assertEqual(specialized_code.co_code, fast_code.co_code)

        # specialize creates a new code object and copies the code name and the
        # first number line
        self.assertEqual(specialized_code.co_name, func_code.co_name)
        self.assertEqual(specialized_code.co_firstlineno, func_code.co_firstlineno)

        self.assertEqual(func(), "fast")

    def test_specialize_builtin(self):
        def func(obj):
            return obj

        self.assertEqual(func("abc"), "abc")

        # pass builtin function len()
        _testcapi.func_specialize(func, len, [self.guard])
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 1)
        self.assertIs(_testcapi.func_get_specialized(func)[0][0], len)

        self.assertEqual(func("abc"), 3)

    def test_invalid_types(self):
        def func():
            return "slow"

        def fast_func():
            return "fast"

        # specialize code must be a callable or code object
        with self.assertRaises(TypeError):
            _testcapi.func_specialize(func, 123, [self.guard])

        # guards must be a sequence
        with self.assertRaises(TypeError):
            _testcapi.func_specialize(func, fast_func.__code__, 123)

        # guard must be a Guard
        with self.assertRaises(TypeError):
            _testcapi.func_specialize(func, fast_func.__code__, [123])


class CallSpecializedTests(unittest.TestCase):
    def setUp(self):
        self.guard = MyGuard()

        def func():
            return "slow"
        self.func = func

    def specialize(self):
        def fast_func():
            return "fast"

        _testcapi.func_specialize(self.func,
                                  fast_func.__code__,
                                  [self.guard])

    def test_guard_check_error(self):
        self.specialize()

        self.guard.check_result = 3
        with self.assertRaises(RuntimeError):
            self.func()

    def test_temporary_guard_fail(self):
        self.specialize()
        self.assertEqual(self.func(), "fast")

        # the guard temporarely fails
        self.guard.check_result = 1
        self.assertEqual(self.func(), "slow")

        # the guard succeed again
        self.guard.check_result = 0
        self.assertEqual(self.func(), "fast")

    def test_despecialize(self):
        self.specialize()
        self.assertEqual(len(_testcapi.func_get_specialized(self.func)), 1)

        # guard will always fail
        self.guard.check_result = 2
        self.assertEqual(self.func(), "slow")

        # the call removed the specialized code
        self.assertEqual(len(_testcapi.func_get_specialized(self.func)), 0)

        # check if the function still works
        self.guard.check_result = 0
        self.assertEqual(self.func(), "slow")

    def test_despecialize_multiple_codes(self):
        def func():
            return "slow"

        def fast_func1():
            return "fast1"

        guard1 = MyGuard()
        guard1.check_result = 1
        _testcapi.func_specialize(func, fast_func1.__code__, [guard1])

        def fast_func2():
            return "fast2"

        guard2 = MyGuard()
        guard2.check_result = 1
        _testcapi.func_specialize(func, fast_func2.__code__, [guard2])

        def fast_func3():
            return "fast3"

        guard3 = MyGuard()
        _testcapi.func_specialize(func, fast_func3.__code__, [guard3])

        self.assertEqual(func(), "fast3")
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 3)

        # guard 2 will always fail:
        # fast_func2 specialized code will be removed
        guard2.check_result = 2
        self.assertEqual(func(), "fast3")
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 2)

        # guard 3 will always fail:
        # fast_func3 specialized code will be removed
        guard3.check_result = 2
        self.assertEqual(func(), "slow")
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 1)

        # guard 1 succeed again
        guard1.check_result = 0
        self.assertEqual(func(), "fast1")
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 1)

        # guard 3 will always fail:
        # the last specialized code (fast_func1) will be removed
        guard1.check_result = 2
        self.assertEqual(func(), "slow")
        self.assertEqual(len(_testcapi.func_get_specialized(func)), 0)

    def test_set_code(self):
        self.specialize()
        self.assertEqual(len(_testcapi.func_get_specialized(self.func)), 1)

        def func2():
            return "func2"

        # replacing the code object removes all specialized codes
        self.func.__code__ = func2.__code__
        self.assertEqual(len(_testcapi.func_get_specialized(self.func)), 0)


if __name__ == "__main__":
    unittest.main()
