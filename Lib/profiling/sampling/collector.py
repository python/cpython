from abc import ABC, abstractmethod

# Thread status flags
try:
    from _remote_debugging import THREAD_STATUS_HAS_GIL, THREAD_STATUS_ON_CPU, THREAD_STATUS_UNKNOWN, THREAD_STATUS_GIL_REQUESTED
except ImportError:
    # Fallback for tests or when module is not available
    THREAD_STATUS_HAS_GIL = (1 << 0)
    THREAD_STATUS_ON_CPU = (1 << 1)
    THREAD_STATUS_UNKNOWN = (1 << 2)
    THREAD_STATUS_GIL_REQUESTED = (1 << 3)

class Collector(ABC):
    @abstractmethod
    def collect(self, stack_frames):
        """Collect profiling data from stack frames."""

    @abstractmethod
    def export(self, filename):
        """Export collected data to a file."""

    def _iter_all_frames(self, stack_frames, skip_idle=False):
        """Iterate over all frame stacks from all interpreters and threads."""
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
