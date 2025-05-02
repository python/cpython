import argparse
from collections import defaultdict
from itertools import count
from enum import Enum
import sys
from _remotedebugging import get_all_awaited_by


class NodeType(Enum):
    COROUTINE = 1
    TASK = 2


# ─── indexing helpers ───────────────────────────────────────────
def _index(result):
    id2name, awaits = {}, []
    for _thr_id, tasks in result:
        for tid, tname, awaited in tasks:
            id2name[tid] = tname
            for stack, parent_id in awaited:
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

    # touch every task so it’s present even if it awaits nobody
    for tid in id2name:
        children[(NodeType.TASK, tid)]

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
    roots = [n for n, lbl in id2label.items() if lbl == "Task-1"]
    if roots:
        return roots
    all_children = {c for kids in children.values() for c in kids}
    return [n for n in id2label if n not in all_children]

# ─── detect cycles in the task-to-task graph ───────────────────────
def _task_graph(awaits):
    """Return {parent_task_id: {child_task_id, …}, …}."""
    from collections import defaultdict
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
    color = {n: WHITE for n in graph}
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
def print_async_tree(result, task_emoji="(T)", cor_emoji="", printer=print):
    """
    Pretty-print the async call tree produced by `get_all_async_stacks()`,
    prefixing tasks with *task_emoji* and coroutine frames with *cor_emoji*.
    """
    id2name, awaits = _index(result)
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

    result = []
    for r, root in enumerate(_roots(labels, children)):
        result.append(render(root))
    return result


def build_task_table(result):
    id2name, awaits = _index(result)
    table = []
    for tid, tasks in result:
        for task_id, task_name, awaited in tasks:
            for stack, awaiter_id in awaited:
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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Show Python async tasks in a process")
    parser.add_argument("pid", type=int, help="Process ID(s) to inspect.")
    parser.add_argument(
        "--tree", "-t", action="store_true", help="Display tasks in a tree format"
    )
    args = parser.parse_args()

    try:
        tasks = get_all_awaited_by(args.pid)
    except RuntimeError as e:
        print(f"Error retrieving tasks: {e}")
        sys.exit(1)

    if args.tree:
        # Print the async call tree
        id2name, awaits = _index(tasks)
        g = _task_graph(awaits)
        cycles = _find_cycles(g)

        if cycles:
            print("ERROR: await-graph contains cycles – cannot print a tree!\n")
            for c in cycles:
                # pretty-print task names instead of bare ids
                names = " → ".join(id2name.get(tid, hex(tid)) for tid in c)
                print(f"  cycle: {names}")
            sys.exit(1)

        result = print_async_tree(tasks)
        for tree in result:
            print("\n".join(tree))
    else:
        # Build and print the task table
        table = build_task_table(tasks)
        # Print the table in a simple tabular format
        print(
            f"{'tid':<10} {'task id':<20} {'task name':<20} {'coroutine chain':<50} {'awaiter name':<20} {'awaiter id':<15}"
        )
        print("-" * 135)
        for row in table:
            print(f"{row[0]:<10} {row[1]:<20} {row[2]:<20} {row[3]:<50} {row[4]:<20} {row[5]:<15}")
