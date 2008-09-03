#!/usr/bin/env python2.5
"""Tests that run all fixer modules over an input stream.

This has been broken out into its own test module because of its
running time.
"""
# Author: Collin Winter

# Testing imports
try:
    from . import support
except ImportError:
    import support

# Python imports
import unittest

# Local imports
from .. import pytree
from .. import refactor

class Test_all(support.TestCase):
    def setUp(self):
        options = {"print_function" : False}
        self.refactor = support.get_refactorer(options=options)

    def test_all_project_files(self):
        for filepath in support.all_project_files():
            print "Fixing %s..." % filepath
            self.refactor.refactor_string(open(filepath).read(), filepath)


if __name__ == "__main__":
    import __main__
    support.run_all_tests(__main__)
