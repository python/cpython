import unittest
from test.support import Py_DEBUG, Py_GIL_DISABLED, import_helper, threading_helper, skip_if_sanitizer
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

        objects = [object(), [], {"item": []}, set(), bytes([1, 2])]
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

    def test_immortalize_dict_merge(self):
        import_helper.import_module('_testlimitedcapi')
        script = """if True:
        import _testcapi
        import _testlimitedcapi
        import gc

        mapping = {}
        assert _testcapi.py_immortalize(mapping) == 1
        assert not gc.is_tracked(mapping)
        # Both no-op and nonempty merges accept an untracked immortal dict.
        assert _testlimitedcapi.dict_update(mapping, {}) == 0
        assert _testlimitedcapi.dict_update(mapping, {"item": []}) == 0
        assert _testlimitedcapi.dict_merge(mapping, {"other": 42}, 1) == 0
        assert mapping == {"item": [], "other": 42}
        """
        assert_python_ok("-c", script)

    @unittest.skipIf(Py_GIL_DISABLED, "mortal stack reference optimization is GIL-only")
    def test_immortalize_updates_live_frames(self):
        script = """if True:
        import _testcapi
        import sys

        def check(obj, clear=False):
            assert _testcapi.py_immortalize(obj) == 1
            before = sys.getrefcount(obj)
            for _ in range(100):
                # Force LOAD_FAST_CHECK, which duplicates the old counted
                # reference, rather than borrowing it with LOAD_FAST_BORROW.
                if clear:
                    del obj
                refs = [obj]
            after = sys.getrefcount(obj)
            # BUILD_LIST steals the duplicate, but list destruction skips
            # decrefing immortal items. Unbounded increments can overflow.
            assert after == before, (before, after)

        check(object())
        """
        assert_python_ok("-c", script)

    @unittest.skipIf(Py_GIL_DISABLED, "mortal stack reference optimization is GIL-only")
    @threading_helper.requires_working_threading()
    def test_immortalize_other_thread_frames(self):
        script = """if True:
        import _testcapi
        import sys
        import threading
        from test.support import threading_helper

        ready = threading.Event()
        resume = threading.Event()
        results = []
        obj = object()

        def worker(local, clear=False):
            ready.set()
            resume.wait()
            before = sys.getrefcount(local)
            for _ in range(100):
                if clear:
                    del local
                refs = [local]
            results.append((before, sys.getrefcount(local)))

        thread = threading.Thread(target=worker, args=(obj,))
        with threading_helper.start_threads([thread]):
            ready.wait()
            try:
                assert _testcapi.py_immortalize(obj) == 1
            finally:
                resume.set()
        assert len(results) == 1
        before, after = results[0]
        assert before == after, results
        """
        assert_python_ok("-c", script)

    @unittest.skipIf(Py_GIL_DISABLED, "mortal stack reference optimization is GIL-only")
    def test_immortalize_suspended_frames(self):
        script = """if True:
        import _testcapi
        import sys

        def gen(obj, clear=False):
            yield
            before = sys.getrefcount(obj)
            for _ in range(100):
                if clear:
                    del obj
                refs = [obj]
            yield before, sys.getrefcount(obj)

        for immortal_generator in (False, True):
            obj = object()
            generator = gen(obj)
            next(generator)
            if immortal_generator:
                _testcapi.py_immortalize(generator)
            assert _testcapi.py_immortalize(obj) == 1
            before, after = next(generator)
            assert before == after, (before, after)
            generator.close()
        """
        assert_python_ok("-c", script)

    @unittest.skipUnless(Py_GIL_DISABLED, "deferred refcounting is free-threaded only")
    def test_immortalize_keeps_deferred_refcount(self):
        script = """if True:
        import _testcapi
        import _testinternalcapi
        import gc
        import sys
        import types

        def func():
            return 42

        class Heap:
            attr = 42

        targets = [func, func.__code__, Heap, types.ModuleType("immortal"), []]
        _testcapi.pyobject_enable_deferred_refcount(targets[-1])
        for obj in targets:
            assert _testinternalcapi.has_deferred_refcount(obj)
            assert _testcapi.py_immortalize(obj) == 1
            assert _testinternalcapi.has_deferred_refcount(obj)
            assert sys._is_immortal(obj)
            assert not gc.is_tracked(obj)
        gc.collect()
        assert func() == Heap.attr == 42
        """
        assert_python_ok("-c", script)

    @unittest.skipUnless(Py_GIL_DISABLED, "requires free threading")
    @threading_helper.requires_working_threading()
    def test_immortalize_concurrent(self):
        script = """if True:
        import _testcapi
        import threading
        from test.support import threading_helper

        objects = [object() for _ in range(25)]
        barrier = threading.Barrier(2)
        results = [[], []]
        def worker(index):
            for obj in objects:
                barrier.wait()
                results[index].append(_testcapi.py_immortalize(obj))

        threads = [threading.Thread(target=worker, args=(i,)) for i in range(2)]
        with threading_helper.start_threads(threads):
            pass
        assert all(len(result) == len(objects) for result in results)
        assert all(sorted(pair) == [0, 1] for pair in zip(*results)), results
        """
        assert_python_ok("-c", script)

    @unittest.skipUnless(Py_GIL_DISABLED, "per-thread refcounting is free-threaded only")
    @threading_helper.requires_working_threading()
    def test_immortalize_flushes_other_thread_refcounts(self):
        script = """if True:
        import _testcapi
        import gc
        import threading
        import types
        import weakref
        from test.support import threading_helper

        gc.disable()
        code = compile("pass", "<immortal-code>", "exec")
        ready = threading.Event()
        release = threading.Event()

        def worker():
            functions = [types.FunctionType(code, {}) for _ in range(100)]
            ready.set()
            release.wait()
            functions.clear()

        thread = threading.Thread(target=worker)
        with threading_helper.start_threads([thread]):
            ready.wait()
            try:
                assert _testcapi.py_immortalize(code) == 1
                # The replacement reuses the id just released by code.
                victim = code.replace()
                victim_ref = weakref.ref(victim)
                gc.collect()
                del victim
            finally:
                release.set()

        gc.collect()
        # Pending counts from the worker must not keep the victim alive.
        assert victim_ref() is None, "stale per-thread counts reached a reused id"
        """
        assert_python_ok("-c", script)

    def test_immortalize_rejects_strings(self):
        script = """if True:
        import _testcapi
        import gc
        import sys

        string = "".join(["immortal", "string"])
        if sys.argv[1] == "interned":
            string = sys.intern(string)
        elif sys.argv[1] == "subclass":
            class String(str):
                pass
            string = String(string)
        was_immortal = sys._is_immortal(string)
        was_tracked = gc.is_tracked(string)
        if sys.argv[1] != "interned":
            assert not was_immortal
        assert _testcapi.py_immortalize(string) == 0
        assert sys._is_immortal(string) == was_immortal
        assert gc.is_tracked(string) == was_tracked

        # Exercise Unicode's consistency checks and both interning orders.
        assert string.encode() == b"immortalstring"
        if sys.argv[1] != "subclass":
            assert sys.intern(string) is string
        """
        for state in ("not interned", "interned", "subclass"):
            with self.subTest(state=state):
                assert_python_ok("-c", script, state)

    @skip_if_sanitizer(thread=True)
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

    @unittest.skipUnless(Py_DEBUG and Py_GIL_DISABLED, "requires a free-threaded debug build")
    def test_immortalize_per_thread_ref_total(self):
        script = """if True:
        import _testcapi
        import gc
        import sys

        gc.disable()
        class Heap:
            pass

        instances = [Heap() for _ in range(100)]
        controls = [object() for _ in range(100)]
        assert _testcapi.py_immortalize(Heap) == 1

        def measure_drop(objects):
            before = sys.gettotalrefcount()
            objects.clear()
            return sys.gettotalrefcount() - before

        measure_drop([])
        immortal_type = measure_drop(instances)
        static_type = measure_drop(controls)
        assert immortal_type == static_type, (immortal_type, static_type)
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
                later = yield
                yield weakref.ref(later)
                yield later

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
        next(generator)
        if sys.argv[1] == "frozen":
            gc.freeze()
        class Later:
            pass
        assert _testinternalcapi.has_deferred_refcount(Later)
        # send() stores a new deferred reference after immortalization.
        later_ref = generator.send(Later)
        del Later
        gc.collect()
        assert later_ref() is not None, "resumed immortal generator lost its local"
        assert next(generator) is later_ref()
        generator.close()
        """
        for state in ("normal", "frozen"):
            with self.subTest(state=state):
                assert_python_ok("-c", script, state)

    @unittest.skipUnless(Py_GIL_DISABLED, "deferred refcounting is free-threaded only")
    def test_immortalize_frame_deferred_locals(self):
        script = """if True:
        import _testcapi
        import gc
        import sys
        import weakref

        def make_frame():
            frame = sys._getframe()
            assert _testcapi.py_immortalize(frame) == 1
            # LOAD_GLOBAL creates a deferred reference after immortalization.
            local = Local
            return frame, weakref.ref(local)

        class Local:
            pass
        frame, local_ref = make_frame()
        del Local
        gc.collect()
        assert local_ref() is not None, "immortal frame lost its local"
        assert frame.f_locals["local"] is local_ref()
        frame.clear()
        """
        assert_python_ok("-c", script)

    def test_immortalize_subinterpreters(self):
        # Each interpreter owns and releases its frame registry independently.
        script = """if True:
        import _testcapi
        import gc

        code = '''if True:
            import _testcapi
            import gc
            import sys

            def gen():
                yield object()

            generator = gen()
            next(generator)
            assert _testcapi.py_immortalize(generator) == 1

            def make_frame():
                frame = sys._getframe()
                assert _testcapi.py_immortalize(frame) == 1
                return frame

            frame = make_frame()
            gc.collect()
        '''
        for _ in range(3):
            assert _testcapi.run_in_subinterp(code) == 0
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
