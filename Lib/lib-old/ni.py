"""New import scheme with package support.

Quick Reference
---------------

- To enable package support, execute "import ni" before importing any
  packages.  Importing this module automatically installs the relevant
  import hooks.

- To create a package named spam containing sub-modules ham, bacon and
  eggs, create a directory spam somewhere on Python's module search
  path (i.e. spam's parent directory must be one of the directories in
  sys.path or $PYTHONPATH); then create files ham.py, bacon.py and
  eggs.py inside spam.

- To import module ham from package spam and use function hamneggs()
  from that module, you can either do

    import spam.ham             # *not* "import spam" !!!
    spam.ham.hamneggs()

  or

    from spam import ham
    ham.hamneggs()

  or

    from spam.ham import hamneggs
    hamneggs()

- Importing just "spam" does not do what you expect: it creates an
  empty package named spam if one does not already exist, but it does
  not import spam's submodules.  The only submodule that is guaranteed
  to be imported is spam.__init__, if it exists.  Note that
  spam.__init__ is a submodule of package spam.  It can reference to
  spam's namespace via the '__.' prefix, for instance

    __.spam_inited = 1          # Set a package-level variable



Theory of Operation
-------------------

A Package is a module that can contain other modules.  Packages can be
nested.  Package introduce dotted names for modules, like P.Q.M, which
could correspond to a file P/Q/M.py found somewhere on sys.path.  It
is possible to import a package itself, though this makes little sense
unless the package contains a module called __init__.

A package has two variables that control the namespace used for
packages and modules, both initialized to sensible defaults the first
time the package is referenced.

(1) A package's *module search path*, contained in the per-package
variable __path__, defines a list of *directories* where submodules or
subpackages of the package are searched.  It is initialized to the
directory containing the package.  Setting this variable to None makes
the module search path default to sys.path (this is not quite the same
as setting it to sys.path, since the latter won't track later
assignments to sys.path).

(2) A package's *import domain*, contained in the per-package variable
__domain__, defines a list of *packages* that are searched (using
their respective module search paths) to satisfy imports.  It is
initialized to the list consisting of the package itself, its parent
package, its parent's parent, and so on, ending with the root package
(the nameless package containing all top-level packages and modules,
whose module search path is None, implying sys.path).

The default domain implements a search algorithm called "expanding
search".  An alternative search algorithm called "explicit search"
fixes the import search path to contain only the root package,
requiring the modules in the package to name all imported modules by
their full name.  The convention of using '__' to refer to the current
package (both as a per-module variable and in module names) can be
used by packages using explicit search to refer to modules in the same
package; this combination is known as "explicit-relative search".

The PackageImporter and PackageLoader classes together implement the
following policies:

- There is a root package, whose name is ''.  It cannot be imported
  directly but may be referenced, e.g. by using '__' from a top-level
  module.

- In each module or package, the variable '__' contains a reference to
  the parent package; in the root package, '__' points to itself.

- In the name for imported modules (e.g. M in "import M" or "from M
  import ..."), a leading '__' refers to the current package (i.e.
  the package containing the current module); leading '__.__' and so
  on refer to the current package's parent, and so on.  The use of
  '__' elsewhere in the module name is not supported.

- Modules are searched using the "expanding search" algorithm by
  virtue of the default value for __domain__.

- If A.B.C is imported, A is searched using __domain__; then
  subpackage B is searched in A using its __path__, and so on.

- Built-in modules have priority: even if a file sys.py exists in a
  package, "import sys" imports the built-in sys module.

- The same holds for frozen modules, for better or for worse.

- Submodules and subpackages are not automatically loaded when their
  parent packages is loaded.

- The construct "from package import *" is illegal.  (It can still be
  used to import names from a module.)

- When "from package import module1, module2, ..." is used, those
    modules are explicitly loaded.

- When a package is loaded, if it has a submodule __init__, that
  module is loaded.  This is the place where required submodules can
  be loaded, the __path__ variable extended, etc.  The __init__ module
  is loaded even if the package was loaded only in order to create a
  stub for a sub-package: if "import P.Q.R" is the first reference to
  P, and P has a submodule __init__, P.__init__ is loaded before P.Q
  is even searched.

Caveats:

- It is possible to import a package that has no __init__ submodule;
  this is not particularly useful but there may be useful applications
  for it (e.g. to manipulate its search paths from the outside!).

- There are no special provisions for os.chdir().  If you plan to use
  os.chdir() before you have imported all your modules, it is better
  not to have relative pathnames in sys.path.  (This could actually be
  fixed by changing the implementation of path_join() in the hook to
  absolutize paths.)

- Packages and modules are introduced in sys.modules as soon as their
  loading is started.  When the loading is terminated by an exception,
  the sys.modules entries remain around.

- There are no special measures to support mutually recursive modules,
  but it will work under the same conditions where it works in the
  flat module space system.

- Sometimes dummy entries (whose value is None) are entered in
  sys.modules, to indicate that a particular module does not exist --
  this is done to speed up the expanding search algorithm when a
  module residing at a higher level is repeatedly imported (Python
  promises that importing a previously imported module is cheap!)

- Although dynamically loaded extensions are allowed inside packages,
  the current implementation (hardcoded in the interpreter) of their
  initialization may cause problems if an extension invokes the
  interpreter during its initialization.

- reload() may find another version of the module only if it occurs on
  the package search path.  Thus, it keeps the connection to the
  package to which the module belongs, but may find a different file.

XXX Need to have an explicit name for '', e.g. '__root__'.

"""


import imp
import string
import sys
import __builtin__

import ihooks
from ihooks import ModuleLoader, ModuleImporter


class PackageLoader(ModuleLoader):

    """A subclass of ModuleLoader with package support.

    find_module_in_dir() will succeed if there's a subdirectory with
    the given name; load_module() will create a stub for a package and
    load its __init__ module if it exists.

    """

    def find_module_in_dir(self, name, dir):
        if dir is not None:
            dirname = self.hooks.path_join(dir, name)
            if self.hooks.path_isdir(dirname):
                return None, dirname, ('', '', 'PACKAGE')
        return ModuleLoader.find_module_in_dir(self, name, dir)

    def load_module(self, name, stuff):
        file, filename, info = stuff
        suff, mode, type = info
        if type == 'PACKAGE':
            return self.load_package(name, stuff)
        if sys.modules.has_key(name):
            m = sys.modules[name]
        else:
            sys.modules[name] = m = imp.new_module(name)
        self.set_parent(m)
        if type == imp.C_EXTENSION and '.' in name:
            return self.load_dynamic(name, stuff)
        else:
            return ModuleLoader.load_module(self, name, stuff)

    def load_dynamic(self, name, stuff):
        file, filename, (suff, mode, type) = stuff
        # Hack around restriction in imp.load_dynamic()
        i = string.rfind(name, '.')
        tail = name[i+1:]
        if sys.modules.has_key(tail):
            save = sys.modules[tail]
        else:
            save = None
        sys.modules[tail] = imp.new_module(name)
        try:
            m = imp.load_dynamic(tail, filename, file)
        finally:
            if save:
                sys.modules[tail] = save
            else:
                del sys.modules[tail]
        sys.modules[name] = m
        return m

    def load_package(self, name, stuff):
        file, filename, info = stuff
        if sys.modules.has_key(name):
            package = sys.modules[name]
        else:
            sys.modules[name] = package = imp.new_module(name)
        package.__path__ = [filename]
        self.init_package(package)
        return package

    def init_package(self, package):
        self.set_parent(package)
        self.set_domain(package)
        self.call_init_module(package)

    def set_parent(self, m):
        name = m.__name__
        if '.' in name:
            name = name[:string.rfind(name, '.')]
        else:
            name = ''
        m.__ = sys.modules[name]

    def set_domain(self, package):
        name = package.__name__
        package.__domain__ = domain = [name]
        while '.' in name:
            name = name[:string.rfind(name, '.')]
            domain.append(name)
        if name:
            domain.append('')

    def call_init_module(self, package):
        stuff = self.find_module('__init__', package.__path__)
        if stuff:
            m = self.load_module(package.__name__ + '.__init__', stuff)
            package.__init__ = m


class PackageImporter(ModuleImporter):

    """Importer that understands packages and '__'."""

    def __init__(self, loader = None, verbose = 0):
        ModuleImporter.__init__(self,
        loader or PackageLoader(None, verbose), verbose)

    def import_module(self, name, globals={}, locals={}, fromlist=[]):
        if globals.has_key('__'):
            package = globals['__']
        else:
            # No calling context, assume in root package
            package = sys.modules['']
        if name[:3] in ('__.', '__'):
            p = package
            name = name[3:]
            while name[:3] in ('__.', '__'):
                p = p.__
                name = name[3:]
            if not name:
                return self.finish(package, p, '', fromlist)
            if '.' in name:
                i = string.find(name, '.')
                name, tail = name[:i], name[i:]
            else:
                tail = ''
            mname = p.__name__ and p.__name__+'.'+name or name
            m = self.get1(mname)
            return self.finish(package, m, tail, fromlist)
        if '.' in name:
            i = string.find(name, '.')
            name, tail = name[:i], name[i:]
        else:
            tail = ''
        for pname in package.__domain__:
            mname = pname and pname+'.'+name or name
            m = self.get0(mname)
            if m: break
        else:
            raise ImportError, "No such module %s" % name
        return self.finish(m, m, tail, fromlist)

    def finish(self, module, m, tail, fromlist):
        # Got ....A; now get ....A.B.C.D
        yname = m.__name__
        if tail and sys.modules.has_key(yname + tail): # Fast path
            yname, tail = yname + tail, ''
            m = self.get1(yname)
        while tail:
            i = string.find(tail, '.', 1)
            if i > 0:
                head, tail = tail[:i], tail[i:]
            else:
                head, tail = tail, ''
            yname = yname + head
            m = self.get1(yname)

        # Got ....A.B.C.D; now finalize things depending on fromlist
        if not fromlist:
            return module
        if '__' in fromlist:
            raise ImportError, "Can't import __ from anywhere"
        if not hasattr(m, '__path__'): return m
        if '*' in fromlist:
            raise ImportError, "Can't import * from a package"
        for f in fromlist:
            if hasattr(m, f): continue
            fname = yname + '.' + f
            self.get1(fname)
        return m

    def get1(self, name):
        m = self.get(name)
        if not m:
            raise ImportError, "No module named %s" % name
        return m

    def get0(self, name):
        m = self.get(name)
        if not m:
            sys.modules[name] = None
        return m

    def get(self, name):
        # Internal routine to get or load a module when its parent exists
        if sys.modules.has_key(name):
            return sys.modules[name]
        if '.' in name:
            i = string.rfind(name, '.')
            head, tail = name[:i], name[i+1:]
        else:
            head, tail = '', name
        path = sys.modules[head].__path__
        stuff = self.loader.find_module(tail, path)
        if not stuff:
            return None
        sys.modules[name] = m = self.loader.load_module(name, stuff)
        if head:
            setattr(sys.modules[head], tail, m)
        return m

    def reload(self, module):
        name = module.__name__
        if '.' in name:
            i = string.rfind(name, '.')
            head, tail = name[:i], name[i+1:]
            path = sys.modules[head].__path__
        else:
            tail = name
            path = sys.modules[''].__path__
        stuff = self.loader.find_module(tail, path)
        if not stuff:
            raise ImportError, "No module named %s" % name
        return self.loader.load_module(name, stuff)

    def unload(self, module):
        if hasattr(module, '__path__'):
            raise ImportError, "don't know how to unload packages yet"
        PackageImporter.unload(self, module)

    def install(self):
        if not sys.modules.has_key(''):
            sys.modules[''] = package = imp.new_module('')
            package.__path__ = None
            self.loader.init_package(package)
            for m in sys.modules.values():
                if not m: continue
                if not hasattr(m, '__'):
                    self.loader.set_parent(m)
        ModuleImporter.install(self)


def install(v = 0):
    ihooks.install(PackageImporter(None, v))

def uninstall():
    ihooks.uninstall()

def ni(v = 0):
    install(v)

def no():
    uninstall()

def test():
    import pdb
    try:
        testproper()
    except:
        sys.last_type, sys.last_value, sys.last_traceback = sys.exc_info()
        print
        print sys.last_type, ':', sys.last_value
        print
        pdb.pm()

def testproper():
    install(1)
    try:
        import mactest
        print dir(mactest)
        raw_input('OK?')
    finally:
        uninstall()


if __name__ == '__main__':
    test()
else:
    install()
