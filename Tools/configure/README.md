# configure.py, a replacement for configure.ac

CPython's build configuration is driven by `configure.ac` (8,344 lines, ~271
KB), which uses autoconf/M4 macros to generate a 946 KB shell script
(`configure`). This system is:
- Opaque and hard to read/maintain (M4 macro language)
- Inefficient (generates a giant shell script, then runs it)
- Difficult to extend or debug

This replaces it with a Python-based build configuration system where
`Tools/configure/configure.py` is a real Python script that imports a `pyconf`
module.  The `pyconf` functions generally match the autoconf behaviour, so that
translation from the configure.ac file is mostly direct and mechanical.

There is also a transpiler that converts configure.py into POSIX AWK (wrapped
in a small shell stub).  The transpiler lives in `Tools/configure/transpiler/`
and the pipeline is: Python AST → pysh_ast → awk_ast → AWK text.  The sh and
AWK code needs to be compatible with those tools on various Unix-like operating
systems.

Note that configure.py needs to be cross platform.  Implementation that only
only works on the current OS is *not* okay.  This configure script will run on
many different Unix like platforms: Linux, FreeBSD, OpenBSD, NetBSD, MacOS,
AIX.  Avoid "baking in" Linux specific behaviour that is going to break on
other platforms.

We compare output files, produced by the script, in order to ensure we
faithfully re-implement the configure.ac script. When comparing output files,
like Makefile, pyconfig.h, we generally don't care about whitespace changes.
So `diff -u -bw` could be used to compare.

You can run the new configure.py script by running `./configure`.  The autoconf
script has been renamed to `./configure-old`.

When making code changes, you should use `ruff check Tools/configure/*.py` and
`ruff format Tools/configure/*.py` to ensure code is formatted and lint clean.

There are unit tests for the pyconf module, test_pyconf.py.  You can run these
tests with `uv run <file>`.  There are also unit tests for the transpiler, in
test_transpile.py.  To compare the output of the transpiled version, run
`compare-transpile.sh`. You can compare outputs of the two configure
implementations by running `./compare-conf.sh`.
