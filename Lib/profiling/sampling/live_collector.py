"""Live profiling collector that displays top-like statistics using curses.

                    ┌─────────────────────────────┐
                    │   Target Python Process     │
                    │   (being profiled)          │
                    └──────────────┬──────────────┘
                                   │ Stack sampling at
                                   │ configured interval
                                   │ (e.g., 10000µs)
                                   ▼
                    ┌─────────────────────────────┐
                    │  LiveStatsCollector         │
                    │  ┌───────────────────────┐  │
                    │  │ collect()             │  │ Aggregates samples
                    │  │ - Iterates frames     │  │ into statistics
                    │  │ - Updates counters    │  │
                    │  └───────────┬───────────┘  │
                    │              │              │
                    │              ▼              │
                    │  ┌───────────────────────┐  │
                    │  │ Data Storage          │  │
                    │  │ - result dict         │  │ Tracks per-function:
                    │  │ - direct_calls        │  │ • Direct samples
                    │  │ - cumulative_calls    │  │ • Cumulative samples
                    │  └───────────┬───────────┘  │ • Derived time stats
                    │              │              │
                    │              ▼              │
                    │  ┌───────────────────────┐  │
                    │  │ Display Update        │  │
                    │  │ (10Hz by default)     │  │ Rate-limited refresh
                    │  └───────────┬───────────┘  │
                    └──────────────┼──────────────┘
                                   │
                                   ▼
                    ┌─────────────────────────────┐
                    │   DisplayInterface          │
                    │   (Abstract layer)          │
                    └──────────────┬──────────────┘
                           ┌───────┴────────┐
                           │                │
                ┌──────────▼────────┐  ┌───▼──────────┐
                │ CursesDisplay     │  │ MockDisplay  │
                │ - Real terminal   │  │ - Testing    │
                │ - ncurses backend │  │ - No UI      │
                └─────────┬─────────┘  └──────────────┘
                          │
                          ▼
                ┌─────────────────────────────────────┐
                │      Widget-Based Rendering         │
                │ ┌─────────────────────────────────┐ │
                │ │  HeaderWidget                   │ │
                │ │  • PID, uptime, time, interval  │ │
                │ │  • Sample stats & progress bar  │ │
                │ │  • Efficiency bar               │ │
                │ │  • Function summary             │ │
                │ │  • Top 3 hottest functions      │ │
                │ ├─────────────────────────────────┤ │
                │ │  TableWidget                    │ │
                │ │  • Column headers (sortable)    │ │ Interactive display
                │ │  • Stats rows (scrolling)       │ │ with keyboard controls:
                │ │    - nsamples  %  time          │ │ s: sort, p: pause
                │ │    - function  file:line        │ │ r: reset, /: filter
                │ ├─────────────────────────────────┤ │ q: quit, h: help
                │ │  FooterWidget                   │ │
                │ │  • Legend and status            │ │
                │ │  • Filter input prompt          │ │
                │ └─────────────────────────────────┘ │
                └─────────────────────────────────────┘

Architecture:

The live collector is organized into four layers. The data collection layer
(LiveStatsCollector) aggregates stack samples into per-function statistics without
any knowledge of how they will be presented. The display abstraction layer
(DisplayInterface) defines rendering operations without coupling to curses or any
specific UI framework. The widget layer (Widget, HeaderWidget, TableWidget,
FooterWidget, HelpWidget, ProgressBarWidget) encapsulates individual UI components
with their own rendering logic, promoting modularity and reusability. The
presentation layer (CursesDisplay/MockDisplay) implements the actual rendering for
terminal output and testing.

The system runs two independent update loops. The sampling loop is driven by the
profiler at the configured interval (e.g., 10000µs) and continuously collects
stack frames and updates statistics. The display loop runs at a fixed refresh rate
(default 10Hz) and updates the terminal independently of sampling frequency. This
separation allows high-frequency sampling without overwhelming the terminal with
constant redraws.

Statistics are computed incrementally as samples arrive. The collector maintains
running counters (direct calls and cumulative calls) in a dictionary keyed by
function location. Derived metrics like time estimates and percentages are computed
on-demand during display updates rather than being stored, which minimizes memory
overhead as the number of tracked functions grows.

User input is processed asynchronously during display updates using non-blocking I/O.
This allows interactive controls (sorting, filtering, pausing) without interrupting
the data collection pipeline. The collector maintains mode flags (paused,
filter_input_mode) that affect what gets displayed but not what gets collected.

"""

import collections
import contextlib
import curses
import fnmatch
import os
import site
import sys
import sysconfig
import time
from abc import ABC, abstractmethod
from .collector import Collector, THREAD_STATE_RUNNING

# Time conversion constants
MICROSECONDS_PER_SECOND = 1_000_000

# Display update constants
DISPLAY_UPDATE_HZ = 10
DISPLAY_UPDATE_INTERVAL = 1.0 / DISPLAY_UPDATE_HZ  # 0.1 seconds

# Terminal size constraints
MIN_TERMINAL_WIDTH = 60
MIN_TERMINAL_HEIGHT = 12

# Column width thresholds
WIDTH_THRESHOLD_SAMPLE_PCT = 80
WIDTH_THRESHOLD_TOTTIME = 100
WIDTH_THRESHOLD_CUMUL_PCT = 120
WIDTH_THRESHOLD_CUMTIME = 140

# Display layout constants
HEADER_LINES = 9
FOOTER_LINES = 2
SAFETY_MARGIN = 1
TOP_FUNCTIONS_DISPLAY_COUNT = 3

# Column widths for data display
COL_WIDTH_NSAMPLES = 13
COL_SPACING = 2
COL_WIDTH_SAMPLE_PCT = 5
COL_WIDTH_TIME = 10

# Function name display
MIN_FUNC_NAME_WIDTH = 10
MAX_FUNC_NAME_WIDTH = 40
MIN_AVAILABLE_SPACE = 10

# Progress bar display
MIN_BAR_WIDTH = 10
MAX_SAMPLE_RATE_BAR_WIDTH = 30
MAX_EFFICIENCY_BAR_WIDTH = 60

# Sample rate scaling
MIN_SAMPLE_RATE_FOR_SCALING = 100

# Finished banner display
FINISHED_BANNER_EXTRA_LINES = 3  # Blank line + banner + blank line

# Color pair IDs
COLOR_PAIR_HEADER_BG = 4
COLOR_PAIR_CYAN = 5
COLOR_PAIR_YELLOW = 6
COLOR_PAIR_GREEN = 7
COLOR_PAIR_MAGENTA = 8
COLOR_PAIR_RED = 9
COLOR_PAIR_SORTED_HEADER = 10

# Default display settings
DEFAULT_SORT_BY = "tottime"
DEFAULT_DISPLAY_LIMIT = 20


class Widget(ABC):
    """Base class for UI widgets."""

    def __init__(self, display, colors):
        """
        Initialize widget.

        Args:
            display: DisplayInterface implementation
            colors: Dictionary of color attributes
        """
        self.display = display
        self.colors = colors

    @abstractmethod
    def render(self, line, width, **kwargs):
        """
        Render the widget starting at the given line.

        Args:
            line: Starting line number
            width: Available width
            **kwargs: Additional rendering parameters

        Returns:
            Next available line number after rendering
        """
        pass

    def add_str(self, line, col, text, attr=0):
        """Add a string to the display at the specified position."""
        self.display.add_str(line, col, text, attr)


class ProgressBarWidget(Widget):
    """Reusable progress bar widget."""

    def render(self, line, width, **kwargs):
        """Render is not used for progress bars - use render_bar instead."""
        raise NotImplementedError("Use render_bar method instead")

    def render_bar(
        self, filled, total, max_width, fill_char="█", empty_char="░"
    ):
        """
        Render a progress bar and return the bar string and its length.

        Args:
            filled: Current filled amount
            total: Total amount (max value)
            max_width: Maximum width for the bar
            fill_char: Character to use for filled portion
            empty_char: Character to use for empty portion

        Returns:
            Tuple of (bar_string, bar_length)
        """
        bar_width = min(max_width, max_width)
        normalized = min(filled / max(total, 1), 1.0)
        bar_fill = int(normalized * bar_width)

        bar = "["
        for i in range(bar_width):
            if i < bar_fill:
                bar += fill_char
            else:
                bar += empty_char
        bar += "]"
        return bar, len(bar)


class HeaderWidget(Widget):
    """Widget for rendering the header section (lines 0-8)."""

    def __init__(self, display, colors, collector):
        """
        Initialize header widget.

        Args:
            display: DisplayInterface implementation
            colors: Dictionary of color attributes
            collector: Reference to LiveStatsCollector for accessing stats
        """
        super().__init__(display, colors)
        self.collector = collector
        self.progress_bar = ProgressBarWidget(display, colors)

    def render(self, line, width, **kwargs):
        """
        Render the complete header section.

        Args:
            line: Starting line number
            width: Available width
            kwargs: Must contain 'elapsed' key

        Returns:
            Next available line number
        """
        elapsed = kwargs["elapsed"]

        line = self.draw_header_info(line, width, elapsed)
        line = self.draw_sample_stats(line, width, elapsed)
        line = self.draw_efficiency_bar(line, width)
        line = self.draw_function_stats(
            line, width, kwargs.get("stats_list", [])
        )
        line = self.draw_top_functions(
            line, width, kwargs.get("stats_list", [])
        )

        # Show prominent finished banner if profiling is complete
        if self.collector.finished:
            line = self.draw_finished_banner(line, width)

        # Separator
        A_DIM = self.display.get_attr("A_DIM")
        separator = "─" * (width - 1)
        self.add_str(line, 0, separator[: width - 1], A_DIM)
        line += 1

        return line

    def format_uptime(self, elapsed):
        """Format elapsed time as uptime string."""
        uptime_sec = int(elapsed)
        hours = uptime_sec // 3600
        minutes = (uptime_sec % 3600) // 60
        seconds = uptime_sec % 60
        if hours > 0:
            return f"{hours}h{minutes:02d}m{seconds:02d}s"
        else:
            return f"{minutes}m{seconds:02d}s"

    def draw_header_info(self, line, width, elapsed):
        """Draw the header information line with PID, uptime, time, and interval."""
        # Draw title
        A_BOLD = self.display.get_attr("A_BOLD")
        title = "Tachyon Profiler"
        self.add_str(line, 0, title, A_BOLD | self.colors["cyan"])
        line += 1

        current_time = time.strftime("%H:%M:%S")
        uptime = self.format_uptime(elapsed)

        # Calculate display refresh rate
        refresh_hz = (
            1.0 / DISPLAY_UPDATE_INTERVAL if DISPLAY_UPDATE_INTERVAL > 0 else 0
        )

        header_parts = [
            ("PID: ", curses.A_BOLD),
            (f"{self.collector.pid}", self.colors["cyan"]),
            (" │ ", curses.A_DIM),
            ("Uptime: ", curses.A_BOLD),
            (uptime, self.colors["green"]),
            (" │ ", curses.A_DIM),
            ("Time: ", curses.A_BOLD),
            (current_time, self.colors["yellow"]),
            (" │ ", curses.A_DIM),
            ("Interval: ", curses.A_BOLD),
            (
                f"{self.collector.sample_interval_usec}µs",
                self.colors["magenta"],
            ),
            (" │ ", curses.A_DIM),
            ("Display: ", curses.A_BOLD),
            (f"{refresh_hz:.1f}Hz", self.colors["cyan"]),
        ]

        col = 0
        for text, attr in header_parts:
            if col < width - 1:
                self.add_str(line, col, text, attr)
                col += len(text)
        return line + 1

    def format_rate_with_units(self, rate_hz):
        """Format a rate in Hz with appropriate units (Hz, KHz, MHz)."""
        if rate_hz >= 1_000_000:
            return f"{rate_hz / 1_000_000:.1f}MHz"
        elif rate_hz >= 1_000:
            return f"{rate_hz / 1_000:.1f}KHz"
        else:
            return f"{rate_hz:.1f}Hz"

    def draw_sample_stats(self, line, width, elapsed):
        """Draw sample statistics with visual progress bar."""
        sample_rate = (
            self.collector.total_samples / elapsed if elapsed > 0 else 0
        )

        # Update max sample rate
        if sample_rate > self.collector._max_sample_rate:
            self.collector._max_sample_rate = sample_rate

        col = 0
        self.add_str(line, col, "Samples: ", curses.A_BOLD)
        col += 9
        self.add_str(
            line,
            col,
            f"{self.collector.total_samples:>8}",
            self.colors["cyan"],
        )
        col += 8
        self.add_str(
            line, col, f" total ({sample_rate:>7.1f}/s) ", curses.A_NORMAL
        )
        col += 23

        # Draw sample rate bar
        target_rate = MICROSECONDS_PER_SECOND / self.collector.sample_interval_usec

        # Show current/target ratio with percentage
        if sample_rate > 0 and target_rate > 0:
            percentage = min((sample_rate / target_rate) * 100, 100)
            current_formatted = self.format_rate_with_units(sample_rate)
            target_formatted = self.format_rate_with_units(target_rate)

            if percentage >= 99.5:  # Show 100% when very close
                rate_label = f" {current_formatted}/{target_formatted} (100%)"
            else:
                rate_label = f" {current_formatted}/{target_formatted} ({percentage:>4.1f}%)"
        else:
            target_formatted = self.format_rate_with_units(target_rate)
            rate_label = f" target: {target_formatted}"

        available_width = width - col - len(rate_label) - 3

        if available_width >= MIN_BAR_WIDTH:
            bar_width = min(MAX_SAMPLE_RATE_BAR_WIDTH, available_width)
            # Use target rate as the reference, with a minimum for scaling
            reference_rate = max(target_rate, MIN_SAMPLE_RATE_FOR_SCALING)
            normalized_rate = min(sample_rate / reference_rate, 1.0)
            bar_fill = int(normalized_rate * bar_width)

            bar = "["
            for i in range(bar_width):
                bar += "█" if i < bar_fill else "░"
            bar += "]"
            self.add_str(line, col, bar, self.colors["green"])
            col += len(bar)

            if col + len(rate_label) < width - 1:
                self.add_str(line, col + 1, rate_label, curses.A_DIM)
        return line + 1

    def draw_efficiency_bar(self, line, width):
        """Draw sample efficiency bar showing success/failure rates."""
        success_pct = (
            self.collector._successful_samples
            / max(1, self.collector.total_samples)
        ) * 100
        failed_pct = (
            self.collector._failed_samples
            / max(1, self.collector.total_samples)
        ) * 100

        col = 0
        self.add_str(line, col, "Efficiency:", curses.A_BOLD)
        col += 11

        label = f" {success_pct:>5.1f}% good, {failed_pct:>4.1f}% failed"
        available_width = width - col - len(label) - 3

        if available_width >= MIN_BAR_WIDTH:
            bar_width = min(MAX_EFFICIENCY_BAR_WIDTH, available_width)
            success_fill = int(
                (
                    self.collector._successful_samples
                    / max(1, self.collector.total_samples)
                )
                * bar_width
            )
            failed_fill = bar_width - success_fill

            self.add_str(line, col, "[", curses.A_NORMAL)
            col += 1
            if success_fill > 0:
                self.add_str(
                    line, col, "█" * success_fill, self.colors["green"]
                )
                col += success_fill
            if failed_fill > 0:
                self.add_str(line, col, "█" * failed_fill, self.colors["red"])
                col += failed_fill
            self.add_str(line, col, "]", curses.A_NORMAL)
            col += 1

            self.add_str(line, col + 1, label, curses.A_NORMAL)
        return line + 1

    def draw_function_stats(self, line, width, stats_list):
        """Draw function statistics summary."""
        total_funcs = len(self.collector.result)
        funcs_shown = len(stats_list)
        executing_funcs = sum(
            1 for f in self.collector.result.values() if f["direct_calls"] > 0
        )
        stack_only = total_funcs - executing_funcs

        col = 0
        self.add_str(line, col, "Functions: ", curses.A_BOLD)
        col += 11
        self.add_str(line, col, f"{total_funcs:>5}", self.colors["cyan"])
        col += 5
        self.add_str(line, col, " total", curses.A_NORMAL)
        col += 6

        if col < width - 25:
            self.add_str(line, col, " │ ", curses.A_DIM)
            col += 3
            self.add_str(
                line, col, f"{executing_funcs:>5}", self.colors["green"]
            )
            col += 5
            self.add_str(line, col, " exec", curses.A_NORMAL)
            col += 5

        if col < width - 25:
            self.add_str(line, col, " │ ", curses.A_DIM)
            col += 3
            self.add_str(line, col, f"{stack_only:>5}", self.colors["yellow"])
            col += 5
            self.add_str(line, col, " stack", curses.A_NORMAL)
            col += 6

        if col < width - 20:
            self.add_str(line, col, " │ ", curses.A_DIM)
            col += 3
            self.add_str(
                line, col, f"{funcs_shown:>5}", self.colors["magenta"]
            )
            col += 5
            self.add_str(line, col, " shown", curses.A_NORMAL)
        return line + 1

    def draw_top_functions(self, line, width, stats_list):
        """Draw top N hottest functions."""
        col = 0
        self.add_str(
            line,
            col,
            f"Top {TOP_FUNCTIONS_DISPLAY_COUNT}:     ",
            curses.A_BOLD,
        )
        col += 11

        top_by_samples = sorted(
            stats_list, key=lambda x: x["direct_calls"], reverse=True
        )
        emojis = ["🥇", "🥈", "🥉"]
        medal_colors = [
            self.colors["red"],
            self.colors["yellow"],
            self.colors["green"],
        ]

        displayed = 0
        for func_data in top_by_samples:
            if displayed >= TOP_FUNCTIONS_DISPLAY_COUNT:
                break
            if col >= width - 20:
                break
            if func_data["direct_calls"] == 0:
                continue

            func_name = func_data["func"][2]
            func_pct = (
                func_data["direct_calls"]
                / max(1, self.collector.total_samples)
            ) * 100

            # Medal emoji
            if col + 3 < width - 15:
                self.add_str(
                    line, col, emojis[displayed] + " ", medal_colors[displayed]
                )
                col += 3

            # Function name (truncate to fit)
            available_for_name = width - col - 15
            max_name_len = min(25, max(5, available_for_name))
            if len(func_name) > max_name_len:
                func_name = func_name[: max_name_len - 3] + "..."

            if col + len(func_name) < width - 10:
                self.add_str(line, col, func_name, medal_colors[displayed])
                col += len(func_name)

                pct_str = (
                    f" ({func_pct:.1f}%)"
                    if func_pct >= 0.1
                    else f" ({func_data['direct_calls']})"
                )
                self.add_str(line, col, pct_str, curses.A_DIM)
                col += len(pct_str)

                displayed += 1

                if displayed < 3 and col < width - 30:
                    self.add_str(line, col, " │ ", curses.A_DIM)
                    col += 3

        if displayed == 0 and col < width - 25:
            self.add_str(line, col, "(collecting samples...)", curses.A_DIM)

        return line + 1

    def draw_finished_banner(self, line, width):
        """Draw a prominent banner when profiling is finished."""
        A_REVERSE = self.display.get_attr("A_REVERSE")
        A_BOLD = self.display.get_attr("A_BOLD")

        # Add blank line for separation
        line += 1

        # Create the banner message
        message = "  ✓ PROFILING COMPLETE - Final Results Below - Press 'q' to Quit  "

        # Center the message and fill the width with reverse video
        if len(message) < width - 1:
            padding_total = width - len(message) - 1
            padding_left = padding_total // 2
            padding_right = padding_total - padding_left
            full_message = " " * padding_left + message + " " * padding_right
        else:
            full_message = message[: width - 1]

        # Draw the banner with reverse video and bold
        self.add_str(
            line, 0, full_message, A_REVERSE | A_BOLD | self.colors["green"]
        )
        line += 1

        # Add blank line for separation
        line += 1

        return line


class TableWidget(Widget):
    """Widget for rendering column headers and data rows."""

    def __init__(self, display, colors, collector):
        """
        Initialize table widget.

        Args:
            display: DisplayInterface implementation
            colors: Dictionary of color attributes
            collector: Reference to LiveStatsCollector for accessing stats
        """
        super().__init__(display, colors)
        self.collector = collector

    def render(self, line, width, **kwargs):
        """
        Render column headers and data rows.

        Args:
            line: Starting line number
            width: Available width
            kwargs: Must contain 'height' and 'stats_list' keys

        Returns:
            Next available line number
        """
        height = kwargs["height"]
        stats_list = kwargs["stats_list"]

        # Draw column headers
        line, show_sample_pct, show_tottime, show_cumul_pct, show_cumtime = (
            self.draw_column_headers(line, width)
        )
        column_flags = (
            show_sample_pct,
            show_tottime,
            show_cumul_pct,
            show_cumtime,
        )

        # Draw data rows
        line = self.draw_stats_rows(
            line, height, width, stats_list, column_flags
        )

        return line

    def draw_column_headers(self, line, width):
        """Draw column headers with sort indicators."""
        col = 0

        # Determine which columns to show based on width
        show_sample_pct = width >= WIDTH_THRESHOLD_SAMPLE_PCT
        show_tottime = width >= WIDTH_THRESHOLD_TOTTIME
        show_cumul_pct = width >= WIDTH_THRESHOLD_CUMUL_PCT
        show_cumtime = width >= WIDTH_THRESHOLD_CUMTIME

        sorted_header = self.colors["sorted_header"]
        normal_header = self.colors["normal_header"]

        # Determine which column is sorted
        sort_col = {
            "nsamples": 0,
            "sample_pct": 1,
            "tottime": 2,
            "cumul_pct": 3,
            "cumtime": 4,
        }.get(self.collector.sort_by, -1)

        # Column 0: nsamples
        attr = sorted_header if sort_col == 0 else normal_header
        text = f"{'▼nsamples' if sort_col == 0 else 'nsamples':>13}"
        self.add_str(line, col, text, attr)
        col += 15

        # Column 1: sample %
        if show_sample_pct:
            attr = sorted_header if sort_col == 1 else normal_header
            text = f"{'▼%' if sort_col == 1 else '%':>5}"
            self.add_str(line, col, text, attr)
            col += 7

        # Column 2: tottime
        if show_tottime:
            attr = sorted_header if sort_col == 2 else normal_header
            text = f"{'▼tottime' if sort_col == 2 else 'tottime':>10}"
            self.add_str(line, col, text, attr)
            col += 12

        # Column 3: cumul %
        if show_cumul_pct:
            attr = sorted_header if sort_col == 3 else normal_header
            text = f"{'▼%' if sort_col == 3 else '%':>5}"
            self.add_str(line, col, text, attr)
            col += 7

        # Column 4: cumtime
        if show_cumtime:
            attr = sorted_header if sort_col == 4 else normal_header
            text = f"{'▼cumtime' if sort_col == 4 else 'cumtime':>10}"
            self.add_str(line, col, text, attr)
            col += 12

        # Remaining headers
        if col < width - 15:
            remaining_space = width - col - 1
            func_width = min(
                MAX_FUNC_NAME_WIDTH,
                max(MIN_FUNC_NAME_WIDTH, remaining_space // 2),
            )
            self.add_str(
                line, col, f"{'function':<{func_width}}", normal_header
            )
            col += func_width + 2

            if col < width - 10:
                self.add_str(line, col, "file:line", normal_header)

        return (
            line + 1,
            show_sample_pct,
            show_tottime,
            show_cumul_pct,
            show_cumtime,
        )

    def draw_stats_rows(self, line, height, width, stats_list, column_flags):
        """Draw the statistics data rows."""
        show_sample_pct, show_tottime, show_cumul_pct, show_cumtime = (
            column_flags
        )

        # Get color attributes from the colors dict (already initialized)
        color_samples = self.colors.get("color_samples", curses.A_NORMAL)
        color_file = self.colors.get("color_file", curses.A_NORMAL)
        color_func = self.colors.get("color_func", curses.A_NORMAL)

        for stat in stats_list:
            if line >= height - FOOTER_LINES:
                break

            func = stat["func"]
            direct_calls = stat["direct_calls"]
            cumulative_calls = stat["cumulative_calls"]
            total_time = stat["total_time"]
            cumulative_time = stat["cumulative_time"]

            sample_pct = (
                (direct_calls / self.collector.total_samples * 100)
                if self.collector.total_samples > 0
                else 0
            )
            cum_pct = (
                (cumulative_calls / self.collector.total_samples * 100)
                if self.collector.total_samples > 0
                else 0
            )

            filename, lineno, funcname = func[0], func[1], func[2]
            samples_str = f"{direct_calls}/{cumulative_calls}"
            col = 0

            # Samples column
            self.add_str(line, col, f"{samples_str:>13}", color_samples)
            col += 15

            # Sample % column
            if show_sample_pct:
                self.add_str(line, col, f"{sample_pct:>5.1f}")
                col += 7

            # Total time column
            if show_tottime:
                self.add_str(line, col, f"{total_time:>10.3f}")
                col += 12

            # Cumul % column
            if show_cumul_pct:
                self.add_str(line, col, f"{cum_pct:>5.1f}")
                col += 7

            # Cumul time column
            if show_cumtime:
                self.add_str(line, col, f"{cumulative_time:>10.3f}")
                col += 12

            # Function name column
            if col < width - 15:
                remaining_space = width - col - 1
                func_width = min(
                    MAX_FUNC_NAME_WIDTH,
                    max(MIN_FUNC_NAME_WIDTH, remaining_space // 2),
                )

                func_display = funcname
                if len(funcname) > func_width:
                    func_display = funcname[: func_width - 3] + "..."
                func_display = f"{func_display:<{func_width}}"
                self.add_str(line, col, func_display, color_func)
                col += func_width + 2

                # File:line column
                if col < width - 10:
                    simplified_path = self.collector._simplify_path(filename)
                    file_line = f"{simplified_path}:{lineno}"
                    remaining_width = width - col - 1
                    self.add_str(
                        line, col, file_line[:remaining_width], color_file
                    )

            line += 1

        return line


class FooterWidget(Widget):
    """Widget for rendering the footer section (legend and controls)."""

    def __init__(self, display, colors, collector):
        """
        Initialize footer widget.

        Args:
            display: DisplayInterface implementation
            colors: Dictionary of color attributes
            collector: Reference to LiveStatsCollector for accessing state
        """
        super().__init__(display, colors)
        self.collector = collector

    def render(self, line, width, **kwargs):
        """
        Render the footer at the specified position.

        Args:
            line: Starting line number (should be height - 2)
            width: Available width

        Returns:
            Next available line number
        """
        A_DIM = self.display.get_attr("A_DIM")
        A_BOLD = self.display.get_attr("A_BOLD")

        # Legend line
        legend = "nsamples: direct/cumulative (direct=executing, cumulative=on stack)"
        self.add_str(line, 0, legend[: width - 1], A_DIM)
        line += 1

        # Controls line with status
        sort_names = {
            "tottime": "Total Time",
            "nsamples": "Direct Samples",
            "cumtime": "Cumulative Time",
            "sample_pct": "Sample %",
            "cumul_pct": "Cumulative %",
        }
        sort_display = sort_names.get(
            self.collector.sort_by, self.collector.sort_by
        )

        # Build status indicators
        status = []
        if self.collector.finished:
            status.append("[PROFILING FINISHED - Press 'q' to quit]")
        elif self.collector.paused:
            status.append("[PAUSED]")
        if self.collector.filter_pattern:
            status.append(f"[Filter: {self.collector.filter_pattern}]")
        status_str = " ".join(status) + " " if status else ""

        if self.collector.finished:
            footer = f"{status_str}"
        else:
            footer = f"{status_str}Sort: {sort_display} | 'h':help 'q':quit"
        self.add_str(
            line,
            0,
            footer[: width - 1],
            A_BOLD
            if (self.collector.paused or self.collector.finished)
            else A_DIM,
        )

        return line + 1

    def render_filter_input_prompt(self, line, width):
        """Draw the filter input prompt at the bottom of the screen."""
        A_BOLD = self.display.get_attr("A_BOLD")
        A_REVERSE = self.display.get_attr("A_REVERSE")

        # Draw prompt on last line
        prompt = f"Function filter: {self.collector.filter_input_buffer}_"
        self.add_str(line, 0, prompt[: width - 1], A_REVERSE | A_BOLD)


class HelpWidget(Widget):
    """Widget for rendering the help screen overlay."""

    def render(self, line, width, **kwargs):
        """
        Render the help screen.

        Args:
            line: Starting line number (ignored, help is centered)
            width: Available width
            kwargs: Must contain 'height' key

        Returns:
            Next available line number (not used for overlays)
        """
        height = kwargs["height"]
        A_BOLD = self.display.get_attr("A_BOLD")
        A_NORMAL = self.display.get_attr("A_NORMAL")

        help_lines = [
            ("Tachyon Profiler - Interactive Commands", A_BOLD),
            ("", A_NORMAL),
            ("Navigation & Display:", A_BOLD),
            ("  s           - Cycle through sort modes", A_NORMAL),
            ("  +           - Faster refresh rate", A_NORMAL),
            ("  -           - Slower refresh rate", A_NORMAL),
            ("", A_NORMAL),
            ("Control:", A_BOLD),
            ("  p           - Freeze display (snapshot)", A_NORMAL),
            ("  r           - Reset all statistics", A_NORMAL),
            ("", A_NORMAL),
            ("Filtering:", A_BOLD),
            ("  /           - Enter function filter (substring)", A_NORMAL),
            ("  c           - Clear filter", A_NORMAL),
            ("  ESC         - Cancel filter input", A_NORMAL),
            ("", A_NORMAL),
            ("Other:", A_BOLD),
            ("  h or ?      - Show/hide this help", A_NORMAL),
            ("  q           - Quit profiler", A_NORMAL),
            ("", A_NORMAL),
            ("Press any key to close this help screen", A_BOLD),
        ]

        start_line = (height - len(help_lines)) // 2
        for i, (text, attr) in enumerate(help_lines):
            if start_line + i < height - 1:
                col = 2  # Left-align with small margin
                self.add_str(start_line + i, col, text[: width - 3], attr)

        return line  # Not used for overlays


class DisplayInterface(ABC):
    """Abstract interface for display operations to enable testing."""

    @abstractmethod
    def get_dimensions(self):
        """Get terminal dimensions as (height, width)."""
        pass

    @abstractmethod
    def clear(self):
        """Clear the screen."""
        pass

    @abstractmethod
    def refresh(self):
        """Refresh the screen to show changes."""
        pass

    @abstractmethod
    def redraw(self):
        """Redraw the entire window."""
        pass

    @abstractmethod
    def add_str(self, line, col, text, attr=0):
        """Add a string at the specified position."""
        pass

    @abstractmethod
    def get_input(self):
        """Get a character from input (non-blocking). Returns -1 if no input."""
        pass

    @abstractmethod
    def set_nodelay(self, flag):
        """Set non-blocking mode for input."""
        pass

    @abstractmethod
    def has_colors(self):
        """Check if terminal supports colors."""
        pass

    @abstractmethod
    def init_color_pair(self, pair_id, fg, bg):
        """Initialize a color pair."""
        pass

    @abstractmethod
    def get_color_pair(self, pair_id):
        """Get a color pair attribute."""
        pass

    @abstractmethod
    def get_attr(self, name):
        """Get a display attribute by name (e.g., 'A_BOLD', 'A_REVERSE')."""
        pass


class CursesDisplay(DisplayInterface):
    """Real curses display implementation."""

    def __init__(self, stdscr):
        self.stdscr = stdscr

    def get_dimensions(self):
        return self.stdscr.getmaxyx()

    def clear(self):
        self.stdscr.clear()

    def refresh(self):
        self.stdscr.refresh()

    def redraw(self):
        self.stdscr.redrawwin()

    def add_str(self, line, col, text, attr=0):
        try:
            height, width = self.get_dimensions()
            if 0 <= line < height and 0 <= col < width:
                max_len = width - col - 1
                if len(text) > max_len:
                    text = text[:max_len]
                self.stdscr.addstr(line, col, text, attr)
        except curses.error:
            pass

    def get_input(self):
        try:
            return self.stdscr.getch()
        except (KeyError, curses.error):
            return -1

    def set_nodelay(self, flag):
        self.stdscr.nodelay(flag)

    def has_colors(self):
        return curses.has_colors()

    def init_color_pair(self, pair_id, fg, bg):
        try:
            curses.init_pair(pair_id, fg, bg)
        except curses.error:
            pass

    def get_color_pair(self, pair_id):
        return curses.color_pair(pair_id)

    def get_attr(self, name):
        return getattr(curses, name, 0)


class MockDisplay(DisplayInterface):
    """Mock display for testing."""

    def __init__(self, height=40, width=160):
        self.height = height
        self.width = width
        self.buffer = {}
        self.cleared = False
        self.refreshed = False
        self.redrawn = False
        self.input_queue = []
        self.nodelay_flag = True
        self.colors_supported = True
        self.color_pairs = {}

    def get_dimensions(self):
        return (self.height, self.width)

    def clear(self):
        self.buffer.clear()
        self.cleared = True

    def refresh(self):
        self.refreshed = True

    def redraw(self):
        self.redrawn = True

    def add_str(self, line, col, text, attr=0):
        if 0 <= line < self.height and 0 <= col < self.width:
            max_len = self.width - col - 1
            if len(text) > max_len:
                text = text[:max_len]
            self.buffer[(line, col)] = (text, attr)

    def get_input(self):
        if self.input_queue:
            return self.input_queue.pop(0)
        return -1

    def set_nodelay(self, flag):
        self.nodelay_flag = flag

    def has_colors(self):
        return self.colors_supported

    def init_color_pair(self, pair_id, fg, bg):
        self.color_pairs[pair_id] = (fg, bg)

    def get_color_pair(self, pair_id):
        return pair_id << 8

    def get_attr(self, name):
        attrs = {
            "A_NORMAL": 0,
            "A_BOLD": 1 << 16,
            "A_REVERSE": 1 << 17,
            "A_UNDERLINE": 1 << 18,
            "A_DIM": 1 << 19,
        }
        return attrs.get(name, 0)

    def simulate_input(self, char):
        """Helper method for tests to simulate keyboard input."""
        self.input_queue.append(char)

    def get_text_at(self, line, col):
        """Helper method for tests to inspect buffer content."""
        if (line, col) in self.buffer:
            return self.buffer[(line, col)][0]
        return None

    def get_all_lines(self):
        """Get all display content as a list of lines (for testing)."""
        if not self.buffer:
            return []

        max_line = max(pos[0] for pos in self.buffer.keys())
        lines = []
        for line_num in range(max_line + 1):
            line_parts = []
            for col in range(self.width):
                if (line_num, col) in self.buffer:
                    text, _ = self.buffer[(line_num, col)]
                    line_parts.append((col, text))

            # Reconstruct line from parts
            if line_parts:
                line_parts.sort(key=lambda x: x[0])
                line = ""
                last_col = 0
                for col, text in line_parts:
                    if col > last_col:
                        line += " " * (col - last_col)
                    line += text
                    last_col = col + len(text)
                lines.append(line.rstrip())
            else:
                lines.append("")

        # Remove trailing empty lines
        while lines and not lines[-1]:
            lines.pop()

        return lines

    def find_text(self, pattern):
        """Find text matching pattern in buffer (for testing). Returns (line, col) or None."""
        for (line, col), (text, _) in self.buffer.items():
            if pattern in text:
                return (line, col)
        return None

    def contains_text(self, text):
        """Check if display contains the given text anywhere (for testing)."""
        return self.find_text(text) is not None


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
        self._saved_stdout = None
        self._saved_stderr = None
        self._devnull = None
        self._last_display_update = None
        self._max_sample_rate = 0  # Track maximum sample rate seen
        self._successful_samples = 0  # Track samples that captured frames
        self._failed_samples = 0  # Track samples that failed to capture frames

        # Interactive controls state
        self.paused = False  # Pause UI updates (profiling continues)
        self.show_help = False  # Show help screen
        self.filter_pattern = None  # Glob pattern to filter functions
        self.filter_input_mode = False  # Currently entering filter text
        self.filter_input_buffer = ""  # Buffer for filter input
        self.finished = False  # Program has finished, showing final state

        # Calculate common path prefixes to strip
        self._path_prefixes = self._get_common_path_prefixes()

        # Widgets (initialized when display is available)
        self._header_widget = None
        self._table_widget = None
        self._footer_widget = None
        self._help_widget = None

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

    def _simplify_path(self, filepath):
        """Simplify a file path by removing common prefixes."""
        # Try to match against known prefixes
        for prefix_path in self._path_prefixes:
            if filepath.startswith(prefix_path):
                # Remove the prefix completely
                relative = filepath[len(prefix_path) :].lstrip(os.sep)
                return relative

        # If no match, return the original path
        return filepath

    def _process_frames(self, frames):
        """Process a single thread's frame stack."""
        if not frames:
            return

        # Process each frame in the stack to track cumulative calls
        for frame in frames:
            location = (frame.filename, frame.lineno, frame.funcname)
            self.result[location]["cumulative_calls"] += 1

        # The top frame gets counted as an inline call (directly executing)
        top_location = (
            frames[0].filename,
            frames[0].lineno,
            frames[0].funcname,
        )
        self.result[top_location]["direct_calls"] += 1

    def collect(self, stack_frames):
        """Collect and display profiling data."""
        if self.start_time is None:
            self.start_time = time.perf_counter()
            self._last_display_update = self.start_time

        # Always collect data, even when paused
        # Track if we got any frames this sample
        got_frames = False
        for frames, thread_id in self._iter_all_frames(
            stack_frames, skip_idle=self.skip_idle
        ):
            self._process_frames(frames)
            if frames:
                got_frames = True

        if got_frames:
            self._successful_samples += 1
        else:
            self._failed_samples += 1

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
                >= DISPLAY_UPDATE_INTERVAL
            ):
                self._update_display()
                self._last_display_update = current_time

    def _prepare_display_data(self, height):
        """Prepare data for display rendering."""
        elapsed = (
            time.perf_counter() - self.start_time if self.start_time else 0
        )
        stats_list = self._build_stats_list()

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
        if self._header_widget is None:
            self._header_widget = HeaderWidget(self.display, colors, self)
            self._table_widget = TableWidget(self.display, colors, self)
            self._footer_widget = FooterWidget(self.display, colors, self)
            self._help_widget = HelpWidget(self.display, colors)

    def _render_display_sections(
        self, height, width, elapsed, stats_list, colors
    ):
        """Render all display sections to the screen."""
        line = 0
        try:
            # Initialize widgets if not already done
            self._initialize_widgets(colors)

            # Render header
            line = self._header_widget.render(
                line, width, elapsed=elapsed, stats_list=stats_list
            )

            # Render table
            line = self._table_widget.render(
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

            # Show help screen if requested
            if self.show_help:
                colors = self._setup_colors()
                self._initialize_widgets(colors)
                self._help_widget.render(0, width, height=height)
                self.display.refresh()
                return

            # Prepare data
            elapsed, stats_list = self._prepare_display_data(height)

            # Setup colors
            colors = self._setup_colors()

            # Initialize widgets if needed
            self._initialize_widgets(colors)

            # Render all sections
            self._render_display_sections(
                height, width, elapsed, stats_list, colors
            )

            # Footer
            self._footer_widget.render(height - 2, width)

            # Show filter input prompt if in filter input mode
            if self.filter_input_mode:
                self._footer_widget.render_filter_input_prompt(
                    height - 1, width
                )

            # Refresh display
            self.display.redraw()
            self.display.refresh()

        except Exception:
            pass

    def _cycle_sort(self):
        """Cycle through different sort modes in column order (left to right)."""
        sort_modes = [
            "nsamples",
            "sample_pct",
            "tottime",
            "cumul_pct",
            "cumtime",
        ]
        try:
            current_idx = sort_modes.index(self.sort_by)
            self.sort_by = sort_modes[(current_idx + 1) % len(sort_modes)]
        except ValueError:
            self.sort_by = "nsamples"

    def _setup_colors(self):
        """Set up color pairs and return color attributes."""
        A_BOLD = self.display.get_attr("A_BOLD")
        A_REVERSE = self.display.get_attr("A_REVERSE")
        A_UNDERLINE = self.display.get_attr("A_UNDERLINE")
        A_NORMAL = self.display.get_attr("A_NORMAL")

        if self.display.has_colors():
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
        }

    def _build_stats_list(self):
        """Build and sort the statistics list."""
        stats_list = []
        for func, call_counts in self.result.items():
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

            direct_calls = call_counts["direct_calls"]
            cumulative_calls = call_counts["cumulative_calls"]
            total_time = direct_calls * self.sample_interval_sec
            cumulative_time = cumulative_calls * self.sample_interval_sec

            stats_list.append(
                {
                    "func": func,
                    "direct_calls": direct_calls,
                    "cumulative_calls": cumulative_calls,
                    "total_time": total_time,
                    "cumulative_time": cumulative_time,
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
        self.total_samples = 0
        self._successful_samples = 0
        self._failed_samples = 0
        self._max_sample_rate = 0
        self.start_time = time.perf_counter()
        self._last_display_update = self.start_time

    def mark_finished(self):
        """Mark the profiling session as finished."""
        self.finished = True
        # Force a final display update to show the finished message
        if self.display is not None:
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
        global DISPLAY_UPDATE_INTERVAL

        self.display.set_nodelay(True)
        ch = self.display.get_input()

        # If showing help, any key closes it
        if self.show_help and ch != -1:
            self.show_help = False
            return

        # Handle filter input mode
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
            return

        # If finished, only allow 'q' to quit
        if self.finished:
            if ch == ord("q") or ch == ord("Q"):
                self.running = False
            return

        # Handle regular commands
        if ch == ord("q") or ch == ord("Q"):
            self.running = False

        elif ch == ord("s") or ch == ord("S"):
            self._cycle_sort()

        elif ch == ord("h") or ch == ord("H") or ch == ord("?"):
            self.show_help = not self.show_help

        elif ch == ord("p") or ch == ord("P"):
            self.paused = not self.paused

        elif ch == ord("r") or ch == ord("R"):
            self.reset_stats()

        elif ch == ord("+") or ch == ord("="):
            # Decrease update interval (faster refresh)
            new_interval = max(
                0.05, DISPLAY_UPDATE_INTERVAL - 0.05
            )  # Min 20Hz
            DISPLAY_UPDATE_INTERVAL = new_interval

        elif ch == ord("-") or ch == ord("_"):
            # Increase update interval (slower refresh)
            new_interval = min(1.0, DISPLAY_UPDATE_INTERVAL + 0.05)  # Max 1Hz
            DISPLAY_UPDATE_INTERVAL = new_interval

        elif ch == ord("c") or ch == ord("C"):
            if self.filter_pattern:
                self.filter_pattern = None

        elif ch == ord("/"):
            self.filter_input_mode = True
            self.filter_input_buffer = self.filter_pattern or ""

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
