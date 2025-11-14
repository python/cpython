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
        """Iterate over linear stacks for all leaf tasks (hot path optimized)."""
        # Build adjacency graph (O(n))
        task_map = {}
        child_to_parent = {}
        all_task_ids = set()

        for awaited_info in awaited_info_list:
            thread_id = awaited_info.thread_id
            for task_info in awaited_info.awaited_by:
                task_id = task_info.task_id
                task_map[task_id] = (task_info, thread_id)
                all_task_ids.add(task_id)
                if task_info.awaited_by:
                    child_to_parent[task_id] = task_info.awaited_by[0].task_name

        # Identify leaf tasks (O(n))
        leaf_task_ids = all_task_ids - set(child_to_parent.values())

        # Build linear stacks for each leaf (O(n × depth))
        for leaf_id in leaf_task_ids:
            frames = []
            current_id = leaf_id
            thread_id = None

            while current_id in task_map:
                task_info, tid = task_map[current_id]
                if thread_id is None:
                    thread_id = tid

                # Add frames from coroutine stack
                if task_info.coroutine_stack:
                    for frame in task_info.coroutine_stack[0].call_stack:
                        frames.append(frame)

                # Add task marker
                task_name = task_info.task_name or "Task-" + str(task_info.task_id)
                frames.append(FrameInfo(("<task>", 0, task_name)))

                # Move to parent
                current_id = child_to_parent.get(current_id)

            yield frames, thread_id, leaf_id
