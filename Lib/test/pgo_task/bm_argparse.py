"""
Benchmark argparse programs with:
1) multiple subparsers, each with their own subcommands, and then parse a series of command-line arguments.
2) a large number of optional arguments, and then parse a series of command-line arguments.

Author: Savannah Ostrowski
"""

import argparse


def generate_arguments(i: int) -> list:
    arguments = ["input.txt", "output.txt"]
    for i in range(i):
        arguments.extend([f"--option{i}", f"value{i}"])
    return arguments


def bm_many_optionals() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="A version control system CLI")

    parser.add_argument("--version", action="version", version="1.0")

    subparsers = parser.add_subparsers(dest="command", required=True)

    add_parser = subparsers.add_parser("add", help="Add a file to the repository")
    add_parser.add_argument("files", nargs="+", help="List of files to add to staging")

    commit_parser = subparsers.add_parser(
        "commit", help="Commit changes to the repository"
    )
    commit_parser.add_argument("-m", "--message", required=True, help="Commit message")

    commit_group = commit_parser.add_mutually_exclusive_group(required=False)
    commit_group.add_argument(
        "--amend", action="store_true", help="Amend the last commit"
    )
    commit_group.add_argument(
        "--no-edit", action="store_true", help="Reuse the last commit message"
    )

    push_parser = subparsers.add_parser(
        "push", help="Push changes to remote repository"
    )

    network_group = push_parser.add_argument_group("Network options")
    network_group.add_argument("--dryrun", action="store_true", help="Simulate changes")
    network_group.add_argument(
        "--timeout", type=int, default=30, help="Timeout in seconds"
    )

    auth_group = push_parser.add_argument_group("Authentication options")
    auth_group.add_argument(
        "--username", required=True, help="Username for authentication"
    )
    auth_group.add_argument(
        "--password", required=True, help="Password for authentication"
    )

    global_group = parser.add_mutually_exclusive_group()
    global_group.add_argument("--verbose", action="store_true", help="Verbose output")
    global_group.add_argument("--quiet", action="store_true", help="Quiet output")

    argument_lists = [
        ["--verbose", "add", "file1.txt", "file2.txt"],
        ["add", "file1.txt", "file2.txt"],
        ["commit", "-m", "Initial commit"],
        ["commit", "-m", "Add new feature", "--amend"],
        [
            "push",
            "--dryrun",
            "--timeout",
            "60",
            "--username",
            "user",
            "--password",
            "pass",
        ],
    ]

    for arguments in argument_lists:
        parser.parse_args(arguments)


def bm_subparsers() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()

    parser.add_argument("input_file", type=str, help="The input file")
    parser.add_argument("output_file", type=str, help="The output file")

    for i in range(1000):
        parser.add_argument(f"--option{i}", type=str, help=f"Optional argument {i}")

    argument_lists = [
        generate_arguments(500),
        generate_arguments(1000),
    ]

    for args in argument_lists:
        parser.parse_args(args)


BENCHMARKS = {
    "many_optionals": bm_many_optionals,
    "subparsers": bm_subparsers,
}


def add_cmdline_args(cmd, args):
    cmd.append(args.benchmark)


def add_parser_args(parser):
    parser.add_argument("benchmark", choices=BENCHMARKS, help="Which benchmark to run.")


def run_pgo():
    for bm_func in BENCHMARKS.values():
        for _ in range(10):
            bm_func()
