"""Restricted execution facilities.

The class RExec exports methods rexec(), reval(), rexecfile(), and
import_module(), which correspond roughly to the built-in operations
exec, eval(), execfile() and import, but executing the code in an
environment that only exposes those built-in operations that are
deemed safe.  To this end, a modest collection of 'fake' modules is
created which mimics the standard modules by the same names.  It is a
policy decision which built-in modules and operations are made
available; this module provides a reasonable default, but derived
classes can change the policies e.g. by overriding or extending class
variables like ok_builtin_modules or methods like make_sys().

XXX To do:
- r_open should allow writing tmp dir
- r_exec etc. with explicit globals/locals? (Use rexec("exec ... in ...")?)
- r_reload should reload from same location (that's one for ihooks?)

"""


import sys
import __builtin__
import os
import marshal
import ihooks


class FileBase:

	ok_file_methods = ('fileno', 'flush', 'isatty', 'read', 'readline',
		'readlines', 'seek', 'tell', 'write', 'writelines')


class FileWrapper(FileBase):

	def __init__(self, f):
		self.f = f
		for m in self.ok_file_methods:
			if not hasattr(self, m):
				setattr(self, m, getattr(f, m))
	
	def close(f):
		self.flush()


TEMPLATE = """
def %s(self, *args):
	return apply(getattr(self.mod, self.name).%s, args)
"""

class FileDelegate(FileBase):

	def __init__(self, mod, name):
		self.mod = mod
		self.name = name
	
	for m in FileBase.ok_file_methods + ('close',):
		exec TEMPLATE % (m, m)


class RHooks(ihooks.Hooks):

    def __init__(self, rexec, verbose=0):
	ihooks.Hooks.__init__(self, verbose)
	self.rexec = rexec

    def is_builtin(self, name):
	return self.rexec.is_builtin(name)

    def init_builtin(self, name):
	m = __import__(name)
	return self.rexec.copy_except(m, ())

    def init_frozen(self, name): raise SystemError, "don't use this"
    def load_source(self, *args): raise SystemError, "don't use this"
    def load_compiled(self, *args): raise SystemError, "don't use this"

    def load_dynamic(self, *args):
	raise ImportError, "import of dynamically loaded modules not allowed"

    def add_module(self, name):
	return self.rexec.add_module(name)

    def modules_dict(self):
	return self.rexec.modules

    def default_path(self):
	return self.rexec.modules['sys'].path


class RModuleLoader(ihooks.FancyModuleLoader):

    def load_module(self, name, stuff):
        file, filename, info = stuff
        m = ihooks.FancyModuleLoader.load_module(self, name, stuff)
        m.__filename__ = filename
        return m


class RModuleImporter(ihooks.ModuleImporter):

    def reload(self, module, path=None):
        if path is None and hasattr(module, '__filename__'):
            path = [module.__filename__]
        return ihooks.ModuleImporter.reload(self, module, path)


class RExec(ihooks._Verbose):

    """Restricted Execution environment."""

    ok_path = tuple(sys.path)		# That's a policy decision

    ok_builtin_modules = ('array', 'binascii', 'audioop', 'imageop',
			  'marshal', 'math',
			  'md5', 'parser', 'regex', 'rotor', 'select',
			  'strop', 'struct', 'time')

    ok_posix_names = ('error', 'fstat', 'listdir', 'lstat', 'readlink',
		      'stat', 'times', 'uname', 'getpid', 'getppid',
		      'getcwd', 'getuid', 'getgid', 'geteuid', 'getegid')

    ok_sys_names = ('ps1', 'ps2', 'copyright', 'version',
		    'platform', 'exit', 'maxint')

    nok_builtin_names = ('open', 'reload', '__import__')

    def __init__(self, hooks = None, verbose = 0):
	ihooks._Verbose.__init__(self, verbose)
	# XXX There's a circular reference here:
	self.hooks = hooks or RHooks(self, verbose)
	self.modules = {}
	self.ok_builtin_modules = map(None, filter(
	    lambda mname: mname in sys.builtin_module_names,
	    self.ok_builtin_modules))
	self.make_builtin()
	self.make_initial_modules()
	# make_sys must be last because it adds the already created
	# modules to its builtin_module_names
	self.make_sys()
	self.loader = RModuleLoader(self.hooks, verbose)
	self.importer = RModuleImporter(self.loader, verbose)

    def make_initial_modules(self):
	self.make_main()
	self.make_osname()

    # Helpers for RHooks

    def is_builtin(self, mname):
	return mname in self.ok_builtin_modules

    # The make_* methods create specific built-in modules

    def make_builtin(self):
	m = self.copy_except(__builtin__, self.nok_builtin_names)
	m.__import__ = self.r_import
	m.reload = self.r_reload
	m.open = self.r_open

    def make_main(self):
	m = self.add_module('__main__')

    def make_osname(self):
	osname = os.name
	src = __import__(osname)
	dst = self.copy_only(src, self.ok_posix_names)
	dst.environ = e = {}
	for key, value in os.environ.items():
	    e[key] = value

    def make_sys(self):
	m = self.copy_only(sys, self.ok_sys_names)
	m.modules = self.modules
	m.argv = ['RESTRICTED']
	m.path = map(None, self.ok_path)
	m = self.modules['sys']
	m.builtin_module_names = \
		self.modules.keys() + self.ok_builtin_modules
	m.builtin_module_names.sort()

    # The copy_* methods copy existing modules with some changes

    def copy_except(self, src, exceptions):
	dst = self.copy_none(src)
	for name in dir(src):
	    if name not in exceptions:
		setattr(dst, name, getattr(src, name))
	return dst

    def copy_only(self, src, names):
	dst = self.copy_none(src)
	for name in names:
	    try:
		value = getattr(src, name)
	    except AttributeError:
		continue
	    setattr(dst, name, value)
	return dst

    def copy_none(self, src):
	return self.add_module(src.__name__)

    # Add a module -- return an existing module or create one

    def add_module(self, mname):
	if self.modules.has_key(mname):
	    return self.modules[mname]
	self.modules[mname] = m = self.hooks.new_module(mname)
	m.__builtins__ = self.modules['__builtin__']
	return m

    # The r* methods are public interfaces

    def r_exec(self, code):
	m = self.add_module('__main__')
	exec code in m.__dict__

    def r_eval(self, code):
	m = self.add_module('__main__')
	return eval(code, m.__dict__)

    def r_execfile(self, file):
	m = self.add_module('__main__')
	return execfile(file, m.__dict__)

    def r_import(self, mname, globals={}, locals={}, fromlist=[]):
	return self.importer.import_module(mname, globals, locals, fromlist)

    def r_reload(self, m):
        return self.importer.reload(m)

    def r_unload(self, m):
        return self.importer.unload(m)
    
    # The s_* methods are similar but also swap std{in,out,err}

    def set_files(self):
        s = self.modules['sys']
        s.stdin = FileWrapper(sys.stdin)
        s.stdout = FileWrapper(sys.stdout)
        s.stderr = FileWrapper(sys.stderr)
	sys.stdin = FileDelegate(s, 'stdin')
	sys.stdout = FileDelegate(s, 'stdout')
	sys.stderr = FileDelegate(s, 'stderr')

    def save_files(self):
        self.save_stdin = sys.stdin
        self.save_stdout = sys.stdout
        self.save_stderr = sys.stderr

    def restore_files(files):
	sys.stdin = self.save_sydin
	sys.stdout = self.save_stdout
	sys.stderr = self.save_stderr
    
    def s_apply(self, func, *args, **kw):
	self.save_files()
	try:
	    self.set_files()
	    r = apply(func, args, kw)
        finally:
	    self.restore_files()
    
    def s_exec(self, *args):
        self.s_apply(self.r_exec, args)
    
    def s_eval(self, *args):
        self.s_apply(self.r_eval, args)
    
    def s_execfile(self, *args):
        self.s_apply(self.r_execfile, args)
    
    def s_import(self, *args):
        self.s_apply(self.r_import, args)
    
    def s_reload(self, *args):
        self.s_apply(self.r_reload, args)
    
    def s_unload(self, *args):
        self.s_apply(self.r_unload, args)
    
    # Restricted open(...)
    
    def r_open(self, file, mode='r', buf=-1):
        if mode not in ('r', 'rb'):
            raise IOError, "can't open files for writing in restricted mode"
        return open(file, mode, buf)


def test():
    import traceback
    r = RExec(None, '-v' in sys.argv[1:])
    print "*** RESTRICTED *** Python", sys.version
    print sys.copyright
    while 1:
	try:
	    try:
		s = raw_input('>>> ')
	    except EOFError:
		print
		break
	    if s and s[0] != '#':
		s = s + '\n'
		c = compile(s, '<stdin>', 'single')
		r.r_exec(c)
	except SystemExit, n:
	    sys.exit(n)
	except:
	    traceback.print_exc()


if __name__ == '__main__':
    test()
