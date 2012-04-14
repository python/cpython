import functools
import importlib
import unittest


using___import__ = False


def import_(*args, **kwargs):
    """Delegate to allow for injecting different implementations of import."""
    if using___import__:
        return __import__(*args, **kwargs)
    else:
        return importlib.__import__(*args, **kwargs)


def importlib_only(fxn):
    """Decorator to skip a test if using __builtins__.__import__."""
    return unittest.skipIf(using___import__, "importlib-specific test")(fxn)


def mock_path_hook(*entries, importer):
    """A mock sys.path_hooks entry."""
    def hook(entry):
        if entry not in entries:
            raise ImportError
        return importer
    return hook
