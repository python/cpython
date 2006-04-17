"""Utilities to support packages."""

# NOTE: This module must remain compatible with Python 2.3, as it is shared
# by setuptools for distribution with Python 2.3 and up.

import os
import sys
import imp
import os.path
from types import ModuleType

__all__ = [
    'get_importer', 'iter_importers', 'get_loader', 'find_loader',
    'ImpImporter', 'ImpLoader', 'read_code', 'extend_path',
]

def read_code(stream):
    # This helper is needed in order for the PEP 302 emulation to
    # correctly handle compiled files
    import marshal

    magic = stream.read(4)
    if magic != imp.get_magic():
        return None

    stream.read(4) # Skip timestamp
    return marshal.load(stream)


class ImpImporter:
    """PEP 302 Importer that wraps Python's "classic" import algorithm

    ImpImporter(dirname) produces a PEP 302 importer that searches that
    directory.  ImpImporter(None) produces a PEP 302 importer that searches
    the current sys.path, plus any modules that are frozen or built-in.

    Note that ImpImporter does not currently support being used by placement
    on sys.meta_path.
    """

    def __init__(self, path=None):
        self.path = path

    def find_module(self, fullname, path=None):
        # Note: we ignore 'path' argument since it is only used via meta_path
        subname = fullname.split(".")[-1]
        if subname != fullname and self.path is None:
            return None
        if self.path is None:
            path = None
        else:
            path = [self.path]
        try:
            file, filename, etc = imp.find_module(subname, path)
        except ImportError:
            return None
        return ImpLoader(fullname, file, filename, etc)


class ImpLoader:
    """PEP 302 Loader that wraps Python's "classic" import algorithm
    """
    code = source = None

    def __init__(self, fullname, file, filename, etc):
        self.file = file
        self.filename = filename
        self.fullname = fullname
        self.etc = etc

    def load_module(self, fullname):
        self._reopen()
        try:
            mod = imp.load_module(fullname, self.file, self.filename, self.etc)
        finally:
            if self.file:
                self.file.close()
        # Note: we don't set __loader__ because we want the module to look
        # normal; i.e. this is just a wrapper for standard import machinery
        return mod

    def get_data(self, pathname):
        return open(pathname, "rb").read()

    def _reopen(self):
        if self.file and self.file.closed:
            if mod_type==imp.PY_SOURCE:
                self.file = open(self.filename, 'rU')
            elif mod_type in (imp.PY_COMPILED, imp.C_EXTENSION):
                self.file = open(self.filename, 'rb')

    def _fix_name(self, fullname):
        if fullname is None:
            fullname = self.fullname
        elif fullname != self.fullname:
            raise ImportError("Loader for module %s cannot handle "
                              "module %s" % (self.fullname, fullname))
        return fullname

    def is_package(self):
        return self.etc[2]==imp.PKG_DIRECTORY

    def get_code(self, fullname=None):
        fullname = self._fix_name(fullname)
        if self.code is None:
            mod_type = self.etc[2]
            if mod_type==imp.PY_SOURCE:
                source = self.get_source(fullname)
                self.code = compile(source, self.filename, 'exec')
            elif mod_type==imp.PY_COMPILED:
                self._reopen()
                try:
                    self.code = read_code(self.file)
                finally:
                    self.file.close()
            elif mod_type==imp.PKG_DIRECTORY:
                self.code = self._get_delegate().get_code()
        return self.code

    def get_source(self, fullname=None):
        fullname = self._fix_name(fullname)
        if self.source is None:
            mod_type = self.etc[2]
            if mod_type==imp.PY_SOURCE:
                self._reopen()
                try:
                    self.source = self.file.read()
                finally:
                    self.file.close()
            elif mod_type==imp.PY_COMPILED:
                if os.path.exists(self.filename[:-1]):
                    f = open(self.filename[:-1], 'rU')
                    self.source = f.read()
                    f.close()
            elif mod_type==imp.PKG_DIRECTORY:
                self.source = self._get_delegate().get_source()
        return self.source

    def _get_delegate(self):
        return ImpImporter(self.filename).find_module('__init__')

    def get_filename(self, fullname=None):
        fullname = self._fix_name(fullname)
        mod_type = self.etc[2]
        if self.etc[2]==imp.PKG_DIRECTORY:
            return self._get_delegate().get_filename()
        elif self.etc[2] in (imp.PY_SOURCE, imp.PY_COMPILED, imp.C_EXTENSION):
            return self.filename
        return None


def get_importer(path_item):
    """Retrieve a PEP 302 importer for the given path item

    The returned importer is cached in sys.path_importer_cache
    if it was newly created by a path hook.

    If there is no importer, a wrapper around the basic import
    machinery is returned. This wrapper is never inserted into
    the importer cache (None is inserted instead).

    The cache (or part of it) can be cleared manually if a
    rescan of sys.path_hooks is necessary.
    """
    try:
        importer = sys.path_importer_cache[path_item]
    except KeyError:
        for path_hook in sys.path_hooks:
            try:
                importer = path_hook(path_item)
                break
            except ImportError:
                pass
        else:
            importer = None
        sys.path_importer_cache.setdefault(path_item,importer)

    if importer is None:
        try:
            importer = ImpImporter(path_item)
        except ImportError:
            pass
    return importer


def iter_importers(fullname):
    """Yield PEP 302 importers for the given module name

    If fullname contains a '.', the importers will be for the package
    containing fullname, otherwise they will be importers for sys.meta_path,
    sys.path, and Python's "classic" import machinery, in that order.  If
    the named module is in a package, that package is imported as a side
    effect of invoking this function.

    Non PEP 302 mechanisms (e.g. the Windows registry) used by the
    standard import machinery to find files in alternative locations
    are partially supported, but are searched AFTER sys.path. Normally,
    these locations are searched BEFORE sys.path, preventing sys.path
    entries from shadowing them.

    For this to cause a visible difference in behaviour, there must
    be a module or package name that is accessible via both sys.path
    and one of the non PEP 302 file system mechanisms. In this case,
    the emulation will find the former version, while the builtin
    import mechanism will find the latter.

    Items of the following types can be affected by this discrepancy:
        imp.C_EXTENSION, imp.PY_SOURCE, imp.PY_COMPILED, imp.PKG_DIRECTORY
    """
    if fullname.startswith('.'):
        raise ImportError("Relative module names not supported")
    if '.' in fullname:
        # Get the containing package's __path__
        pkg = '.'.join(fullname.split('.')[:-1])
        if pkg not in sys.modules:
            __import__(pkg)
        path = getattr(sys.modules[pkg],'__path__',None) or []
    else:
        for importer in sys.meta_path:
            yield importer
        path = sys.path
    for item in path:
        yield get_importer(item)
    if '.' not in fullname:
        yield ImpImporter()


def get_loader(module_or_name):
    """Get a PEP 302 "loader" object for module_or_name

    If the module or package is accessible via the normal import
    mechanism, a wrapper around the relevant part of that machinery
    is returned.  Returns None if the module cannot be found or imported.
    If the named module is not already imported, its containing package
    (if any) is imported, in order to establish the package __path__.

    This function uses iter_importers(), and is thus subject to the same
    limitations regarding platform-specific special import locations such
    as the Windows registry.
    """
    if module_or_name in sys.modules:
        module_or_name = sys.modules[module_or_name]
    if isinstance(module_or_name, ModuleType):
        module = module_or_name
        loader = getattr(module,'__loader__',None)
        if loader is not None:
            return loader
        fullname = module.__name__
    else:
        fullname = module_or_name
    return find_loader(fullname)


def find_loader(fullname):
    """Find a PEP 302 "loader" object for fullname

    If fullname contains dots, path must be the containing package's __path__.
    Returns None if the module cannot be found or imported. This function uses
    iter_importers(), and is thus subject to the same limitations regarding
    platform-specific special import locations such as the Windows registry.
    """
    for importer in iter_importers(fullname):
        loader = importer.find_module(fullname)
        if loader is not None:
            return loader

    return None


def extend_path(path, name):
    """Extend a package's path.

    Intended use is to place the following code in a package's __init__.py:

        from pkgutil import extend_path
        __path__ = extend_path(__path__, __name__)

    This will add to the package's __path__ all subdirectories of
    directories on sys.path named after the package.  This is useful
    if one wants to distribute different parts of a single logical
    package as multiple directories.

    It also looks for *.pkg files beginning where * matches the name
    argument.  This feature is similar to *.pth files (see site.py),
    except that it doesn't special-case lines starting with 'import'.
    A *.pkg file is trusted at face value: apart from checking for
    duplicates, all entries found in a *.pkg file are added to the
    path, regardless of whether they are exist the filesystem.  (This
    is a feature.)

    If the input path is not a list (as is the case for frozen
    packages) it is returned unchanged.  The input path is not
    modified; an extended copy is returned.  Items are only appended
    to the copy at the end.

    It is assumed that sys.path is a sequence.  Items of sys.path that
    are not (unicode or 8-bit) strings referring to existing
    directories are ignored.  Unicode items of sys.path that cause
    errors when used as filenames may cause this function to raise an
    exception (in line with os.path.isdir() behavior).
    """

    if not isinstance(path, list):
        # This could happen e.g. when this is called from inside a
        # frozen package.  Return the path unchanged in that case.
        return path

    pname = os.path.join(*name.split('.')) # Reconstitute as relative path
    # Just in case os.extsep != '.'
    sname = os.extsep.join(name.split('.'))
    sname_pkg = sname + os.extsep + "pkg"
    init_py = "__init__" + os.extsep + "py"

    path = path[:] # Start with a copy of the existing path

    for dir in sys.path:
        if not isinstance(dir, basestring) or not os.path.isdir(dir):
            continue
        subdir = os.path.join(dir, pname)
        # XXX This may still add duplicate entries to path on
        # case-insensitive filesystems
        initfile = os.path.join(subdir, init_py)
        if subdir not in path and os.path.isfile(initfile):
            path.append(subdir)
        # XXX Is this the right thing for subpackages like zope.app?
        # It looks for a file named "zope.app.pkg"
        pkgfile = os.path.join(dir, sname_pkg)
        if os.path.isfile(pkgfile):
            try:
                f = open(pkgfile)
            except IOError, msg:
                sys.stderr.write("Can't open %s: %s\n" %
                                 (pkgfile, msg))
            else:
                for line in f:
                    line = line.rstrip('\n')
                    if not line or line.startswith('#'):
                        continue
                    path.append(line) # Don't check for existence!
                f.close()

    return path
