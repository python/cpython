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
        """
        # First, index all tasks by their IDs so we can look up parents easily
        all_tasks = {}
        tasks_by_name = {}
        for awaited_info in awaited_info_list:
            for task_info in awaited_info.awaited_by:
                all_tasks[task_info.task_id] = (task_info, awaited_info.thread_id)
                display_name = task_info.task_name or f"Task-{task_info.task_id}"
                tasks_by_name.setdefault(display_name, []).append(
                    (task_info, awaited_info.thread_id)
                )
                fallback_name = f"Task-{task_info.task_id}"
                if fallback_name != display_name:
                    tasks_by_name.setdefault(fallback_name, []).append(
                        (task_info, awaited_info.thread_id)
                    )

        # Use a cache for memoizing parent chains so we don't recompute them repeatedly
        cache = {}
        root_frame = FrameInfo(("<root>", 0, "<all tasks>"))

        def build_parent_chain(task_id, parent_name, thread_id, await_frames):
            """
            Recursively build the parent chain for a given task by:
            - Finding the parent's await-site frames
            - Recursing up the parent chain until reaching Program Root
            - Add Program Root at the top of the chain
            - Cache results along the way to avoid redundant work
            """
            def frame_signature(frame):
                func = getattr(frame, "function", None)
                if func is None:
                    func = getattr(frame, "funcname", None)
                return (
                    getattr(frame, "filename", None),
                    getattr(frame, "lineno", None),
                    func,
                )

            frames_signature = tuple(
                frame_signature(frame) for frame in await_frames or []
            )
            cache_key = (task_id, parent_name, thread_id, frames_signature)
            if cache_key in cache:
                return cache[cache_key]

            if not parent_name:
                chain = list(await_frames or []) + [root_frame]
                cache[cache_key] = chain
                return chain

            parent_entry = None
            for candidate_info, candidate_tid in tasks_by_name.get(parent_name, []):
                if candidate_tid == thread_id:
                    parent_entry = (candidate_info, candidate_tid)
                    break

            if parent_entry is None:
                chain = list(await_frames or []) + [root_frame]
                cache[cache_key] = chain
                return chain

            parent_info, parent_thread = parent_entry

            # Recursively build grandparent chain, or terminate with Program Root
            grandparent_chain = resolve_parent_chain(
                parent_info.task_id, parent_thread, parent_info.awaited_by
            )
            chain = list(await_frames or []) + grandparent_chain

            cache[cache_key] = chain
            return chain

        def resolve_parent_chain(task_id, thread_id, awaited_by_list):
            """Find the best available parent chain for the given task.
            Best means the longest chain (most frames) among all possible parents."""
            best_chain = [root_frame]
            for coro_info in awaited_by_list or []:
                parent_name = coro_info.task_name
                await_frames = list(coro_info.call_stack or [])
                candidate = build_parent_chain(
                    task_id,
                    parent_name,
                    thread_id,
                    await_frames,
                )
                if len(candidate) > len(best_chain):
                    best_chain = candidate
                if len(best_chain) > 1:
                    break
            return best_chain

        # Yield one complete stack per task in LEAF→ROOT order
        for task_id, (task_info, thread_id) in all_tasks.items():
            # Start with the task's own body frames (deepest frames first)
            body_frames = [
                frame
                for coro in (task_info.coroutine_stack or [])
                for frame in (coro.call_stack or [])
            ]

            if task_info.awaited_by:
                # Add synthetic frame for the task itself
                task_name = task_info.task_name or f"Task-{task_id}"
                synthetic = FrameInfo(("<task>", 0, f"running {task_name}"))

                # Append parent chain (await-site frames + parents recursively)
                parent_chain = resolve_parent_chain(
                    task_id, thread_id, task_info.awaited_by
                )
                yield body_frames + [synthetic] + parent_chain, task_id
            else:
                # Root task: no synthetic marker needed, just add root marker
                yield body_frames + [root_frame], task_id
