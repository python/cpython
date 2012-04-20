"""Run Python's standard test suite using importlib.__import__.

Tests known to fail because of assumptions that importlib (properly)
invalidates are automatically skipped if the entire test suite is run.
Otherwise all command-line options valid for test.regrtest are also valid for
this script.

"""
import importlib
import sys
from test import regrtest

if __name__ == '__main__':
    __builtins__.__import__ = importlib.__import__
    sys.path_importer_cache.clear()

    regrtest.main(quiet=True, verbose2=True)
