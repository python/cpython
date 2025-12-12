"""Tests for --subprocesses subprocess profiling support."""

import argparse
import io
import os
import signal
import subprocess
import sys
import tempfile
import threading
import time
import unittest

from test.support import SHORT_TIMEOUT, reap_children, requires_subprocess

from .helpers import (
    skip_if_not_supported,
    PROCESS_VM_READV_SUPPORTED,
    _cleanup_process,
)

# String to check for in stderr when profiler lacks permissions (e.g., macOS)
_PERMISSION_ERROR_MSG = "Permission Error"
_SKIP_PERMISSION_MSG = "Insufficient permissions for remote profiling"


def _readline_with_timeout(file_obj, timeout):
    # Thread-based readline with timeout - works across all platforms
    # including Windows where select() doesn't work with pipes.
    # Returns the line read, or None if timeout occurred.
    result = [None]
    exception = [None]

    def reader():
        try:
            result[0] = file_obj.readline()
        except Exception as e:
            exception[0] = e

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    thread.join(timeout=timeout)

    if thread.is_alive():
        return None

    if exception[0] is not None:
        raise exception[0]

    return result[0]


def _wait_for_process_ready(proc, timeout):
    # Wait for a subprocess to be ready using polling instead of fixed sleep.
    # Returns True if process is ready, False if it exited or timeout.
    deadline = time.time() + timeout
    poll_interval = 0.01

    while time.time() < deadline:
        if proc.poll() is not None:
            return False

        try:
            if sys.platform == "linux":
                if os.path.exists(f"/proc/{proc.pid}/exe"):
                    return True
            else:
                return True
        except OSError:
            pass

        time.sleep(poll_interval)
        poll_interval = min(poll_interval * 2, 0.1)

    return proc.poll() is None


@skip_if_not_supported
@requires_subprocess()
class TestGetChildPids(unittest.TestCase):
    """Tests for the get_child_pids function."""

    def setUp(self):
        reap_children()

    def tearDown(self):
        reap_children()

    def test_get_child_pids_from_remote_debugging(self):
        """Test get_child_pids from _remote_debugging module."""
        try:
            import _remote_debugging

            # Test that the function exists
            self.assertTrue(hasattr(_remote_debugging, "get_child_pids"))

            # Test with current process (should return empty or have children if any)
            result = _remote_debugging.get_child_pids(os.getpid())
            self.assertIsInstance(result, list)
        except (ImportError, AttributeError):
            self.skipTest("_remote_debugging.get_child_pids not available")

    def test_get_child_pids_fallback(self):
        """Test the fallback implementation for get_child_pids."""
        from profiling.sampling._child_monitor import get_child_pids

        # Test with current process
        result = get_child_pids(os.getpid())
        self.assertIsInstance(result, list)

    @unittest.skipUnless(sys.platform == "linux", "Linux only")
    def test_discover_child_process_linux(self):
        """Test that we can discover child processes on Linux."""
        from profiling.sampling._child_monitor import get_child_pids

        # Create a child process
        proc = subprocess.Popen(
            [sys.executable, "-c", "import time; time.sleep(10)"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        try:
            # Poll until child appears
            deadline = time.time() + SHORT_TIMEOUT
            children = []
            while time.time() < deadline:
                children = get_child_pids(os.getpid())
                if proc.pid in children:
                    break
                time.sleep(0.05)

            self.assertIn(
                proc.pid,
                children,
                f"Child PID {proc.pid} not discovered within {SHORT_TIMEOUT}s. "
                f"Found PIDs: {children}",
            )
        finally:
            _cleanup_process(proc)

    def test_recursive_child_discovery(self):
        """Test that recursive=True finds grandchildren."""
        from profiling.sampling._child_monitor import get_child_pids

        # Create a child that spawns a grandchild and keeps a reference to it
        # so we can clean it up via the child process
        code = """
import subprocess
import sys
import threading
grandchild = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(60)'])
print(grandchild.pid, flush=True)
# Wait for parent to send signal byte (cross-platform)
# Using threading with timeout so test doesn't hang if something goes wrong
# Timeout is 60s (2x test timeout) to ensure child outlives test in worst case
def wait_for_signal():
    try:
        sys.stdin.buffer.read(1)
    except:
        pass
t = threading.Thread(target=wait_for_signal, daemon=True)
t.start()
t.join(timeout=60)
# Clean up grandchild before exiting
if grandchild.poll() is None:
    grandchild.terminate()
    try:
        grandchild.wait(timeout=2)
    except subprocess.TimeoutExpired:
        grandchild.kill()
        try:
            grandchild.wait(timeout=2)
        except subprocess.TimeoutExpired:
            grandchild.wait()
"""
        proc = subprocess.Popen(
            [sys.executable, "-c", code],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )

        grandchild_pid = None
        try:
            # Read grandchild PID with thread-based timeout
            # This prevents indefinite blocking on all platforms
            grandchild_pid_line = _readline_with_timeout(
                proc.stdout, SHORT_TIMEOUT
            )
            if grandchild_pid_line is None:
                self.fail(
                    f"Timeout waiting for grandchild PID from child process "
                    f"(child PID: {proc.pid})"
                )
            if not grandchild_pid_line:
                self.fail(
                    f"Child process {proc.pid} closed stdout without printing "
                    f"grandchild PID"
                )
            grandchild_pid = int(grandchild_pid_line.strip())

            # Poll until grandchild is visible
            deadline = time.time() + SHORT_TIMEOUT
            pids_recursive = []
            while time.time() < deadline:
                pids_recursive = get_child_pids(os.getpid(), recursive=True)
                if grandchild_pid in pids_recursive:
                    break
                time.sleep(0.05)

            self.assertIn(
                proc.pid,
                pids_recursive,
                f"Child PID {proc.pid} not found in recursive discovery. "
                f"Found: {pids_recursive}",
            )
            self.assertIn(
                grandchild_pid,
                pids_recursive,
                f"Grandchild PID {grandchild_pid} not found in recursive discovery. "
                f"Found: {pids_recursive}",
            )

            # Non-recursive should find only direct child
            pids_direct = get_child_pids(os.getpid(), recursive=False)
            self.assertIn(
                proc.pid,
                pids_direct,
                f"Child PID {proc.pid} not found in non-recursive discovery. "
                f"Found: {pids_direct}",
            )
            self.assertNotIn(
                grandchild_pid,
                pids_direct,
                f"Grandchild PID {grandchild_pid} should NOT be in non-recursive "
                f"discovery. Found: {pids_direct}",
            )
        finally:
            # Send signal byte to child to trigger cleanup, then close stdin
            try:
                proc.stdin.write(b"x")
                proc.stdin.flush()
                proc.stdin.close()
            except OSError:
                pass
            proc.stdout.close()
            _cleanup_process(proc)
            # The grandchild may not have been cleaned up by the child process
            # (e.g., if the child was killed). Explicitly terminate the
            # grandchild to prevent PermissionError on Windows when removing
            # temp directories.
            if grandchild_pid is not None:
                try:
                    os.kill(grandchild_pid, signal.SIGTERM)
                except (OSError, ProcessLookupError):
                    pass  # Process already exited

    def test_nonexistent_pid_returns_empty(self):
        """Test that nonexistent PID returns empty list."""
        from profiling.sampling._child_monitor import get_child_pids

        # Use a very high PID that's unlikely to exist
        result = get_child_pids(999999999)
        self.assertEqual(result, [])


@skip_if_not_supported
@requires_subprocess()
class TestChildProcessMonitor(unittest.TestCase):
    """Tests for the ChildProcessMonitor class."""

    def setUp(self):
        reap_children()

    def tearDown(self):
        reap_children()

    def test_monitor_creation(self):
        """Test that ChildProcessMonitor can be created."""
        from profiling.sampling._child_monitor import ChildProcessMonitor

        monitor = ChildProcessMonitor(
            pid=os.getpid(),
            cli_args=["-i", "100", "-d", "5"],
            output_pattern="test_{pid}.pstats",
        )
        self.assertEqual(monitor.parent_pid, os.getpid())
        self.assertEqual(monitor.cli_args, ["-i", "100", "-d", "5"])
        self.assertEqual(monitor.output_pattern, "test_{pid}.pstats")

    def test_monitor_lifecycle(self):
        """Test monitor lifecycle via context manager."""
        from profiling.sampling._child_monitor import ChildProcessMonitor

        monitor = ChildProcessMonitor(
            pid=os.getpid(), cli_args=[], output_pattern=None
        )

        # Before entering context, thread should not exist
        self.assertIsNone(monitor._monitor_thread)

        with monitor:
            # Inside context, thread should be running
            self.assertIsNotNone(monitor._monitor_thread)
            self.assertTrue(monitor._monitor_thread.is_alive())

        # After exiting context, thread should be stopped
        self.assertFalse(monitor._monitor_thread.is_alive())

    def test_spawned_profilers_property(self):
        """Test that spawned_profilers returns a copy of the list."""
        from profiling.sampling._child_monitor import ChildProcessMonitor

        monitor = ChildProcessMonitor(
            pid=os.getpid(), cli_args=[], output_pattern=None
        )

        # Should return empty list initially
        profilers = monitor.spawned_profilers
        self.assertEqual(profilers, [])
        self.assertIsNot(profilers, monitor._spawned_profilers)

    def test_context_manager(self):
        """Test that ChildProcessMonitor works as a context manager."""
        from profiling.sampling._child_monitor import ChildProcessMonitor

        with ChildProcessMonitor(
            pid=os.getpid(), cli_args=[], output_pattern=None
        ) as monitor:
            self.assertIsNotNone(monitor._monitor_thread)
            self.assertTrue(monitor._monitor_thread.is_alive())

        # After exiting context, thread should be stopped
        self.assertFalse(monitor._monitor_thread.is_alive())


@skip_if_not_supported
@requires_subprocess()
class TestCLIChildrenFlag(unittest.TestCase):
    """Tests for the --subprocesses CLI flag."""

    def setUp(self):
        reap_children()

    def tearDown(self):
        reap_children()

    def test_subprocesses_flag_parsed(self):
        """Test that --subprocesses flag is recognized."""
        from profiling.sampling.cli import _add_sampling_options

        parser = argparse.ArgumentParser()
        _add_sampling_options(parser)

        # Parse with --subprocesses
        args = parser.parse_args(["--subprocesses"])
        self.assertTrue(args.subprocesses)

        # Parse without --subprocesses
        args = parser.parse_args([])
        self.assertFalse(args.subprocesses)

    def test_subprocesses_incompatible_with_live(self):
        """Test that --subprocesses is incompatible with --live."""
        from profiling.sampling.cli import _validate_args

        # Create mock args with both subprocesses and live
        args = argparse.Namespace(
            subprocesses=True,
            live=True,
            async_aware=False,
            format="pstats",
            mode="wall",
            sort=None,
            limit=None,
            no_summary=False,
            opcodes=False,
        )

        parser = argparse.ArgumentParser()

        with self.assertRaises(SystemExit):
            _validate_args(args, parser)

    def test_build_child_profiler_args(self):
        """Test building CLI args for child profilers."""
        from profiling.sampling.cli import _build_child_profiler_args

        args = argparse.Namespace(
            interval=200,
            duration=15,
            all_threads=True,
            realtime_stats=False,
            native=True,
            gc=True,
            opcodes=False,
            async_aware=False,
            mode="cpu",
            format="flamegraph",
        )

        child_args = _build_child_profiler_args(args)

        # Verify flag-value pairs are correctly paired (flag followed by value)
        def assert_flag_value_pair(flag, value):
            self.assertIn(
                flag,
                child_args,
                f"Flag '{flag}' not found in args: {child_args}",
            )
            flag_index = child_args.index(flag)
            self.assertGreater(
                len(child_args),
                flag_index + 1,
                f"No value after flag '{flag}' in args: {child_args}",
            )
            self.assertEqual(
                child_args[flag_index + 1],
                str(value),
                f"Flag '{flag}' should be followed by '{value}', got "
                f"'{child_args[flag_index + 1]}' in args: {child_args}",
            )

        assert_flag_value_pair("-i", 200)
        assert_flag_value_pair("-d", 15)
        assert_flag_value_pair("--mode", "cpu")

        # Verify standalone flags are present
        self.assertIn(
            "-a", child_args, f"Flag '-a' not found in args: {child_args}"
        )
        self.assertIn(
            "--native",
            child_args,
            f"Flag '--native' not found in args: {child_args}",
        )
        self.assertIn(
            "--flamegraph",
            child_args,
            f"Flag '--flamegraph' not found in args: {child_args}",
        )

    def test_build_child_profiler_args_no_gc(self):
        """Test building CLI args with --no-gc."""
        from profiling.sampling.cli import _build_child_profiler_args

        args = argparse.Namespace(
            interval=100,
            duration=5,
            all_threads=False,
            realtime_stats=False,
            native=False,
            gc=False,  # Explicitly disabled
            opcodes=False,
            async_aware=False,
            mode="wall",
            format="pstats",
        )

        child_args = _build_child_profiler_args(args)

        self.assertIn(
            "--no-gc",
            child_args,
            f"Flag '--no-gc' not found when gc=False. Args: {child_args}",
        )

    def test_build_output_pattern_with_outfile(self):
        """Test output pattern generation with user-specified output."""
        from profiling.sampling.cli import _build_output_pattern

        # With extension
        args = argparse.Namespace(outfile="output.html", format="flamegraph")
        pattern = _build_output_pattern(args)
        self.assertEqual(pattern, "output_{pid}.html")

        # Without extension
        args = argparse.Namespace(outfile="output", format="pstats")
        pattern = _build_output_pattern(args)
        self.assertEqual(pattern, "output_{pid}")

    def test_build_output_pattern_default(self):
        """Test output pattern generation with default output."""
        from profiling.sampling.cli import _build_output_pattern

        # Flamegraph format
        args = argparse.Namespace(outfile=None, format="flamegraph")
        pattern = _build_output_pattern(args)
        self.assertIn("{pid}", pattern)
        self.assertIn("flamegraph", pattern)
        self.assertTrue(pattern.endswith(".html"))

        # Heatmap format
        args = argparse.Namespace(outfile=None, format="heatmap")
        pattern = _build_output_pattern(args)
        self.assertEqual(pattern, "heatmap_{pid}")


@skip_if_not_supported
@requires_subprocess()
@unittest.skipUnless(
    sys.platform != "linux" or PROCESS_VM_READV_SUPPORTED,
    "Test requires process_vm_readv support on Linux",
)
class TestChildrenIntegration(unittest.TestCase):
    """Integration tests for --subprocesses functionality."""

    def setUp(self):
        reap_children()

    def tearDown(self):
        reap_children()

    def test_setup_child_monitor(self):
        """Test setting up a child monitor from args."""
        from profiling.sampling.cli import _setup_child_monitor

        args = argparse.Namespace(
            interval=100,
            duration=5,
            all_threads=False,
            realtime_stats=False,
            native=False,
            gc=True,
            opcodes=False,
            async_aware=False,
            mode="wall",
            format="pstats",
            outfile=None,
        )

        monitor = _setup_child_monitor(args, os.getpid())
        # Use addCleanup to ensure monitor is properly cleaned up even if
        # assertions fail
        self.addCleanup(monitor.__exit__, None, None, None)

        self.assertIsNotNone(monitor)
        self.assertEqual(
            monitor.parent_pid,
            os.getpid(),
            f"Monitor parent_pid should be {os.getpid()}, got {monitor.parent_pid}",
        )


@skip_if_not_supported
@requires_subprocess()
class TestIsPythonProcess(unittest.TestCase):
    """Tests for the is_python_process function."""

    def setUp(self):
        reap_children()

    def tearDown(self):
        reap_children()

    def test_is_python_process_current_process(self):
        """Test that current process is detected as Python."""
        from profiling.sampling._child_monitor import is_python_process

        # Current process should be Python
        self.assertTrue(
            is_python_process(os.getpid()),
            f"Current process (PID {os.getpid()}) should be detected as Python",
        )

    def test_is_python_process_python_subprocess(self):
        """Test that a Python subprocess is detected as Python."""
        from profiling.sampling._child_monitor import is_python_process

        # Start a Python subprocess
        proc = subprocess.Popen(
            [sys.executable, "-c", "import time; time.sleep(10)"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        try:
            # Poll until Python runtime structures are initialized
            # (is_python_process probes for runtime structures which take
            # time to initialize after process start)
            deadline = time.time() + SHORT_TIMEOUT
            detected = False
            while time.time() < deadline:
                if proc.poll() is not None:
                    self.fail(f"Process {proc.pid} exited unexpectedly")
                try:
                    if is_python_process(proc.pid):
                        detected = True
                        break
                except PermissionError:
                    self.skipTest(_SKIP_PERMISSION_MSG)
                time.sleep(0.05)

            self.assertTrue(
                detected,
                f"Python subprocess (PID {proc.pid}) should be detected as Python "
                f"within {SHORT_TIMEOUT}s",
            )
        finally:
            _cleanup_process(proc)

    @unittest.skipUnless(sys.platform == "linux", "Linux only test")
    def test_is_python_process_non_python_subprocess(self):
        """Test that a non-Python subprocess is not detected as Python."""
        from profiling.sampling._child_monitor import is_python_process

        # Start a non-Python subprocess (sleep command)
        proc = subprocess.Popen(
            ["sleep", "10"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        try:
            # Wait for process to be ready using polling
            self.assertTrue(
                _wait_for_process_ready(proc, SHORT_TIMEOUT),
                f"Process {proc.pid} should be ready within {SHORT_TIMEOUT}s",
            )

            self.assertFalse(
                is_python_process(proc.pid),
                f"Non-Python subprocess 'sleep' (PID {proc.pid}) should NOT be "
                f"detected as Python",
            )
        finally:
            _cleanup_process(proc)

    def test_is_python_process_nonexistent_pid(self):
        """Test that nonexistent PID returns False."""
        from profiling.sampling._child_monitor import is_python_process

        # Use a very high PID that's unlikely to exist
        self.assertFalse(
            is_python_process(999999999),
            "Nonexistent PID 999999999 should return False",
        )

    def test_is_python_process_exited_process(self):
        """Test handling of a process that exits quickly."""
        from profiling.sampling._child_monitor import is_python_process

        # Start a process that exits immediately
        proc = subprocess.Popen(
            [sys.executable, "-c", "pass"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        # Wait for it to exit
        proc.wait(timeout=SHORT_TIMEOUT)

        # Should return False for exited process (not raise)
        result = is_python_process(proc.pid)
        self.assertFalse(
            result, f"Exited process (PID {proc.pid}) should return False"
        )


@skip_if_not_supported
@requires_subprocess()
class TestMaxChildProfilersLimit(unittest.TestCase):
    """Tests for the _MAX_CHILD_PROFILERS limit."""

    def setUp(self):
        reap_children()

    def tearDown(self):
        reap_children()

    def test_max_profilers_constant_exists(self):
        """Test that _MAX_CHILD_PROFILERS constant is defined."""
        from profiling.sampling._child_monitor import _MAX_CHILD_PROFILERS

        self.assertEqual(
            _MAX_CHILD_PROFILERS,
            100,
            f"_MAX_CHILD_PROFILERS should be 100, got {_MAX_CHILD_PROFILERS}",
        )

    def test_cleanup_interval_constant_exists(self):
        """Test that _CLEANUP_INTERVAL_CYCLES constant is defined."""
        from profiling.sampling._child_monitor import _CLEANUP_INTERVAL_CYCLES

        self.assertEqual(
            _CLEANUP_INTERVAL_CYCLES,
            10,
            f"_CLEANUP_INTERVAL_CYCLES should be 10, got {_CLEANUP_INTERVAL_CYCLES}",
        )

    def test_monitor_respects_max_limit(self):
        """Test that monitor refuses to spawn more than _MAX_CHILD_PROFILERS."""
        from profiling.sampling._child_monitor import (
            ChildProcessMonitor,
            _MAX_CHILD_PROFILERS,
        )
        from unittest.mock import MagicMock, patch

        # Create a monitor
        monitor = ChildProcessMonitor(
            pid=os.getpid(),
            cli_args=["-i", "100", "-d", "5"],
            output_pattern="test_{pid}.pstats",
        )

        # Manually fill up the profilers list to the limit
        mock_profilers = [MagicMock() for _ in range(_MAX_CHILD_PROFILERS)]
        for mock_proc in mock_profilers:
            mock_proc.poll.return_value = None  # Simulate running process
        monitor._spawned_profilers = mock_profilers

        # Try to spawn another profiler - should be rejected
        stderr_capture = io.StringIO()
        with patch("sys.stderr", stderr_capture):
            monitor._spawn_profiler_for_child(99999)

        # Verify warning was printed
        stderr_output = stderr_capture.getvalue()
        self.assertIn(
            "Max child profilers",
            stderr_output,
            f"Expected warning about max profilers, got: {stderr_output}",
        )
        self.assertIn(
            str(_MAX_CHILD_PROFILERS),
            stderr_output,
            f"Warning should mention limit ({_MAX_CHILD_PROFILERS}): {stderr_output}",
        )

        # Verify no new profiler was added
        self.assertEqual(
            len(monitor._spawned_profilers),
            _MAX_CHILD_PROFILERS,
            f"Should still have {_MAX_CHILD_PROFILERS} profilers, got "
            f"{len(monitor._spawned_profilers)}",
        )


@skip_if_not_supported
@requires_subprocess()
class TestWaitForProfilers(unittest.TestCase):
    """Tests for the wait_for_profilers method."""

    def setUp(self):
        reap_children()

    def tearDown(self):
        reap_children()

    def test_wait_for_profilers_empty_list(self):
        """Test that wait_for_profilers returns immediately with no profilers."""
        from profiling.sampling._child_monitor import ChildProcessMonitor

        monitor = ChildProcessMonitor(
            pid=os.getpid(), cli_args=[], output_pattern=None
        )

        # Should return immediately without printing anything
        stderr_capture = io.StringIO()
        with unittest.mock.patch("sys.stderr", stderr_capture):
            start = time.time()
            monitor.wait_for_profilers(timeout=10.0)
            elapsed = time.time() - start

        # Should complete very quickly (less than 1 second)
        self.assertLess(
            elapsed,
            1.0,
            f"wait_for_profilers with empty list took {elapsed:.2f}s, expected < 1s",
        )
        # No "Waiting for..." message should be printed
        self.assertNotIn(
            "Waiting for",
            stderr_capture.getvalue(),
            "Should not print waiting message when no profilers",
        )

    def test_wait_for_profilers_with_completed_process(self):
        """Test waiting for profilers that complete quickly."""
        from profiling.sampling._child_monitor import ChildProcessMonitor

        monitor = ChildProcessMonitor(
            pid=os.getpid(), cli_args=[], output_pattern=None
        )

        # Start a process that exits quickly
        proc = subprocess.Popen(
            [sys.executable, "-c", "pass"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        # Add to spawned profilers
        monitor._spawned_profilers.append(proc)

        try:
            stderr_capture = io.StringIO()
            with unittest.mock.patch("sys.stderr", stderr_capture):
                start = time.time()
                monitor.wait_for_profilers(timeout=SHORT_TIMEOUT)
                elapsed = time.time() - start

            # Should complete quickly since process exits fast
            self.assertLess(
                elapsed,
                5.0,
                f"wait_for_profilers took {elapsed:.2f}s for quick process",
            )
            # Should print waiting message
            self.assertIn(
                "Waiting for 1 child profiler",
                stderr_capture.getvalue(),
                "Should print waiting message",
            )
        finally:
            _cleanup_process(proc)

    def test_wait_for_profilers_timeout(self):
        """Test that wait_for_profilers respects timeout."""
        from profiling.sampling._child_monitor import ChildProcessMonitor

        monitor = ChildProcessMonitor(
            pid=os.getpid(), cli_args=[], output_pattern=None
        )

        # Start a process that runs for a long time
        proc = subprocess.Popen(
            [sys.executable, "-c", "import time; time.sleep(60)"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        # Add to spawned profilers
        monitor._spawned_profilers.append(proc)

        try:
            stderr_capture = io.StringIO()
            with unittest.mock.patch("sys.stderr", stderr_capture):
                start = time.time()
                # Use short timeout
                monitor.wait_for_profilers(timeout=0.5)
                elapsed = time.time() - start

            # Should timeout after approximately 0.5 seconds
            self.assertGreater(
                elapsed,
                0.4,
                f"wait_for_profilers returned too quickly ({elapsed:.2f}s)",
            )
            self.assertLess(
                elapsed,
                2.0,
                f"wait_for_profilers took too long ({elapsed:.2f}s), timeout not respected",
            )
        finally:
            _cleanup_process(proc)

    def test_wait_for_profilers_multiple(self):
        """Test waiting for multiple profilers."""
        from profiling.sampling._child_monitor import ChildProcessMonitor

        monitor = ChildProcessMonitor(
            pid=os.getpid(), cli_args=[], output_pattern=None
        )

        # Start multiple processes
        procs = []
        for _ in range(3):
            proc = subprocess.Popen(
                [sys.executable, "-c", "import time; time.sleep(0.1)"],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            procs.append(proc)
            monitor._spawned_profilers.append(proc)

        try:
            stderr_capture = io.StringIO()
            with unittest.mock.patch("sys.stderr", stderr_capture):
                monitor.wait_for_profilers(timeout=SHORT_TIMEOUT)

            # Should report correct count
            self.assertIn(
                "Waiting for 3 child profiler",
                stderr_capture.getvalue(),
                "Should report correct profiler count",
            )
        finally:
            for proc in procs:
                _cleanup_process(proc)


@skip_if_not_supported
@requires_subprocess()
@unittest.skipUnless(
    sys.platform != "linux" or PROCESS_VM_READV_SUPPORTED,
    "Test requires process_vm_readv support on Linux",
)
class TestEndToEndChildrenCLI(unittest.TestCase):
    """End-to-end tests for --subprocesses CLI flag."""

    def setUp(self):
        reap_children()

    def tearDown(self):
        reap_children()

    def test_subprocesses_flag_spawns_child_and_creates_output(self):
        """Test that --subprocesses flag works end-to-end with actual subprocesses."""
        # Create a temporary directory for output files
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create a script that spawns a child Python process
            parent_script = f"""
import subprocess
import sys
import time

# Spawn a child that does some work
child = subprocess.Popen([
    sys.executable, '-c',
    'import time; [i**2 for i in range(1000)]; time.sleep(2)'
])
# Do some work in parent
for i in range(1000):
    _ = i ** 2
time.sleep(2)
child.wait()
"""
            script_file = os.path.join(tmpdir, "parent_script.py")
            with open(script_file, "w") as f:
                f.write(parent_script)

            output_file = os.path.join(tmpdir, "profile.pstats")

            # Run the profiler with --subprocesses flag
            result = subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "profiling.sampling",
                    "run",
                    "--subprocesses",
                    "-d",
                    "3",
                    "-i",
                    "10000",
                    "-o",
                    output_file,
                    script_file,
                ],
                capture_output=True,
                text=True,
                timeout=SHORT_TIMEOUT,
            )

            if _PERMISSION_ERROR_MSG in result.stderr:
                self.skipTest(_SKIP_PERMISSION_MSG)

            # Check that parent output file was created
            self.assertTrue(
                os.path.exists(output_file),
                f"Parent profile output not created. "
                f"stdout: {result.stdout}, stderr: {result.stderr}",
            )

            # Check for child profiler output files (pattern: profile_{pid}.pstats)
            output_files = os.listdir(tmpdir)
            child_profiles = [
                f
                for f in output_files
                if f.startswith("profile_") and f.endswith(".pstats")
            ]

            # Note: Child profiling is best-effort; the child may exit before
            # profiler attaches, or the process may not be detected as Python.
            # We just verify the mechanism doesn't crash.
            if result.returncode != 0:
                self.fail(
                    f"Profiler exited with code {result.returncode}. "
                    f"stdout: {result.stdout}, stderr: {result.stderr}"
                )

    def test_subprocesses_flag_with_flamegraph_output(self):
        """Test --subprocesses with flamegraph output format."""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Simple parent that spawns a child
            parent_script = f"""
import subprocess
import sys
import time
child = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(1)'])
time.sleep(1)
child.wait()
"""
            script_file = os.path.join(tmpdir, "parent.py")
            with open(script_file, "w") as f:
                f.write(parent_script)

            output_file = os.path.join(tmpdir, "flame.html")

            result = subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "profiling.sampling",
                    "run",
                    "--subprocesses",
                    "-d",
                    "2",
                    "-i",
                    "10000",
                    "--flamegraph",
                    "-o",
                    output_file,
                    script_file,
                ],
                capture_output=True,
                text=True,
                timeout=SHORT_TIMEOUT,
            )

            if _PERMISSION_ERROR_MSG in result.stderr:
                self.skipTest(_SKIP_PERMISSION_MSG)

            self.assertTrue(
                os.path.exists(output_file),
                f"Flamegraph output not created. stderr: {result.stderr}",
            )

            # Verify it's valid HTML
            with open(output_file, "r") as f:
                content = f.read()
                self.assertIn(
                    "<html",
                    content.lower(),
                    "Flamegraph output should be HTML",
                )

    def test_subprocesses_flag_no_crash_on_quick_child(self):
        """Test that --subprocesses doesn't crash when child exits quickly."""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Parent spawns a child that exits immediately
            parent_script = f"""
import subprocess
import sys
import time
# Child exits immediately
child = subprocess.Popen([sys.executable, '-c', 'pass'])
child.wait()
time.sleep(1)
"""
            script_file = os.path.join(tmpdir, "parent.py")
            with open(script_file, "w") as f:
                f.write(parent_script)

            output_file = os.path.join(tmpdir, "profile.pstats")

            result = subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "profiling.sampling",
                    "run",
                    "--subprocesses",
                    "-d",
                    "2",
                    "-i",
                    "10000",
                    "-o",
                    output_file,
                    script_file,
                ],
                capture_output=True,
                text=True,
                timeout=SHORT_TIMEOUT,
            )

            if _PERMISSION_ERROR_MSG in result.stderr:
                self.skipTest(_SKIP_PERMISSION_MSG)

            # Should not crash - exit code 0
            self.assertEqual(
                result.returncode,
                0,
                f"Profiler crashed with quick-exit child. "
                f"stderr: {result.stderr}",
            )


if __name__ == "__main__":
    unittest.main()
