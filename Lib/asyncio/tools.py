"""Tools to analyze tasks running in asyncio programs."""

from dataclasses import dataclass
from collections import defaultdict
from itertools import count
from enum import Enum
import platform
import sys
from _remote_debugging import get_all_awaited_by


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
def _format_stack_entry(elem: tuple[str, str, int] | str) -> str:
    if isinstance(elem, tuple):
        fqname, path, line_no = elem
        return f"{fqname} {path}:{line_no}"

    return elem


def _index(result):
    id2name, awaits = {}, []
    for _thr_id, tasks in result:
        for tid, tname, awaited in tasks:
            id2name[tid] = tname
            for stack, parent_id in awaited:
                stack = [_format_stack_entry(elem) for elem in stack]
                awaits.append((parent_id, stack, tid))
    return id2name, awaits


def _build_tree(id2name, awaits):
    id2label = {(NodeType.TASK, tid): name for tid, name in id2name.items()}
    children = defaultdict(list)
    cor_names = defaultdict(dict)  # (parent) -> {frame: node}
    cor_id_seq = count(1)

    def _cor_node(parent_key, frame_name):
        """Return an existing or new (NodeType.COROUTINE, …) node under *parent_key*."""
        bucket = cor_names[parent_key]
        if frame_name in bucket:
            return bucket[frame_name]
        node_key = (NodeType.COROUTINE, f"c{next(cor_id_seq)}")
        id2label[node_key] = frame_name
        children[parent_key].append(node_key)
        bucket[frame_name] = node_key
        return node_key

    # lay down parent ➜ …frames… ➜ child paths
    for parent_id, stack, child_id in awaits:
        cur = (NodeType.TASK, parent_id)
        for frame in reversed(stack):  # outer-most → inner-most
            cur = _cor_node(cur, frame)
        child_key = (NodeType.TASK, child_id)
        if child_key not in children[cur]:
            children[cur].append(child_key)

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
def build_async_tree(result, task_emoji="(T)", cor_emoji=""):
    """
    Build a list of strings for pretty-print an async call tree.

    The call tree is produced by `get_all_async_stacks()`, prefixing tasks
    with `task_emoji` and coroutine frames with `cor_emoji`.
    """
    id2name, awaits = _index(result)
    g = _task_graph(awaits)
    cycles = _find_cycles(g)
    if cycles:
        raise CycleFoundException(cycles, id2name)
    labels, children = _build_tree(id2name, awaits)

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
    id2name, awaits = _index(result)
    table = []
    for tid, tasks in result:
        for task_id, task_name, awaited in tasks:
            if not awaited:
                table.append(
                    [
                        tid,
                        hex(task_id),
                        task_name,
                        "",
                        "",
                        "0x0"
                    ]
                )
            for stack, awaiter_id in awaited:
                stack = [elem[0] if isinstance(elem, tuple) else elem for elem in stack]
                coroutine_chain = " -> ".join(stack)
                awaiter_name = id2name.get(awaiter_id, "Unknown")
                table.append(
                    [
                        tid,
                        hex(task_id),
                        task_name,
                        coroutine_chain,
                        awaiter_name,
                        hex(awaiter_id),
                    ]
                )

    return table

def _print_cycle_exception(exception: CycleFoundException):
    print("ERROR: await-graph contains cycles - cannot print a tree!", file=sys.stderr)
    print("", file=sys.stderr)
    for c in exception.cycles:
        inames = " → ".join(exception.id2name.get(tid, hex(tid)) for tid in c)
        print(f"cycle: {inames}", file=sys.stderr)


LINUX_PERMISSION_HELP_TEXT = """
Error: The specified process cannot be attached to due to insufficient
permissions.

This could be because the tracer lacks the required capability
(CAP_SYS_PTRACE), or because the process is already being traced. Additionally,
security restrictions may prevent attaching to processes you cannot signal, or
those running with set-user-ID/set-group-ID.

If you are trying to attach to a process you own, you can try the following:

* Re-run the command with elevated privileges, for example: 'sudo -E !!'

* Temporarily relax ptrace restrictions for the current session (until reboot)
by running:
    echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope

Note: Disabling ptrace scope reduces system hardening and should only be done
in trusted environments.
"""

MACOS_PERMISSION_HELP_TEXT = """
Error: The specified process cannot be attached to due to insufficient
permissions.

Try re-running the command with elevated privileges (e.g., using 'sudo'):

    sudo python -m pdb -p <PID>

Note: Security restrictions may block debugging even for processes you own
unless run with root privileges.
"""

WINDOWS_PERMISSION_HELP_TEXT = """
Error: The specified process cannot be attached to due to insufficient
permissions.

Try running the command prompt or terminal as Administrator and re-run the
command.

Note: Some processes may still be inaccessible without special privileges such
as 'SeDebugPrivilege', even when running as Administrator.

To adjust file or folder permissions:

1. Right-click the file or folder and select "Properties".
2. Go to the "Security" tab. At the top, you'll see a list of users and groups
   with current access.
3. Click "Edit" to change permissions.
4. In the "Group or user names" section, select your user account.
5. In the "Permissions" section below, check "Read" or "Full control" as needed.
6. Click "Apply", then "OK" to confirm changes.
"""


def exit_with_permission_help_text():
    """
    Prints platform-specific permission help text and exits the program.

    This function is called when a PermissionError is encountered while trying
    to attach to a process.
    """
    system = platform.system()
    if system == "Linux":
        print(LINUX_PERMISSION_HELP_TEXT)
    elif system == "Darwin":
        print(MACOS_PERMISSION_HELP_TEXT)
    elif system == "Windows":
        print(WINDOWS_PERMISSION_HELP_TEXT)
    else:
        print(
            "Permission denied when trying to attach to the process. "
            "Make sure you have sufficient privileges."
        )
    sys.exit(1)


def _get_awaited_by_tasks(pid: int) -> list:
    try:
        return get_all_awaited_by(pid)
    except RuntimeError as e:
        while e.__context__ is not None:
            e = e.__context__
        print(f"Error retrieving tasks: {e}")
        sys.exit(1)
    except PermissionError as e:
        exit_with_permission_help_text()


def display_awaited_by_tasks_table(pid: int) -> None:
    """Build and print a table of all pending tasks under `pid`."""

    tasks = _get_awaited_by_tasks(pid)
    table = build_task_table(tasks)
    # Print the table in a simple tabular format
    print(
        f"{'tid':<10} {'task id':<20} {'task name':<20} {'coroutine chain':<50} {'awaiter name':<20} {'awaiter id':<15}"
    )
    print("-" * 135)
    for row in table:
        print(f"{row[0]:<10} {row[1]:<20} {row[2]:<20} {row[3]:<50} {row[4]:<20} {row[5]:<15}")


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
