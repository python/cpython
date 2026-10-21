import unittest
from test.support import Py_DEBUG, Py_GIL_DISABLED, import_helper
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

    def test_immortalize_strings(self):
        script = """if True:
        import _testcapi
        import sys

        string = "".join(["immortal", "string"])
        if sys.argv[1] == "interned":
            string = sys.intern(string)
        was_immortal = sys._is_immortal(string)
        assert _testcapi.py_immortalize(string) == int(not was_immortal)
        assert sys._is_immortal(string)

        # Exercise Unicode's consistency checks and both interning orders.
        assert string.encode() == b"immortalstring"
        assert sys.intern(string) is string
        """
        for state in ("not interned", "interned"):
            with self.subTest(state=state):
                assert_python_ok("-c", script, state)

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

    @unittest.skipUnless(Py_DEBUG, "requires a debug build")
    def test_immortalize_ref_total(self):
        # References to immortal objects must be excluded from RefTotal,
        # since subsequent decrefs will not be counted.
        script = """if True:
        import _testcapi
        import gc
        import sys

        gc.disable()

        def measure(immortalize):
            before = sys.gettotalrefcount()
            for _ in range(100):
                obj = object()
                refs = [obj, obj]
                if immortalize:
                    _testcapi.py_immortalize(obj)
                del obj, refs
            return sys.gettotalrefcount() - before

        # Warm up both paths before comparing their reference totals.
        measure(False)
        measure(True)
        mortal = measure(False)
        immortal = measure(True)
        assert immortal == mortal, (immortal, mortal)
        """
        assert_python_ok("-c", script)

    @unittest.skipUnless(Py_GIL_DISABLED, "deferred refcounting is free-threaded only")
    def test_immortalize_generator_deferred_locals(self):
        # Untracking an immortal generator must not hide its deferred locals
        # from the GC while they are still needed by the suspended frame.
        script = """if True:
        import _testcapi
        import _testinternalcapi
        import gc
        import sys
        import weakref

        gc.disable()

        def make_generator():
            class Local:
                pass

            def gen(local):
                yield
                yield local

            assert _testinternalcapi.has_deferred_refcount(Local)
            generator = gen(Local)
            next(generator)
            return generator, weakref.ref(Local)

        generator, local_ref = make_generator()
        assert local_ref() is not None
        assert _testcapi.py_immortalize(generator) == 1
        assert sys._is_immortal(generator)
        assert not gc.is_tracked(generator)

        gc.collect()
        # Check the weakref before resuming, to avoid using a dangling local.
        assert local_ref() is not None, "immortal generator lost its local"
        assert next(generator) is local_ref()
        generator.close()
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
