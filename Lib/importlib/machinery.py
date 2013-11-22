"""The machinery of importlib: finders, loaders, hooks, etc."""

import _imp

from ._bootstrap import (SOURCE_SUFFIXES, DEBUG_BYTECODE_SUFFIXES,
                         OPTIMIZED_BYTECODE_SUFFIXES, BYTECODE_SUFFIXES,
                         EXTENSION_SUFFIXES)
from ._bootstrap import ModuleSpec
from ._bootstrap import BuiltinImporter
from ._bootstrap import FrozenImporter
from ._bootstrap import WindowsRegistryFinder
from ._bootstrap import PathFinder
from ._bootstrap import FileFinder
from ._bootstrap import SourceFileLoader
from ._bootstrap import SourcelessFileLoader
from ._bootstrap import ExtensionFileLoader


def all_suffixes():
    """Returns a list of all recognized module suffixes for this process"""
    return SOURCE_SUFFIXES + BYTECODE_SUFFIXES + EXTENSION_SUFFIXES
