import importlib.test
import os


def test_suite():
    directory = os.path.dirname(__file__)
    return importlib.test.test_suite('importlib.test.builtin', directory)


if __name__ == '__main__':
    from test.support import run_unittest
    run_unittest(test_suite())
