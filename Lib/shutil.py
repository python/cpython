"""Utility functions for copying files and directory trees.

XXX The functions here don't copy the resource fork or other metadata on Mac.

"""

import os
import sys
import stat


def copyfileobj(fsrc, fdst, length=16*1024):
    """copy data from file-like object fsrc to file-like object fdst"""
    while 1:
        buf = fsrc.read(length)
        if not buf:
            break
        fdst.write(buf)

	
def copyfile(src, dst):
    """Copy data from src to dst"""
    fsrc = None
    fdst = None
    try:
        fsrc = open(src, 'rb')
        fdst = open(dst, 'wb')
        copyfileobj(fsrc, fdst)
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
    os.utime(dst, (st[stat.ST_ATIME], st[stat.ST_MTIME]))
    os.chmod(dst, mode)


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
                copytree(srcname, dstname, symlinks)
            else:
                copy2(srcname, dstname)
            # XXX What about devices, sockets etc.?
        except (IOError, os.error), why:
            print "Can't copy %s to %s: %s" % (`srcname`, `dstname`, str(why))

def rmtree(path, ignore_errors=0, onerror=None):
    """Recursively delete a directory tree.

    If ignore_errors is set, errors are ignored; otherwise, if
    onerror is set, it is called to handle the error; otherwise, an
    exception is raised.

    """
    cmdtuples = []
    _build_cmdtuple(path, cmdtuples)
    for cmd in cmdtuples:
        try:
            apply(cmd[0], (cmd[1],))
        except:
            exc = sys.exc_info()
            if ignore_errors:
                pass
            elif onerror:
                onerror(cmd[0], cmd[1], exc)
            else:
                raise exc[0], (exc[1][0], exc[1][1] + ' removing '+cmd[1])

# Helper for rmtree()
def _build_cmdtuple(path, cmdtuples):
    for f in os.listdir(path):
        real_f = os.path.join(path,f)
        if os.path.isdir(real_f) and not os.path.islink(real_f):
            _build_cmdtuple(real_f, cmdtuples)
        else:
            cmdtuples.append((os.remove, real_f))
    cmdtuples.append((os.rmdir, path))
