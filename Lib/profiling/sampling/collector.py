from abc import ABC, abstractmethod
from .constants import (
    DEFAULT_LOCATION,
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_ON_CPU,
    THREAD_STATUS_GIL_REQUESTED,
    THREAD_STATUS_UNKNOWN,
    THREAD_STATUS_HAS_EXCEPTION,
)

try:
    from _remote_debugging import FrameInfo
except ImportError:
    # Fallback definition if _remote_debugging is not available
    FrameInfo = None


def normalize_location(location):
    """Normalize location to a 4-tuple format.

    Args:
        location: tuple (lineno, end_lineno, col_offset, end_col_offset) or None

    Returns:
        tuple: (lineno, end_lineno, col_offset, end_col_offset)
    """
    if location is None:
        return DEFAULT_LOCATION
    return location


def extract_lineno(location):
    """Extract lineno from location.

    Args:
        location: tuple (lineno, end_lineno, col_offset, end_col_offset) or None

    Returns:
        int: The line number (0 for synthetic frames)
    """
    if location is None:
        return 0
    return location[0]

class Collector(ABC):
    @abstractmethod
    def collect(self, stack_frames, timestamps_us=None):
        """Collect profiling data from stack frames.

        Args:
            stack_frames: List of InterpreterInfo objects
            timestamps_us: Optional list of timestamps in microseconds. If provided
                (from binary replay with RLE batching), use these instead of current
                time. If None, collectors should use time.monotonic() or similar.
                The list may contain multiple timestamps when samples are batched
                together (same stack, different times).
        """

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

    def _iter_async_frames(self, awaited_info_list):
        # Phase 1: Index tasks and build parent relationships with pre-computed selection
        task_map, child_to_parent, all_task_ids, all_parent_ids = self._build_task_graph(awaited_info_list)

        # Phase 2: Find leaf tasks (tasks not awaited by anyone)
        leaf_task_ids = self._find_leaf_tasks(all_task_ids, all_parent_ids)

        # Phase 3: Build linear stacks from each leaf to root (optimized - no sorting!)
        yield from self._build_linear_stacks(leaf_task_ids, task_map, child_to_parent)

    def _iter_stacks(self, stack_frames, skip_idle=False):
        """Yield (frames, thread_id) for all stacks, handling both sync and async modes."""
        if stack_frames and hasattr(stack_frames[0], "awaited_by"):
            for frames, thread_id, _ in self._iter_async_frames(stack_frames):
                if frames:
                    yield frames, thread_id
        else:
            for frames, thread_id in self._iter_all_frames(stack_frames, skip_idle=skip_idle):
                if frames:
                    yield frames, thread_id

    def _build_task_graph(self, awaited_info_list):
        task_map = {}
        child_to_parent = {}  # Maps child_id -> (selected_parent_id, parent_count)
        all_task_ids = set()
        all_parent_ids = set()  # Track ALL parent IDs for leaf detection

        for awaited_info in awaited_info_list:
            thread_id = awaited_info.thread_id
            for task_info in awaited_info.awaited_by:
                task_id = task_info.task_id
                task_map[task_id] = (task_info, thread_id)
                all_task_ids.add(task_id)

                # Pre-compute selected parent and count for optimization
                if task_info.awaited_by:
                    parent_ids = [p.task_name for p in task_info.awaited_by]
                    parent_count = len(parent_ids)
                    # Track ALL parents for leaf detection
                    all_parent_ids.update(parent_ids)
                    # Use min() for O(n) instead of sorted()[0] which is O(n log n)
                    selected_parent = min(parent_ids) if parent_count > 1 else parent_ids[0]
                    child_to_parent[task_id] = (selected_parent, parent_count)

        return task_map, child_to_parent, all_task_ids, all_parent_ids

    def _find_leaf_tasks(self, all_task_ids, all_parent_ids):
        # Leaves are tasks that are not parents of any other task
        return all_task_ids - all_parent_ids

    def _build_linear_stacks(self, leaf_task_ids, task_map, child_to_parent):
        for leaf_id in leaf_task_ids:
            frames = []
            visited = set()
            current_id = leaf_id
            thread_id = None

            # Follow the single parent chain from leaf to root
            while current_id is not None:
                # Cycle detection
                if current_id in visited:
                    break
                visited.add(current_id)

                # Check if task exists in task_map
                if current_id not in task_map:
                    break

                task_info, tid = task_map[current_id]

                # Set thread_id from first task
                if thread_id is None:
                    thread_id = tid

                # Add all frames from all coroutines in this task
                if task_info.coroutine_stack:
                    for coro_info in task_info.coroutine_stack:
                        for frame in coro_info.call_stack:
                            frames.append(frame)

                # Get pre-computed parent info (no sorting needed!)
                parent_info = child_to_parent.get(current_id)

                # Add task boundary marker with parent count annotation if multiple parents
                task_name = task_info.task_name or "Task-" + str(task_info.task_id)
                if parent_info:
                    selected_parent, parent_count = parent_info
                    if parent_count > 1:
                        task_name = f"{task_name} ({parent_count} parents)"
                    frames.append(FrameInfo(("<task>", None, task_name, None)))
                    current_id = selected_parent
                else:
                    # Root task - no parent
                    frames.append(FrameInfo(("<task>", None, task_name, None)))
                    current_id = None

            # Yield the complete stack if we collected any frames
            if frames and thread_id is not None:
                yield frames, thread_id, leaf_id

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
                - aggregate_status_counts: dict with has_gil, on_cpu, has_exception, etc.
                - has_gc_frame: bool indicating if any thread has GC frames
                - per_thread_stats: dict mapping thread_id to per-thread counts
        """
        status_counts = {
            "has_gil": 0,
            "on_cpu": 0,
            "gil_requested": 0,
            "unknown": 0,
            "has_exception": 0,
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
                if status_flags & THREAD_STATUS_HAS_EXCEPTION:
                    status_counts["has_exception"] += 1

                # Track per-thread statistics
                thread_id = getattr(thread_info, "thread_id", None)
                if thread_id is not None:
                    if thread_id not in per_thread_stats:
                        per_thread_stats[thread_id] = {
                            "has_gil": 0,
                            "on_cpu": 0,
                            "gil_requested": 0,
                            "unknown": 0,
                            "has_exception": 0,
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
                    if status_flags & THREAD_STATUS_HAS_EXCEPTION:
                        thread_stats["has_exception"] += 1

                    # Check for GC frames in this thread
                    frames = getattr(thread_info, "frame_info", None)
                    if frames:
                        for frame in frames:
                            if self._is_gc_frame(frame):
                                thread_stats["gc_samples"] += 1
                                has_gc_frame = True
                                break

        return status_counts, has_gc_frame, per_thread_stats
