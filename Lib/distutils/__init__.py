"""distutils

The main package for the Python Module Distribution Utilities.  Normally
used from a setup script as

   from distutils.core import setup

   setup (...)
"""

# This module should be kept compatible with Python 2.1.

__revision__ = "$Id$"

import sys
__version__ = "%d.%d.%d" % sys.version_info[:3]
del sys
