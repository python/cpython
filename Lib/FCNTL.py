"""Backward-compatibility version of FCNTL; export constants exported by
fcntl, and issue a deprecation warning.
"""

import warnings
warnings.warn("the FCNTL module is deprecated; please use fcntl",
              DeprecationWarning)


# Export the constants known to the fcntl module:
from fcntl import *

# and *only* the constants:
__all__ = [s for s in dir() if s[0] in "ABCDEFGHIJKLMNOPQRSTUVWXYZ"]
