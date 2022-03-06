#!/usr/bin/env python3.8

"""pegen -- PEG Generator.

Search the web for PEG Parsers for reference.
"""

import argparse
import sys
import time
import token
import traceback

from typing import Tuple

from pegen.build import Grammar, Parser, Tokenizer, ParserGenerator
from pegen.validator import validate_grammar


def generate_c_code(
    args: argparse.Namespace,
) -> Tuple[Grammar, Parser, Tokenizer, ParserGenerator]:
    from pegen.build import build_c_parser_and_generator

    verbose = args.verbose
    verbose_tokenizer = verbose >= 3
    verbose_parser = verbose == 2 or verbose >= 4
    try:
        grammar, parser, tokenizer, gen = build_c_parser_and_generator(
            args.grammar_filename,
            args.tokens_filename,
            args.output,
            args.compile_extension,
            verbose_tokenizer,
            verbose_parser,
            args.verbose,
            keep_asserts_in_extension=False if args.optimized else True,
            skip_actions=args.skip_actions,
        )
        return grammar, parser, tokenizer, gen
    except Exception as err:
        if args.verbose:
            raise  # Show traceback
        traceback.print_exception(err.__class__, err, None)
        sys.stderr.write("For full traceback, use -v\n")
        sys.exit(1)


def generate_python_code(
    args: argparse.Namespace,
) -> Tuple[Grammar, Parser, Tokenizer, ParserGenerator]:
    from pegen.build import build_python_parser_and_generator

    verbose = args.verbose
    verbose_tokenizer = verbose >= 3
    verbose_parser = verbose == 2 or verbose >= 4
    try:
        grammar, parser, tokenizer, gen = build_python_parser_and_generator(
            args.grammar_filename,
            args.output,
            verbose_tokenizer,
            verbose_parser,
            skip_actions=args.skip_actions,
        )
        return grammar, parser, tokenizer, gen
    except Exception as err:
        if args.verbose:
            raise  # Show traceback
        traceback.print_exception(err.__class__, err, None)
        sys.stderr.write("For full traceback, use -v\n")
        sys.exit(1)


argparser = argparse.ArgumentParser(
    prog="pegen", description="Experimental PEG-like parser generator"
)
argparser.add_argument("-q", "--quiet", action="store_true", help="Don't print the parsed grammar")
argparser.add_argument(
    "-v",
    "--verbose",
    action="count",
    default=0,
    help="Print timing stats; repeat for more debug output",
)
subparsers = argparser.add_subparsers(help="target language for the generated code")

c_parser = subparsers.add_parser("c", help="Generate C code for inclusion into CPython")
c_parser.set_defaults(func=generate_c_code)
c_parser.add_argument("grammar_filename", help="Grammar description")
c_parser.add_argument("tokens_filename", help="Tokens description")
c_parser.add_argument(
    "-o", "--output", metavar="OUT", default="parse.c", help="Where to write the generated parser"
)
c_parser.add_argument(
    "--compile-extension",
    action="store_true",
    help="Compile generated C code into an extension module",
)
c_parser.add_argument(
    "--optimized", action="store_true", help="Compile the extension in optimized mode"
)
c_parser.add_argument(
    "--skip-actions", action="store_true", help="Suppress code emission for rule actions",
)

python_parser = subparsers.add_parser("python", help="Generate Python code")
python_parser.set_defaults(func=generate_python_code)
python_parser.add_argument("grammar_filename", help="Grammar description")
python_parser.add_argument(
    "-o",
    "--output",
    metavar="OUT",
    default="parse.py",
    help="Where to write the generated parser",
)
python_parser.add_argument(
    "--skip-actions", action="store_true", help="Suppress code emission for rule actions",
)


def main() -> None:
    from pegen.testutil import print_memstats

    args = argparser.parse_args()
    if "func" not in args:
        argparser.error("Must specify the target language mode ('c' or 'python')")

    t0 = time.time()
    grammar, parser, tokenizer, gen = args.func(args)
    t1 = time.time()

    validate_grammar(grammar)

    if not args.quiet:
        if args.verbose:
            print("Raw Grammar:")
            for line in repr(grammar).splitlines():
                print(" ", line)

        print("Clean Grammar:")
        for line in str(grammar).splitlines():
            print(" ", line)

    if args.verbose:
        print("First Graph:")
        for src, dsts in gen.first_graph.items():
            print(f"  {src} -> {', '.join(dsts)}")
        print("First SCCS:")
        for scc in gen.first_sccs:
            print(" ", scc, end="")
            if len(scc) > 1:
                print(
                    "  # Indirectly left-recursive; leaders:",
                    {name for name in scc if grammar.rules[name].leader},
                )
            else:
                name = next(iter(scc))
                if name in gen.first_graph[name]:
                    print("  # Left-recursive")
                else:
                    print()

    if args.verbose:
        dt = t1 - t0
        diag = tokenizer.diagnose()
        nlines = diag.end[0]
        if diag.type == token.ENDMARKER:
            nlines -= 1
        print(f"Total time: {dt:.3f} sec; {nlines} lines", end="")
        if dt:
            print(f"; {nlines / dt:.0f} lines/sec")
        else:
            print()
        print("Caches sizes:")
        print(f"  token array : {len(tokenizer._tokens):10}")
        print(f"        cache : {len(parser._cache):10}")
        if not print_memstats():
            print("(Can't find psutil; install it for memory stats.)")


if __name__ == "__main__":
    if sys.version_info < (3, 8):
        print("ERROR: using pegen requires at least Python 3.8!", file=sys.stderr)
        sys.exit(1)
    main()
