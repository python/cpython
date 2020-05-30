import contextlib
import sys
import token
from collections import defaultdict
from itertools import accumulate
from typing import Dict, Iterator, List, Optional

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
    PositiveLookahead,
    Repeat0,
    Repeat1,
    Rhs,
    Rule,
    StringLeaf,
)
from pegen.parser_generator import ParserGenerator


class CParserGenerator(ParserGenerator, GrammarVisitor):
    def __init__(
        self, grammar: grammar.Grammar,
    ):
        super().__init__(grammar, token.tok_name, sys.stdout)

        self.opcode_buffer: Optional[List[str]] = None

    @contextlib.contextmanager
    def set_opcode_buffer(self, buffer: List[str]) -> Iterator[None]:
        self.opcode_buffer = buffer
        yield
        self.opcode_buffer = None

    def add_opcode(self, opcode: str) -> None:
        assert self.opcode_buffer is not None
        self.opcode_buffer.append(opcode)

    def generate(self, filename: str) -> None:
        while self.todo:
            self.print("static Rule all_rules[] = {")
            for rulename, rule in list(self.todo.items()):
                del self.todo[rulename]
                with self.indent():
                    self.visit(rule)
            self.print("}")

    def visit_Rule(self, node: Rule) -> None:
        is_loop = node.is_loop()
        is_gather = node.is_gather()
        rhs = node.flatten()
        if node.left_recursive:
            raise ValueError("No left recursiveness")
        self.print(f'{{"{node.name}",')
        self.print(f" R_{node.name.upper()},")
        self.current_rule = node  # TODO: make this a context manager
        with self.indent():
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

    def visit_Rhs(self, node: Rhs, is_loop: bool = False, is_gather: bool = False) -> None:
        opcodes_by_alt: Dict[int, List[str]] = defaultdict(list)
        for index, alt in enumerate(node.alts):
            with self.set_opcode_buffer(opcodes_by_alt[index]):
                self.visit(alt, is_loop=is_loop, is_gather=is_gather)
                if alt.action:
                    self.add_opcode("OP_RETURN")
                    self.add_opcode(f"A_{self.current_rule.name.upper()}_{index}")

        *indexes, _ = accumulate(map(len, opcodes_by_alt.values()))
        indexes = [0, *indexes, -1]
        self.print(f" {{{', '.join(map(str, indexes))}}},")

        self.print(" {")
        with self.indent():
            for index, opcodes in opcodes_by_alt.items():
                self.print(", ".join(opcodes) + ",")
                self.print()
        self.print(" },")

    def visit_Alt(self, node: Alt, is_loop: bool, is_gather: bool) -> None:
        for item in node.items:
            self.visit(item)


def main() -> None:
    grammar, parser, tokenizer = build_parser("./data/simple.gram", False, False)
    p = CParserGenerator(grammar)
    p.generate("")


if __name__ == "__main__":
    main()
