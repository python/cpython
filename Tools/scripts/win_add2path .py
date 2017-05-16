"""Add Python to the search path on Windows

This is a simple script to add Python to the Windows search path. It
modifies the current user (HKCU) tree of the registry.

Copyright (c) 2008 by Christian Heimes <christian@cheimes.de>
Licensed to PSF under a Contributor Agreement.
"""

import sys
import site
import os
import winreg
import ctypes

HKCU = winreg.HKEY_CURRENT_USER
ENV = "Environment"
PATH = "PATH"

def modify():
    pythonpath = os.path.dirname(os.path.normpath(sys.executable))
    scripts = os.path.join(pythonpath, "Scripts")
    appdata = os.environ["APPDATA"]

    with winreg.CreateKey(HKCU, ENV) as key:
        try:
            envpath, dtype = winreg.QueryValueEx(key, PATH)
        except FileNotFoundError:
            envpath, dtype = "", winreg.REG_EXPAND_SZ
            pass
        except:
            raise OSError("Failed to load PATH value")

        if hasattr(site, "USER_SITE") and dtype != winreg.REG_SZ:
            usersite = site.USER_SITE.replace(appdata, "%APPDATA%")
            userpath = os.path.dirname(usersite)
            userscripts = os.path.join(userpath, "Scripts")
        else:
            userscripts = None

        paths = []
        for path in (pythonpath, scripts, userscripts):
            if (path and path not in envpath):
                paths.append(path)

        if envpath == "":
            envpath = os.pathsep.join(paths)
        else:
            envpath = os.pathsep.join([envpath] + paths)
        winreg.SetValueEx(key, PATH, 0, dtype, envpath)
        return paths, envpath

def refresh_environment_variables():
        HWND_BROADCAST = 0xFFFF
        WM_SETTINGCHANGE = 0x001A
        SMTO_ABORTIFHUNG = 0x0002
        if not ctypes.windll.user32.SendMessageTimeoutW(
                HWND_BROADCAST,
                WM_SETTINGCHANGE,
                0,
                ENV,
                SMTO_ABORTIFHUNG,
                1000):
            raise ctypes.WinError()

def main():
    paths, envpath = modify()
    if len(paths) > 1:
        print("Path(s) added:")
        print('\n'.join(paths))
        refresh_environment_variables()
    else:
        print("No path was added")
    print("\nPATH is now:\n%s\n" % envpath)
    print("Expanded:")
    print(winreg.ExpandEnvironmentStrings(envpath))

if __name__ == '__main__':
    main()
