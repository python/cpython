# os.py -- either mac, dos or posix depending on what system we're on.

# This exports:
# - all functions from either posix or mac, e.g., os.unlink, os.stat, etc.
# - os.path is either module posixpath or macpath
# - os.name is either 'posix' or 'mac'
# - os.curdir is a string representing the current directory ('.' or ':')
# - os.pardir is a string representing the parent directory ('..' or '::')
# - os.sep is the (or a most common) pathname separator ('/' or ':' or '\\')
# - os.altsep is the alternatte pathname separator (None or '/')
# - os.pathsep is the component separator used in $PATH etc
# - os.defpath is the default search path for executables

# Programs that import and use 'os' stand a better chance of being
# portable between different platforms.  Of course, they must then
# only use functions that are defined by all platforms (e.g., unlink
# and opendir), and leave all pathname manipulation to os.path
# (e.g., split and join).

import sys

_names = sys.builtin_module_names

altsep = None

if 'posix' in _names:
	name = 'posix'
	curdir = '.'; pardir = '..'; sep = '/'; pathsep = ':'
	defpath = ':/bin:/usr/bin'
	from posix import *
	try:
		from posix import _exit
	except ImportError:
		pass
	import posixpath
	path = posixpath
	del posixpath
elif 'nt' in _names:
	name = 'nt'
	curdir = '.'; pardir = '..'; sep = '\\'; pathsep = ';'
	defpath = '.;C:\\bin'
	from nt import *
	try:
		from nt import _exit
	except ImportError:
		pass
	import ntpath
	path = ntpath
	del ntpath
elif 'dos' in _names:
	name = 'dos'
	curdir = '.'; pardir = '..'; sep = '\\'; pathsep = ';'
	defpath = '.;C:\\bin'
	from dos import *
	try:
		from dos import _exit
	except ImportError:
		pass
	import dospath
	path = dospath
	del dospath
elif 'os2' in _names:
	name = 'os2'
	curdir = '.'; pardir = '..'; sep = '\\'; pathsep = ';'
	defpath = '.;C:\\bin'
	from os2 import *
	try:
		from os2 import _exit
	except ImportError:
		pass
	import ntpath
	path = ntpath
	del ntpath
elif 'mac' in _names:
	name = 'mac'
	curdir = ':'; pardir = '::'; sep = ':'; pathsep = '\n'
	defpath = ':'
	from mac import *
	try:
		from mac import _exit
	except ImportError:
		pass
	import macpath
	path = macpath
	del macpath
else:
	raise ImportError, 'no os specific module found'

del _names

# Make sure os.environ exists, at least
try:
	environ
except NameError:
	environ = {}

def execl(file, *args):
	execv(file, args)

def execle(file, *args):
	env = args[-1]
	execve(file, args[:-1], env)

def execlp(file, *args):
	execvp(file, args)

def execlpe(file, *args):
	env = args[-1]
	execvpe(file, args[:-1], env)

def execvp(file, args):
	_execvpe(file, args)

def execvpe(file, args, env):
	_execvpe(file, args, env)

_notfound = None
def _execvpe(file, args, env = None):
	if env:
		func = execve
		argrest = (args, env)
	else:
		func = execv
		argrest = (args,)
		env = environ
	global _notfound
	head, tail = path.split(file)
	if head:
		apply(func, (file,) + argrest)
		return
	if env.has_key('PATH'):
		envpath = env['PATH']
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
			apply(func, (fullname,) + argrest)
		except error, (errno, msg):
			if errno != arg[0]:
				exc, arg = error, (errno, msg)
	raise exc, arg

# Change environ to automatically call putenv() if it exists
try:
	# This will fail if there's no putenv
	putenv
except NameError:
	pass
else:
	import UserDict

	class _Environ(UserDict.UserDict):
		def __init__(self, environ):
			UserDict.UserDict.__init__(self)
			self.data = environ
		def __getinitargs__(self):
			import copy
			return (copy.copy(self.data),)
		def __setitem__(self, key, item):
			putenv(key, item)
			self.data[key] = item
		def __copy__(self):
			return _Environ(self.data.copy())

	environ = _Environ(environ)
