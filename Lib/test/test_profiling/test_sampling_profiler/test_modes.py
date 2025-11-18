"""Tests for sampling profiler mode filtering (CPU and GIL modes)."""

import io
import unittest
from unittest import mock

try:
    import _remote_debugging  # noqa: F401
    import profiling.sampling
    import profiling.sampling.sample
    from profiling.sampling.pstats_collector import PstatsCollector
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from test.support import requires_subprocess
from test.support import captured_stdout, captured_stderr

from .helpers import test_subprocess
from .mocks import MockFrameInfo, MockInterpreterInfo


class TestCpuModeFiltering(unittest.TestCase):
    """Test CPU mode filtering functionality (--mode=cpu)."""

    def test_mode_validation(self):
        """Test that CLI validates mode choices correctly."""
        # Invalid mode choice should raise SystemExit
        test_args = [
            "profiling.sampling.sample",
            "--mode",
            "invalid",
            "-p",
            "12345",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("sys.stderr", io.StringIO()) as mock_stderr,
            self.assertRaises(SystemExit) as cm,
        ):
            profiling.sampling.sample.main()

        self.assertEqual(cm.exception.code, 2)  # argparse error
        error_msg = mock_stderr.getvalue()
        self.assertIn("invalid choice", error_msg)

    def test_frames_filtered_with_skip_idle(self):
        """Test that frames are actually filtered when skip_idle=True."""
        # Import thread status flags
        try:
            from _remote_debugging import (
                THREAD_STATUS_HAS_GIL,
                THREAD_STATUS_ON_CPU,
            )
        except ImportError:
            THREAD_STATUS_HAS_GIL = 1 << 0
            THREAD_STATUS_ON_CPU = 1 << 1

        # Create mock frames with different thread statuses
        class MockThreadInfoWithStatus:
            def __init__(self, thread_id, frame_info, status):
                self.thread_id = thread_id
                self.frame_info = frame_info
                self.status = status

        # Create test data: active thread (HAS_GIL | ON_CPU), idle thread (neither), and another active thread
        ACTIVE_STATUS = (
            THREAD_STATUS_HAS_GIL | THREAD_STATUS_ON_CPU
        )  # Has GIL and on CPU
        IDLE_STATUS = 0  # Neither has GIL nor on CPU

        test_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfoWithStatus(
                        1,
                        [MockFrameInfo("active1.py", 10, "active_func1")],
                        ACTIVE_STATUS,
                    ),
                    MockThreadInfoWithStatus(
                        2,
                        [MockFrameInfo("idle.py", 20, "idle_func")],
                        IDLE_STATUS,
                    ),
                    MockThreadInfoWithStatus(
                        3,
                        [MockFrameInfo("active2.py", 30, "active_func2")],
                        ACTIVE_STATUS,
                    ),
                ],
            )
        ]

        # Test with skip_idle=True - should only process running threads
        collector_skip = PstatsCollector(
            sample_interval_usec=1000, skip_idle=True
        )
        collector_skip.collect(test_frames)

        # Should only have functions from running threads (status 0)
        active1_key = ("active1.py", 10, "active_func1")
        active2_key = ("active2.py", 30, "active_func2")
        idle_key = ("idle.py", 20, "idle_func")

        self.assertIn(active1_key, collector_skip.result)
        self.assertIn(active2_key, collector_skip.result)
        self.assertNotIn(
            idle_key, collector_skip.result
        )  # Idle thread should be filtered out

        # Test with skip_idle=False - should process all threads
        collector_no_skip = PstatsCollector(
            sample_interval_usec=1000, skip_idle=False
        )
        collector_no_skip.collect(test_frames)

        # Should have functions from all threads
        self.assertIn(active1_key, collector_no_skip.result)
        self.assertIn(active2_key, collector_no_skip.result)
        self.assertIn(
            idle_key, collector_no_skip.result
        )  # Idle thread should be included

    @requires_subprocess()
    def test_cpu_mode_integration_filtering(self):
        """Integration test: CPU mode should only capture active threads, not idle ones."""
        # Script with one mostly-idle thread and one CPU-active thread
        cpu_vs_idle_script = """
import time
import threading

cpu_ready = threading.Event()

def idle_worker():
    time.sleep(999999)

def cpu_active_worker():
    cpu_ready.set()
    x = 1
    while True:
        x += 1

def main():
    # Start both threads
    idle_thread = threading.Thread(target=idle_worker)
    cpu_thread = threading.Thread(target=cpu_active_worker)
    idle_thread.start()
    cpu_thread.start()

    # Wait for CPU thread to be running, then signal test
    cpu_ready.wait()
    _test_sock.sendall(b"threads_ready")

    idle_thread.join()
    cpu_thread.join()

main()

"""
        with test_subprocess(cpu_vs_idle_script) as subproc:
            # Wait for signal that threads are running
            response = subproc.socket.recv(1024)
            self.assertEqual(response, b"threads_ready")

            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    profiling.sampling.sample.sample(
                        subproc.process.pid,
                        duration_sec=2.0,
                        sample_interval_usec=5000,
                        mode=1,  # CPU mode
                        show_summary=False,
                        all_threads=True,
                    )
                except (PermissionError, RuntimeError) as e:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

                cpu_mode_output = captured_output.getvalue()

            # Test wall-clock mode (mode=0) - should capture both functions
            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    profiling.sampling.sample.sample(
                        subproc.process.pid,
                        duration_sec=2.0,
                        sample_interval_usec=5000,
                        mode=0,  # Wall-clock mode
                        show_summary=False,
                        all_threads=True,
                    )
                except (PermissionError, RuntimeError) as e:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

                wall_mode_output = captured_output.getvalue()

            # Verify both modes captured samples
            self.assertIn("Captured", cpu_mode_output)
            self.assertIn("samples", cpu_mode_output)
            self.assertIn("Captured", wall_mode_output)
            self.assertIn("samples", wall_mode_output)

            # CPU mode should strongly favor cpu_active_worker over mostly_idle_worker
            self.assertIn("cpu_active_worker", cpu_mode_output)
            self.assertNotIn("idle_worker", cpu_mode_output)

            # Wall-clock mode should capture both types of work
            self.assertIn("cpu_active_worker", wall_mode_output)
            self.assertIn("idle_worker", wall_mode_output)

    def test_cpu_mode_with_no_samples(self):
        """Test that CPU mode handles no samples gracefully when no samples are collected."""
        # Mock a collector that returns empty stats
        mock_collector = mock.MagicMock()
        mock_collector.stats = {}
        mock_collector.create_stats = mock.MagicMock()

        with (
            io.StringIO() as captured_output,
            mock.patch("sys.stdout", captured_output),
            mock.patch(
                "profiling.sampling.sample.PstatsCollector",
                return_value=mock_collector,
            ),
            mock.patch(
                "profiling.sampling.sample.SampleProfiler"
            ) as mock_profiler_class,
        ):
            mock_profiler = mock.MagicMock()
            mock_profiler_class.return_value = mock_profiler

            profiling.sampling.sample.sample(
                12345,  # dummy PID
                duration_sec=0.5,
                sample_interval_usec=5000,
                mode=1,  # CPU mode
                show_summary=False,
                all_threads=True,
            )

            output = captured_output.getvalue()

        # Should see the "No samples were collected" message
        self.assertIn("No samples were collected", output)
        self.assertIn("CPU mode", output)


class TestGilModeFiltering(unittest.TestCase):
    """Test GIL mode filtering functionality (--mode=gil)."""

    def test_gil_mode_validation(self):
        """Test that CLI accepts gil mode choice correctly."""
        test_args = [
            "profiling.sampling.sample",
            "--mode",
            "gil",
            "-p",
            "12345",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
        ):
            try:
                profiling.sampling.sample.main()
            except SystemExit:
                pass  # Expected due to invalid PID

        # Should have attempted to call sample with mode=2 (GIL mode)
        mock_sample.assert_called_once()
        call_args = mock_sample.call_args[1]
        self.assertEqual(call_args["mode"], 2)  # PROFILING_MODE_GIL

    def test_gil_mode_sample_function_call(self):
        """Test that sample() function correctly uses GIL mode."""
        with (
            mock.patch(
                "profiling.sampling.sample.SampleProfiler"
            ) as mock_profiler,
            mock.patch(
                "profiling.sampling.sample.PstatsCollector"
            ) as mock_collector,
        ):
            # Mock the profiler instance
            mock_instance = mock.Mock()
            mock_profiler.return_value = mock_instance

            # Mock the collector instance
            mock_collector_instance = mock.Mock()
            mock_collector.return_value = mock_collector_instance

            # Call sample with GIL mode and a filename to avoid pstats creation
            profiling.sampling.sample.sample(
                12345,
                mode=2,  # PROFILING_MODE_GIL
                duration_sec=1,
                sample_interval_usec=1000,
                filename="test_output.txt",
            )

            # Verify SampleProfiler was created with correct mode
            mock_profiler.assert_called_once()
            call_args = mock_profiler.call_args
            self.assertEqual(call_args[1]["mode"], 2)  # mode parameter

            # Verify profiler.sample was called
            mock_instance.sample.assert_called_once()

            # Verify collector.export was called since we provided a filename
            mock_collector_instance.export.assert_called_once_with(
                "test_output.txt"
            )

    def test_gil_mode_collector_configuration(self):
        """Test that collectors are configured correctly for GIL mode."""
        with (
            mock.patch(
                "profiling.sampling.sample.SampleProfiler"
            ) as mock_profiler,
            mock.patch(
                "profiling.sampling.sample.PstatsCollector"
            ) as mock_collector,
            captured_stdout(),
            captured_stderr(),
        ):
            # Mock the profiler instance
            mock_instance = mock.Mock()
            mock_profiler.return_value = mock_instance

            # Call sample with GIL mode
            profiling.sampling.sample.sample(
                12345,
                mode=2,  # PROFILING_MODE_GIL
                output_format="pstats",
            )

            # Verify collector was created with skip_idle=True (since mode != WALL)
            mock_collector.assert_called_once()
            call_args = mock_collector.call_args[1]
            self.assertTrue(call_args["skip_idle"])

    def test_gil_mode_with_collapsed_format(self):
        """Test GIL mode with collapsed stack format."""
        with (
            mock.patch(
                "profiling.sampling.sample.SampleProfiler"
            ) as mock_profiler,
            mock.patch(
                "profiling.sampling.sample.CollapsedStackCollector"
            ) as mock_collector,
        ):
            # Mock the profiler instance
            mock_instance = mock.Mock()
            mock_profiler.return_value = mock_instance

            # Call sample with GIL mode and collapsed format
            profiling.sampling.sample.sample(
                12345,
                mode=2,  # PROFILING_MODE_GIL
                output_format="collapsed",
                filename="test_output.txt",
            )

            # Verify collector was created with skip_idle=True
            mock_collector.assert_called_once()
            call_args = mock_collector.call_args[1]
            self.assertTrue(call_args["skip_idle"])

    def test_gil_mode_cli_argument_parsing(self):
        """Test CLI argument parsing for GIL mode with various options."""
        test_args = [
            "profiling.sampling.sample",
            "--mode",
            "gil",
            "--interval",
            "500",
            "--duration",
            "5",
            "-p",
            "12345",
        ]

        with (
            mock.patch("sys.argv", test_args),
            mock.patch("profiling.sampling.sample.sample") as mock_sample,
        ):
            try:
                profiling.sampling.sample.main()
            except SystemExit:
                pass  # Expected due to invalid PID

        # Verify all arguments were parsed correctly
        mock_sample.assert_called_once()
        call_args = mock_sample.call_args[1]
        self.assertEqual(call_args["mode"], 2)  # GIL mode
        self.assertEqual(call_args["sample_interval_usec"], 500)
        self.assertEqual(call_args["duration_sec"], 5)

    @requires_subprocess()
    def test_gil_mode_integration_behavior(self):
        """Integration test: GIL mode should capture GIL-holding threads."""
        # Create a test script with GIL-releasing operations
        gil_test_script = """
import time
import threading

gil_ready = threading.Event()

def gil_releasing_work():
    time.sleep(999999)

def gil_holding_work():
    gil_ready.set()
    x = 1
    while True:
        x += 1

def main():
    # Start both threads
    idle_thread = threading.Thread(target=gil_releasing_work)
    cpu_thread = threading.Thread(target=gil_holding_work)
    idle_thread.start()
    cpu_thread.start()

    # Wait for GIL-holding thread to be running, then signal test
    gil_ready.wait()
    _test_sock.sendall(b"threads_ready")

    idle_thread.join()
    cpu_thread.join()

main()
"""
        with test_subprocess(gil_test_script) as subproc:
            # Wait for signal that threads are running
            response = subproc.socket.recv(1024)
            self.assertEqual(response, b"threads_ready")

            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    profiling.sampling.sample.sample(
                        subproc.process.pid,
                        duration_sec=2.0,
                        sample_interval_usec=5000,
                        mode=2,  # GIL mode
                        show_summary=False,
                        all_threads=True,
                    )
                except (PermissionError, RuntimeError) as e:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

                gil_mode_output = captured_output.getvalue()

            # Test wall-clock mode for comparison
            with (
                io.StringIO() as captured_output,
                mock.patch("sys.stdout", captured_output),
            ):
                try:
                    profiling.sampling.sample.sample(
                        subproc.process.pid,
                        duration_sec=0.5,
                        sample_interval_usec=5000,
                        mode=0,  # Wall-clock mode
                        show_summary=False,
                        all_threads=True,
                    )
                except (PermissionError, RuntimeError) as e:
                    self.skipTest(
                        "Insufficient permissions for remote profiling"
                    )

                wall_mode_output = captured_output.getvalue()

            # GIL mode should primarily capture GIL-holding work
            # (Note: actual behavior depends on threading implementation)
            self.assertIn("gil_holding_work", gil_mode_output)

            # Wall-clock mode should capture both types of work
            self.assertIn("gil_holding_work", wall_mode_output)

    def test_mode_constants_are_defined(self):
        """Test that all profiling mode constants are properly defined."""
        self.assertEqual(profiling.sampling.sample.PROFILING_MODE_WALL, 0)
        self.assertEqual(profiling.sampling.sample.PROFILING_MODE_CPU, 1)
        self.assertEqual(profiling.sampling.sample.PROFILING_MODE_GIL, 2)

    def test_parse_mode_function(self):
        """Test the _parse_mode function with all valid modes."""
        self.assertEqual(profiling.sampling.sample._parse_mode("wall"), 0)
        self.assertEqual(profiling.sampling.sample._parse_mode("cpu"), 1)
        self.assertEqual(profiling.sampling.sample._parse_mode("gil"), 2)

        # Test invalid mode raises KeyError
        with self.assertRaises(KeyError):
            profiling.sampling.sample._parse_mode("invalid")
