"""Tests for blocking mode sampling profiler."""

import io
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

from test.support import requires_remote_subprocess_debugging

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
        # When consume_generator is on the arithmetic lines (temp1, temp2, etc.),
        # fibonacci_generator should NOT be in the stack at all.
        # Line numbers are important here - see ARITHMETIC_LINES below.
        cls.generator_script = '''
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
'''
        # Line numbers of the arithmetic operations in consume_generator.
        # These are the lines where fibonacci_generator should NOT be in the stack.
        # The socket injection code adds 7 lines before our script.
        # temp1 = value + 1  -> line 17
        # temp2 = value * 2  -> line 18
        # temp3 = value - 1  -> line 19
        # result = ...       -> line 20
        cls.ARITHMETIC_LINES = {17, 18, 19, 20}

    def test_generator_not_under_consumer_arithmetic(self):
        """Test that fibonacci_generator doesn't appear when consume_generator does arithmetic.

        When consume_generator is executing arithmetic lines (temp1, temp2, etc.),
        fibonacci_generator should NOT be anywhere in the stack - it's not being
        called at that point.

        Valid stacks:
          - consume_generator at 'for value in gen:' line WITH fibonacci_generator
            at the top (generator is yielding)
          - consume_generator at arithmetic lines WITHOUT fibonacci_generator
            (we're just doing math, not calling the generator)

        Invalid stacks (indicate torn/inconsistent reads):
          - consume_generator at arithmetic lines WITH fibonacci_generator
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

        for (call_tree, _thread_id), count in collector.stack_counter.items():
            total_samples += count

            if not call_tree:
                continue

            # Find consume_generator in the stack and check its line number
            for i, (filename, lineno, funcname) in enumerate(call_tree):
                if funcname == "consume_generator" and lineno in self.ARITHMETIC_LINES:
                    arithmetic_samples += count
                    # Check if fibonacci_generator appears anywhere in this stack
                    func_names = [frame[2] for frame in call_tree]
                    if "fibonacci_generator" in func_names:
                        invalid_stacks += count
                    break

        self.assertGreater(total_samples, 10,
            f"Expected at least 10 samples, got {total_samples}")

        # We should have some samples on the arithmetic lines
        self.assertGreater(arithmetic_samples, 0,
            f"Expected some samples on arithmetic lines, got {arithmetic_samples}")

        self.assertEqual(invalid_stacks, 0,
            f"Found {invalid_stacks}/{arithmetic_samples} invalid stacks where "
            f"fibonacci_generator appears in the stack when consume_generator "
            f"is on an arithmetic line. This indicates torn/inconsistent stack "
            f"traces are being captured.")
