"""
The separate Idle version was eliminated years ago;
idlelib.idlever is no longer used by Idle
and will be removed in 3.6 or later.  Use
    from sys import version
    IDLE_VERSION = version[:version.index(' ')]
"""
# Kept for now only for possible existing extension use
import warnings as w
w.warn(__doc__, DeprecationWarning, stacklevel=2)
from sys import version
IDLE_VERSION = version[:version.index(' ')]
