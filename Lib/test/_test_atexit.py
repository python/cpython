"""
Tests run by test_atexit in a subprocess since it clears atexit callbacks.
"""
import atexit
import sys
import unittest
from test import support


class GeneralTest(unittest.TestCase):
    def setUp(self):
        atexit._clear()

    def tearDown(self):
        atexit._clear()

    def assert_raises_unraisable(self, exc_type, func, *args):
        with support.catch_unraisable_exception() as cm:
            atexit.register(func, *args)
            atexit._run_exitfuncs()

            self.assertEqual(cm.unraisable.object, func)
            self.assertEqual(cm.unraisable.exc_type, exc_type)
            self.assertEqual(type(cm.unraisable.exc_value), exc_type)

    def test_order(self):
        # Check that callbacks are called in reverse order with the expected
        # positional and keyword arguments.
        calls = []

        def func1(*args, **kwargs):
            calls.append(('func1', args, kwargs))

        def func2(*args, **kwargs):
            calls.append(('func2', args, kwargs))

        # be sure args are handled properly
        atexit.register(func1, 1, 2)
        atexit.register(func2)
        atexit.register(func2, 3, key="value")
        atexit._run_exitfuncs()

        self.assertEqual(calls,
                         [('func2', (3,), {'key': 'value'}),
                          ('func2', (), {}),
                          ('func1', (1, 2), {})])

    def test_badargs(self):
        def func():
            pass

        # func() has no parameter, but it's called with 2 parameters
        self.assert_raises_unraisable(TypeError, func, 1 ,2)

    def test_raise(self):
        def raise_type_error():
            raise TypeError

        self.assert_raises_unraisable(TypeError, raise_type_error)

    def test_raise_unnormalized(self):
        # bpo-10756: Make sure that an unnormalized exception is handled
        # properly.
        def div_zero():
            1 / 0

        self.assert_raises_unraisable(ZeroDivisionError, div_zero)

    def test_exit(self):
        self.assert_raises_unraisable(SystemExit, sys.exit)

    def test_stress(self):
        a = [0]
        def inc():
            a[0] += 1

        for i in range(128):
            atexit.register(inc)
        atexit._run_exitfuncs()

        self.assertEqual(a[0], 128)

    def test_clear(self):
        a = [0]
        def inc():
            a[0] += 1

        atexit.register(inc)
        atexit._clear()
        atexit._run_exitfuncs()

        self.assertEqual(a[0], 0)

    def test_unregister(self):
        a = [0]
        def inc():
            a[0] += 1
        def dec():
            a[0] -= 1

        for i in range(4):
            atexit.register(inc)
        atexit.register(dec)
        atexit.unregister(inc)
        atexit._run_exitfuncs()

        self.assertEqual(a[0], -1)

    def test_bound_methods(self):
        l = []
        atexit.register(l.append, 5)
        atexit._run_exitfuncs()
        self.assertEqual(l, [5])

        atexit.unregister(l.append)
        atexit._run_exitfuncs()
        self.assertEqual(l, [5])


if __name__ == "__main__":
    unittest.main()
