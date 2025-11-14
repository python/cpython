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
        child_to_parents = {}
        all_task_ids = set()

        for awaited_info in awaited_info_list:
            thread_id = awaited_info.thread_id
            for task_info in awaited_info.awaited_by:
                task_id = task_info.task_id
                task_map[task_id] = (task_info, thread_id)
                all_task_ids.add(task_id)
                if task_info.awaited_by:
                    # Store all parent coroutines, not just [0]
                    child_to_parents[task_id] = task_info.awaited_by

        # Identify leaf tasks (O(n))
        # Collect all parent task IDs from all coroutines
        all_parent_ids = set()
        for parent_coros in child_to_parents.values():
            for parent_coro in parent_coros:
                all_parent_ids.add(parent_coro.task_name)
        leaf_task_ids = all_task_ids - all_parent_ids

        # Build linear stacks for each leaf (O(n × depth × num_paths))
        # For tasks with multiple parents, we generate one stack per parent path
        for leaf_id in leaf_task_ids:
            # Use BFS to explore all paths from leaf to root
            # Queue items: (current_task_id, frames_accumulated)
            queue = [(leaf_id, [])]
            visited = set()

            while queue:
                current_id, frames = queue.pop(0)

                # Avoid processing the same task twice in this path
                if current_id in visited:
                    continue
                visited.add(current_id)

                if current_id not in task_map:
                    # Reached end of path - yield if we have frames
                    if frames:
                        _, thread_id = task_map[leaf_id]
                        yield frames, thread_id, leaf_id
                    continue

                task_info, tid = task_map[current_id]

                # Add this task's frames
                new_frames = list(frames)
                if task_info.coroutine_stack:
                    for frame in task_info.coroutine_stack[0].call_stack:
                        new_frames.append(frame)

                # Add task boundary marker
                task_name = task_info.task_name or "Task-" + str(task_info.task_id)
                new_frames.append(FrameInfo(("<task>", 0, task_name)))

                # Get parent coroutines
                parent_coros = child_to_parents.get(current_id)
                if not parent_coros:
                    # No parents - this is the root, yield the complete stack
                    yield new_frames, tid, leaf_id
                    continue

                # For each parent coroutine, add its await frames and continue to parent task
                for parent_coro in parent_coros:
                    parent_task_id = parent_coro.task_name

                    # Add the parent's await-site frames (where parent awaits this task)
                    path_frames = list(new_frames)
                    for frame in parent_coro.call_stack:
                        path_frames.append(frame)

                    # Continue BFS with parent task
                    # Note: parent_coro.call_stack contains the frames from the parent task,
                    # so we should NOT add parent task's coroutine_stack again
                    queue.append((parent_task_id, path_frames))
