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
                │ │  • Thread status & GC stats     │ │
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

# Re-export all public classes and constants for backward compatibility
from .collector import LiveStatsCollector
from .display import DisplayInterface, CursesDisplay, MockDisplay
from .widgets import (
    Widget,
    ProgressBarWidget,
    HeaderWidget,
    TableWidget,
    FooterWidget,
    HelpWidget,
)
from .constants import (
    MICROSECONDS_PER_SECOND,
    DISPLAY_UPDATE_HZ,
    DISPLAY_UPDATE_INTERVAL,
    MIN_TERMINAL_WIDTH,
    MIN_TERMINAL_HEIGHT,
    WIDTH_THRESHOLD_SAMPLE_PCT,
    WIDTH_THRESHOLD_TOTTIME,
    WIDTH_THRESHOLD_CUMUL_PCT,
    WIDTH_THRESHOLD_CUMTIME,
    HEADER_LINES,
    FOOTER_LINES,
    SAFETY_MARGIN,
    TOP_FUNCTIONS_DISPLAY_COUNT,
    COL_WIDTH_NSAMPLES,
    COL_SPACING,
    COL_WIDTH_SAMPLE_PCT,
    COL_WIDTH_TIME,
    MIN_FUNC_NAME_WIDTH,
    MAX_FUNC_NAME_WIDTH,
    MIN_AVAILABLE_SPACE,
    MIN_BAR_WIDTH,
    MAX_SAMPLE_RATE_BAR_WIDTH,
    MAX_EFFICIENCY_BAR_WIDTH,
    MIN_SAMPLE_RATE_FOR_SCALING,
    FINISHED_BANNER_EXTRA_LINES,
    COLOR_PAIR_HEADER_BG,
    COLOR_PAIR_CYAN,
    COLOR_PAIR_YELLOW,
    COLOR_PAIR_GREEN,
    COLOR_PAIR_MAGENTA,
    COLOR_PAIR_RED,
    COLOR_PAIR_SORTED_HEADER,
    DEFAULT_SORT_BY,
    DEFAULT_DISPLAY_LIMIT,
)

__all__ = [
    # Main collector
    "LiveStatsCollector",
    # Display interfaces
    "DisplayInterface",
    "CursesDisplay",
    "MockDisplay",
    # Widgets
    "Widget",
    "ProgressBarWidget",
    "HeaderWidget",
    "TableWidget",
    "FooterWidget",
    "HelpWidget",
    # Constants
    "MICROSECONDS_PER_SECOND",
    "DISPLAY_UPDATE_HZ",
    "DISPLAY_UPDATE_INTERVAL",
    "MIN_TERMINAL_WIDTH",
    "MIN_TERMINAL_HEIGHT",
    "WIDTH_THRESHOLD_SAMPLE_PCT",
    "WIDTH_THRESHOLD_TOTTIME",
    "WIDTH_THRESHOLD_CUMUL_PCT",
    "WIDTH_THRESHOLD_CUMTIME",
    "HEADER_LINES",
    "FOOTER_LINES",
    "SAFETY_MARGIN",
    "TOP_FUNCTIONS_DISPLAY_COUNT",
    "COL_WIDTH_NSAMPLES",
    "COL_SPACING",
    "COL_WIDTH_SAMPLE_PCT",
    "COL_WIDTH_TIME",
    "MIN_FUNC_NAME_WIDTH",
    "MAX_FUNC_NAME_WIDTH",
    "MIN_AVAILABLE_SPACE",
    "MIN_BAR_WIDTH",
    "MAX_SAMPLE_RATE_BAR_WIDTH",
    "MAX_EFFICIENCY_BAR_WIDTH",
    "MIN_SAMPLE_RATE_FOR_SCALING",
    "FINISHED_BANNER_EXTRA_LINES",
    "COLOR_PAIR_HEADER_BG",
    "COLOR_PAIR_CYAN",
    "COLOR_PAIR_YELLOW",
    "COLOR_PAIR_GREEN",
    "COLOR_PAIR_MAGENTA",
    "COLOR_PAIR_RED",
    "COLOR_PAIR_SORTED_HEADER",
    "DEFAULT_SORT_BY",
    "DEFAULT_DISPLAY_LIMIT",
]
