# Module 'shutil' -- utility functions usable in a shell-like program.
# XXX The copy*() functions here don't copy the data fork on Mac.
# XXX Consider this example code rather than flexible tools.

import os

MODEBITS = 010000	# Lower 12 mode bits
# Change this to 01000 (9 mode bits) to avoid copying setuid etc.

# Copy data from src to dst
#
def copyfile(src, dst):
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

# Copy mode bits from src to dst
#
def copymode(src, dst):
    st = os.stat(src)
    mode = divmod(st[0], MODEBITS)[1]
    os.chmod(dst, mode)

# Copy all stat info (mode bits, atime and mtime) from src to dst
#
def copystat(src, dst):
    st = os.stat(src)
    mode = divmod(st[0], MODEBITS)[1]
    os.chmod(dst, mode)
    os.utime(dst, st[7:9])

# Copy data and mode bits ("cp src dst").
# Support directory as target.
#
def copy(src, dst):
    if os.path.isdir(dst):
	dst = os.path.join(dst, os.path.basename(src))
    copyfile(src, dst)
    copymode(src, dst)

# Copy data and all stat info ("cp -p src dst").
# Support directory as target.
#
def copy2(src, dst):
    if os.path.isdir(dst):
	dst = os.path.join(dst, os.path.basename(src))
    copyfile(src, dst)
    copystat(src, dst)

# Recursively copy a directory tree.
# The destination must not already exist.
#
def copytree(src, dst):
    names = os.listdir(src)
    os.mkdir(dst, 0777)
    for name in names:
	srcname = os.path.join(src, name)
	dstname = os.path.join(dst, name)
	#print 'Copying', srcname, 'to', dstname
	try:
	    #if os.path.islink(srcname):
	    #	linkto = os.readlink(srcname)
	    #	os.symlink(linkto, dstname)
	    #elif os.path.isdir(srcname):
	    if os.path.isdir(srcname):
		copytree(srcname, dstname)
	    else:
		copy2(srcname, dstname)
	    # XXX What about devices, sockets etc.?
	except os.error, why:
	    print 'Could not copy', srcname, 'to', dstname,
	    print '(', why[1], ')'
