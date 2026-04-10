import gc
import json
import os
import subprocess
import sys
import textwrap
import unittest

from test.support import (
    SHORT_TIMEOUT,
    requires_remote_subprocess_debugging,
)


def get_interpreter_identifiers(gc_stats: tuple[dict[str, str|int|float]]) -> list[str]:
    return [s["iid"] for s in gc_stats]


def get_generations(gc_stats: tuple[dict[str, str|int|float]]) -> tuple[int,int,int]:
    generations = set()
    for s in gc_stats:
        generations.add(s["gen"])

    return tuple(sorted(generations))


def get_last_item_for_generation(gc_stats: tuple[dict[str, str|int|float]],
                                 generation:int) -> dict[str, str|int|float] | None:
    item = None
    for s in gc_stats:
        if s["gen"] == generation:
            if item is None or item["ts_start"] < s["ts_start"]:
                item = s

    return item



@requires_remote_subprocess_debugging()
class TestGetStackTrace(unittest.TestCase):

    def run_child_process(self):
        # Run the test in a subprocess to avoid side effects
        script = textwrap.dedent("""\
            import json
            import os
            import sys
            import _remote_debugging

            pid = int(sys.argv[1])
            gc_stats = _remote_debugging.get_gc_stats(pid, all_interpreters=False)
            print(json.dumps(gc_stats, indent=1))
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
        return result

    def test_get_gc_stats_for_main_interpreter(self):
        """Test that RemoteUnwinder works on the same process after _ctypes import.

        When _ctypes is imported, it may call dlopen on the libpython shared
        library, creating a duplicate mapping in the process address space.
        The remote debugging code must skip these uninitialized duplicate
        mappings and find the real PyRuntime. See gh-144563.
        """

        # Skip the test if the _ctypes module is missing.

        before_stats = json.loads(self.run_child_process().stdout)
        after_stats = json.loads(self.run_child_process().stdout)

        before_iids = get_interpreter_identifiers(before_stats)
        after_iids = get_interpreter_identifiers(after_stats)

        self.assertTrue(all([0 == iid for iid in before_iids]))
        self.assertTrue(all([0 == iid for iid in after_iids]))

        before_gens = get_generations(before_stats)
        after_gens = get_generations(after_stats)

        self.assertEqual(before_gens, (0, 1, 2))
        self.assertEqual(after_gens, (0, 1, 2))

        before_last_items = (get_last_item_for_generation(before_stats, 0),
                             get_last_item_for_generation(before_stats, 1),
                             get_last_item_for_generation(before_stats, 2))

        after_last_items = (get_last_item_for_generation(after_stats, 0),
                            get_last_item_for_generation(after_stats, 1),
                            get_last_item_for_generation(after_stats, 2))

        for before, after in zip(before_last_items, after_last_items):
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
                self.assertGreaterEqual(after["objects_transitively_reachable"], before["objects_transitively_reachable"], (before, after))
                self.assertGreaterEqual(after["objects_not_transitively_reachable"], before["objects_not_transitively_reachable"], (before, after))
