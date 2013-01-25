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


def _write_atomic(path, data, mode=0o666):
    """Best-effort function to write data to a path atomically.
    Be prepared to handle a FileExistsError if concurrent writing of the
    temporary file is attempted."""
    # id() is used to generate a pseudo-random filename.
    path_tmp = '{}.{}'.format(path, id(path))
    fd = _os.open(path_tmp,
                  _os.O_EXCL | _os.O_CREAT | _os.O_WRONLY, mode & 0o666)
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
        return "_ModuleLock({!r}) at {}".format(self.name, id(self))


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
        return "_DummyModuleLock({!r}) at {}".format(self.name, id(self))


# The following two functions are for consumption by Python/import.c.

def _get_module_lock(name):
    """Get or create the module lock for a given module name.

    Should only be called with the import lock taken."""
    lock = None
    try:
        lock = _module_locks[name]()
    except KeyError:
        pass
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

# Frame stripping magic ###############################################

def _call_with_frames_removed(f, *args, **kwds):
    """remove_importlib_frames in import.c will always remove sequences
    of importlib frames that end with a call to this function

    Use it instead of a normal call in places where including the importlib
    frames introduces unwanted noise into the traceback (e.g. when executing
    module code)
    """
    return f(*args, **kwds)


# Finder/loader utility code ###############################################

"""Magic word to reject .pyc files generated by other Python versions.
It should change for each incompatible change to the bytecode.

The value of CR and LF is incorporated so if you ever read or write
a .pyc file in text mode the magic number will be wrong; also, the
Apple MPW compiler swaps their values, botching string constants.

The magic numbers must be spaced apart at least 2 values, as the
-U interpeter flag will cause MAGIC+1 being used. They have been
odd numbers for some time now.

There were a variety of old schemes for setting the magic number.
The current working scheme is to increment the previous value by
10.

Starting with the adoption of PEP 3147 in Python 3.2, every bump in magic
number also includes a new "magic tag", i.e. a human readable string used
to represent the magic number in __pycache__ directories.  When you change
the magic number, you must also set a new unique magic tag.  Generally this
can be named after the Python major version of the magic number bump, but
it can really be anything, as long as it's different than anything else
that's come before.  The tags are included in the following table, starting
with Python 3.2a0.

Known values:
 Python 1.5:   20121
 Python 1.5.1: 20121
    Python 1.5.2: 20121
    Python 1.6:   50428
    Python 2.0:   50823
    Python 2.0.1: 50823
    Python 2.1:   60202
    Python 2.1.1: 60202
    Python 2.1.2: 60202
    Python 2.2:   60717
    Python 2.3a0: 62011
    Python 2.3a0: 62021
    Python 2.3a0: 62011 (!)
    Python 2.4a0: 62041
    Python 2.4a3: 62051
    Python 2.4b1: 62061
    Python 2.5a0: 62071
    Python 2.5a0: 62081 (ast-branch)
    Python 2.5a0: 62091 (with)
    Python 2.5a0: 62092 (changed WITH_CLEANUP opcode)
    Python 2.5b3: 62101 (fix wrong code: for x, in ...)
    Python 2.5b3: 62111 (fix wrong code: x += yield)
    Python 2.5c1: 62121 (fix wrong lnotab with for loops and
                         storing constants that should have been removed)
    Python 2.5c2: 62131 (fix wrong code: for x, in ... in listcomp/genexp)
    Python 2.6a0: 62151 (peephole optimizations and STORE_MAP opcode)
    Python 2.6a1: 62161 (WITH_CLEANUP optimization)
    Python 3000:   3000
                   3010 (removed UNARY_CONVERT)
                   3020 (added BUILD_SET)
                   3030 (added keyword-only parameters)
                   3040 (added signature annotations)
                   3050 (print becomes a function)
                   3060 (PEP 3115 metaclass syntax)
                   3061 (string literals become unicode)
                   3071 (PEP 3109 raise changes)
                   3081 (PEP 3137 make __file__ and __name__ unicode)
                   3091 (kill str8 interning)
                   3101 (merge from 2.6a0, see 62151)
                   3103 (__file__ points to source file)
    Python 3.0a4: 3111 (WITH_CLEANUP optimization).
    Python 3.0a5: 3131 (lexical exception stacking, including POP_EXCEPT)
    Python 3.1a0: 3141 (optimize list, set and dict comprehensions:
            change LIST_APPEND and SET_ADD, add MAP_ADD)
    Python 3.1a0: 3151 (optimize conditional branches:
            introduce POP_JUMP_IF_FALSE and POP_JUMP_IF_TRUE)
    Python 3.2a0: 3160 (add SETUP_WITH)
                  tag: cpython-32
    Python 3.2a1: 3170 (add DUP_TOP_TWO, remove DUP_TOPX and ROT_FOUR)
                  tag: cpython-32
    Python 3.2a2  3180 (add DELETE_DEREF)
    Python 3.3a0  3190 __class__ super closure changed
    Python 3.3a0  3200 (__qualname__ added)
                     3210 (added size modulo 2**32 to the pyc header)
    Python 3.3a1  3220 (changed PEP 380 implementation)
    Python 3.3a4  3230 (revert changes to implicit __class__ closure)

MAGIC must change whenever the bytecode emitted by the compiler may no
longer be understood by older implementations of the eval loop (usually
due to the addition of new opcodes).

"""
_RAW_MAGIC_NUMBER = 3230 | ord('\r') << 16 | ord('\n') << 24
_MAGIC_BYTES = bytes(_RAW_MAGIC_NUMBER >> n & 0xff for n in range(0, 25, 8))

_PYCACHE = '__pycache__'

SOURCE_SUFFIXES = ['.py']  # _setup() adds .pyw as needed.

DEBUG_BYTECODE_SUFFIXES = ['.pyc']
OPTIMIZED_BYTECODE_SUFFIXES = ['.pyo']

def cache_from_source(path, debug_override=None):
    """Given the path to a .py file, return the path to its .pyc/.pyo file.

    The .py file does not need to exist; this simply returns the path to the
    .pyc/.pyo file calculated as if the .py file were imported.  The extension
    will be .pyc unless sys.flags.optimize is non-zero, then it will be .pyo.

    If debug_override is not None, then it must be a boolean and is used in
    place of sys.flags.optimize.

    If sys.implementation.cache_tag is None then NotImplementedError is raised.

    """
    debug = not sys.flags.optimize if debug_override is None else debug_override
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


def source_from_cache(path):
    """Given the path to a .pyc./.pyo file, return the path to its .py file.

    The .pyc/.pyo file does not need to exist; this simply returns the path to
    the .py file calculated to correspond to the .pyc/.pyo file.  If path does
    not conform to PEP 3147 format, ValueError will be raised. If
    sys.implementation.cache_tag is None then NotImplementedError is raised.

    """
    if sys.implementation.cache_tag is None:
        raise NotImplementedError('sys.implementation.cache_tag is None')
    head, pycache_filename = _path_split(path)
    head, pycache = _path_split(head)
    if pycache != _PYCACHE:
        raise ValueError('{} not bottom-level directory in '
                         '{!r}'.format(_PYCACHE, path))
    if pycache_filename.count('.') != 2:
        raise ValueError('expected only 2 dots in '
                         '{!r}'.format(pycache_filename))
    base_filename = pycache_filename.partition('.')[0]
    return _path_join(head, base_filename + SOURCE_SUFFIXES[0])


def _get_sourcefile(bytecode_path):
    """Convert a bytecode file path to a source path (if possible).

    This function exists purely for backwards-compatibility for
    PyImport_ExecCodeModuleWithFilenames() in the C API.

    """
    if len(bytecode_path) == 0:
        return None
    rest, _, extension = bytecode_path.rparition('.')
    if not rest or extension.lower()[-3:-1] != '.py':
        return bytecode_path

    try:
        source_path = source_from_cache(bytecode_path)
    except (NotImplementedError, ValueError):
        source_path = bytcode_path[-1:]

    return source_path if _path_isfile(source_stats) else bytecode_path


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
            # This must be done before putting the module in sys.modules
            # (otherwise an optimization shortcut in import.c becomes wrong)
            module.__initializing__ = True
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
        else:
            module.__initializing__ = True
        try:
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


def _find_module_shim(self, fullname):
    """Try to find a loader for the specified module by delegating to
    self.find_loader()."""
    # Call find_loader(). If it returns a string (indicating this
    # is a namespace package portion), generate a warning and
    # return None.
    loader, portions = self.find_loader(fullname)
    if loader is None and len(portions):
        msg = "Not importing directory {}: missing __init__"
        _warnings.warn(msg.format(portions[0]), ImportWarning)
    return loader


def _validate_bytecode_header(data, source_stats=None, name=None, path=None):
    """Validate the header of the passed-in bytecode against source_stats (if
    given) and returning the bytecode that can be compiled by compile().

    All other arguments are used to enhance error reporting.

    ImportError is raised when the magic number is incorrect or the bytecode is
    found to be stale. EOFError is raised when the data is found to be
    truncated.

    """
    exc_details = {}
    if name is not None:
        exc_details['name'] = name
    else:
        # To prevent having to make all messages have a conditional name.
        name = '<bytecode>'
    if path is not None:
        exc_details['path'] = path
    magic = data[:4]
    raw_timestamp = data[4:8]
    raw_size = data[8:12]
    if magic != _MAGIC_BYTES:
        msg = 'incomplete magic number in {!r}: {!r}'.format(name, magic)
        raise ImportError(msg, **exc_details)
    elif len(raw_timestamp) != 4:
        message = 'incomplete timestamp in {!r}'.format(name)
        _verbose_message(message)
        raise EOFError(message)
    elif len(raw_size) != 4:
        message = 'incomplete size in {!r}'.format(name)
        _verbose_message(message)
        raise EOFError(message)
    if source_stats is not None:
        try:
            source_mtime = int(source_stats['mtime'])
        except KeyError:
            pass
        else:
            if _r_long(raw_timestamp) != source_mtime:
                message = 'bytecode is stale for {!r}'.format(name)
                _verbose_message(message)
                raise ImportError(message, **exc_details)
        try:
            source_size = source_stats['size'] & 0xFFFFFFFF
        except KeyError:
            pass
        else:
            if _r_long(raw_size) != source_size:
                raise ImportError("bytecode is stale for {!r}".format(name),
                                  **exc_details)
    return data[12:]


def _compile_bytecode(data, name=None, bytecode_path=None, source_path=None):
    """Compile bytecode as returned by _validate_bytecode_header()."""
    code = marshal.loads(data)
    if isinstance(code, _code_type):
        _verbose_message('code object from {!r}', bytecode_path)
        if source_path is not None:
            _imp._fix_co_filename(code, source_path)
        return code
    else:
        raise ImportError("Non-code object in {!r}".format(bytecode_path),
                          name=name, path=bytecode_path)


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
            return _call_with_frames_removed(_imp.init_builtin, fullname)
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
            m = _call_with_frames_removed(_imp.init_frozen, fullname)
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
        """Return True if the frozen module is a package."""
        return _imp.is_frozen_package(fullname)


class WindowsRegistryFinder:

    """Meta path finder for modules declared in the Windows registry.
    """

    REGISTRY_KEY = (
        "Software\\Python\\PythonCore\\{sys_version}"
        "\\Modules\\{fullname}")
    REGISTRY_KEY_DEBUG = (
        "Software\\Python\\PythonCore\\{sys_version}"
        "\\Modules\\{fullname}\\Debug")
    DEBUG_BUILD = False  # Changed in _setup()

    @classmethod
    def _open_registry(cls, key):
        try:
            return _winreg.OpenKey(_winreg.HKEY_CURRENT_USER, key)
        except OSError:
            return _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, key)

    @classmethod
    def _search_registry(cls, fullname):
        if cls.DEBUG_BUILD:
            registry_key = cls.REGISTRY_KEY_DEBUG
        else:
            registry_key = cls.REGISTRY_KEY
        key = registry_key.format(fullname=fullname,
                                  sys_version=sys.version[:3])
        try:
            with cls._open_registry(key) as hkey:
                filepath = _winreg.QueryValue(hkey, "")
        except OSError:
            return None
        return filepath

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find module named in the registry."""
        filepath = cls._search_registry(fullname)
        if filepath is None:
            return None
        try:
            _os.stat(filepath)
        except OSError:
            return None
        for loader, suffixes, _ in _get_supported_file_loaders():
            if filepath.endswith(tuple(suffixes)):
                return loader(fullname, filepath)


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
        _call_with_frames_removed(exec, code_object, module.__dict__)
        return module


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

    def _cache_bytecode(self, source_path, cache_path, data):
        """Optional method which writes data (bytes) to a file path (a str).

        Implementing this method allows for the writing of bytecode files.

        The source path is needed in order to correctly transfer permissions
        """
        # For backwards compatibility, we delegate to set_data()
        return self.set_data(cache_path, data)

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
        except OSError as exc:
            raise ImportError("source not available through get_data()",
                              name=fullname) from exc
        readsource = _io.BytesIO(source_bytes).readline
        try:
            encoding = tokenize.detect_encoding(readsource)
        except SyntaxError as exc:
            raise ImportError("Failed to detect encoding",
                              name=fullname) from exc
        newline_decoder = _io.IncrementalNewlineDecoder(None, True)
        try:
            return newline_decoder.decode(source_bytes.decode(encoding[0]))
        except UnicodeDecodeError as exc:
            raise ImportError("Failed to decode source file",
                              name=fullname) from exc

    def source_to_code(self, data, path):
        """Return the code object compiled from source.

        The 'data' argument can be any object type that compile() supports.
        """
        return _call_with_frames_removed(compile, data, path, 'exec',
                                        dont_inherit=True)

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
                except OSError:
                    pass
                else:
                    try:
                        bytes_data = _validate_bytecode_header(data,
                                source_stats=st, name=fullname,
                                path=bytecode_path)
                    except (ImportError, EOFError):
                        pass
                    else:
                        _verbose_message('{} matches {}', bytecode_path,
                                        source_path)
                        return _compile_bytecode(bytes_data, name=fullname,
                                                 bytecode_path=bytecode_path,
                                                 source_path=source_path)
        source_bytes = self.get_data(source_path)
        code_object = self.source_to_code(source_bytes, source_path)
        _verbose_message('code object from {}', source_path)
        if (not sys.dont_write_bytecode and bytecode_path is not None and
            source_mtime is not None):
            data = bytearray(_MAGIC_BYTES)
            data.extend(_w_long(source_mtime))
            data.extend(_w_long(len(source_bytes)))
            data.extend(marshal.dumps(code_object))
            try:
                self._cache_bytecode(source_path, bytecode_path, data)
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
        """Return the metadata for the path."""
        st = _os.stat(path)
        return {'mtime': st.st_mtime, 'size': st.st_size}

    def _cache_bytecode(self, source_path, bytecode_path, data):
        # Adapt between the two APIs
        try:
            mode = _os.stat(source_path).st_mode
        except OSError:
            mode = 0o666
        # We always ensure write access so we can update cached files
        # later even when the source files are read-only on Windows (#6074)
        mode |= 0o200
        return self.set_data(bytecode_path, data, _mode=mode)

    def set_data(self, path, data, *, _mode=0o666):
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
            except OSError as exc:
                # Could be a permission error, read-only filesystem: just forget
                # about writing the data.
                _verbose_message('could not create {!r}: {!r}', parent, exc)
                return
        try:
            _write_atomic(path, data, _mode)
            _verbose_message('created {!r}', path)
        except OSError as exc:
            # Same as above: just don't write the bytecode.
            _verbose_message('could not create {!r}: {!r}', path, exc)


class SourcelessFileLoader(FileLoader, _LoaderBasics):

    """Loader which handles sourceless file imports."""

    def load_module(self, fullname):
        return self._load_module(fullname, sourceless=True)

    def get_code(self, fullname):
        path = self.get_filename(fullname)
        data = self.get_data(path)
        bytes_data = _validate_bytecode_header(data, name=fullname, path=path)
        return _compile_bytecode(bytes_data, name=fullname, bytecode_path=path)

    def get_source(self, fullname):
        """Return None as there is no source code."""
        return None


# Filled in by _setup().
EXTENSION_SUFFIXES = []


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
            module = _call_with_frames_removed(_imp.load_dynamic,
                                               fullname, self.path)
            _verbose_message('extension module loaded from {!r}', self.path)
            if self.is_package(fullname) and not hasattr(module, '__path__'):
                module.__path__ = [_path_split(self.path)[0]]
            return module
        except:
            if not is_reload and fullname in sys.modules:
                del sys.modules[fullname]
            raise

    def is_package(self, fullname):
        """Return True if the extension module is a package."""
        file_name = _path_split(self.path)[1]
        return any(file_name == '__init__' + suffix
                   for suffix in EXTENSION_SUFFIXES)

    def get_code(self, fullname):
        """Return None as an extension module cannot create a code object."""
        return None

    def get_source(self, fullname):
        """Return None as extension modules have no source code."""
        return None


class _NamespacePath:
    """Represents a namespace package's path.  It uses the module name
    to find its parent module, and from there it looks up the parent's
    __path__.  When this changes, the module's own path is recomputed,
    using path_finder.  For top-level modules, the parent module's path
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

    """Meta path finder for sys.path and package __path__ attributes."""

    @classmethod
    def invalidate_caches(cls):
        """Call the invalidate_caches() method on all path entry finders
        stored in sys.path_importer_caches (where implemented)."""
        for finder in sys.path_importer_cache.values():
            if hasattr(finder, 'invalidate_caches'):
                finder.invalidate_caches()

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
        """Get the finder for the path entry from sys.path_importer_cache.

        If the path entry is not in the cache, find the appropriate finder
        and cache it. If no finder is available, store None.

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
            if not isinstance(entry, (str, bytes)):
                continue
            finder = cls._path_importer_cache(entry)
            if finder is not None:
                if hasattr(finder, 'find_loader'):
                    loader, portions = finder.find_loader(fullname)
                else:
                    loader = finder.find_module(fullname)
                    portions = []
                if loader is not None:
                    # We found a loader: return it immediately.
                    return loader, namespace_path
                # This is possibly part of a namespace package.
                #  Remember these path entries (if any) for when we
                #  create a namespace package, and continue iterating
                #  on path.
                namespace_path.extend(portions)
        else:
            return None, namespace_path

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
        loaders = []
        for loader, suffixes in details:
            loaders.extend((suffix, loader) for suffix in suffixes)
        self._loaders = loaders
        # Base (directory) path
        self.path = path or '.'
        self._path_mtime = -1
        self._path_cache = set()
        self._relaxed_path_cache = set()

    def invalidate_caches(self):
        """Invalidate the directory mtime."""
        self._path_mtime = -1

    find_module = _find_module_shim

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
                for suffix, loader in self._loaders:
                    init_filename = '__init__' + suffix
                    full_path = _path_join(base_path, init_filename)
                    if _path_isfile(full_path):
                        return (loader(fullname, full_path), [base_path])
                else:
                    # A namespace package, return the path if we don't also
                    #  find a module in the next section.
                    is_namespace = True
        # Check for a file w/ a proper suffix exists.
        for suffix, loader in self._loaders:
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
        try:
            contents = _os.listdir(path)
        except (FileNotFoundError, PermissionError, NotADirectoryError):
            # Directory has either been removed, turned into a file, or made
            # unreadable.
            contents = []
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
        return "FileFinder({!r})".format(self.path)


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
            _call_with_frames_removed(import_, parent)
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
        exc = ImportError(_ERR_MSG.format(name), name=name)
        # TODO(brett): switch to a proper ModuleNotFound exception in Python
        # 3.4.
        exc._not_found = True
        raise exc
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
        if '*' in fromlist:
            fromlist = list(fromlist)
            fromlist.remove('*')
            if hasattr(module, '__all__'):
                fromlist.extend(module.__all__)
        for x in fromlist:
            if not hasattr(module, x):
                from_name = '{}.{}'.format(module.__name__, x)
                try:
                    _call_with_frames_removed(import_, from_name)
                except ImportError as exc:
                    # Backwards-compatibility dictates we ignore failed
                    # imports triggered by fromlist for modules that don't
                    # exist.
                    # TODO(brett): In Python 3.4, have import raise
                    #   ModuleNotFound and catch that.
                    if getattr(exc, '_not_found', False):
                        if exc.name == from_name:
                            continue
                    raise
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


def _get_supported_file_loaders():
    """Returns a list of file-based module loaders.

    Each item is a tuple (loader, suffixes, allow_packages).
    """
    extensions = ExtensionFileLoader, _imp.extension_suffixes()
    source = SourceFileLoader, SOURCE_SUFFIXES
    bytecode = SourcelessFileLoader, BYTECODE_SUFFIXES
    return [extensions, source, bytecode]


def __import__(name, globals=None, locals=None, fromlist=(), level=0):
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
        globals_ = globals if globals is not None else {}
        package = _calc___package__(globals_)
        module = _gcd_import(name, package, level)
    if not fromlist:
        # Return up to the first dot in 'name'. This is complicated by the fact
        # that 'name' may be relative.
        if level == 0:
            return _gcd_import(name.partition('.')[0])
        elif not name:
            return module
        else:
            # Figure out where to slice the module's name up to the first dot
            # in 'name'.
            cut_off = len(name) - len(name.partition('.')[0])
            # Slice end needs to be positive to alleviate need to special-case
            # when ``'.' not in name``.
            return sys.modules[module.__name__[:len(module.__name__)-cut_off]]
    else:
        return _handle_fromlist(module, fromlist, _gcd_import)



def _setup(sys_module, _imp_module):
    """Setup importlib by importing needed built-in modules and injecting them
    into the global namespace.

    As sys is needed for sys.modules access and _imp is needed to load built-in
    modules, those two modules must be explicitly passed in.

    """
    global _imp, sys, BYTECODE_SUFFIXES
    _imp = _imp_module
    sys = sys_module

    if sys.flags.optimize:
        BYTECODE_SUFFIXES = OPTIMIZED_BYTECODE_SUFFIXES
    else:
        BYTECODE_SUFFIXES = DEBUG_BYTECODE_SUFFIXES

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

    os_details = ('posix', ['/']), ('nt', ['\\', '/'])
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

    if builtin_os == 'nt':
        winreg_module = BuiltinImporter.load_module('winreg')
        setattr(self_module, '_winreg', winreg_module)

    setattr(self_module, '_os', os_module)
    setattr(self_module, '_thread', thread_module)
    setattr(self_module, '_weakref', weakref_module)
    setattr(self_module, 'path_sep', path_sep)
    setattr(self_module, 'path_separators', set(path_separators))
    # Constants
    setattr(self_module, '_relax_case', _make_relax_case())
    EXTENSION_SUFFIXES.extend(_imp.extension_suffixes())
    if builtin_os == 'nt':
        SOURCE_SUFFIXES.append('.pyw')
        if '_d.pyd' in EXTENSION_SUFFIXES:
            WindowsRegistryFinder.DEBUG_BUILD = True


def _install(sys_module, _imp_module):
    """Install importlib as the implementation of import."""
    _setup(sys_module, _imp_module)
    supported_loaders = _get_supported_file_loaders()
    sys.path_hooks.extend([FileFinder.path_hook(*supported_loaders)])
    sys.meta_path.append(BuiltinImporter)
    sys.meta_path.append(FrozenImporter)
    if _os.__name__ == 'nt':
        sys.meta_path.append(WindowsRegistryFinder)
    sys.meta_path.append(PathFinder)
