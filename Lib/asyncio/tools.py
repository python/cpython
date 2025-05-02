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
    """Return every task that is *not awaited by anybody*."""
    all_children = {c for kids in children.values() for c in kids}
    return [n for n in id2label if n not in all_children]

# ─── helpers for _roots() ─────────────────────────────────────────
from collections import defaultdict

def _roots(id2label, children):
    """
    Return one root per *source* strongly-connected component (SCC).

    • Build the graph that contains **only tasks** as nodes and edges
      parent-task ─▶ child-task (ignore coroutine frames).

    • Collapse it into SCCs with Tarjan (linear time).

    • For every component whose condensation-DAG in-degree is 0, choose a
      stable representative (lexicographically-smallest label, fallback to
      smallest object-id) and return that list.
    """
    TASK = NodeType.TASK
    task_nodes = [n for n in id2label if n[0] == TASK]

    # ------------ adjacency list among *tasks only* -----------------
    adj = defaultdict(list)
    for p in task_nodes:
        adj[p] = [c for c in children.get(p, []) if c[0] == TASK]

    # ------------ Tarjan’s algorithm --------------------------------
    index = 0
    stack, on_stack = [], set()
    node_index, low = {}, {}
    comp_of = {}                      # node -> comp-id
    comps = defaultdict(list)         # comp-id -> [members]

    def strong(v):
        nonlocal index
        node_index[v] = low[v] = index
        index += 1
        stack.append(v)
        on_stack.add(v)

        for w in adj[v]:
            if w not in node_index:
                strong(w)
                low[v] = min(low[v], low[w])
            elif w in on_stack:
                low[v] = min(low[v], node_index[w])

        if low[v] == node_index[v]:           # root of an SCC
            while True:
                w = stack.pop()
                on_stack.remove(w)
                comp_of[w] = v               # use root-node as comp-id
                comps[v].append(w)
                if w == v:
                    break

    for v in task_nodes:
        if v not in node_index:
            strong(v)

    # ------------ condensation DAG in-degrees -----------------------
    indeg = defaultdict(int)
    for p in task_nodes:
        cp = comp_of[p]
        for q in adj[p]:
            cq = comp_of[q]
            if cp != cq:
                indeg[cq] += 1

    # ------------ choose one representative per source-SCC ----------
    roots = []
    for cid, members in comps.items():
        if indeg[cid] == 0:                      # source component
            roots.append(min(
                members,
                key=lambda n: (id2label[n], n[1])   # stable pick
            ))
    return roots


# ─── PRINT TREE FUNCTION ───────────────────────────────────────
def print_async_tree(result, task_emoji="(T)", cor_emoji="", printer=print):
    """
    Pretty-print the async call tree produced by `get_all_async_stacks()`,
    coping safely with cycles.
    """
    id2name, awaits = _index(result)
    labels, children = _build_tree(id2name, awaits)

    def pretty(node):
        flag = task_emoji if node[0] == NodeType.TASK else cor_emoji
        return f"{flag} {labels[node]}"

    def render(node, prefix="", last=True, buf=None, ancestry=frozenset()):
        """
        DFS renderer that stops if *node* already occurs in *ancestry*
        (i.e. we just found a cycle).
        """
        if buf is None:
            buf = []

        if node in ancestry:
            buf.append(f"{prefix}{'└── ' if last else '├── '}↺ {pretty(node)} (cycle)")
            return buf

        buf.append(f"{prefix}{'└── ' if last else '├── '}{pretty(node)}")
        new_pref = prefix + ("    " if last else "│   ")
        kids = children.get(node, [])
        for i, kid in enumerate(kids):
            render(kid, new_pref, i == len(kids) - 1, buf, ancestry | {node})
        return buf

    forest = []
    for root in _roots(labels, children):
        forest.append(render(root))
    return forest


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
        print(tasks)
    except RuntimeError as e:
        print(f"Error retrieving tasks: {e}")
        sys.exit(1)

    if args.tree:
        # Print the async call tree
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
