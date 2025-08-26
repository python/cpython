"""Statistical sampling profiler for Python.

This module provides low-overhead profiling by periodically sampling the
call stack rather than tracing every function call.
"""

from profiling.sampling.collector import Collector
from profiling.sampling.pstats_collector import PstatsCollector
from profiling.sampling.stack_collector import CollapsedStackCollector

__all__ = ("Collector", "PstatsCollector", "CollapsedStackCollector")
