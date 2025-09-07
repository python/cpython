"""Statistical sampling profiler for Python.

This module provides low-overhead profiling by periodically sampling the
call stack rather than tracing every function call.
"""

from .collector import Collector
from .pstats_collector import PstatsCollector
from .stack_collector import CollapsedStackCollector
from .sample_profiler_context_manager import SampleProfilerContextManager

__all__ = ("Collector", "PstatsCollector", "CollapsedStackCollector", "SampleProfilerContextManager")
