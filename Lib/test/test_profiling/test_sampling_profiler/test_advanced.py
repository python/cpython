"""Tests for advanced sampling profiler features (GC tracking, native frames, ProcessPoolExecutor support)."""

import io
import os
import subprocess
import tempfile
import unittest
from unittest import mock

try:
    import _remote_debugging  # noqa: F401
    import profiling.sampling
    import profiling.sampling.sample
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from test.support import (
    SHORT_TIMEOUT,
    SuppressCrashReport,
    os_helper,
    requires_remote_subprocess_debugging,
    script_helper,
)

from .helpers import close_and_unlink, test_subprocess


@requires_remote_subprocess_debugging()
class TestGCFrameTracking(unittest.TestCase):
    """Tests for GC frame tracking in the sampling profiler."""

    @classmethod
    def setUpClass(cls):
        """Create a static test script with GC frames and CPU-intensive work."""
        cls.gc_test_script = '''
import gc

class ExpensiveGarbage:
    def __init__(self):
        self.cycle = self

    def __del__(self):
        result = 0
        for i in range(100000):
            result += i * i
            if i % 1000 == 0:
                result = result % 1000000

_test_sock.sendall(b"working")
while True:
    ExpensiveGarbage()
    gc.collect()
'''

    def test_gc_frames_enabled(self):
        """Test that GC frames appear when gc tracking is enabled."""
        with (
            test_subprocess(self.gc_test_script, wait_for_working=True) as subproc,
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
        ):
            from profiling.sampling.pstats_collector import PstatsCollector
            collector = PstatsCollector(sample_interval_usec=5000, skip_idle=False)
            profiling.sampling.sample.sample(
                subproc.process.pid,
                collector,
                duration_sec=1,
                native=False,
                gc=True,
            )
            collector.print_stats(show_summary=False)

            output = captured_output.getvalue()

        # Should capture samples
        self.assertIn("Captured", output)
        self.assertIn("samples", output)

        # GC frames should be present
        self.assertIn("<GC>", output)

    def test_gc_frames_disabled(self):
        """Test that GC frames do not appear when gc tracking is disabled."""
        with (
            test_subprocess(self.gc_test_script, wait_for_working=True) as subproc,
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
        ):
            from profiling.sampling.pstats_collector import PstatsCollector
            collector = PstatsCollector(sample_interval_usec=5000, skip_idle=False)
            profiling.sampling.sample.sample(
                subproc.process.pid,
                collector,
                duration_sec=1,
                native=False,
                gc=False,
            )
            collector.print_stats(show_summary=False)

            output = captured_output.getvalue()

        # Should capture samples
        self.assertIn("Captured", output)
        self.assertIn("samples", output)

        # GC frames should NOT be present
        self.assertNotIn("<GC>", output)


@requires_remote_subprocess_debugging()
class TestNativeFrameTracking(unittest.TestCase):
    """Tests for native frame tracking in the sampling profiler."""

    @classmethod
    def setUpClass(cls):
        """Create a static test script with native frames and CPU-intensive work."""
        cls.native_test_script = """
import operator

def inner():
    for _ in range(1_000_0000):
        pass

_test_sock.sendall(b"working")
while True:
    operator.call(inner)
"""

    def test_native_frames_enabled(self):
        """Test that native frames appear when native tracking is enabled."""
        collapsed_file = tempfile.NamedTemporaryFile(
            suffix=".txt", delete=False
        )
        self.addCleanup(close_and_unlink, collapsed_file)

        with test_subprocess(self.native_test_script, wait_for_working=True) as subproc:
            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                from profiling.sampling.stack_collector import CollapsedStackCollector
                collector = CollapsedStackCollector(1000, skip_idle=False)
                profiling.sampling.sample.sample(
                    subproc.process.pid,
                    collector,
                    duration_sec=1,
                    native=True,
                )
                collector.export(collapsed_file.name)

            # Verify file was created and contains valid data
            self.assertTrue(os.path.exists(collapsed_file.name))
            self.assertGreater(os.path.getsize(collapsed_file.name), 0)

            # Check file format
            with open(collapsed_file.name, "r") as f:
                content = f.read()

        lines = content.strip().split("\n")
        self.assertGreater(len(lines), 0)

        stacks = [line.rsplit(" ", 1)[0] for line in lines]

        # Most samples should have native code in the middle of the stack:
        self.assertTrue(any(";<native>;" in stack for stack in stacks))

        # No samples should have native code at the top of the stack:
        self.assertFalse(any(stack.endswith(";<native>") for stack in stacks))

    def test_native_frames_disabled(self):
        """Test that native frames do not appear when native tracking is disabled."""
        with (
            test_subprocess(self.native_test_script, wait_for_working=True) as subproc,
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
        ):
            from profiling.sampling.pstats_collector import PstatsCollector
            collector = PstatsCollector(sample_interval_usec=5000, skip_idle=False)
            profiling.sampling.sample.sample(
                subproc.process.pid,
                collector,
                duration_sec=1,
            )
            collector.print_stats(show_summary=False)
            output = captured_output.getvalue()
        # Native frames should NOT be present:
        self.assertNotIn("<native>", output)


@requires_remote_subprocess_debugging()
class TestProcessPoolExecutorSupport(unittest.TestCase):
    """
    Test that ProcessPoolExecutor works correctly with profiling.sampling.
    """

    def test_process_pool_executor_pickle(self):
        # gh-140729: test use ProcessPoolExecutor.map() can sampling
        test_script = """
import concurrent.futures

def worker(x):
    return x * 2

if __name__ == "__main__":
    with concurrent.futures.ProcessPoolExecutor() as executor:
        results = list(executor.map(worker, [1, 2, 3]))
        print(f"Results: {results}")
"""
        with os_helper.temp_dir() as temp_dir:
            script = script_helper.make_script(
                temp_dir, "test_process_pool_executor_pickle", test_script
            )
            with SuppressCrashReport():
                with script_helper.spawn_python(
                    "-m",
                    "profiling.sampling",
                    "run",
                    "-d",
                    "5",
                    "-r",
                    "10",
                    script,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                ) as proc:
                    try:
                        stdout, stderr = proc.communicate(
                            timeout=SHORT_TIMEOUT
                        )
                    except subprocess.TimeoutExpired:
                        proc.kill()
                        stdout, stderr = proc.communicate()

        self.assertIn("Results: [2, 4, 6]", stdout)
        self.assertNotIn("Can't pickle", stderr)
