import ast
import contextlib
import re
import sys
from abc import abstractmethod
from collections.abc import Iterator
from typing import IO

from pegen.grammar import (
    Alt,
    Gather,
    Grammar,
    GrammarError,
    GrammarVisitor,
    NamedItem,
    NameLeaf,
    Plain,
    Rhs,
    Rule,
    RuleKind,
    StringLeaf,
)
from pegen.grammar_analysis import (
    InitialNamesVisitor as InitialNamesVisitor,
    NullableVisitor as NullableVisitor,
    compute_left_recursives as compute_left_recursives,
    compute_nullables as compute_nullables,
    make_first_graph as make_first_graph,
)


class RuleCollectorVisitor(GrammarVisitor):
    """Visitor that invokes a provided callmaker visitor with just the NamedItem nodes"""

    def __init__(self, callmakervisitor: GrammarVisitor) -> None:
        self.callmaker = callmakervisitor

    def visit_Rule(self, rule: Rule) -> None:
        self.visit(rule.flatten())

    def visit_NamedItem(self, item: NamedItem) -> None:
        self.callmaker.visit(item)


class KeywordCollectorVisitor(GrammarVisitor):
    """Visitor that collects all the keywords and soft keywords in the Grammar"""

    def __init__(self, gen: "ParserGenerator", keywords: dict[str, int], soft_keywords: set[str]):
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
    def __init__(self, rules: dict[str, Rule], tokens: set[str]):
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

    def __init__(self, grammar: Grammar, tokens: set[str], file: IO[str] | None):
        self.grammar = grammar
        self.tokens = tokens
        self.keywords: dict[str, int] = {}
        self.soft_keywords: set[str] = set()
        self.rules = grammar.rules
        self._validate_grammar()
        self.file = file
        self.level = 0
        self.first_graph, self.first_sccs = compute_left_recursives(self.rules)
        self.counter = 0  # For name_rule()/name_loop()
        self.keyword_counter = 499  # For keyword_type()
        self.all_rules: dict[str, Rule] = self.rules.copy()  # Rules + temporal rules
        self._local_variable_stack: list[list[str]] = []

    def _validate_grammar(self) -> None:
        self.validate_rule_names()
        if "trailer" not in self.grammar.metas and "start" not in self.rules:
            raise GrammarError("Grammar without a trailer must have a 'start' rule")
        checker = RuleCheckingVisitor(self.rules, self.tokens)
        for rule in self.rules.values():
            checker.visit(rule)

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
    def local_variable_names(self) -> list[str]:
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

        rule_collector = RuleCollectorVisitor(self.callmakervisitor)
        done: set[str] = set()
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
            kind = RuleKind.LOOP1
        else:
            prefix = "_loop0_"
            kind = RuleKind.LOOP0
        name = f"{prefix}{self.counter}"
        self.all_rules[name] = Rule(
            name, None, Rhs([Alt([NamedItem(None, node)])]), kind=kind
        )
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
            kind=RuleKind.LOOP0,
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
            kind=RuleKind.GATHER,
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
