import sys
import ast
import contextlib
import re
from abc import abstractmethod
from typing import (
    IO,
    AbstractSet,
    Any,
    Dict,
    Iterable,
    Iterator,
    List,
    Optional,
    Set,
    Text,
    Tuple,
    Union,
)

from pegen import sccutils
from pegen.grammar import (
    Alt,
    Cut,
    Forced,
    Gather,
    Grammar,
    GrammarError,
    GrammarVisitor,
    Group,
    Lookahead,
    NamedItem,
    NameLeaf,
    Opt,
    Plain,
    Repeat0,
    Repeat1,
    Rhs,
    Rule,
    StringLeaf,
)


class RuleCollectorVisitor(GrammarVisitor):
    """Visitor that invokes a provided callmaker visitor with just the NamedItem nodes"""

    def __init__(self, rules: Dict[str, Rule], callmakervisitor: GrammarVisitor) -> None:
        self.rulses = rules
        self.callmaker = callmakervisitor

    def visit_Rule(self, rule: Rule) -> None:
        self.visit(rule.flatten())

    def visit_NamedItem(self, item: NamedItem) -> None:
        self.callmaker.visit(item)


class KeywordCollectorVisitor(GrammarVisitor):
    """Visitor that collects all the keywords and soft keywords in the Grammar"""

    def __init__(self, gen: "ParserGenerator", keywords: Dict[str, int], soft_keywords: Set[str]):
        self.generator = gen
        self.keywords = keywords
        self.soft_keywords = soft_keywords

    def visit_StringLeaf(self, node: StringLeaf) -> None:
        val = ast.literal_eval(node.value)
        if re.match(r"[a-zA-Z_]\w*\Z", val):  # This is a keyword
            if node.value.endswith("'") and node.value not in self.keywords:
                self.keywords[val] = self.generator.keyword_type()
            else:
                return self.soft_keywords.add(node.value.replace('"', ""))


class RuleCheckingVisitor(GrammarVisitor):
    def __init__(self, rules: Dict[str, Rule], tokens: Set[str]):
        self.rules = rules
        self.tokens = tokens
        # If python < 3.12 add the virtual fstring tokens
        if sys.version_info < (3, 12):
            self.tokens.add("FSTRING_START")
            self.tokens.add("FSTRING_END")
            self.tokens.add("FSTRING_MIDDLE")
        # If python < 3.14 add the virtual tstring tokens
        if sys.version_info < (3, 14, 0, 'beta', 1):
            self.tokens.add("TSTRING_START")
            self.tokens.add("TSTRING_END")
            self.tokens.add("TSTRING_MIDDLE")

    def visit_NameLeaf(self, node: NameLeaf) -> None:
        if node.value not in self.rules and node.value not in self.tokens:
            raise GrammarError(f"Dangling reference to rule {node.value!r}")

    def visit_NamedItem(self, node: NamedItem) -> None:
        if node.name and node.name.startswith("_"):
            raise GrammarError(f"Variable names cannot start with underscore: '{node.name}'")
        self.visit(node.item)


class ParserGenerator:
    callmakervisitor: GrammarVisitor

    def __init__(self, grammar: Grammar, tokens: Set[str], file: Optional[IO[Text]]):
        self.grammar = grammar
        self.tokens = tokens
        self.keywords: Dict[str, int] = {}
        self.soft_keywords: Set[str] = set()
        self.rules = grammar.rules
        self.validate_rule_names()
        if "trailer" not in grammar.metas and "start" not in self.rules:
            raise GrammarError("Grammar without a trailer must have a 'start' rule")
        checker = RuleCheckingVisitor(self.rules, self.tokens)
        for rule in self.rules.values():
            checker.visit(rule)
        self.file = file
        self.level = 0
        self.first_graph, self.first_sccs = compute_left_recursives(self.rules)
        self.counter = 0  # For name_rule()/name_loop()
        self.keyword_counter = 499  # For keyword_type()
        self.all_rules: Dict[str, Rule] = self.rules.copy()  # Rules + temporal rules
        self._local_variable_stack: List[List[str]] = []

    def validate_rule_names(self) -> None:
        for rule in self.rules:
            if rule.startswith("_"):
                raise GrammarError(f"Rule names cannot start with underscore: '{rule}'")

    @contextlib.contextmanager
    def local_variable_context(self) -> Iterator[None]:
        self._local_variable_stack.append([])
        yield
        self._local_variable_stack.pop()

    @property
    def local_variable_names(self) -> List[str]:
        return self._local_variable_stack[-1]

    @abstractmethod
    def generate(self, filename: str) -> None:
        raise NotImplementedError

    @contextlib.contextmanager
    def indent(self) -> Iterator[None]:
        self.level += 1
        try:
            yield
        finally:
            self.level -= 1

    def print(self, *args: object) -> None:
        if not args:
            print(file=self.file)
        else:
            print("    " * self.level, end="", file=self.file)
            print(*args, file=self.file)

    def printblock(self, lines: str) -> None:
        for line in lines.splitlines():
            self.print(line)

    def collect_rules(self) -> None:
        keyword_collector = KeywordCollectorVisitor(self, self.keywords, self.soft_keywords)
        for rule in self.all_rules.values():
            keyword_collector.visit(rule)

        rule_collector = RuleCollectorVisitor(self.rules, self.callmakervisitor)
        done: Set[str] = set()
        while True:
            computed_rules = list(self.all_rules)
            todo = [i for i in computed_rules if i not in done]
            if not todo:
                break
            done = set(self.all_rules)
            for rulename in todo:
                rule_collector.visit(self.all_rules[rulename])

    def keyword_type(self) -> int:
        self.keyword_counter += 1
        return self.keyword_counter

    def artificial_rule_from_rhs(self, rhs: Rhs) -> str:
        self.counter += 1
        name = f"_tmp_{self.counter}"  # TODO: Pick a nicer name.
        self.all_rules[name] = Rule(name, None, rhs)
        return name

    def artificial_rule_from_repeat(self, node: Plain, is_repeat1: bool) -> str:
        self.counter += 1
        if is_repeat1:
            prefix = "_loop1_"
        else:
            prefix = "_loop0_"
        name = f"{prefix}{self.counter}"
        self.all_rules[name] = Rule(name, None, Rhs([Alt([NamedItem(None, node)])]))
        return name

    def artificial_rule_from_gather(self, node: Gather) -> str:
        self.counter += 1
        extra_function_name = f"_loop0_{self.counter}"
        extra_function_alt = Alt(
            [NamedItem(None, node.separator), NamedItem("elem", node.node)],
            action="elem",
        )
        self.all_rules[extra_function_name] = Rule(
            extra_function_name,
            None,
            Rhs([extra_function_alt]),
        )
        self.counter += 1
        name = f"_gather_{self.counter}"
        alt = Alt(
            [NamedItem("elem", node.node), NamedItem("seq", NameLeaf(extra_function_name))],
        )
        self.all_rules[name] = Rule(
            name,
            None,
            Rhs([alt]),
        )
        return name

    def dedupe(self, name: str) -> str:
        origname = name
        counter = 0
        while name in self.local_variable_names:
            counter += 1
            name = f"{origname}_{counter}"
        self.local_variable_names.append(name)
        return name


class NullableVisitor(GrammarVisitor):
    def __init__(self, rules: Dict[str, Rule]) -> None:
        self.rules = rules
        self.visited: Set[Any] = set()
        self.nullables: Set[Union[Rule, NamedItem]] = set()

    def visit_Rule(self, rule: Rule) -> bool:
        if rule in self.visited:
            return False
        self.visited.add(rule)
        if self.visit(rule.rhs):
            self.nullables.add(rule)
        return rule in self.nullables

    def visit_Rhs(self, rhs: Rhs) -> bool:
        for alt in rhs.alts:
            if self.visit(alt):
                return True
        return False

    def visit_Alt(self, alt: Alt) -> bool:
        for item in alt.items:
            if not self.visit(item):
                return False
        return True

    def visit_Forced(self, force: Forced) -> bool:
        return True

    def visit_LookAhead(self, lookahead: Lookahead) -> bool:
        return True

    def visit_Opt(self, opt: Opt) -> bool:
        return True

    def visit_Repeat0(self, repeat: Repeat0) -> bool:
        return True

    def visit_Repeat1(self, repeat: Repeat1) -> bool:
        return False

    def visit_Gather(self, gather: Gather) -> bool:
        return False

    def visit_Cut(self, cut: Cut) -> bool:
        return False

    def visit_Group(self, group: Group) -> bool:
        return self.visit(group.rhs)

    def visit_NamedItem(self, item: NamedItem) -> bool:
        if self.visit(item.item):
            self.nullables.add(item)
        return item in self.nullables

    def visit_NameLeaf(self, node: NameLeaf) -> bool:
        if node.value in self.rules:
            return self.visit(self.rules[node.value])
        # Token or unknown; never empty.
        return False

    def visit_StringLeaf(self, node: StringLeaf) -> bool:
        # The string token '' is considered empty.
        return not node.value


def compute_nullables(rules: Dict[str, Rule]) -> Set[Any]:
    """Compute which rules in a grammar are nullable.

    Thanks to TatSu (tatsu/leftrec.py) for inspiration.
    """
    nullable_visitor = NullableVisitor(rules)
    for rule in rules.values():
        nullable_visitor.visit(rule)
    return nullable_visitor.nullables


class InitialNamesVisitor(GrammarVisitor):
    def __init__(self, rules: Dict[str, Rule]) -> None:
        self.rules = rules
        self.nullables = compute_nullables(rules)

    def generic_visit(self, node: Iterable[Any], *args: Any, **kwargs: Any) -> Set[Any]:
        names: Set[str] = set()
        for value in node:
            if isinstance(value, list):
                for item in value:
                    names |= self.visit(item, *args, **kwargs)
            else:
                names |= self.visit(value, *args, **kwargs)
        return names

    def visit_Alt(self, alt: Alt) -> Set[Any]:
        names: Set[str] = set()
        for item in alt.items:
            names |= self.visit(item)
            if item not in self.nullables:
                break
        return names

    def visit_Forced(self, force: Forced) -> Set[Any]:
        return set()

    def visit_LookAhead(self, lookahead: Lookahead) -> Set[Any]:
        return set()

    def visit_Cut(self, cut: Cut) -> Set[Any]:
        return set()

    def visit_NameLeaf(self, node: NameLeaf) -> Set[Any]:
        return {node.value}

    def visit_StringLeaf(self, node: StringLeaf) -> Set[Any]:
        return set()


def compute_left_recursives(
    rules: Dict[str, Rule]
) -> Tuple[Dict[str, AbstractSet[str]], List[AbstractSet[str]]]:
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


def make_first_graph(rules: Dict[str, Rule]) -> Dict[str, AbstractSet[str]]:
    """Compute the graph of left-invocations.

    There's an edge from A to B if A may invoke B at its initial
    position.

    Note that this requires the nullable flags to have been computed.
    """
    initial_name_visitor = InitialNamesVisitor(rules)
    graph = {}
    vertices: Set[str] = set()
    for rulename, rhs in rules.items():
        graph[rulename] = names = initial_name_visitor.visit(rhs)
        vertices |= names
    for vertex in vertices:
        graph.setdefault(vertex, set())
    return graph
