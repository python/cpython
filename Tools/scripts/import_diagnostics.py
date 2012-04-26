#!/usr/bin/env python3
"""Miscellaneous diagnostics for the import system"""

import sys
import argparse
from pprint import pprint

def _dump_state(args):
    print(sys.version)
    print("sys.path:")
    pprint(sys.path)
    print("sys.meta_path")
    pprint(sys.meta_path)
    print("sys.path_hooks")
    pprint(sys.path_hooks)
    print("sys.path_importer_cache")
    pprint(sys.path_importer_cache)
    print("sys.modules:")
    pprint(sys.modules)

COMMANDS = (
  ("dump", "Dump import state", _dump_state),
)

def _make_parser():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(title="Commands")
    for name, description, implementation in COMMANDS:
        cmd = sub.add_parser(name, help=description)
        cmd.set_defaults(command=implementation)
    return parser

def main(args):
    parser = _make_parser()
    args = parser.parse_args(args)
    return args.command(args)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
