"""Interface to the Expat non-validating XML parser."""
__version__ = '$Revision$'

import sys

try:
    from pyexpat import *
except ImportError:
    del sys.modules[__name__]
    del sys
    raise

del sys
