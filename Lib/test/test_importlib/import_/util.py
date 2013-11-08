from .. import util

frozen_importlib, source_importlib = util.import_importlib('importlib')

import builtins
import functools
import importlib
import unittest


__import__ = staticmethod(builtins.__import__), staticmethod(source_importlib.__import__)


def mock_path_hook(*entries, importer):
    """A mock sys.path_hooks entry."""
    def hook(entry):
        if entry not in entries:
            raise ImportError
        return importer
    return hook
