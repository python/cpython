"""The machinery of importlib: finders, loaders, hooks, etc."""

from ._bootstrap import ModuleSpec  # noqa
from ._bootstrap import BuiltinImporter  # noqa
from ._bootstrap import FrozenImporter  # noqa
from ._bootstrap_external import (SOURCE_SUFFIXES, DEBUG_BYTECODE_SUFFIXES,  # noqa
                     OPTIMIZED_BYTECODE_SUFFIXES, BYTECODE_SUFFIXES,  # noqa
                     EXTENSION_SUFFIXES)  # noqa
from ._bootstrap_external import WindowsRegistryFinder  # noqa
from ._bootstrap_external import PathFinder  # noqa
from ._bootstrap_external import FileFinder  # noqa
from ._bootstrap_external import SourceFileLoader  # noqa
from ._bootstrap_external import SourcelessFileLoader  # noqa
from ._bootstrap_external import ExtensionFileLoader  # noqa
from ._bootstrap_external import AppleFrameworkLoader  # noqa
from ._bootstrap_external import NamespaceLoader  # noqa


def all_suffixes():
    """Returns a list of all recognized module suffixes for this process"""
    return SOURCE_SUFFIXES + BYTECODE_SUFFIXES + EXTENSION_SUFFIXES
