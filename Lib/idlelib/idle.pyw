#! /usr/bin/env python

try:
    import idlelib.PyShell
    idlelib.PyShell.main()
except:
    # IDLE is not installed, but maybe PyShell is on sys.path:
    import PyShell
    PyShell.main()
