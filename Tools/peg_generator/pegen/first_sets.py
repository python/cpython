#!/usr/bin/env python3.8

import argparse
import pathlib
import pprint
import sys

from pegen.build import build_parser
from pegen.grammar_analysis import FirstSetVisitor

argparser = argparse.ArgumentParser(
    prog="calculate_first_sets",
    description="Calculate the first sets of a grammar",
)
argparser.add_argument("grammar_file", type=pathlib.Path, help="The grammar file")


def main() -> None:
    args = argparser.parse_args()

    try:
        grammar, parser, tokenizer = build_parser(args.grammar_file)
    except Exception as err:
        print("ERROR: Failed to parse grammar file", err, file=sys.stderr)
        sys.exit(1)

    first_sets = FirstSetVisitor(grammar.rules).first_sets
    pprint.pprint(first_sets)


if __name__ == "__main__":
    main()
