#!/usr/bin/env python3

import argparse
import ast
import sys
import os
from time import time

try:
    import memory_profiler
except ModuleNotFoundError:
    print(
        "Please run `make venv` to create a virtual environment and install"
        " all the dependencies, before running this script."
    )
    sys.exit(1)

sys.path.insert(0, os.getcwd())
from scripts.test_parse_directory import parse_directory

argparser = argparse.ArgumentParser(
    prog="benchmark", description="Reproduce the various pegen benchmarks"
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
def time_compile(source):
    return compile(source, "<string>", "exec")


@benchmark
def time_parse(source):
    return ast.parse(source)


def run_benchmark_xxl(subcommand, source):
    if subcommand == "compile":
        time_compile(source)
    elif subcommand == "parse":
        time_parse(source)


def run_benchmark_stdlib(subcommand):
    modes = {"compile": 2, "parse": 1}
    for _ in range(3):
        parse_directory(
            "../../Lib",
            verbose=False,
            excluded_files=["*/bad*", "*/lib2to3/tests/data/*",],
            short=True,
            mode=modes[subcommand],
        )


def main():
    args = argparser.parse_args()
    subcommand = args.subcommand
    target = args.target

    if subcommand is None:
        argparser.error("A benchmark to run is required")

    if target == "xxl":
        with open(os.path.join("data", "xxl.py"), "r") as f:
            source = f.read()
            run_benchmark_xxl(subcommand, source)
    elif target == "stdlib":
        run_benchmark_stdlib(subcommand)


if __name__ == "__main__":
    main()
