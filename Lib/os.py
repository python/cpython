# os.py -- either mac, dos or posix depending on what system we're on.

# This exports:
# - all functions from either posix or mac, e.g., os.unlink, os.stat, etc.
# - os.path is either module posixpath or macpath
# - os.name is either 'posix' or 'mac'
# - os.curdir is a string representing the current directory ('.' or ':')
# - os.pardir is a string representing the parent directory ('..' or '::')
# - os.sep is the (or a most common) pathname separator ('/' or ':')
# - os.pathsep is the component separator used in $PATH etc
# - os.defpath is the default search path for executables

# Programs that import and use 'os' stand a better chance of being
# portable between different platforms.  Of course, they must then
# only use functions that are defined by all platforms (e.g., unlink
# and opendir), and leave all pathname manipulation to os.path
# (e.g., split and join).

_osindex = {
	  'posix': ('.', '..', '/', ':', ':/bin:/usr/bin'),
	  'dos':   ('.', '..', '\\', ';', '.;C:\\bin'),
	  'nt':    ('.', '..', '\\', ';', '.;C:\\bin'),
	  'mac':   (':', '::', ':', ' ', ':'),
}

# For freeze.py script:
if 0:
	import posix

import sys
for name in _osindex.keys():
	if name in sys.builtin_module_names:
		curdir, pardir, sep, pathsep, defpath = _osindex[name]
		exec 'from %s import *' % name
		exec 'import %spath' % name
		exec 'path = %spath' % name
		exec 'del %spath' % name
		try:
			exec 'from %s import _exit' % name
		except ImportError:
			pass
		break
else:
	del name
	raise ImportError, 'no os specific module found'

def execl(file, *args):
	execv(file, args)

def execle(file, *args):
	env = args[-1]
	execve(file, args[:-1], env)

def execlp(file, *args):
	execvp(file, args)

_notfound = None
def execvp(file, args):
	global _notfound
	head, tail = path.split(file)
	if head:
		execv(file, args)
		return
	ENOENT = 2
	if environ.has_key('PATH'):
		envpath = environ['PATH']
	else:
		envpath = defpath
	import string
	PATH = string.splitfields(envpath, pathsep)
	if not _notfound:
		import tempfile
		# Exec a file that is guaranteed not to exist
		try: execv(tempfile.mktemp(), ())
		except error, _notfound: pass
	exc, arg = error, _notfound
	for dir in PATH:
		fullname = path.join(dir, file)
		try:
			execv(fullname, args)
		except error, (errno, msg):
			if errno != arg[0]:
				exc, arg = error, (errno, msg)
	raise exc, arg

# Provide listdir for Windows NT that doesn't have it built in
if name == 'nt':
	try:
		_tmp = listdir
		del _tmp
	except NameError:
		def listdir(name):
			if path.ismount(name):
				list = ['.']
			else:
				list = ['.', '..']
			f = popen('dir/l/b ' + name, 'r')
			line = f.readline()
			while line:
				list.append(line[:-1])
				line = f.readline()
			return list
