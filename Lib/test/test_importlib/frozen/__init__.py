from .. import test_suite
import os.path
import unittest


def test_suite():
    directory = os.path.dirname(__file__)
    return test_suite('importlib.test.frozen', directory)


if __name__ == '__main__':
    from test.support import run_unittest
    run_unittest(test_suite())
