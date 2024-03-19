"""Common pathname manipulations.

This exports all functions from posixpath or ntpath, e.g. abspath,
basename, etc.

Programs that import and use 'os.path' stand a better chance of being
portable between different platforms.
"""
import sys

_names = sys.builtin_module_names
if 'posix' in _names:
    from posixpath import *
    from posixpath import __all__ as __all__
elif 'nt' in _names:
    from ntpath import *
    from ntpath import __all__ as __all__
else:
    raise ImportError('no os specific module found')

del _names
del sys
