import struct, fcntl, FCNTL

def writelock(f):
	_lock(f, FCNTL.F_WRLCK)

def readlock(f):
	_lock(f, FCNTL.F_RDLCK)

def unlock(f):
	_lock(f, FCNTL.F_UNLCK)

def _lock(f, op):
	dummy = fcntl.fcntl(f.fileno(), FCNTL.F_SETLKW,
			    struct.pack('2h8l', op,
					0, 0, 0, 0, 0, 0, 0, 0, 0))
