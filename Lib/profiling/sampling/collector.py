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
        status_counts = {
            "has_gil": 0,
            "on_cpu": 0,
            "gil_requested": 0,
            "unknown": 0,
            "total": 0,
        }
        has_gc_frame = False

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

                # Check for GC frames
                frames = getattr(thread_info, "frame_info", None)
                if frames and not has_gc_frame:
                    for frame in frames:
                        if self._is_gc_frame(frame):
                            has_gc_frame = True
                            break

        return status_counts, has_gc_frame
