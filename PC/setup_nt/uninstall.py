"""Uninstaller for Windows NT 3.5 and Windows 95.

Actions:

1. Remove our entries from the Registry:
   - Software\Python\PythonCore\<winver>
   - Software\Microsoft\Windows\CurrentVersion\Uninstall\Python<winver>
   (Should we also remove the entry for .py and Python.Script?)

2. Remove the installation tree -- this is assumed to be the directory
   whose path is both os.path.dirname(sys.argv[0]) and sys.path[0]

"""

import sys
import nt
import os
import win32api
import win32con

def rmkey(parent, key, level=0):
    sep = "    "*level
    try:
        handle = win32api.RegOpenKey(parent, key)
    except win32api.error, msg:
        print sep + "No key", `key`
        return
    print sep + "Removing key", key
    while 1:
        try:
            subkey = win32api.RegEnumKey(handle, 0)
        except win32api.error, msg:
            break
        rmkey(handle, subkey, level+1)
    win32api.RegCloseKey(handle)
    win32api.RegDeleteKey(parent, key)
    print sep + "Done with", key

roothandle = win32con.HKEY_LOCAL_MACHINE
pythonkey = "Software\\Python\\PythonCore\\" + sys.winver
rmkey(roothandle, pythonkey)
uninstallkey = \
 "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Python"+sys.winver
rmkey(roothandle, uninstallkey)

def rmtree(dir, level=0):
    sep = "    "*level
    print sep+"rmtree", dir
    for name in os.listdir(dir):
        if level == 0 and \
           os.path.normcase(name) == os.path.normcase("uninstall.bat"):
            continue
        fn = os.path.join(dir, name)
        if os.path.isdir(fn):
            rmtree(fn, level+1)
        else:
            try:
                os.remove(fn)
            except os.error, msg:
                print sep+"  can't remove", `fn`, msg
            else:
                print sep+"  removed", `fn`
    try:
        os.rmdir(dir)
    except os.error, msg:
        print sep+"can't remove directory", `dir`, msg
    else:
        print sep+"removed directory", `dir`    

pwd = os.getcwd()
scriptdir = os.path.normpath(os.path.join(pwd, os.path.dirname(sys.argv[0])))
pathdir = os.path.normpath(os.path.join(pwd, sys.path[0]))
if scriptdir == pathdir:
    rmtree(pathdir)
else:
    print "inconsistend script directory, not removing any files."
    print "script directory =", `scriptdir`
    print "path directory =", `pathdir`
