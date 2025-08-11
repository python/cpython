"""Tools to analyze tasks running in asyncio programs."""

from collections import defaultdict
import csv
from itertools import count
from enum import Enum, StrEnum, auto
import sys
from _remote_debugging import RemoteUnwinder, FrameInfo

class NodeType(Enum):
    COROUTINE = 1
    TASK = 2


class CycleFoundException(Exception):
    """Raised when there is a cycle when drawing the call tree."""
    def __init__(
            self,
            cycles: list[list[int]],
            id2name: dict[int, str],
        ) -> None:
        super().__init__(cycles, id2name)
        self.cycles = cycles
        self.id2name = id2name



# ─── indexing helpers ───────────────────────────────────────────
def _format_stack_entry(elem: str|FrameInfo) -> str:
    if not isinstance(elem, str):
        if elem.lineno == 0 and elem.filename == "":
            return f"{elem.funcname}"
        else:
            return f"{elem.funcname} {elem.filename}:{elem.lineno}"
    return elem


def _index(result):
    id2name, awaits, task_stacks = {}, [], {}
    for awaited_info in result:
        for task_info in awaited_info.awaited_by:
            task_id = task_info.task_id
            task_name = task_info.task_name
            id2name[task_id] = task_name

            # Store the internal coroutine stack for this task
            if task_info.coroutine_stack:
                for coro_info in task_info.coroutine_stack:
                    call_stack = coro_info.call_stack
                    internal_stack = [_format_stack_entry(frame) for frame in call_stack]
                    task_stacks[task_id] = internal_stack

            # Add the awaited_by relationships (external dependencies)
            if task_info.awaited_by:
                for coro_info in task_info.awaited_by:
                    call_stack = coro_info.call_stack
                    parent_task_id = coro_info.task_name
                    stack = [_format_stack_entry(frame) for frame in call_stack]
                    awaits.append((parent_task_id, stack, task_id))
    return id2name, awaits, task_stacks


def _build_tree(id2name, awaits, task_stacks):
    id2label = {(NodeType.TASK, tid): name for tid, name in id2name.items()}
    children = defaultdict(list)
    cor_nodes = defaultdict(dict)  # Maps parent -> {frame_name: node_key}
    next_cor_id = count(1)

    def get_or_create_cor_node(parent, frame):
        """Get existing coroutine node or create new one under parent"""
        if frame in cor_nodes[parent]:
            return cor_nodes[parent][frame]

        node_key = (NodeType.COROUTINE, f"c{next(next_cor_id)}")
        id2label[node_key] = frame
        children[parent].append(node_key)
        cor_nodes[parent][frame] = node_key
        return node_key

    # Build task dependency tree with coroutine frames
    for parent_id, stack, child_id in awaits:
        cur = (NodeType.TASK, parent_id)
        for frame in reversed(stack):
            cur = get_or_create_cor_node(cur, frame)

        child_key = (NodeType.TASK, child_id)
        if child_key not in children[cur]:
            children[cur].append(child_key)

    # Add coroutine stacks for leaf tasks
    awaiting_tasks = {parent_id for parent_id, _, _ in awaits}
    for task_id in id2name:
        if task_id not in awaiting_tasks and task_id in task_stacks:
            cur = (NodeType.TASK, task_id)
            for frame in reversed(task_stacks[task_id]):
                cur = get_or_create_cor_node(cur, frame)

    return id2label, children


def _roots(id2label, children):
    all_children = {c for kids in children.values() for c in kids}
    return [n for n in id2label if n not in all_children]

# ─── detect cycles in the task-to-task graph ───────────────────────
def _task_graph(awaits):
    """Return {parent_task_id: {child_task_id, …}, …}."""
    g = defaultdict(set)
    for parent_id, _stack, child_id in awaits:
        g[parent_id].add(child_id)
    return g


def _find_cycles(graph):
    """
    Depth-first search for back-edges.

    Returns a list of cycles (each cycle is a list of task-ids) or an
    empty list if the graph is acyclic.
    """
    WHITE, GREY, BLACK = 0, 1, 2
    color = defaultdict(lambda: WHITE)
    path, cycles = [], []

    def dfs(v):
        color[v] = GREY
        path.append(v)
        for w in graph.get(v, ()):
            if color[w] == WHITE:
                dfs(w)
            elif color[w] == GREY:            # back-edge → cycle!
                i = path.index(w)
                cycles.append(path[i:] + [w])  # make a copy
        color[v] = BLACK
        path.pop()

    for v in list(graph):
        if color[v] == WHITE:
            dfs(v)
    return cycles


# ─── PRINT TREE FUNCTION ───────────────────────────────────────
def get_all_awaited_by(pid):
    unwinder = RemoteUnwinder(pid)
    return unwinder.get_all_awaited_by()


def build_async_tree(result, task_emoji="(T)", cor_emoji=""):
    """
    Build a list of strings for pretty-print an async call tree.

    The call tree is produced by `get_all_async_stacks()`, prefixing tasks
    with `task_emoji` and coroutine frames with `cor_emoji`.
    """
    id2name, awaits, task_stacks = _index(result)
    g = _task_graph(awaits)
    cycles = _find_cycles(g)
    if cycles:
        raise CycleFoundException(cycles, id2name)
    labels, children = _build_tree(id2name, awaits, task_stacks)

    def pretty(node):
        flag = task_emoji if node[0] == NodeType.TASK else cor_emoji
        return f"{flag} {labels[node]}"

    def render(node, prefix="", last=True, buf=None):
        if buf is None:
            buf = []
        buf.append(f"{prefix}{'└── ' if last else '├── '}{pretty(node)}")
        new_pref = prefix + ("    " if last else "│   ")
        kids = children.get(node, [])
        for i, kid in enumerate(kids):
            render(kid, new_pref, i == len(kids) - 1, buf)
        return buf

    return [render(root) for root in _roots(labels, children)]


def build_task_table(result):
    id2name, _, _ = _index(result)
    table = []

    for awaited_info in result:
        thread_id = awaited_info.thread_id
        for task_info in awaited_info.awaited_by:
            # Get task info
            task_id = task_info.task_id
            task_name = task_info.task_name

            # Build coroutine stack string
            frames = [frame for coro in task_info.coroutine_stack
                     for frame in coro.call_stack]
            coro_stack = " -> ".join(_format_stack_entry(x).split(" ")[0]
                                   for x in frames)

            # Handle tasks with no awaiters
            if not task_info.awaited_by:
                table.append([thread_id, hex(task_id), task_name, coro_stack,
                            "", "", "0x0"])
                continue

            # Handle tasks with awaiters
            for coro_info in task_info.awaited_by:
                parent_id = coro_info.task_name
                awaiter_frames = [_format_stack_entry(x).split(" ")[0]
                                for x in coro_info.call_stack]
                awaiter_chain = " -> ".join(awaiter_frames)
                awaiter_name = id2name.get(parent_id, "Unknown")
                parent_id_str = (hex(parent_id) if isinstance(parent_id, int)
                               else str(parent_id))

                table.append([thread_id, hex(task_id), task_name, coro_stack,
                            awaiter_chain, awaiter_name, parent_id_str])

    return table

def _print_cycle_exception(exception: CycleFoundException):
    print("ERROR: await-graph contains cycles - cannot print a tree!", file=sys.stderr)
    print("", file=sys.stderr)
    for c in exception.cycles:
        inames = " → ".join(exception.id2name.get(tid, hex(tid)) for tid in c)
        print(f"cycle: {inames}", file=sys.stderr)


def _get_awaited_by_tasks(pid: int) -> list:
    try:
        return get_all_awaited_by(pid)
    except RuntimeError as e:
        while e.__context__ is not None:
            e = e.__context__
        print(f"Error retrieving tasks: {e}")
        sys.exit(1)


class TaskTableOutputFormat(StrEnum):
    table = auto()
    csv = auto()
    bsv = auto()
    # 🍌SV is not just a format. It's a lifestyle. A philosophy.
    # https://www.youtube.com/watch?v=RrsVi1P6n0w


def display_awaited_by_tasks_table(pid, *, format=TaskTableOutputFormat.table):
    """Build and print a table of all pending tasks under `pid`."""

    tasks = _get_awaited_by_tasks(pid)
    table = build_task_table(tasks)
    format = TaskTableOutputFormat(format)
    if format == TaskTableOutputFormat.table:
        _display_awaited_by_tasks_table(table)
    else:
        _display_awaited_by_tasks_csv(table, format=format)


_row_header = ('tid', 'task id', 'task name', 'coroutine stack',
               'awaiter chain', 'awaiter name', 'awaiter id')


def _display_awaited_by_tasks_table(table):
    """Print the table in a simple tabular format."""
    print(_fmt_table_row(*_row_header))
    print('-' * 180)
    for row in table:
        print(_fmt_table_row(*row))


def _fmt_table_row(tid, task_id, task_name, coro_stack,
                   awaiter_chain, awaiter_name, awaiter_id):
    # Format a single row for the table format
    return (f'{tid:<10} {task_id:<20} {task_name:<20} {coro_stack:<50} '
            f'{awaiter_chain:<50} {awaiter_name:<15} {awaiter_id:<15}')


def _display_awaited_by_tasks_csv(table, *, format):
    """Print the table in CSV format"""
    if format == TaskTableOutputFormat.csv:
        delimiter = ','
    elif format == TaskTableOutputFormat.bsv:
        delimiter = '\N{BANANA}'
    else:
        raise ValueError(f"Unknown output format: {format}")
    csv_writer = csv.writer(sys.stdout, delimiter=delimiter)
    csv_writer.writerow(_row_header)
    csv_writer.writerows(table)


def display_awaited_by_tasks_tree(pid: int) -> None:
    """Build and print a tree of all pending tasks under `pid`."""

    tasks = _get_awaited_by_tasks(pid)
    try:
        result = build_async_tree(tasks)
    except CycleFoundException as e:
        _print_cycle_exception(e)
        sys.exit(1)

    for tree in result:
        print("\n".join(tree))
