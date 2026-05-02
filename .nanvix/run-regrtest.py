#!/usr/bin/env python3
# run-regrtest.py - Run CPython regrtest inside a nanvixd guest
#
# Copyright(c) The Maintainers of Nanvix.
# Licensed under the MIT License.
#
# Guest-side test runner.  Reads the test module list from a file and
# invokes regrtest in-process.  No batching, no subprocess, no ramfs —
# all of that is the host's concern.
#
# Test list source (in priority order):
#   1. File at NANVIX_TEST_LIST_FILE (default test-list.txt),
#      one module name per line.  Blank lines and # comments are skipped.
#   2. Command-line arguments (fallback for ad-hoc runs).
#
# Options:
#   --tmpdir DIR           - set TMPDIR for this process (avoids /tmp collisions
#                            when multiple nanvixd instances run in parallel)
#
# Environment variables:
#   NANVIX_TEST_LIST_FILE  - path to the test list file (default: test-list.txt)
#   REGRTEST_TIMEOUT       - per-test timeout in seconds (default: 120)

import os
import sys


def main() -> int:
    # -- Parse --tmpdir (must happen before anything touches tempfile) ----------
    argv = sys.argv[1:]
    if len(argv) >= 2 and argv[0] == "--tmpdir":
        os.environ["TMPDIR"] = argv[1]
        argv = argv[2:]

    # -- Resolve test modules --------------------------------------------------
    list_file = os.environ.get("NANVIX_TEST_LIST_FILE", "test-list.txt")
    modules = None
    if argv:
        modules = argv
    else:
        try:
            with open(list_file) as f:
                modules = [
                    line.strip()
                    for line in f
                    if line.strip() and not line.strip().startswith("#")
                ]
        except FileNotFoundError:
            print(f"run-regrtest.py: Could not find list file at {list_file}")
            return 1

    if not modules:
        print("run-regrtest.py: no test modules specified", file=sys.stderr)
        return 1
    print(f"run-regrtest.py: Got modules: {modules}")

    # -- Build regrtest arguments ----------------------------------------------
    timeout = os.environ.get("REGRTEST_TIMEOUT", "120")
    args = [f"--timeout={timeout}"] + modules

    # -- Run -------------------------------------------------------------------
    sys.argv[1:] = args
    from test.libregrtest.main import main as regrtest_main  # type: ignore[import-not-found]

    try:
        regrtest_main()
    except SystemExit as e:
        if e.code is None:
            return 0
        return e.code if isinstance(e.code, int) else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
