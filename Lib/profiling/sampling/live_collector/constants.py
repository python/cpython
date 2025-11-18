"""Constants for the live profiling collector."""

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
HEADER_LINES = 10  # Increased to include thread status line
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
DEFAULT_SORT_BY = "nsamples"  # Number of samples in leaf (self time)
DEFAULT_DISPLAY_LIMIT = 20
