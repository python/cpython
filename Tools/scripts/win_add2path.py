"""Add Python to the search path on Windows

This is a simple script to add Python to the Windows search path. It
modifies the current user (HKCU) tree of the registry.

Copyright (c) 2008 by Christian Heimes <christian@cheimes.de>
Licensed to PSF under a Contributor Agreement.
"""

import sys
import site
import os
import _winreg

HKCU = _winreg.HKEY_CURRENT_USER
ENV = "Environment"
PATH = "PATH"
DEFAULT = u"%PATH%"

def modify():
    pythonpath = os.path.dirname(os.path.normpath(sys.executable))
    scripts = os.path.join(pythonpath, "Scripts")
    appdata = os.environ["APPDATA"]
    if hasattr(site, "USER_SITE"):
        userpath = site.USER_SITE.replace(appdata, "%APPDATA%")
        userscripts = os.path.join(userpath, "Scripts")
    else:
        userscripts = None

    with _winreg.CreateKey(HKCU, ENV) as key:
        try:
            envpath = _winreg.QueryValueEx(key, PATH)[0]
        except WindowsError:
            envpath = DEFAULT

        paths = [envpath]
        for path in (pythonpath, scripts, userscripts):
            if path and path not in envpath and os.path.isdir(path):
                paths.append(path)

        envpath = os.pathsep.join(paths)
        _winreg.SetValueEx(key, PATH, 0, _winreg.REG_EXPAND_SZ, envpath)
        return paths, envpath

def main():
    paths, envpath = modify()
    if len(paths) > 1:
        print "Path(s) added:"
        print '\n'.join(paths[1:])
    else:
        print "No path was added"
    print "\nPATH is now:\n%s\n" % envpath
    print "Expanded:"
    print _winreg.ExpandEnvironmentStrings(envpath)

if __name__ == '__main__':
    main()
