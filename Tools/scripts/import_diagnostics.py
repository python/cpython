#!/usr/bin/env python3
"""Miscellaneous diagnostics for the import system"""

import sys
import argparse
from pprint import pprint

def _dump_state(args):
    print(sys.version)
    for name in args.attributes:
        print("sys.{}:".format(name))
        pprint(getattr(sys, name))

def _add_dump_args(cmd):
    cmd.add_argument("attributes", metavar="ATTR", nargs="+",
                     help="sys module attribute to display")

COMMANDS = (
  ("dump", "Dump import state", _dump_state, _add_dump_args),
)

def _make_parser():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(title="Commands")
    for name, description, implementation, add_args in COMMANDS:
        cmd = sub.add_parser(name, help=description)
        cmd.set_defaults(command=implementation)
        add_args(cmd)
    return parser

def main(args):
    parser = _make_parser()
    args = parser.parse_args(args)
    return args.command(args)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
