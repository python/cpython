"""Packaging test suite runner."""

# Ripped from importlib tests, thanks Brett!

import os
import sys
import unittest
from test.support import run_unittest, reap_children


def test_main():
    start_dir = os.path.dirname(__file__)
    top_dir = os.path.dirname(os.path.dirname(start_dir))
    test_loader = unittest.TestLoader()
    run_unittest(test_loader.discover(start_dir, top_level_dir=top_dir))
    reap_children()


if __name__ == '__main__':
    test_main()
