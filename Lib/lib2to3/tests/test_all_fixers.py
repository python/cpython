"""Tests that run all fixer modules over an input stream.

This has been broken out into its own test module because of its
running time.
"""
# Author: Collin Winter

# Python imports
import unittest
import test.support

# Local imports
from lib2to3 import refactor
from . import support


@test.support.requires_resource('cpu')
class Test_all(support.TestCase):

    def setUp(self):
        self.refactor = support.get_refactorer()

    def test_all_project_files(self):
        for filepath in support.all_project_files():
            self.refactor.refactor_file(filepath)

if __name__ == '__main__':
    unittest.main()
