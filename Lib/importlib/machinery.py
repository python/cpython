"""The machinery of importlib: finders, loaders, hooks, etc."""

from ._bootstrap import ModuleSpec
from ._bootstrap import BuiltinImporter
from ._bootstrap import FrozenImporter
from ._bootstrap_external import (
    SOURCE_SUFFIXES, BYTECODE_SUFFIXES, EXTENSION_SUFFIXES,
    DEBUG_BYTECODE_SUFFIXES as _DEBUG_BYTECODE_SUFFIXES,
    OPTIMIZED_BYTECODE_SUFFIXES as _OPTIMIZED_BYTECODE_SUFFIXES
)
from ._bootstrap_external import WindowsRegistryFinder
from ._bootstrap_external import PathFinder
from ._bootstrap_external import FileFinder
from ._bootstrap_external import SourceFileLoader
from ._bootstrap_external import SourcelessFileLoader
from ._bootstrap_external import ExtensionFileLoader
from ._bootstrap_external import AppleFrameworkLoader
from ._bootstrap_external import NamespaceLoader


def all_suffixes():
    """Returns a list of all recognized module suffixes for this process"""
    return SOURCE_SUFFIXES + BYTECODE_SUFFIXES + EXTENSION_SUFFIXES


__all__ = ['AppleFrameworkLoader', 'BYTECODE_SUFFIXES', 'BuiltinImporter',
           'DEBUG_BYTECODE_SUFFIXES', 'EXTENSION_SUFFIXES',
           'ExtensionFileLoader', 'FileFinder', 'FrozenImporter', 'ModuleSpec',
           'NamespaceLoader', 'OPTIMIZED_BYTECODE_SUFFIXES', 'PathFinder',
           'SOURCE_SUFFIXES', 'SourceFileLoader', 'SourcelessFileLoader',
           'WindowsRegistryFinder', 'all_suffixes']


def __getattr__(name):
    import warnings

    if name == 'DEBUG_BYTECODE_SUFFIXES':
        warnings.warn('importlib.machinery.DEBUG_BYTECODE_SUFFIXES is '
                      'deprecated; use importlib.machinery.BYTECODE_SUFFIXES '
                      'instead.',
                      DeprecationWarning, stacklevel=2)
        return _DEBUG_BYTECODE_SUFFIXES
    elif name == 'OPTIMIZED_BYTECODE_SUFFIXES':
        warnings.warn('importlib.machinery.OPTIMIZED_BYTECODE_SUFFIXES is '
                      'deprecated; use importlib.machinery.BYTECODE_SUFFIXES '
                      'instead.',
                      DeprecationWarning, stacklevel=2)
        return _OPTIMIZED_BYTECODE_SUFFIXES

    raise AttributeError(f'module {__name__!r} has no attribute {name!r}')
