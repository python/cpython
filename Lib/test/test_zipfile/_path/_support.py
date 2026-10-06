import importlib
import unittest


def import_or_skip(name):
    try:
        return importlib.import_module(name)
    except ImportError:  # pragma: no cover
        raise unittest.SkipTest(f'Unable to import {name}')
