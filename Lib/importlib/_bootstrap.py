"""Core implementation of import.

This module is NOT meant to be directly imported! It has been designed such
that it can be bootstrapped into Python as the implementation of import. As
such it requires the injection of specific modules and attributes in order to
work. One should use importlib as the public-facing version of this module.

"""
#
# IMPORTANT: Whenever making changes to this module, be sure to run
# a top-level make in order to get the frozen version of the module
# update. Not doing so, will result in the Makefile to fail for
# all others who don't have a ./python around to freeze the module
# in the early stages of compilation.
#

# See importlib._setup() for what is injected into the global namespace.

# When editing this code be aware that code executed at import time CANNOT
# reference any injected objects! This includes not only global code but also
# anything specified at the class level.

# XXX Make sure all public names have no single leading underscore and all
#     others do.


# Bootstrap-related code ######################################################

_CASE_INSENSITIVE_PLATFORMS = 'win', 'cygwin', 'darwin'


def _make_relax_case():
    if sys.platform.startswith(_CASE_INSENSITIVE_PLATFORMS):
        def _relax_case():
            """True if filenames must be checked case-insensitively."""
            return b'PYTHONCASEOK' in _os.environ
    else:
        def _relax_case():
            """True if filenames must be checked case-insensitively."""
            return False
    return _relax_case


# TODO: Expose from marshal
def _w_long(x):
    """Convert a 32-bit integer to little-endian.

    XXX Temporary until marshal's long functions are exposed.

    """
    x = int(x)
    int_bytes = []
    int_bytes.append(x & 0xFF)
    int_bytes.append((x >> 8) & 0xFF)
    int_bytes.append((x >> 16) & 0xFF)
    int_bytes.append((x >> 24) & 0xFF)
    return bytearray(int_bytes)


# TODO: Expose from marshal
def _r_long(int_bytes):
    """Convert 4 bytes in little-endian to an integer.

    XXX Temporary until marshal's long function are exposed.

    """
    x = int_bytes[0]
    x |= int_bytes[1] << 8
    x |= int_bytes[2] << 16
    x |= int_bytes[3] << 24
    return x


def _path_join(*path_parts):
    """Replacement for os.path.join()."""
    new_parts = []
    for part in path_parts:
        if not part:
            continue
        new_parts.append(part)
        if part[-1] not in path_separators:
            new_parts.append(path_sep)
    return ''.join(new_parts[:-1])  # Drop superfluous path separator.


def _path_split(path):
    """Replacement for os.path.split()."""
    for x in reversed(path):
        if x in path_separators:
            sep = x
            break
    else:
        sep = path_sep
    front, _, tail = path.rpartition(sep)
    return front, tail


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


def _write_atomic(path, data):
    """Best-effort function to write data to a path atomically.
    Be prepared to handle a FileExistsError if concurrent writing of the
    temporary file is attempted."""
    # id() is used to generate a pseudo-random filename.
    path_tmp = '{}.{}'.format(path, id(path))
    fd = _os.open(path_tmp, _os.O_EXCL | _os.O_CREAT | _os.O_WRONLY, 0o666)
    try:
        # We first write data to a temporary file, and then use os.replace() to
        # perform an atomic rename.
        with _io.FileIO(fd, 'wb') as file:
            file.write(data)
        _os.replace(path_tmp, path)
    except OSError:
        try:
            _os.unlink(path_tmp)
        except OSError:
            pass
        raise


def _wrap(new, old):
    """Simple substitute for functools.update_wrapper."""
    for replace in ['__module__', '__name__', '__qualname__', '__doc__']:
        if hasattr(old, replace):
            setattr(new, replace, getattr(old, replace))
    new.__dict__.update(old.__dict__)


_code_type = type(_wrap.__code__)


def new_module(name):
    """Create a new module.

    The module is not entered into sys.modules.

    """
    return type(_io)(name)


# Module-level locking ########################################################

# A dict mapping module names to weakrefs of _ModuleLock instances
_module_locks = {}
# A dict mapping thread ids to _ModuleLock instances
_blocking_on = {}


class _DeadlockError(RuntimeError):
    pass


class _ModuleLock:
    """A recursive lock implementation which is able to detect deadlocks
    (e.g. thread 1 trying to take locks A then B, and thread 2 trying to
    take locks B then A).
    """

    def __init__(self, name):
        self.lock = _thread.allocate_lock()
        self.wakeup = _thread.allocate_lock()
        self.name = name
        self.owner = None
        self.count = 0
        self.waiters = 0

    def has_deadlock(self):
        # Deadlock avoidance for concurrent circular imports.
        me = _thread.get_ident()
        tid = self.owner
        while True:
            lock = _blocking_on.get(tid)
            if lock is None:
                return False
            tid = lock.owner
            if tid == me:
                return True

    def acquire(self):
        """
        Acquire the module lock.  If a potential deadlock is detected,
        a _DeadlockError is raised.
        Otherwise, the lock is always acquired and True is returned.
        """
        tid = _thread.get_ident()
        _blocking_on[tid] = self
        try:
            while True:
                with self.lock:
                    if self.count == 0 or self.owner == tid:
                        self.owner = tid
                        self.count += 1
                        return True
                    if self.has_deadlock():
                        raise _DeadlockError("deadlock detected by %r" % self)
                    if self.wakeup.acquire(False):
                        self.waiters += 1
                # Wait for a release() call
                self.wakeup.acquire()
                self.wakeup.release()
        finally:
            del _blocking_on[tid]

    def release(self):
        tid = _thread.get_ident()
        with self.lock:
            if self.owner != tid:
                raise RuntimeError("cannot release un-acquired lock")
            assert self.count > 0
            self.count -= 1
            if self.count == 0:
                self.owner = None
                if self.waiters:
                    self.waiters -= 1
                    self.wakeup.release()

    def __repr__(self):
        return "_ModuleLock(%r) at %d" % (self.name, id(self))


class _DummyModuleLock:
    """A simple _ModuleLock equivalent for Python builds without
    multi-threading support."""

    def __init__(self, name):
        self.name = name
        self.count = 0

    def acquire(self):
        self.count += 1
        return True

    def release(self):
        if self.count == 0:
            raise RuntimeError("cannot release un-acquired lock")
        self.count -= 1

    def __repr__(self):
        return "_DummyModuleLock(%r) at %d" % (self.name, id(self))


# The following two functions are for consumption by Python/import.c.

def _get_module_lock(name):
    """Get or create the module lock for a given module name.

    Should only be called with the import lock taken."""
    lock = None
    if name in _module_locks:
        lock = _module_locks[name]()
    if lock is None:
        if _thread is None:
            lock = _DummyModuleLock(name)
        else:
            lock = _ModuleLock(name)
        def cb(_):
            del _module_locks[name]
        _module_locks[name] = _weakref.ref(lock, cb)
    return lock

def _lock_unlock_module(name):
    """Release the global import lock, and acquires then release the
    module lock for a given module name.
    This is used to ensure a module is completely initialized, in the
    event it is being imported by another thread.

    Should only be called with the import lock taken."""
    lock = _get_module_lock(name)
    _imp.release_lock()
    try:
        lock.acquire()
    except _DeadlockError:
        # Concurrent circular import, we'll accept a partially initialized
        # module object.
        pass
    else:
        lock.release()


# Finder/loader utility code ##################################################

_PYCACHE = '__pycache__'

SOURCE_SUFFIXES = ['.py']  # _setup() adds .pyw as needed.

DEBUG_BYTECODE_SUFFIXES = ['.pyc']
OPTIMIZED_BYTECODE_SUFFIXES = ['.pyo']
if __debug__:
    BYTECODE_SUFFIXES = DEBUG_BYTECODE_SUFFIXES
else:
    BYTECODE_SUFFIXES = OPTIMIZED_BYTECODE_SUFFIXES

def cache_from_source(path, debug_override=None):
    """Given the path to a .py file, return the path to its .pyc/.pyo file.

    The .py file does not need to exist; this simply returns the path to the
    .pyc/.pyo file calculated as if the .py file were imported.  The extension
    will be .pyc unless __debug__ is not defined, then it will be .pyo.

    If debug_override is not None, then it must be a boolean and is taken as
    the value of __debug__ instead.

    If sys.implementation.cache_tag is None then NotImplementedError is raised.

    """
    debug = __debug__ if debug_override is None else debug_override
    if debug:
        suffixes = DEBUG_BYTECODE_SUFFIXES
    else:
        suffixes = OPTIMIZED_BYTECODE_SUFFIXES
    head, tail = _path_split(path)
    base_filename, sep, _ = tail.partition('.')
    tag = sys.implementation.cache_tag
    if tag is None:
        raise NotImplementedError('sys.implementation.cache_tag is None')
    filename = ''.join([base_filename, sep, tag, suffixes[0]])
    return _path_join(head, _PYCACHE, filename)


def _verbose_message(message, *args):
    """Print the message to stderr if -v/PYTHONVERBOSE is turned on."""
    if sys.flags.verbose:
        if not message.startswith(('#', 'import ')):
            message = '# ' + message
        print(message.format(*args), file=sys.stderr)


def set_package(fxn):
    """Set __package__ on the returned module."""
    def set_package_wrapper(*args, **kwargs):
        module = fxn(*args, **kwargs)
        if getattr(module, '__package__', None) is None:
            module.__package__ = module.__name__
            if not hasattr(module, '__path__'):
                module.__package__ = module.__package__.rpartition('.')[0]
        return module
    _wrap(set_package_wrapper, fxn)
    return set_package_wrapper


def set_loader(fxn):
    """Set __loader__ on the returned module."""
    def set_loader_wrapper(self, *args, **kwargs):
        module = fxn(self, *args, **kwargs)
        if not hasattr(module, '__loader__'):
            module.__loader__ = self
        return module
    _wrap(set_loader_wrapper, fxn)
    return set_loader_wrapper


def module_for_loader(fxn):
    """Decorator to handle selecting the proper module for loaders.

    The decorated function is passed the module to use instead of the module
    name. The module passed in to the function is either from sys.modules if
    it already exists or is a new module. If the module is new, then __name__
    is set the first argument to the method, __loader__ is set to self, and
    __package__ is set accordingly (if self.is_package() is defined) will be set
    before it is passed to the decorated function (if self.is_package() does
    not work for the module it will be set post-load).

    If an exception is raised and the decorator created the module it is
    subsequently removed from sys.modules.

    The decorator assumes that the decorated function takes the module name as
    the second argument.

    """
    def module_for_loader_wrapper(self, fullname, *args, **kwargs):
        module = sys.modules.get(fullname)
        is_reload = module is not None
        if not is_reload:
            # This must be done before open() is called as the 'io' module
            # implicitly imports 'locale' and would otherwise trigger an
            # infinite loop.
            module = new_module(fullname)
            sys.modules[fullname] = module
            module.__loader__ = self
            try:
                is_package = self.is_package(fullname)
            except (ImportError, AttributeError):
                pass
            else:
                if is_package:
                    module.__package__ = fullname
                else:
                    module.__package__ = fullname.rpartition('.')[0]
        try:
            module.__initializing__ = True
            # If __package__ was not set above, __import__() will do it later.
            return fxn(self, module, *args, **kwargs)
        except:
            if not is_reload:
                del sys.modules[fullname]
            raise
        finally:
            module.__initializing__ = False
    _wrap(module_for_loader_wrapper, fxn)
    return module_for_loader_wrapper


def _check_name(method):
    """Decorator to verify that the module being requested matches the one the
    loader can handle.

    The first argument (self) must define _name which the second argument is
    compared against. If the comparison fails then ImportError is raised.

    """
    def _check_name_wrapper(self, name=None, *args, **kwargs):
        if name is None:
            name = self.name
        elif self.name != name:
            raise ImportError("loader cannot handle %s" % name, name=name)
        return method(self, name, *args, **kwargs)
    _wrap(_check_name_wrapper, method)
    return _check_name_wrapper


def _requires_builtin(fxn):
    """Decorator to verify the named module is built-in."""
    def _requires_builtin_wrapper(self, fullname):
        if fullname not in sys.builtin_module_names:
            raise ImportError("{} is not a built-in module".format(fullname),
                              name=fullname)
        return fxn(self, fullname)
    _wrap(_requires_builtin_wrapper, fxn)
    return _requires_builtin_wrapper


def _requires_frozen(fxn):
    """Decorator to verify the named module is frozen."""
    def _requires_frozen_wrapper(self, fullname):
        if not _imp.is_frozen(fullname):
            raise ImportError("{} is not a frozen module".format(fullname),
                              name=fullname)
        return fxn(self, fullname)
    _wrap(_requires_frozen_wrapper, fxn)
    return _requires_frozen_wrapper


# Loaders #####################################################################

class BuiltinImporter:

    """Meta path import for built-in modules.

    All methods are either class or static methods to avoid the need to
    instantiate the class.

    """

    @classmethod
    def module_repr(cls, module):
        return "<module '{}' (built-in)>".format(module.__name__)

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find the built-in module.

        If 'path' is ever specified then the search is considered a failure.

        """
        if path is not None:
            return None
        return cls if _imp.is_builtin(fullname) else None

    @classmethod
    @set_package
    @set_loader
    @_requires_builtin
    def load_module(cls, fullname):
        """Load a built-in module."""
        is_reload = fullname in sys.modules
        try:
            return cls._exec_module(fullname)
        except:
            if not is_reload and fullname in sys.modules:
                del sys.modules[fullname]
            raise

    @classmethod
    def _exec_module(cls, fullname):
        """Helper for load_module, allowing to isolate easily (when
        looking at a traceback) whether an error comes from executing
        an imported module's code."""
        return _imp.init_builtin(fullname)

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
        """Return False as built-in modules are never packages."""
        return False


class FrozenImporter:

    """Meta path import for frozen modules.

    All methods are either class or static methods to avoid the need to
    instantiate the class.

    """

    @classmethod
    def module_repr(cls, m):
        return "<module '{}' (frozen)>".format(m.__name__)

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find a frozen module."""
        return cls if _imp.is_frozen(fullname) else None

    @classmethod
    @set_package
    @set_loader
    @_requires_frozen
    def load_module(cls, fullname):
        """Load a frozen module."""
        is_reload = fullname in sys.modules
        try:
            m = cls._exec_module(fullname)
            # Let our own module_repr() method produce a suitable repr.
            del m.__file__
            return m
        except:
            if not is_reload and fullname in sys.modules:
                del sys.modules[fullname]
            raise

    @classmethod
    @_requires_frozen
    def get_code(cls, fullname):
        """Return the code object for the frozen module."""
        return _imp.get_frozen_object(fullname)

    @classmethod
    @_requires_frozen
    def get_source(cls, fullname):
        """Return None as frozen modules do not have source code."""
        return None

    @classmethod
    @_requires_frozen
    def is_package(cls, fullname):
        """Return if the frozen module is a package."""
        return _imp.is_frozen_package(fullname)

    @classmethod
    def _exec_module(cls, fullname):
        """Helper for load_module, allowing to isolate easily (when
        looking at a traceback) whether an error comes from executing
        an imported module's code."""
        return _imp.init_frozen(fullname)


class _LoaderBasics:

    """Base class of common code needed by both SourceLoader and
    SourcelessFileLoader."""

    def is_package(self, fullname):
        """Concrete implementation of InspectLoader.is_package by checking if
        the path returned by get_filename has a filename of '__init__.py'."""
        filename = _path_split(self.get_filename(fullname))[1]
        filename_base = filename.rsplit('.', 1)[0]
        tail_name = fullname.rpartition('.')[2]
        return filename_base == '__init__' and tail_name != '__init__'

    def _bytes_from_bytecode(self, fullname, data, bytecode_path, source_stats):
        """Return the marshalled bytes from bytecode, verifying the magic
        number, timestamp and source size along the way.

        If source_stats is None then skip the timestamp check.

        """
        magic = data[:4]
        raw_timestamp = data[4:8]
        raw_size = data[8:12]
        if magic != _MAGIC_NUMBER:
            msg = 'bad magic number in {!r}: {!r}'.format(fullname, magic)
            raise ImportError(msg, name=fullname, path=bytecode_path)
        elif len(raw_timestamp) != 4:
            message = 'bad timestamp in {}'.format(fullname)
            _verbose_message(message)
            raise EOFError(message)
        elif len(raw_size) != 4:
            message = 'bad size in {}'.format(fullname)
            _verbose_message(message)
            raise EOFError(message)
        if source_stats is not None:
            try:
                source_mtime = int(source_stats['mtime'])
            except KeyError:
                pass
            else:
                if _r_long(raw_timestamp) != source_mtime:
                    message = 'bytecode is stale for {}'.format(fullname)
                    _verbose_message(message)
                    raise ImportError(message, name=fullname,
                                      path=bytecode_path)
            try:
                source_size = source_stats['size'] & 0xFFFFFFFF
            except KeyError:
                pass
            else:
                if _r_long(raw_size) != source_size:
                    raise ImportError(
                        "bytecode is stale for {}".format(fullname),
                        name=fullname, path=bytecode_path)
        # Can't return the code object as errors from marshal loading need to
        # propagate even when source is available.
        return data[12:]

    @module_for_loader
    def _load_module(self, module, *, sourceless=False):
        """Helper for load_module able to handle either source or sourceless
        loading."""
        name = module.__name__
        code_object = self.get_code(name)
        module.__file__ = self.get_filename(name)
        if not sourceless:
            try:
                module.__cached__ = cache_from_source(module.__file__)
            except NotImplementedError:
                module.__cached__ = module.__file__
        else:
            module.__cached__ = module.__file__
        module.__package__ = name
        if self.is_package(name):
            module.__path__ = [_path_split(module.__file__)[0]]
        else:
            module.__package__ = module.__package__.rpartition('.')[0]
        module.__loader__ = self
        self._exec_module(code_object, module.__dict__)
        return module

    def _exec_module(self, code_object, module_dict):
        """Helper for _load_module, allowing to isolate easily (when
        looking at a traceback) whether an error comes from executing
        an imported module's code."""
        exec(code_object, module_dict)


class SourceLoader(_LoaderBasics):

    def path_mtime(self, path):
        """Optional method that returns the modification time (an int) for the
        specified path, where path is a str.
        """
        raise NotImplementedError

    def path_stats(self, path):
        """Optional method returning a metadata dict for the specified path
        to by the path (str).
        Possible keys:
        - 'mtime' (mandatory) is the numeric timestamp of last source
          code modification;
        - 'size' (optional) is the size in bytes of the source code.

        Implementing this method allows the loader to read bytecode files.
        """
        return {'mtime': self.path_mtime(path)}

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
            raise ImportError("source not available through get_data()",
                              name=fullname)
        encoding = tokenize.detect_encoding(_io.BytesIO(source_bytes).readline)
        newline_decoder = _io.IncrementalNewlineDecoder(None, True)
        return newline_decoder.decode(source_bytes.decode(encoding[0]))

    def get_code(self, fullname):
        """Concrete implementation of InspectLoader.get_code.

        Reading of bytecode requires path_stats to be implemented. To write
        bytecode, set_data must also be implemented.

        """
        source_path = self.get_filename(fullname)
        source_mtime = None
        try:
            bytecode_path = cache_from_source(source_path)
        except NotImplementedError:
            bytecode_path = None
        else:
            try:
                st = self.path_stats(source_path)
            except NotImplementedError:
                pass
            else:
                source_mtime = int(st['mtime'])
                try:
                    data = self.get_data(bytecode_path)
                except IOError:
                    pass
                else:
                    try:
                        bytes_data = self._bytes_from_bytecode(fullname, data,
                                                               bytecode_path,
                                                               st)
                    except (ImportError, EOFError):
                        pass
                    else:
                        _verbose_message('{} matches {}', bytecode_path,
                                        source_path)
                        found = marshal.loads(bytes_data)
                        if isinstance(found, _code_type):
                            _imp._fix_co_filename(found, source_path)
                            _verbose_message('code object from {}',
                                            bytecode_path)
                            return found
                        else:
                            msg = "Non-code object in {}"
                            raise ImportError(msg.format(bytecode_path),
                                              name=fullname, path=bytecode_path)
        source_bytes = self.get_data(source_path)
        code_object = compile(source_bytes, source_path, 'exec',
                                dont_inherit=True)
        _verbose_message('code object from {}', source_path)
        if (not sys.dont_write_bytecode and bytecode_path is not None and
            source_mtime is not None):
            data = bytearray(_MAGIC_NUMBER)
            data.extend(_w_long(source_mtime))
            data.extend(_w_long(len(source_bytes)))
            data.extend(marshal.dumps(code_object))
            try:
                self.set_data(bytecode_path, data)
                _verbose_message('wrote {!r}', bytecode_path)
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


class FileLoader:

    """Base file loader class which implements the loader protocol methods that
    require file system usage."""

    def __init__(self, fullname, path):
        """Cache the module name and the path to the file found by the
        finder."""
        self.name = fullname
        self.path = path

    @_check_name
    def load_module(self, fullname):
        """Load a module from a file."""
        # Issue #14857: Avoid the zero-argument form so the implementation
        # of that form can be updated without breaking the frozen module
        return super(FileLoader, self).load_module(fullname)

    @_check_name
    def get_filename(self, fullname):
        """Return the path to the source file as found by the finder."""
        return self.path

    def get_data(self, path):
        """Return the data from path as raw bytes."""
        with _io.FileIO(path, 'r') as file:
            return file.read()


class SourceFileLoader(FileLoader, SourceLoader):

    """Concrete implementation of SourceLoader using the file system."""

    def path_stats(self, path):
        """Return the metadat for the path."""
        st = _os.stat(path)
        return {'mtime': st.st_mtime, 'size': st.st_size}

    def set_data(self, path, data):
        """Write bytes data to a file."""
        parent, filename = _path_split(path)
        path_parts = []
        # Figure out what directories are missing.
        while parent and not _path_isdir(parent):
            parent, part = _path_split(parent)
            path_parts.append(part)
        # Create needed directories.
        for part in reversed(path_parts):
            parent = _path_join(parent, part)
            try:
                _os.mkdir(parent)
            except FileExistsError:
                # Probably another Python process already created the dir.
                continue
            except PermissionError:
                # If can't get proper access, then just forget about writing
                # the data.
                return
        try:
            _write_atomic(path, data)
            _verbose_message('created {!r}', path)
        except (PermissionError, FileExistsError):
            # Don't worry if you can't write bytecode or someone is writing
            # it at the same time.
            pass


class SourcelessFileLoader(FileLoader, _LoaderBasics):

    """Loader which handles sourceless file imports."""

    def load_module(self, fullname):
        return self._load_module(fullname, sourceless=True)

    def get_code(self, fullname):
        path = self.get_filename(fullname)
        data = self.get_data(path)
        bytes_data = self._bytes_from_bytecode(fullname, data, path, None)
        found = marshal.loads(bytes_data)
        if isinstance(found, _code_type):
            _verbose_message('code object from {!r}', path)
            return found
        else:
            raise ImportError("Non-code object in {}".format(path),
                              name=fullname, path=path)

    def get_source(self, fullname):
        """Return None as there is no source code."""
        return None


class ExtensionFileLoader:

    """Loader for extension modules.

    The constructor is designed to work with FileFinder.

    """

    def __init__(self, name, path):
        self.name = name
        self.path = path

    @_check_name
    @set_package
    @set_loader
    def load_module(self, fullname):
        """Load an extension module."""
        is_reload = fullname in sys.modules
        try:
            module = self._exec_module(fullname, self.path)
            _verbose_message('extension module loaded from {!r}', self.path)
            return module
        except:
            if not is_reload and fullname in sys.modules:
                del sys.modules[fullname]
            raise

    def is_package(self, fullname):
        """Return False as an extension module can never be a package."""
        return False

    def get_code(self, fullname):
        """Return None as an extension module cannot create a code object."""
        return None

    def get_source(self, fullname):
        """Return None as extension modules have no source code."""
        return None

    def _exec_module(self, fullname, path):
        """Helper for load_module, allowing to isolate easily (when
        looking at a traceback) whether an error comes from executing
        an imported module's code."""
        return _imp.load_dynamic(fullname, path)


class _NamespacePath:
    """Represents a namespace package's path.  It uses the module name
    to find its parent module, and from there it looks up the parent's
    __path__.  When this changes, the module's own path is recomputed,
    using path_finder.  For top-leve modules, the parent module's path
    is sys.path."""

    def __init__(self, name, path, path_finder):
        self._name = name
        self._path = path
        self._last_parent_path = tuple(self._get_parent_path())
        self._path_finder = path_finder

    def _find_parent_path_names(self):
        """Returns a tuple of (parent-module-name, parent-path-attr-name)"""
        parent, dot, me = self._name.rpartition('.')
        if dot == '':
            # This is a top-level module. sys.path contains the parent path.
            return 'sys', 'path'
        # Not a top-level module. parent-module.__path__ contains the
        #  parent path.
        return parent, '__path__'

    def _get_parent_path(self):
        parent_module_name, path_attr_name = self._find_parent_path_names()
        return getattr(sys.modules[parent_module_name], path_attr_name)

    def _recalculate(self):
        # If the parent's path has changed, recalculate _path
        parent_path = tuple(self._get_parent_path()) # Make a copy
        if parent_path != self._last_parent_path:
            loader, new_path = self._path_finder(self._name, parent_path)
            # Note that no changes are made if a loader is returned, but we
            #  do remember the new parent path
            if loader is None:
                self._path = new_path
            self._last_parent_path = parent_path     # Save the copy
        return self._path

    def __iter__(self):
        return iter(self._recalculate())

    def __len__(self):
        return len(self._recalculate())

    def __repr__(self):
        return "_NamespacePath({!r})".format(self._path)

    def __contains__(self, item):
        return item in self._recalculate()

    def append(self, item):
        self._path.append(item)


class NamespaceLoader:
    def __init__(self, name, path, path_finder):
        self._path = _NamespacePath(name, path, path_finder)

    @classmethod
    def module_repr(cls, module):
        return "<module '{}' (namespace)>".format(module.__name__)

    @module_for_loader
    def load_module(self, module):
        """Load a namespace module."""
        _verbose_message('namespace module loaded with path {!r}', self._path)
        module.__path__ = self._path
        return module


# Finders #####################################################################

class PathFinder:

    """Meta path finder for sys.(path|path_hooks|path_importer_cache)."""

    @classmethod
    def _path_hooks(cls, path):
        """Search sequence of hooks for a finder for 'path'.

        If 'hooks' is false then use sys.path_hooks.

        """
        if not sys.path_hooks:
            _warnings.warn('sys.path_hooks is empty', ImportWarning)
        for hook in sys.path_hooks:
            try:
                return hook(path)
            except ImportError:
                continue
        else:
            return None

    @classmethod
    def _path_importer_cache(cls, path):
        """Get the finder for the path from sys.path_importer_cache.

        If the path is not in the cache, find the appropriate finder and cache
        it. If no finder is available, store None.

        """
        if path == '':
            path = '.'
        try:
            finder = sys.path_importer_cache[path]
        except KeyError:
            finder = cls._path_hooks(path)
            sys.path_importer_cache[path] = finder
        return finder

    @classmethod
    def _get_loader(cls, fullname, path):
        """Find the loader or namespace_path for this module/package name."""
        # If this ends up being a namespace package, namespace_path is
        #  the list of paths that will become its __path__
        namespace_path = []
        for entry in path:
            finder = cls._path_importer_cache(entry)
            if finder is not None:
                if hasattr(finder, 'find_loader'):
                    loader, portions = finder.find_loader(fullname)
                else:
                    loader = finder.find_module(fullname)
                    portions = []
                if loader is not None:
                    # We found a loader: return it immediately.
                    return (loader, namespace_path)
                # This is possibly part of a namespace package.
                #  Remember these path entries (if any) for when we
                #  create a namespace package, and continue iterating
                #  on path.
                namespace_path.extend(portions)
        else:
            return (None, namespace_path)

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find the module on sys.path or 'path' based on sys.path_hooks and
        sys.path_importer_cache."""
        if path is None:
            path = sys.path
        loader, namespace_path = cls._get_loader(fullname, path)
        if loader is not None:
            return loader
        else:
            if namespace_path:
                # We found at least one namespace path.  Return a
                #  loader which can create the namespace package.
                return NamespaceLoader(fullname, namespace_path, cls._get_loader)
            else:
                return None


class FileFinder:

    """File-based finder.

    Interactions with the file system are cached for performance, being
    refreshed when the directory the finder is handling has been modified.

    """

    def __init__(self, path, *details):
        """Initialize with the path to search on and a variable number of
        3-tuples containing the loader, file suffixes the loader recognizes,
        and a boolean of whether the loader handles packages."""
        packages = []
        modules = []
        for loader, suffixes, supports_packages in details:
            modules.extend((suffix, loader) for suffix in suffixes)
            if supports_packages:
                packages.extend((suffix, loader) for suffix in suffixes)
        self.packages = packages
        self.modules = modules
        # Base (directory) path
        self.path = path or '.'
        self._path_mtime = -1
        self._path_cache = set()
        self._relaxed_path_cache = set()

    def invalidate_caches(self):
        """Invalidate the directory mtime."""
        self._path_mtime = -1

    def find_module(self, fullname):
        """Try to find a loader for the specified module."""
        # Call find_loader(). If it returns a string (indicating this
        # is a namespace package portion), generate a warning and
        # return None.
        loader, portions = self.find_loader(fullname)
        assert len(portions) in [0, 1]
        if loader is None and len(portions):
            msg = "Not importing directory {}: missing __init__"
            _warnings.warn(msg.format(portions[0]), ImportWarning)
        return loader

    def find_loader(self, fullname):
        """Try to find a loader for the specified module, or the namespace
        package portions. Returns (loader, list-of-portions)."""
        is_namespace = False
        tail_module = fullname.rpartition('.')[2]
        try:
            mtime = _os.stat(self.path).st_mtime
        except OSError:
            mtime = -1
        if mtime != self._path_mtime:
            self._fill_cache()
            self._path_mtime = mtime
        # tail_module keeps the original casing, for __file__ and friends
        if _relax_case():
            cache = self._relaxed_path_cache
            cache_module = tail_module.lower()
        else:
            cache = self._path_cache
            cache_module = tail_module
        # Check if the module is the name of a directory (and thus a package).
        if cache_module in cache:
            base_path = _path_join(self.path, tail_module)
            if _path_isdir(base_path):
                for suffix, loader in self.packages:
                    init_filename = '__init__' + suffix
                    full_path = _path_join(base_path, init_filename)
                    if _path_isfile(full_path):
                        return (loader(fullname, full_path), [base_path])
                else:
                    # A namespace package, return the path if we don't also
                    #  find a module in the next section.
                    is_namespace = True
        # Check for a file w/ a proper suffix exists.
        for suffix, loader in self.modules:
            if cache_module + suffix in cache:
                full_path = _path_join(self.path, tail_module + suffix)
                if _path_isfile(full_path):
                    return (loader(fullname, full_path), [])
        if is_namespace:
            return (None, [base_path])
        return (None, [])

    def _fill_cache(self):
        """Fill the cache of potential modules and packages for this directory."""
        path = self.path
        contents = _os.listdir(path)
        # We store two cached versions, to handle runtime changes of the
        # PYTHONCASEOK environment variable.
        if not sys.platform.startswith('win'):
            self._path_cache = set(contents)
        else:
            # Windows users can import modules with case-insensitive file
            # suffixes (for legacy reasons). Make the suffix lowercase here
            # so it's done once instead of for every import. This is safe as
            # the specified suffixes to check against are always specified in a
            # case-sensitive manner.
            lower_suffix_contents = set()
            for item in contents:
                name, dot, suffix = item.partition('.')
                if dot:
                    new_name = '{}.{}'.format(name, suffix.lower())
                else:
                    new_name = name
                lower_suffix_contents.add(new_name)
            self._path_cache = lower_suffix_contents
        if sys.platform.startswith(_CASE_INSENSITIVE_PLATFORMS):
            self._relaxed_path_cache = set(fn.lower() for fn in contents)

    @classmethod
    def path_hook(cls, *loader_details):
        """A class method which returns a closure to use on sys.path_hook
        which will return an instance using the specified loaders and the path
        called on the closure.

        If the path called on the closure is not a directory, ImportError is
        raised.

        """
        def path_hook_for_FileFinder(path):
            """Path hook for importlib.machinery.FileFinder."""
            if not _path_isdir(path):
                raise ImportError("only directories are supported", path=path)
            return cls(path, *loader_details)

        return path_hook_for_FileFinder

    def __repr__(self):
        return "FileFinder(%r)" % (self.path,)


# Import itself ###############################################################

class _ImportLockContext:

    """Context manager for the import lock."""

    def __enter__(self):
        """Acquire the import lock."""
        _imp.acquire_lock()

    def __exit__(self, exc_type, exc_value, exc_traceback):
        """Release the import lock regardless of any raised exceptions."""
        _imp.release_lock()


def _resolve_name(name, package, level):
    """Resolve a relative module name to an absolute one."""
    bits = package.rsplit('.', level - 1)
    if len(bits) < level:
        raise ValueError('attempted relative import beyond top-level package')
    base = bits[0]
    return '{}.{}'.format(base, name) if name else base


def _find_module(name, path):
    """Find a module's loader."""
    if not sys.meta_path:
        _warnings.warn('sys.meta_path is empty', ImportWarning)
    for finder in sys.meta_path:
        with _ImportLockContext():
            loader = finder.find_module(name, path)
        if loader is not None:
            # The parent import may have already imported this module.
            if name not in sys.modules:
                return loader
            else:
                return sys.modules[name].__loader__
    else:
        return None


def _sanity_check(name, package, level):
    """Verify arguments are "sane"."""
    if not isinstance(name, str):
        raise TypeError("module name must be str, not {}".format(type(name)))
    if level < 0:
        raise ValueError('level must be >= 0')
    if package:
        if not isinstance(package, str):
            raise TypeError("__package__ not set to a string")
        elif package not in sys.modules:
            msg = ("Parent module {!r} not loaded, cannot perform relative "
                   "import")
            raise SystemError(msg.format(package))
    if not name and level == 0:
        raise ValueError("Empty module name")


_ERR_MSG = 'No module named {!r}'

def _find_and_load_unlocked(name, import_):
    path = None
    parent = name.rpartition('.')[0]
    if parent:
        if parent not in sys.modules:
            import_(parent)
        # Crazy side-effects!
        if name in sys.modules:
            return sys.modules[name]
        # Backwards-compatibility; be nicer to skip the dict lookup.
        parent_module = sys.modules[parent]
        try:
            path = parent_module.__path__
        except AttributeError:
            msg = (_ERR_MSG + '; {} is not a package').format(name, parent)
            raise ImportError(msg, name=name)
    loader = _find_module(name, path)
    if loader is None:
        raise ImportError(_ERR_MSG.format(name), name=name)
    elif name not in sys.modules:
        # The parent import may have already imported this module.
        loader.load_module(name)
        _verbose_message('import {!r} # {!r}', name, loader)
    # Backwards-compatibility; be nicer to skip the dict lookup.
    module = sys.modules[name]
    if parent:
        # Set the module as an attribute on its parent.
        parent_module = sys.modules[parent]
        setattr(parent_module, name.rpartition('.')[2], module)
    # Set __package__ if the loader did not.
    if getattr(module, '__package__', None) is None:
        try:
            module.__package__ = module.__name__
            if not hasattr(module, '__path__'):
                module.__package__ = module.__package__.rpartition('.')[0]
        except AttributeError:
            pass
    # Set loader if need be.
    if not hasattr(module, '__loader__'):
        try:
            module.__loader__ = loader
        except AttributeError:
            pass
    return module


def _find_and_load(name, import_):
    """Find and load the module, and release the import lock."""
    try:
        lock = _get_module_lock(name)
    finally:
        _imp.release_lock()
    lock.acquire()
    try:
        return _find_and_load_unlocked(name, import_)
    finally:
        lock.release()


def _gcd_import(name, package=None, level=0):
    """Import and return the module based on its name, the package the call is
    being made from, and the level adjustment.

    This function represents the greatest common denominator of functionality
    between import_module and __import__. This includes setting __package__ if
    the loader did not.

    """
    _sanity_check(name, package, level)
    if level > 0:
        name = _resolve_name(name, package, level)
    _imp.acquire_lock()
    if name not in sys.modules:
        return _find_and_load(name, _gcd_import)
    module = sys.modules[name]
    if module is None:
        _imp.release_lock()
        message = ("import of {} halted; "
                    "None in sys.modules".format(name))
        raise ImportError(message, name=name)
    _lock_unlock_module(name)
    return module


def _handle_fromlist(module, fromlist, import_):
    """Figure out what __import__ should return.

    The import_ parameter is a callable which takes the name of module to
    import. It is required to decouple the function from assuming importlib's
    import implementation is desired.

    """
    # The hell that is fromlist ...
    # If a package was imported, try to import stuff from fromlist.
    if hasattr(module, '__path__'):
        if '*' in fromlist and hasattr(module, '__all__'):
            fromlist = list(fromlist)
            fromlist.remove('*')
            fromlist.extend(module.__all__)
        for x in fromlist:
            if not hasattr(module, x):
                try:
                    import_('{}.{}'.format(module.__name__, x))
                except ImportError:
                    pass
    return module


def _calc___package__(globals):
    """Calculate what __package__ should be.

    __package__ is not guaranteed to be defined or could be set to None
    to represent that its proper value is unknown.

    """
    package = globals.get('__package__')
    if package is None:
        package = globals['__name__']
        if '__path__' not in globals:
            package = package.rpartition('.')[0]
    return package


def __import__(name, globals={}, locals={}, fromlist=[], level=0):
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
        package = _calc___package__(globals)
        module = _gcd_import(name, package, level)
    if not fromlist:
        # Return up to the first dot in 'name'. This is complicated by the fact
        # that 'name' may be relative.
        if level == 0:
            return _gcd_import(name.partition('.')[0])
        elif not name:
            return module
        else:
            cut_off = len(name) - len(name.partition('.')[0])
            return sys.modules[module.__name__[:len(module.__name__)-cut_off]]
    else:
        return _handle_fromlist(module, fromlist, _gcd_import)


_MAGIC_NUMBER = None  # Set in _setup()


def _setup(sys_module, _imp_module):
    """Setup importlib by importing needed built-in modules and injecting them
    into the global namespace.

    As sys is needed for sys.modules access and _imp is needed to load built-in
    modules, those two modules must be explicitly passed in.

    """
    global _imp, sys
    _imp = _imp_module
    sys = sys_module

    for module in (_imp, sys):
        if not hasattr(module, '__loader__'):
            module.__loader__ = BuiltinImporter

    self_module = sys.modules[__name__]
    for builtin_name in ('_io', '_warnings', 'builtins', 'marshal'):
        if builtin_name not in sys.modules:
            builtin_module = BuiltinImporter.load_module(builtin_name)
        else:
            builtin_module = sys.modules[builtin_name]
        setattr(self_module, builtin_name, builtin_module)

    os_details = ('posix', ['/']), ('nt', ['\\', '/']), ('os2', ['\\', '/'])
    for builtin_os, path_separators in os_details:
        # Assumption made in _path_join()
        assert all(len(sep) == 1 for sep in path_separators)
        path_sep = path_separators[0]
        if builtin_os in sys.modules:
            os_module = sys.modules[builtin_os]
            break
        else:
            try:
                os_module = BuiltinImporter.load_module(builtin_os)
                # TODO: rip out os2 code after 3.3 is released as per PEP 11
                if builtin_os == 'os2' and 'EMX GCC' in sys.version:
                    path_sep = path_separators[1]
                break
            except ImportError:
                continue
    else:
        raise ImportError('importlib requires posix or nt')

    try:
        thread_module = BuiltinImporter.load_module('_thread')
    except ImportError:
        # Python was built without threads
        thread_module = None
    weakref_module = BuiltinImporter.load_module('_weakref')

    setattr(self_module, '_os', os_module)
    setattr(self_module, '_thread', thread_module)
    setattr(self_module, '_weakref', weakref_module)
    setattr(self_module, 'path_sep', path_sep)
    setattr(self_module, 'path_separators', set(path_separators))
    # Constants
    setattr(self_module, '_relax_case', _make_relax_case())
    setattr(self_module, '_MAGIC_NUMBER', _imp_module.get_magic())
    if builtin_os == 'nt':
        SOURCE_SUFFIXES.append('.pyw')


def _install(sys_module, _imp_module):
    """Install importlib as the implementation of import."""
    _setup(sys_module, _imp_module)
    extensions = ExtensionFileLoader, _imp_module.extension_suffixes(), False
    source = SourceFileLoader, SOURCE_SUFFIXES, True
    bytecode = SourcelessFileLoader, BYTECODE_SUFFIXES, True
    supported_loaders = [extensions, source, bytecode]
    sys.path_hooks.extend([FileFinder.path_hook(*supported_loaders)])
    sys.meta_path.extend([BuiltinImporter, FrozenImporter, PathFinder])
