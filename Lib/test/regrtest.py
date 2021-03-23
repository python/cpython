#! /usr/bin/env python3

"""
Script to run Python regression tests.

Run this script with -h or --help for documentation.
"""

import os
import sys
from test.libregrtest import main


# Alias for backward compatibility (just in case)
main_in_temp_cwd = main


def _main():
    global __file__

    # Remove regrtest.py's own directory from the module search path. Despite
    # the elimination of implicit relative imports, this is still needed to
    # ensure that submodules of the test package do not inappropriately appear
    # as top-level modules even when people (or buildbots!) invoke regrtest.py
    # directly instead of using the -m switch
    mydir = os.path.abspath(os.path.normpath(os.path.dirname(sys.argv[0])))
    i = len(sys.path) - 1
    while i >= 0:
        if os.path.abspath(os.path.normpath(sys.path[i])) == mydir:
            del sys.path[i]
        else:
            i -= 1

    # findtestdir() gets the dirname out of __file__, so we have to make it
    # absolute before changing the working directory.
    # For example __file__ may be relative when running trace or profile.
    # See issue #9323.
    __file__ = os.path.abspath(__file__)

    # sanity check
    assert __file__ == os.path.abspath(sys.argv[0])

    main()


if __name__ == '__main__':
    _main()
