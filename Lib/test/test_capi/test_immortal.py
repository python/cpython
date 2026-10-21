import unittest
from test.support import Py_GIL_DISABLED, import_helper
from test.support.script_helper import assert_python_ok

_testcapi = import_helper.import_module('_testcapi')
_testinternalcapi = import_helper.import_module('_testinternalcapi')


class TestUnstableCAPI(unittest.TestCase):
    def test_immortal(self):
        # Not extensive
        known_immortals = (True, False, None, 0, ())
        for immortal in known_immortals:
            with self.subTest(immortal=immortal):
                self.assertTrue(_testcapi.is_immortal(immortal))

        # Some arbitrary mutable objects
        non_immortals = (object(), self, [object()])
        for non_immortal in non_immortals:
            with self.subTest(non_immortal=non_immortal):
                self.assertFalse(_testcapi.is_immortal(non_immortal))

        # CRASHES _testcapi.is_immortal(NULL)

    def test_set_immortal(self):
        _testcapi.test_py_set_immortal()

    def test_immortalize(self):
        script = """if True:
        import _testcapi
        import gc
        import sys

        objects = [object(), [], {"item": []}, set(),
                   "".join(["immortal", "string"])]
        shared = list(objects)
        for obj in objects:
            assert not sys._is_immortal(obj)
            assert _testcapi.py_immortalize(obj) == 1
            assert sys._is_immortal(obj)
            assert not gc.is_tracked(obj)
            assert _testcapi.py_immortalize(obj) == 0
        assert all(sys._is_immortal(obj) for obj in shared)
        assert _testcapi.py_immortalize(None) == 0
        """
        assert_python_ok("-c", script)

    def test_arbitrary_immortalization(self):
        # This test stresses immortalizing a multitude of objects
        import textwrap

        script = """
        import _testcapi
        import contextlib
        import warnings
        import sys

        warnings.simplefilter("ignore", DeprecationWarning)

        # These have weird side effects
        blacklisted = {"antigravity", "this"}

        def immortalize(obj):
            was_immortal = sys._is_immortal(obj)
            result = _testcapi.py_immortalize(obj)
            if was_immortal:
                assert result == 0

            if result == 1:
                assert sys._is_immortal(obj)

        def immortalize_everything(mod):
            _testcapi.py_immortalize(mod)

            for i in dir(mod):
                attr = getattr(mod, i)
                immortalize(attr)

                for name in dir(attr):
                    with contextlib.suppress(Exception):
                        immortalize(getattr(attr, name))

        for name in sys.stdlib_module_names:
            if name in blacklisted:
                continue

            with contextlib.suppress(ImportError):
                immortalize_everything(__import__(name))
        """
        assert_python_ok("-c", textwrap.dedent(script))

    @unittest.skipUnless(Py_GIL_DISABLED, "per-thread refcounting is free-threaded only")
    def test_immortalize_flushes_per_thread_refcounts(self):
        # The calling thread's pending per-thread reference counts for the
        # object being immortalized must be merged before its unique id is
        # recycled, otherwise a stale count is later applied to whatever
        # object reuses that id, corrupting its reference count.
        script = """if True:
        import _testcapi
        import gc
        import sys
        import threading
        import types

        gc.disable()

        def make_code():
            def inner(a, b):
                return a + b
            return inner.__code__

        code = make_code()

        seed = types.FunctionType(code, {})

        bag = []
        def worker():
            for _ in range(500):
                bag.append(types.FunctionType(code, {}))
        t = threading.Thread(target=worker)
        t.start()
        t.join()

        del seed
        bag.clear()

        assert _testcapi.py_immortalize(code) == 1
        assert sys._is_immortal(code)

        victims = [compile(f"x = {i}", f"<v{i}>", "exec") for i in range(40)]
        keep = list(victims)

        gc.enable()
        for _ in range(10):
            gc.collect()

        ns = {}
        for v in keep:
            exec(v, ns)

        del victims, keep
        for _ in range(10):
            gc.collect()
        """
        assert_python_ok("-c", script)

    @unittest.skipUnless(Py_GIL_DISABLED, "deferred refcounting is free-threaded only")
    def test_immortalize_disables_deferred_refcount(self):
        # Objects with deferred refcounting must be cleaned upon immortalization.
        script = """if True:
        import _testcapi
        import gc
        import sys
        import types

        def func(value):
            return value * 2

        class Meta(type):
            pass

        class Heap(metaclass=Meta):
            attr = 1

        mod = types.ModuleType("immortal_module")

        targets = [func, func.__code__, Heap, Meta, mod]

        gc.collect()

        for obj in targets:
            assert _testcapi.py_immortalize(obj) == 1, obj
            assert sys._is_immortal(obj), obj

        assert func(21) == 42
        assert Heap.attr == 1
        for _ in range(3):
            gc.collect()
        """
        assert_python_ok("-c", script)

    def test_immortalize_updates_live_frames(self):
        # When an object is immortalized while a live frame still holds a counted
        # stack reference to it (including a frame that refers to itself), that
        # stack reference has to be rewritten so it no longer participates in
        # reference counting. This keeps the interpreter's accounting for the
        # (now immortal) object consistent as those frames keep running and
        # eventually unwind.
        script = """if True:
        import _testcapi
        import gc
        import inspect
        import sys

        IMMORTAL_REFCOUNT = sys.getrefcount(None)

        def check(obj):
            assert sys._is_immortal(obj), obj
            assert sys.getrefcount(obj) == IMMORTAL_REFCOUNT, sys.getrefcount(obj)

        def push():
            frame = inspect.currentframe()
            _testcapi.py_immortalize(frame)
            _testcapi.py_immortalize(frame.f_locals)
            check(frame)

        push()

        def deeply_nested_frame():
            frame = inspect.currentframe()
            def a():
                def b():
                    def c():
                        here = frame
                        pair = [frame, frame]
                        _testcapi.py_immortalize(frame)
                        frame.f_locals["here"] = frame
                        check(frame)
                        return here, pair
                    return c()
                return b()
            return a()

        for _ in range(50):
            deeply_nested_frame()
            gc.collect()

        holder = []
        def frame_local():
            class Foo:
                pass
            frame = inspect.currentframe()
            inst = Foo()
            holder.append(inst)
            _testcapi.py_immortalize(inst)
            del Foo
            assert sys._is_immortal(frame.f_locals["inst"])
            check(inst)

        for _ in range(50):
            frame_local()
        for obj in holder:
            check(obj)
        for _ in range(10):
            gc.collect()
        """
        assert_python_ok("-c", script)


class TestInternalCAPI(unittest.TestCase):

    def test_immortal_builtins(self):
        for obj in range(-5, 1025):
            self.assertTrue(_testinternalcapi.is_static_immortal(obj))
        self.assertTrue(_testinternalcapi.is_static_immortal(None))
        self.assertTrue(_testinternalcapi.is_static_immortal(False))
        self.assertTrue(_testinternalcapi.is_static_immortal(True))
        self.assertTrue(_testinternalcapi.is_static_immortal(...))
        self.assertTrue(_testinternalcapi.is_static_immortal(()))
        for obj in range(1025, 1125):
            self.assertFalse(_testinternalcapi.is_static_immortal(obj))
        for obj in ([], {}, set()):
            self.assertFalse(_testinternalcapi.is_static_immortal(obj))


if __name__ == "__main__":
    unittest.main()
