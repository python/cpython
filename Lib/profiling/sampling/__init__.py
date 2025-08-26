"""Statistical sampling profiler for Python.

This module provides low-overhead profiling by periodically sampling the
call stack rather than tracing every function call.
"""

from profiling.collector import Collector
from profiling.pstats_collector import PstatsCollector
from profiling.stack_collector import CollapsedStackCollector

__all__ = ("Collector", "PstatsCollector", "CollapsedStackCollector")
