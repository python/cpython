"""Tests for the sampling profiler (profiling.sampling)."""

import os
from test.support import load_package_tests


def load_tests(*args):
    """Load all tests from this subpackage."""
    return load_package_tests(os.path.dirname(__file__), *args)
