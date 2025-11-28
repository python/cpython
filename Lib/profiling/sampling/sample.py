import _remote_debugging
import os
import pstats
import statistics
import sys
import sysconfig
import time
from collections import deque
from _colorize import ANSIColors

from .pstats_collector import PstatsCollector
from .stack_collector import CollapsedStackCollector, FlamegraphCollector
from .gecko_collector import GeckoCollector
from .constants import (
    PROFILING_MODE_WALL,
    PROFILING_MODE_CPU,
    PROFILING_MODE_GIL,
    PROFILING_MODE_ALL,
)
try:
    from .live_collector import LiveStatsCollector
except ImportError:
    LiveStatsCollector = None

_FREE_THREADED_BUILD = sysconfig.get_config_var("Py_GIL_DISABLED") is not None



class SampleProfiler:
    def __init__(self, pid, sample_interval_usec, all_threads, *, mode=PROFILING_MODE_WALL, native=False, gc=True, skip_non_matching_threads=True):
        self.pid = pid
        self.sample_interval_usec = sample_interval_usec
        self.all_threads = all_threads
        if _FREE_THREADED_BUILD:
            self.unwinder = _remote_debugging.RemoteUnwinder(
                self.pid, all_threads=self.all_threads, mode=mode, native=native, gc=gc,
                skip_non_matching_threads=skip_non_matching_threads
            )
        else:
            only_active_threads = bool(self.all_threads)
            self.unwinder = _remote_debugging.RemoteUnwinder(
                self.pid, only_active_thread=only_active_threads, mode=mode, native=native, gc=gc,
                skip_non_matching_threads=skip_non_matching_threads
            )
        # Track sample intervals and total sample count
        self.sample_intervals = deque(maxlen=100)
        self.total_samples = 0
        self.realtime_stats = False

    def sample(self, collector, duration_sec=10):
        sample_interval_sec = self.sample_interval_usec / 1_000_000
        running_time = 0
        num_samples = 0
        errors = 0
        start_time = next_time = time.perf_counter()
        last_sample_time = start_time
        realtime_update_interval = 1.0  # Update every second
        last_realtime_update = start_time

        while running_time < duration_sec:
            # Check if live collector wants to stop
            if hasattr(collector, 'running') and not collector.running:
                break

            current_time = time.perf_counter()
            if next_time < current_time:
                try:
                    stack_frames = self.unwinder.get_stack_trace()
                    collector.collect(stack_frames)
                except ProcessLookupError:
                    duration_sec = current_time - start_time
                    break
                except (RuntimeError, UnicodeDecodeError, MemoryError, OSError):
                    collector.collect_failed_sample()
                    errors += 1
                except Exception as e:
                    if not self._is_process_running():
                        break
                    raise e from None

                # Track actual sampling intervals for real-time stats
                if num_samples > 0:
                    actual_interval = current_time - last_sample_time
                    self.sample_intervals.append(
                        1.0 / actual_interval
                    )  # Convert to Hz
                    self.total_samples += 1

                    # Print real-time statistics if enabled
                    if (
                        self.realtime_stats
                        and (current_time - last_realtime_update)
                        >= realtime_update_interval
                    ):
                        self._print_realtime_stats()
                        last_realtime_update = current_time

                last_sample_time = current_time
                num_samples += 1
                next_time += sample_interval_sec

            running_time = time.perf_counter() - start_time

        # Clear real-time stats line if it was being displayed
        if self.realtime_stats and len(self.sample_intervals) > 0:
            print()  # Add newline after real-time stats

        sample_rate = num_samples / running_time
        error_rate = (errors / num_samples) * 100 if num_samples > 0 else 0

        # Don't print stats for live mode (curses is handling display)
        is_live_mode = LiveStatsCollector is not None and isinstance(collector, LiveStatsCollector)
        if not is_live_mode:
            print(f"Captured {num_samples} samples in {running_time:.2f} seconds")
            print(f"Sample rate: {sample_rate:.2f} samples/sec")
            print(f"Error rate: {error_rate:.2f}%")

        # Pass stats to flamegraph collector if it's the right type
        if hasattr(collector, 'set_stats'):
            collector.set_stats(self.sample_interval_usec, running_time, sample_rate, error_rate)

        expected_samples = int(duration_sec / sample_interval_sec)
        if num_samples < expected_samples and not is_live_mode:
            print(
                f"Warning: missed {expected_samples - num_samples} samples "
                f"from the expected total of {expected_samples} "
                f"({(expected_samples - num_samples) / expected_samples * 100:.2f}%)"
            )

    def _is_process_running(self):
        if sys.platform == "linux" or sys.platform == "darwin":
            try:
                os.kill(self.pid, 0)
                return True
            except ProcessLookupError:
                return False
        elif sys.platform == "win32":
            try:
                _remote_debugging.RemoteUnwinder(self.pid)
            except Exception:
                return False
            return True
        else:
            raise ValueError(f"Unsupported platform: {sys.platform}")

    def _print_realtime_stats(self):
        """Print real-time sampling statistics."""
        if len(self.sample_intervals) < 2:
            return

        # Calculate statistics on the Hz values (deque automatically maintains rolling window)
        hz_values = list(self.sample_intervals)
        mean_hz = statistics.mean(hz_values)
        min_hz = min(hz_values)
        max_hz = max(hz_values)

        # Calculate microseconds per sample for all metrics (1/Hz * 1,000,000)
        mean_us_per_sample = (1.0 / mean_hz) * 1_000_000 if mean_hz > 0 else 0
        min_us_per_sample = (
            (1.0 / max_hz) * 1_000_000 if max_hz > 0 else 0
        )  # Min time = Max Hz
        max_us_per_sample = (
            (1.0 / min_hz) * 1_000_000 if min_hz > 0 else 0
        )  # Max time = Min Hz

        # Clear line and print stats
        print(
            f"\r\033[K{ANSIColors.BOLD_BLUE}Real-time sampling stats:{ANSIColors.RESET} "
            f"{ANSIColors.YELLOW}Mean: {mean_hz:.1f}Hz ({mean_us_per_sample:.2f}µs){ANSIColors.RESET} "
            f"{ANSIColors.GREEN}Min: {min_hz:.1f}Hz ({max_us_per_sample:.2f}µs){ANSIColors.RESET} "
            f"{ANSIColors.RED}Max: {max_hz:.1f}Hz ({min_us_per_sample:.2f}µs){ANSIColors.RESET} "
            f"{ANSIColors.CYAN}Samples: {self.total_samples}{ANSIColors.RESET}",
            end="",
            flush=True,
        )


def sample(
    pid,
    collector,
    *,
    duration_sec=10,
    all_threads=False,
    realtime_stats=False,
    mode=PROFILING_MODE_WALL,
    native=False,
    gc=True,
):
    """Sample a process using the provided collector.

    Args:
        pid: Process ID to sample
        collector: Collector instance to use for gathering samples
        duration_sec: How long to sample for (seconds)
        all_threads: Whether to sample all threads
        realtime_stats: Whether to print real-time sampling statistics
        mode: Profiling mode - WALL (all samples), CPU (only when on CPU),
              GIL (only when holding GIL), ALL (includes GIL and CPU status)
        native: Whether to include native frames
        gc: Whether to include GC frames

    Returns:
        The collector with collected samples
    """
    # Get sample interval from collector
    sample_interval_usec = collector.sample_interval_usec

    # PROFILING_MODE_ALL implies no skipping at all
    if mode == PROFILING_MODE_ALL:
        skip_non_matching_threads = False
    else:
        # For most modes, skip non-matching threads
        # Gecko collector overrides this by setting skip_idle=False
        skip_non_matching_threads = True

    profiler = SampleProfiler(
        pid,
        sample_interval_usec,
        all_threads=all_threads,
        mode=mode,
        native=native,
        gc=gc,
        skip_non_matching_threads=skip_non_matching_threads
    )
    profiler.realtime_stats = realtime_stats

    # Run the sampling
    profiler.sample(collector, duration_sec)

    return collector


def sample_live(
    pid,
    collector,
    *,
    duration_sec=10,
    all_threads=False,
    realtime_stats=False,
    mode=PROFILING_MODE_WALL,
    native=False,
    gc=True,
):
    """Sample a process in live/interactive mode with curses TUI.

    Args:
        pid: Process ID to sample
        collector: LiveStatsCollector instance
        duration_sec: How long to sample for (seconds)
        all_threads: Whether to sample all threads
        realtime_stats: Whether to print real-time sampling statistics
        mode: Profiling mode - WALL (all samples), CPU (only when on CPU),
              GIL (only when holding GIL), ALL (includes GIL and CPU status)
        native: Whether to include native frames
        gc: Whether to include GC frames

    Returns:
        The collector with collected samples
    """
    import curses

    # Get sample interval from collector
    sample_interval_usec = collector.sample_interval_usec

    # PROFILING_MODE_ALL implies no skipping at all
    if mode == PROFILING_MODE_ALL:
        skip_non_matching_threads = False
    else:
        skip_non_matching_threads = True

    profiler = SampleProfiler(
        pid,
        sample_interval_usec,
        all_threads=all_threads,
        mode=mode,
        native=native,
        gc=gc,
        skip_non_matching_threads=skip_non_matching_threads
    )
    profiler.realtime_stats = realtime_stats

    def curses_wrapper_func(stdscr):
        collector.init_curses(stdscr)
        try:
            profiler.sample(collector, duration_sec)
            # Mark as finished and keep the TUI running until user presses 'q'
            collector.mark_finished()
            # Keep processing input until user quits
            while collector.running:
                collector._handle_input()
                time.sleep(0.05)  # Small sleep to avoid busy waiting
        finally:
            collector.cleanup_curses()

    try:
        curses.wrapper(curses_wrapper_func)
    except KeyboardInterrupt:
        pass

    return collector
