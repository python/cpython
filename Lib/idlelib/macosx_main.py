#!/usr/bin/env pythonw
# IDLE.app
#
# Installation:
#   see the install_IDLE target in python/dist/src/Mac/OSX/Makefile
#
# Usage:
#
# 1. Double clicking IDLE icon will open IDLE.
# 2. Dropping file on IDLE icon will open that file in IDLE.
# 3. Launch from command line with files with this command-line:
#
#     /Applications/Python/IDLE.app/Contents/MacOS/python file1 file2 file3
#
#

# Add IDLE.app/Contents/Resources/idlelib to path.
# __file__ refers to this file when it is used as a module, sys.argv[0]
# refers to this file when it is used as a script (pythonw macosx_main.py)
import sys

from os.path import split, join, isdir
try:
    __file__
except NameError:
    __file__ = sys.argv[0]
idlelib = join(split(__file__)[0], 'idlelib')
if isdir(idlelib):
    sys.path.append(idlelib)

# see if we are being asked to execute the subprocess code
if '-p' in sys.argv:
    # run expects only the port number in sys.argv
    sys.argv.remove('-p')

    # this module will become the namespace used by the interactive
    # interpreter; remove all variables we have defined.
    del sys, __file__, split, join, isdir, idlelib
    __import__('run').main()
else:
    # Load idlelib/idle.py which starts the application.
    import idle
