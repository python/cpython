# gh-144503: Test that the forkserver can start when the parent process has
# a very large sys.argv.  Prior to the fix, sys.argv was repr'd into the
# forkserver ``-c`` command string which could exceed the OS limit on the
# length of a single argv element (MAX_ARG_STRLEN on Linux, ~128 KiB),
# causing posix_spawn to fail and the parent to see a BrokenPipeError.

import multiprocessing
import sys

EXPECTED_LEN = 5002  # argv[0] + 5000 padding entries + sentinel


def fun():
    print(f"worker:{len(sys.argv)}:{sys.argv[-1]}")


if __name__ == "__main__":
    # Inflate sys.argv well past 128 KiB before the forkserver is started.
    sys.argv[1:] = ["x" * 50] * 5000 + ["sentinel"]
    assert len(sys.argv) == EXPECTED_LEN

    ctx = multiprocessing.get_context("forkserver")
    p = ctx.Process(target=fun)
    p.start()
    p.join()
    sys.exit(p.exitcode)
else:
    # This branch runs when the forkserver preloads this module as
    # __mp_main__; confirm the large argv was propagated intact.
    print(f"preload:{len(sys.argv)}:{sys.argv[-1]}")
