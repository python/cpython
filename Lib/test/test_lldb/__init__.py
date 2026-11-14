"""Tests for the CPython LLDB helpers."""

import os
import sysconfig
import unittest
from test import support


if support.MS_WINDOWS:
    raise unittest.SkipTest(
        'test_lldb requires a Unix build with debug symbols'
    )
if support.PGO or support.check_cflags_pgo():
    raise unittest.SkipTest('test_lldb is not useful for PGO')
if support.check_bolt_optimized():
    raise unittest.SkipTest(
        'test_lldb is not reliable on BOLT optimized builds'
    )
if not sysconfig.is_python_build():
    raise unittest.SkipTest('test_lldb requires a source build')


def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
