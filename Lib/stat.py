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
# systems like Macintosh or MS-DOS).

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
	return mode & 07777

def S_IFMT(mode):
	return mode & ~07777

# Constants used as S_IFMT() for various file types
# (not all are implemented on all systems)

S_IFDIR  = 0040000
S_IFCHR  = 0020000
S_IFBLK  = 0060000
S_IFREG  = 0100000
S_IFIFO  = 0010000
S_IFLNK  = 0120000
S_IFSOCK = 0140000

# Functions to test for each file type

def S_ISDIR(mode):
	return S_IFMT(mode) == S_IFDIR

def S_ISCHR(mode):
	return S_IFMT(mode) == S_IFCHR

def S_ISBLK(mode):
	return S_IFMT(mode) == S_IFBLK

def S_ISREG(mode):
	return S_IFMT(mode) == S_IFREG

def S_ISFIFO(mode):
	return S_IFMT(mode) == S_IFIFO

def S_ISLNK(mode):
	return S_IFMT(mode) == S_IFLNK

def S_ISSOCK(mode):
	return S_IFMT(mode) == S_IFSOCK
