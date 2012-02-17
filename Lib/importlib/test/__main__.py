"""Run importlib's test suite.

Specifying the ``--builtin`` flag will run tests, where applicable, with
builtins.__import__ instead of importlib.__import__.

"""
import argparse
from importlib.test.import_ import util
import os.path
from test.support import run_unittest
import unittest


def test_main():
    parser = argparse.ArgumentParser(description='Execute the importlib test '
                                                  'suite')
    parser.add_argument('-b', '--builtin', action='store_true', default=False,
                        help='use builtins.__import__() instead of importlib')
    args = parser.parse_args()
    if args.builtin:
        util.using___import__ = True
    start_dir = os.path.dirname(__file__)
    top_dir = os.path.dirname(os.path.dirname(start_dir))
    test_loader = unittest.TestLoader()
    run_unittest(test_loader.discover(start_dir, top_level_dir=top_dir))


if __name__ == '__main__':
    test_main()
