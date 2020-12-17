# Copyright (c) Twisted Matrix Laboratories.
# See LICENSE for details.

from constantly._constants import (
    NamedConstant, Names, ValueConstant, Values, FlagConstant, Flags
)

from ._version import get_versions
__version__ = get_versions()['version']
del get_versions

__author__  = "Twisted Matrix Laboratories"
__license__ = "MIT"
__copyright__ = "Copyright 2011-2015 {0}".format(__author__)


__all__ = [
    'NamedConstant',
    'ValueConstant',
    'FlagConstant',
    'Names',
    'Values',
    'Flags',
]
