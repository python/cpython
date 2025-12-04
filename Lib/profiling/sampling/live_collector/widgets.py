"""Widget classes for the live profiling collector UI."""

import curses
import time
from abc import ABC, abstractmethod

from .constants import (
    TOP_FUNCTIONS_DISPLAY_COUNT,
    MIN_FUNC_NAME_WIDTH,
    MAX_FUNC_NAME_WIDTH,
    WIDTH_THRESHOLD_SAMPLE_PCT,
    WIDTH_THRESHOLD_TOTTIME,
    WIDTH_THRESHOLD_CUMUL_PCT,
    WIDTH_THRESHOLD_CUMTIME,
    MICROSECONDS_PER_SECOND,
    DISPLAY_UPDATE_INTERVAL,
    MIN_BAR_WIDTH,
    MAX_SAMPLE_RATE_BAR_WIDTH,
    MAX_EFFICIENCY_BAR_WIDTH,
    MIN_SAMPLE_RATE_FOR_SCALING,
    FOOTER_LINES,
    FINISHED_BANNER_EXTRA_LINES,
)
from ..constants import (
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_ON_CPU,
    THREAD_STATUS_UNKNOWN,
    THREAD_STATUS_GIL_REQUESTED,
    PROFILING_MODE_CPU,
    PROFILING_MODE_GIL,
    PROFILING_MODE_WALL,
)


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
        line = self.draw_thread_status(line, width)
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

        current_time = self.collector.current_time_display
        uptime = self.format_uptime(elapsed)

        # Calculate display refresh rate
        refresh_hz = (
            1.0 / self.collector.display_update_interval if self.collector.display_update_interval > 0 else 0
        )

        # Get current view mode and thread display
        if self.collector.view_mode == "ALL":
            thread_name = "ALL"
            thread_color = self.colors["green"]
        else:
            # PER_THREAD mode
            if self.collector.current_thread_index < len(
                self.collector.thread_ids
            ):
                thread_id = self.collector.thread_ids[
                    self.collector.current_thread_index
                ]
                num_threads = len(self.collector.thread_ids)
                thread_name = f"{thread_id} ({self.collector.current_thread_index + 1}/{num_threads})"
                thread_color = self.colors["magenta"]
            else:
                thread_name = "ALL"
                thread_color = self.colors["green"]

        header_parts = [
            ("PID: ", curses.A_BOLD),
            (f"{self.collector.pid}", self.colors["cyan"]),
            (" │ ", curses.A_DIM),
            ("Thread: ", curses.A_BOLD),
            (thread_name, thread_color),
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
        if sample_rate > self.collector.max_sample_rate:
            self.collector.max_sample_rate = sample_rate

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
        target_rate = (
            MICROSECONDS_PER_SECOND / self.collector.sample_interval_usec
        )

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
            self.collector.successful_samples
            / max(1, self.collector.total_samples)
        ) * 100
        failed_pct = (
            self.collector.failed_samples
            / max(1, self.collector.total_samples)
        ) * 100

        col = 0
        self.add_str(line, col, "Efficiency:", curses.A_BOLD)
        col += 11

        label = f" {success_pct:>5.2f}% good, {failed_pct:>4.2f}% failed"
        available_width = width - col - len(label) - 3

        if available_width >= MIN_BAR_WIDTH:
            bar_width = min(MAX_EFFICIENCY_BAR_WIDTH, available_width)
            success_fill = int(
                (
                    self.collector.successful_samples
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

    def _add_percentage_stat(
        self, line, col, value, label, color, add_separator=False
    ):
        """Add a percentage stat to the display.

        Args:
            line: Line number
            col: Starting column
            value: Percentage value
            label: Label text
            color: Color attribute
            add_separator: Whether to add separator before the stat

        Returns:
            Updated column position
        """
        if add_separator:
            self.add_str(line, col, " │ ", curses.A_DIM)
            col += 3

        self.add_str(line, col, f"{value:>4.1f}", color)
        col += 4
        self.add_str(line, col, f"% {label}", curses.A_NORMAL)
        col += len(label) + 2

        return col

    def draw_thread_status(self, line, width):
        """Draw thread status statistics and GC information."""
        # Get status counts for current view mode
        thread_data = self.collector._get_current_thread_data()
        status_counts = thread_data.as_status_dict() if thread_data else self.collector.thread_status_counts

        # Calculate percentages
        total_threads = max(1, status_counts["total"])
        pct_on_gil = (status_counts["has_gil"] / total_threads) * 100
        pct_off_gil = 100.0 - pct_on_gil
        pct_gil_requested = (status_counts["gil_requested"] / total_threads) * 100

        # Get GC percentage based on view mode
        if thread_data:
            total_samples = max(1, thread_data.sample_count)
            pct_gc = (thread_data.gc_frame_samples / total_samples) * 100
        else:
            total_samples = max(1, self.collector.total_samples)
            pct_gc = (self.collector.gc_frame_samples / total_samples) * 100

        col = 0
        self.add_str(line, col, "Threads:   ", curses.A_BOLD)
        col += 11

        # Show GIL stats only if mode is not GIL (GIL mode filters to only GIL holders)
        if self.collector.mode != PROFILING_MODE_GIL:
            col = self._add_percentage_stat(
                line, col, pct_on_gil, "on gil", self.colors["green"]
            )
            col = self._add_percentage_stat(
                line,
                col,
                pct_off_gil,
                "off gil",
                self.colors["red"],
                add_separator=True,
            )

        # Show "waiting for gil" only if mode is not GIL
        if self.collector.mode != PROFILING_MODE_GIL and col < width - 30:
            col = self._add_percentage_stat(
                line,
                col,
                pct_gil_requested,
                "waiting for gil",
                self.colors["yellow"],
                add_separator=True,
            )

        # Always show GC stats
        if col < width - 15:
            col = self._add_percentage_stat(
                line,
                col,
                pct_gc,
                "GC",
                self.colors["magenta"],
                add_separator=(col > 11),
            )

        return line + 1

    def draw_function_stats(self, line, width, stats_list):
        """Draw function statistics summary."""
        result_set = self.collector._get_current_result_source()
        total_funcs = len(result_set)
        funcs_shown = len(stats_list)
        executing_funcs = sum(
            1 for f in result_set.values() if f.get("direct_calls", 0) > 0
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

        # Get trend tracker for color decisions
        trend_tracker = self.collector._trend_tracker

        for stat in stats_list:
            if line >= height - FOOTER_LINES:
                break

            func = stat["func"]
            direct_calls = stat["direct_calls"]
            cumulative_calls = stat["cumulative_calls"]
            total_time = stat["total_time"]
            cumulative_time = stat["cumulative_time"]
            trends = stat.get("trends", {})

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

            # Helper function to get trend color for a specific column
            def get_trend_color(column_name):
                trend = trends.get(column_name, "stable")
                if trend_tracker is not None:
                    return trend_tracker.get_color(trend)
                return curses.A_NORMAL

            filename, lineno, funcname = func[0], func[1], func[2]
            samples_str = f"{direct_calls}/{cumulative_calls}"
            col = 0

            # Samples column - apply trend color based on nsamples trend
            nsamples_color = get_trend_color("nsamples")
            self.add_str(line, col, f"{samples_str:>13}", nsamples_color)
            col += 15

            # Sample % column
            if show_sample_pct:
                sample_pct_color = get_trend_color("sample_pct")
                self.add_str(line, col, f"{sample_pct:>5.1f}", sample_pct_color)
                col += 7

            # Total time column
            if show_tottime:
                tottime_color = get_trend_color("tottime")
                self.add_str(line, col, f"{total_time:>10.3f}", tottime_color)
                col += 12

            # Cumul % column
            if show_cumul_pct:
                cumul_pct_color = get_trend_color("cumul_pct")
                self.add_str(line, col, f"{cum_pct:>5.1f}", cumul_pct_color)
                col += 7

            # Cumul time column
            if show_cumtime:
                cumtime_color = get_trend_color("cumtime")
                self.add_str(line, col, f"{cumulative_time:>10.3f}", cumtime_color)
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
                    simplified_path = self.collector.simplify_path(filename)
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
            status.append(
                f"[Filter: {self.collector.filter_pattern} (c to clear)]"
            )
        # Show trend colors status if disabled
        if self.collector._trend_tracker is not None and not self.collector._trend_tracker.enabled:
            status.append("[Trend colors: OFF]")
        status_str = " ".join(status) + " " if status else ""

        if self.collector.finished:
            footer = f"{status_str}"
        else:
            footer = f"{status_str}Sort: {sort_display} | 't':mode 'x':trends ←→:thread 'h':help 'q':quit"
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
            ("  s           - Cycle through sort modes (forward)", A_NORMAL),
            ("  S           - Cycle through sort modes (backward)", A_NORMAL),
            ("  t           - Toggle view mode (ALL / per-thread)", A_NORMAL),
            ("  x           - Toggle trend colors (on/off)", A_NORMAL),
            ("  ← →  ↑ ↓   - Navigate threads (in per-thread mode)", A_NORMAL),
            ("  +           - Faster display refresh rate", A_NORMAL),
            ("  -           - Slower display refresh rate", A_NORMAL),
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
