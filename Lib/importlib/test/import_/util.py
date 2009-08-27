import functools
import importlib
import importlib._bootstrap
import unittest


using___import__ = False


def import_(*args, **kwargs):
    """Delegate to allow for injecting different implementations of import."""
    if using___import__:
        return __import__(*args, **kwargs)
    else:
        return importlib._bootstrap.__import__(*args, **kwargs)


importlib_only = unittest.skipIf(using___import__, "importlib-specific test")


def mock_path_hook(*entries, importer):
    """A mock sys.path_hooks entry."""
    def hook(entry):
        if entry not in entries:
            raise ImportError
        return importer
    return hook
