# Module 'path' -- common operations on POSIX pathnames

import posix
import stat


# Intelligent pathname concatenation.
# Inserts a '/' unless the first part is empty or already ends in '/'.
# Ignores the first part altogether if the second part is absolute
# (begins with '/').
#
def cat(a, b):
	if b[:1] = '/': return b
	if a = '' or a[-1:] = '/': return a + b
	return a + '/' + b


# Split a path in head (empty or ending in '/') and tail (no '/').
# The tail will be empty if the path ends in '/'.
#
def split(p):
	head, tail = '', ''
	for c in p:
		tail = tail + c
		if c = '/':
			head, tail = head + tail, ''
	return head, tail


# Return the tail (basename) part of a path.
#
def basename(p):
	return split(p)[1]


# Return the longest prefix of all list elements.
#
def commonprefix(m):
	if not m: return ''
	prefix = m[0]
	for item in m:
		for i in range(len(prefix)):
			if prefix[:i+1] <> item[:i+1]:
				prefix = prefix[:i]
				if i = 0: return ''
				break
	return prefix


# Does a file/directory exist?
#
def exists(path):
	try:
		st = posix.stat(path)
	except posix.error:
		return 0
	return 1


# Is a path a posix directory?
#
def isdir(path):
	try:
		st = posix.stat(path)
	except posix.error:
		return 0
	return stat.S_ISDIR(st[stat.ST_MODE])


# Is a path a symbolic link?
# This will always return false on systems where posix.lstat doesn't exist.
#
def islink(path):
	try:
		st = posix.lstat(path)
	except (posix.error, NameError):
		return 0
	return stat.S_ISLNK(st[stat.ST_MODE])


_mounts = []

def _getmounts():
	import commands, string
	mounts = []
	data = commands.getoutput('/etc/mount')
	lines = string.splitfields(data, '\n')
	for line in lines:
		words = string.split(line)
		if len(words) >= 3 and words[1] = 'on':
			mounts.append(words[2])
	return mounts


# Is a path a mount point?
# This only works for normalized, absolute paths,
# and only if the mount table as printed by /etc/mount is correct.
# Sorry.
#
def ismount(path):
	if not _mounts:
		_mounts[:] = _getmounts()
	return path in _mounts


# Directory tree walk.
# For each directory under top (including top itself),
# func(arg, dirname, filenames) is called, where dirname
# is the name of the directory and filenames is the list of
# files (and subdirectories etc.) in the directory.
# func may modify the filenames list, to implement a filter,
# or to impose a different order of visiting.
#
def walk(top, func, arg):
	try:
		names = posix.listdir(top)
	except posix.error:
		return
	func(arg, top, names)
	exceptions = ('.', '..')
	for name in names:
		if name not in exceptions:
			name = cat(top, name)
			if isdir(name):
				walk(name, func, arg)
