from pegen.build import build_parser
import contextlib
import token
import sys

from typing import Any, Dict, Optional, IO, Text, Tuple

from pegen import grammar
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
        self,
        grammar: grammar.Grammar,
    ):
        super().__init__(grammar, token.tok_name, sys.stdout)

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
        self.print(f' R_{node.name.upper()},')
        self.print(' {TODO_INDEX_INTO_OPCODES_FOR_EVERY_ALT, -1},')
        self.print(' {')
        with self.indent():
            self.current_rule = node #TODO: make this a context manager
            self.visit(rhs, is_loop=is_loop, is_gather=is_gather)
        self.print('},')

    def visit_NamedItem(self, node: NamedItem) -> None:
        self.visit(node.item)
    
    def _get_rule_opcode(self, name: str) -> str:
        return f"R_{name.upper()}"
    
    def visit_NameLeaf(self, node: NameLeaf) -> Tuple[Optional[str], str]:
        name = node.value
        if name in ("NAME", "NUMBER", "STRING"):
            self.print(f"OP_{name}")
        elif name in ("NEWLINE", "DEDENT", "INDENT", "ENDMARKER", "ASYNC", "AWAIT"):
            self.print(f'OP_TOKEN, {name},')
        else:
            self.print(f'OP_RULE, {self._get_rule_opcode(name)},')

    def visit_StringLeaf(self, node: StringLeaf) -> Tuple[str, str]:
        tok_str = node.value.replace("'", "")
        tok_num = token.EXACT_TOKEN_TYPES[tok_str]
        tok_name = token.tok_name[tok_num]
        self.print(f"OP_TOKEN, {tok_name}")

    def visit_Rhs(self, node: Rhs, is_loop: bool = False, is_gather: bool = False) -> None:
        for index, alt in enumerate(node.alts):
            self.visit(alt, is_loop=is_loop, is_gather=is_gather)
            if alt.action:
                self.print(f"OP_RETURN, A_{self.current_rule.name.upper()}_{index},")
            self.print(f"")

    def visit_Alt(self, node: Alt, is_loop: bool, is_gather: bool) -> None:
        for item in node.items:
            self.visit(item)

GRAMMAR_FILE = ".data/simple.gram"
grammar, parser, tokenizer = build_parser("./data/simple.gram", False, False)
p = CParserGenerator(grammar)
p.generate("")