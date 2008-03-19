"""Support code for test_*.py files"""
# Author: Collin Winter

# Python imports
import unittest
import sys
import os
import os.path
import re
from textwrap import dedent

#sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

# Local imports
from .. import pytree
from ..pgen2 import driver

test_dir = os.path.dirname(__file__)
proj_dir = os.path.normpath(os.path.join(test_dir, ".."))
grammar_path = os.path.join(test_dir, "..", "Grammar.txt")
grammar = driver.load_grammar(grammar_path)
driver = driver.Driver(grammar, convert=pytree.convert)

def parse_string(string):
    return driver.parse_string(reformat(string), debug=True)

# Python 2.3's TestSuite is not iter()-able
if sys.version_info < (2, 4):
    def TestSuite_iter(self):
        return iter(self._tests)
    unittest.TestSuite.__iter__ = TestSuite_iter

def run_all_tests(test_mod=None, tests=None):
    if tests is None:
        tests = unittest.TestLoader().loadTestsFromModule(test_mod)
    unittest.TextTestRunner(verbosity=2).run(tests)

def reformat(string):
    return dedent(string) + "\n\n"

def all_project_files():
    for dirpath, dirnames, filenames in os.walk(proj_dir):
        for filename in filenames:
            if filename.endswith(".py"):
                yield os.path.join(dirpath, filename)

TestCase = unittest.TestCase
