"""Tests perf trampoline in a multi-threaded environment to verify thread safety in free-threaded builds."""

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
        if not hasattr(sys, "_perf_trampoline_init") or not hasattr(sys, "_perf_trampoline_fini"):
            self.skipTest("perf trampoline APIs are not supported on this platform")

        try:
            sys._perf_trampoline_init(1)
        except ValueError as exc:
            self.skipTest(f"perf trampoline activation failed: {exc}")
        self.addCleanup(sys._perf_trampoline_fini)

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
                del ns  # Immediate drop to prevent delaying cleanup, no internal gc.collect()

        threading_helper.run_concurrently(
            nthreads=NTHREADS, worker_func=worker
        )
        # Single final sweep to ensure code objects and extra tracking clear cleanly
        gc.collect()

    def test_concurrent_shared_code_execution(self):
        """Verify multiple threads safely handle entering evaluation loops for the same code object."""
        if not hasattr(sys, "_perf_trampoline_init") or not hasattr(sys, "_perf_trampoline_fini"):
            self.skipTest("perf trampoline APIs are not supported on this platform")

        try:
            sys._perf_trampoline_init(1)
        except ValueError as exc:
            self.skipTest(f"perf trampoline activation failed: {exc}")
        self.addCleanup(sys._perf_trampoline_fini)

        barrier = threading.Barrier(NTHREADS)

        # Pre-compile a single function instance to share across all threads
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
        # Single final sweep to clean up shared resources safely
        gc.collect()


if __name__ == "__main__":
    unittest.main()