"""A pure Python implementation of import."""
__all__ = ['__import__', 'import_module', 'invalidate_caches']

# Bootstrap help #####################################################
import imp
import sys

try:
    _bootstrap = sys.modules['_frozen_importlib']
except ImportError:
    from . import _bootstrap
    _bootstrap._setup(sys, imp)
else:
    # importlib._bootstrap is the built-in import, ensure we don't create
    # a second copy of the module.
    _bootstrap.__name__ = 'importlib._bootstrap'
    _bootstrap.__package__ = 'importlib'
    _bootstrap.__file__ = __file__.replace('__init__.py', '_bootstrap.py')
    sys.modules['importlib._bootstrap'] = _bootstrap

# To simplify imports in test code
_w_long = _bootstrap._w_long
_r_long = _bootstrap._r_long


# Public API #########################################################

from ._bootstrap import __import__


def invalidate_caches():
    """Call the invalidate_caches() method on all finders stored in
    sys.path_importer_caches (where implemented)."""
    for finder in sys.path_importer_cache.values():
        if hasattr(finder, 'invalidate_caches'):
            finder.invalidate_caches()


def find_loader(name, path=None):
    """Find the loader for the specified module.

    First, sys.modules is checked to see if the module was already imported. If
    so, then sys.modules[name].__loader__ is returned. If that happens to be
    set to None, then ValueError is raised. If the module is not in
    sys.modules, then sys.meta_path is searched for a suitable loader with the
    value of 'path' given to the finders. None is returned if no loader could
    be found.

    Dotted names do not have their parent packages implicitly imported.

    """
    try:
        loader = sys.modules[name].__loader__
        if loader is None:
            raise ValueError('{}.__loader__ is None'.format(name))
        else:
            return loader
    except KeyError:
        pass
    return _bootstrap._find_module(name, path)


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
