"""Find modules used by a script, using introspection."""

import dis
import imp
import marshal
import os
import re
import string
import sys

if sys.platform=="win32":
    # On Windows, we can locate modules in the registry with
    # the help of the win32api package.
    try:
        import win32api
    except ImportError:
        print "The win32api module is not available - modules listed"
        print "in the registry will not be found."
        win32api = None


IMPORT_NAME = dis.opname.index('IMPORT_NAME')
IMPORT_FROM = dis.opname.index('IMPORT_FROM')

# Modulefinder does a good job at simulating Python's, but it can not
# handle __path__ modifications packages make at runtime.  Therefore there
# is a mechanism whereby you can register extra paths in this map for a
# package, and it will be honoured.

# Note this is a mapping is lists of paths.
packagePathMap = {}

# A Public interface
def AddPackagePath(packagename, path):
    paths = packagePathMap.get(packagename, [])
    paths.append(path)
    packagePathMap[packagename] = paths

class Module:

    def __init__(self, name, file=None, path=None):
        self.__name__ = name
        self.__file__ = file
        self.__path__ = path
        self.__code__ = None

    def __repr__(self):
        s = "Module(%s" % `self.__name__`
        if self.__file__ is not None:
            s = s + ", %s" % `self.__file__`
        if self.__path__ is not None:
            s = s + ", %s" % `self.__path__`
        s = s + ")"
        return s


class ModuleFinder:

    def __init__(self, path=None, debug=0, excludes = []):
        if path is None:
            path = sys.path
        self.path = path
        self.modules = {}
        self.badmodules = {}
        self.debug = debug
        self.indent = 0
        self.excludes = excludes

    def msg(self, level, str, *args):
        if level <= self.debug:
            for i in range(self.indent):
                print "   ",
            print str,
            for arg in args:
                print repr(arg),
            print

    def msgin(self, *args):
        level = args[0]
        if level <= self.debug:
            self.indent = self.indent + 1
            apply(self.msg, args)

    def msgout(self, *args):
        level = args[0]
        if level <= self.debug:
            self.indent = self.indent - 1
            apply(self.msg, args)

    def run_script(self, pathname):
        self.msg(2, "run_script", pathname)
        fp = open(pathname)
        stuff = ("", "r", imp.PY_SOURCE)
        self.load_module('__main__', fp, pathname, stuff)

    def load_file(self, pathname):
        dir, name = os.path.split(pathname)
        name, ext = os.path.splitext(name)
        fp = open(pathname)
        stuff = (ext, "r", imp.PY_SOURCE)
        self.load_module(name, fp, pathname, stuff)

    def import_hook(self, name, caller=None, fromlist=None):
        self.msg(3, "import_hook", name, caller, fromlist)
        parent = self.determine_parent(caller)
        q, tail = self.find_head_package(parent, name)
        m = self.load_tail(q, tail)
        if not fromlist:
            return q
        if m.__path__:
            self.ensure_fromlist(m, fromlist)

    def determine_parent(self, caller):
        self.msgin(4, "determine_parent", caller)
        if not caller:
            self.msgout(4, "determine_parent -> None")
            return None
        pname = caller.__name__
        if caller.__path__:
            parent = self.modules[pname]
            assert caller is parent
            self.msgout(4, "determine_parent ->", parent)
            return parent
        if '.' in pname:
            i = string.rfind(pname, '.')
            pname = pname[:i]
            parent = self.modules[pname]
            assert parent.__name__ == pname
            self.msgout(4, "determine_parent ->", parent)
            return parent
        self.msgout(4, "determine_parent -> None")
        return None

    def find_head_package(self, parent, name):
        self.msgin(4, "find_head_package", parent, name)
        if '.' in name:
            i = string.find(name, '.')
            head = name[:i]
            tail = name[i+1:]
        else:
            head = name
            tail = ""
        if parent:
            qname = "%s.%s" % (parent.__name__, head)
        else:
            qname = head
        q = self.import_module(head, qname, parent)
        if q:
            self.msgout(4, "find_head_package ->", (q, tail))
            return q, tail
        if parent:
            qname = head
            parent = None
            q = self.import_module(head, qname, parent)
            if q:
                self.msgout(4, "find_head_package ->", (q, tail))
                return q, tail
        self.msgout(4, "raise ImportError: No module named", qname)
        raise ImportError, "No module named " + qname

    def load_tail(self, q, tail):
        self.msgin(4, "load_tail", q, tail)
        m = q
        while tail:
            i = string.find(tail, '.')
            if i < 0: i = len(tail)
            head, tail = tail[:i], tail[i+1:]
            mname = "%s.%s" % (m.__name__, head)
            m = self.import_module(head, mname, m)
            if not m:
                self.msgout(4, "raise ImportError: No module named", mname)
                raise ImportError, "No module named " + mname
        self.msgout(4, "load_tail ->", m)
        return m

    def ensure_fromlist(self, m, fromlist, recursive=0):
        self.msg(4, "ensure_fromlist", m, fromlist, recursive)
        for sub in fromlist:
            if sub == "*":
                if not recursive:
                    all = self.find_all_submodules(m)
                    if all:
                        self.ensure_fromlist(m, all, 1)
            elif not hasattr(m, sub):
                subname = "%s.%s" % (m.__name__, sub)
                submod = self.import_module(sub, subname, m)
                if not submod:
                    raise ImportError, "No module named " + subname

    def find_all_submodules(self, m):
        if not m.__path__:
            return
        modules = {}
        suffixes = [".py", ".pyc", ".pyo"]
        for dir in m.__path__:
            try:
                names = os.listdir(dir)
            except os.error:
                self.msg(2, "can't list directory", dir)
                continue
            for name in names:
                mod = None
                for suff in suffixes:
                    n = len(suff)
                    if name[-n:] == suff:
                        mod = name[:-n]
                        break
                if mod and mod != "__init__":
                    modules[mod] = mod
        return modules.keys()

    def import_module(self, partname, fqname, parent):
        self.msgin(3, "import_module", partname, fqname, parent)
        try:
            m = self.modules[fqname]
        except KeyError:
            pass
        else:
            self.msgout(3, "import_module ->", m)
            return m
        if self.badmodules.has_key(fqname):
            self.msgout(3, "import_module -> None")
            self.badmodules[fqname][parent.__name__] = None
            return None
        try:
            fp, pathname, stuff = self.find_module(partname,
                                                   parent and parent.__path__)
        except ImportError:
            self.msgout(3, "import_module ->", None)
            return None
        try:
            m = self.load_module(fqname, fp, pathname, stuff)
        finally:
            if fp: fp.close()
        if parent:
            setattr(parent, partname, m)
        self.msgout(3, "import_module ->", m)
        return m

    def load_module(self, fqname, fp, pathname, (suffix, mode, type)):
        self.msgin(2, "load_module", fqname, fp and "fp", pathname)
        if type == imp.PKG_DIRECTORY:
            m = self.load_package(fqname, pathname)
            self.msgout(2, "load_module ->", m)
            return m
        if type == imp.PY_SOURCE:
            co = compile(fp.read()+'\n', pathname, 'exec')
        elif type == imp.PY_COMPILED:
            if fp.read(4) != imp.get_magic():
                self.msgout(2, "raise ImportError: Bad magic number", pathname)
                raise ImportError, "Bad magic number in %s", pathname
            fp.read(4)
            co = marshal.load(fp)
        else:
            co = None
        m = self.add_module(fqname)
        m.__file__ = pathname
        if co:
            m.__code__ = co
            self.scan_code(co, m)
        self.msgout(2, "load_module ->", m)
        return m

    def scan_code(self, co, m):
        code = co.co_code
        n = len(code)
        i = 0
        lastname = None
        while i < n:
            c = code[i]
            i = i+1
            op = ord(c)
            if op >= dis.HAVE_ARGUMENT:
                oparg = ord(code[i]) + ord(code[i+1])*256
                i = i+2
            if op == IMPORT_NAME:
                name = lastname = co.co_names[oparg]
                if not self.badmodules.has_key(lastname):
                    try:
                        self.import_hook(name, m)
                    except ImportError, msg:
                        self.msg(2, "ImportError:", str(msg))
                        if not self.badmodules.has_key(name):
                            self.badmodules[name] = {}
                        self.badmodules[name][m.__name__] = None
            elif op == IMPORT_FROM:
                name = co.co_names[oparg]
                assert lastname is not None
                if not self.badmodules.has_key(lastname):
                    try:
                        self.import_hook(lastname, m, [name])
                    except ImportError, msg:
                        self.msg(2, "ImportError:", str(msg))
                        fullname = lastname + "." + name
                        if not self.badmodules.has_key(fullname):
                            self.badmodules[fullname] = {}
                        self.badmodules[fullname][m.__name__] = None
            else:
                lastname = None
        for c in co.co_consts:
            if isinstance(c, type(co)):
                self.scan_code(c, m)

    def load_package(self, fqname, pathname):
        self.msgin(2, "load_package", fqname, pathname)
        m = self.add_module(fqname)
        m.__file__ = pathname
        m.__path__ = [pathname]

        # As per comment at top of file, simulate runtime __path__ additions.
        m.__path__ = m.__path__ + packagePathMap.get(fqname, [])

        fp, buf, stuff = self.find_module("__init__", m.__path__)
        self.load_module(fqname, fp, buf, stuff)
        self.msgout(2, "load_package ->", m)
        return m

    def add_module(self, fqname):
        if self.modules.has_key(fqname):
            return self.modules[fqname]
        self.modules[fqname] = m = Module(fqname)
        return m

    def find_module(self, name, path):
        if name in self.excludes:
            self.msgout(3, "find_module -> Excluded")
            raise ImportError, name

        if path is None:
            if name in sys.builtin_module_names:
                return (None, None, ("", "", imp.C_BUILTIN))

            # Emulate the Registered Module support on Windows.
            if sys.platform=="win32" and win32api is not None:
                HKEY_LOCAL_MACHINE = 0x80000002
                try:
                    pathname = win32api.RegQueryValue(HKEY_LOCAL_MACHINE, "Software\\Python\\PythonCore\\%s\\Modules\\%s" % (sys.winver, name))
                    fp = open(pathname, "rb")
                    # XXX - To do - remove the hard code of C_EXTENSION.
                    stuff = "", "rb", imp.C_EXTENSION
                    return fp, pathname, stuff
                except win32api.error:
                    pass

            path = self.path
        return imp.find_module(name, path)

    def report(self):
        print
        print "  %-25s %s" % ("Name", "File")
        print "  %-25s %s" % ("----", "----")
        # Print modules found
        keys = self.modules.keys()
        keys.sort()
        for key in keys:
            m = self.modules[key]
            if m.__path__:
                print "P",
            else:
                print "m",
            print "%-25s" % key, m.__file__ or ""

        # Print missing modules
        keys = self.badmodules.keys()
        keys.sort()
        for key in keys:
            # ... but not if they were explicitely excluded.
            if key not in self.excludes:
                mods = self.badmodules[key].keys()
                mods.sort()
                print "?", key, "from", string.join(mods, ', ')


def test():
    # Parse command line
    import getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], "dmp:qx:")
    except getopt.error, msg:
        print msg
        return

    # Process options
    debug = 1
    domods = 0
    addpath = []
    exclude = []
    for o, a in opts:
        if o == '-d':
            debug = debug + 1
        if o == '-m':
            domods = 1
        if o == '-p':
            addpath = addpath + string.split(a, os.pathsep)
        if o == '-q':
            debug = 0
        if o == '-x':
            exclude.append(a)

    # Provide default arguments
    if not args:
        script = "hello.py"
    else:
        script = args[0]

    # Set the path based on sys.path and the script directory
    path = sys.path[:]
    path[0] = os.path.dirname(script)
    path = addpath + path
    if debug > 1:
        print "path:"
        for item in path:
            print "   ", `item`

    # Create the module finder and turn its crank
    mf = ModuleFinder(path, debug, exclude)
    for arg in args[1:]:
        if arg == '-m':
            domods = 1
            continue
        if domods:
            if arg[-2:] == '.*':
                mf.import_hook(arg[:-2], None, ["*"])
            else:
                mf.import_hook(arg)
        else:
            mf.load_file(arg)
    mf.run_script(script)
    mf.report()


if __name__ == '__main__':
    try:
        test()
    except KeyboardInterrupt:
        print "\n[interrupt]"
