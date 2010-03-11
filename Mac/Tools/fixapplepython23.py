#!/usr/bin/env python
"""fixapplepython23 - Fix Apple-installed Python 2.3 (on Mac OS X 10.3)

Python 2.3 (and 2.3.X for X<5) have the problem that building an extension
for a framework installation may accidentally pick up the framework
of a newer Python, in stead of the one that was used to build the extension.

This script modifies the Makefile (in .../lib/python2.3/config) to use
the newer method of linking extensions with "-undefined dynamic_lookup"
which fixes this problem.

The script will first check all prerequisites, and return a zero exit
status also when nothing needs to be fixed.
"""
import sys
import os
import gestalt

MAKEFILE='/System/Library/Frameworks/Python.framework/Versions/2.3/lib/python2.3/config/Makefile'
CHANGES=((
    'LDSHARED=\t$(CC) $(LDFLAGS) -bundle -framework $(PYTHONFRAMEWORK)\n',
    'LDSHARED=\t$(CC) $(LDFLAGS) -bundle -undefined dynamic_lookup\n'
    ),(
    'BLDSHARED=\t$(CC) $(LDFLAGS) -bundle -framework $(PYTHONFRAMEWORK)\n',
    'BLDSHARED=\t$(CC) $(LDFLAGS) -bundle -undefined dynamic_lookup\n'
    ),(
    'CC=\t\tgcc\n',
    'CC=\t\t/System/Library/Frameworks/Python.framework/Versions/2.3/lib/python2.3/config/PantherPythonFix/run-gcc\n'
    ),(
    'CXX=\t\tc++\n',
    'CXX=\t\t/System/Library/Frameworks/Python.framework/Versions/2.3/lib/python2.3/config/PantherPythonFix/run-g++\n'
))

GCC_SCRIPT='/System/Library/Frameworks/Python.framework/Versions/2.3/lib/python2.3/config/PantherPythonFix/run-gcc'
GXX_SCRIPT='/System/Library/Frameworks/Python.framework/Versions/2.3/lib/python2.3/config/PantherPythonFix/run-g++'
SCRIPT="""#!/bin/sh
export MACOSX_DEPLOYMENT_TARGET=10.3
exec %s "${@}"
"""

def findline(lines, start):
    """return line starting with given string or -1"""
    for i in range(len(lines)):
        if lines[i][:len(start)] == start:
            return i
    return -1

def fix(makefile, do_apply):
    """Fix the Makefile, if required."""
    fixed = False
    lines = open(makefile).readlines()

    for old, new in CHANGES:
        i = findline(lines, new)
        if i >= 0:
            # Already fixed
            continue
        i = findline(lines, old)
        if i < 0:
            print 'fixapplepython23: Python installation not fixed (appears broken)'
            print 'fixapplepython23: missing line:', old
            return 2
        lines[i] = new
        fixed = True

    if fixed:
        if do_apply:
            print 'fixapplepython23: Fix to Apple-installed Python 2.3 applied'
            os.rename(makefile, makefile + '~')
            open(makefile, 'w').writelines(lines)
            return 0
        else:
            print 'fixapplepython23: Fix to Apple-installed Python 2.3 should be applied'
            return 1
    else:
        print 'fixapplepython23: No fix needed, appears to have been applied before'
        return 0

def makescript(filename, compiler):
    """Create a wrapper script for a compiler"""
    dirname = os.path.split(filename)[0]
    if not os.access(dirname, os.X_OK):
        os.mkdir(dirname, 0755)
    fp = open(filename, 'w')
    fp.write(SCRIPT % compiler)
    fp.close()
    os.chmod(filename, 0755)
    print 'fixapplepython23: Created', filename

def main():
    # Check for -n option
    if len(sys.argv) > 1 and sys.argv[1] == '-n':
        do_apply = False
    else:
        do_apply = True
    # First check OS version
    if sys.byteorder == 'little':
        # All intel macs are fine
        print "fixapplypython23: no fix is needed on MacOSX on Intel"
        sys.exit(0)

    if gestalt.gestalt('sysv') < 0x1030:
        print 'fixapplepython23: no fix needed on MacOSX < 10.3'
        sys.exit(0)

    if gestalt.gestalt('sysv') >= 0x1040:
        print 'fixapplepython23: no fix needed on MacOSX >= 10.4'
        sys.exit(0)

    # Test that a framework Python is indeed installed
    if not os.path.exists(MAKEFILE):
        print 'fixapplepython23: Python framework does not appear to be installed (?), nothing fixed'
        sys.exit(0)
    # Check that we can actually write the file
    if do_apply and not os.access(MAKEFILE, os.W_OK):
        print 'fixapplepython23: No write permission, please run with "sudo"'
        sys.exit(2)
    # Create the shell scripts
    if do_apply:
        if not os.access(GCC_SCRIPT, os.X_OK):
            makescript(GCC_SCRIPT, "gcc")
        if not os.access(GXX_SCRIPT, os.X_OK):
            makescript(GXX_SCRIPT, "g++")
    #  Finally fix the makefile
    rv = fix(MAKEFILE, do_apply)
    #sys.exit(rv)
    sys.exit(0)

if __name__ == '__main__':
    main()
