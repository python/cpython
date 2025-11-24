"""Tests for sampling profiler integration and error handling."""

import contextlib
import io
import marshal
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

try:
    import _remote_debugging
    import profiling.sampling
    import profiling.sampling.sample
    from profiling.sampling.pstats_collector import PstatsCollector
    from profiling.sampling.stack_collector import CollapsedStackCollector
    from profiling.sampling.sample import SampleProfiler
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from test.support import (
    requires_subprocess,
    SHORT_TIMEOUT,
)

from .helpers import (
    test_subprocess,
    close_and_unlink,
    skip_if_not_supported,
    PROCESS_VM_READV_SUPPORTED,
)
from .mocks import MockFrameInfo, MockThreadInfo, MockInterpreterInfo

# Duration for profiling tests - long enough for process to complete naturally
PROFILING_TIMEOUT = str(int(SHORT_TIMEOUT))


@skip_if_not_supported
@unittest.skipIf(
    sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
    "Test only runs on Linux with process_vm_readv support",
)
class TestRecursiveFunctionProfiling(unittest.TestCase):
    """Test profiling of recursive functions and complex call patterns."""

    def test_recursive_function_call_counting(self):
        """Test that recursive function calls are counted correctly."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Simulate a recursive call pattern: fibonacci(5) calling itself
        recursive_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [  # First sample: deep in recursion
                            MockFrameInfo("fib.py", 10, "fibonacci"),
                            MockFrameInfo(
                                "fib.py", 10, "fibonacci"
                            ),  # recursive call
                            MockFrameInfo(
                                "fib.py", 10, "fibonacci"
                            ),  # deeper recursion
                            MockFrameInfo(
                                "fib.py", 10, "fibonacci"
                            ),  # even deeper
                            MockFrameInfo("main.py", 5, "main"),  # main caller
                        ],
                    )
                ],
            ),
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [  # Second sample: different recursion depth
                            MockFrameInfo("fib.py", 10, "fibonacci"),
                            MockFrameInfo(
                                "fib.py", 10, "fibonacci"
                            ),  # recursive call
                            MockFrameInfo("main.py", 5, "main"),  # main caller
                        ],
                    )
                ],
            ),
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [  # Third sample: back to deeper recursion
                            MockFrameInfo("fib.py", 10, "fibonacci"),
                            MockFrameInfo("fib.py", 10, "fibonacci"),
                            MockFrameInfo("fib.py", 10, "fibonacci"),
                            MockFrameInfo("main.py", 5, "main"),
                        ],
                    )
                ],
            ),
        ]

        for frames in recursive_frames:
            collector.collect([frames])

        collector.create_stats()

        # Check that recursive calls are counted properly
        fib_key = ("fib.py", 10, "fibonacci")
        main_key = ("main.py", 5, "main")

        self.assertIn(fib_key, collector.stats)
        self.assertIn(main_key, collector.stats)

        # Fibonacci should have many calls due to recursion
        fib_stats = collector.stats[fib_key]
        direct_calls, cumulative_calls, tt, ct, callers = fib_stats

        # Should have recorded multiple calls (9 total appearances in samples)
        self.assertEqual(cumulative_calls, 9)
        self.assertGreater(tt, 0)  # Should have some total time
        self.assertGreater(ct, 0)  # Should have some cumulative time

        # Main should have fewer calls
        main_stats = collector.stats[main_key]
        main_direct_calls, main_cumulative_calls = main_stats[0], main_stats[1]
        self.assertEqual(main_direct_calls, 0)  # Never directly executing
        self.assertEqual(main_cumulative_calls, 3)  # Appears in all 3 samples

    def test_nested_function_hierarchy(self):
        """Test profiling of deeply nested function calls."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Simulate a deep call hierarchy
        deep_call_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("level1.py", 10, "level1_func"),
                            MockFrameInfo("level2.py", 20, "level2_func"),
                            MockFrameInfo("level3.py", 30, "level3_func"),
                            MockFrameInfo("level4.py", 40, "level4_func"),
                            MockFrameInfo("level5.py", 50, "level5_func"),
                            MockFrameInfo("main.py", 5, "main"),
                        ],
                    )
                ],
            ),
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [  # Same hierarchy sampled again
                            MockFrameInfo("level1.py", 10, "level1_func"),
                            MockFrameInfo("level2.py", 20, "level2_func"),
                            MockFrameInfo("level3.py", 30, "level3_func"),
                            MockFrameInfo("level4.py", 40, "level4_func"),
                            MockFrameInfo("level5.py", 50, "level5_func"),
                            MockFrameInfo("main.py", 5, "main"),
                        ],
                    )
                ],
            ),
        ]

        for frames in deep_call_frames:
            collector.collect([frames])

        collector.create_stats()

        # All levels should be recorded
        for level in range(1, 6):
            key = (f"level{level}.py", level * 10, f"level{level}_func")
            self.assertIn(key, collector.stats)

            stats = collector.stats[key]
            direct_calls, cumulative_calls, tt, ct, callers = stats

            # Each level should appear in stack twice (2 samples)
            self.assertEqual(cumulative_calls, 2)

            # Only level1 (deepest) should have direct calls
            if level == 1:
                self.assertEqual(direct_calls, 2)
            else:
                self.assertEqual(direct_calls, 0)

            # Deeper levels should have lower cumulative time than higher levels
            # (since they don't include time from functions they call)
            if level == 1:  # Deepest level with most time
                self.assertGreater(ct, 0)

    def test_alternating_call_patterns(self):
        """Test profiling with alternating call patterns."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Simulate alternating execution paths
        pattern_frames = [
            # Pattern A: path through func_a
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("module.py", 10, "func_a"),
                            MockFrameInfo("module.py", 30, "shared_func"),
                            MockFrameInfo("main.py", 5, "main"),
                        ],
                    )
                ],
            ),
            # Pattern B: path through func_b
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("module.py", 20, "func_b"),
                            MockFrameInfo("module.py", 30, "shared_func"),
                            MockFrameInfo("main.py", 5, "main"),
                        ],
                    )
                ],
            ),
            # Pattern A again
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("module.py", 10, "func_a"),
                            MockFrameInfo("module.py", 30, "shared_func"),
                            MockFrameInfo("main.py", 5, "main"),
                        ],
                    )
                ],
            ),
            # Pattern B again
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("module.py", 20, "func_b"),
                            MockFrameInfo("module.py", 30, "shared_func"),
                            MockFrameInfo("main.py", 5, "main"),
                        ],
                    )
                ],
            ),
        ]

        for frames in pattern_frames:
            collector.collect([frames])

        collector.create_stats()

        # Check that both paths are recorded equally
        func_a_key = ("module.py", 10, "func_a")
        func_b_key = ("module.py", 20, "func_b")
        shared_key = ("module.py", 30, "shared_func")
        main_key = ("main.py", 5, "main")

        # func_a and func_b should each be directly executing twice
        self.assertEqual(collector.stats[func_a_key][0], 2)  # direct_calls
        self.assertEqual(collector.stats[func_a_key][1], 2)  # cumulative_calls
        self.assertEqual(collector.stats[func_b_key][0], 2)  # direct_calls
        self.assertEqual(collector.stats[func_b_key][1], 2)  # cumulative_calls

        # shared_func should appear in all samples (4 times) but never directly executing
        self.assertEqual(collector.stats[shared_key][0], 0)  # direct_calls
        self.assertEqual(collector.stats[shared_key][1], 4)  # cumulative_calls

        # main should appear in all samples but never directly executing
        self.assertEqual(collector.stats[main_key][0], 0)  # direct_calls
        self.assertEqual(collector.stats[main_key][1], 4)  # cumulative_calls

    def test_collapsed_stack_with_recursion(self):
        """Test collapsed stack collector with recursive patterns."""
        collector = CollapsedStackCollector(1000)

        # Recursive call pattern
        recursive_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            ("factorial.py", 10, "factorial"),
                            ("factorial.py", 10, "factorial"),  # recursive
                            ("factorial.py", 10, "factorial"),  # deeper
                            ("main.py", 5, "main"),
                        ],
                    )
                ],
            ),
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            ("factorial.py", 10, "factorial"),
                            (
                                "factorial.py",
                                10,
                                "factorial",
                            ),  # different depth
                            ("main.py", 5, "main"),
                        ],
                    )
                ],
            ),
        ]

        for frames in recursive_frames:
            collector.collect([frames])

        # Should capture both call paths
        self.assertEqual(len(collector.stack_counter), 2)

        # First path should be longer (deeper recursion) than the second
        path_tuples = list(collector.stack_counter.keys())
        paths = [p[0] for p in path_tuples]  # Extract just the call paths
        lengths = [len(p) for p in paths]
        self.assertNotEqual(lengths[0], lengths[1])

        # Both should contain factorial calls
        self.assertTrue(
            any(any(f[2] == "factorial" for f in p) for p in paths)
        )

        # Verify total occurrences via aggregation
        factorial_key = ("factorial.py", 10, "factorial")
        main_key = ("main.py", 5, "main")

        def total_occurrences(func):
            total = 0
            for (path, thread_id), count in collector.stack_counter.items():
                total += sum(1 for f in path if f == func) * count
            return total

        self.assertEqual(total_occurrences(factorial_key), 5)
        self.assertEqual(total_occurrences(main_key), 2)


@requires_subprocess()
@skip_if_not_supported
class TestSampleProfilerIntegration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_script = '''
import time
import os

def slow_fibonacci(n):
    """Recursive fibonacci - should show up prominently in profiler."""
    if n <= 1:
        return n
    return slow_fibonacci(n-1) + slow_fibonacci(n-2)

def cpu_intensive_work():
    """CPU intensive work that should show in profiler."""
    result = 0
    for i in range(10000):
        result += i * i
        if i % 100 == 0:
            result = result % 1000000
    return result

def main_loop():
    """Main test loop."""
    max_iterations = 200

    for iteration in range(max_iterations):
        if iteration % 2 == 0:
            result = slow_fibonacci(15)
        else:
            result = cpu_intensive_work()

if __name__ == "__main__":
    main_loop()
'''

    def test_sampling_basic_functionality(self):
        with (
            test_subprocess(self.test_script) as subproc,
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
        ):
            try:
                # Sample for up to SHORT_TIMEOUT seconds, but process exits after fixed iterations
                collector = PstatsCollector(sample_interval_usec=1000, skip_idle=False)
                profiling.sampling.sample.sample(
                    subproc.process.pid,
                    collector,
                    duration_sec=SHORT_TIMEOUT,
                )
                collector.print_stats(show_summary=False)
            except PermissionError:
                self.skipTest("Insufficient permissions for remote profiling")

            output = captured_output.getvalue()

        # Basic checks on output
        self.assertIn("Captured", output)
        self.assertIn("samples", output)
        self.assertIn("Profile Stats", output)

        # Should see some of our test functions
        self.assertIn("slow_fibonacci", output)

    def test_sampling_with_pstats_export(self):
        pstats_out = tempfile.NamedTemporaryFile(
            suffix=".pstats", delete=False
        )
        self.addCleanup(close_and_unlink, pstats_out)

        with test_subprocess(self.test_script) as subproc:
            # Suppress profiler output when testing file export
            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    collector = PstatsCollector(sample_interval_usec=10000, skip_idle=False)
                    profiling.sampling.sample.sample(
                        subproc.process.pid,
                        collector,
                        duration_sec=1,
                    )
                    collector.export(pstats_out.name)
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

            # Verify file was created and contains valid data
            self.assertTrue(os.path.exists(pstats_out.name))
            self.assertGreater(os.path.getsize(pstats_out.name), 0)

            # Try to load the stats file
            with open(pstats_out.name, "rb") as f:
                stats_data = marshal.load(f)

            # Should be a dictionary with the sampled marker
            self.assertIsInstance(stats_data, dict)
            self.assertIn(("__sampled__",), stats_data)
            self.assertTrue(stats_data[("__sampled__",)])

            # Should have some function data
            function_entries = [
                k for k in stats_data.keys() if k != ("__sampled__",)
            ]
            self.assertGreater(len(function_entries), 0)

    def test_sampling_with_collapsed_export(self):
        collapsed_file = tempfile.NamedTemporaryFile(
            suffix=".txt", delete=False
        )
        self.addCleanup(close_and_unlink, collapsed_file)

        with (
            test_subprocess(self.test_script) as subproc,
        ):
            # Suppress profiler output when testing file export
            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    collector = CollapsedStackCollector(1000, skip_idle=False)
                    profiling.sampling.sample.sample(
                        subproc.process.pid,
                        collector,
                        duration_sec=1,
                    )
                    collector.export(collapsed_file.name)
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

            # Verify file was created and contains valid data
            self.assertTrue(os.path.exists(collapsed_file.name))
            self.assertGreater(os.path.getsize(collapsed_file.name), 0)

            # Check file format
            with open(collapsed_file.name, "r") as f:
                content = f.read()

            lines = content.strip().split("\n")
            self.assertGreater(len(lines), 0)

            # Each line should have format: stack_trace count
            for line in lines:
                parts = line.rsplit(" ", 1)
                self.assertEqual(len(parts), 2)

                stack_trace, count_str = parts
                self.assertGreater(len(stack_trace), 0)
                self.assertTrue(count_str.isdigit())
                self.assertGreater(int(count_str), 0)

                # Stack trace should contain semicolon-separated entries
                if ";" in stack_trace:
                    stack_parts = stack_trace.split(";")
                    for part in stack_parts:
                        # Each part should be file:function:line
                        self.assertIn(":", part)

    def test_sampling_all_threads(self):
        with (
            test_subprocess(self.test_script) as subproc,
            # Suppress profiler output
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
        ):
            try:
                collector = PstatsCollector(sample_interval_usec=10000, skip_idle=False)
                profiling.sampling.sample.sample(
                    subproc.process.pid,
                    collector,
                    duration_sec=1,
                    all_threads=True,
                )
                collector.print_stats(show_summary=False)
            except PermissionError:
                self.skipTest("Insufficient permissions for remote profiling")

        # Just verify that sampling completed without error
        # We're not testing output format here

    def test_sample_target_script(self):
        script_file = tempfile.NamedTemporaryFile(delete=False)
        script_file.write(self.test_script.encode("utf-8"))
        script_file.flush()
        self.addCleanup(close_and_unlink, script_file)

        # Sample for up to SHORT_TIMEOUT seconds, but process exits after fixed iterations
        test_args = ["profiling.sampling.sample", "run", "-d", PROFILING_TIMEOUT, script_file.name]

        with (
            mock.patch("sys.argv", test_args),
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
        ):
            try:
                from profiling.sampling.cli import main
                main()
            except PermissionError:
                self.skipTest("Insufficient permissions for remote profiling")

            output = captured_output.getvalue()

        # Basic checks on output
        self.assertIn("Captured", output)
        self.assertIn("samples", output)
        self.assertIn("Profile Stats", output)

        # Should see some of our test functions
        self.assertIn("slow_fibonacci", output)

    def test_sample_target_module(self):
        tempdir = tempfile.TemporaryDirectory(delete=False)
        self.addCleanup(lambda x: shutil.rmtree(x), tempdir.name)

        module_path = os.path.join(tempdir.name, "test_module.py")

        with open(module_path, "w") as f:
            f.write(self.test_script)

        test_args = [
            "profiling.sampling.cli",
            "run",
            "-d",
            PROFILING_TIMEOUT,
            "-m",
            "test_module",
        ]

        with (
            mock.patch("sys.argv", test_args),
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
            # Change to temp directory so subprocess can find the module
            contextlib.chdir(tempdir.name),
        ):
            try:
                from profiling.sampling.cli import main
                main()
            except PermissionError:
                self.skipTest("Insufficient permissions for remote profiling")

            output = captured_output.getvalue()

        # Basic checks on output
        self.assertIn("Captured", output)
        self.assertIn("samples", output)
        self.assertIn("Profile Stats", output)

        # Should see some of our test functions
        self.assertIn("slow_fibonacci", output)


@skip_if_not_supported
@unittest.skipIf(
    sys.platform == "linux" and not PROCESS_VM_READV_SUPPORTED,
    "Test only runs on Linux with process_vm_readv support",
)
class TestSampleProfilerErrorHandling(unittest.TestCase):
    def test_invalid_pid(self):
        with self.assertRaises((OSError, RuntimeError)):
            collector = PstatsCollector(sample_interval_usec=100, skip_idle=False)
            profiling.sampling.sample.sample(-1, collector, duration_sec=1)

    def test_process_dies_during_sampling(self):
        with test_subprocess(
            "import time; time.sleep(0.5); exit()"
        ) as subproc:
            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    collector = PstatsCollector(sample_interval_usec=50000, skip_idle=False)
                    profiling.sampling.sample.sample(
                        subproc.process.pid,
                        collector,
                        duration_sec=2,  # Longer than process lifetime
                    )
                except PermissionError:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

                output = captured_output.getvalue()

            self.assertIn("Error rate", output)

    def test_is_process_running(self):
        with test_subprocess("import time; time.sleep(1000)") as subproc:
            try:
                profiler = SampleProfiler(
                    pid=subproc.process.pid,
                    sample_interval_usec=1000,
                    all_threads=False,
                )
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            self.assertTrue(profiler._is_process_running())
            self.assertIsNotNone(profiler.unwinder.get_stack_trace())
            subproc.process.kill()
            subproc.process.wait()
            self.assertRaises(
                ProcessLookupError, profiler.unwinder.get_stack_trace
            )

        # Exit the context manager to ensure the process is terminated
        self.assertFalse(profiler._is_process_running())
        self.assertRaises(
            ProcessLookupError, profiler.unwinder.get_stack_trace
        )

    @unittest.skipUnless(sys.platform == "linux", "Only valid on Linux")
    def test_esrch_signal_handling(self):
        with test_subprocess("import time; time.sleep(1000)") as subproc:
            try:
                unwinder = _remote_debugging.RemoteUnwinder(
                    subproc.process.pid
                )
            except PermissionError:
                self.skipTest(
                    "Insufficient permissions to read the stack trace"
                )
            initial_trace = unwinder.get_stack_trace()
            self.assertIsNotNone(initial_trace)

            subproc.process.kill()

            # Wait for the process to die and try to get another trace
            subproc.process.wait()

            with self.assertRaises(ProcessLookupError):
                unwinder.get_stack_trace()

    def test_script_error_treatment(self):
        script_file = tempfile.NamedTemporaryFile(
            "w", delete=False, suffix=".py"
        )
        script_file.write("open('nonexistent_file.txt')\n")
        script_file.close()
        self.addCleanup(os.unlink, script_file.name)

        result = subprocess.run(
            [
                sys.executable,
                "-m",
                "profiling.sampling.cli",
                "run",
                "-d",
                "1",
                script_file.name,
            ],
            capture_output=True,
            text=True,
        )
        output = result.stdout + result.stderr

        if "PermissionError" in output:
            self.skipTest("Insufficient permissions for remote profiling")
        self.assertNotIn("Script file not found", output)
        self.assertIn(
            "No such file or directory: 'nonexistent_file.txt'", output
        )

    def test_live_incompatible_with_pstats_options(self):
        """Test that --live is incompatible with individual pstats options."""
        test_cases = [
            (["--sort", "tottime"], "--sort"),
            (["--limit", "30"], "--limit"),
            (["--no-summary"], "--no-summary"),
        ]

        for args, expected_flag in test_cases:
            with self.subTest(args=args):
                test_args = ["profiling.sampling.cli", "run", "--live"] + args + ["test.py"]
                with mock.patch("sys.argv", test_args):
                    with self.assertRaises(SystemExit) as cm:
                        from profiling.sampling.cli import main
                        main()
                    self.assertNotEqual(cm.exception.code, 0)

    def test_live_incompatible_with_multiple_pstats_options(self):
        """Test that --live is incompatible with multiple pstats options."""
        test_args = [
            "profiling.sampling.cli", "run", "--live",
            "--sort", "cumtime", "--limit", "25", "--no-summary", "test.py"
        ]

        with mock.patch("sys.argv", test_args):
            with self.assertRaises(SystemExit) as cm:
                from profiling.sampling.cli import main
                main()
            self.assertNotEqual(cm.exception.code, 0)

    def test_live_incompatible_with_pstats_default_values(self):
        """Test that --live blocks pstats options even with default values."""
        # Test with --sort=nsamples (the default value)
        test_args = ["profiling.sampling.cli", "run", "--live", "--sort=nsamples", "test.py"]

        with mock.patch("sys.argv", test_args):
            with self.assertRaises(SystemExit) as cm:
                from profiling.sampling.cli import main
                main()
            self.assertNotEqual(cm.exception.code, 0)

        # Test with --limit=15 (the default value)
        test_args = ["profiling.sampling.cli", "run", "--live", "--limit=15", "test.py"]

        with mock.patch("sys.argv", test_args):
            with self.assertRaises(SystemExit) as cm:
                from profiling.sampling.cli import main
                main()
            self.assertNotEqual(cm.exception.code, 0)
