import functools
import importlib._bootstrap


using___import__ = False


def import_(*args, **kwargs):
    """Delegate to allow for injecting different implementations of import."""
    if using___import__:
        return __import__(*args, **kwargs)
    else:
        return importlib._bootstrap.__import__(*args, **kwargs)


def importlib_only(fxn):
    """Decorator to mark which tests are not supported by the current
    implementation of __import__()."""
    def inner(*args, **kwargs):
        if using___import__:
            return
        else:
            return fxn(*args, **kwargs)
    functools.update_wrapper(inner, fxn)
    return inner


def mock_path_hook(*entries, importer):
    """A mock sys.path_hooks entry."""
    def hook(entry):
        if entry not in entries:
            raise ImportError
        return importer
    return hook
