# Adapted from mypy (mypy/build.py) under the MIT license.

from collections.abc import Iterable, Iterator, Set


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


def find_cycles_in_scc(
    graph: dict[str, Set[str]], scc: Set[str], start: str
) -> Iterable[list[str]]:
    """Find cycles in SCC emanating from start.

    Yields lists of the form ['A', 'B', 'C', 'A'], which means there's
    a path from A -> B -> C -> A.  The first item is always the start
    argument, but the last item may be another element, e.g.  ['A',
    'B', 'C', 'B'] means there's a path from A to B and there's a
    cycle from B to C and back.
    """
    # Basic input checks.
    assert start in scc, (start, scc)
    assert scc <= graph.keys(), scc - graph.keys()

    # Reduce the graph to nodes in the SCC.
    graph = {src: {dst for dst in dsts if dst in scc} for src, dsts in graph.items() if src in scc}
    assert start in graph

    # Recursive helper that yields cycles.
    def dfs(node: str, path: list[str]) -> Iterator[list[str]]:
        if node in path:
            yield path + [node]
            return
        path = path + [node]  # TODO: Make this not quadratic.
        for child in graph[node]:
            yield from dfs(child, path)

    yield from dfs(start, [])
