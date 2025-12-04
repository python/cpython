"""LiveStatsCollector - Main collector class for live profiling."""

import collections
import contextlib
import curses
from dataclasses import dataclass, field
import os
import site
import sys
import sysconfig
import time
import _colorize

from ..collector import Collector
from ..constants import (
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_ON_CPU,
    THREAD_STATUS_UNKNOWN,
    THREAD_STATUS_GIL_REQUESTED,
    PROFILING_MODE_CPU,
    PROFILING_MODE_GIL,
    PROFILING_MODE_WALL,
)
from .constants import (
    MICROSECONDS_PER_SECOND,
    DISPLAY_UPDATE_INTERVAL,
    MIN_TERMINAL_WIDTH,
    MIN_TERMINAL_HEIGHT,
    HEADER_LINES,
    FOOTER_LINES,
    SAFETY_MARGIN,
    FINISHED_BANNER_EXTRA_LINES,
    DEFAULT_SORT_BY,
    DEFAULT_DISPLAY_LIMIT,
    COLOR_PAIR_HEADER_BG,
    COLOR_PAIR_CYAN,
    COLOR_PAIR_YELLOW,
    COLOR_PAIR_GREEN,
    COLOR_PAIR_MAGENTA,
    COLOR_PAIR_RED,
    COLOR_PAIR_SORTED_HEADER,
)
from .display import CursesDisplay
from .widgets import HeaderWidget, TableWidget, FooterWidget, HelpWidget
from .trend_tracker import TrendTracker


@dataclass
class ThreadData:
    """Encapsulates all profiling data for a single thread."""

    thread_id: int

    # Function call statistics: {location: {direct_calls: int, cumulative_calls: int}}
    result: dict = field(default_factory=lambda: collections.defaultdict(
        lambda: dict(direct_calls=0, cumulative_calls=0)
    ))

    # Thread status statistics
    has_gil: int = 0
    on_cpu: int = 0
    gil_requested: int = 0
    unknown: int = 0
    total: int = 0  # Total status samples for this thread

    # Sample counts
    sample_count: int = 0
    gc_frame_samples: int = 0

    def increment_status_flag(self, status_flags):
        """Update status counts based on status bit flags."""
        if status_flags & THREAD_STATUS_HAS_GIL:
            self.has_gil += 1
        if status_flags & THREAD_STATUS_ON_CPU:
            self.on_cpu += 1
        if status_flags & THREAD_STATUS_GIL_REQUESTED:
            self.gil_requested += 1
        if status_flags & THREAD_STATUS_UNKNOWN:
            self.unknown += 1
        self.total += 1

    def as_status_dict(self):
        """Return status counts as a dict for compatibility."""
        return {
            "has_gil": self.has_gil,
            "on_cpu": self.on_cpu,
            "gil_requested": self.gil_requested,
            "unknown": self.unknown,
            "total": self.total,
        }


class LiveStatsCollector(Collector):
    """Collector that displays live top-like statistics using ncurses."""

    def __init__(
        self,
        sample_interval_usec,
        *,
        skip_idle=False,
        sort_by=DEFAULT_SORT_BY,
        limit=DEFAULT_DISPLAY_LIMIT,
        pid=None,
        display=None,
        mode=None,
    ):
        """
        Initialize the live stats collector.

        Args:
            sample_interval_usec: Sampling interval in microseconds
            skip_idle: Whether to skip idle threads
            sort_by: Sort key ('tottime', 'nsamples', 'cumtime', 'sample_pct', 'cumul_pct')
            limit: Maximum number of functions to display
            pid: Process ID being profiled
            display: DisplayInterface implementation (None means curses will be used)
            mode: Profiling mode ('cpu', 'gil', etc.) - affects what stats are shown
        """
        self.result = collections.defaultdict(
            lambda: dict(total_rec_calls=0, direct_calls=0, cumulative_calls=0)
        )
        self.sample_interval_usec = sample_interval_usec
        self.sample_interval_sec = (
            sample_interval_usec / MICROSECONDS_PER_SECOND
        )
        self.skip_idle = skip_idle
        self.sort_by = sort_by
        self.limit = limit
        self.total_samples = 0
        self.start_time = None
        self.stdscr = None
        self.display = display  # DisplayInterface implementation
        self.running = True
        self.pid = pid
        self.mode = mode  # Profiling mode
        self._saved_stdout = None
        self._saved_stderr = None
        self._devnull = None
        self._last_display_update = None
        self.max_sample_rate = 0  # Track maximum sample rate seen
        self.successful_samples = 0  # Track samples that captured frames
        self.failed_samples = 0  # Track samples that failed to capture frames
        self.display_update_interval = DISPLAY_UPDATE_INTERVAL  # Instance variable for display refresh rate

        # Thread status statistics (bit flags)
        self.thread_status_counts = {
            "has_gil": 0,
            "on_cpu": 0,
            "gil_requested": 0,
            "unknown": 0,
            "total": 0,  # Total thread count across all samples
        }
        self.gc_frame_samples = 0  # Track samples with GC frames

        # Interactive controls state
        self.paused = False  # Pause UI updates (profiling continues)
        self.show_help = False  # Show help screen
        self.filter_pattern = None  # Glob pattern to filter functions
        self.filter_input_mode = False  # Currently entering filter text
        self.filter_input_buffer = ""  # Buffer for filter input
        self.finished = False  # Program has finished, showing final state
        self.finish_timestamp = None  # When profiling finished (for time freezing)
        self.finish_wall_time = None  # Wall clock time when profiling finished

        # Thread tracking state
        self.thread_ids = []  # List of thread IDs seen
        self.view_mode = "ALL"  # "ALL" or "PER_THREAD"
        self.current_thread_index = (
            0  # Index into thread_ids when in PER_THREAD mode
        )
        self.per_thread_data = {}  # {thread_id: ThreadData}

        # Calculate common path prefixes to strip
        self._path_prefixes = self._get_common_path_prefixes()

        # Widgets (initialized when display is available)
        self.header_widget = None
        self.table_widget = None
        self.footer_widget = None
        self.help_widget = None

        # Color mode
        self._can_colorize = _colorize.can_colorize()

        # Trend tracking (initialized after colors are set up)
        self._trend_tracker = None

    @property
    def elapsed_time(self):
        """Get the elapsed time, frozen when finished."""
        if self.finished and self.finish_timestamp is not None:
            return self.finish_timestamp - self.start_time
        return time.perf_counter() - self.start_time if self.start_time else 0

    @property
    def current_time_display(self):
        """Get the current time for display, frozen when finished."""
        if self.finished and self.finish_wall_time is not None:
            return time.strftime("%H:%M:%S", time.localtime(self.finish_wall_time))
        return time.strftime("%H:%M:%S")

    def _get_or_create_thread_data(self, thread_id):
        """Get or create ThreadData for a thread ID."""
        if thread_id not in self.per_thread_data:
            self.per_thread_data[thread_id] = ThreadData(thread_id=thread_id)
        return self.per_thread_data[thread_id]

    def _get_current_thread_data(self):
        """Get ThreadData for currently selected thread in PER_THREAD mode."""
        if self.view_mode == "PER_THREAD" and self.current_thread_index < len(self.thread_ids):
            thread_id = self.thread_ids[self.current_thread_index]
            return self.per_thread_data.get(thread_id)
        return None

    def _get_current_result_source(self):
        """Get result dict for current view mode (aggregated or per-thread)."""
        if self.view_mode == "ALL":
            return self.result
        thread_data = self._get_current_thread_data()
        return thread_data.result if thread_data else {}

    def _get_common_path_prefixes(self):
        """Get common path prefixes to strip from file paths."""
        prefixes = []

        # Get the actual stdlib location from the os module
        # This works for both installed Python and development builds
        os_module_file = os.__file__
        if os_module_file:
            # os.__file__ points to os.py, get its directory
            stdlib_dir = os.path.dirname(os.path.abspath(os_module_file))
            prefixes.append(stdlib_dir)

        # Get stdlib location from sysconfig (may be different or same)
        stdlib_path = sysconfig.get_path("stdlib")
        if stdlib_path:
            prefixes.append(stdlib_path)

        # Get platstdlib location (platform-specific stdlib)
        platstdlib_path = sysconfig.get_path("platstdlib")
        if platstdlib_path:
            prefixes.append(platstdlib_path)

        # Get site-packages locations
        for site_path in site.getsitepackages():
            prefixes.append(site_path)

        # Also check user site-packages
        user_site = site.getusersitepackages()
        if user_site:
            prefixes.append(user_site)

        # Remove duplicates and sort by length (longest first) to match most specific paths first
        prefixes = list(set(prefixes))
        prefixes.sort(key=lambda x: len(x), reverse=True)

        return prefixes

    def simplify_path(self, filepath):
        """Simplify a file path by removing common prefixes."""
        # Try to match against known prefixes
        for prefix_path in self._path_prefixes:
            if filepath.startswith(prefix_path):
                # Remove the prefix completely
                relative = filepath[len(prefix_path) :].lstrip(os.sep)
                return relative

        # If no match, return the original path
        return filepath

    def process_frames(self, frames, thread_id=None):
        """Process a single thread's frame stack.

        Args:
            frames: List of frame information
            thread_id: Thread ID for per-thread tracking (optional)
        """
        if not frames:
            return

        # Get per-thread data if tracking per-thread
        thread_data = self._get_or_create_thread_data(thread_id) if thread_id is not None else None

        # Process each frame in the stack to track cumulative calls
        for frame in frames:
            location = (frame.filename, frame.lineno, frame.funcname)
            self.result[location]["cumulative_calls"] += 1
            if thread_data:
                thread_data.result[location]["cumulative_calls"] += 1

        # The top frame gets counted as an inline call (directly executing)
        top_location = (frames[0].filename, frames[0].lineno, frames[0].funcname)
        self.result[top_location]["direct_calls"] += 1
        if thread_data:
            thread_data.result[top_location]["direct_calls"] += 1

    def collect_failed_sample(self):
        self.failed_samples += 1
        self.total_samples += 1

    def collect(self, stack_frames):
        """Collect and display profiling data."""
        if self.start_time is None:
            self.start_time = time.perf_counter()
            self._last_display_update = self.start_time

        # Thread status counts for this sample
        temp_status_counts = {
            "has_gil": 0,
            "on_cpu": 0,
            "gil_requested": 0,
            "unknown": 0,
            "total": 0,
        }
        has_gc_frame = False

        # Always collect data, even when paused
        # Track thread status flags and GC frames
        for interpreter_info in stack_frames:
            threads = getattr(interpreter_info, "threads", [])
            for thread_info in threads:
                temp_status_counts["total"] += 1

                # Track thread status using bit flags
                status_flags = getattr(thread_info, "status", 0)
                thread_id = getattr(thread_info, "thread_id", None)

                # Update aggregated counts
                if status_flags & THREAD_STATUS_HAS_GIL:
                    temp_status_counts["has_gil"] += 1
                if status_flags & THREAD_STATUS_ON_CPU:
                    temp_status_counts["on_cpu"] += 1
                if status_flags & THREAD_STATUS_GIL_REQUESTED:
                    temp_status_counts["gil_requested"] += 1
                if status_flags & THREAD_STATUS_UNKNOWN:
                    temp_status_counts["unknown"] += 1

                # Update per-thread status counts
                if thread_id is not None:
                    thread_data = self._get_or_create_thread_data(thread_id)
                    thread_data.increment_status_flag(status_flags)

                # Process frames (respecting skip_idle)
                if self.skip_idle:
                    has_gil = bool(status_flags & THREAD_STATUS_HAS_GIL)
                    on_cpu = bool(status_flags & THREAD_STATUS_ON_CPU)
                    if not (has_gil or on_cpu):
                        continue

                frames = getattr(thread_info, "frame_info", None)
                if frames:
                    self.process_frames(frames, thread_id=thread_id)

                    # Track thread IDs only for threads that actually have samples
                    if (
                        thread_id is not None
                        and thread_id not in self.thread_ids
                    ):
                        self.thread_ids.append(thread_id)

                    # Increment per-thread sample count and check for GC frames
                    thread_has_gc_frame = False
                    for frame in frames:
                        funcname = getattr(frame, "funcname", "")
                        if "<GC>" in funcname or "gc_collect" in funcname:
                            has_gc_frame = True
                            thread_has_gc_frame = True
                            break

                    if thread_id is not None:
                        thread_data = self._get_or_create_thread_data(thread_id)
                        thread_data.sample_count += 1
                        if thread_has_gc_frame:
                            thread_data.gc_frame_samples += 1

        # Update cumulative thread status counts
        for key, count in temp_status_counts.items():
            self.thread_status_counts[key] += count

        if has_gc_frame:
            self.gc_frame_samples += 1

        self.successful_samples += 1
        self.total_samples += 1

        # Handle input on every sample for instant responsiveness
        if self.display is not None:
            self._handle_input()

        # Update display at configured rate if display is initialized and not paused
        if self.display is not None and not self.paused:
            current_time = time.perf_counter()
            if (
                self._last_display_update is None
                or (current_time - self._last_display_update)
                >= self.display_update_interval
            ):
                self._update_display()
                self._last_display_update = current_time

    def _prepare_display_data(self, height):
        """Prepare data for display rendering."""
        elapsed = self.elapsed_time
        stats_list = self.build_stats_list()

        # Calculate available space for stats
        # Add extra lines for finished banner when in finished state
        extra_header_lines = (
            FINISHED_BANNER_EXTRA_LINES if self.finished else 0
        )
        max_stats_lines = max(
            0,
            height
            - HEADER_LINES
            - extra_header_lines
            - FOOTER_LINES
            - SAFETY_MARGIN,
        )
        stats_list = stats_list[:max_stats_lines]

        return elapsed, stats_list

    def _initialize_widgets(self, colors):
        """Initialize widgets with display and colors."""
        if self.header_widget is None:
            # Initialize trend tracker with colors
            if self._trend_tracker is None:
                self._trend_tracker = TrendTracker(colors, enabled=True)

            self.header_widget = HeaderWidget(self.display, colors, self)
            self.table_widget = TableWidget(self.display, colors, self)
            self.footer_widget = FooterWidget(self.display, colors, self)
            self.help_widget = HelpWidget(self.display, colors)

    def _render_display_sections(
        self, height, width, elapsed, stats_list, colors
    ):
        """Render all display sections to the screen."""
        line = 0
        try:
            # Initialize widgets if not already done
            self._initialize_widgets(colors)

            # Render header
            line = self.header_widget.render(
                line, width, elapsed=elapsed, stats_list=stats_list
            )

            # Render table
            line = self.table_widget.render(
                line, width, height=height, stats_list=stats_list
            )

        except curses.error:
            pass

    def _update_display(self):
        """Update the display with current stats."""
        try:
            # Clear screen and get dimensions
            self.display.clear()
            height, width = self.display.get_dimensions()

            # Check terminal size
            if width < MIN_TERMINAL_WIDTH or height < MIN_TERMINAL_HEIGHT:
                self._show_terminal_too_small(height, width)
                self.display.refresh()
                return

            # Setup colors and initialize widgets (needed for both help and normal display)
            colors = self._setup_colors()
            self._initialize_widgets(colors)

            # Show help screen if requested
            if self.show_help:
                self.help_widget.render(0, width, height=height)
                self.display.refresh()
                return

            # Prepare data
            elapsed, stats_list = self._prepare_display_data(height)

            # Render all sections
            self._render_display_sections(
                height, width, elapsed, stats_list, colors
            )

            # Footer
            self.footer_widget.render(height - 2, width)

            # Show filter input prompt if in filter input mode
            if self.filter_input_mode:
                self.footer_widget.render_filter_input_prompt(
                    height - 1, width
                )

            # Refresh display
            self.display.redraw()
            self.display.refresh()

        except Exception:
            pass

    def _cycle_sort(self, reverse=False):
        """Cycle through different sort modes in column order.

        Args:
            reverse: If True, cycle backwards (right to left), otherwise forward (left to right)
        """
        sort_modes = [
            "nsamples",
            "sample_pct",
            "tottime",
            "cumul_pct",
            "cumtime",
        ]
        try:
            current_idx = sort_modes.index(self.sort_by)
            if reverse:
                self.sort_by = sort_modes[(current_idx - 1) % len(sort_modes)]
            else:
                self.sort_by = sort_modes[(current_idx + 1) % len(sort_modes)]
        except ValueError:
            self.sort_by = "nsamples"

    def _setup_colors(self):
        """Set up color pairs and return color attributes."""

        A_BOLD = self.display.get_attr("A_BOLD")
        A_REVERSE = self.display.get_attr("A_REVERSE")
        A_UNDERLINE = self.display.get_attr("A_UNDERLINE")
        A_NORMAL = self.display.get_attr("A_NORMAL")

        # Check both curses color support and _colorize.can_colorize()
        if self.display.has_colors() and self._can_colorize:
            with contextlib.suppress(Exception):
                # Color constants (using curses values for compatibility)
                COLOR_CYAN = 6
                COLOR_GREEN = 2
                COLOR_YELLOW = 3
                COLOR_BLACK = 0
                COLOR_MAGENTA = 5
                COLOR_RED = 1

                # Initialize all color pairs used throughout the UI
                self.display.init_color_pair(
                    1, COLOR_CYAN, -1
                )  # Data colors for stats rows
                self.display.init_color_pair(2, COLOR_GREEN, -1)
                self.display.init_color_pair(3, COLOR_YELLOW, -1)
                self.display.init_color_pair(
                    COLOR_PAIR_HEADER_BG, COLOR_BLACK, COLOR_GREEN
                )
                self.display.init_color_pair(
                    COLOR_PAIR_CYAN, COLOR_CYAN, COLOR_BLACK
                )
                self.display.init_color_pair(
                    COLOR_PAIR_YELLOW, COLOR_YELLOW, COLOR_BLACK
                )
                self.display.init_color_pair(
                    COLOR_PAIR_GREEN, COLOR_GREEN, COLOR_BLACK
                )
                self.display.init_color_pair(
                    COLOR_PAIR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK
                )
                self.display.init_color_pair(
                    COLOR_PAIR_RED, COLOR_RED, COLOR_BLACK
                )
                self.display.init_color_pair(
                    COLOR_PAIR_SORTED_HEADER, COLOR_BLACK, COLOR_YELLOW
                )

                return {
                    "header": self.display.get_color_pair(COLOR_PAIR_HEADER_BG)
                    | A_BOLD,
                    "cyan": self.display.get_color_pair(COLOR_PAIR_CYAN)
                    | A_BOLD,
                    "yellow": self.display.get_color_pair(COLOR_PAIR_YELLOW)
                    | A_BOLD,
                    "green": self.display.get_color_pair(COLOR_PAIR_GREEN)
                    | A_BOLD,
                    "magenta": self.display.get_color_pair(COLOR_PAIR_MAGENTA)
                    | A_BOLD,
                    "red": self.display.get_color_pair(COLOR_PAIR_RED)
                    | A_BOLD,
                    "sorted_header": self.display.get_color_pair(
                        COLOR_PAIR_SORTED_HEADER
                    )
                    | A_BOLD,
                    "normal_header": A_REVERSE | A_BOLD,
                    "color_samples": self.display.get_color_pair(1),
                    "color_file": self.display.get_color_pair(2),
                    "color_func": self.display.get_color_pair(3),
                    # Trend colors (stock-like indicators)
                    "trend_up": self.display.get_color_pair(COLOR_PAIR_GREEN) | A_BOLD,
                    "trend_down": self.display.get_color_pair(COLOR_PAIR_RED) | A_BOLD,
                    "trend_stable": A_NORMAL,
                }

        # Fallback to non-color attributes
        return {
            "header": A_REVERSE | A_BOLD,
            "cyan": A_BOLD,
            "yellow": A_BOLD,
            "green": A_BOLD,
            "magenta": A_BOLD,
            "red": A_BOLD,
            "sorted_header": A_REVERSE | A_BOLD | A_UNDERLINE,
            "normal_header": A_REVERSE | A_BOLD,
            "color_samples": A_NORMAL,
            "color_file": A_NORMAL,
            "color_func": A_NORMAL,
            # Trend colors (fallback to bold/normal for monochrome)
            "trend_up": A_BOLD,
            "trend_down": A_BOLD,
            "trend_stable": A_NORMAL,
        }

    def build_stats_list(self):
        """Build and sort the statistics list."""
        stats_list = []
        result_source = self._get_current_result_source()

        for func, call_counts in result_source.items():
            # Apply filter if set (using substring matching)
            if self.filter_pattern:
                filename, lineno, funcname = func
                # Simple substring match (case-insensitive)
                pattern_lower = self.filter_pattern.lower()
                filename_lower = filename.lower()
                funcname_lower = funcname.lower()

                # Match if pattern is substring of filename, funcname, or combined
                matched = (
                    pattern_lower in filename_lower
                    or pattern_lower in funcname_lower
                    or pattern_lower in f"{filename_lower}:{funcname_lower}"
                )
                if not matched:
                    continue

            direct_calls = call_counts.get("direct_calls", 0)
            cumulative_calls = call_counts.get("cumulative_calls", 0)
            total_time = direct_calls * self.sample_interval_sec
            cumulative_time = cumulative_calls * self.sample_interval_sec

            # Calculate sample percentages
            sample_pct = (direct_calls / self.total_samples * 100) if self.total_samples > 0 else 0
            cumul_pct = (cumulative_calls / self.total_samples * 100) if self.total_samples > 0 else 0

            # Calculate trends for all columns using TrendTracker
            trends = {}
            if self._trend_tracker is not None:
                trends = self._trend_tracker.update_metrics(
                    func,
                    {
                        'nsamples': direct_calls,
                        'tottime': total_time,
                        'cumtime': cumulative_time,
                        'sample_pct': sample_pct,
                        'cumul_pct': cumul_pct,
                    }
                )

            stats_list.append(
                {
                    "func": func,
                    "direct_calls": direct_calls,
                    "cumulative_calls": cumulative_calls,
                    "total_time": total_time,
                    "cumulative_time": cumulative_time,
                    "trends": trends,  # Dictionary of trends for all columns
                }
            )

        # Sort the stats
        if self.sort_by == "nsamples":
            stats_list.sort(key=lambda x: x["direct_calls"], reverse=True)
        elif self.sort_by == "tottime":
            stats_list.sort(key=lambda x: x["total_time"], reverse=True)
        elif self.sort_by == "cumtime":
            stats_list.sort(key=lambda x: x["cumulative_time"], reverse=True)
        elif self.sort_by == "sample_pct":
            stats_list.sort(
                key=lambda x: (x["direct_calls"] / self.total_samples * 100)
                if self.total_samples > 0
                else 0,
                reverse=True,
            )
        elif self.sort_by == "cumul_pct":
            stats_list.sort(
                key=lambda x: (
                    x["cumulative_calls"] / self.total_samples * 100
                )
                if self.total_samples > 0
                else 0,
                reverse=True,
            )

        return stats_list

    def reset_stats(self):
        """Reset all collected statistics."""
        self.result.clear()
        self.per_thread_data.clear()
        self.thread_ids.clear()
        self.view_mode = "ALL"
        self.current_thread_index = 0
        self.total_samples = 0
        self.successful_samples = 0
        self.failed_samples = 0
        self.max_sample_rate = 0
        self.thread_status_counts = {
            "has_gil": 0,
            "on_cpu": 0,
            "gil_requested": 0,
            "unknown": 0,
            "total": 0,
        }
        self.gc_frame_samples = 0
        # Clear trend tracking
        if self._trend_tracker is not None:
            self._trend_tracker.clear()
        # Reset finished state and finish timestamp
        self.finished = False
        self.finish_timestamp = None
        self.finish_wall_time = None
        self.start_time = time.perf_counter()
        self._last_display_update = self.start_time

    def mark_finished(self):
        """Mark the profiling session as finished."""
        self.finished = True
        # Capture the finish timestamp to freeze all timing displays
        self.finish_timestamp = time.perf_counter()
        self.finish_wall_time = time.time()  # Wall clock time for display
        # Force a final display update to show the finished message
        if self.display is not None:
            self._update_display()

    def _handle_finished_input_update(self, had_input):
        """Update display after input when program is finished."""
        if self.finished and had_input and self.display is not None:
            self._update_display()

    def _show_terminal_too_small(self, height, width):
        """Display a message when terminal is too small."""
        A_BOLD = self.display.get_attr("A_BOLD")
        msg1 = "Terminal too small!"
        msg2 = f"Need: {MIN_TERMINAL_WIDTH}x{MIN_TERMINAL_HEIGHT}"
        msg3 = f"Have: {width}x{height}"
        msg4 = "Please resize"

        # Center the messages
        if height >= 4:
            self.display.add_str(
                height // 2 - 2,
                max(0, (width - len(msg1)) // 2),
                msg1[: width - 1],
                A_BOLD,
            )
            self.display.add_str(
                height // 2 - 1,
                max(0, (width - len(msg2)) // 2),
                msg2[: width - 1],
            )
            self.display.add_str(
                height // 2,
                max(0, (width - len(msg3)) // 2),
                msg3[: width - 1],
            )
            self.display.add_str(
                height // 2 + 1,
                max(0, (width - len(msg4)) // 2),
                msg4[: width - 1],
            )
        elif height >= 1:
            self.display.add_str(0, 0, msg1[: width - 1], A_BOLD)

    def _show_terminal_size_warning_and_wait(self, height, width):
        """Show terminal size warning during initialization and wait for user acknowledgment."""
        A_BOLD = self.display.get_attr("A_BOLD")
        A_DIM = self.display.get_attr("A_DIM")

        self.display.clear()
        msg1 = "WARNING: Terminal too small!"
        msg2 = f"Required: {MIN_TERMINAL_WIDTH}x{MIN_TERMINAL_HEIGHT}"
        msg3 = f"Current:  {width}x{height}"
        msg4 = "Please resize your terminal for best experience"
        msg5 = "Press any key to continue..."

        # Center the messages
        if height >= 5:
            self.display.add_str(
                height // 2 - 2,
                max(0, (width - len(msg1)) // 2),
                msg1[: width - 1],
                A_BOLD,
            )
            self.display.add_str(
                height // 2 - 1,
                max(0, (width - len(msg2)) // 2),
                msg2[: width - 1],
            )
            self.display.add_str(
                height // 2,
                max(0, (width - len(msg3)) // 2),
                msg3[: width - 1],
            )
            self.display.add_str(
                height // 2 + 1,
                max(0, (width - len(msg4)) // 2),
                msg4[: width - 1],
            )
            self.display.add_str(
                height // 2 + 3,
                max(0, (width - len(msg5)) // 2),
                msg5[: width - 1],
                A_DIM,
            )
        elif height >= 1:
            self.display.add_str(0, 0, msg1[: width - 1], A_BOLD)

        self.display.refresh()
        # Wait for user acknowledgment (2 seconds timeout)
        self.display.set_nodelay(False)
        # Note: timeout is curses-specific, skipping for now
        self.display.get_input()
        self.display.set_nodelay(True)

    def _handle_input(self):
        """Handle keyboard input (non-blocking)."""
        from . import constants

        self.display.set_nodelay(True)
        ch = self.display.get_input()

        # Handle filter input mode FIRST - takes precedence over all commands
        if self.filter_input_mode:
            if ch == 27:  # ESC key
                self.filter_input_mode = False
                self.filter_input_buffer = ""
            elif ch == 10 or ch == 13:  # Enter key
                self.filter_pattern = (
                    self.filter_input_buffer
                    if self.filter_input_buffer
                    else None
                )
                self.filter_input_mode = False
                self.filter_input_buffer = ""
            elif ch == 127 or ch == 263:  # Backspace
                if self.filter_input_buffer:
                    self.filter_input_buffer = self.filter_input_buffer[:-1]
            elif ch >= 32 and ch < 127:  # Printable characters
                self.filter_input_buffer += chr(ch)

            # Update display if input was processed while finished
            self._handle_finished_input_update(ch != -1)
            return

        # Handle help toggle keys
        if ch == ord("h") or ch == ord("H") or ch == ord("?"):
            self.show_help = not self.show_help

        # If showing help, any other key closes it
        elif self.show_help and ch != -1:
            self.show_help = False

        # Handle regular commands
        if ch == ord("q") or ch == ord("Q"):
            self.running = False

        elif ch == ord("s"):
            self._cycle_sort(reverse=False)

        elif ch == ord("S"):
            self._cycle_sort(reverse=True)

        elif ch == ord("p") or ch == ord("P"):
            self.paused = not self.paused

        elif ch == ord("r") or ch == ord("R"):
            # Don't allow reset when profiling is finished
            if not self.finished:
                self.reset_stats()

        elif ch == ord("+") or ch == ord("="):
            # Decrease update interval (faster refresh)
            self.display_update_interval = max(
                0.05, self.display_update_interval - 0.05
            )  # Min 20Hz

        elif ch == ord("-") or ch == ord("_"):
            # Increase update interval (slower refresh)
            self.display_update_interval = min(
                1.0, self.display_update_interval + 0.05
            )  # Max 1Hz

        elif ch == ord("c") or ch == ord("C"):
            if self.filter_pattern:
                self.filter_pattern = None

        elif ch == ord("/"):
            self.filter_input_mode = True
            self.filter_input_buffer = self.filter_pattern or ""

        elif ch == ord("t") or ch == ord("T"):
            # Toggle between ALL and PER_THREAD modes
            if self.view_mode == "ALL":
                if len(self.thread_ids) > 0:
                    self.view_mode = "PER_THREAD"
                    self.current_thread_index = 0
            else:
                self.view_mode = "ALL"

        elif ch == ord("x") or ch == ord("X"):
            # Toggle trend colors on/off
            if self._trend_tracker is not None:
                self._trend_tracker.toggle()

        elif ch == curses.KEY_LEFT or ch == curses.KEY_UP:
            # Navigate to previous thread in PER_THREAD mode, or switch from ALL to PER_THREAD
            if len(self.thread_ids) > 0:
                if self.view_mode == "ALL":
                    self.view_mode = "PER_THREAD"
                    self.current_thread_index = 0
                else:
                    self.current_thread_index = (
                        self.current_thread_index - 1
                    ) % len(self.thread_ids)

        elif ch == curses.KEY_RIGHT or ch == curses.KEY_DOWN:
            # Navigate to next thread in PER_THREAD mode, or switch from ALL to PER_THREAD
            if len(self.thread_ids) > 0:
                if self.view_mode == "ALL":
                    self.view_mode = "PER_THREAD"
                    self.current_thread_index = 0
                else:
                    self.current_thread_index = (
                        self.current_thread_index + 1
                    ) % len(self.thread_ids)

        # Update display if input was processed while finished
        self._handle_finished_input_update(ch != -1)

    def init_curses(self, stdscr):
        """Initialize curses display and suppress stdout/stderr."""
        self.stdscr = stdscr
        self.display = CursesDisplay(stdscr)

        # Check terminal size upfront and warn if too small
        height, width = self.display.get_dimensions()

        if width < MIN_TERMINAL_WIDTH or height < MIN_TERMINAL_HEIGHT:
            # Show warning and wait briefly for user to see it
            self._show_terminal_size_warning_and_wait(height, width)

        curses.curs_set(0)  # Hide cursor
        stdscr.nodelay(True)  # Non-blocking input
        stdscr.scrollok(False)  # Disable scrolling
        stdscr.idlok(False)  # Disable hardware insert/delete
        stdscr.leaveok(True)  # Don't care about cursor position

        if curses.has_colors():
            curses.start_color()
            curses.use_default_colors()

        # Suppress stdout and stderr to prevent interfering with curses display
        # Use contextlib.redirect_stdout/stderr for better resource management
        self._saved_stdout = sys.stdout
        self._saved_stderr = sys.stderr
        # Open devnull and ensure it's cleaned up even if an exception occurs
        try:
            self._devnull = open(os.devnull, "w")
            sys.stdout = self._devnull
            sys.stderr = self._devnull
        except Exception:
            # If redirection fails, restore original streams
            sys.stdout = self._saved_stdout
            sys.stderr = self._saved_stderr
            raise

        # Initial clear
        self.display.clear()
        self.display.refresh()

    def cleanup_curses(self):
        """Clean up curses display and restore stdout/stderr."""
        # Restore stdout and stderr in reverse order
        # Use try-finally to ensure cleanup even if restoration fails
        try:
            if self._saved_stdout is not None:
                sys.stdout = self._saved_stdout
                self._saved_stdout = None
            if self._saved_stderr is not None:
                sys.stderr = self._saved_stderr
                self._saved_stderr = None
        finally:
            # Always close devnull, even if stdout/stderr restoration fails
            if self._devnull is not None:
                with contextlib.suppress(Exception):
                    self._devnull.close()
                self._devnull = None

        if self.display is not None and self.stdscr is not None:
            with contextlib.suppress(Exception):
                curses.curs_set(1)  # Show cursor
                self.display.set_nodelay(False)

    def export(self, filename):
        """Export is not supported in live mode."""
        raise NotImplementedError(
            "Export to file is not supported in live mode. "
            "Use the live TUI to view statistics in real-time."
        )
