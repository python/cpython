"""Restricted execution facilities.

The class RExec exports methods r_exec(), r_eval(), r_execfile(), and
r_import(), which correspond roughly to the built-in operations
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

"""


import sys
import __builtin__
import os
import ihooks


class FileBase:

    ok_file_methods = ('fileno', 'flush', 'isatty', 'read', 'readline',
            'readlines', 'seek', 'tell', 'write', 'writelines')


class FileWrapper(FileBase):

    # XXX This is just like a Bastion -- should use that!

    def __init__(self, f):
        self.f = f
        for m in self.ok_file_methods:
            if not hasattr(self, m) and hasattr(f, m):
                setattr(self, m, getattr(f, m))

    def close(self):
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

    def __init__(self, *args):
        # Hacks to support both old and new interfaces:
        # old interface was RHooks(rexec[, verbose])
        # new interface is RHooks([verbose])
        verbose = 0
        rexec = None
        if args and type(args[-1]) == type(0):
            verbose = args[-1]
            args = args[:-1]
        if args and hasattr(args[0], '__class__'):
            rexec = args[0]
            args = args[1:]
        if args:
            raise TypeError, "too many arguments"
        ihooks.Hooks.__init__(self, verbose)
        self.rexec = rexec

    def set_rexec(self, rexec):
        # Called by RExec instance to complete initialization
        self.rexec = rexec

    def is_builtin(self, name):
        return self.rexec.is_builtin(name)

    def init_builtin(self, name):
        m = __import__(name)
        return self.rexec.copy_except(m, ())

    def init_frozen(self, name): raise SystemError, "don't use this"
    def load_source(self, *args): raise SystemError, "don't use this"
    def load_compiled(self, *args): raise SystemError, "don't use this"
    def load_package(self, *args): raise SystemError, "don't use this"

    def load_dynamic(self, name, filename, file):
        return self.rexec.load_dynamic(name, filename, file)

    def add_module(self, name):
        return self.rexec.add_module(name)

    def modules_dict(self):
        return self.rexec.modules

    def default_path(self):
        return self.rexec.modules['sys'].path


# XXX Backwards compatibility
RModuleLoader = ihooks.FancyModuleLoader
RModuleImporter = ihooks.ModuleImporter


class RExec(ihooks._Verbose):

    """Restricted Execution environment."""

    ok_path = tuple(sys.path)           # That's a policy decision

    ok_builtin_modules = ('audioop', 'array', 'binascii',
                          'cmath', 'errno', 'imageop',
                          'marshal', 'math', 'md5', 'operator',
                          'parser', 'regex', 'pcre', 'rotor', 'select',
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
        self.hooks = hooks or RHooks(verbose)
        self.hooks.set_rexec(self)
        self.modules = {}
        self.ok_dynamic_modules = self.ok_builtin_modules
        list = []
        for mname in self.ok_builtin_modules:
            if mname in sys.builtin_module_names:
                list.append(mname)
        self.ok_builtin_modules = tuple(list)
        self.set_trusted_path()
        self.make_builtin()
        self.make_initial_modules()
        # make_sys must be last because it adds the already created
        # modules to its builtin_module_names
        self.make_sys()
        self.loader = RModuleLoader(self.hooks, verbose)
        self.importer = RModuleImporter(self.loader, verbose)
        # but since re isn't normally built-in, we can add it at the end;
        # we need the imported to be set before this can be imported.
        self.make_re()

    def set_trusted_path(self):
        # Set the path from which dynamic modules may be loaded.
        # Those dynamic modules must also occur in ok_builtin_modules
        self.trusted_path = filter(os.path.isabs, sys.path)

    def load_dynamic(self, name, filename, file):
        if name not in self.ok_dynamic_modules:
            raise ImportError, "untrusted dynamic module: %s" % name
        if sys.modules.has_key(name):
            src = sys.modules[name]
        else:
            import imp
            src = imp.load_dynamic(name, filename, file)
        dst = self.copy_except(src, [])
        return dst

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

    def make_re(self):
        dst = self.add_module("re")
        src = self.r_import("pre")
        for name in dir(src):
            if name != "__name__":
                setattr(dst, name, getattr(src, name))

    def make_sys(self):
        m = self.copy_only(sys, self.ok_sys_names)
        m.modules = self.modules
        m.argv = ['RESTRICTED']
        m.path = map(None, self.ok_path)
        m.exc_info = self.r_exc_info
        m = self.modules['sys']
        l = self.modules.keys() + list(self.ok_builtin_modules)
        l.sort()
        m.builtin_module_names = tuple(l)

    # The copy_* methods copy existing modules with some changes

    def copy_except(self, src, exceptions):
        dst = self.copy_none(src)
        for name in dir(src):
            setattr(dst, name, getattr(src, name))
        for name in exceptions:
            try:
                delattr(dst, name)
            except AttributeError:
                pass
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
        m = self.add_module(src.__name__)
        m.__doc__ = src.__doc__
        return m

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

    def make_delegate_files(self):
        s = self.modules['sys']
        self.delegate_stdin = FileDelegate(s, 'stdin')
        self.delegate_stdout = FileDelegate(s, 'stdout')
        self.delegate_stderr = FileDelegate(s, 'stderr')
        self.restricted_stdin = FileWrapper(sys.stdin)
        self.restricted_stdout = FileWrapper(sys.stdout)
        self.restricted_stderr = FileWrapper(sys.stderr)

    def set_files(self):
        if not hasattr(self, 'save_stdin'):
            self.save_files()
        if not hasattr(self, 'delegate_stdin'):
            self.make_delegate_files()
        s = self.modules['sys']
        s.stdin = self.restricted_stdin
        s.stdout = self.restricted_stdout
        s.stderr = self.restricted_stderr
        sys.stdin = self.delegate_stdin
        sys.stdout = self.delegate_stdout
        sys.stderr = self.delegate_stderr

    def reset_files(self):
        self.restore_files()
        s = self.modules['sys']
        self.restricted_stdin = s.stdin
        self.restricted_stdout = s.stdout
        self.restricted_stderr = s.stderr
        

    def save_files(self):
        self.save_stdin = sys.stdin
        self.save_stdout = sys.stdout
        self.save_stderr = sys.stderr

    def restore_files(self):
        sys.stdin = self.save_stdin
        sys.stdout = self.save_stdout
        sys.stderr = self.save_stderr
    
    def s_apply(self, func, args=(), kw=None):
        self.save_files()
        try:
            self.set_files()
            if kw:
                r = apply(func, args, kw)
            else:
                r = apply(func, args)
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

    # Restricted version of sys.exc_info()

    def r_exc_info(self):
        ty, va, tr = sys.exc_info()
        tr = None
        return ty, va, tr


def test():
    import sys, getopt, traceback
    opts, args = getopt.getopt(sys.argv[1:], 'vt:')
    verbose = 0
    trusted = []
    for o, a in opts:
        if o == '-v':
            verbose = verbose+1
        if o == '-t':
            trusted.append(a)
    r = RExec(verbose=verbose)
    if trusted:
        r.ok_builtin_modules = r.ok_builtin_modules + tuple(trusted)
    if args:
        r.modules['sys'].argv = args
        r.modules['sys'].path.insert(0, os.path.dirname(args[0]))
    else:
        r.modules['sys'].path.insert(0, "")
    fp = sys.stdin
    if args and args[0] != '-':
        try:
            fp = open(args[0])
        except IOError, msg:
            print "%s: can't open file %s" % (sys.argv[0], `args[0]`)
            return 1
    if fp.isatty():
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
                    r.s_exec(c)
            except SystemExit, n:
                return n
            except:
                traceback.print_exc()
    else:
        text = fp.read()
        fp.close()
        c = compile(text, fp.name, 'exec')
        try:
            r.s_exec(c)
        except SystemExit, n:
            return n
        except:
            traceback.print_exc()
            return 1


if __name__ == '__main__':
    sys.exit(test())
