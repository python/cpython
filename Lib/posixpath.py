# Module 'posixpath' -- common operations on Posix pathnames.
# Some of this can actually be useful on non-Posix systems too, e.g.
# for manipulation of the pathname component of URLs.
# The "os.path" name is an alias for this module on Posix systems;
# on other systems (e.g. Mac, Windows), os.path provides the same
# operations in a manner specific to that platform, and is an alias
# to another module (e.g. macpath, ntpath).

import os
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


# Join pathnames.
# Ignore the previous parts if a part is absolute.
# Insert a '/' unless the first part is empty or already ends in '/'.

def join(a, *p):
	path = a
	for b in p:
		if b[:1] == '/':
			path = b
		elif path == '' or path[-1:] == '/':
			path = path + b
		else:
			path = path + '/' + b
	return path


# Split a path in head (everything up to the last '/') and tail (the
# rest).  If the path ends in '/', tail will be empty.  If there is no
# '/' in the path, head  will be empty.
# Trailing '/'es are stripped from head unless it is the root.

def split(p):
	import string
	i = string.rfind(p, '/') + 1
	head, tail = p[:i], p[i:]
	if head and head <> '/'*len(head):
		while head[-1] == '/':
			head = head[:-1]
	return head, tail


# Split a path in root and extension.
# The extension is everything starting at the last dot in the last
# pathname component; the root is everything before that.
# It is always true that root + ext == p.

def splitext(p):
	root, ext = '', ''
	for c in p:
		if c == '/':
			root, ext = root + ext + c, ''
		elif c == '.':
			if ext:
				root, ext = root + ext, c
			else:
				ext = c
		elif ext:
			ext = ext + c
		else:
			root = root + c
	return root, ext


# Split a pathname into a drive specification and the rest of the
# path.  Useful on DOS/Windows/NT; on Unix, the drive is always empty.

def splitdrive(p):
	return '', p


# Return the tail (basename) part of a path.

def basename(p):
	return split(p)[1]


# Return the head (dirname) part of a path.

def dirname(p):
	return split(p)[0]


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
# This will always return false on systems where os.lstat doesn't exist.

def islink(path):
	try:
		st = os.lstat(path)
	except (os.error, AttributeError):
		return 0
	return stat.S_ISLNK(st[stat.ST_MODE])


# Does a path exist?
# This is false for dangling symbolic links.

def exists(path):
	try:
		st = os.stat(path)
	except os.error:
		return 0
	return 1


# Is a path a directory?
# This follows symbolic links, so both islink() and isdir() can be true
# for the same path.

def isdir(path):
	try:
		st = os.stat(path)
	except os.error:
		return 0
	return stat.S_ISDIR(st[stat.ST_MODE])


# Is a path a regular file?
# This follows symbolic links, so both islink() and isfile() can be true
# for the same path.

def isfile(path):
	try:
		st = os.stat(path)
	except os.error:
		return 0
	return stat.S_ISREG(st[stat.ST_MODE])


# Are two filenames really pointing to the same file?

def samefile(f1, f2):
	s1 = os.stat(f1)
	s2 = os.stat(f2)
	return samestat(s1, s2)


# Are two open files really referencing the same file?
# (Not necessarily the same file descriptor!)

def sameopenfile(fp1, fp2):
	s1 = os.fstat(fp1)
	s2 = os.fstat(fp2)
	return samestat(s1, s2)


# Are two stat buffers (obtained from stat, fstat or lstat)
# describing the same file?

def samestat(s1, s2):
	return s1[stat.ST_INO] == s2[stat.ST_INO] and \
		s1[stat.ST_DEV] == s2[stat.ST_DEV]


# Is a path a mount point?
# (Does this work for all UNIXes?  Is it even guaranteed to work by Posix?)

def ismount(path):
	try:
		s1 = os.stat(path)
		s2 = os.stat(join(path, '..'))
	except os.error:
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
		names = os.listdir(top)
	except os.error:
		return
	func(arg, top, names)
	exceptions = ('.', '..')
	for name in names:
		if name not in exceptions:
			name = join(top, name)
			if isdir(name) and not islink(name):
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
		if not os.environ.has_key('HOME'):
			return path
		userhome = os.environ['HOME']
	else:
		import pwd
		try:
			pwent = pwd.getpwnam(path[1:i])
		except KeyError:
			return path
		userhome = pwent[5]
	if userhome[-1:] == '/': i = i+1
	return userhome + path[i:]


# Expand paths containing shell variable substitutions.
# This expands the forms $variable and ${variable} only.
# Non-existant variables are left unchanged.

_varprog = None

def expandvars(path):
	global _varprog
	if '$' not in path:
		return path
	if not _varprog:
		import re
		_varprog = re.compile(r'\$(\w+|\{[^}]*\})')
	i = 0
	while 1:
		m = _varprog.search(path, i)
		if not m:
			break
		i, j = m.span(0)
		name = m.group(1)
		if name[:1] == '{' and name[-1:] == '}':
			name = name[1:-1]
		if os.environ.has_key(name):
			tail = path[j:]
			path = path[:i] + os.environ[name]
			i = len(path)
			path = path + tail
		else:
			i = j
	return path


# Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A/B.
# It should be understood that this may change the meaning of the path
# if it contains symbolic links!

def normpath(path):
	import string
	# Treat initial slashes specially
	slashes = ''
	while path[:1] == '/':
		slashes = slashes + '/'
		path = path[1:]
	comps = string.splitfields(path, '/')
	i = 0
	while i < len(comps):
		if comps[i] == '.':
			del comps[i]
		elif comps[i] == '..' and i > 0 and \
					  comps[i-1] not in ('', '..'):
			del comps[i-1:i+1]
			i = i-1
		elif comps[i] == '' and i > 0 and comps[i-1] <> '':
			del comps[i]
		else:
			i = i+1
	# If the path is now empty, substitute '.'
	if not comps and not slashes:
		comps.append('.')
	return slashes + string.joinfields(comps, '/')
