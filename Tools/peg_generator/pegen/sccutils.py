# Adapted from mypy (mypy/build.py) under the MIT license.

from typing import *


def strongly_connected_components(
    vertices: AbstractSet[str], edges: Dict[str, AbstractSet[str]]
) -> Iterator[AbstractSet[str]]:
    """Compute Strongly Connected Components of a directed graph.

    Args:
      vertices: the labels for the vertices
      edges: for each vertex, gives the target vertices of its outgoing edges

    Returns:
      An iterator yielding strongly connected components, each
      represented as a set of vertices.  Each input vertex will occur
      exactly once; vertices not part of a SCC are returned as
      singleton sets.

    From http://code.activestate.com/recipes/578507/.
    """
    identified: Set[str] = set()
    stack: List[str] = []
    index: Dict[str, int] = {}
    boundaries: List[int] = []

    def dfs(v: str) -> Iterator[Set[str]]:
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


def topsort(
    data: Dict[AbstractSet[str], Set[AbstractSet[str]]]
) -> Iterable[AbstractSet[AbstractSet[str]]]:
    """Topological sort.

    Args:
      data: A map from SCCs (represented as frozen sets of strings) to
            sets of SCCs, its dependencies.  NOTE: This data structure
            is modified in place -- for normalization purposes,
            self-dependencies are removed and entries representing
            orphans are added.

    Returns:
      An iterator yielding sets of SCCs that have an equivalent
      ordering.  NOTE: The algorithm doesn't care about the internal
      structure of SCCs.

    Example:
      Suppose the input has the following structure:

        {A: {B, C}, B: {D}, C: {D}}

      This is normalized to:

        {A: {B, C}, B: {D}, C: {D}, D: {}}

      The algorithm will yield the following values:

        {D}
        {B, C}
        {A}

    From http://code.activestate.com/recipes/577413/.
    """
    # TODO: Use a faster algorithm?
    for k, v in data.items():
        v.discard(k)  # Ignore self dependencies.
    for item in set.union(*data.values()) - set(data.keys()):
        data[item] = set()
    while True:
        ready = {item for item, dep in data.items() if not dep}
        if not ready:
            break
        yield ready
        data = {item: (dep - ready) for item, dep in data.items() if item not in ready}
    assert not data, "A cyclic dependency exists amongst %r" % data


def find_cycles_in_scc(
    graph: Dict[str, AbstractSet[str]], scc: AbstractSet[str], start: str
) -> Iterable[List[str]]:
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
    def dfs(node: str, path: List[str]) -> Iterator[List[str]]:
        if node in path:
            yield path + [node]
            return
        path = path + [node]  # TODO: Make this not quadratic.
        for child in graph[node]:
            yield from dfs(child, path)

    yield from dfs(start, [])
