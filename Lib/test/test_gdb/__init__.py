# Verify that gdb can pretty-print the various PyObject* types
#
# The code for testing gdb was adapted from similar work in Unladen Swallow's
# Lib/test/test_jit_gdb.py

import os
import sysconfig
import unittest
from test import support


if support.MS_WINDOWS:
    # On Windows, Python is usually built by MSVC. Passing /p:DebugSymbols=true
    # option to MSBuild produces PDB debug symbols, but gdb doesn't support PDB
    # debug symbol files.
    raise unittest.SkipTest("test_gdb doesn't work on Windows")

if support.PGO:
    raise unittest.SkipTest("test_gdb is not useful for PGO")

if not sysconfig.is_python_build():
    raise unittest.SkipTest("test_gdb only works on source builds at the moment.")

if support.check_cflags_pgo():
    raise unittest.SkipTest("test_gdb is not reliable on PGO builds")


def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
