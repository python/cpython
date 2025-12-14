"""Constants for the sampling profiler."""

# Profiling mode constants
PROFILING_MODE_WALL = 0
PROFILING_MODE_CPU = 1
PROFILING_MODE_GIL = 2
PROFILING_MODE_ALL = 3  # Combines GIL + CPU checks
PROFILING_MODE_EXCEPTION = 4  # Only samples when thread has an active exception

# Sort mode constants
SORT_MODE_NSAMPLES = 0
SORT_MODE_TOTTIME = 1
SORT_MODE_CUMTIME = 2
SORT_MODE_SAMPLE_PCT = 3
SORT_MODE_CUMUL_PCT = 4
SORT_MODE_NSAMPLES_CUMUL = 5

# Default location for synthetic frames (native, GC) that have no source location
# Format: (lineno, end_lineno, col_offset, end_col_offset)
DEFAULT_LOCATION = (0, 0, -1, -1)

# Thread status flags
try:
    from _remote_debugging import (
        THREAD_STATUS_HAS_GIL,
        THREAD_STATUS_ON_CPU,
        THREAD_STATUS_UNKNOWN,
        THREAD_STATUS_GIL_REQUESTED,
        THREAD_STATUS_HAS_EXCEPTION,
    )
except ImportError:
    # Fallback for tests or when module is not available
    THREAD_STATUS_HAS_GIL = (1 << 0)
    THREAD_STATUS_ON_CPU = (1 << 1)
    THREAD_STATUS_UNKNOWN = (1 << 2)
    THREAD_STATUS_GIL_REQUESTED = (1 << 3)
    THREAD_STATUS_HAS_EXCEPTION = (1 << 4)
