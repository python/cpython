# Test the most dynamic corner cases of Python's runtime semantics.

import builtins
import sys
import unittest

from test.support import swap_item, swap_attr


class RebindBuiltinsTests(unittest.TestCase):

    """Test all the ways that we can change/shadow globals/builtins."""

    def configure_func(self, func, *args):
        """Perform TestCase-specific configuration on a function before testing.

        By default, this does nothing. Example usage: spinning a function so
        that a JIT will optimize it. Subclasses should override this as needed.

        Args:
            func: function to configure.
            *args: any arguments that should be passed to func, if calling it.

        Returns:
            Nothing. Work will be performed on func in-place.
        """
        pass

    def test_globals_shadow_builtins(self):
        # Modify globals() to shadow an entry in builtins.
        def foo():
            return len([1, 2, 3])
        self.configure_func(foo)

        self.assertEqual(foo(), 3)
        with swap_item(globals(), "len", lambda x: 7):
            self.assertEqual(foo(), 7)

    def test_modify_builtins(self):
        # Modify the builtins module directly.
        def foo():
            return len([1, 2, 3])
        self.configure_func(foo)

        self.assertEqual(foo(), 3)
        with swap_attr(builtins, "len", lambda x: 7):
            self.assertEqual(foo(), 7)

    def test_modify_builtins_while_generator_active(self):
        # Modify the builtins out from under a live generator.
        def foo():
            x = range(3)
            yield len(x)
            yield len(x)
        self.configure_func(foo)

        g = foo()
        self.assertEqual(next(g), 3)
        with swap_attr(builtins, "len", lambda x: 7):
            self.assertEqual(next(g), 7)

    def test_modify_builtins_from_leaf_function(self):
        # Verify that modifications made by leaf functions percolate up the
        # callstack.
        with swap_attr(builtins, "len", len):
            def bar():
                builtins.len = lambda x: 4

            def foo(modifier):
                l = []
                l.append(len(range(7)))
                modifier()
                l.append(len(range(7)))
                return l
            self.configure_func(foo, lambda: None)

            self.assertEqual(foo(bar), [7, 4])

    def test_cannot_change_globals_or_builtins_with_eval(self):
        def foo():
            return len([1, 2, 3])
        self.configure_func(foo)

        # Note that this *doesn't* change the definition of len() seen by foo().
        builtins_dict = {"len": lambda x: 7}
        globals_dict = {"foo": foo, "__builtins__": builtins_dict,
                        "len": lambda x: 8}
        self.assertEqual(eval("foo()", globals_dict), 3)

        self.assertEqual(eval("foo()", {"foo": foo}), 3)

    def test_cannot_change_globals_or_builtins_with_exec(self):
        def foo():
            return len([1, 2, 3])
        self.configure_func(foo)

        globals_dict = {"foo": foo}
        exec("x = foo()", globals_dict)
        self.assertEqual(globals_dict["x"], 3)

        # Note that this *doesn't* change the definition of len() seen by foo().
        builtins_dict = {"len": lambda x: 7}
        globals_dict = {"foo": foo, "__builtins__": builtins_dict,
                        "len": lambda x: 8}

        exec("x = foo()", globals_dict)
        self.assertEqual(globals_dict["x"], 3)

    def test_cannot_replace_builtins_dict_while_active(self):
        def foo():
            x = range(3)
            yield len(x)
            yield len(x)
        self.configure_func(foo)

        g = foo()
        self.assertEqual(next(g), 3)
        with swap_item(globals(), "__builtins__", {"len": lambda x: 7}):
            self.assertEqual(next(g), 3)

    def test_cannot_replace_builtins_dict_between_calls(self):
        def foo():
            return len([1, 2, 3])
        self.configure_func(foo)

        self.assertEqual(foo(), 3)
        with swap_item(globals(), "__builtins__", {"len": lambda x: 7}):
            self.assertEqual(foo(), 3)

    def test_eval_gives_lambda_custom_globals(self):
        globals_dict = {"len": lambda x: 7}
        foo = eval("lambda: len([])", globals_dict)
        self.configure_func(foo)

        self.assertEqual(foo(), 7)

    def test_load_global_specialization_failure_keeps_oparg(self):
        # https://github.com/python/cpython/issues/91625
        class MyGlobals(dict):
            def __missing__(self, key):
                return int(key.removeprefix("_number_"))

        code = "lambda: " + "+".join(f"_number_{i}" for i in range(1000))
        sum_1000 = eval(code, MyGlobals())
        expected = sum(range(1000))
        # Warm up the the function for quickening (PEP 659)
        for _ in range(30):
            self.assertEqual(sum_1000(), expected)


class TestTracing(unittest.TestCase):

    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())
        sys.settrace(None)

    def test_after_specialization(self):

        def trace(frame, event, arg):
            return trace

        turn_on_trace = False

        class C:
            def __init__(self, x):
                self.x = x
            def __del__(self):
                if turn_on_trace:
                    sys.settrace(trace)

        def f():
            # LOAD_GLOBAL[_BUILTIN] immediately follows the call to C.__del__
            C(0).x, len

        def g():
            # BINARY_SUSCR[_LIST_INT] immediately follows the call to C.__del__
            [0][C(0).x]

        def h():
            # BINARY_OP[_ADD_INT] immediately follows the call to C.__del__
            0 + C(0).x

        for func in (f, g, h):
            with self.subTest(func.__name__):
                for _ in range(58):
                    func()
                turn_on_trace = True
                func()
                sys.settrace(None)
                turn_on_trace = False


if __name__ == "__main__":
    unittest.main()
