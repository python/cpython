"""The machinery of importlib: finders, loaders, hooks, etc."""

from ._bootstrap import ModuleSpec  # noqa: F401
from ._bootstrap import BuiltinImporter  # noqa: F401
from ._bootstrap import FrozenImporter  # noqa: F401
from ._bootstrap_external import (SOURCE_SUFFIXES, DEBUG_BYTECODE_SUFFIXES,  # noqa: F401
                     OPTIMIZED_BYTECODE_SUFFIXES, BYTECODE_SUFFIXES,
                     EXTENSION_SUFFIXES)
from ._bootstrap_external import WindowsRegistryFinder  # noqa: F401
from ._bootstrap_external import PathFinder  # noqa: F401
from ._bootstrap_external import FileFinder  # noqa: F401
from ._bootstrap_external import SourceFileLoader  # noqa: F401
from ._bootstrap_external import SourcelessFileLoader  # noqa: F401
from ._bootstrap_external import ExtensionFileLoader  # noqa: F401
from ._bootstrap_external import AppleFrameworkLoader  # noqa: F401
from ._bootstrap_external import NamespaceLoader  # noqa: F401


def all_suffixes():
    """Returns a list of all recognized module suffixes for this process"""
    return SOURCE_SUFFIXES + BYTECODE_SUFFIXES + EXTENSION_SUFFIXES
