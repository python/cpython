# Module 'posixpath' -- common operations on POSIX pathnames

import posix
import stat


# Normalize the case of a pathname.  Trivial in Posix, string.lower on Mac.
# On MS-DOS this may also turn slashes into backslashes; however, other
# normalizations (such as optimizing '../' away) are not allowed
# (another function should be defined to do that).

def normcase(s):
	return s


# Return wheter a path is absolute.
# Trivial in Posix, harder on the Mac or MS-DOS.

def isabs(s):
	return s[:1] == '/'


# Join two pathnames.
# Ignore the first part if the second part is absolute.
# Insert a '/' unless the first part is empty or already ends in '/'.

def join(a, b):
	if b[:1] == '/': return b
	if a == '' or a[-1:] == '/': return a + b
	# Note: join('x', '') returns 'x/'; is this what we want?
	return a + '/' + b


# Split a path in head (everything up to the last '/') and tail (the
# rest).  If the original path ends in '/' but is not the root, this
# '/' is stripped.  After the trailing '/' is stripped, the invariant
# join(head, tail) == p holds.
# The resulting head won't end in '/' unless it is the root.

def split(p):
	if p[-1:] == '/' and p <> '/'*len(p):
		while p[-1] == '/':
			p = p[:-1]
	head, tail = '', ''
	for c in p:
		tail = tail + c
		if c == '/':
			head, tail = head + tail, ''
	if head[-1:] == '/' and head <> '/'*len(head):
		while head[-1] == '/':
			head = head[:-1]
	return head, tail


# Split a path in root and extension.
# The extension is everything starting at the first dot in the last
# pathname component; the root is everything before that.
# It is always true that root + ext == p.

def splitext(p):
	root, ext = '', ''
	for c in p:
		if c == '/':
			root, ext = root + ext + c, ''
		elif c == '.' or ext:
			ext = ext + c
		else:
			root = root + c
	return root, ext


# Return the tail (basename) part of a path.

def basename(p):
	return split(p)[1]


# Return the longest prefix of all list elements.

def commonprefix(m):
	if not m: return ''
	prefix = m[0]
	for item in m:
		for i in range(len(prefix)):
			if prefix[:i+1] <> item[:i+1]:
				prefix = prefix[:i]
				if i == 0: return ''
				break
	return prefix


# Is a path a symbolic link?
# This will always return false on systems where posix.lstat doesn't exist.

def islink(path):
	try:
		st = posix.lstat(path)
	except (posix.error, AttributeError):
		return 0
	return stat.S_ISLNK(st[stat.ST_MODE])


# Does a path exist?
# This is false for dangling symbolic links.

def exists(path):
	try:
		st = posix.stat(path)
	except posix.error:
		return 0
	return 1


# Is a path a posix directory?
# This follows symbolic links, so both islink() and isdir() can be true
# for the same path.

def isdir(path):
	try:
		st = posix.stat(path)
	except posix.error:
		return 0
	return stat.S_ISDIR(st[stat.ST_MODE])


# Is a path a regular file?
# This follows symbolic links, so both islink() and isdir() can be true
# for the same path.

def isfile(path):
	try:
		st = posix.stat(path)
	except posix.error:
		return 0
	return stat.S_ISREG(st[stat.ST_MODE])


# Are two filenames really pointing to the same file?

def samefile(f1, f2):
	s1 = posix.stat(f1)
	s2 = posix.stat(f2)
	return samestat(s1, s2)


# Are two open files really referencing the same file?
# (Not necessarily the same file descriptor!)
# XXX Oops, posix.fstat() doesn't exist yet!

def sameopenfile(fp1, fp2):
	s1 = posix.fstat(fp1)
	s2 = posix.fstat(fp2)
	return samestat(s1, s2)


# Are two stat buffers (obtained from stat, fstat or lstat)
# describing the same file?

def samestat(s1, s2):
	return s1[stat.ST_INO] == s2[stat.ST_INO] and \
		s1[stat.ST_DEV] == s2[stat.ST_DEV]


# Is a path a mount point?
# (Does this work for all UNIXes?  Is it even guaranteed to work by POSIX?)

def ismount(path):
	try:
		s1 = posix.stat(path)
		s2 = posix.stat(join(path, '..'))
	except posix.error:
		return 0 # It doesn't exist -- so not a mount point :-)
	dev1 = s1[stat.ST_DEV]
	dev2 = s2[stat.ST_DEV]
	if dev1 != dev2:
		return 1		# path/.. on a different device as path
	ino1 = s1[stat.ST_INO]
	ino2 = s2[stat.ST_INO]
	if ino1 == ino2:
		return 1		# path/.. is the same i-node as path
	return 0


# Directory tree walk.
# For each directory under top (including top itself, but excluding
# '.' and '..'), func(arg, dirname, filenames) is called, where
# dirname is the name of the directory and filenames is the list
# files files (and subdirectories etc.) in the directory.
# The func may modify the filenames list, to implement a filter,
# or to impose a different order of visiting.

def walk(top, func, arg):
	try:
		names = posix.listdir(top)
	except posix.error:
		return
	func(arg, top, names)
	exceptions = ('.', '..')
	for name in names:
		if name not in exceptions:
			name = join(top, name)
			if isdir(name):
				walk(name, func, arg)


# Expand paths beginning with '~' or '~user'.
# '~' means $HOME; '~user' means that user's home directory.
# If the path doesn't begin with '~', or if the user or $HOME is unknown,
# the path is returned unchanged (leaving error reporting to whatever
# function is called with the expanded path as argument).
# See also module 'glob' for expansion of *, ? and [...] in pathnames.
# (A function should also be defined to do full *sh-style environment
# variable expansion.)

def expanduser(path):
	if path[:1] <> '~':
		return path
	i, n = 1, len(path)
	while i < n and path[i] <> '/':
		i = i+1
	if i == 1:
		if not posix.environ.has_key('HOME'):
			return path
		userhome = posix.environ['HOME']
	else:
		import pwd
		try:
			pwent = pwd.getpwnam(path[1:i])
		except KeyError:
			return path
		userhome = pwent[5]
	return userhome + path[i:]
