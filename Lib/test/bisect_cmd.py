#!/usr/bin/env python3
"""
Command line tool to bisect failing CPython tests.

Find the test_os test method which alters the environment:

    ./python -m test.bisect_cmd --fail-env-changed test_os

Find a reference leak in "test_os", write the list of failing tests into the
"bisect" file:

    ./python -m test.bisect_cmd -o bisect -R 3:3 test_os

Load an existing list of tests from a file using -i option:

    ./python -m test --list-cases -m FileTests test_os > tests
    ./python -m test.bisect_cmd -i tests test_os
"""

import argparse
import datetime
import os.path
import math
import random
import subprocess
import sys
import tempfile
import time


def write_tests(filename, tests):
    with open(filename, "w") as fp:
        for name in tests:
            print(name, file=fp)
        fp.flush()


def write_output(filename, tests):
    if not filename:
        return
    print("Writing %s tests into %s" % (len(tests), filename))
    write_tests(filename, tests)
    return filename


def format_shell_args(args):
    return ' '.join(args)


def python_cmd():
    cmd = [sys.executable]
    cmd.extend(subprocess._args_from_interpreter_flags())
    cmd.extend(subprocess._optim_args_from_interpreter_flags())
    cmd.extend(('-X', 'faulthandler'))
    return cmd


def list_cases(args):
    cmd = python_cmd()
    cmd.extend(['-m', 'test', '--list-cases'])
    cmd.extend(args.test_args)
    proc = subprocess.run(cmd,
                          stdout=subprocess.PIPE,
                          universal_newlines=True)
    exitcode = proc.returncode
    if exitcode:
        cmd = format_shell_args(cmd)
        print("Failed to list tests: %s failed with exit code %s"
              % (cmd, exitcode))
        sys.exit(exitcode)
    tests = proc.stdout.splitlines()
    return tests


def run_tests(args, tests, huntrleaks=None):
    tmp = tempfile.mktemp()
    try:
        write_tests(tmp, tests)

        cmd = python_cmd()
        cmd.extend(['-u', '-m', 'test', '--matchfile', tmp])
        cmd.extend(args.test_args)
        print("+ %s" % format_shell_args(cmd))

        sys.stdout.flush()
        sys.stderr.flush()

        proc = subprocess.run(cmd)
        return proc.returncode
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input',
                        help='Test names produced by --list-tests written '
                             'into a file. If not set, run --list-tests')
    parser.add_argument('-o', '--output',
                        help='Result of the bisection')
    parser.add_argument('-n', '--max-tests', type=int, default=1,
                        help='Maximum number of tests to stop the bisection '
                             '(default: 1)')
    parser.add_argument('-N', '--max-iter', type=int, default=100,
                        help='Maximum number of bisection iterations '
                             '(default: 100)')
    # FIXME: document that following arguments are test arguments

    args, test_args = parser.parse_known_args()
    args.test_args = test_args
    return args


def main():
    args = parse_args()
    for opt in ('-w', '--rerun', '--verbose2'):
        if opt in args.test_args:
            print(f"WARNING: {opt} option should not be used to bisect!")
            print()

    if args.input:
        with open(args.input) as fp:
            tests = [line.strip() for line in fp]
    else:
        tests = list_cases(args)

    print("Start bisection with %s tests" % len(tests))
    print("Test arguments: %s" % format_shell_args(args.test_args))
    print("Bisection will stop when getting %s or less tests "
          "(-n/--max-tests option), or after %s iterations "
          "(-N/--max-iter option)"
          % (args.max_tests, args.max_iter))
    output = write_output(args.output, tests)
    print()

    start_time = time.monotonic()
    iteration = 1
    try:
        while len(tests) > args.max_tests and iteration <= args.max_iter:
            ntest = len(tests)
            ntest = max(ntest // 2, 1)
            subtests = random.sample(tests, ntest)

            print(f"[+] Iteration {iteration}/{args.max_iter}: "
                  f"run {len(subtests)} tests/{len(tests)}")
            print()

            exitcode = run_tests(args, subtests)

            print("ran %s tests/%s" % (ntest, len(tests)))
            print("exit", exitcode)
            if exitcode:
                print("Tests failed: continuing with this subtest")
                tests = subtests
                output = write_output(args.output, tests)
            else:
                print("Tests succeeded: skipping this subtest, trying a new subset")
            print()
            iteration += 1
    except KeyboardInterrupt:
        print()
        print("Bisection interrupted!")
        print()

    print("Tests (%s):" % len(tests))
    for test in tests:
        print("* %s" % test)
    print()

    if output:
        print("Output written into %s" % output)

    dt = math.ceil(time.monotonic() - start_time)
    if len(tests) <= args.max_tests:
        print("Bisection completed in %s iterations and %s"
              % (iteration, datetime.timedelta(seconds=dt)))
    else:
        print("Bisection failed after %s iterations and %s"
              % (iteration, datetime.timedelta(seconds=dt)))
        sys.exit(1)


if __name__ == "__main__":
    main()
