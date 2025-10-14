"""Compatibility wrapper for cProfile module.

This module maintains backward compatibility by importing from the new
profiling.tracing module.
"""

from profiling.tracing import run, runctx, Profile

__all__ = ["run", "runctx", "Profile"]

if __name__ == "__main__":
    import sys
    from profiling.tracing.__main__ import main
    main()
