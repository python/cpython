# Adapted from mypy (mypy/build.py) under the MIT license.

from collections.abc import Iterator, Set


def strongly_connected_components(
    vertices: Set[str], edges: dict[str, Set[str]]
) -> Iterator[Set[str]]:
    """Compute Strongly Connected Components of a directed graph.

    Args:
      vertices: the labels for the vertices
      edges: for each vertex, gives the target vertices of its outgoing edges

    Returns:
      An iterator yielding strongly connected components, each
      represented as a set of vertices.  Each input vertex will occur
      exactly once; vertices not part of a SCC are returned as
      singleton sets.

    From https://code.activestate.com/recipes/578507-strongly-connected-components-of-a-directed-graph/.
    """
    identified: set[str] = set()
    stack: list[str] = []
    index: dict[str, int] = {}
    boundaries: list[int] = []

    def dfs(v: str) -> Iterator[set[str]]:
        index[v] = len(stack)
        stack.append(v)
        boundaries.append(index[v])

        for w in edges[v]:
            if w not in index:
                yield from dfs(w)
            elif w not in identified:
                while index[w] < boundaries[-1]:
                    boundaries.pop()

        if boundaries[-1] == index[v]:
            boundaries.pop()
            scc = set(stack[index[v] :])
            del stack[index[v] :]
            identified.update(scc)
            yield scc

    for v in vertices:
        if v not in index:
            yield from dfs(v)


def is_acyclic(graph: dict[str, Set[str]], vertices: Set[str]) -> bool:
    """Check the subgraph induced by vertices using a topological sort."""
    indegree = dict.fromkeys(vertices, 0)
    for src in vertices:
        for dst in graph[src]:
            if dst in vertices:
                indegree[dst] += 1

    ready = [node for node, degree in indegree.items() if degree == 0]
    processed = 0
    while ready:
        src = ready.pop()
        processed += 1
        for dst in graph[src]:
            if dst in vertices:
                indegree[dst] -= 1
                if indegree[dst] == 0:
                    ready.append(dst)
    return processed == len(vertices)
