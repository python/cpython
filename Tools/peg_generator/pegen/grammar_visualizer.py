import argparse
import sys
from typing import Any, Callable, Iterator

from pegen.build import build_parser
from pegen.grammar import Grammar, Rule

argparser = argparse.ArgumentParser(
    prog="pegen", description="Pretty print the AST for a given PEG grammar"
)
argparser.add_argument("filename", help="Grammar description")


class ASTGrammarPrinter:
    def children(self, node: Rule) -> Iterator[Any]:
        for value in node:
            if isinstance(value, list):
                yield from value
            else:
                yield value

    def name(self, node: Rule) -> str:
        if not list(self.children(node)):
            return repr(node)
        return node.__class__.__name__

    def print_grammar_ast(self, grammar: Grammar, printer: Callable[..., None] = print) -> None:
        for rule in grammar.rules.values():
            printer(self.print_nodes_recursively(rule))

    def print_nodes_recursively(self, node: Rule, prefix: str = "", istail: bool = True) -> str:
        children = list(self.children(node))
        value = self.name(node)

        line = prefix + ("└──" if istail else "├──") + value + "\n"
        sufix = "   " if istail else "│  "

        if not children:
            return line

        *children, last = children
        for child in children:
            line += self.print_nodes_recursively(child, prefix + sufix, False)
        line += self.print_nodes_recursively(last, prefix + sufix, True)

        return line


def main() -> None:
    args = argparser.parse_args()

    try:
        grammar, parser, tokenizer = build_parser(args.filename)
    except Exception as err:
        print("ERROR: Failed to parse grammar file", file=sys.stderr)
        sys.exit(1)

    visitor = ASTGrammarPrinter()
    visitor.print_grammar_ast(grammar)


if __name__ == "__main__":
    main()
