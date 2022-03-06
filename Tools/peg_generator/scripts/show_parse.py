#!/usr/bin/env python3.8

"""Show the parse tree for a given program, nicely formatted.

Example:

$ scripts/show_parse.py a+b
Module(
    body=[
        Expr(
            value=BinOp(
                left=Name(id="a", ctx=Load()), op=Add(), right=Name(id="b", ctx=Load())
            )
        )
    ],
    type_ignores=[],
)
$

Use -v to show line numbers and column offsets.

The formatting is done using black.  You can also import this module
and call one of its functions.
"""

import argparse
import ast
import difflib
import os
import sys
import tempfile

from typing import List

sys.path.insert(0, os.getcwd())
from pegen.ast_dump import ast_dump

parser = argparse.ArgumentParser()
parser.add_argument(
    "-d", "--diff", action="store_true", help="show diff between grammar and ast (requires -g)"
)
parser.add_argument("-g", "--grammar-file", help="grammar to use (default: use the ast module)")
parser.add_argument(
    "-m",
    "--multiline",
    action="store_true",
    help="concatenate program arguments using newline instead of space",
)
parser.add_argument("-v", "--verbose", action="store_true", help="show line/column numbers")
parser.add_argument("program", nargs="+", help="program to parse (will be concatenated)")


def format_tree(tree: ast.AST, verbose: bool = False) -> str:
    with tempfile.NamedTemporaryFile("w+") as tf:
        tf.write(ast_dump(tree, include_attributes=verbose))
        tf.write("\n")
        tf.flush()
        cmd = f"black -q {tf.name}"
        sts = os.system(cmd)
        if sts:
            raise RuntimeError(f"Command {cmd!r} failed with status 0x{sts:x}")
        tf.seek(0)
        return tf.read()


def diff_trees(a: ast.AST, b: ast.AST, verbose: bool = False) -> List[str]:
    sa = format_tree(a, verbose)
    sb = format_tree(b, verbose)
    la = sa.splitlines()
    lb = sb.splitlines()
    return list(difflib.unified_diff(la, lb, "a", "b", lineterm=""))


def show_parse(source: str, verbose: bool = False) -> str:
    tree = ast.parse(source)
    return format_tree(tree, verbose).rstrip("\n")


def print_parse(source: str, verbose: bool = False) -> None:
    print(show_parse(source, verbose))


def main() -> None:
    args = parser.parse_args()
    if args.diff and not args.grammar_file:
        parser.error("-d/--diff requires -g/--grammar-file")
    if args.multiline:
        sep = "\n"
    else:
        sep = " "
    program = sep.join(args.program)
    if args.grammar_file:
        sys.path.insert(0, os.curdir)
        from pegen.build import build_parser_and_generator

        build_parser_and_generator(args.grammar_file, "peg_parser/parse.c", compile_extension=True)
        from pegen.parse import parse_string  # type: ignore[import]

        tree = parse_string(program, mode=1)

        if args.diff:
            a = tree
            b = ast.parse(program)
            diff = diff_trees(a, b, args.verbose)
            if diff:
                for line in diff:
                    print(line)
            else:
                print("# Trees are the same")
        else:
            print(f"# Parsed using {args.grammar_file}")
            print(format_tree(tree, args.verbose))
    else:
        tree = ast.parse(program)
        print("# Parse using ast.parse()")
        print(format_tree(tree, args.verbose))


if __name__ == "__main__":
    main()
