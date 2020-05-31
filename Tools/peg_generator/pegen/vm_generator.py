import contextlib
import sys
import token
from collections import defaultdict
from itertools import accumulate
from typing import Any, Dict, Iterator, List, Optional

from pegen import grammar
from pegen.build import build_parser
from pegen.grammar import (
    Alt,
    Cut,
    Gather,
    GrammarVisitor,
    Group,
    Lookahead,
    NamedItem,
    NameLeaf,
    NegativeLookahead,
    Opt,
    Plain,
    PositiveLookahead,
    Repeat0,
    Repeat1,
    Rhs,
    Rule,
    StringLeaf,
)
from pegen.parser_generator import ParserGenerator


class RootRule(Rule):
    def __init__(self, name: str, startrulename: str):
        super().__init__(name, "void *", Rhs([]), None)
        self.startrulename = startrulename


class VMCallMakerVisitor(GrammarVisitor):
    def __init__(
        self, parser_generator: ParserGenerator,
    ):
        self.gen = parser_generator
        self.cache: Dict[Any, Any] = {}

    def visit_Repeat0(self, node: Repeat0) -> None:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_loop(node.node, False)
        self.cache[node] = name

    def visit_Repeat1(self, node: Repeat1) -> None:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_loop(node.node, True)
        self.cache[node] = name


class VMParserGenerator(ParserGenerator, GrammarVisitor):
    def __init__(
        self, grammar: grammar.Grammar,
    ):
        super().__init__(grammar, token.tok_name, sys.stdout)

        self.opcode_buffer: Optional[List[str]] = None
        self.callmakervisitor: VMCallMakerVisitor = VMCallMakerVisitor(self)

    @contextlib.contextmanager
    def set_opcode_buffer(self, buffer: List[str]) -> Iterator[None]:
        self.opcode_buffer = buffer
        yield
        self.opcode_buffer = None

    def add_opcode(self, opcode: str) -> None:
        assert self.opcode_buffer is not None
        self.opcode_buffer.append(opcode)

    def generate(self, filename: str) -> None:
        self.add_root_rules()
        self.collect_todo()
        while self.todo:
            self.print("static Rule all_rules[] = {")
            for rulename, rule in list(self.todo.items()):
                del self.todo[rulename]
                with self.indent():
                    self.visit(rule)
            self.print("}")

    def add_root_rules(self) -> None:
        assert "root" not in self.todo
        assert "root" not in self.rules
        root = RootRule("root", "start")  # TODO: determine start rules dynamically
        # Odd way of updating todo/rules, to make the root rule appear first.
        # TODO: Is this necessary?
        self.todo = {"root": root} | self.todo
        self.rules = {"root": root} | self.rules

    def visit_RootRule(self, node: RootRule) -> None:
        # TODO: Refactor visit_Rule() so we can share code.
        self.print(f'{{"{node.name}",')
        self.print(f" R_{node.name.upper()},")

        opcodes_by_alt: Dict[int, List[str]] = {0: [], 1: []}
        with self.set_opcode_buffer(opcodes_by_alt[0]):
            self.add_opcode("OP_RULE")
            self.add_opcode(f"R_{node.startrulename.upper()}")
            self.add_opcode("OP_SUCCESS")
        with self.set_opcode_buffer(opcodes_by_alt[1]):
            self.add_opcode("OP_FAILURE")

        indexes = [0, len(opcodes_by_alt[0]), -1]

        self.print(f" {{{', '.join(map(str, indexes))}}},")

        self.print(" {")
        with self.indent():
            for index, opcodes in opcodes_by_alt.items():
                self.print(", ".join(opcodes) + ",")
        self.print(" },")

        self.print("},")

    def visit_Rule(self, node: Rule) -> None:
        is_loop = node.is_loop()
        is_gather = node.is_gather()
        rhs = node.flatten()
        if node.left_recursive:
            raise ValueError("No left recursiveness")
        self.print(f'{{"{node.name}",')
        self.print(f" R_{node.name.upper()},")
        self.current_rule = node  # TODO: make this a context manager
        self.visit(rhs, is_loop=is_loop, is_gather=is_gather)
        self.print("},")

    def visit_NamedItem(self, node: NamedItem) -> None:
        self.visit(node.item)

    def _get_rule_opcode(self, name: str) -> str:
        return f"R_{name.upper()}"

    def visit_NameLeaf(self, node: NameLeaf) -> None:
        name = node.value
        if name in ("NAME", "NUMBER", "STRING"):
            self.add_opcode(f"OP_{name}")
        elif name in ("NEWLINE", "DEDENT", "INDENT", "ENDMARKER", "ASYNC", "AWAIT"):
            self.add_opcode("OP_TOKEN")
            self.add_opcode(name)
        else:
            self.add_opcode("OP_RULE")
            self.add_opcode(self._get_rule_opcode(name))

    def visit_StringLeaf(self, node: StringLeaf) -> None:
        self.add_opcode("OP_TOKEN")
        tok_str = node.value.replace("'", "")
        tok_num = token.EXACT_TOKEN_TYPES[tok_str]
        tok_name = token.tok_name[tok_num]
        self.add_opcode(tok_name)

    def handle_loop_rhs(self, node: Rhs, opcodes_by_alt: Dict[int, List[str]]) -> None:
        with self.set_opcode_buffer(opcodes_by_alt[0]):
            self.add_opcode("OP_LOOP_START")
        self.handle_default_rhs(node, opcodes_by_alt, is_loop=True, is_gather=False)
        with self.set_opcode_buffer(opcodes_by_alt[len(node.alts)]):
            self.add_opcode("OP_LOOP_COLLECT")

    def handle_default_rhs(
        self,
        node: Rhs,
        opcodes_by_alt: Dict[int, List[str]],
        is_loop: bool = False,
        is_gather: bool = False,
    ) -> None:
        for index, alt in enumerate(node.alts):
            with self.set_opcode_buffer(opcodes_by_alt[index]):
                self.visit(alt, is_loop=False, is_gather=False)
                if alt.action:
                    self.add_opcode("OP_RETURN")
                    self.add_opcode(f"A_{self.current_rule.name.upper()}_{index}")
                if is_loop:
                    self.add_opcode("OP_LOOP_ITERATE")

    def visit_Rhs(self, node: Rhs, is_loop: bool = False, is_gather: bool = False) -> None:
        opcodes_by_alt: Dict[int, List[str]] = defaultdict(list)
        if is_loop:
            self.handle_loop_rhs(node, opcodes_by_alt)
        else:
            self.handle_default_rhs(node, opcodes_by_alt, is_loop=is_loop, is_gather=is_gather)
        *indexes, _ = accumulate(map(len, opcodes_by_alt.values()))
        indexes = [0, *indexes, -1]
        self.print(f" {{{', '.join(map(str, indexes))}}},")

        self.print(" {")
        with self.indent():
            for index, opcodes in opcodes_by_alt.items():
                self.print(", ".join(opcodes) + ",")
        self.print(" },")

    def visit_Alt(self, node: Alt, is_loop: bool, is_gather: bool) -> None:
        for item in node.items:
            self.visit(item)

    def visit_Repeat0(self, node: Repeat0) -> None:
        name = self.callmakervisitor.visit(node)
        self.add_opcode("OP_RULE")
        self.add_opcode(self._get_rule_opcode(name))


def main() -> None:
    grammar, parser, tokenizer = build_parser("./data/simple.gram", False, False)
    p = VMParserGenerator(grammar)
    p.generate("")


if __name__ == "__main__":
    main()
