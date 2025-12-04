from abc import ABC, abstractmethod
from .constants import (
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_ON_CPU,
    THREAD_STATUS_UNKNOWN,
    THREAD_STATUS_GIL_REQUESTED,
)

class Collector(ABC):
    @abstractmethod
    def collect(self, stack_frames):
        """Collect profiling data from stack frames."""

    def collect_failed_sample(self):
        """Collect data about a failed sample attempt."""

    @abstractmethod
    def export(self, filename):
        """Export collected data to a file."""

    def _iter_all_frames(self, stack_frames, skip_idle=False):
        for interpreter_info in stack_frames:
            for thread_info in interpreter_info.threads:
                # skip_idle now means: skip if thread is not actively running
                # A thread is "active" if it has the GIL OR is on CPU
                if skip_idle:
                    status_flags = thread_info.status
                    has_gil = bool(status_flags & THREAD_STATUS_HAS_GIL)
                    on_cpu = bool(status_flags & THREAD_STATUS_ON_CPU)
                    if not (has_gil or on_cpu):
                        continue
                frames = thread_info.frame_info
                if frames:
                    yield frames, thread_info.thread_id

    def _is_gc_frame(self, frame):
        if isinstance(frame, tuple):
            funcname = frame[2] if len(frame) >= 3 else ""
        else:
            funcname = getattr(frame, "funcname", "")

        return "<GC>" in funcname or "gc_collect" in funcname

    def _collect_thread_status_stats(self, stack_frames):
        """Collect aggregate and per-thread status statistics from a sample.

        Returns:
            tuple: (aggregate_status_counts, has_gc_frame, per_thread_stats)
                - aggregate_status_counts: dict with has_gil, on_cpu, etc.
                - has_gc_frame: bool indicating if any thread has GC frames
                - per_thread_stats: dict mapping thread_id to per-thread counts
        """
        status_counts = {
            "has_gil": 0,
            "on_cpu": 0,
            "gil_requested": 0,
            "unknown": 0,
            "total": 0,
        }
        has_gc_frame = False
        per_thread_stats = {}

        for interpreter_info in stack_frames:
            threads = getattr(interpreter_info, "threads", [])
            for thread_info in threads:
                status_counts["total"] += 1

                # Track thread status using bit flags
                status_flags = getattr(thread_info, "status", 0)

                if status_flags & THREAD_STATUS_HAS_GIL:
                    status_counts["has_gil"] += 1
                if status_flags & THREAD_STATUS_ON_CPU:
                    status_counts["on_cpu"] += 1
                if status_flags & THREAD_STATUS_GIL_REQUESTED:
                    status_counts["gil_requested"] += 1
                if status_flags & THREAD_STATUS_UNKNOWN:
                    status_counts["unknown"] += 1

                # Track per-thread statistics
                thread_id = getattr(thread_info, "thread_id", None)
                if thread_id is not None:
                    if thread_id not in per_thread_stats:
                        per_thread_stats[thread_id] = {
                            "has_gil": 0,
                            "on_cpu": 0,
                            "gil_requested": 0,
                            "unknown": 0,
                            "total": 0,
                            "gc_samples": 0,
                        }

                    thread_stats = per_thread_stats[thread_id]
                    thread_stats["total"] += 1

                    if status_flags & THREAD_STATUS_HAS_GIL:
                        thread_stats["has_gil"] += 1
                    if status_flags & THREAD_STATUS_ON_CPU:
                        thread_stats["on_cpu"] += 1
                    if status_flags & THREAD_STATUS_GIL_REQUESTED:
                        thread_stats["gil_requested"] += 1
                    if status_flags & THREAD_STATUS_UNKNOWN:
                        thread_stats["unknown"] += 1

                    # Check for GC frames in this thread
                    frames = getattr(thread_info, "frame_info", None)
                    if frames:
                        for frame in frames:
                            if self._is_gc_frame(frame):
                                thread_stats["gc_samples"] += 1
                                has_gc_frame = True
                                break

        return status_counts, has_gc_frame, per_thread_stats
