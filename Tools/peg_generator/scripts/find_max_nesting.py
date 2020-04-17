#!/usr/bin/env python3.8
"""Find the maximum amount of nesting for an expression that can be parsed
without causing a parse error.

Starting at the INITIAL_NESTING_DEPTH, an expression containing n parenthesis
around a 0 is generated then tested with both the C and Python parsers. We
continue incrementing the number of parenthesis by 10 until both parsers have
failed. As soon as a single parser fails, we stop testing that parser.

The grammar file, initial nesting size, and amount by which the nested size is
incremented on each success can be controlled by changing the GRAMMAR_FILE,
INITIAL_NESTING_DEPTH, or NESTED_INCR_AMT variables.

Usage: python -m scripts.find_max_nesting
"""
import os
import sys
from tempfile import TemporaryDirectory
from pathlib import Path
from typing import Any

from _peg_parser import parse_string

GRAMMAR_FILE = "data/python.gram"
INITIAL_NESTING_DEPTH = 10
NESTED_INCR_AMT = 10


FAIL = "\033[91m"
ENDC = "\033[0m"


def check_nested_expr(nesting_depth: int) -> bool:
    expr = f"{'(' * nesting_depth}0{')' * nesting_depth}"

    try:
        parse_string(expr)
        print(f"Nesting depth of {nesting_depth} is successful")
        return True
    except Exception as err:
        print(f"{FAIL}(Failed with nesting depth of {nesting_depth}{ENDC}")
        print(f"{FAIL}\t{err}{ENDC}")
        return False


def main() -> None:
    print(f"Testing {GRAMMAR_FILE} starting at nesting depth of {INITIAL_NESTING_DEPTH}...")

    nesting_depth = INITIAL_NESTING_DEPTH
    succeeded = True
    while succeeded:
        expr = f"{'(' * nesting_depth}0{')' * nesting_depth}"
        if succeeded:
            succeeded = check_nested_expr(nesting_depth)
        nesting_depth += NESTED_INCR_AMT

    sys.exit(1)


if __name__ == "__main__":
    main()
