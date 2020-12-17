from __future__ import absolute_import, unicode_literals

from collections import OrderedDict, defaultdict


def stable_topological_sort(graph):
    to_order = set(graph.keys())  # keep a log of what  we need to order

    # normalize graph - fill missing nodes (assume no dependency)
    for values in list(graph.values()):
        for value in values:
            if value not in graph:
                graph[value] = ()

    inverse_graph = defaultdict(set)
    for key, depends in graph.items():
        for depend in depends:
            inverse_graph[depend].add(key)

    topology = []
    degree = {k: len(v) for k, v in graph.items()}
    ready_to_visit = {n for n, d in degree.items() if not d}
    need_to_visit = OrderedDict((i, None) for i in graph.keys())
    while need_to_visit:
        # to keep stable, pick the first node ready to visit in the original order
        for node in need_to_visit:
            if node in ready_to_visit:
                break
        else:
            break
        del need_to_visit[node]

        topology.append(node)

        # decrease degree for nodes we're going too
        for to_node in inverse_graph[node]:
            degree[to_node] -= 1
            if not degree[to_node]:  # if a node has no more incoming node it's ready to visit
                ready_to_visit.add(to_node)

    result = [n for n in topology if n in to_order]  # filter out missing nodes we extended

    if len(result) < len(to_order):
        identify_cycle(graph)
        msg = "could not order tox environments and failed to detect circle"  # pragma: no cover
        raise ValueError(msg)  # pragma: no cover
    return result


def identify_cycle(graph):
    path = OrderedDict()
    visited = set()

    def visit(vertex):
        if vertex in visited:
            return None
        visited.add(vertex)
        path[vertex] = None
        for neighbour in graph.get(vertex, ()):
            if neighbour in path or visit(neighbour):
                return path
        del path[vertex]
        return None

    for node in graph:
        result = visit(node)
        if result is not None:
            raise ValueError("{}".format(" | ".join(result.keys())))
