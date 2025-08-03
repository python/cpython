"""The machinery of importlib: finders, loaders, hooks, etc."""

from ._bootstrap import BuiltinImporter, FrozenImporter, ModuleSpec
from ._bootstrap_external import (
    BYTECODE_SUFFIXES,
    EXTENSION_SUFFIXES,
    SOURCE_SUFFIXES,
    AppleFrameworkLoader,
    ExtensionFileLoader,
    FileFinder,
    NamespaceLoader,
    PathFinder,
    SourceFileLoader,
    SourcelessFileLoader,
    WindowsRegistryFinder,
)
from ._bootstrap_external import (
    DEBUG_BYTECODE_SUFFIXES as _DEBUG_BYTECODE_SUFFIXES,
)
from ._bootstrap_external import (
    OPTIMIZED_BYTECODE_SUFFIXES as _OPTIMIZED_BYTECODE_SUFFIXES,
)


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
