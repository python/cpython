# Module 'shutil' -- utility functions usable in a shell-like program

import posix
import path

MODEBITS = 010000	# Lower 12 mode bits
# Change this to 01000 (9 mode bits) to avoid copying setuid etc.

# Copy data from src to dst
#
def copyfile(src, dst):
	fsrc = open(src, 'r')
	fdst = open(dst, 'w')
	while 1:
		buf = fsrc.read(16*1024)
		if not buf: break
		fdst.write(buf)

# Copy mode bits from src to dst
#
def copymode(src, dst):
	st = posix.stat(src)
	mode = divmod(st[0], MODEBITS)[1]
	posix.chmod(dst, mode)

# Copy all stat info (mode bits, atime and mtime) from src to dst
#
def copystat(src, dst):
	st = posix.stat(src)
	mode = divmod(st[0], MODEBITS)[1]
	posix.chmod(dst, mode)
	posix.utimes(dst, st[7:9])

# Copy data and mode bits ("cp src dst")
#
def copy(src, dst):
	copyfile(src, dst)
	copymode(src, dst)

# Copy data and all stat info ("cp -p src dst")
#
def copy2(src, dst):
	copyfile(src, dst)
	copystat(src, dst)

# Recursively copy a directory tree.
# The destination must not already exist.
#
def copytree(src, dst):
	names = posix.listdir(src)
	posix.mkdir(dst, 0777)
	dot_dotdot = '.', '..'
	for name in names:
		if name not in dot_dotdot:
			srcname = path.cat(src, name)
			dstname = path.cat(dst, name)
			#print 'Copying', srcname, 'to', dstname
			try:
				#if path.islink(srcname):
				#	linkto = posix.readlink(srcname)
				#	posix.symlink(linkto, dstname)
				#elif path.isdir(srcname):
				if path.isdir(srcname):
					copytree(srcname, dstname)
				else:
					copy2(srcname, dstname)
				# XXX What about devices, sockets etc.?
			except posix.error, why:
				print 'Could not copy', srcname, 'to', dstname,
				print '(', why[1], ')'
