"""distutils

The main package for the Python Module Distribution Utilities.  Normally
used from a setup script as

   from distutils.core import setup

   setup (...)
"""

import sys
import warnings

__version__ = sys.version[:sys.version.index(' ')]

warnings.warn("The distutils package is deprecated and slated for "
              "removal in Python 3.12. Use setuptools or check "
              "PEP 632 for potential alternatives",
              DeprecationWarning, 2)
