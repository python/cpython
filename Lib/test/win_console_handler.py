"""Script used to test os.kill on Windows, for issue #1220212

This script is started as a subprocess in test_os and is used to test the
CTRL_C_EVENT and CTRL_BREAK_EVENT signals, which requires a custom handler
to be written into the kill target.

See http://msdn.microsoft.com/en-us/library/ms685049%28v=VS.85%29.aspx for a
similar example in C.
"""

from ctypes import wintypes, WINFUNCTYPE
import signal
import ctypes
import mmap
import sys

# Function prototype for the handler function. Returns BOOL, takes a DWORD.
HandlerRoutine = WINFUNCTYPE(wintypes.BOOL, wintypes.DWORD)

def _ctrl_handler(sig):
    """Handle a sig event and return 0 to terminate the process"""
    if sig == signal.CTRL_C_EVENT:
        pass
    elif sig == signal.CTRL_BREAK_EVENT:
        pass
    else:
        print("UNKNOWN EVENT")
    return 0

ctrl_handler = HandlerRoutine(_ctrl_handler)


SetConsoleCtrlHandler = ctypes.windll.kernel32.SetConsoleCtrlHandler
SetConsoleCtrlHandler.argtypes = (HandlerRoutine, wintypes.BOOL)
SetConsoleCtrlHandler.restype = wintypes.BOOL

if __name__ == "__main__":
    # Add our console control handling function with value 1
    if not SetConsoleCtrlHandler(ctrl_handler, 1):
        print("Unable to add SetConsoleCtrlHandler")
        exit(-1)

    # Awaken mail process
    m = mmap.mmap(-1, 1, sys.argv[1])
    m[0] = '1'

    # Do nothing but wait for the signal
    while True:
        pass
