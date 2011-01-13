"""Core implementation of import.

This module is NOT meant to be directly imported! It has been designed such
that it can be bootstrapped into Python as the implementation of import. As
such it requires the injection of specific modules and attributes in order to
work. One should use importlib as the public-facing version of this module.

"""

# Injected modules are '_warnings', 'imp', 'sys', 'marshal', 'errno', '_io',
# and '_os' (a.k.a. 'posix', 'nt' or 'os2').
# Injected attribute is path_sep.
#
# When editing this code be aware that code executed at import time CANNOT
# reference any injected objects! This includes not only global code but also
# anything specified at the class level.


# Bootstrap-related code ######################################################

# XXX Could also expose Modules/getpath.c:joinpath()
def _path_join(*args):
    """Replacement for os.path.join."""
    return path_sep.join(x[:-len(path_sep)] if x.endswith(path_sep) else x
                            for x in args if x)


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
    if not path:
        path = _os.getcwd()
    return _path_is_mode_type(path, 0o040000)


def _path_without_ext(path, ext_type):
    """Replacement for os.path.splitext()[0]."""
    for suffix in _suffix_list(ext_type):
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


def _wrap(new, old):
    """Simple substitute for functools.wraps."""
    for replace in ['__module__', '__name__', '__doc__']:
        setattr(new, replace, getattr(old, replace))
    new.__dict__.update(old.__dict__)


code_type = type(_wrap.__code__)

# Finder/loader utility code ##################################################

def set_package(fxn):
    """Set __package__ on the returned module."""
    def wrapper(*args, **kwargs):
        module = fxn(*args, **kwargs)
        if not hasattr(module, '__package__') or module.__package__ is None:
            module.__package__ = module.__name__
            if not hasattr(module, '__path__'):
                module.__package__ = module.__package__.rpartition('.')[0]
        return module
    _wrap(wrapper, fxn)
    return wrapper


def set_loader(fxn):
    """Set __loader__ on the returned module."""
    def wrapper(self, *args, **kwargs):
        module = fxn(self, *args, **kwargs)
        if not hasattr(module, '__loader__'):
            module.__loader__ = self
        return module
    _wrap(wrapper, fxn)
    return wrapper


def module_for_loader(fxn):
    """Decorator to handle selecting the proper module for loaders.

    The decorated function is passed the module to use instead of the module
    name. The module passed in to the function is either from sys.modules if
    it already exists or is a new module which has __name__ set and is inserted
    into sys.modules. If an exception is raised and the decorator created the
    module it is subsequently removed from sys.modules.

    The decorator assumes that the decorated function takes the module name as
    the second argument.

    """
    def decorated(self, fullname, *args, **kwargs):
        module = sys.modules.get(fullname)
        is_reload = bool(module)
        if not is_reload:
            # This must be done before open() is called as the 'io' module
            # implicitly imports 'locale' and would otherwise trigger an
            # infinite loop.
            module = imp.new_module(fullname)
            sys.modules[fullname] = module
        try:
            return fxn(self, module, *args, **kwargs)
        except:
            if not is_reload:
                del sys.modules[fullname]
            raise
    _wrap(decorated, fxn)
    return decorated


def _check_name(method):
    """Decorator to verify that the module being requested matches the one the
    loader can handle.

    The first argument (self) must define _name which the second argument is
    comapred against. If the comparison fails then ImportError is raised.

    """
    def inner(self, name, *args, **kwargs):
        if self._name != name:
            raise ImportError("loader cannot handle %s" % name)
        return method(self, name, *args, **kwargs)
    _wrap(inner, method)
    return inner


def _requires_builtin(fxn):
    """Decorator to verify the named module is built-in."""
    def wrapper(self, fullname):
        if fullname not in sys.builtin_module_names:
            raise ImportError("{0} is not a built-in module".format(fullname))
        return fxn(self, fullname)
    _wrap(wrapper, fxn)
    return wrapper


def _requires_frozen(fxn):
    """Decorator to verify the named module is frozen."""
    def wrapper(self, fullname):
        if not imp.is_frozen(fullname):
            raise ImportError("{0} is not a frozen module".format(fullname))
        return fxn(self, fullname)
    _wrap(wrapper, fxn)
    return wrapper


def _suffix_list(suffix_type):
    """Return a list of file suffixes based on the imp file type."""
    return [suffix[0] for suffix in imp.get_suffixes()
            if suffix[2] == suffix_type]


# Loaders #####################################################################

class BuiltinImporter:

    """Meta path import for built-in modules.

    All methods are either class or static methods to avoid the need to
    instantiate the class.

    """

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find the built-in module.

        If 'path' is ever specified then the search is considered a failure.

        """
        if path is not None:
            return None
        return cls if imp.is_builtin(fullname) else None

    @classmethod
    @set_package
    @set_loader
    @_requires_builtin
    def load_module(cls, fullname):
        """Load a built-in module."""
        is_reload = fullname in sys.modules
        try:
            return imp.init_builtin(fullname)
        except:
            if not is_reload and fullname in sys.modules:
                del sys.modules[fullname]
            raise

    @classmethod
    @_requires_builtin
    def get_code(cls, fullname):
        """Return None as built-in modules do not have code objects."""
        return None

    @classmethod
    @_requires_builtin
    def get_source(cls, fullname):
        """Return None as built-in modules do not have source code."""
        return None

    @classmethod
    @_requires_builtin
    def is_package(cls, fullname):
        """Return None as built-in module are never packages."""
        return False


class FrozenImporter:

    """Meta path import for frozen modules.

    All methods are either class or static methods to avoid the need to
    instantiate the class.

    """

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find a frozen module."""
        return cls if imp.is_frozen(fullname) else None

    @classmethod
    @set_package
    @set_loader
    @_requires_frozen
    def load_module(cls, fullname):
        """Load a frozen module."""
        is_reload = fullname in sys.modules
        try:
            return imp.init_frozen(fullname)
        except:
            if not is_reload and fullname in sys.modules:
                del sys.modules[fullname]
            raise

    @classmethod
    @_requires_frozen
    def get_code(cls, fullname):
        """Return the code object for the frozen module."""
        return imp.get_frozen_object(fullname)

    @classmethod
    @_requires_frozen
    def get_source(cls, fullname):
        """Return None as frozen modules do not have source code."""
        return None

    @classmethod
    @_requires_frozen
    def is_package(cls, fullname):
        """Return if the frozen module is a package."""
        return imp.is_frozen_package(fullname)


class _LoaderBasics:

    """Base class of common code needed by both SourceLoader and
    _SourcelessFileLoader."""

    def is_package(self, fullname):
        """Concrete implementation of InspectLoader.is_package by checking if
        the path returned by get_filename has a filename of '__init__.py'."""
        filename = self.get_filename(fullname).rpartition(path_sep)[2]
        return filename.rsplit('.', 1)[0] == '__init__'

    def _bytes_from_bytecode(self, fullname, data, source_mtime):
        """Return the marshalled bytes from bytecode, verifying the magic
        number and timestamp alon the way.

        If source_mtime is None then skip the timestamp check.

        """
        magic = data[:4]
        raw_timestamp = data[4:8]
        if len(magic) != 4 or magic != imp.get_magic():
            raise ImportError("bad magic number in {}".format(fullname))
        elif len(raw_timestamp) != 4:
            raise EOFError("bad timestamp in {}".format(fullname))
        elif source_mtime is not None:
            if marshal._r_long(raw_timestamp) != source_mtime:
                raise ImportError("bytecode is stale for {}".format(fullname))
        # Can't return the code object as errors from marshal loading need to
        # propagate even when source is available.
        return data[8:]

    @module_for_loader
    def _load_module(self, module, *, sourceless=False):
        """Helper for load_module able to handle either source or sourceless
        loading."""
        name = module.__name__
        code_object = self.get_code(name)
        module.__file__ = self.get_filename(name)
        if not sourceless:
            module.__cached__ = imp.cache_from_source(module.__file__)
        else:
            module.__cached__ = module.__file__
        module.__package__ = name
        if self.is_package(name):
            module.__path__ = [module.__file__.rsplit(path_sep, 1)[0]]
        else:
            module.__package__ = module.__package__.rpartition('.')[0]
        module.__loader__ = self
        exec(code_object, module.__dict__)
        return module


class SourceLoader(_LoaderBasics):

    def path_mtime(self, path):
        """Optional method that returns the modification time (an int) for the
        specified path, where path is a str.

        Implementing this method allows the loader to read bytecode files.

        """
        raise NotImplementedError

    def set_data(self, path, data):
        """Optional method which writes data (bytes) to a file path (a str).

        Implementing this method allows for the writing of bytecode files.

        """
        raise NotImplementedError


    def get_source(self, fullname):
        """Concrete implementation of InspectLoader.get_source."""
        import tokenize
        path = self.get_filename(fullname)
        try:
            source_bytes = self.get_data(path)
        except IOError:
            raise ImportError("source not available through get_data()")
        encoding = tokenize.detect_encoding(_io.BytesIO(source_bytes).readline)
        newline_decoder = _io.IncrementalNewlineDecoder(None, True)
        return newline_decoder.decode(source_bytes.decode(encoding[0]))

    def get_code(self, fullname):
        """Concrete implementation of InspectLoader.get_code.

        Reading of bytecode requires path_mtime to be implemented. To write
        bytecode, set_data must also be implemented.

        """
        source_path = self.get_filename(fullname)
        bytecode_path = imp.cache_from_source(source_path)
        source_mtime = None
        if bytecode_path is not None:
            try:
                source_mtime = self.path_mtime(source_path)
            except NotImplementedError:
                pass
            else:
                try:
                    data = self.get_data(bytecode_path)
                except IOError:
                    pass
                else:
                    try:
                        bytes_data = self._bytes_from_bytecode(fullname, data,
                                                               source_mtime)
                    except (ImportError, EOFError):
                        pass
                    else:
                        found = marshal.loads(bytes_data)
                        if isinstance(found, code_type):
                            return found
                        else:
                            msg = "Non-code object in {}"
                            raise ImportError(msg.format(bytecode_path))
        source_bytes = self.get_data(source_path)
        code_object = compile(source_bytes, source_path, 'exec',
                                dont_inherit=True)
        if (not sys.dont_write_bytecode and bytecode_path is not None and
                source_mtime is not None):
            # If e.g. Jython ever implements imp.cache_from_source to have
            # their own cached file format, this block of code will most likely
            # throw an exception.
            data = bytearray(imp.get_magic())
            data.extend(marshal._w_long(source_mtime))
            data.extend(marshal.dumps(code_object))
            try:
                self.set_data(bytecode_path, data)
            except NotImplementedError:
                pass
        return code_object

    def load_module(self, fullname):
        """Concrete implementation of Loader.load_module.

        Requires ExecutionLoader.get_filename and ResourceLoader.get_data to be
        implemented to load source code. Use of bytecode is dictated by whether
        get_code uses/writes bytecode.

        """
        return self._load_module(fullname)


class _FileLoader:

    """Base file loader class which implements the loader protocol methods that
    require file system usage."""

    def __init__(self, fullname, path):
        """Cache the module name and the path to the file found by the
        finder."""
        self._name = fullname
        self._path = path

    @_check_name
    def get_filename(self, fullname):
        """Return the path to the source file as found by the finder."""
        return self._path

    def get_data(self, path):
        """Return the data from path as raw bytes."""
        with _io.FileIO(path, 'r') as file:
            return file.read()


class _SourceFileLoader(_FileLoader, SourceLoader):

    """Concrete implementation of SourceLoader using the file system."""

    def path_mtime(self, path):
        """Return the modification time for the path."""
        return int(_os.stat(path).st_mtime)

    def set_data(self, path, data):
        """Write bytes data to a file."""
        parent, _, filename = path.rpartition(path_sep)
        path_parts = []
        # Figure out what directories are missing.
        while parent and not _path_isdir(parent):
            parent, _, part = parent.rpartition(path_sep)
            path_parts.append(part)
        # Create needed directories.
        for part in reversed(path_parts):
            parent = _path_join(parent, part)
            try:
                _os.mkdir(parent)
            except OSError as exc:
                # Probably another Python process already created the dir.
                if exc.errno == errno.EEXIST:
                    continue
                else:
                    raise
            except IOError as exc:
                # If can't get proper access, then just forget about writing
                # the data.
                if exc.errno == errno.EACCES:
                    return
                else:
                    raise
        try:
            with _io.FileIO(path, 'wb') as file:
                file.write(data)
        except IOError as exc:
            # Don't worry if you can't write bytecode.
            if exc.errno == errno.EACCES:
                return
            else:
                raise


class _SourcelessFileLoader(_FileLoader, _LoaderBasics):

    """Loader which handles sourceless file imports."""

    def load_module(self, fullname):
        return self._load_module(fullname, sourceless=True)

    def get_code(self, fullname):
        path = self.get_filename(fullname)
        data = self.get_data(path)
        bytes_data = self._bytes_from_bytecode(fullname, data, None)
        found = marshal.loads(bytes_data)
        if isinstance(found, code_type):
            return found
        else:
            raise ImportError("Non-code object in {}".format(path))

    def get_source(self, fullname):
        """Return None as there is no source code."""
        return None


class _ExtensionFileLoader:

    """Loader for extension modules.

    The constructor is designed to work with FileFinder.

    """

    def __init__(self, name, path):
        """Initialize the loader.

        If is_pkg is True then an exception is raised as extension modules
        cannot be the __init__ module for an extension module.

        """
        self._name = name
        self._path = path

    @_check_name
    @set_package
    @set_loader
    def load_module(self, fullname):
        """Load an extension module."""
        is_reload = fullname in sys.modules
        try:
            return imp.load_dynamic(fullname, self._path)
        except:
            if not is_reload and fullname in sys.modules:
                del sys.modules[fullname]
            raise

    @_check_name
    def is_package(self, fullname):
        """Return False as an extension module can never be a package."""
        return False

    @_check_name
    def get_code(self, fullname):
        """Return None as an extension module cannot create a code object."""
        return None

    @_check_name
    def get_source(self, fullname):
        """Return None as extension modules have no source code."""
        return None


# Finders #####################################################################

class PathFinder:

    """Meta path finder for sys.(path|path_hooks|path_importer_cache)."""

    @classmethod
    def _path_hooks(cls, path, hooks=None):
        """Search sequence of hooks for a finder for 'path'.

        If 'hooks' is false then use sys.path_hooks.

        """
        if not hooks:
            hooks = sys.path_hooks
        for hook in hooks:
            try:
                return hook(path)
            except ImportError:
                continue
        else:
            raise ImportError("no path hook found for {0}".format(path))

    @classmethod
    def _path_importer_cache(cls, path, default=None):
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
            if finder is None and default:
                # Raises ImportError on failure.
                finder = default(path)
                sys.path_importer_cache[path] = finder
        return finder

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find the module on sys.path or 'path' based on sys.path_hooks and
        sys.path_importer_cache."""
        if not path:
            path = sys.path
        for entry in path:
            try:
                finder = cls._path_importer_cache(entry)
            except ImportError:
                continue
            if finder:
                loader = finder.find_module(fullname)
                if loader:
                    return loader
        else:
            return None


class _FileFinder:

    """File-based finder.

    Constructor takes a list of objects detailing what file extensions their
    loader supports along with whether it can be used for a package.

    """

    def __init__(self, path, *details):
        """Initialize with finder details."""
        packages = []
        modules = []
        for detail in details:
            modules.extend((suffix, detail.loader) for suffix in detail.suffixes)
            if detail.supports_packages:
                packages.extend((suffix, detail.loader)
                                for suffix in detail.suffixes)
        self.packages = packages
        self.modules = modules
        self.path = path

    def find_module(self, fullname):
        """Try to find a loader for the specified module."""
        tail_module = fullname.rpartition('.')[2]
        base_path = _path_join(self.path, tail_module)
        if _path_isdir(base_path) and _case_ok(self.path, tail_module):
            for suffix, loader in self.packages:
                init_filename = '__init__' + suffix
                full_path = _path_join(base_path, init_filename)
                if (_path_isfile(full_path) and
                        _case_ok(base_path, init_filename)):
                    return loader(fullname, full_path)
            else:
                msg = "Not importing directory {}: missing __init__"
                _warnings.warn(msg.format(base_path), ImportWarning)
        for suffix, loader in self.modules:
            mod_filename = tail_module + suffix
            full_path = _path_join(self.path, mod_filename)
            if _path_isfile(full_path) and _case_ok(self.path, mod_filename):
                return loader(fullname, full_path)
        return None

class _SourceFinderDetails:

    loader = _SourceFileLoader
    supports_packages = True

    def __init__(self):
        self.suffixes = _suffix_list(imp.PY_SOURCE)

class _SourcelessFinderDetails:

    loader = _SourcelessFileLoader
    supports_packages = True

    def __init__(self):
        self.suffixes = _suffix_list(imp.PY_COMPILED)


class _ExtensionFinderDetails:

    loader = _ExtensionFileLoader
    supports_packages = False

    def __init__(self):
        self.suffixes = _suffix_list(imp.C_EXTENSION)


# Import itself ###############################################################

def _file_path_hook(path):
    """If the path is a directory, return a file-based finder."""
    if _path_isdir(path):
        return _FileFinder(path, _ExtensionFinderDetails(),
                           _SourceFinderDetails(),
                           _SourcelessFinderDetails())
    else:
        raise ImportError("only directories are supported")


_DEFAULT_PATH_HOOK = _file_path_hook

class _DefaultPathFinder(PathFinder):

    """Subclass of PathFinder that implements implicit semantics for
    __import__."""

    @classmethod
    def _path_hooks(cls, path):
        """Search sys.path_hooks as well as implicit path hooks."""
        try:
            return super()._path_hooks(path)
        except ImportError:
            implicit_hooks = [_DEFAULT_PATH_HOOK, imp.NullImporter]
            return super()._path_hooks(path, implicit_hooks)

    @classmethod
    def _path_importer_cache(cls, path):
        """Use the default path hook when None is stored in
        sys.path_importer_cache."""
        return super()._path_importer_cache(path, _DEFAULT_PATH_HOOK)


class _ImportLockContext:

    """Context manager for the import lock."""

    def __enter__(self):
        """Acquire the import lock."""
        imp.acquire_lock()

    def __exit__(self, exc_type, exc_value, exc_traceback):
        """Release the import lock regardless of any raised exceptions."""
        imp.release_lock()


_IMPLICIT_META_PATH = [BuiltinImporter, FrozenImporter, _DefaultPathFinder]

_ERR_MSG = 'No module named {}'

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
    with _ImportLockContext():
        try:
            module = sys.modules[name]
            if module is None:
                message = ("import of {} halted; "
                            "None in sys.modules".format(name))
                raise ImportError(message)
            return module
        except KeyError:
            pass
        parent = name.rpartition('.')[0]
        path = None
        if parent:
            if parent not in sys.modules:
                _gcd_import(parent)
            # Backwards-compatibility; be nicer to skip the dict lookup.
            parent_module = sys.modules[parent]
            try:
                path = parent_module.__path__
            except AttributeError:
                msg = (_ERR_MSG + '; {} is not a package').format(name, parent)
                raise ImportError(msg)
        meta_path = sys.meta_path + _IMPLICIT_META_PATH
        for finder in meta_path:
            loader = finder.find_module(name, path)
            if loader is not None:
                loader.load_module(name)
                break
        else:
            raise ImportError(_ERR_MSG.format(name))
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


def __import__(name, globals={}, locals={}, fromlist=[], level=0):
    """Import a module.

    The 'globals' argument is used to infer where the import is occuring from
    to handle relative imports. The 'locals' argument is ignored. The
    'fromlist' argument specifies what should exist as attributes on the module
    being imported (e.g. ``from module import <fromlist>``).  The 'level'
    argument represents the package location to import from in a relative
    import (e.g. ``from ..pkg import mod`` would have a 'level' of 2).

    """
    if not hasattr(name, 'rpartition'):
        raise TypeError("module name must be str, not {}".format(type(name)))
    if level == 0:
        module = _gcd_import(name)
    else:
        # __package__ is not guaranteed to be defined or could be set to None
        # to represent that it's proper value is unknown
        package = globals.get('__package__')
        if package is None:
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
                fromlist = list(fromlist)
                fromlist.remove('*')
                fromlist.extend(module.__all__)
            for x in (y for y in fromlist if not hasattr(module,y)):
                try:
                    _gcd_import('{0}.{1}'.format(module.__name__, x))
                except ImportError:
                    pass
        return module
