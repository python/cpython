# This script is best run with pystats enabled to help visualize the shape of the traces.
# ./configure --enable-experimental-jit=interpreter  -C --with-pydebug --enable-pystats

# The resulting images can be visualize on linux as follows:
# $ cd folder_with_gv_files
# $ dot -Tsvg -Osvg *.gv
# $ firefox *.gv.svg

# type: ignore

import sys
import os.path
from types import FunctionType

# All functions declared in this module will be run to generate
# a .gv file of the executors, unless the name starts with an underscore.


def _gen(n):
    for _ in range(n):
        yield n


def gen_in_loop(n):
    t = 0
    for n in _gen(n):
        t += n
    return n


def short_loop(n):
    t = 0
    for _ in range(n):
        t += 1
        t += 1
        t += 1
        t += 1
        t += 1
    return t


exec(
    "\n".join(
        ["def mid_loop(n):"]
        + ["    t = 0"]
        + ["    for _ in range(n):"]
        + ["        t += 1"] * 20
        + ["    return t"]
    ),
    globals(),
)

exec(
    "\n".join(
        ["def long_loop(n):"]
        + ["    t = 0"]
        + ["    for _ in range(n):"]
        + ["        t += 1"] * 100
        + ["    return t"]
    ),
    globals(),
)


def _add(a, b):
    return a + b


def short_loop_with_calls(n):
    t = 0
    for _ in range(n):
        t = _add(t, 1)
        t = _add(t, 1)
        t = _add(t, 1)
        t = _add(t, 1)
        t = _add(t, 1)
    return t


exec(
    "\n".join(
        ["def mid_loop_with_calls(n):"]
        + ["    t = 0"]
        + ["    for _ in range(n):"]
        + ["        t = _add(t, 1)"] * 20
        + ["    return t"]
    ),
    globals(),
)

exec(
    "\n".join(
        ["def long_loop_with_calls(n):"]
        + ["    t = 0"]
        + ["    for _ in range(n):"]
        + ["        t = _add(t, 1)"] * 100
        + ["    return t"]
    ),
    globals(),
)


def short_loop_with_side_exits(n):
    t = 0
    for i in range(n):
        if t < 0:
            break
        t += 1
        if t < 0:
            break
        t += 1
        if t < 0:
            break
        t += 1
        if t < 0:
            break
        t += 1
        if t < 0:
            break
        t += 1
    return t


exec(
    "\n".join(
        ["def mid_loop_with_side_exits(n):"]
        + ["    t = 0"]
        + ["    for _ in range(n):"]
        + ["        if t < 0:", "            break", "        t += 1"] * 20
        + ["    return t"]
    ),
    globals(),
)

exec(
    "\n".join(
        ["def long_loop_with_side_exits(n):"]
        + ["    t = 0"]
        + ["    for _ in range(n):"]
        + ["        if t < 0:", "            break", "        t += 1"] * 100
        + ["    return t"]
    ),
    globals(),
)


def short_branchy_loop(n):
    # Branches are correlated and exit 1 time in 4.
    t = 0
    for i in range(n):
        # Start with a few operations to form a viable trace
        t += 1
        t += 1
        t += 1
        if not t & 6:
            continue
        t += 1
        if not t & 12:
            continue
        t += 1
        if not t & 24:
            continue
        t += 1
        if not t & 48:
            continue
        t += 1
    return t


def _run_and_dump(func, n, outdir):
    sys._clear_internal_caches()
    func(n)
    sys._dump_tracelets(os.path.join(outdir, f"{func.__name__}.gv"))


def _main():
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print(f"Usage: {sys.argv[0] if sys.argv else " "} OUTDIR [loops]")
    outdir = sys.argv[1]
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 5000
    functions = [
        func
        for func in globals().values()
        if isinstance(func, FunctionType) and not func.__name__.startswith("_")
    ]
    for func in functions:
        _run_and_dump(func, n, outdir)


if __name__ == "__main__":
    _main()
