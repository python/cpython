"""Test script for the grp module."""

# XXX This really needs some work, but what are the expected invariants?

import grp
import test_support
import unittest


class GroupDatabaseTestCase(unittest.TestCase):

    def setUp(self):
        self.groups = grp.getgrall()

    def test_getgrgid(self):
        entry = grp.getgrgid(self.groups[0][2])

    def test_getgrnam(self):
        entry = grp.getgrnam(self.groups[0][0])


def test_main():
    test_support.run_unittest(GroupDatabaseTestCase)


if __name__ == "__main__":
    test_main()
