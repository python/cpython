"""Tests for the perf trampoline in multi-threaded environments."""

import gc
import sys
import threading
import unittest
from test.support import threading_helper

NTHREADS = 10
ITERATIONS_PER_THREAD = 50


@threading_helper.requires_working_threading()
class TestPerfTrampolineThreadSafety(unittest.TestCase):

    def test_concurrent_code_compilation(self):
        """Stress test simultaneous allocation of code arenas and tracking hooks."""
        if not hasattr(sys, "activate_stack_trampoline") or not hasattr(sys, "deactivate_stack_trampoline"):
            self.skipTest("perf trampoline APIs are not supported on this platform")

        try:
            sys.activate_stack_trampoline("perf")
        except ValueError as exc:
            self.skipTest(f"perf trampoline activation failed: {exc}")
        self.addCleanup(sys.deactivate_stack_trampoline)

        barrier = threading.Barrier(NTHREADS)
        def worker():
            barrier.wait()
            for i in range(ITERATIONS_PER_THREAD):
                ns = {}
                tid = threading.get_ident()
                exec(
                    f"def func_{tid}_{i}(): return {i}\n"
                    f"result = func_{tid}_{i}()",
                    ns,
                )
                self.assertEqual(ns["result"], i)
                del ns

        threading_helper.run_concurrently(
            nthreads=NTHREADS, worker_func=worker
        )
        gc.collect()

    def test_concurrent_shared_code_execution(self):
        """Verify multiple threads safely handle entering evaluation loops for the same code object."""
        if not hasattr(sys, "activate_stack_trampoline") or not hasattr(sys, "deactivate_stack_trampoline"):
            self.skipTest("perf trampoline APIs are not supported on this platform")

        try:
            sys.activate_stack_trampoline("perf")
        except ValueError as exc:
            self.skipTest(f"perf trampoline activation failed: {exc}")
        self.addCleanup(sys.deactivate_stack_trampoline)

        barrier = threading.Barrier(NTHREADS)

        shared_ns = {}
        exec("def shared_func(): return 42", shared_ns)
        shared_func = shared_ns["shared_func"]

        def worker():
            barrier.wait()
            for _ in range(ITERATIONS_PER_THREAD):
                self.assertEqual(shared_func(), 42)

        threading_helper.run_concurrently(
            nthreads=NTHREADS, worker_func=worker
        )

        del shared_func
        del shared_ns
        gc.collect()


if __name__ == "__main__":
    unittest.main()
