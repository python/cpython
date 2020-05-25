#!/usr/bin/env python3

import argparse
import ast
import sys
import os
from time import time

import _peg_parser

try:
    import memory_profiler
except ModuleNotFoundError:
    print("Please run `make venv` to create a virtual environment and install"
          " all the dependencies, before running this script.")
    sys.exit(1)

sys.path.insert(0, os.getcwd())
from scripts.test_parse_directory import parse_directory

argparser = argparse.ArgumentParser(
    prog="benchmark", description="Reproduce the various pegen benchmarks"
)
argparser.add_argument(
    "--parser",
    action="store",
    choices=["pegen", "cpython"],
    default="pegen",
    help="Which parser to benchmark (default is pegen)",
)
argparser.add_argument(
    "--target",
    action="store",
    choices=["xxl", "stdlib"],
    default="xxl",
    help="Which target to use for the benchmark (default is xxl.py)",
)

subcommands = argparser.add_subparsers(title="Benchmarks", dest="subcommand")
command_compile = subcommands.add_parser(
    "compile", help="Benchmark parsing and compiling to bytecode"
)
command_parse = subcommands.add_parser("parse", help="Benchmark parsing and generating an ast.AST")


def benchmark(func):
    def wrapper(*args):
        times = list()
        for _ in range(3):
            start = time()
            result = func(*args)
            end = time()
            times.append(end - start)
        memory = memory_profiler.memory_usage((func, args))
        print(f"{func.__name__}")
        print(f"\tTime: {sum(times)/3:.3f} seconds on an average of 3 runs")
        print(f"\tMemory: {max(memory)} MiB on an average of 3 runs")
        return result

    return wrapper


@benchmark
def time_compile(source, parser):
    if parser == "cpython":
        return _peg_parser.compile_string(
            source,
            oldparser=True,
        )
    else:
        return _peg_parser.compile_string(source)


@benchmark
def time_parse(source, parser):
    if parser == "cpython":
        return _peg_parser.parse_string(source, oldparser=True)
    else:
        return _peg_parser.parse_string(source)


def run_benchmark_xxl(subcommand, parser, source):
    if subcommand == "compile":
        time_compile(source, parser)
    elif subcommand == "parse":
        time_parse(source, parser)


def run_benchmark_stdlib(subcommand, parser):
    for _ in range(3):
        parse_directory(
            "../../Lib",
            "../../Grammar/python.gram",
            "../../Grammar/Tokens",
            verbose=False,
            excluded_files=["*/bad*", "*/lib2to3/tests/data/*",],
            skip_actions=False,
            tree_arg=0,
            short=True,
            mode=2 if subcommand == "compile" else 1,
            parser=parser,
        )


def main():
    args = argparser.parse_args()
    subcommand = args.subcommand
    parser = args.parser
    target = args.target

    if subcommand is None:
        argparser.error("A benchmark to run is required")

    if target == "xxl":
        with open(os.path.join("data", "xxl.py"), "r") as f:
            source = f.read()
            run_benchmark_xxl(subcommand, parser, source)
    elif target == "stdlib":
        run_benchmark_stdlib(subcommand, parser)


if __name__ == "__main__":
    main()
