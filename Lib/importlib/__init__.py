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

import os
import re
import tokenize

# To simplify imports in test code
_w_long = _bootstrap._w_long
_r_long = _bootstrap._r_long


# Bootstrap help #####################################################

# TODO: Expose from import.c, else handle encode/decode as _os.environ returns
#       bytes.
def _case_ok(directory, check):
    """Check if the directory contains something matching 'check'.

    No check is done if the file/directory exists or not.

    """
    if 'PYTHONCASEOK' in os.environ:
        return True
    if not directory:
        directory = os.getcwd()
    if check in os.listdir(directory):
        return True
    return False

_bootstrap._case_ok = _case_ok


# Required built-in modules.
try:
    import posix as _os
except ImportError:
    try:
        import nt as _os
    except ImportError:
        try:
            import os2 as _os
        except ImportError:
            raise ImportError('posix, nt, or os2 module required for importlib')
_bootstrap._os = _os
import imp, sys, marshal, _io
_bootstrap.imp = imp
_bootstrap.sys = sys
_bootstrap.marshal = marshal
_bootstrap._io = _io
import _warnings
_bootstrap._warnings = _warnings


from os import sep
# For os.path.join replacement; pull from Include/osdefs.h:SEP .
_bootstrap.path_sep = sep


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
