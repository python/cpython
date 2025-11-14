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
        # Phase 1: Index tasks and build parent relationships
        task_map, child_to_parents, all_task_ids = self._build_task_graph(awaited_info_list)

        # Phase 2: Find leaf tasks (tasks not awaited by anyone)
        leaf_task_ids = self._find_leaf_tasks(child_to_parents, all_task_ids)

        # Phase 3: Build linear stacks via BFS from each leaf to root
        yield from self._build_linear_stacks(leaf_task_ids, task_map, child_to_parents)

    def _build_task_graph(self, awaited_info_list):
        task_map = {}
        child_to_parents = {}
        all_task_ids = set()

        for awaited_info in awaited_info_list:
            thread_id = awaited_info.thread_id
            for task_info in awaited_info.awaited_by:
                task_id = task_info.task_id
                task_map[task_id] = (task_info, thread_id)
                all_task_ids.add(task_id)

                # Store parent task IDs (not frames - those are in task_info.coroutine_stack)
                if task_info.awaited_by:
                    child_to_parents[task_id] = [p.task_name for p in task_info.awaited_by]

        return task_map, child_to_parents, all_task_ids

    def _find_leaf_tasks(self, child_to_parents, all_task_ids):
        all_parent_ids = set()
        for parent_ids in child_to_parents.values():
            all_parent_ids.update(parent_ids)
        return all_task_ids - all_parent_ids

    def _build_linear_stacks(self, leaf_task_ids, task_map, child_to_parents):
        for leaf_id in leaf_task_ids:
            # BFS queue: (current_task_id, frames_so_far, path_for_cycle_detection)
            queue = [(leaf_id, [], frozenset())]

            while queue:
                current_id, frames, path = queue.pop(0)

                # Cycle detection
                if current_id in path:
                    continue

                # End of path (parent ID not in task_map)
                if current_id not in task_map:
                    if frames:
                        _, thread_id = task_map[leaf_id]
                        yield frames, thread_id, leaf_id
                    continue

                # Process current task
                task_info, tid = task_map[current_id]
                new_frames = list(frames)
                new_path = path | {current_id}

                # Add all frames from all coroutines in this task
                if task_info.coroutine_stack:
                    for coro_info in task_info.coroutine_stack:
                        for frame in coro_info.call_stack:
                            new_frames.append(frame)

                # Add task boundary marker
                task_name = task_info.task_name or "Task-" + str(task_info.task_id)
                new_frames.append(FrameInfo(("<task>", 0, task_name)))

                # Get parent task IDs
                parent_ids = child_to_parents.get(current_id, [])

                if not parent_ids:
                    # Root task - yield complete stack
                    yield new_frames, tid, leaf_id
                else:
                    # Continue to each parent (creates multiple paths if >1 parent)
                    for parent_id in parent_ids:
                        queue.append((parent_id, new_frames, new_path))
