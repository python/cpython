#!/usr/bin/env python3.8

import argparse
import pprint
import sys

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
    Repeat0,
    Repeat1,
    Rhs,
    Rule,
    StringLeaf,
)
from pegen.parser_generator import compute_nullables

argparser = argparse.ArgumentParser(
    prog="calculate_first_sets",
    description="Calculate the first sets of a grammar",
)
argparser.add_argument("grammar_file", help="The grammar file")


class FirstSetCalculator(GrammarVisitor):
    def __init__(self, rules: dict[str, Rule]) -> None:
        self.rules = rules
        self.nullables = compute_nullables(rules)
        self.first_sets: dict[str, set[str]] = dict()
        self.in_process: set[str] = set()

    def calculate(self) -> dict[str, set[str]]:
        for name, rule in self.rules.items():
            self.visit(rule)
        return self.first_sets

    def visit_Alt(self, item: Alt) -> set[str]:
        result: set[str] = set()
        to_remove: set[str] = set()
        for other in item.items:
            new_terminals = self.visit(other)
            if isinstance(other.item, NegativeLookahead):
                to_remove |= new_terminals
            result |= new_terminals
            if to_remove:
                result -= to_remove

            # If the set of new terminals can start with the empty string,
            # it means that the item is completely nullable and we should
            # also considering at least the next item in case the current
            # one fails to parse.

            if "" in new_terminals:
                continue

            if not isinstance(other.item, (Opt, NegativeLookahead, Repeat0)):
                break

        # Do not allow the empty string to propagate.
        result.discard("")

        return result

    def visit_Cut(self, item: Cut) -> set[str]:
        return set()

    def visit_Group(self, item: Group) -> set[str]:
        return self.visit(item.rhs)

    def visit_PositiveLookahead(self, item: Lookahead) -> set[str]:
        return self.visit(item.node)

    def visit_NegativeLookahead(self, item: NegativeLookahead) -> set[str]:
        return self.visit(item.node)

    def visit_NamedItem(self, item: NamedItem) -> set[str]:
        return self.visit(item.item)

    def visit_Opt(self, item: Opt) -> set[str]:
        return self.visit(item.node)

    def visit_Gather(self, item: Gather) -> set[str]:
        return self.visit(item.node)

    def visit_Repeat0(self, item: Repeat0) -> set[str]:
        return self.visit(item.node)

    def visit_Repeat1(self, item: Repeat1) -> set[str]:
        return self.visit(item.node)

    def visit_NameLeaf(self, item: NameLeaf) -> set[str]:
        if item.value not in self.rules:
            return {item.value}

        if item.value not in self.first_sets:
            self.first_sets[item.value] = self.visit(self.rules[item.value])
            return self.first_sets[item.value]
        elif item.value in self.in_process:
            return set()

        return self.first_sets[item.value]

    def visit_StringLeaf(self, item: StringLeaf) -> set[str]:
        return {item.value}

    def visit_Rhs(self, item: Rhs) -> set[str]:
        result: set[str] = set()
        for alt in item.alts:
            result |= self.visit(alt)
        return result

    def visit_Rule(self, item: Rule) -> set[str]:
        if item.name in self.in_process:
            return set()
        elif item.name not in self.first_sets:
            self.in_process.add(item.name)
            terminals = self.visit(item.rhs)
            if item in self.nullables:
                terminals.add("")
            self.first_sets[item.name] = terminals
            self.in_process.remove(item.name)
        return self.first_sets[item.name]


def main() -> None:
    args = argparser.parse_args()

    try:
        grammar, parser, tokenizer = build_parser(args.grammar_file)
    except Exception as err:
        print("ERROR: Failed to parse grammar file", err, file=sys.stderr)
        sys.exit(1)

    firs_sets = FirstSetCalculator(grammar.rules).calculate()
    pprint.pprint(firs_sets)


if __name__ == "__main__":
    main()
