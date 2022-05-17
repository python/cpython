import unittest.test

from test import support


def load_tests(*_):
    # used by unittest
    return unittest.test.suite()


def tearDownModule():
    support.reap_children()


if __name__ == "__main__":
    unittest.main()
