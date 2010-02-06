"""Tests that run all fixer modules over an input stream.

This has been broken out into its own test module because of its
running time.
"""
# Author: Collin Winter

# Python imports
import unittest

# Local imports
from lib2to3 import refactor
from . import support


class Test_all(support.TestCase):

    def setUp(self):
        self.refactor = support.get_refactorer()

    def test_all_project_files(self):
        for filepath in support.all_project_files():
            self.refactor.refactor_file(filepath)
