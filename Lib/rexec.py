# Implement restricted execution of Python code

import __builtin__
import new
import os
import sys
import types

def trace(fmt, *args):
	if 0:
		sys.stderr.write(fmt % args + '\n')

def copydict(src, dst, exceptions = [], only = None):
	if only is None:
		for key in src.keys():
			if key not in exceptions:
				dst[key] = src[key]
	else:
		for key in only:
			dst[key] = src[key]

def copymodule(src, dst, exceptions = [], only = None):
	copydict(src.__dict__, dst.__dict__, exceptions, only)

safe_path = ['/ufs/guido/lib/python']
safe_modules = ['array', 'math', 'regex', 'strop', 'time']
unsafe_builtin_names = ['open', 'reload', '__import__',
			'eval', 'execfile', 'dir', 'vars',
			'raw_input', 'input']
safe_posix_names = ['error', 'fstat', 'listdir', 'lstat', 'readlink', 'stat',
		    'times', 'uname', 'getpid', 'getppid', 'getcwd',
		    'getuid', 'getgid', 'geteuid', 'getegid']

safe_sys = new.module('sys')
safe_sys.modules = {}
safe_sys.modules['sys'] = safe_sys
safe_sys.path = safe_path[:]
safe_sys.argv = ['-']
safe_sys.builtin_module_names = safe_modules[:] + ['posix']
safe_sys.builtin_module_names.sort()
safe_sys.copyright = sys.copyright
safe_sys.version = sys.version + ' [restricted mode]'
safe_sys.exit = sys.exit

def new_module(name):
	safe_sys.modules[name] = m = new.module(name)
	return m

safe_builtin = new_module('__builtin__')
copymodule(__builtin__, safe_builtin, unsafe_builtin_names)

safe_main = new_module('__main__')

safe_posix = new_module('posix')
import posix
copymodule(posix, safe_posix, None, safe_posix_names)
safe_posix.environ = {}
copydict(posix.environ, safe_posix.environ)

safe_types = new_module('types')
copymodule(types, safe_types)

def safe_import(name):
	if safe_sys.modules.has_key(name):
		return safe_sys.modules[name]
	if name in safe_modules:
		temp = {}
		exec "import "+name in temp
		m = new_module(name)
		copymodule(temp[name], m)
		return m
	for dirname in safe_path:
		filename = os.path.join(dirname, name + '.py')
		try:
			f = open(filename, 'r')
			f.close()
		except IOError:
			continue
		m = new_module(name)
		rexecfile(filename, m.__dict__)
		return m
	raise ImportError, name
safe_builtin.__import__ = safe_import

def safe_open(file, mode = 'r'):
	if type(file) != types.StringType or type(mode) != types.StringType:
		raise TypeError, 'open argument(s) must be string(s)'
	if mode not in ('r', 'rb'):
		raise IOError, 'open for writing not allowed'
	if '/' in file:
		raise IOError, 'open pathname not allowed'
	return open(file, mode)
safe_builtin.open = safe_open

def safe_dir(object = safe_main):
	keys = object.__dict__.keys()
	keys.sort()
	return keys
safe_builtin.dir = safe_dir

def safe_vars(object = safe_main):
	keys = safe_dir(object)
	dict = {}
	copydict(object.__dict__, dict, None, keys)
	return dict
safe_builtin.vars = safe_vars


def exterior():
	"""Return env of caller's caller, as triple: (name, locals, globals).

	Name will be None if env is __main__, and locals will be None if same
	as globals, ie local env is global env."""

	import sys, __main__

	bogus = 'bogus'			# A locally usable exception
	try: raise bogus		# Force an exception
	except bogus:
		at = sys.exc_traceback.tb_frame.f_back # The external frame.
		if at.f_back: at = at.f_back # And further, if any.
		where, globals, locals = at.f_code, at.f_globals, at.f_locals
		if locals == globals:	# Exterior is global?
			locals = None
		if where:
			where = where.co_name
		return (where, locals, globals)


def rexec(str, globals = None, locals = None):
	trace('rexec(%s, ...)', `str`)
	if globals is None:
		globals = locals = exterior()[2]
	elif locals is None:
		locals = globals
	globals['__builtins__'] = safe_builtin.__dict__
	safe_sys.stdout = sys.stdout
	safe_sys.stderr = sys.stderr
	exec str in globals, locals

def rexecfile(file, globals = None, locals = None):
	trace('rexecfile(%s, ...)', `file`)
	if globals is None:
		globals = locals = exterior()[2]
	elif locals is None:
		locals = globals
	globals['__builtins__'] = safe_builtin.__dict__
	safe_sys.stdout = sys.stdout
	safe_sys.stderr = sys.stderr
	return execfile(file, globals, locals)

def reval(str, globals = None, locals = None):
	trace('reval(%s, ...)', `str`)
	if globals is None:
		globals = locals = exterior()[2]
	elif locals is None:
		locals = globals
	globals['__builtins__'] = safe_builtin.__dict__
	safe_sys.stdout = sys.stdout
	safe_sys.stderr = sys.stderr
	return eval(str, globals, locals)
safe_builtin.eval = reval


def test():
	import traceback
	g = {}
	while 1:
		try:
			s = raw_input('--> ')
		except EOFError:
			break
		try:
			try:
				c = compile(s, '', 'eval')
			except:
				rexec(s, g)
			else:
				print reval(c, g)
		except:
			traceback.print_exc()

if __name__ == '__main__':
	test()
