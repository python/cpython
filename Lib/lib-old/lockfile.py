import struct, fcntl

def writelock(f):
    _lock(f, fcntl.F_WRLCK)

def readlock(f):
    _lock(f, fcntl.F_RDLCK)

def unlock(f):
    _lock(f, fcntl.F_UNLCK)

def _lock(f, op):
    dummy = fcntl.fcntl(f.fileno(), fcntl.F_SETLKW,
                        struct.pack('2h8l', op,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0))
