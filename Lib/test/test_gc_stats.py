import gc
import os
import textwrap
import time
import unittest

from test.support import (
    Py_GIL_DISABLED,
    import_helper,
    requires_gil_enabled,
    requires_remote_subprocess_debugging,
)
from test.test_profiling.test_sampling_profiler.helpers import test_subprocess

try:
    import _remote_debugging  # noqa: F401
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )


GC_STATS_FIELDS = (
    "gen", "iid", "ts_start", "ts_stop", "collections", "collected",
    "uncollectable", "candidates", "duration")


def get_interpreter_identifiers(gc_stats) -> tuple[int,...]:
    return tuple(sorted({s.iid for s in gc_stats}))


def get_generations(gc_stats) -> tuple[int,int,int]:
    generations = set()
    for s in gc_stats:
        generations.add(s.gen)

    return tuple(sorted(generations))


def get_last_item(gc_stats, generation: int, iid: int):
    item = None
    for s in gc_stats:
        if s.gen == generation and s.iid == iid:
            if item is None or item.ts_start < s.ts_start:
                item = s

    return item


def has_local_process_debugging():
    try:
        return _remote_debugging.is_python_process(os.getpid())
    except Exception:
        return False


def check_gc_stats_fields(testcase, stats):
    testcase.assertIsInstance(stats, list)
    testcase.assertGreater(len(stats), 0)
    for item in stats:
        testcase.assertIsInstance(item, _remote_debugging.GCStatsInfo)
        testcase.assertEqual(type(item).__match_args__, GC_STATS_FIELDS)
        testcase.assertEqual(len(item), len(GC_STATS_FIELDS))


def gc_stats_counters_advanced(before_stats, after_stats, generations, iid):
    for generation in generations:
        before = get_last_item(before_stats, generation, iid)
        after = get_last_item(after_stats, generation, iid)
        if after is None or before is None:
            return False
        if after.duration <= before.duration:
            return False
        if after.candidates <= before.candidates:
            return False
    return True


@unittest.skipUnless(
    has_local_process_debugging(), "requires local process debugging")
class TestLocalGCStats(unittest.TestCase):

    _main_iid = 0  # main interpreter ID

    def test_gc_stats_fields(self):
        monitor = _remote_debugging.GCMonitor(os.getpid(), debug=True)
        stats = monitor.get_gc_stats(all_interpreters=False)
        check_gc_stats_fields(self, stats)

    def test_module_get_gc_stats_fields(self):
        stats = _remote_debugging.get_gc_stats(
            os.getpid(), all_interpreters=False)
        check_gc_stats_fields(self, stats)

    def test_all_interpreters_filter_for_local_process(self):
        interpreters = import_helper.import_module("concurrent.interpreters")
        source = """
            import gc
            objects = []
            obj = []
            obj.append(obj)
            objects.append(obj)
            gc.collect(0)
            gc.collect(1)
            gc.collect(2)
        """
        interp = interpreters.create()
        try:
            interp.exec(textwrap.dedent(source))
            for generation in range(3):
                gc.collect(generation)

            main_stats = _remote_debugging.get_gc_stats(
                os.getpid(), all_interpreters=False)
            all_stats = _remote_debugging.get_gc_stats(
                os.getpid(), all_interpreters=True)
        finally:
            interp.close()

        self.assertEqual(get_interpreter_identifiers(main_stats), (0,))
        self.assertIn(0, get_interpreter_identifiers(all_stats))
        self.assertGreater(len(get_interpreter_identifiers(all_stats)), 1)
        self.assertEqual(get_generations(main_stats), (0, 1, 2))
        self.assertEqual(get_generations(all_stats), (0, 1, 2))
        for iid in get_interpreter_identifiers(all_stats):
            for generation in range(3):
                self.assertIsNotNone(get_last_item(all_stats, generation, iid))

    @unittest.skipUnless(Py_GIL_DISABLED, "requires free-threaded GC")
    def test_gc_stats_counters_for_main_interpreter_free_threaded(self):
        generations = (0, 1, 2)
        before_stats = _remote_debugging.get_gc_stats(
            os.getpid(), all_interpreters=False)
        for generation in generations:
            self.assertIsNotNone(
                get_last_item(before_stats, generation, self._main_iid))

        objects = []
        for _ in range(1000):
            obj = []
            obj.append(obj)
            objects.append(obj)
        for generation in generations:
            gc.collect(generation)

        after_stats = _remote_debugging.get_gc_stats(
            os.getpid(), all_interpreters=False)
        self.assertTrue(
            gc_stats_counters_advanced(
                before_stats, after_stats, generations, self._main_iid),
            (before_stats, after_stats)
        )


@requires_remote_subprocess_debugging()
class TestGCStats(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls._main_iid = 0 # main interpreter ID
        cls._main_interpreter_script = '''
            import gc
            import time

            gc.collect(0)
            gc.collect(1)
            gc.collect(2)

            _test_sock.sendall(b"working")
            objects = []
            while True:
                if len(objects) > 100:
                    objects = []

                obj = []
                obj.append(obj)
                objects.append(obj)

                time.sleep(0.1)
                gc.collect(0)
                gc.collect(1)
                gc.collect(2)
            '''
        cls._script = '''
            import concurrent.interpreters as interpreters
            import gc
            import time

            source = """if True:
                import gc

                if "objects" not in globals():
                    objects = []
                if len(objects) > 100:
                    objects = []

                obj = []
                obj.append(obj)
                objects.append(obj)

                gc.collect(0)
                gc.collect(1)
                gc.collect(2)
            """

            if {0}:
                interp = interpreters.create()
                interp.exec(source)

            gc.collect(0)
            gc.collect(1)
            gc.collect(2)

            _test_sock.sendall(b"working")
            objects = []
            while True:
                if len(objects) > 100:
                    objects = []

                obj = []
                obj.append(obj)
                objects.append(obj)

                time.sleep(0.1)
                if {0}:
                    interp.exec(source)
                gc.collect(0)
                gc.collect(1)
                gc.collect(2)
            '''

    def _gc_stats_advanced(self, before_stats, after_stats, generations):
        for generation in generations:
            before = get_last_item(before_stats, generation, self._main_iid)
            after = get_last_item(after_stats, generation, self._main_iid)
            if after is None or before is None:
                return False
            if after.ts_stop <= before.ts_stop:
                return False
        return True

    def _collect_gc_stats(self, script: str, all_interpreters: bool,
                          generations=(2,)):
        with (test_subprocess(script, wait_for_working=True) as subproc):
            monitor = _remote_debugging.GCMonitor(subproc.process.pid, debug=True)
            before_stats = monitor.get_gc_stats(all_interpreters=all_interpreters)
            for generation in generations:
                before = get_last_item(before_stats, generation, self._main_iid)
                self.assertIsNotNone(before)

            after_stats = before_stats
            for _ in range(10):
                time.sleep(0.5)
                after_stats = monitor.get_gc_stats(all_interpreters=all_interpreters)
                if self._gc_stats_advanced(before_stats, after_stats, generations):
                    break
            else:
                self.fail(
                    f"GC stats for generations {generations!r} did not "
                    f"advance: {before_stats!r} -> {after_stats!r}"
                )

        return before_stats, after_stats

    def _check_gc_stats(self, before, after):
        self.assertIsNotNone(before)
        self.assertIsNotNone(after)

        self.assertGreater(after.collections, before.collections, (before, after))
        self.assertGreater(after.ts_start, before.ts_start, (before, after))
        self.assertGreater(after.ts_stop, before.ts_stop, (before, after))
        self.assertGreater(after.duration, before.duration, (before, after))

        self.assertGreater(after.candidates, before.candidates, (before, after))

        # may not grow
        self.assertGreaterEqual(after.collected, before.collected, (before, after))
        self.assertGreaterEqual(after.uncollectable, before.uncollectable, (before, after))

    def _check_interpreter_gc_stats(self, before_stats, after_stats):
        before_iids = get_interpreter_identifiers(before_stats)
        after_iids = get_interpreter_identifiers(after_stats)

        self.assertEqual(before_iids, after_iids)

        self.assertEqual(get_generations(before_stats), (0, 1, 2))
        self.assertEqual(get_generations(after_stats), (0, 1, 2))

        for iid in after_iids:
            with self.subTest(f"interpreter id={iid}"):
                before_last_items = (get_last_item(before_stats, 0, iid),
                                     get_last_item(before_stats, 1, iid),
                                     get_last_item(before_stats, 2, iid))

                after_last_items = (get_last_item(after_stats, 0, iid),
                                    get_last_item(after_stats, 1, iid),
                                    get_last_item(after_stats, 2, iid))

                for before, after in zip(before_last_items, after_last_items):
                    self._check_gc_stats(before, after)

    def test_gc_stats_timestamps_for_main_interpreter(self):
        script = textwrap.dedent(self._main_interpreter_script)
        before_stats, after_stats = self._collect_gc_stats(
            script, False, generations=(0, 1, 2))

        for generation in range(3):
            with self.subTest(generation=generation):
                before = get_last_item(before_stats, generation, self._main_iid)
                after = get_last_item(after_stats, generation, self._main_iid)

                self.assertIsNotNone(before)
                self.assertIsNotNone(after)
                self.assertGreater(
                    after.collections, before.collections,
                    (before, after))
                self.assertGreater(
                    after.ts_start, before.ts_start,
                    (before, after))
                self.assertGreater(
                    after.ts_stop, before.ts_stop,
                    (before, after))

    @requires_gil_enabled()
    def test_gc_stats_for_main_interpreter(self):
        script = textwrap.dedent(self._script.format(False))
        before_stats, after_stats = self._collect_gc_stats(script, False)

        self._check_interpreter_gc_stats(before_stats, after_stats)

    @requires_gil_enabled()
    def test_gc_stats_for_main_interpreter_if_subinterpreter_exists(self):
        script = textwrap.dedent(self._script.format(True))
        before_stats, after_stats = self._collect_gc_stats(script, False)

        self._check_interpreter_gc_stats(before_stats, after_stats)

    @requires_gil_enabled()
    def test_gc_stats_for_all_interpreters(self):
        script = textwrap.dedent(self._script.format(True))
        before_stats, after_stats = self._collect_gc_stats(script, True)

        before_iids = get_interpreter_identifiers(before_stats)
        after_iids = get_interpreter_identifiers(after_stats)

        self.assertGreater(len(before_iids), 1)
        self.assertGreater(len(after_iids), 1)
        self.assertEqual(before_iids, after_iids)

        self._check_interpreter_gc_stats(before_stats, after_stats)
