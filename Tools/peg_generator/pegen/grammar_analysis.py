"""Nullable and left-recursion analysis for source grammar rules."""

from collections.abc import Iterable, Set
from typing import Any

from pegen import sccutils
from pegen.grammar import (
    Alt,
    Cut,
    Forced,
    Gather,
    GrammarVisitor,
    Group,
    NamedItem,
    NameLeaf,
    Opt,
    Repeat0,
    Repeat1,
    Rhs,
    Rule,
    StringLeaf,
)


class NullableVisitor(GrammarVisitor):
    def __init__(self, rules: dict[str, Rule]) -> None:
        self.rules = rules
        self.visited: set[Any] = set()
        self.nullables: set[Rule | NamedItem] = set()

    def visit(self, node: Any, *args: Any, **kwargs: Any) -> bool | None:
        match node:
            case Rule(rhs=rhs):
                if node in self.visited:
                    return False
                self.visited.add(node)
                if self.visit(rhs):
                    self.nullables.add(node)
                return node in self.nullables
            case NamedItem(item=item):
                if self.visit(item):
                    self.nullables.add(node)
                return node in self.nullables
            case Rhs(alts=alts):
                return any(self.visit(alt) for alt in alts)
            case Alt(items=items):
                return all(self.visit(item) for item in items)
            case Forced() | Opt() | Repeat0():
                return True
            case Repeat1() | Gather() | Cut():
                return False
            case Group(rhs=rhs):
                return self.visit(rhs)
            case NameLeaf(value=name):
                if (rule := self.rules.get(name)) is not None:
                    return self.visit(rule)
                # Token or unknown; never empty.
                return False
            case StringLeaf(value=value):
                # The string token '' is considered empty.
                return not value
            case _:
                return self.generic_visit(node, *args, **kwargs)


def compute_nullables(rules: dict[str, Rule]) -> set[Any]:
    """Compute which rules in a grammar are nullable.

    Thanks to TatSu (tatsu/leftrec.py) for inspiration.
    """
    nullable_visitor = NullableVisitor(rules)
    for rule in rules.values():
        nullable_visitor.visit(rule)
    return nullable_visitor.nullables


class InitialNamesVisitor(GrammarVisitor):
    def __init__(self, rules: dict[str, Rule]) -> None:
        self.rules = rules
        self.nullables = compute_nullables(rules)

    def generic_visit(self, node: Iterable[Any], *args: Any, **kwargs: Any) -> set[Any]:
        names: set[str] = set()
        for value in node:
            if isinstance(value, list):
                for item in value:
                    names |= self.visit(item, *args, **kwargs)
            else:
                names |= self.visit(value, *args, **kwargs)
        return names

    def visit(self, node: Any, *args: Any, **kwargs: Any) -> set[Any]:
        match node:
            case Alt(items=items):
                names: set[str] = set()
                for item in items:
                    names |= self.visit(item)
                    if item not in self.nullables:
                        break
                return names
            case Forced() | Cut() | StringLeaf():
                return set()
            case NameLeaf(value=name):
                return {name}
            case _:
                return self.generic_visit(node, *args, **kwargs)


def compute_left_recursives(
    rules: dict[str, Rule]
) -> tuple[dict[str, Set[str]], list[Set[str]]]:
    graph = make_first_graph(rules)
    sccs = list(sccutils.strongly_connected_components(graph.keys(), graph))
    for scc in sccs:
        if len(scc) > 1:
            for name in scc:
                rules[name].left_recursive = True
            # Try to find a leader such that all cycles go through it.
            leaders = set(scc)
            for start in scc:
                for cycle in sccutils.find_cycles_in_scc(graph, scc, start):
                    # print("Cycle:", " -> ".join(cycle))
                    leaders -= scc - set(cycle)
                    if not leaders:
                        raise ValueError(
                            f"SCC {scc} has no leadership candidate (no element is included in all cycles)"
                        )
            # print("Leaders:", leaders)
            leader = min(leaders)  # Pick an arbitrary leader from the candidates.
            rules[leader].leader = True
        else:
            name = min(scc)  # The only element.
            if name in graph[name]:
                rules[name].left_recursive = True
                rules[name].leader = True
    return graph, sccs


def make_first_graph(rules: dict[str, Rule]) -> dict[str, Set[str]]:
    """Compute the graph of left-invocations.

    There's an edge from A to B if A may invoke B at its initial
    position.

    Note that this requires the nullable flags to have been computed.
    """
    initial_name_visitor = InitialNamesVisitor(rules)
    graph: dict[str, Set[str]] = {}
    vertices: set[str] = set()
    for rulename, rhs in rules.items():
        graph[rulename] = names = initial_name_visitor.visit(rhs)
        vertices |= names
    for vertex in vertices:
        graph.setdefault(vertex, set())
    return graph
