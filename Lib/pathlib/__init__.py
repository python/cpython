"""Object-oriented filesystem paths.

This module provides classes to represent abstract paths and concrete
paths with operations that have semantics appropriate for different
operating systems.
"""

from ._os import *
from ._local import *

__all__ = (_os.__all__ +
           _local.__all__)
