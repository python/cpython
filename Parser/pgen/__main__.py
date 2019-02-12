import os
import sys
import argparse
import collections

from lib2to3.pgen2 import grammar, tokenize

from . import token
from . import grammar as pgen_grammar

def monkey_patch_pgen2(token_lines):
    tokens = dict(token.generate_tokens(token_lines))
    for name, value in tokens.items():
        setattr(tokenize, name, value)

from .pgen import ParserGenerator


def main(grammar_file, tokens_file, gramminit_h_file, gramminit_c_file, verbose):
    with open(tokens_file) as tok_file:
        token_lines = tok_file.readlines()

    monkey_patch_pgen2(token_lines)

    p = ParserGenerator(grammar_file, token_lines, verbose=verbose)
    grammar = p.make_grammar()
    grammar.produce_graminit_h(gramminit_h_file.write)
    grammar.produce_graminit_c(gramminit_c_file.write)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Parser generator main program.")
    parser.add_argument(
        "grammar", type=str, help="The file with the grammar definition in EBNF format"
    )
    parser.add_argument(
        "tokens", type=str, help="The file with the token definition"
    )
    parser.add_argument(
        "gramminit_h",
        type=argparse.FileType('w'),
        help="The path to write the grammar's non-terminals as #defines",
    )
    parser.add_argument(
        "gramminit_c",
        type=argparse.FileType('w'),
        help="The path to write the grammar as initialized data",
    )
    parser.add_argument("--verbose", "-v", action="count")
    args = parser.parse_args()
    main(args.grammar, args.tokens, args.gramminit_h, args.gramminit_c, args.verbose)
