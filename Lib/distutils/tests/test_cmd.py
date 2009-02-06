"""Tests for distutils.cmd."""
import unittest

from distutils.cmd import Command
from distutils.dist import Distribution
from distutils.errors import DistutilsOptionError

class CommandTestCase(unittest.TestCase):

    def test_ensure_string_list(self):

        class MyCmd(Command):

            def initialize_options(self):
                pass

        dist = Distribution()
        cmd = MyCmd(dist)

        cmd.not_string_list = ['one', 2, 'three']
        cmd.yes_string_list = ['one', 'two', 'three']
        cmd.not_string_list2 = object()
        cmd.yes_string_list2 = 'ok'

        cmd.ensure_string_list('yes_string_list')
        cmd.ensure_string_list('yes_string_list2')

        self.assertRaises(DistutilsOptionError,
                          cmd.ensure_string_list, 'not_string_list')

        self.assertRaises(DistutilsOptionError,
                          cmd.ensure_string_list, 'not_string_list2')

def test_suite():
    return unittest.makeSuite(CommandTestCase)

if __name__ == '__main__':
    test_support.run_unittest(test_suite())
