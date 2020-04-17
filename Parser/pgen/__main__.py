import argparse

from .pgen import ParserGenerator


def main():
    parser = argparse.ArgumentParser(description="Parser generator main program.")
    parser.add_argument(
        "grammar", type=str, help="The file with the grammar definition in EBNF format"
    )
    parser.add_argument("tokens", type=str, help="The file with the token definitions")
    parser.add_argument(
        "graminit_h",
        type=argparse.FileType("w"),
        help="The path to write the grammar's non-terminals as #defines",
    )
    parser.add_argument(
        "graminit_c",
        type=argparse.FileType("w"),
        help="The path to write the grammar as initialized data",
    )

    parser.add_argument("--verbose", "-v", action="count")
    parser.add_argument(
        "--graph",
        type=argparse.FileType("w"),
        action="store",
        metavar="GRAPH_OUTPUT_FILE",
        help="Dumps a DOT representation of the generated automata in a file",
    )

    args = parser.parse_args()

    p = ParserGenerator(
        args.grammar, args.tokens, verbose=args.verbose, graph_file=args.graph
    )
    grammar = p.make_grammar()
    grammar.produce_graminit_h(args.graminit_h.write)
    grammar.produce_graminit_c(args.graminit_c.write)


if __name__ == "__main__":
    main()
