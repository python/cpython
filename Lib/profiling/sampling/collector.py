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
        """
        Iterate over all async frame stacks from awaited info.
        Yields one stack per task in LEAF→ROOT order: [task body, task marker, parent body, ..., Program Root]
        """
        # Index all tasks by ID
        all_tasks = {}
        for awaited_info in awaited_info_list:
            for task_info in awaited_info.awaited_by:
                all_tasks[task_info.task_id] = (task_info, awaited_info.thread_id)

        cache = {}  # Memoize parent chains

        def build_parent_chain(task_id, parent_id):
            """Build ancestor chain: [await-site frames, grandparent chain..., Program Root]"""
            if parent_id in cache:
                return cache[parent_id]

            if parent_id not in all_tasks:
                return []

            parent_info, _ = all_tasks[parent_id]

            # Find the await-site frames for this parent relationship
            await_frames = []
            for coro_info in all_tasks[task_id][0].awaited_by:
                if coro_info.task_name == parent_id:
                    await_frames = list(coro_info.call_stack or [])
                    break

            # Recursively build grandparent chain, or terminate with Program Root
            if (parent_info.awaited_by and parent_info.awaited_by[0].task_name and
                parent_info.awaited_by[0].task_name in all_tasks):
                grandparent_id = parent_info.awaited_by[0].task_name
                chain = await_frames + build_parent_chain(parent_id, grandparent_id)
            else:
                # Parent is root or grandparent not tracked
                root_frame = FrameInfo(("<thread>", 0, "Program Root"))
                chain = await_frames + [root_frame]

            cache[parent_id] = chain
            return chain

        # Find all parent task IDs (tasks that have children)
        parent_task_ids = {
            coro.task_name
            for task_info, _ in all_tasks.values()
            for coro in (task_info.awaited_by or [])
            if coro.task_name
        }

        # Yield one stack per leaf task (tasks that are not parents)
        for task_id, (task_info, thread_id) in all_tasks.items():
            # Skip parent tasks - they'll be included in their children's stacks
            if task_id in parent_task_ids:
                continue
            # Collect task's coroutine frames
            body_frames = [
                frame
                for coro in (task_info.coroutine_stack or [])
                for frame in (coro.call_stack or [])
            ]

            # Add synthetic task marker
            task_name = task_info.task_name or f"Task-{task_id}"
            synthetic = FrameInfo(("<task>", 0, f"running {task_name}"))

            # Build complete stack with parent chain
            if task_info.awaited_by and task_info.awaited_by[0].task_name:
                parent_id = task_info.awaited_by[0].task_name
                if parent_id in all_tasks:
                    parent_chain = build_parent_chain(task_id, parent_id)
                    yield body_frames + [synthetic] + parent_chain, thread_id, 0
                else:
                    # Parent not tracked, treat as root task
                    root = FrameInfo(("<thread>", 0, "Program Root"))
                    yield body_frames + [synthetic, root], thread_id, 0
            else:
                # Root task (no parents or empty awaited_by)
                root = FrameInfo(("<thread>", 0, "Program Root"))
                yield body_frames + [synthetic, root], thread_id, 0
