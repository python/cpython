"""Import hook support.

Consistent use of this module will make it possible to change the
different mechanisms involved in loading modules independently.

While the built-in module imp exports interfaces to the built-in
module searching and loading algorithm, and it is possible to replace
the built-in function __import__ in order to change the semantics of
the import statement, until now it has been difficult to combine the
effect of different __import__ hacks, like loading modules from URLs
(rimport.py), implementing a hierarchical module namespace (newimp.py)
or restricted execution (rexec.py).

This module defines three new concepts:

(1) A "file system hooks" class provides an interface to a filesystem.

One hooks class is defined (Hooks), which uses the interface provided
by standard modules os and os.path.  It should be used as the base
class for other hooks classes.

(2) A "module loader" class provides an interface to to search for a
module in a search path and to load it.  It defines a method which
searches for a module in a single directory; by overriding this method
one can redefine the details of the search.  If the directory is None,
built-in and frozen modules are searched instead.

Two module loader class are defined, both implementing the search
strategy used by the built-in __import__ function: ModuleLoader uses
the imp module's find_module interface, while HookableModuleLoader
uses a file system hooks class to interact with the file system.  Both
use the imp module's load_* interfaces to actually load the module.

(3) A "module importer" class provides an interface to import a
module, as well as interfaces to reload and unload a module.  It also
provides interfaces to install and uninstall itself instead of the
default __import__ and reload (and unload) functions.

One module importer class is defined (ModuleImporter), which uses a
module loader instance passed in (by default HookableModuleLoader is
instantiated).

The classes defined here should be used as base classes for extended
functionality along those lines.

If a module mporter class supports dotted names, its import_module()
must return a different value depending on whether it is called on
behalf of a "from ... import ..." statement or not.  (This is caused
by the way the __import__ hook is used by the Python interpreter.)  It
would also do wise to install a different version of reload().

XXX Should the imp.load_* functions also be called via the hooks
instance?

"""


import __builtin__
import imp
import os
import sys


from imp import C_EXTENSION, PY_SOURCE, PY_COMPILED
BUILTIN_MODULE = 32
FROZEN_MODULE = 33


class _Verbose:

    def __init__(self, verbose = 0):
        self.verbose = verbose

    def get_verbose(self):
        return self.verbose

    def set_verbose(self, verbose):
        self.verbose = verbose

    # XXX The following is an experimental interface

    def note(self, *args):
        if self.verbose:
            apply(self.message, args)

    def message(self, format, *args):
        print format%args


class BasicModuleLoader(_Verbose):

    """Basic module loader.

    This provides the same functionality as built-in import.  It
    doesn't deal with checking sys.modules -- all it provides is
    find_module() and a load_module(), as well as find_module_in_dir()
    which searches just one directory, and can be overridden by a
    derived class to change the module search algorithm when the basic
    dependency on sys.path is unchanged.

    The interface is a little more convenient than imp's:
    find_module(name, [path]) returns None or 'stuff', and
    load_module(name, stuff) loads the module.

    """

    def find_module(self, name, path = None):
        if path is None: 
            path = [None] + self.default_path()
        for dir in path:
            stuff = self.find_module_in_dir(name, dir)
            if stuff: return stuff
        return None

    def default_path(self):
        return sys.path

    def find_module_in_dir(self, name, dir):
        if dir is None:
            return self.find_builtin_module(name)
        else:
            try:
                return imp.find_module(name, [dir])
            except ImportError:
                return None

    def find_builtin_module(self, name):
        if imp.is_builtin(name):
            return None, '', ('', '', BUILTIN_MODULE)
        if imp.is_frozen(name):
            return None, '', ('', '', FROZEN_MODULE)
        return None

    def load_module(self, name, stuff):
        file, filename, (suff, mode, type) = stuff
        try:
            if type == BUILTIN_MODULE:
                return imp.init_builtin(name)
            if type == FROZEN_MODULE:
                return imp.init_frozen(name)
            if type == C_EXTENSION:
                return imp.load_dynamic(name, filename, file)
            if type == PY_SOURCE:
                return imp.load_source(name, filename, file)
            if type == PY_COMPILED:
                return imp.load_compiled(name, filename, file)
        finally:
            if file: file.close()
        raise ImportError, "Unrecognized module type (%s) for %s" % \
                           (`type`, name)


class Hooks(_Verbose):

    """Hooks into the filesystem and interpreter.

    By deriving a subclass you can redefine your filesystem interface,
    e.g. to merge it with the URL space.

    This base class behaves just like the native filesystem.

    """

    # imp interface
    def get_suffixes(self): return imp.get_suffixes()
    def new_module(self, name): return imp.new_module(name)
    def is_builtin(self, name): return imp.is_builtin(name)
    def init_builtin(self, name): return imp.init_builtin(name)
    def is_frozen(self, name): return imp.is_frozen(name)
    def init_frozen(self, name): return imp.init_frozen(name)
    def get_frozen_object(self, name): return imp.get_frozen_object(name)
    def load_source(self, name, filename, file=None):
        return imp.load_source(name, filename, file)
    def load_compiled(self, name, filename, file=None):
        return imp.load_compiled(name, filename, file)
    def load_dynamic(self, name, filename, file=None):
        return imp.load_dynamic(name, filename, file)

    def add_module(self, name):
        d = self.modules_dict()
        if d.has_key(name): return d[name]
        d[name] = m = self.new_module(name)
        return m

    # sys interface
    def modules_dict(self): return sys.modules
    def default_path(self): return sys.path

    def path_split(self, x): return os.path.split(x)
    def path_join(self, x, y): return os.path.join(x, y)
    def path_isabs(self, x): return os.path.isabs(x)
    # etc.

    def path_exists(self, x): return os.path.exists(x)
    def path_isdir(self, x): return os.path.isdir(x)
    def path_isfile(self, x): return os.path.isfile(x)
    def path_islink(self, x): return os.path.islink(x)
    # etc.

    def openfile(self, *x): return apply(open, x)
    openfile_error = IOError
    def listdir(self, x): return os.listdir(x)
    listdir_error = os.error
    # etc.


class ModuleLoader(BasicModuleLoader):

    """Default module loader; uses file system hooks.

    By defining suitable hooks, you might be able to load modules from
    other sources than the file system, e.g. from compressed or
    encrypted files, tar files or (if you're brave!) URLs.

    """

    def __init__(self, hooks = None, verbose = 0):
        BasicModuleLoader.__init__(self, verbose)
        self.hooks = hooks or Hooks(verbose)

    def default_path(self):
        return self.hooks.default_path()

    def modules_dict(self):
        return self.hooks.modules_dict()

    def get_hooks(self):
        return self.hooks

    def set_hooks(self, hooks):
        self.hooks = hooks

    def find_builtin_module(self, name):
        if self.hooks.is_builtin(name):
            return None, '', ('', '', BUILTIN_MODULE)
        if self.hooks.is_frozen(name):
            return None, '', ('', '', FROZEN_MODULE)
        return None

    def find_module_in_dir(self, name, dir):
        if dir is None:
            return self.find_builtin_module(name)
        for info in self.hooks.get_suffixes():
            suff, mode, type = info
            fullname = self.hooks.path_join(dir, name+suff)
            try:
                fp = self.hooks.openfile(fullname, mode)
                return fp, fullname, info
            except self.hooks.openfile_error:
                pass
        return None

    def load_module(self, name, stuff):
        file, filename, (suff, mode, type) = stuff
        try:
            if type == BUILTIN_MODULE:
                return self.hooks.init_builtin(name)
            if type == FROZEN_MODULE:
                return self.hooks.init_frozen(name)
            if type == C_EXTENSION:
                m = self.hooks.load_dynamic(name, filename, file)
            elif type == PY_SOURCE:
                m = self.hooks.load_source(name, filename, file)
            elif type == PY_COMPILED:
                m = self.hooks.load_compiled(name, filename, file)
            else:
                raise ImportError, "Unrecognized module type (%s) for %s" % \
                      (`type`, name)
        finally:
            if file: file.close()
        m.__file__ = filename
        return m


class FancyModuleLoader(ModuleLoader):

    """Fancy module loader -- parses and execs the code itself."""

    def load_module(self, name, stuff):
        file, filename, (suff, mode, type) = stuff
        if type == FROZEN_MODULE:
            code = self.hooks.get_frozen_object(name)
        elif type == PY_COMPILED:
            import marshal
            file.seek(8)
            code = marshal.load(file)
        elif type == PY_SOURCE:
            data = file.read()
            code = compile(data, filename, 'exec')
        else:
            return ModuleLoader.load_module(self, name, stuff)
        m = self.hooks.add_module(name)
        m.__file__ = filename
        exec code in m.__dict__
        return m


class ModuleImporter(_Verbose):

    """Default module importer; uses module loader.

    This provides the same functionality as built-in import, when
    combined with ModuleLoader.

    """

    def __init__(self, loader = None, verbose = 0):
        _Verbose.__init__(self, verbose)
        self.loader = loader or ModuleLoader(None, verbose)
        self.modules = self.loader.modules_dict()

    def get_loader(self):
        return self.loader

    def set_loader(self, loader):
        self.loader = loader

    def get_hooks(self):
        return self.loader.get_hooks()

    def set_hooks(self, hooks):
        return self.loader.set_hooks(hooks)

    def import_module(self, name, globals={}, locals={}, fromlist=[]):
        if self.modules.has_key(name):
            return self.modules[name] # Fast path
        stuff = self.loader.find_module(name)
        if not stuff:
            raise ImportError, "No module named %s" % name
        return self.loader.load_module(name, stuff)

    def reload(self, module, path = None):
        name = module.__name__
        stuff = self.loader.find_module(name, path)
        if not stuff:
            raise ImportError, "Module %s not found for reload" % name
        return self.loader.load_module(name, stuff)

    def unload(self, module):
        del self.modules[module.__name__]
        # XXX Should this try to clear the module's namespace?

    def install(self):
        self.save_import_module = __builtin__.__import__
        self.save_reload = __builtin__.reload
        if not hasattr(__builtin__, 'unload'):
            __builtin__.unload = None
        self.save_unload = __builtin__.unload
        __builtin__.__import__ = self.import_module
        __builtin__.reload = self.reload
        __builtin__.unload = self.unload

    def uninstall(self):
        __builtin__.__import__ = self.save_import_module
        __builtin__.reload = self.save_reload
        __builtin__.unload = self.save_unload
        if not __builtin__.unload:
            del __builtin__.unload


default_importer = None
current_importer = None

def install(importer = None):
    global current_importer
    current_importer = importer or default_importer or ModuleImporter()
    current_importer.install()

def uninstall():
    global current_importer
    current_importer.uninstall()
