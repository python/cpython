# Module 'stat'
#
# Defines constants and functions for interpreting stat/lstat struct
# as returned by os.stat() and os.lstat() (if it exists).
#
# Suggested usage: from stat import *
#
# XXX Strictly spoken, this module may have to be adapted for each POSIX
# implementation; in practice, however, the numeric constants used by
# stat() are almost universal (even for stat() emulations on non-UNIX
# systems like MS-DOS).

# Indices for stat struct members in tuple returned by os.stat()

ST_MODE  = 0
ST_INO   = 1
ST_DEV   = 2
ST_NLINK = 3
ST_UID   = 4
ST_GID   = 5
ST_SIZE  = 6
ST_ATIME = 7
ST_MTIME = 8
ST_CTIME = 9

# Extract bits from the mode

def S_IMODE(mode):
	return 0

def S_IFMT(mode):
	return mode & 0xFFFF

# Constants used as S_IFMT() for various file types
# (not all are implemented on all systems)

S_IFDIR  = 0x0000
S_IFREG  = 0x0003

# Functions to test for each file type

def S_ISDIR(mode):
	return S_IFMT(mode) == S_IFDIR

def S_ISCHR(mode):
	return 0

def S_ISBLK(mode):
	return 0

def S_ISREG(mode):
	return S_IFMT(mode) == S_IFREG

def S_ISFIFO(mode):
	return 0

def S_ISLNK(mode):
	return 0

def S_ISSOCK(mode):
	return 0

# Names for permission bits

S_ISUID = 04000
S_ISGID = 02000
S_ENFMT = S_ISGID
S_ISVTX = 01000
S_IREAD = 00400
S_IWRITE = 00200
S_IEXEC = 00100
S_IRWXU = 00700
S_IRUSR = 00400
S_IWUSR = 00200
S_IXUSR = 00100
S_IRWXG = 00070
S_IRGRP = 00040
S_IWGRP = 00020
S_IXGRP = 00010
S_IRWXO = 00007
S_IROTH = 00004
S_IWOTH = 00002
S_IXOTH = 00001
