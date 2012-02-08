"""A pure Python implementation of import.

References on import:

    * Language reference
          http://docs.python.org/ref/import.html
    * __import__ function
          http://docs.python.org/lib/built-in-funcs.html
    * Packages
          http://www.python.org/doc/essays/packages.html
    * PEP 235: Import on Case-Insensitive Platforms
          http://www.python.org/dev/peps/pep-0235
    * PEP 275: Import Modules from Zip Archives
          http://www.python.org/dev/peps/pep-0273
    * PEP 302: New Import Hooks
          http://www.python.org/dev/peps/pep-0302/
    * PEP 328: Imports: Multi-line and Absolute/Relative
          http://www.python.org/dev/peps/pep-0328

"""
__all__ = ['__import__', 'import_module']

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
