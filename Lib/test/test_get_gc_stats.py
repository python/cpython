import textwrap
import time
import unittest

from test.support import (
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


def get_interpreter_identifiers(gc_stats: tuple[dict[str, str|int|float]]) -> tuple[str,...]:
    return tuple(sorted({s["iid"] for s in gc_stats}))


def get_generations(gc_stats: tuple[dict[str, str|int|float]]) -> tuple[int,int,int]:
    generations = set()
    for s in gc_stats:
        generations.add(s["gen"])

    return tuple(sorted(generations))


def get_last_item(gc_stats: tuple[dict[str, str|int|float]],
                  generation:int,
                  iid:int) -> dict[str, str|int|float] | None:
    item = None
    for s in gc_stats:
        if s["gen"] == generation and s["iid"] == iid:
            if item is None or item["ts_start"] < s["ts_start"]:
                item = s

    return item


@requires_gil_enabled()
@requires_remote_subprocess_debugging()
class TestGetGCStats(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls._main_iid = 0 # main interpreter ID
        cls._only_main_interpreter_script = '''
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

                # objects that GC will visit should increase
                objects.append(object())

                time.sleep(0.1)
                gc.collect(0)
                gc.collect(1)
                gc.collect(2)
            '''
        cls._subinterpreters_script = '''
            import concurrent.interpreters as interpreters
            import gc
            import time

            source = """if True:
                import gc

                gc.collect(0)
                gc.collect(1)
                gc.collect(2)
            """

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

                # objects that GC will visit should increase
                objects.append(object())

                time.sleep(0.1)
                interp.exec(source)
                gc.collect(0)
                gc.collect(1)
                gc.collect(2)
            '''

    def _collect_gc_stats(self, script:str, all_interpreters:bool):
        get_gc_stats = _remote_debugging.get_gc_stats
        with (
            test_subprocess(script, wait_for_working=True) as subproc
        ):
            before_stats = get_gc_stats(subproc.process.pid,
                                        all_interpreters=all_interpreters)
            before = get_last_item(before_stats, 2, self._main_iid)
            for _ in range(10):
                time.sleep(0.5)
                after_stats = get_gc_stats(subproc.process.pid,
                                           all_interpreters=all_interpreters)
                after = get_last_item(after_stats, 2, self._main_iid)
                if after["ts_stop"] > before["ts_stop"]:
                    break

        return before_stats, after_stats

    def _check_gc_state(self, before, after):
        self.assertIsNotNone(before)
        self.assertIsNotNone(after)

        self.assertGreater(after["collections"], before["collections"], (before, after))
        self.assertGreater(after["ts_start"], before["ts_start"], (before, after))
        self.assertGreater(after["ts_stop"], before["ts_stop"], (before, after))
        self.assertGreater(after["duration"], before["duration"], (before, after))

        self.assertGreater(after["object_visits"], before["object_visits"], (before, after))
        self.assertGreater(after["candidates"], before["candidates"], (before, after))

        # may not grow
        self.assertGreaterEqual(after["collected"], before["collected"], (before, after))
        self.assertGreaterEqual(after["uncollectable"], before["uncollectable"], (before, after))

        if before["gen"] == 1:
            self.assertGreaterEqual(after["objects_transitively_reachable"],
                                    before["objects_transitively_reachable"],
                                    (before, after))
            self.assertGreaterEqual(after["objects_not_transitively_reachable"],
                                    before["objects_not_transitively_reachable"],
                                    (before, after))

    def _check_main_interpreter_stats(self, before_stats, after_stats):
        before_iids = get_interpreter_identifiers(before_stats)
        after_iids = get_interpreter_identifiers(after_stats)

        self.assertEqual(before_iids, (0,))
        self.assertEqual(after_iids, (0,))

        self.assertEqual(get_generations(before_stats), (0, 1, 2))
        self.assertEqual(get_generations(after_stats), (0, 1, 2))

        before_last_items = (get_last_item(before_stats, 0, self._main_iid),
                             get_last_item(before_stats, 1, self._main_iid),
                             get_last_item(before_stats, 2, self._main_iid))

        after_last_items = (get_last_item(after_stats, 0, self._main_iid),
                            get_last_item(after_stats, 1, self._main_iid),
                            get_last_item(after_stats, 2, self._main_iid))

        for before, after in zip(before_last_items, after_last_items):
            self._check_gc_state(before, after)

    def test_get_gc_stats_for_main_interpreter(self):
        script = textwrap.dedent(self._only_main_interpreter_script)
        before_stats, after_stats = self._collect_gc_stats(script, False)

        self._check_main_interpreter_stats(before_stats,after_stats)

    def test_get_gc_stats_for_main_interpreter_if_subinterpreter_exists(self):
        script = textwrap.dedent(self._subinterpreters_script)
        before_stats, after_stats = self._collect_gc_stats(script, False)

        self._check_main_interpreter_stats(before_stats,after_stats)

    def test_get_gc_stats_for_all_interpreters(self):
        script = textwrap.dedent(self._subinterpreters_script)
        before_stats, after_stats = self._collect_gc_stats(script, True)

        before_iids = get_interpreter_identifiers(before_stats)
        after_iids = get_interpreter_identifiers(after_stats)

        self.assertGreater(len(before_iids), 1)
        self.assertGreater(len(after_iids), 1)
        self.assertEqual(before_iids, after_iids)

        before_gens = get_generations(before_stats)
        after_gens = get_generations(after_stats)

        self.assertEqual(before_gens, (0, 1, 2))
        self.assertEqual(after_gens, (0, 1, 2))

        for iid in after_iids:
            with self.subTest(f"iid={iid}"):
                before_last_items = (get_last_item(before_stats, 0, iid),
                                     get_last_item(before_stats, 1, iid),
                                     get_last_item(before_stats, 2, iid))

                after_last_items = (get_last_item(after_stats, 0, iid),
                                    get_last_item(after_stats, 1, iid),
                                    get_last_item(after_stats, 2, iid))

                for before, after in zip(before_last_items, after_last_items):
                    self._check_gc_state(before, after)
