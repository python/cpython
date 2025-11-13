from abc import ABC, abstractmethod

from _remote_debugging import FrameInfo


# Enums are slow
THREAD_STATE_RUNNING = 0
THREAD_STATE_IDLE = 1
THREAD_STATE_GIL_WAIT = 2
THREAD_STATE_UNKNOWN = 3

STATUS = {
    THREAD_STATE_RUNNING: "running",
    THREAD_STATE_IDLE: "idle",
    THREAD_STATE_GIL_WAIT: "gil_wait",
    THREAD_STATE_UNKNOWN: "unknown",
}

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
                if skip_idle and thread_info.status != THREAD_STATE_RUNNING:
                    continue
                frames = thread_info.frame_info
                if frames:
                    yield frames, thread_info.thread_id

    def _iter_async_frames(self, awaited_info_list):
        all_tasks = {}

        for awaited_info in awaited_info_list:
            thread_id = awaited_info.thread_id
            for task_info in awaited_info.awaited_by:
                all_tasks[task_info.task_id] = (task_info, thread_id)

        # For each task, reconstruct the full call stack by following coroutine chains
        for task_id, (task_info, thread_id) in all_tasks.items():
            frames = [
                frame
                for coro in task_info.coroutine_stack
                for frame in coro.call_stack
            ]

            task_name = task_info.task_name or f"Task-{task_id}"
            synthetic_frame = FrameInfo(("<task>", 0, task_name))
            frames.append(synthetic_frame)

            current_parents = task_info.awaited_by
            visited = set()

            while current_parents:
                next_parents = []
                for parent_coro in current_parents:
                    frames.extend(parent_coro.call_stack)
                    parent_task_id = parent_coro.task_name

                    if parent_task_id in visited or parent_task_id not in all_tasks:
                        continue
                    visited.add(parent_task_id)

                    parent_task_info, _ = all_tasks[parent_task_id]

                    parent_name = parent_task_info.task_name or f"Task-{parent_task_id}"
                    synthetic_parent_frame = FrameInfo(("<task>", 0, parent_name))
                    frames.append(synthetic_parent_frame)

                    if parent_task_info.awaited_by:
                        next_parents.extend(parent_task_info.awaited_by)
                current_parents = next_parents
            yield frames, thread_id, task_id
