import sys

# This is a test module for Python.  It looks in the standard
# places for various *.py files.  If these are moved, you must
# change this module too.

try:
    import string
except:
    print """Could not import the standard "string" module.
  Please check your PYTHONPATH environment variable."""
    sys.exit(1)

try:
    import regex_syntax
except:
    print """Could not import the standard "regex_syntax" module.  If this is
  a PC, you should add the dos_8x3 directory to your PYTHONPATH."""
    sys.exit(1)

import os

for dir in sys.path:
    file = os.path.join(dir, "string.py")
    if os.path.isfile(file):
        test = os.path.join(dir, "test")
        if os.path.isdir(test):
            # Add the "test" directory to PYTHONPATH.
            sys.path = sys.path + [test]

import regrtest # Standard Python tester.
regrtest.main()
