"""Backward-compatibility version of TERMIOS; export constants exported by
termios, and issue a deprecation warning.
"""

import warnings
warnings.warn("the TERMIOS module is deprecated; please use termios",
              DeprecationWarning)


# Export the constants known to the termios module:
from termios import *

# and *only* the constants:
__all__ = [s for s in dir() if s[0] in "ABCDEFGHIJKLMNOPQRSTUVWXYZ"]
