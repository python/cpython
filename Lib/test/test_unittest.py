import unittest.test

from test import test_support


def test_main():
    test_support.run_unittest(unittest.test.suite())
    test_support.reap_children()


if __name__ == "__main__":
    test_main()
