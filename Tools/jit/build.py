"""Build an experimental just-in-time compiler for CPython."""

import argparse
import pathlib
import sys

import _targets

if sys.version_info < (3, 11):
    raise RuntimeError("Building the JIT compiler requires Python 3.11 or newer!")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", help="a PEP 11 target triple to compile for")
    parser.add_argument(
        "-d", "--debug", action="store_true", help="compile for a debug build of Python"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="echo commands as they are run"
    )
    args = parser.parse_args()
    target = _targets.get_target(args.target, debug=args.debug, verbose=args.verbose)
    target.build(pathlib.Path.cwd())
