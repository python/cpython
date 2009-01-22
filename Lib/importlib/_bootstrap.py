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

    @staticmethod
    def load_module(fullname):
        """Load a built-in module."""
        if fullname not in sys.builtin_module_names:
            raise ImportError("{0} is not a built-in module".format(fullname))
        return imp.init_builtin(fullname)


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
    def load_module(cls, fullname):
        """Load a frozen module."""
        if cls.find_module(fullname) is None:
            raise ImportError("{0} is not a frozen module".format(fullname))
        return imp.init_frozen(fullname)


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

    def _source_path(self):
        """Return the path to an existing source file for the module, or None
        if one cannot be found."""
        # Not a property so that it is easy to override.
        return self._find_path(imp.PY_SOURCE)

    def _bytecode_path(self):
        """Return the path to a bytecode file, or None if one does not
        exist."""
        # Not a property for easy overriding.
        return self._find_path(imp.PY_COMPILED)

    @check_name
    @get_module
    def load_module(self, module):
        """Load a Python source or bytecode module."""
        source_path = self._source_path()
        bytecode_path = self._bytecode_path()
        code_object = self.get_code(module.__name__)
        module.__file__ = source_path if source_path else bytecode_path
        module.__loader__ = self
        if self._is_pkg:
            module.__path__  = [module.__file__.rsplit(path_sep, 1)[0]]
            module.__package__ = module.__name__
        elif '.' in module.__name__:
            module.__package__ = module.__name__.rsplit('.', 1)[0]
        else:
            module.__package__ = None
        exec(code_object, module.__dict__)
        return module

    @check_name
    def source_mtime(self, name):
        """Return the modification time of the source for the specified
        module."""
        source_path = self._source_path()
        if not source_path:
            return None
        return int(_os.stat(source_path).st_mtime)

    @check_name
    def get_source(self, fullname):
        """Return the source for the module as a string.

        Return None if the source is not available. Raise ImportError if the
        laoder cannot handle the specified module.

        """
        source_path = self._source_path()
        if source_path is None:
            return None
        import tokenize
        with closing(_fileio._FileIO(source_path, 'r')) as file:
            encoding, lines = tokenize.detect_encoding(file.readline)
        # XXX Will fail when passed to compile() if the encoding is
        # anything other than UTF-8.
        return open(source_path, encoding=encoding).read()

    @check_name
    def read_source(self, fullname):
        """Return the source for the specified module as bytes along with the
        path where the source came from.

        The returned path is used by 'compile' for error messages.

        """
        source_path = self._source_path()
        if source_path is None:
            return None
        with closing(_fileio._FileIO(source_path, 'r')) as bytes_file:
            return bytes_file.read(), source_path

    @check_name
    def read_bytecode(self, name):
        """Return the magic number, timestamp, and the module bytecode for the
        module.

        Raises ImportError (just like get_source) if the laoder cannot handle
        the module. Returns None if there is no bytecode.

        """
        path = self._bytecode_path()
        if path is None:
            return None
        file = _fileio._FileIO(path, 'r')
        try:
            with closing(file) as bytecode_file:
                data = bytecode_file.read()
            return data[:4], marshal._r_long(data[4:8]), data[8:]
        except AttributeError:
            return None

    @check_name
    def write_bytecode(self, name, magic, timestamp, data):
        """Write out 'data' for the specified module using the specific
        timestamp, returning a boolean
        signifying if the write-out actually occurred.

        Raises ImportError (just like get_source) if the specified module
        cannot be handled by the loader.

        """
        bytecode_path = self._bytecode_path()
        if not bytecode_path:
            bytecode_path = self._base_path + suffix_list(imp.PY_COMPILED)[0]
        file = _fileio._FileIO(bytecode_path, 'w')
        try:
            with closing(file) as bytecode_file:
                bytecode_file.write(magic)
                bytecode_file.write(marshal._w_long(timestamp))
                bytecode_file.write(data)
                return True
        except IOError as exc:
            if exc.errno == errno.EACCES:
                return False
            else:
                raise

    # XXX Take an optional argument to flag whether to write bytecode?
    @check_name
    def get_code(self, name):
        """Return the code object for the module.

            'self' must implement:

            * read_bytecode(name:str) -> (int, int, bytes) or None
                Return the magic number, timestamp, and bytecode for the
                module. None is returned if not bytecode is available.

            * source_mtime(name:str) -> int
                Return the last modification time for the source of the module.
                Returns None if their is no source.

            * read_source(name:str) -> (bytes, str)
                Return the source code for the module and the path to use in
                the call to 'compile'. Not called if source_mtime returned
                None.

            * write_bytecode(name:str, magic:bytes, timestamp:int, data:str)
                Write out bytecode for the module with the specified magic
                number and timestamp. Not called if sys.dont_write_bytecode is
                True.

        """
        # XXX Care enough to make sure this call does not happen if the magic
        #     number is bad?
        source_timestamp = self.source_mtime(name)
        # Try to use bytecode if it is available.
        bytecode_tuple = self.read_bytecode(name)
        if bytecode_tuple:
            magic, pyc_timestamp, bytecode = bytecode_tuple
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
        source, source_path = self.read_source(name)
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
            data = marshal.dumps(code_object)
            self.write_bytecode(name, imp.get_magic(), source_timestamp, data)
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


class ImportLockContext(object):

    """Context manager for the import lock."""

    def __enter__(self):
        """Acquire the import lock."""
        imp.acquire_lock()

    def __exit__(self, exc_type, exc_value, exc_traceback):
        """Release the import lock regardless of any raised exceptions."""
        imp.release_lock()


class Import(object):

    """Class that implements the __import__ interface.

    Backwards compatibility is maintained  by extending sys.meta_path
    interally (for handling built-in and frozen modules) and providing a
    default path hooks entry for extension modules, .py, and .pyc
    files.  Both are controlled during instance initialization.

    """

    def __init__(self, default_path_hook=None,
                 extended_meta_path=None):
        """Store a default path hook entry and a sequence to internally extend
        sys.meta_path by (passing in None uses default importers)."""
        if extended_meta_path is None:
            self.extended_meta_path = BuiltinImporter, FrozenImporter
        else:
            self.extended_meta_path = extended_meta_path
        self.default_path_hook = default_path_hook
        if self.default_path_hook is None:
            # Create a handler to deal with extension modules, .py, and .pyc
            # files.  Built-in and frozen modules are handled by sys.meta_path
            # entries.
            importers = [ExtensionFileImporter, PyFileImporter]
            self.default_path_hook = chaining_fs_path_hook(*importers)

    def _search_meta_path(self, name, path=None):
        """Check the importers on sys.meta_path for a loader along with the
        extended meta path sequence stored within this instance.

        The extended sys.meta_path entries are searched after the entries on
        sys.meta_path.

        """
        for entry in (tuple(sys.meta_path) + self.extended_meta_path):
            loader = entry.find_module(name, path)
            if loader:
                return loader
        else:
            raise ImportError("No module named %s" % name)

    def _sys_path_importer(self, path_entry):
        """Return the importer for the specified path, from
        sys.path_importer_cache if possible.

        If None is stored in sys.path_importer_cache then use the default path
        hook.

        """
        try:
            # See if an importer is cached.
            importer = sys.path_importer_cache[path_entry]
            # If None was returned, use default importer factory.
            if importer is None:
                return self.default_path_hook(path_entry)
            else:
                return importer
        except KeyError:
            # No cached importer found; try to get a new one from
            # sys.path_hooks or imp.NullImporter.
            for importer_factory in (sys.path_hooks + [imp.NullImporter]):
                try:
                    importer = importer_factory(path_entry)
                    sys.path_importer_cache[path_entry] = importer
                    return importer
                except ImportError:
                    continue
            else:
                # No importer factory on sys.path_hooks works; use the default
                # importer factory and store None in sys.path_importer_cache.
                try:
                    importer = self.default_path_hook(path_entry)
                    sys.path_importer_cache[path_entry] = None
                    return importer
                except ImportError:
                    raise ImportError("no importer found for %s" % path_entry)

    def _search_std_path(self, name, path=None):
        """Check sys.path or 'path' (depending if 'path' is set) for the
        named module and return its loader."""
        if path:
            search_paths = path
        else:
            search_paths = sys.path
        for entry in search_paths:
            try:
                importer = self._sys_path_importer(entry)
            except ImportError:
                continue
            loader = importer.find_module(name)
            if loader:
                return loader
        else:
            raise ImportError("No module named %s" % name)

    def module_from_cache(self, name):
        """Try to return the named module from sys.modules.

        Return False if the module is not in the cache.
        """
        if name in sys.modules:
            return sys.modules[name]
        else:
            return False

    def post_import(self, module):
        """Perform any desired post-import processing on the module."""
        return module

    def _import_module(self, name, path=None):
        """Import the specified module with no handling of parent modules.

        If None is set for a value in sys.modules (to signify that a relative
        import was attempted and failed) then ImportError is raised.

        """
        cached_module = self.module_from_cache(name)
        if cached_module is not False:
            if cached_module is None:
                raise ImportError("relative import redirect")
            else:
                return cached_module
        try:
            # Attempt to find a loader on sys.meta_path.
            loader = self._search_meta_path(name, path)
        except ImportError:
            # sys.meta_path search failed.  Attempt to find a loader on
            # sys.path.  If this fails then module cannot be found.
            loader = self._search_std_path(name, path)
        # A loader was found.  It is the loader's responsibility to have put an
        # entry in sys.modules.
        module = self.post_import(loader.load_module(name))
        # 'module' could be something like None.
        if not hasattr(module, '__name__'):
            return module
        # Set __package__.
        if not hasattr(module, '__package__') or module.__package__ is None:
            if hasattr(module, '__path__'):
                module.__package__ = module.__name__
            elif '.' in module.__name__:
                pkg_name = module.__name__.rsplit('.', 1)[0]
                module.__package__ = pkg_name
            else:
                module.__package__ = None
        return module


    def _import_full_module(self, name):
        """Import a module and set it on its parent if needed."""
        path_list = None
        parent_name = name.rsplit('.', 1)[0]
        parent = None
        if parent_name != name:
            parent = sys.modules[parent_name]
            try:
                path_list = parent.__path__
            except AttributeError:
                pass
        self._import_module(name, path_list)
        module = sys.modules[name]
        if parent:
            tail = name.rsplit('.', 1)[-1]
            setattr(parent, tail, module)

    def _find_package(self, name, has_path):
        """Return the package that the caller is in or None."""
        if has_path:
            return name
        elif '.' in name:
            return name.rsplit('.', 1)[0]
        else:
            return None

    @staticmethod
    def _resolve_name(name, package, level):
        """Return the absolute name of the module to be imported."""
        level -= 1
        try:
            if package.count('.') < level:
                raise ValueError("attempted relative import beyond top-level "
                                  "package")
        except AttributeError:
            raise ValueError("__package__ not set to a string")
        base = package.rsplit('.', level)[0]
        if name:
            return "{0}.{1}".format(base, name)
        else:
            return base

    def _return_module(self, absolute_name, relative_name, fromlist):
        """Return the proper module based on what module was requested (and its
        absolute module name), who is requesting it, and whether any specific
        attributes were specified.

        The semantics of this method revolve around 'fromlist'.  When it is
        empty, the module up to the first dot is to be returned.  When the
        module being requested is an absolute name this is simple (and
        relative_name is an empty string).  But if the requested module was
        a relative import (as signaled by relative_name having a non-false
        value), then the name up to the first dot in the relative name resolved
        to an absolute name is to be returned.

        When fromlist is not empty and the module being imported is a package,
        then the values
        in fromlist need to be checked for.  If a value is not a pre-existing
        attribute a relative import is attempted.  If it fails then suppressed
        the failure silently.

        """
        if not fromlist:
            if relative_name:
                absolute_base = absolute_name.rpartition(relative_name)[0]
                relative_head = relative_name.split('.', 1)[0]
                to_return = absolute_base + relative_head
            else:
                to_return = absolute_name.split('.', 1)[0]
            return sys.modules[to_return]
        # When fromlist is not empty, return the actual module specified in
        # the import.
        else:
            module = sys.modules[absolute_name]
            if hasattr(module, '__path__') and hasattr(module, '__name__'):
                # When fromlist has a value and the imported module is a
                # package, then if a name in fromlist is not found as an
                # attribute on module, try a relative import to find it.
                # Failure is fine and the exception is suppressed.
                check_for = list(fromlist)
                if '*' in check_for and hasattr(module, '__all__'):
                    check_for.extend(module.__all__)
                for item in check_for:
                    if item == '*':
                        continue
                    if not hasattr(module, item):
                        resolved_name = self._resolve_name(item,
                                                            module.__name__, 1)
                        try:
                            self._import_full_module(resolved_name)
                        except ImportError:
                            pass
            return module

    def __call__(self, name, globals={}, locals={}, fromlist=[], level=0):
        """Import a module.

        The 'name' argument is the name of the module to be imported (e.g.,
        'foo' in ``import foo`` or ``from foo import ...``).

        'globals' and 'locals' are the global and local namespace dictionaries
        of the module where the import statement appears.  'globals' is used to
        introspect the __path__ and __name__ attributes of the module making
        the call.  'local's is ignored.

        'fromlist' lists any specific objects that are to eventually be put
        into the namespace (e.g., ``from for.bar import baz`` would have 'baz'
        in the fromlist, and this includes '*').  An entry of '*' will lead to
        a check for __all__ being defined on the module.  If it is defined then
        the values in __all__ will be checked to make sure that all values are
        attributes on the module, attempting a module import relative to 'name'
        to set that attribute.

        When 'name' is a dotted name, there are two different situations to
        consider for the return value.  One is when the fromlist is empty.
        In this situation the import statement imports and returns the name up
        to the first dot.  All subsequent names are imported but set as
        attributes as needed on parent modules.  When fromlist is not empty
        then the module represented by the full dotted name is returned.

        'level' represents possible relative imports.
        A value of 0 is for absolute module names. Any positive value
        represents the number of dots listed in the relative import statement
        (e.g. has a value of 2 for ``from .. import foo``).

        """
        if not name and level < 1:
            raise ValueError("Empty module name")
        is_pkg = True if '__path__' in globals else False
        caller_name = globals.get('__name__')
        package = globals.get('__package__')
        if caller_name and not package:
            package = self._find_package(caller_name, '__path__' in globals)
        if package and package not in sys.modules:
            if not hasattr(package, 'rsplit'):
                raise ValueError("__package__ not set to a string")
            msg = ("Parent module {0!r} not loaded, "
                    "cannot perform relative import")
            raise SystemError(msg.format(package))
        with ImportLockContext():
            if level:
                imported_name = self._resolve_name(name, package, level)
            else:
                imported_name = name
            parent_name = imported_name.rsplit('.', 1)[0]
            if parent_name != imported_name and parent_name not in sys.modules:
                self.__call__(parent_name, level=0)
            # This call will also handle setting the attribute on the
            # package.
            self._import_full_module(imported_name)
            relative_name = '' if imported_name == name else name
            return self._return_module(imported_name, relative_name, fromlist)

# XXX Eventually replace with a proper __all__ value (i.e., don't expose os
# replacements but do expose _ExtensionFileLoader, etc. for testing).
__all__ = [obj for obj in globals().keys() if not obj.startswith('__')]
