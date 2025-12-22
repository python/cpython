import _remote_debugging
import os
import statistics
import sys
import sysconfig
import time
from collections import deque
from _colorize import ANSIColors

from .constants import (
    PROFILING_MODE_WALL,
    PROFILING_MODE_CPU,
    PROFILING_MODE_GIL,
    PROFILING_MODE_ALL,
    PROFILING_MODE_EXCEPTION,
)
from ._format_utils import fmt
try:
    from .live_collector import LiveStatsCollector
except ImportError:
    LiveStatsCollector = None

_FREE_THREADED_BUILD = sysconfig.get_config_var("Py_GIL_DISABLED") is not None


class SampleProfiler:
    def __init__(self, pid, sample_interval_usec, all_threads, *, mode=PROFILING_MODE_WALL, native=False, gc=True, opcodes=False, skip_non_matching_threads=True, collect_stats=False):
        self.pid = pid
        self.sample_interval_usec = sample_interval_usec
        self.all_threads = all_threads
        self.mode = mode  # Store mode for later use
        self.collect_stats = collect_stats
        try:
            self.unwinder = self._new_unwinder(native, gc, opcodes, skip_non_matching_threads)
        except RuntimeError as err:
            raise SystemExit(err) from err
        # Track sample intervals and total sample count
        self.sample_intervals = deque(maxlen=100)
        self.total_samples = 0
        self.realtime_stats = False

    def _new_unwinder(self, native, gc, opcodes, skip_non_matching_threads):
        if _FREE_THREADED_BUILD:
            unwinder = _remote_debugging.RemoteUnwinder(
                self.pid, all_threads=self.all_threads, mode=self.mode, native=native, gc=gc,
                opcodes=opcodes, skip_non_matching_threads=skip_non_matching_threads,
                cache_frames=True, stats=self.collect_stats
            )
        else:
            unwinder = _remote_debugging.RemoteUnwinder(
                self.pid, only_active_thread=bool(self.all_threads), mode=self.mode, native=native, gc=gc,
                opcodes=opcodes, skip_non_matching_threads=skip_non_matching_threads,
                cache_frames=True, stats=self.collect_stats
            )
        return unwinder

    def sample(self, collector, duration_sec=10, *, async_aware=False):
        sample_interval_sec = self.sample_interval_usec / 1_000_000
        running_time = 0
        num_samples = 0
        errors = 0
        start_time = next_time = time.perf_counter()
        last_sample_time = start_time
        realtime_update_interval = 1.0  # Update every second
        last_realtime_update = start_time
        interrupted = False

        try:
            while running_time < duration_sec:
                # Check if live collector wants to stop
                if hasattr(collector, 'running') and not collector.running:
                    break

                current_time = time.perf_counter()
                if next_time < current_time:
                    try:
                        if async_aware == "all":
                            stack_frames = self.unwinder.get_all_awaited_by()
                        elif async_aware == "running":
                            stack_frames = self.unwinder.get_async_stack_trace()
                        else:
                            stack_frames = self.unwinder.get_stack_trace()
                        collector.collect(stack_frames)
                    except ProcessLookupError:
                        duration_sec = current_time - start_time
                        break
                    except (RuntimeError, UnicodeDecodeError, MemoryError, OSError):
                        collector.collect_failed_sample()
                        errors += 1
                    except Exception as e:
                        if not _is_process_running(self.pid):
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
        except KeyboardInterrupt:
            interrupted = True
            running_time = time.perf_counter() - start_time
            print("Interrupted by user.")

        # Clear real-time stats line if it was being displayed
        if self.realtime_stats and len(self.sample_intervals) > 0:
            print()  # Add newline after real-time stats

        sample_rate = num_samples / running_time if running_time > 0 else 0
        error_rate = (errors / num_samples) * 100 if num_samples > 0 else 0
        expected_samples = int(duration_sec / sample_interval_sec)
        missed_samples = (expected_samples - num_samples) / expected_samples * 100 if expected_samples > 0 else 0

        # Don't print stats for live mode (curses is handling display)
        is_live_mode = LiveStatsCollector is not None and isinstance(collector, LiveStatsCollector)
        if not is_live_mode:
            print(f"Captured {num_samples:n} samples in {fmt(running_time, 2)} seconds")
            print(f"Sample rate: {fmt(sample_rate, 2)} samples/sec")
            print(f"Error rate: {fmt(error_rate, 2)}")

            # Print unwinder stats if stats collection is enabled
            if self.collect_stats:
                self._print_unwinder_stats()

        # Pass stats to flamegraph collector if it's the right type
        if hasattr(collector, 'set_stats'):
            collector.set_stats(self.sample_interval_usec, running_time, sample_rate, error_rate, missed_samples, mode=self.mode)

        if num_samples < expected_samples and not is_live_mode and not interrupted:
            print(
                f"Warning: missed {expected_samples - num_samples} samples "
                f"from the expected total of {expected_samples} "
                f"({fmt((expected_samples - num_samples) / expected_samples * 100, 2)}%)"
            )

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

        # Build cache stats string if stats collection is enabled
        cache_stats_str = ""
        if self.collect_stats:
            try:
                stats = self.unwinder.get_stats()
                hits = stats.get('frame_cache_hits', 0)
                partial = stats.get('frame_cache_partial_hits', 0)
                misses = stats.get('frame_cache_misses', 0)
                total = hits + partial + misses
                if total > 0:
                    hit_pct = (hits + partial) / total * 100
                    cache_stats_str = f" {ANSIColors.MAGENTA}Cache: {fmt(hit_pct)}% ({hits}+{partial}/{misses}){ANSIColors.RESET}"
            except RuntimeError:
                pass

        # Clear line and print stats
        print(
            f"\r\033[K{ANSIColors.BOLD_BLUE}Stats:{ANSIColors.RESET} "
            f"{ANSIColors.YELLOW}{fmt(mean_hz)}Hz ({fmt(mean_us_per_sample)}µs){ANSIColors.RESET} "
            f"{ANSIColors.GREEN}Min: {fmt(min_hz)}Hz{ANSIColors.RESET} "
            f"{ANSIColors.RED}Max: {fmt(max_hz)}Hz{ANSIColors.RESET} "
            f"{ANSIColors.CYAN}N={self.total_samples}{ANSIColors.RESET}"
            f"{cache_stats_str}",
            end="",
            flush=True,
        )

    def _print_unwinder_stats(self):
        """Print unwinder statistics including cache performance."""
        try:
            stats = self.unwinder.get_stats()
        except RuntimeError:
            return  # Stats not enabled

        print(f"\n{ANSIColors.BOLD_BLUE}{'='*50}{ANSIColors.RESET}")
        print(f"{ANSIColors.BOLD_BLUE}Unwinder Statistics:{ANSIColors.RESET}")

        # Frame cache stats
        total_samples = stats.get('total_samples', 0)
        frame_cache_hits = stats.get('frame_cache_hits', 0)
        frame_cache_partial_hits = stats.get('frame_cache_partial_hits', 0)
        frame_cache_misses = stats.get('frame_cache_misses', 0)
        total_lookups = frame_cache_hits + frame_cache_partial_hits + frame_cache_misses

        # Calculate percentages
        hits_pct = (frame_cache_hits / total_lookups * 100) if total_lookups > 0 else 0
        partial_pct = (frame_cache_partial_hits / total_lookups * 100) if total_lookups > 0 else 0
        misses_pct = (frame_cache_misses / total_lookups * 100) if total_lookups > 0 else 0

        print(f"  {ANSIColors.CYAN}Frame Cache:{ANSIColors.RESET}")
        print(f"    Total samples:    {total_samples:n}")
        print(f"    Full hits:        {frame_cache_hits:n} ({ANSIColors.GREEN}{fmt(hits_pct)}%{ANSIColors.RESET})")
        print(f"    Partial hits:     {frame_cache_partial_hits:n} ({ANSIColors.YELLOW}{fmt(partial_pct)}%{ANSIColors.RESET})")
        print(f"    Misses:           {frame_cache_misses:n} ({ANSIColors.RED}{fmt(misses_pct)}%{ANSIColors.RESET})")

        # Frame read stats
        frames_from_cache = stats.get('frames_read_from_cache', 0)
        frames_from_memory = stats.get('frames_read_from_memory', 0)
        total_frames = frames_from_cache + frames_from_memory
        cache_frame_pct = (frames_from_cache / total_frames * 100) if total_frames > 0 else 0
        memory_frame_pct = (frames_from_memory / total_frames * 100) if total_frames > 0 else 0

        print(f"  {ANSIColors.CYAN}Frame Reads:{ANSIColors.RESET}")
        print(f"    From cache:       {frames_from_cache:n} ({ANSIColors.GREEN}{fmt(cache_frame_pct)}%{ANSIColors.RESET})")
        print(f"    From memory:      {frames_from_memory:n} ({ANSIColors.RED}{fmt(memory_frame_pct)}%{ANSIColors.RESET})")

        # Code object cache stats
        code_hits = stats.get('code_object_cache_hits', 0)
        code_misses = stats.get('code_object_cache_misses', 0)
        total_code = code_hits + code_misses
        code_hits_pct = (code_hits / total_code * 100) if total_code > 0 else 0
        code_misses_pct = (code_misses / total_code * 100) if total_code > 0 else 0

        print(f"  {ANSIColors.CYAN}Code Object Cache:{ANSIColors.RESET}")
        print(f"    Hits:             {code_hits:n} ({ANSIColors.GREEN}{fmt(code_hits_pct)}%{ANSIColors.RESET})")
        print(f"    Misses:           {code_misses:n} ({ANSIColors.RED}{fmt(code_misses_pct)}%{ANSIColors.RESET})")

        # Memory operations
        memory_reads = stats.get('memory_reads', 0)
        memory_bytes = stats.get('memory_bytes_read', 0)
        if memory_bytes >= 1024 * 1024:
            memory_str = f"{fmt(memory_bytes / (1024 * 1024))} MB"
        elif memory_bytes >= 1024:
            memory_str = f"{fmt(memory_bytes / 1024)} KB"
        else:
            memory_str = f"{memory_bytes} B"
        print(f"  {ANSIColors.CYAN}Memory:{ANSIColors.RESET}")
        print(f"    Read operations:  {memory_reads:n} ({memory_str})")

        # Stale invalidations
        stale_invalidations = stats.get('stale_cache_invalidations', 0)
        if stale_invalidations > 0:
            print(f"  {ANSIColors.YELLOW}Stale cache invalidations: {stale_invalidations}{ANSIColors.RESET}")


def _is_process_running(pid):
    if pid <= 0:
        return False
    if os.name == "posix":
        try:
            os.kill(pid, 0)
            return True
        except ProcessLookupError:
            return False
        except PermissionError:
            # EPERM means process exists but we can't signal it
            return True
    elif sys.platform == "win32":
        try:
            _remote_debugging.RemoteUnwinder(pid)
        except Exception:
            return False
        return True
    else:
        raise ValueError(f"Unsupported platform: {sys.platform}")


def sample(
    pid,
    collector,
    *,
    duration_sec=10,
    all_threads=False,
    realtime_stats=False,
    mode=PROFILING_MODE_WALL,
    async_aware=None,
    native=False,
    gc=True,
    opcodes=False,
):
    """Sample a process using the provided collector.

    Args:
        pid: Process ID to sample
        collector: Collector instance to use for gathering samples
        duration_sec: How long to sample for (seconds)
        all_threads: Whether to sample all threads
        realtime_stats: Whether to print real-time sampling statistics
        mode: Profiling mode - WALL (all samples), CPU (only when on CPU),
              GIL (only when holding GIL), ALL (includes GIL and CPU status),
              EXCEPTION (only when thread has an active exception)
        native: Whether to include native frames
        gc: Whether to include GC frames
        opcodes: Whether to include opcode information

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
        opcodes=opcodes,
        skip_non_matching_threads=skip_non_matching_threads,
        collect_stats=realtime_stats,
    )
    profiler.realtime_stats = realtime_stats

    # Run the sampling
    profiler.sample(collector, duration_sec, async_aware=async_aware)

    return collector


def sample_live(
    pid,
    collector,
    *,
    duration_sec=10,
    all_threads=False,
    realtime_stats=False,
    mode=PROFILING_MODE_WALL,
    async_aware=None,
    native=False,
    gc=True,
    opcodes=False,
):
    """Sample a process in live/interactive mode with curses TUI.

    Args:
        pid: Process ID to sample
        collector: LiveStatsCollector instance
        duration_sec: How long to sample for (seconds)
        all_threads: Whether to sample all threads
        realtime_stats: Whether to print real-time sampling statistics
        mode: Profiling mode - WALL (all samples), CPU (only when on CPU),
              GIL (only when holding GIL), ALL (includes GIL and CPU status),
              EXCEPTION (only when thread has an active exception)
        native: Whether to include native frames
        gc: Whether to include GC frames
        opcodes: Whether to include opcode information

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
        opcodes=opcodes,
        skip_non_matching_threads=skip_non_matching_threads,
        collect_stats=realtime_stats,
    )
    profiler.realtime_stats = realtime_stats

    def curses_wrapper_func(stdscr):
        collector.init_curses(stdscr)
        try:
            profiler.sample(collector, duration_sec, async_aware=async_aware)
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
