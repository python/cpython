from abc import ABC, abstractmethod


class Collector(ABC):
    @abstractmethod
    def collect(self, stack_frames):
        """Collect profiling data from stack frames."""

    @abstractmethod
    def export(self, filename):
        """Export collected data to a file."""

    def _iter_all_frames(self, stack_frames):
        """Iterate over all frame stacks from all interpreters and threads."""
        for interpreter_info in stack_frames:
            for thread_info in interpreter_info.threads:
                frames = thread_info.frame_info
                if frames:
                    yield frames
