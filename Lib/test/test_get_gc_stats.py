import gc
import json
import os
import subprocess
import sys
import textwrap
import unittest

from test.support import (
    import_helper,
    SHORT_TIMEOUT,
    requires_gil_enabled,
    requires_remote_subprocess_debugging,
)

PROCESS_VM_READV_SUPPORTED = False

try:
    from _remote_debugging import PROCESS_VM_READV_SUPPORTED
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

skip_if_not_supported = unittest.skipIf(
    (
        sys.platform != "darwin"
        and sys.platform != "linux"
        and sys.platform != "win32"
    ),
    "Test only runs on Linux, Windows and MacOS",
)


@requires_gil_enabled()
@requires_remote_subprocess_debugging()
class TestGetGCStats(unittest.TestCase):

    def _run_child_process(self, all_interpreters):
        # Run the test in a subprocess to avoid side effects
        script = textwrap.dedent(f"""\
            import json
            import os
            import sys
            import _remote_debugging
            try:
                from _remote_debugging import PROCESS_VM_READV_SUPPORTED
                supported = True
            except ImportError:
                supported = False

            if supported:
                pid = int(sys.argv[1])
                gc_stats = _remote_debugging.get_gc_stats(pid, all_interpreters={all_interpreters})
                print(json.dumps(gc_stats, indent=1))
            else:
                print(json.dumps(dict([("error", "not supported")])))
            """)

        gc.collect(0)
        gc.collect(1)
        gc.collect(2)

        result = subprocess.run(
            [sys.executable, "-c", script, str(os.getpid())],
            capture_output=True,
            text=True,
            timeout=SHORT_TIMEOUT,
        )
        self.assertEqual(
            result.returncode, 0,
            f"stdout: {result.stdout}\nstderr: {result.stderr}"
        )
        data = json.loads(result.stdout)
        if isinstance(data, dict) and "error" in data:
            if sys.platform == "linux":
                self.skipTest("Testing on Linux requires process_vm_readv support")
            else:
                self.assertTrue(False, f"Unexpected error: {data}")
        return data

    def _run_in_interpreter(self, interp):
        source = f"""if True:
        import gc

        gc.collect(0)
        gc.collect(1)
        gc.collect(2)
        """
        interp.exec(source)

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

    @skip_if_not_supported
    @unittest.skipIf(
        sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
        "Test only runs on Linux with process_vm_readv support",
    )
    def test_get_gc_stats_for_main_interpreter(self):
        before_stats = self._run_child_process(False)
        after_stats = self._run_child_process(False)

        before_iids = get_interpreter_identifiers(before_stats)
        after_iids = get_interpreter_identifiers(after_stats)

        self.assertEqual(before_iids, (0,))
        self.assertEqual(after_iids, (0,))

        before_gens = get_generations(before_stats)
        after_gens = get_generations(after_stats)

        self.assertEqual(before_gens, (0, 1, 2))
        self.assertEqual(after_gens, (0, 1, 2))

        iid = 0 # main interpreter ID
        before_last_items = (get_last_item(before_stats, 0, iid),
                             get_last_item(before_stats, 1, iid),
                             get_last_item(before_stats, 2, iid))

        after_last_items = (get_last_item(after_stats, 0, iid),
                            get_last_item(after_stats, 1, iid),
                            get_last_item(after_stats, 2, iid))

        for before, after in zip(before_last_items, after_last_items):
            self._check_gc_state(before, after)

    def test_get_gc_stats_for_all_interpreters(self):
        interpreters = import_helper.import_module("concurrent.interpreters")
        interp = interpreters.create()

        self._run_in_interpreter(interp) # ensure that subinterpeter have GC stats
        before_stats = self._run_child_process(True)
        self._run_in_interpreter(interp) # ensure that GC stats in subinterpreter changed
        after_stats = self._run_child_process(True)
        interp.close()

        before_iids = get_interpreter_identifiers(before_stats)
        after_iids = get_interpreter_identifiers(after_stats)

        self.assertEqual(before_iids, (0, interp.id))
        self.assertEqual(after_iids, (0, interp.id))

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

    def test_get_gc_stats_for_main_interpreter_if_subinterpreter_exists(self):
        interpreters = import_helper.import_module("concurrent.interpreters")
        interp = interpreters.create()

        self._run_in_interpreter(interp) # ensure that subinterpeter have GC stats
        before_stats = self._run_child_process(False)
        self._run_in_interpreter(interp) # ensure that GC stats in subinterpreter changed
        after_stats = self._run_child_process(False)
        interp.close()

        before_iids = get_interpreter_identifiers(before_stats)
        after_iids = get_interpreter_identifiers(after_stats)

        self.assertEqual(before_iids, (0, ))
        self.assertEqual(after_iids, (0, ))

        before_gens = get_generations(before_stats)
        after_gens = get_generations(after_stats)

        self.assertEqual(before_gens, (0, 1, 2))
        self.assertEqual(after_gens, (0, 1, 2))

        iid = 0 # main interpreter ID
        before_last_items = (get_last_item(before_stats, 0, iid),
                             get_last_item(before_stats, 1, iid),
                             get_last_item(before_stats, 2, iid))

        after_last_items = (get_last_item(after_stats, 0, iid),
                            get_last_item(after_stats, 1, iid),
                            get_last_item(after_stats, 2, iid))

        for before, after in zip(before_last_items, after_last_items):
            self._check_gc_state(before, after)
