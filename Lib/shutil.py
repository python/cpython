"""Utility functions for copying files.

XXX The functions here don't copy the data fork or other metadata on Mac.

"""

import os
import stat


def copyfile(src, dst):
    """Copy data from src to dst"""
    fsrc = None
    fdst = None
    try:
	fsrc = open(src, 'rb')
	fdst = open(dst, 'wb')
	while 1:
	    buf = fsrc.read(16*1024)
	    if not buf:
		break
	    fdst.write(buf)
    finally:
	if fdst:
	    fdst.close()
	if fsrc:
	    fsrc.close()

def copymode(src, dst):
    """Copy mode bits from src to dst"""
    st = os.stat(src)
    mode = stat.S_IMODE(st[stat.ST_MODE])
    os.chmod(dst, mode)

def copystat(src, dst):
    """Copy all stat info (mode bits, atime and mtime) from src to dst"""
    st = os.stat(src)
    mode = stat.S_IMODE(st[stat.ST_MODE])
    os.chmod(dst, mode)
    os.utime(dst, (st[stat.ST_ATIME], st[stat.ST_MODE]))


def copy(src, dst):
    """Copy data and mode bits ("cp src dst").
    
    The destination may be a directory.

    """
    if os.path.isdir(dst):
	dst = os.path.join(dst, os.path.basename(src))
    copyfile(src, dst)
    copymode(src, dst)

def copy2(src, dst):
    """Copy data and all stat info ("cp -p src dst").

    The destination may be a directory.

    """
    if os.path.isdir(dst):
	dst = os.path.join(dst, os.path.basename(src))
    copyfile(src, dst)
    copystat(src, dst)


def copytree(src, dst, symlinks=0):
    """Recursively copy a directory tree using copy2().

    The destination directory must not already exist.
    Error are reported to standard output.

    If the optional symlinks flag is true, symbolic links in the
    source tree result in symbolic links in the destination tree; if
    it is false, the contents of the files pointed to by symbolic
    links are copied.

    XXX Consider this example code rather than the ultimate tool.

    """
    names = os.listdir(src)
    os.mkdir(dst)
    for name in names:
	srcname = os.path.join(src, name)
	dstname = os.path.join(dst, name)
	try:
	    if symlinks and os.path.islink(srcname):
	    	linkto = os.readlink(srcname)
	    	os.symlink(linkto, dstname)
	    elif os.path.isdir(srcname):
		copytree(srcname, dstname)
	    else:
		copy2(srcname, dstname)
	    # XXX What about devices, sockets etc.?
	except (IOError, os.error), why:
	    print "Can't copy %s to %s: %s" % (`srcname`, `dstname`, str(why))
