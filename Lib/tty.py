"""Terminal utilities."""

# Author: Steen Lumholt.

from termios import *

__all__ = ["setraw", "setcbreak"]

# Indexes for termios list.
IFLAG = 0
OFLAG = 1
CFLAG = 2
LFLAG = 3
ISPEED = 4
OSPEED = 5
CC = 6

def setraw(fd, when=TCSAFLUSH, block=True):
    """Put terminal into a raw mode."""
    mode = tcgetattr(fd)
    mode[IFLAG] = mode[IFLAG] & ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON)
    mode[OFLAG] = mode[OFLAG] & ~(OPOST)
    mode[CFLAG] = mode[CFLAG] & ~(CSIZE | PARENB)
    mode[CFLAG] = mode[CFLAG] | CS8
    mode[LFLAG] = mode[LFLAG] & ~(ECHO | ICANON | IEXTEN | ISIG)
    if block:
        mode[CC][VMIN] = 1
        mode[CC][VTIME] = 0
    else:
        mode[CC][VMIN] = 0
        mode[CC][VTIME] = 1
    tcsetattr(fd, when, mode)

def setcbreak(fd, when=TCSAFLUSH):
    """Put terminal into a cbreak mode."""
    mode = tcgetattr(fd)
    mode[LFLAG] = mode[LFLAG] & ~(ECHO | ICANON)
    mode[CC][VMIN] = 1
    mode[CC][VTIME] = 0
    tcsetattr(fd, when, mode)

# New: utilities for backing up and restoring modes

__all__.extend(["save", "restore", "setdefaultfd"])

default_fd = None

def _checkfd(fd):
    if fd is None:
        if default_fd is None:
            raise ValueError("fd unset")
        return default_fd
    return fd

modeStorage = ({}, [])
modeStack = []
modeStackPos = 0

def _modeStackPush(way, key):
    """Save mode change history to stack"""
    global modeStack, modeStackPos
    if modeStackPos < len(modeStack):
        modeStack[modeStackPos] = (way, key)
    else:
        modeStack.append((way, key))
    modeStackPos += 1

def _modeStackPop():
    """Pop from back of mode history stack"""
    global modeStack, modeStackPos
    if modeStackPos <= 0:
        return None
    modeStackPos -= 1
    return modeStack[modeStackPos]

def save(fd=None, key=None):
    """
    Save terminal mode.
    
    Saves without keys can only be accessed through restores without keys.
    
    Saves with keys can be accessed both ways.
    """
    fd = _checkfd(fd)
    if key is None:
        way = 1  # Where this goes to: keyword storage (0) or list space (1)
        key = len(modeStorage[1])
        modeStorage[1].append(tcgetattr(fd))
    else:
        way = 0
        modeStorage[0][key] = tcgetattr(fd)
    _modeStackPush(way, key)

def restore(fd=None, key=None, when=TCSAFLUSH):
    """
    Restore terminal mode.
    
    Any restoration with keys will be ONE NEW STEP in history;
    
    Sequential restoration without keys go BACK in history ONE STEP a time.
    """
    fd = _checkfd(fd)
    way = 0
    if key is None:
        way, key = _modeStackPop()
    else:
        _modeStackPush(way, key)
    tcsetattr(fd, when, modeStorage[way][key])

def setdefaultfd(fd):
    """
    Set default fd.
    
    This affects ONLY `save` and `restore`
    
    This does NOT affect `setraw` and `setcbreak` for compatibility.
    """
    global default_fd
    default_fd = fd
