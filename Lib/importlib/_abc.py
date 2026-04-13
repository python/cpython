"""Subset of importlib.abc used to reduce importlib.util imports."""
from . import _bootstrap
import abc


class Loader(metaclass=abc.ABCMeta):

    """Abstract base class for import loaders."""

    def create_module(self, spec):
        """Return a module to initialize and into which to load.

        This method should raise ImportError if anything prevents it
        from creating a new module.  It may return None to indicate
        that the spec should create the new module.
        """
        # By default, defer to default semantics for the new module.
        return None

    # We don't define exec_module() here since that would break
    # hasattr checks we do to support backward compatibility.
