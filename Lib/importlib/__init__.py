"""A pure Python implementation of import."""
__all__ = ['__import__', 'import_module', 'invalidate_caches']

from . import _bootstrap


# To simplify imports in test code
_w_long = _bootstrap._w_long
_r_long = _bootstrap._r_long


# Bootstrap help #####################################################
import imp
import sys

_bootstrap._setup(sys, imp)


# Public API #########################################################

from ._bootstrap import __import__


def invalidate_caches():
    """Call the invalidate_caches() method on all finders stored in
    sys.path_importer_caches (where implemented)."""
    for finder in sys.path_importer_cache.values():
        if hasattr(finder, 'invalidate_caches'):
            finder.invalidate_caches()


def import_module(name, package=None):
    """Import a module.

    The 'package' argument is required when performing a relative import. It
    specifies the package to use as the anchor point from which to resolve the
    relative import to an absolute import.

    """
    level = 0
    if name.startswith('.'):
        if not package:
            raise TypeError("relative imports require the 'package' argument")
        for character in name:
            if character != '.':
                break
            level += 1
    return _bootstrap._gcd_import(name[level:], package, level)
