# Module 'stat'

# Defines constants and functions for interpreting stat/lstat struct
# as returned by posix.stat() and posix.lstat() (if it exists).

# XXX This module may have to be adapted for UNIXoid systems whose
# <sys/stat.h> deviates from AT&T or BSD UNIX; their S_IF* constants
# may differ.

# Suggested usage: from stat import *

# Tuple indices for stat struct members

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

def S_IMODE(mode):
	return mode%4096
def S_IFMT(mode):
	return mode - mode%4096

S_IFDIR  = 0040000
S_IFCHR  = 0020000
S_IFBLK  = 0060000
S_IFREG  = 0100000
S_IFIFO  = 0010000
S_IFLNK  = 0120000
S_IFSOCK = 0140000

def S_ISDIR(mode):
	return S_IFMT(mode) = S_IFDIR

def S_ISCHR(mode):
	return S_IFMT(mode) = S_IFCHR

def S_ISBLK(mode):
	return S_IFMT(mode) = S_IFBLK

def S_ISREG(mode):
	return S_IFMT(mode) = S_IFREG

def S_ISFIFO(mode):
	return S_IFMT(mode) = S_IFIFO

def S_ISLNK(mode):
	return S_IFMT(mode) = S_IFLNK

def S_ISSOCK(mode):
	return S_IFMT(mode) = S_IFSOCK
