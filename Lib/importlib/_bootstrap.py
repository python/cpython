"""Core implementation of import.

This module is NOT meant to be directly imported! It has been designed such
that it can be bootstrapped into Python as the implementation of import. As
such it requires the injection of specific modules and attributes in order to
work. One should use importlib as the public-facing version of this module.

"""

# Injected modules are '_warnings', 'imp', 'sys', 'marshal', 'errno', and '_os'
# (a.k.a. 'posix', 'nt' or 'os2').
# Injected attribute is path_sep.
#
# When editing this code be aware that code executed at import time CANNOT
# reference any injected objects! This includes not only global code but also
# anything specified at the class level.


# XXX Could also expose Modules/getpath.c:joinpath()
def _path_join(*args):
    """Replacement for os.path.join."""
    return path_sep.join(x[:-len(path_sep)] if x.endswith(path_sep) else x
                            for x in args)


def _path_exists(path):
    """Replacement for os.path.exists."""
    try:
        _os.stat(path)
    except OSError:
        return False
    else:
        return True


def _path_is_mode_type(path, mode):
    """Test whether the path is the specified mode type."""
    try:
        stat_info = _os.stat(path)
    except OSError:
        return False
    return (stat_info.st_mode & 0o170000) == mode


# XXX Could also expose Modules/getpath.c:isfile()
def _path_isfile(path):
    """Replacement for os.path.isfile."""
    return _path_is_mode_type(path, 0o100000)


# XXX Could also expose Modules/getpath.c:isdir()
def _path_isdir(path):
    """Replacement for os.path.isdir."""
    return _path_is_mode_type(path, 0o040000)


def _path_without_ext(path, ext_type):
    """Replacement for os.path.splitext()[0]."""
    for suffix in suffix_list(ext_type):
        if path.endswith(suffix):
            return path[:-len(suffix)]
    else:
        raise ValueError("path is not of the specified type")


def _path_absolute(path):
    """Replacement for os.path.abspath."""
    if not path:
        path = _os.getcwd()
    try:
        return _os._getfullpathname(path)
    except AttributeError:
        if path.startswith('/'):
            return path
        else:
            return _path_join(_os.getcwd(), path)


class closing:

    """Simple replacement for contextlib.closing."""

    def __init__(self, obj):
        self.obj = obj

    def __enter__(self):
        return self.obj

    def __exit__(self, *args):
        self.obj.close()


def set___package__(fxn):
    """Set __package__ on the returned module."""
    def wrapper(*args, **kwargs):
        module = fxn(*args, **kwargs)
        if not hasattr(module, '__package__') or module.__package__ is None:
            module.__package__ = module.__name__
            if not hasattr(module, '__path__'):
                module.__package__ = module.__package__.rpartition('.')[0]
        return module
    return wrapper


class BuiltinImporter:

    """Meta path loader for built-in modules.

    All methods are either class or static methods, allowing direct use of the
    class.

    """

    @classmethod
    def find_module(cls, fullname, path=None):
        """Try to find the built-in module.

        If 'path' is ever specified then the search is considered a failure.

        """
        if path is not None:
            return None
        return cls if imp.is_builtin(fullname) else None

    @classmethod
    @set___package__
    def load_module(cls, fullname):
        """Load a built-in module."""
        if fullname not in sys.builtin_module_names:
            raise ImportError("{0} is not a built-in module".format(fullname))
        module = imp.init_builtin(fullname)
        return module


class FrozenImporter:

    """Meta path class for importing frozen modules.

    All methods are either class or static method to allow direct use of the
    class.

    """

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find a frozen module."""
        return cls if imp.is_frozen(fullname) else None

    @classmethod
    @set___package__
    def load_module(cls, fullname):
        """Load a frozen module."""
        if cls.find_module(fullname) is None:
            raise ImportError("{0} is not a frozen module".format(fullname))
        module = imp.init_frozen(fullname)
        return module


class ChainedImporter(object):

    """Finder that sequentially calls other finders."""

    def __init__(self, *importers):
        self._importers = importers

    def find_module(self, fullname, path=None):
        for importer in self._importers:
            result = importer.find_module(fullname, path)
            if result:
                return result
        else:
            return None


# XXX Don't make filesystem-specific and instead make generic for any path
#     hooks.
def chaining_fs_path_hook(*path_hooks):
    """Create a closure which calls the path hooks sequentially looking for
    which path hooks can handle a path entry.


    Passed-in path hooks work as any other path hooks, raising ImportError if
    they cannot handle the path, otherwise returning a finder.

    """
    def chained_fs_path_hook(path_entry):
        """Closure which sees which of the captured path hooks can handle the
        path entry."""
        absolute_path = _path_absolute(path_entry)
        if not _path_isdir(absolute_path):
            raise ImportError("only directories are supported")
        accepted = []
        for path_hook in path_hooks:
            try:
                accepted.append(path_hook(absolute_path))
            except ImportError:
                continue
        if not accepted:
            raise ImportError("no path hooks could handle %s" % path_entry)
        return ChainedImporter(*accepted)
    return chained_fs_path_hook


def check_name(method):
    """Decorator to verify that the module being requested matches the one the
    loader can handle.

    The first argument (self) must define _name which the second argument is
    comapred against. If the comparison fails then ImportError is raised.

    """
    def inner(self, name, *args, **kwargs):
        if self._name != name:
            raise ImportError("loader cannot handle %s" % name)
        return method(self, name, *args, **kwargs)
    inner.__name__ = method.__name__
    inner.__doc__ = method.__doc__
    inner.__dict__.update(method.__dict__)
    return inner


class _ExtensionFileLoader(object):

    """Loader for extension modules.

    The constructor is designed to work with FileImporter.

    """

    def __init__(self, name, path, is_pkg):
        """Initialize the loader.

        If is_pkg is True then an exception is raised as extension modules
        cannot be the __init__ module for an extension module.

        """
        self._name = name
        self._path = path
        if is_pkg:
            raise ValueError("extension modules cannot be packages")

    @check_name
    @set___package__
    def load_module(self, fullname):
        """Load an extension module."""
        assert self._name == fullname
        try:
            module = imp.load_dynamic(fullname, self._path)
            module.__loader__ = self
            return module
        except:
            # If an error occurred, don't leave a partially initialized module.
            if fullname in sys.modules:
                del sys.modules[fullname]
            raise

    @check_name
    def is_package(self, fullname):
        """Return False as an extension module can never be a package."""
        return False

    @check_name
    def get_code(self, fullname):
        """Return None as an extension module cannot create a code object."""
        return None

    @check_name
    def get_source(self, fullname):
        """Return None as extension modules have no source code."""
        return None


def suffix_list(suffix_type):
    """Return a list of file suffixes based on the imp file type."""
    return [suffix[0] for suffix in imp.get_suffixes()
            if suffix[2] == suffix_type]


# XXX Need a better name.
def get_module(fxn):
    """Decorator to handle selecting the proper module for load_module
    implementations.

    Decorated modules are passed the module to use instead of the module name.
    The module is either from sys.modules if it already exists (for reloading)
    or is a new module which has __name__ set. If any exception is raised by
    the decorated method then __loader__, __name__, __file__, and __path__ are
    all restored on the module to their original values.

    """
    def decorated(self, fullname):
        module = sys.modules.get(fullname)
        is_reload = bool(module)
        if not is_reload:
            # This must be done before open() is called as the 'io' module
            # implicitly imports 'locale' and would otherwise trigger an
            # infinite loop.
            module = imp.new_module(fullname)
            module.__name__ = fullname
            sys.modules[fullname] = module
        else:
            original_values = {}
            modified_attrs = ['__loader__', '__name__', '__file__', '__path__']
            for attr in modified_attrs:
                try:
                    original_values[attr] = getattr(module, attr)
                except AttributeError:
                    pass
        try:
            return fxn(self, module)
        except:
            if not is_reload:
                del sys.modules[fullname]
            else:
                for attr in modified_attrs:
                    if attr in original_values:
                        setattr(module, attr, original_values[attr])
                    elif hasattr(module, attr):
                        delattr(module, attr)
            raise
    return decorated


class _PyFileLoader(object):
    # XXX Still smart to have this as a separate class?  Or would it work
    # better to integrate with PyFileImporter?  Could cache _is_pkg info.
    # FileImporter can be changed to return self instead of a specific loader
    # call.  Otherwise _base_path can be calculated on the fly without issue if
    # it is known whether a module should be treated as a path or package to
    # minimize stat calls.  Could even go as far as to stat the directory the
    # importer is in to detect changes and then cache all the info about what
    # files were found (if stating directories is platform-dependent).

    """Load a Python source or bytecode file."""

    def __init__(self, name, path, is_pkg):
        self._name = name
        self._is_pkg = is_pkg
        # Figure out the base path based on whether it was source or bytecode
        # that was found.
        try:
            self._base_path = _path_without_ext(path, imp.PY_SOURCE)
        except ValueError:
            self._base_path = _path_without_ext(path, imp.PY_COMPILED)

    def _find_path(self, ext_type):
        """Find a path from the base path and the specified extension type that
        exists, returning None if one is not found."""
        for suffix in suffix_list(ext_type):
            path = self._base_path + suffix
            if _path_exists(path):
                return path
        else:
            return None

    @check_name
    def source_path(self, fullname):
        """Return the path to an existing source file for the module, or None
        if one cannot be found."""
        # Not a property so that it is easy to override.
        return self._find_path(imp.PY_SOURCE)

    @check_name
    def bytecode_path(self, fullname):
        """Return the path to a bytecode file, or None if one does not
        exist."""
        # Not a property for easy overriding.
        return self._find_path(imp.PY_COMPILED)

    @check_name
    @get_module
    def load_module(self, module):
        """Load a Python source or bytecode module."""
        name = module.__name__
        source_path = self.source_path(name)
        bytecode_path = self.bytecode_path(name)
        code_object = self.get_code(module.__name__)
        module.__file__ = source_path if source_path else bytecode_path
        module.__loader__ = self
        if self._is_pkg:
            module.__path__  = [module.__file__.rsplit(path_sep, 1)[0]]
        module.__package__ = module.__name__
        if not hasattr(module, '__path__'):
            module.__package__ = module.__package__.rpartition('.')[0]
        exec(code_object, module.__dict__)
        return module

    @check_name
    def source_mtime(self, name):
        """Return the modification time of the source for the specified
        module."""
        source_path = self.source_path(name)
        if not source_path:
            return None
        return int(_os.stat(source_path).st_mtime)

    @check_name
    def get_source(self, fullname):
        """Return the source for the module as a string.

        Return None if the source is not available. Raise ImportError if the
        laoder cannot handle the specified module.

        """
        source_path = self._source_path(name)
        if source_path is None:
            return None
        import tokenize
        with closing(_fileio._FileIO(source_path, 'r')) as file:
            encoding, lines = tokenize.detect_encoding(file.readline)
        # XXX Will fail when passed to compile() if the encoding is
        # anything other than UTF-8.
        return open(source_path, encoding=encoding).read()

    @check_name
    def write_bytecode(self, name, data):
        """Write out 'data' for the specified module, returning a boolean
        signifying if the write-out actually occurred.

        Raises ImportError (just like get_source) if the specified module
        cannot be handled by the loader.

        """
        bytecode_path = self.bytecode_path(name)
        if not bytecode_path:
            bytecode_path = self._base_path + suffix_list(imp.PY_COMPILED)[0]
        file = _fileio._FileIO(bytecode_path, 'w')
        try:
            with closing(file) as bytecode_file:
                bytecode_file.write(data)
                return True
        except IOError as exc:
            if exc.errno == errno.EACCES:
                return False
            else:
                raise

    @check_name
    def get_code(self, name):
        """Return the code object for the module."""
        # XXX Care enough to make sure this call does not happen if the magic
        #     number is bad?
        source_timestamp = self.source_mtime(name)
        # Try to use bytecode if it is available.
        bytecode_path = self.bytecode_path(name)
        if bytecode_path:
            data = self.get_data(bytecode_path)
            magic = data[:4]
            pyc_timestamp = marshal._r_long(data[4:8])
            bytecode = data[8:]
            try:
                # Verify that the magic number is valid.
                if imp.get_magic() != magic:
                    raise ImportError("bad magic number")
                # Verify that the bytecode is not stale (only matters when
                # there is source to fall back on.
                if source_timestamp:
                    if pyc_timestamp < source_timestamp:
                        raise ImportError("bytcode is stale")
            except ImportError:
                # If source is available give it a shot.
                if source_timestamp is not None:
                    pass
                else:
                    raise
            else:
                # Bytecode seems fine, so try to use it.
                # XXX If the bytecode is ill-formed, would it be beneficial to
                #     try for using source if available and issue a warning?
                return marshal.loads(bytecode)
        elif source_timestamp is None:
            raise ImportError("no source or bytecode available to create code "
                                "object for {0!r}".format(name))
        # Use the source.
        source_path = self.source_path(name)
        source = self.get_data(source_path)
        # Convert to universal newlines.
        line_endings = b'\n'
        for index, c in enumerate(source):
            if c == ord(b'\n'):
                break
            elif c == ord(b'\r'):
                line_endings = b'\r'
                try:
                    if source[index+1] == ord(b'\n'):
                        line_endings += b'\n'
                except IndexError:
                    pass
                break
        if line_endings != b'\n':
            source = source.replace(line_endings, b'\n')
        code_object = compile(source, source_path, 'exec', dont_inherit=True)
        # Generate bytecode and write it out.
        if not sys.dont_write_bytecode:
            data = bytearray(imp.get_magic())
            data.extend(marshal._w_long(source_timestamp))
            data.extend(marshal.dumps(code_object))
            self.write_bytecode(name, data)
        return code_object

    def get_data(self, path):
        """Return the data from path as raw bytes."""
        return _fileio._FileIO(path, 'r').read()

    @check_name
    def is_package(self, fullname):
        """Return a boolean based on whether the module is a package.

        Raises ImportError (like get_source) if the loader cannot handle the
        package.

        """
        return self._is_pkg


class FileImporter(object):

    """Base class for file importers.

    Subclasses are expected to define the following attributes:

        * _suffixes
            Sequence of file suffixes whose order will be followed.

        * _possible_package
            True if importer should check for packages.

        * _loader
            A callable that takes the module name, a file path, and whether
            the path points to a package and returns a loader for the module
            found at that path.

    """

    def __init__(self, path_entry):
        """Initialize an importer for the passed-in sys.path entry (which is
        assumed to have already been verified as an existing directory).

        Can be used as an entry on sys.path_hook.

        """
        self._path_entry = path_entry

    def find_module(self, fullname, path=None):
        tail_module = fullname.rsplit('.', 1)[-1]
        package_directory = None
        if self._possible_package:
            for ext in self._suffixes:
                package_directory = _path_join(self._path_entry, tail_module)
                init_filename = '__init__' + ext
                package_init = _path_join(package_directory, init_filename)
                if (_path_isfile(package_init) and
                        _case_ok(self._path_entry, tail_module) and
                        _case_ok(package_directory, init_filename)):
                    return self._loader(fullname, package_init, True)
        for ext in self._suffixes:
            file_name = tail_module + ext
            file_path = _path_join(self._path_entry, file_name)
            if (_path_isfile(file_path) and
                    _case_ok(self._path_entry, file_name)):
                return self._loader(fullname, file_path, False)
        else:
            # Raise a warning if it matches a directory w/o an __init__ file.
            if (package_directory is not None and
                    _path_isdir(package_directory) and
                    _case_ok(self._path_entry, tail_module)):
                _warnings.warn("Not importing directory %s: missing __init__"
                                % package_directory, ImportWarning)
            return None


class ExtensionFileImporter(FileImporter):

    """Importer for extension files."""

    _possible_package = False
    _loader = _ExtensionFileLoader

    def __init__(self, path_entry):
        # Assigning to _suffixes here instead of at the class level because
        # imp is not imported at the time of class creation.
        self._suffixes = suffix_list(imp.C_EXTENSION)
        super(ExtensionFileImporter, self).__init__(path_entry)


class PyFileImporter(FileImporter):

    """Importer for source/bytecode files."""

    _possible_package = True
    _loader = _PyFileLoader

    def __init__(self, path_entry):
        # Lack of imp during class creation means _suffixes is set here.
        # Make sure that Python source files are listed first!  Needed for an
        # optimization by the loader.
        self._suffixes = suffix_list(imp.PY_SOURCE)
        self._suffixes += suffix_list(imp.PY_COMPILED)
        super(PyFileImporter, self).__init__(path_entry)


class PathFinder:

    """Meta path finder for sys.(path|path_hooks|path_importer_cache)."""

    _default_hook = staticmethod(chaining_fs_path_hook(ExtensionFileImporter,
                                                        PyFileImporter))

    # The list of implicit hooks cannot be a class attribute because of
    # bootstrapping issues for accessing imp.
    @classmethod
    def _implicit_hooks(cls):
        """Return a list of the implicit path hooks."""
        return [cls._default_hook, imp.NullImporter]

    @classmethod
    def _path_hooks(cls, path):
        """Search sys.path_hooks for a finder for 'path'.

        Guaranteed to return a finder for the path as NullImporter is the
        default importer for any path that does not have an explicit finder.

        """
        for hook in sys.path_hooks + cls._implicit_hooks():
            try:
                return hook(path)
            except ImportError:
                continue
        else:
            # This point should never be reached thanks to NullImporter.
            raise SystemError("no hook could find an importer for "
                              "{0}".format(path))

    @classmethod
    def _path_importer_cache(cls, path):
        """Get the finder for the path from sys.path_importer_cache.

        If the path is not in the cache, find the appropriate finder and cache
        it. If None is cached, get the default finder and cache that
        (if applicable).

        Because of NullImporter, some finder should be returned. The only
        explicit fail case is if None is cached but the path cannot be used for
        the default hook, for which ImportError is raised.

        """
        try:
            finder = sys.path_importer_cache[path]
        except KeyError:
            finder = cls._path_hooks(path)
            sys.path_importer_cache[path] = finder
        else:
            if finder is None:
                # Raises ImportError on failure.
                finder = cls._default_hook(path)
                sys.path_importer_cache[path] = finder
        return finder

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find the module on sys.path or 'path'."""
        if not path:
            path = sys.path
        for entry in path:
            try:
                finder = cls._path_importer_cache(entry)
            except ImportError:
                continue
            loader = finder.find_module(fullname)
            if loader:
                return loader
        else:
            return None


class ImportLockContext(object):

    """Context manager for the import lock."""

    def __enter__(self):
        """Acquire the import lock."""
        imp.acquire_lock()

    def __exit__(self, exc_type, exc_value, exc_traceback):
        """Release the import lock regardless of any raised exceptions."""
        imp.release_lock()


def _gcd_import(name, package=None, level=0):
    """Import and return the module based on its name, the package the call is
    being made from, and the level adjustment.

    This function represents the greatest common denominator of functionality
    between import_module and __import__. This includes settting __package__ if
    the loader did not.

    """
    if package:
        if not hasattr(package, 'rindex'):
            raise ValueError("__package__ not set to a string")
        elif package not in sys.modules:
            msg = ("Parent module {0!r} not loaded, cannot perform relative "
                   "import")
            raise SystemError(msg.format(package))
    if not name and level == 0:
        raise ValueError("Empty module name")
    if level > 0:
        dot = len(package)
        for x in range(level, 1, -1):
            try:
                dot = package.rindex('.', 0, dot)
            except ValueError:
                raise ValueError("attempted relative import beyond "
                                 "top-level package")
        if name:
            name = "{0}.{1}".format(package[:dot], name)
        else:
            name = package[:dot]
    with ImportLockContext():
        try:
            return sys.modules[name]
        except KeyError:
            pass
        parent = name.rpartition('.')[0]
        path = None
        if parent:
            if parent not in sys.modules:
                _gcd_import(parent)
            # Backwards-compatibility; be nicer to skip the dict lookup.
            parent_module = sys.modules[parent]
            path = parent_module.__path__
        meta_path = (sys.meta_path +
                     [BuiltinImporter, FrozenImporter, PathFinder])
        for finder in meta_path:
            loader = finder.find_module(name, path)
            if loader is not None:
                loader.load_module(name)
                break
        else:
            raise ImportError("No module named {0}".format(name))
        # Backwards-compatibility; be nicer to skip the dict lookup.
        module = sys.modules[name]
        if parent:
            # Set the module as an attribute on its parent.
            setattr(parent_module, name.rpartition('.')[2], module)
        # Set __package__ if the loader did not.
        if not hasattr(module, '__package__') or module.__package__ is None:
            # Watch out for what comes out of sys.modules to not be a module,
            # e.g. an int.
            try:
                module.__package__ = module.__name__
                if not hasattr(module, '__path__'):
                    module.__package__ = module.__package__.rpartition('.')[0]
            except AttributeError:
                pass
        return module


def _import(name, globals={}, locals={}, fromlist=[], level=0):
    """Import a module.

    The 'globals' argument is used to infer where the import is occuring from
    to handle relative imports. The 'locals' argument is ignored. The
    'fromlist' argument specifies what should exist as attributes on the module
    being imported (e.g. ``from module import <fromlist>``).  The 'level'
    argument represents the package location to import from in a relative
    import (e.g. ``from ..pkg import mod`` would have a 'level' of 2).

    """
    if level == 0:
        module = _gcd_import(name)
    else:
        # __package__ is not guaranteed to be defined.
        try:
            package = globals['__package__']
        except KeyError:
            package = globals['__name__']
            if '__path__' not in globals:
                package = package.rpartition('.')[0]
        module = _gcd_import(name, package, level)
    # The hell that is fromlist ...
    if not fromlist:
        # Return up to the first dot in 'name'. This is complicated by the fact
        # that 'name' may be relative.
        if level == 0:
            return sys.modules[name.partition('.')[0]]
        elif not name:
            return module
        else:
            cut_off = len(name) - len(name.partition('.')[0])
            return sys.modules[module.__name__[:-cut_off]]
    else:
        # If a package was imported, try to import stuff from fromlist.
        if hasattr(module, '__path__'):
            if '*' in fromlist and hasattr(module, '__all__'):
                fromlist.remove('*')
                fromlist.extend(module.__all__)
            for x in (y for y in fromlist if not hasattr(module,y)):
                try:
                    _gcd_import('{0}.{1}'.format(module.__name__, x))
                except ImportError:
                    pass
        return module


# XXX Eventually replace with a proper __all__ value (i.e., don't expose os
# replacements but do expose _ExtensionFileLoader, etc. for testing).
__all__ = [obj for obj in globals().keys() if not obj.startswith('__')]
