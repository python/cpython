"""Setup script for Windows NT 3.5 and Windows 95.

Run this with the current directory set to the Python ``root''.
"""

import sys
import strop

del sys.path[1:]

try:
    import nt
except ImportError:
    print "This script should only be run on a Windows (NT or '95) system."
    sys.exit(1)

try:
    sys.winver
    print "This Python version appears to be", sys.winver
except NameError:
    print "Huh?  sys.winver is not defined!"
    sys.exit(1)

# Try to import a common module that *should* work.
print "Looking for Python root directory..."
while 1:
    pwd = nt.getcwd()
    ##print "Could it be", `pwd`, "?"
    try:
        open("Lib\\os.py").close()
        ##print "It appears so."
        break
    except IOError:
        ##print "Hm, it doesn't appear to be.  Try the parent directory."
        try:
            opwd = pwd
            nt.chdir("..")
            pwd = nt.getcwd()
            if opwd == pwd:
                ##print "Seems like we're in the root already."
                raise nt.error
        except nt.error:
            ##print "Can't chdir to the parent -- we're stuck."
            pass
        else:
            ##print "Try again one level higher."
            continue
        print "Hey, would you like to help?"
        print "Please enter the pathname of the Python root."
        while 1:
            try:
                dirname = raw_input("Python root: ")
            except EOFError:
                print "OK, I give up."
                sys.exit(1)
            if not dirname:
                continue
            try:
                nt.chdir(dirname)
            except nt.error:
                print "That directory doesn't seem to exist."
                print "Please try again."
            else:
                break
pwd = nt.getcwd()
print "Python root directory is", pwd
sys.path[1:] = [".\\Lib", ".\\Lib\win", ".\\Bin", ".\\vc40"]

# Now we should be in a position to import win32api and win32con

try:
    import win32api
except ImportError:
    print "Blech.  We *still* can't import win32api."
    print "Giving up."
    sys.exit(1)
try:
    import win32con
except ImportError:
    print "Beh.  We have win32api but not win32con."
    print "Making do with a dummy."
    class win32con:
        REG_NOTIFY_CHANGE_ATTRIBUTES = 0x00000002L
        REG_NOTIFY_CHANGE_SECURITY = 0x00000008L
        REG_RESOURCE_REQUIREMENTS_LIST = 10
        REG_NONE = 0
        REG_SZ = 1
        REG_EXPAND_SZ = 2
        REG_BINARY = 3
        REG_DWORD = 4
        REG_DWORD_LITTLE_ENDIAN = 4
        REG_DWORD_BIG_ENDIAN = 5
        REG_LINK = 6
        REG_MULTI_SZ = 7
        REG_RESOURCE_LIST = 8
        REG_FULL_RESOURCE_DESCRIPTOR = 9
        HKEY_CLASSES_ROOT = 0x80000000
        HKEY_CURRENT_USER = 0x80000001
        HKEY_LOCAL_MACHINE = 0x80000002
        HKEY_USERS = 0x80000003
        HKEY_PERFORMANCE_DATA = 0x80000004
        HKEY_PERFORMANCE_TEXT = 0x80000050
        HKEY_PERFORMANCE_NLSTEXT = 0x80000060


def listtree(handle, level=0):
    i = 0
    while 1:
        try:
            key = win32api.RegEnumKey(handle, i)
        except win32api.error:
            break
        try:
            value = win32api.RegQueryValue(handle, key)
        except win32api.error, msg:
            try:
                msg = msg[2]
            except:
                pass
            value = "*** Error: %s" % str(msg)
        print "    "*level + "%s: %s" % (key, value)
        subhandle = win32api.RegOpenKey(handle, key)
        listtree(subhandle, level+1)
        win32api.RegCloseKey(subhandle)
        i = i+1

roothandle = win32con.HKEY_LOCAL_MACHINE
pythonkey = "Software\\Python"
try:
    pythonhandle = win32api.RegOpenKey(roothandle, pythonkey)
except win32api.error:
    pythonhandle = win32api.RegCreateKey(roothandle, pythonkey)

## listtree(pythonhandle)
## try:
##     handle = win32api.RegOpenKey(pythonhandle, "JustTesting")
## except win32api.error, msg:
##     try: msg = msg[2]
##     except: pass
##     ##print "Error opening, try creating instead:", msg
##     handle = win32api.RegCreateKey(pythonhandle, "JustTesting")
## win32api.RegSetValue(handle, "test1", win32con.REG_SZ, "NO!")
## win32api.RegSetValue(handle, "test2", win32con.REG_SZ, "YES!")
## win32api.RegDeleteKey(handle, "test1")
## win32api.RegDeleteKey(handle, "test2")
## win32api.RegCloseKey(handle)
## win32api.RegDeleteKey(pythonhandle, "JustTesting")
## listtree(pythonhandle)

print "Setting PythonPath..."
corekey = "PythonCore\\%s" % sys.winver
try:
    corehandle = win32api.RegOpenKey(pythonhandle, corekey)
except win32api.error, msg:
    corehandle = win32api.RegCreateKey(pythonhandle, corekey)
path = []
pwd = nt.getcwd()
for i in ["Bin",
          "Lib",
          "Lib\\win",
          "Lib\\tkinter",
          "Lib\\test",
          "Lib\\dos_8x3"]:
    i = pwd + "\\" + i
    path.append(i)
sys.path[1:] = path
pathvalue = strop.join(path, ";")
#print "Setting PythonPath to", pathvalue
win32api.RegSetValue(corehandle, "PythonPath", win32con.REG_SZ, pathvalue)
win32api.RegCloseKey(corehandle)
#listtree(pythonhandle)
win32api.RegCloseKey(pythonhandle)

print "Registering uninstaller..."
pwd = nt.getcwd()
uninstaller = '"%s\\uninstall.bat" "%s"' % (pwd, pwd)
uninstallkey = \
 "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Python"+sys.winver
try:
    uihandle = win32api.RegOpenKey(roothandle, uninstallkey)
except win32api.error, msg:
    uihandle = win32api.RegCreateKey(roothandle, uninstallkey)
win32api.RegSetValueEx(uihandle, "DisplayName", None, win32con.REG_SZ,
                       "Python "+sys.winver)
win32api.RegSetValueEx(uihandle, "UninstallString", None, win32con.REG_SZ,
                       uninstaller)
win32api.RegCloseKey(uihandle)

print "Registering Python Interpreter as shell for *.py files..."
pwd = nt.getcwd()
interpreter = '"%s\\Bin\\python.exe" -i "%%1"' % pwd
print "Interpreter command is", interpreter
root = win32con.HKEY_CLASSES_ROOT
sz = win32con.REG_SZ
win32api.RegSetValue(root, ".py", sz, "Python.Script")
win32api.RegSetValue(root , "Python.Script", sz, "Python Script")
win32api.RegSetValue(root , "Python.Script\\Shell\\Open\\Command", sz,
                     interpreter)

import compileall
print "Compiling all library modules..."
compileall.main()

print "Installation complete."

envkeys = map(strop.upper, nt.environ.keys())
if 'PYTHONPATH' in envkeys:
    print """
**********************************************************************
WARNING!
You have set the environment variable PYTHONPATH.
This will override the default Python module search path
and probably cause you to use an old or broken Python installation.
Go into your control panel *now* and delete PYTHONPATH!
**********************************************************************
"""

raw_input("Press Enter to exit: ")
sys.exit(0)


registry_doc = """Summary of the Win32 API Registry interfaces.

Concepts:
    A _directory_ is a collection of key/value pairs.
    You need a _handle_ for a directory to do anything with it.
    There are some predefined keys, e.g. HKEY_LOCAL_MACHINE.
    A _key_ is an ASCII string; NT file system conventions apply.
    A _value_ has a type and some data; there are predefined types
    (e.g. REG_SZ is a string, REG_DWORD is a 4-byte integer).
    There's some fishiness in that in fact multiple, named values
    can appear under each key, but this seems little used (in this
    case, the value is best seen as a structured value).
    A key can also refer to a _subdirectory_.  In this case the
    associated value is typically empty.  To get a handle for a
    subdirectory, use RegOpenKey(handle, key).  The key can also
    be a backslash-separated path, so you can go directly from one of
    the predefined keys to the directory you are interested in.

Most common functions:
    RegOpenKey(handle, keypath) -> handle
        Get a handle for an existing subdirectory
    RegCreateKey(handle, keypath) -> handle
        Get a handle for a new subdirectory
    RegDeleteKey(handle, key)
        Delete the given subdirectory -- must be empty
    RegCloseKey(handle)
        Close a handle
    RegGetValue(handle, subkey) -> string
        Get the (unnamed) value stored as key in handle
    RegSetValue(handle, subkey, type, value)
        Set the (unnamed) value stored as key in handle, with given
        type; type should be REG_SZ
    RegSetValueEx(handle, name, reserved, type, value)
        Set the value with given name to the given type and value;
        currently reserved is ignored and type should be REG_SZ

Functions to list directory contents (start counting at 0, fail if done):
    RegEnumKey(handle, i)
        Return the i'th subkey
    RegEnumValue(handle, i)
        Return the i'th name and value

Lesser used functions:
    RegFlushKey(handle)
        Flush the changes to the handle to disk (like Unix sync())
    RegSaveKey(handle, filename, reserved)
        Save the contents to a disk file (broken?!)
    RegLoadKey(handle, keypath, filename)
        Load the contents from a disk file (lots of restrictions!)
"""
