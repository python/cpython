"""Tests for blocking mode sampling profiler."""

import io
import os
import subprocess
import sys
import textwrap
import unittest
from unittest import mock

try:
    import _remote_debugging  # noqa: F401
    import profiling.sampling
    import profiling.sampling.sample
    from profiling.sampling.stack_collector import CollapsedStackCollector
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from test.support import (
    SHORT_TIMEOUT,
    os_helper,
    requires_remote_subprocess_debugging,
)

from .helpers import test_subprocess

# Duration for profiling in tests
PROFILING_DURATION_SEC = 1


@requires_remote_subprocess_debugging()
class TestBlockingModeStackAccuracy(unittest.TestCase):
    """Test that blocking mode produces accurate stack traces.

    When using blocking mode, the target process is stopped during sampling.
    This ensures that we see accurate stack traces where functions appear
    in the correct caller/callee relationship.

    These tests verify that generator functions are correctly shown at the
    top of the stack when they are actively executing, and not incorrectly
    shown under their caller's code.
    """

    @classmethod
    def setUpClass(cls):
        # Test script that uses a generator consumed in a loop.
        # When consume_generator is the executing leaf frame on the arithmetic
        # lines (temp1, temp2, etc.), fibonacci_generator should NOT be in the
        # stack at all.
        # Line numbers are important here - see ARITHMETIC_LINES below.
        cls.generator_script = textwrap.dedent('''
            def fibonacci_generator(n):
                a, b = 0, 1
                for _ in range(n):
                    yield a
                    a, b = b, a + b

            def consume_generator():
                gen = fibonacci_generator(10000)
                for value in gen:
                    temp1 = value + 1
                    temp2 = value * 2
                    temp3 = value - 1
                    result = temp1 + temp2 + temp3

            def main():
                while True:
                    consume_generator()

            _test_sock.sendall(b"working")
            main()
        ''')
        # Line numbers of the arithmetic operations in consume_generator.
        # These are the lines where fibonacci_generator should NOT be in the
        # stack when consume_generator is the executing leaf frame. They account
        # for the socket prelude added by test_subprocess().
        # temp1 = value + 1  -> line 16
        # temp2 = value * 2  -> line 17
        # temp3 = value - 1  -> line 18
        # result = ...       -> line 19
        cls.ARITHMETIC_LINES = {16, 17, 18, 19}

    def test_generator_not_under_consumer_arithmetic(self):
        """Test that fibonacci_generator doesn't appear when consume_generator does arithmetic.

        When consume_generator is the leaf frame on arithmetic lines (temp1,
        temp2, etc.), fibonacci_generator should NOT be anywhere in the stack -
        it's not being called at that point. Non-leaf frame line numbers are
        caller/resume metadata, not proof that the frame is executing.

        Valid stacks:
          - fibonacci_generator at the top (generator is executing), with
            consume_generator below it
          - consume_generator at arithmetic lines WITHOUT fibonacci_generator
            (we're just doing math, not calling the generator)

        Invalid stacks (indicate torn/inconsistent reads):
          - consume_generator leaf frame at arithmetic lines WITH
            fibonacci_generator
            anywhere in the stack

        Note: call_tree is ordered from bottom (index 0) to top (index -1).
        """
        with test_subprocess(self.generator_script, wait_for_working=True) as subproc:
            collector = CollapsedStackCollector(sample_interval_usec=100, skip_idle=False)

            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                profiling.sampling.sample.sample(
                    subproc.process.pid,
                    collector,
                    duration_sec=PROFILING_DURATION_SEC,
                    blocking=True,
                )

        # Analyze collected stacks
        total_samples = 0
        invalid_stacks = 0
        arithmetic_samples = 0
        generator_samples = 0
        generator_not_leaf_samples = 0

        for (call_tree, _thread_id), count in collector.stack_counter.items():
            total_samples += count

            if not call_tree:
                continue

            # Non-leaf frame line numbers can point at resume locations while
            # a callee is the executing leaf frame.
            _, lineno, funcname = call_tree[-1]
            func_names = [frame[2] for frame in call_tree]

            if "fibonacci_generator" in func_names:
                generator_samples += count
                if funcname != "fibonacci_generator":
                    generator_not_leaf_samples += count

            if funcname == "consume_generator" and lineno in self.ARITHMETIC_LINES:
                arithmetic_samples += count
                # Check if fibonacci_generator appears anywhere in this stack.
                if "fibonacci_generator" in func_names:
                    invalid_stacks += count

        self.assertGreater(total_samples, 10,
            f"Expected at least 10 samples, got {total_samples}")

        # We should have some samples on the arithmetic lines
        self.assertGreater(arithmetic_samples, 0,
            f"Expected some samples on arithmetic lines, got {arithmetic_samples}")

        self.assertGreater(generator_samples, 0,
            f"Expected some samples in fibonacci_generator, got {generator_samples}")

        self.assertEqual(generator_not_leaf_samples, 0,
            f"Found {generator_not_leaf_samples}/{generator_samples} stacks where "
            f"fibonacci_generator appears but is not the leaf frame.")

        self.assertEqual(invalid_stacks, 0,
            f"Found {invalid_stacks}/{arithmetic_samples} invalid stacks where "
            f"fibonacci_generator appears in the stack when consume_generator "
            f"is the leaf frame on an arithmetic line. This indicates "
            f"torn/inconsistent stack traces are being captured.")


@requires_remote_subprocess_debugging()
@unittest.skipUnless(sys.platform == "win32", "Windows only")
class TestBlockingModeCLI(unittest.TestCase):
    def test_run_blocking_exits_after_target_process_exits(self):
        script = 'print("done")\n'

        tmpdir = os.path.abspath(os_helper.TESTFN + "_profiling_blocking")
        with os_helper.temp_dir(tmpdir) as tmpdir:
            script_path = os.path.join(tmpdir, "tiny_target.py")
            profile_path = os.path.join(tmpdir, "blocking.bin")
            with open(script_path, "w", encoding="utf-8") as file:
                file.write(script)

            cmd = [
                sys.executable, "-m", "profiling.sampling", "run",
                "--binary", "-o", profile_path,
                "--mode=cpu", "--blocking", "-r", "100",
                script_path,
            ]
            result = subprocess.run(
                cmd,
                cwd=tmpdir,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=SHORT_TIMEOUT,
            )

            self.assertEqual(
                result.returncode, 0,
                f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}",
            )
            self.assertGreater(os.path.getsize(profile_path), 0)

            replay = subprocess.run(
                [sys.executable, "-m", "profiling.sampling", "replay",
                 profile_path],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=SHORT_TIMEOUT,
            )
            self.assertEqual(
                replay.returncode, 0,
                f"stdout:\n{replay.stdout}\nstderr:\n{replay.stderr}",
            )
