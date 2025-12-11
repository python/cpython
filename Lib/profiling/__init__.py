"""Python profiling tools.

This package provides two types of profilers:

- profiling.tracing: Deterministic tracing profiler that instruments every
  function call and return. Higher overhead but provides exact call counts
  and timing.

- profiling.sampling: Statistical sampling profiler that periodically samples
  the call stack. Low overhead and suitable for production use.
"""

__all__ = ("tracing", "sampling")
