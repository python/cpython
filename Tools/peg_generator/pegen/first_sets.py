#!/usr/bin/env python3

import argparse
import pprint
import sys

from pegen.grammar import (
    Alt,
    Cut,
    Forced,
    Gather,
    Group,
    Lookahead,
    NamedItem,
    NameLeaf,
    Opt,
    Repeat0,
    Repeat1,
    Rhs,
    Rule,
    StringLeaf,
)

argparser = argparse.ArgumentParser(
    prog="calculate_first_sets",
    description="Calculate the first sets of a grammar",
)
argparser.add_argument("grammar_file", help="The grammar file")


class FirstSetCalculator:
    """Conservative FIRST sets, including "" for a possibly empty match.

    None means parsing can commit or run an action before consuming a token,
    so callers must not use that set to skip a parse attempt.
    """

    def __init__(self, rules: dict[str, Rule]) -> None:
        self.rules = rules
        self.first_sets: dict[str, set[str | None]] = {name: set() for name in rules}

    def calculate(self) -> dict[str, set[str | None]]:
        # Recursive rules can discover terminals through each other. Keep
        # propagating them until every rule's set has stopped growing.
        while True:
            changed = False
            for name, rule in self.rules.items():
                terminals = self.visit(rule.rhs)
                if not terminals <= self.first_sets[name]:
                    self.first_sets[name].update(terminals)
                    changed = True
            if not changed:
                return self.first_sets

    def visit(self, item: object) -> set[str | None]:
        match item:
            case Alt(items=items, action=action):
                result: set[str | None] = set()
                for other in items:
                    terminals = self.visit(other)
                    result.update(terminals - {""})
                    if "" not in terminals:
                        break
                else:
                    result.add("")
                    if action:
                        # An empty action can raise before a later token fails
                        # to match. Do not bypass it with token dispatch.
                        result.add(None)
                return result
            case Rhs(alts=alts):
                result = set()
                for alt in alts:
                    result.update(self.visit(alt))
                return result
            case Rule(name=name):
                return self.first_sets[name]
            case NameLeaf(value=name):
                return self.first_sets[name] if name in self.rules else {name}
            case StringLeaf(value=value):
                return {value}
            case NamedItem(item=node) | Group(rhs=node) | Repeat1(node=node):
                return self.visit(node)
            case Opt(node=node) | Repeat0(node=node):
                return self.visit(node) | {""}
            case Gather(node=node, separator=separator):
                terminals = self.visit(node)
                if "" in terminals:
                    return terminals | self.visit(separator)
                return terminals
            case Cut():
                return {"", None}
            case Forced(node=node):
                # A mismatch raises instead of trying the next alternative.
                return self.visit(node) | {None}
            case Lookahead():
                # Predicates may raise after consuming tokens internally, and
                # negative predicates cannot safely narrow the next FIRST set.
                return {"", None}
            case _:
                raise TypeError(f"Unexpected grammar node: {type(item).__name__}")


def main() -> None:
    # build imports c_generator, which imports FirstSetCalculator.
    from pegen.build import build_parser

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
