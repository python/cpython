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
OLD_LDSHARED='LDSHARED=\t$(CC) $(LDFLAGS) -bundle -framework $(PYTHONFRAMEWORK)\n'
OLD_BLDSHARED='B' + OLD_LDSHARED
NEW_LDSHARED='LDSHARED=\t$(CC) $(LDFLAGS) -bundle -undefined dynamic_lookup\n'
NEW_BLDSHARED='B' + NEW_LDSHARED

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
    
    i = findline(lines, 'LDSHARED=')
    if i < 0:
        print 'fixapplepython23: Python installation not fixed (appears broken, no LDSHARED)'
        return 2
    if lines[i] == OLD_LDSHARED:
        lines[i] = NEW_LDSHARED
        fixed = True
    elif lines[i] != NEW_LDSHARED:
        print 'fixapplepython23: Python installation not fixed (appears modified, unexpected LDSHARED)'
        return 2
        
    i = findline(lines, 'BLDSHARED=')
    if i < 0:
        print 'fixapplepython23: Python installation not fixed (appears broken, no BLDSHARED)'
        return 2
    if lines[i] == OLD_BLDSHARED:
        lines[i] = NEW_BLDSHARED
        fixed = True
    elif lines[i] != NEW_BLDSHARED:
        print 'fixapplepython23: Python installation not fixed (appears modified, unexpected BLDSHARED)'
        return 2
        
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
        
def main():
    # Check for -n option
    if len(sys.argv) > 1 and sys.argv[1] == '-n':
        do_apply = False
    else:
        do_apply = True
    # First check OS version
    if gestalt.gestalt('sysv') < 0x1030:
        print 'fixapplepython23: no fix needed on MacOSX < 10.3'
        sys.exit(0)
    # Test that a framework Python is indeed installed
    if not os.path.exists(MAKEFILE):
        print 'fixapplepython23: Python framework does not appear to be installed (?), nothing fixed'
        sys.exit(0)
    # Check that we can actually write the file
    if do_apply and not os.access(MAKEFILE, os.W_OK):
        print 'fixapplepython23: No write permission, please run with "sudo"'
        sys.exit(2)
    # And finally fix it
    rv = fix(MAKEFILE, do_apply)
    sys.exit(rv)
    
if __name__ == '__main__':
    main()
    
