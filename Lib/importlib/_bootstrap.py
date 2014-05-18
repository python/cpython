"""Core implementation of import.

This module is NOT meant to be directly imported! It has been designed such
that it can be bootstrapped into Python as the implementation of import. As
such it requires the injection of specific modules and attributes in order to
work. One should use importlib as the public-facing version of this module.

"""
#
# IMPORTANT: Whenever making changes to this module, be sure to run
# a top-level make in order to get the frozen version of the module
# update. Not doing so will result in the Makefile to fail for
# all others who don't have a ./python around to freeze the module
# in the early stages of compilation.
#

# See importlib._setup() for what is injected into the global namespace.

# When editing this code be aware that code executed at import time CANNOT
# reference any injected objects! This includes not only global code but also
# anything specified at the class level.

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


def _w_long(x):
    """Convert a 32-bit integer to little-endian."""
    return (int(x) & 0xFFFFFFFF).to_bytes(4, 'little')


def _r_long(int_bytes):
    """Convert 4 bytes in little-endian to an integer."""
    return int.from_bytes(int_bytes, 'little')


def _path_join(*path_parts):
    """Replacement for os.path.join()."""
    return path_sep.join([part.rstrip(path_separators)
                          for part in path_parts if part])


def _path_split(path):
    """Replacement for os.path.split()."""
    if len(path_separators) == 1:
        front, _, tail = path.rpartition(path_sep)
        return front, tail
    for x in reversed(path):
        if x in path_separators:
            front, tail = path.rsplit(x, maxsplit=1)
            return front, tail
    return '', path


def _path_stat(path):
    """Stat the path.

    Made a separate function to make it easier to override in experiments
    (e.g. cache stat results).

    """
    return _os.stat(path)


def _path_is_mode_type(path, mode):
    """Test whether the path is the specified mode type."""
    try:
        stat_info = _path_stat(path)
    except OSError:
        return False
    return (stat_info.st_mode & 0o170000) == mode


def _path_isfile(path):
    """Replacement for os.path.isfile."""
    return _path_is_mode_type(path, 0o100000)


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


def _new_module(name):
    return type(sys)(name)


_code_type = type(_wrap.__code__)



class _ManageReload:

    """Manages the possible clean-up of sys.modules for load_module()."""

    def __init__(self, name):
        self._name = name

    def __enter__(self):
        self._is_reload = self._name in sys.modules

    def __exit__(self, *args):
        if any(arg is not None for arg in args) and not self._is_reload:
            try:
                del sys.modules[self._name]
            except KeyError:
                pass

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
                        raise _DeadlockError('deadlock detected by %r' % self)
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
                raise RuntimeError('cannot release un-acquired lock')
            assert self.count > 0
            self.count -= 1
            if self.count == 0:
                self.owner = None
                if self.waiters:
                    self.waiters -= 1
                    self.wakeup.release()

    def __repr__(self):
        return '_ModuleLock({!r}) at {}'.format(self.name, id(self))


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
            raise RuntimeError('cannot release un-acquired lock')
        self.count -= 1

    def __repr__(self):
        return '_DummyModuleLock({!r}) at {}'.format(self.name, id(self))


class _ModuleLockManager:

    def __init__(self, name):
        self._name = name
        self._lock = None

    def __enter__(self):
        try:
            self._lock = _get_module_lock(self._name)
        finally:
            _imp.release_lock()
        self._lock.acquire()

    def __exit__(self, *args, **kwargs):
        self._lock.release()


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

# Magic word to reject .pyc files generated by other Python versions.
# It should change for each incompatible change to the bytecode.
#
# The value of CR and LF is incorporated so if you ever read or write
# a .pyc file in text mode the magic number will be wrong; also, the
# Apple MPW compiler swaps their values, botching string constants.
#
# The magic numbers must be spaced apart at least 2 values, as the
# -U interpeter flag will cause MAGIC+1 being used. They have been
# odd numbers for some time now.
#
# There were a variety of old schemes for setting the magic number.
# The current working scheme is to increment the previous value by
# 10.
#
# Starting with the adoption of PEP 3147 in Python 3.2, every bump in magic
# number also includes a new "magic tag", i.e. a human readable string used
# to represent the magic number in __pycache__ directories.  When you change
# the magic number, you must also set a new unique magic tag.  Generally this
# can be named after the Python major version of the magic number bump, but
# it can really be anything, as long as it's different than anything else
# that's come before.  The tags are included in the following table, starting
# with Python 3.2a0.
#
# Known values:
#  Python 1.5:   20121
#  Python 1.5.1: 20121
#     Python 1.5.2: 20121
#     Python 1.6:   50428
#     Python 2.0:   50823
#     Python 2.0.1: 50823
#     Python 2.1:   60202
#     Python 2.1.1: 60202
#     Python 2.1.2: 60202
#     Python 2.2:   60717
#     Python 2.3a0: 62011
#     Python 2.3a0: 62021
#     Python 2.3a0: 62011 (!)
#     Python 2.4a0: 62041
#     Python 2.4a3: 62051
#     Python 2.4b1: 62061
#     Python 2.5a0: 62071
#     Python 2.5a0: 62081 (ast-branch)
#     Python 2.5a0: 62091 (with)
#     Python 2.5a0: 62092 (changed WITH_CLEANUP opcode)
#     Python 2.5b3: 62101 (fix wrong code: for x, in ...)
#     Python 2.5b3: 62111 (fix wrong code: x += yield)
#     Python 2.5c1: 62121 (fix wrong lnotab with for loops and
#                          storing constants that should have been removed)
#     Python 2.5c2: 62131 (fix wrong code: for x, in ... in listcomp/genexp)
#     Python 2.6a0: 62151 (peephole optimizations and STORE_MAP opcode)
#     Python 2.6a1: 62161 (WITH_CLEANUP optimization)
#     Python 2.7a0: 62171 (optimize list comprehensions/change LIST_APPEND)
#     Python 2.7a0: 62181 (optimize conditional branches:
#                          introduce POP_JUMP_IF_FALSE and POP_JUMP_IF_TRUE)
#     Python 2.7a0  62191 (introduce SETUP_WITH)
#     Python 2.7a0  62201 (introduce BUILD_SET)
#     Python 2.7a0  62211 (introduce MAP_ADD and SET_ADD)
#     Python 3000:   3000
#                    3010 (removed UNARY_CONVERT)
#                    3020 (added BUILD_SET)
#                    3030 (added keyword-only parameters)
#                    3040 (added signature annotations)
#                    3050 (print becomes a function)
#                    3060 (PEP 3115 metaclass syntax)
#                    3061 (string literals become unicode)
#                    3071 (PEP 3109 raise changes)
#                    3081 (PEP 3137 make __file__ and __name__ unicode)
#                    3091 (kill str8 interning)
#                    3101 (merge from 2.6a0, see 62151)
#                    3103 (__file__ points to source file)
#     Python 3.0a4: 3111 (WITH_CLEANUP optimization).
#     Python 3.0a5: 3131 (lexical exception stacking, including POP_EXCEPT)
#     Python 3.1a0: 3141 (optimize list, set and dict comprehensions:
#             change LIST_APPEND and SET_ADD, add MAP_ADD)
#     Python 3.1a0: 3151 (optimize conditional branches:
#             introduce POP_JUMP_IF_FALSE and POP_JUMP_IF_TRUE)
#     Python 3.2a0: 3160 (add SETUP_WITH)
#                   tag: cpython-32
#     Python 3.2a1: 3170 (add DUP_TOP_TWO, remove DUP_TOPX and ROT_FOUR)
#                   tag: cpython-32
#     Python 3.2a2  3180 (add DELETE_DEREF)
#     Python 3.3a0  3190 __class__ super closure changed
#     Python 3.3a0  3200 (__qualname__ added)
#                      3210 (added size modulo 2**32 to the pyc header)
#     Python 3.3a1  3220 (changed PEP 380 implementation)
#     Python 3.3a4  3230 (revert changes to implicit __class__ closure)
#     Python 3.4a1  3250 (evaluate positional default arguments before
#                        keyword-only defaults)
#     Python 3.4a1  3260 (add LOAD_CLASSDEREF; allow locals of class to override
#                        free vars)
#     Python 3.4a1  3270 (various tweaks to the __class__ closure)
#     Python 3.4a1  3280 (remove implicit class argument)
#     Python 3.4a4  3290 (changes to __qualname__ computation)
#     Python 3.4a4  3300 (more changes to __qualname__ computation)
#     Python 3.4rc2 3310 (alter __qualname__ computation)
#
# MAGIC must change whenever the bytecode emitted by the compiler may no
# longer be understood by older implementations of the eval loop (usually
# due to the addition of new opcodes).

MAGIC_NUMBER = (3310).to_bytes(2, 'little') + b'\r\n'
_RAW_MAGIC_NUMBER = int.from_bytes(MAGIC_NUMBER, 'little')  # For import.c

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
    rest, _, extension = bytecode_path.rpartition('.')
    if not rest or extension.lower()[-3:-1] != 'py':
        return bytecode_path
    try:
        source_path = source_from_cache(bytecode_path)
    except (NotImplementedError, ValueError):
        source_path = bytecode_path[:-1]
    return source_path if _path_isfile(source_path) else bytecode_path


def _calc_mode(path):
    """Calculate the mode permissions for a bytecode file."""
    try:
        mode = _path_stat(path).st_mode
    except OSError:
        mode = 0o666
    # We always ensure write access so we can update cached files
    # later even when the source files are read-only on Windows (#6074)
    mode |= 0o200
    return mode


def _verbose_message(message, *args, verbosity=1):
    """Print the message to stderr if -v/PYTHONVERBOSE is turned on."""
    if sys.flags.verbose >= verbosity:
        if not message.startswith(('#', 'import ')):
            message = '# ' + message
        print(message.format(*args), file=sys.stderr)


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
            raise ImportError('loader cannot handle %s' % name, name=name)
        return method(self, name, *args, **kwargs)
    _wrap(_check_name_wrapper, method)
    return _check_name_wrapper


def _requires_builtin(fxn):
    """Decorator to verify the named module is built-in."""
    def _requires_builtin_wrapper(self, fullname):
        if fullname not in sys.builtin_module_names:
            raise ImportError('{!r} is not a built-in module'.format(fullname),
                              name=fullname)
        return fxn(self, fullname)
    _wrap(_requires_builtin_wrapper, fxn)
    return _requires_builtin_wrapper


def _requires_frozen(fxn):
    """Decorator to verify the named module is frozen."""
    def _requires_frozen_wrapper(self, fullname):
        if not _imp.is_frozen(fullname):
            raise ImportError('{!r} is not a frozen module'.format(fullname),
                              name=fullname)
        return fxn(self, fullname)
    _wrap(_requires_frozen_wrapper, fxn)
    return _requires_frozen_wrapper


def _find_module_shim(self, fullname):
    """Try to find a loader for the specified module by delegating to
    self.find_loader().

    This method is deprecated in favor of finder.find_spec().

    """
    # Call find_loader(). If it returns a string (indicating this
    # is a namespace package portion), generate a warning and
    # return None.
    loader, portions = self.find_loader(fullname)
    if loader is None and len(portions):
        msg = 'Not importing directory {}: missing __init__'
        _warnings.warn(msg.format(portions[0]), ImportWarning)
    return loader


def _load_module_shim(self, fullname):
    """Load the specified module into sys.modules and return it.

    This method is deprecated.  Use loader.exec_module instead.

    """
    spec = spec_from_loader(fullname, self)
    methods = _SpecMethods(spec)
    if fullname in sys.modules:
        module = sys.modules[fullname]
        methods.exec(module)
        return sys.modules[fullname]
    else:
        return methods.load()


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
    if magic != MAGIC_NUMBER:
        message = 'bad magic number in {!r}: {!r}'.format(name, magic)
        _verbose_message(message)
        raise ImportError(message, **exc_details)
    elif len(raw_timestamp) != 4:
        message = 'reached EOF while reading timestamp in {!r}'.format(name)
        _verbose_message(message)
        raise EOFError(message)
    elif len(raw_size) != 4:
        message = 'reached EOF while reading size of source in {!r}'.format(name)
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
                raise ImportError('bytecode is stale for {!r}'.format(name),
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
        raise ImportError('Non-code object in {!r}'.format(bytecode_path),
                          name=name, path=bytecode_path)

def _code_to_bytecode(code, mtime=0, source_size=0):
    """Compile a code object into bytecode for writing out to a byte-compiled
    file."""
    data = bytearray(MAGIC_NUMBER)
    data.extend(_w_long(mtime))
    data.extend(_w_long(source_size))
    data.extend(marshal.dumps(code))
    return data


def decode_source(source_bytes):
    """Decode bytes representing source code and return the string.

    Universal newline support is used in the decoding.
    """
    import tokenize  # To avoid bootstrap issues.
    source_bytes_readline = _io.BytesIO(source_bytes).readline
    encoding = tokenize.detect_encoding(source_bytes_readline)
    newline_decoder = _io.IncrementalNewlineDecoder(None, True)
    return newline_decoder.decode(source_bytes.decode(encoding[0]))


# Module specifications #######################################################

def _module_repr(module):
    # The implementation of ModuleType__repr__().
    loader = getattr(module, '__loader__', None)
    if hasattr(loader, 'module_repr'):
        # As soon as BuiltinImporter, FrozenImporter, and NamespaceLoader
        # drop their implementations for module_repr. we can add a
        # deprecation warning here.
        try:
            return loader.module_repr(module)
        except Exception:
            pass
    try:
        spec = module.__spec__
    except AttributeError:
        pass
    else:
        if spec is not None:
            return _SpecMethods(spec).module_repr()

    # We could use module.__class__.__name__ instead of 'module' in the
    # various repr permutations.
    try:
        name = module.__name__
    except AttributeError:
        name = '?'
    try:
        filename = module.__file__
    except AttributeError:
        if loader is None:
            return '<module {!r}>'.format(name)
        else:
            return '<module {!r} ({!r})>'.format(name, loader)
    else:
        return '<module {!r} from {!r}>'.format(name, filename)


class _installed_safely:

    def __init__(self, module):
        self._module = module
        self._spec = module.__spec__

    def __enter__(self):
        # This must be done before putting the module in sys.modules
        # (otherwise an optimization shortcut in import.c becomes
        # wrong)
        self._spec._initializing = True
        sys.modules[self._spec.name] = self._module

    def __exit__(self, *args):
        try:
            spec = self._spec
            if any(arg is not None for arg in args):
                try:
                    del sys.modules[spec.name]
                except KeyError:
                    pass
            else:
                _verbose_message('import {!r} # {!r}', spec.name, spec.loader)
        finally:
            self._spec._initializing = False


class ModuleSpec:
    """The specification for a module, used for loading.

    A module's spec is the source for information about the module.  For
    data associated with the module, including source, use the spec's
    loader.

    `name` is the absolute name of the module.  `loader` is the loader
    to use when loading the module.  `parent` is the name of the
    package the module is in.  The parent is derived from the name.

    `is_package` determines if the module is considered a package or
    not.  On modules this is reflected by the `__path__` attribute.

    `origin` is the specific location used by the loader from which to
    load the module, if that information is available.  When filename is
    set, origin will match.

    `has_location` indicates that a spec's "origin" reflects a location.
    When this is True, `__file__` attribute of the module is set.

    `cached` is the location of the cached bytecode file, if any.  It
    corresponds to the `__cached__` attribute.

    `submodule_search_locations` is the sequence of path entries to
    search when importing submodules.  If set, is_package should be
    True--and False otherwise.

    Packages are simply modules that (may) have submodules.  If a spec
    has a non-None value in `submodule_search_locations`, the import
    system will consider modules loaded from the spec as packages.

    Only finders (see importlib.abc.MetaPathFinder and
    importlib.abc.PathEntryFinder) should modify ModuleSpec instances.

    """

    def __init__(self, name, loader, *, origin=None, loader_state=None,
                 is_package=None):
        self.name = name
        self.loader = loader
        self.origin = origin
        self.loader_state = loader_state
        self.submodule_search_locations = [] if is_package else None

        # file-location attributes
        self._set_fileattr = False
        self._cached = None

    def __repr__(self):
        args = ['name={!r}'.format(self.name),
                'loader={!r}'.format(self.loader)]
        if self.origin is not None:
            args.append('origin={!r}'.format(self.origin))
        if self.submodule_search_locations is not None:
            args.append('submodule_search_locations={}'
                        .format(self.submodule_search_locations))
        return '{}({})'.format(self.__class__.__name__, ', '.join(args))

    def __eq__(self, other):
        smsl = self.submodule_search_locations
        try:
            return (self.name == other.name and
                    self.loader == other.loader and
                    self.origin == other.origin and
                    smsl == other.submodule_search_locations and
                    self.cached == other.cached and
                    self.has_location == other.has_location)
        except AttributeError:
            return False

    @property
    def cached(self):
        if self._cached is None:
            if self.origin is not None and self._set_fileattr:
                filename = self.origin
                if filename.endswith(tuple(SOURCE_SUFFIXES)):
                    try:
                        self._cached = cache_from_source(filename)
                    except NotImplementedError:
                        pass
                elif filename.endswith(tuple(BYTECODE_SUFFIXES)):
                    self._cached = filename
        return self._cached

    @cached.setter
    def cached(self, cached):
        self._cached = cached

    @property
    def parent(self):
        """The name of the module's parent."""
        if self.submodule_search_locations is None:
            return self.name.rpartition('.')[0]
        else:
            return self.name

    @property
    def has_location(self):
        return self._set_fileattr

    @has_location.setter
    def has_location(self, value):
        self._set_fileattr = bool(value)


def spec_from_loader(name, loader, *, origin=None, is_package=None):
    """Return a module spec based on various loader methods."""
    if hasattr(loader, 'get_filename'):
        if is_package is None:
            return spec_from_file_location(name, loader=loader)
        search = [] if is_package else None
        return spec_from_file_location(name, loader=loader,
                                       submodule_search_locations=search)

    if is_package is None:
        if hasattr(loader, 'is_package'):
            try:
                is_package = loader.is_package(name)
            except ImportError:
                is_package = None  # aka, undefined
        else:
            # the default
            is_package = False

    return ModuleSpec(name, loader, origin=origin, is_package=is_package)


_POPULATE = object()


def spec_from_file_location(name, location=None, *, loader=None,
                            submodule_search_locations=_POPULATE):
    """Return a module spec based on a file location.

    To indicate that the module is a package, set
    submodule_search_locations to a list of directory paths.  An
    empty list is sufficient, though its not otherwise useful to the
    import system.

    The loader must take a spec as its only __init__() arg.

    """
    if location is None:
        # The caller may simply want a partially populated location-
        # oriented spec.  So we set the location to a bogus value and
        # fill in as much as we can.
        location = '<unknown>'
        if hasattr(loader, 'get_filename'):
            # ExecutionLoader
            try:
                location = loader.get_filename(name)
            except ImportError:
                pass

    # If the location is on the filesystem, but doesn't actually exist,
    # we could return None here, indicating that the location is not
    # valid.  However, we don't have a good way of testing since an
    # indirect location (e.g. a zip file or URL) will look like a
    # non-existent file relative to the filesystem.

    spec = ModuleSpec(name, loader, origin=location)
    spec._set_fileattr = True

    # Pick a loader if one wasn't provided.
    if loader is None:
        for loader_class, suffixes in _get_supported_file_loaders():
            if location.endswith(tuple(suffixes)):
                loader = loader_class(name, location)
                spec.loader = loader
                break
        else:
            return None

    # Set submodule_search_paths appropriately.
    if submodule_search_locations is _POPULATE:
        # Check the loader.
        if hasattr(loader, 'is_package'):
            try:
                is_package = loader.is_package(name)
            except ImportError:
                pass
            else:
                if is_package:
                    spec.submodule_search_locations = []
    else:
        spec.submodule_search_locations = submodule_search_locations
    if spec.submodule_search_locations == []:
        if location:
            dirname = _path_split(location)[0]
            spec.submodule_search_locations.append(dirname)

    return spec


def _spec_from_module(module, loader=None, origin=None):
    # This function is meant for use in _setup().
    try:
        spec = module.__spec__
    except AttributeError:
        pass
    else:
        if spec is not None:
            return spec

    name = module.__name__
    if loader is None:
        try:
            loader = module.__loader__
        except AttributeError:
            # loader will stay None.
            pass
    try:
        location = module.__file__
    except AttributeError:
        location = None
    if origin is None:
        if location is None:
            try:
                origin = loader._ORIGIN
            except AttributeError:
                origin = None
        else:
            origin = location
    try:
        cached = module.__cached__
    except AttributeError:
        cached = None
    try:
        submodule_search_locations = list(module.__path__)
    except AttributeError:
        submodule_search_locations = None

    spec = ModuleSpec(name, loader, origin=origin)
    spec._set_fileattr = False if location is None else True
    spec.cached = cached
    spec.submodule_search_locations = submodule_search_locations
    return spec


class _SpecMethods:

    """Convenience wrapper around spec objects to provide spec-specific
    methods."""

    # The various spec_from_* functions could be made factory methods here.

    def __init__(self, spec):
        self.spec = spec

    def module_repr(self):
        """Return the repr to use for the module."""
        # We mostly replicate _module_repr() using the spec attributes.
        spec = self.spec
        name = '?' if spec.name is None else spec.name
        if spec.origin is None:
            if spec.loader is None:
                return '<module {!r}>'.format(name)
            else:
                return '<module {!r} ({!r})>'.format(name, spec.loader)
        else:
            if spec.has_location:
                return '<module {!r} from {!r}>'.format(name, spec.origin)
            else:
                return '<module {!r} ({})>'.format(spec.name, spec.origin)

    def init_module_attrs(self, module, *, _override=False, _force_name=True):
        """Set the module's attributes.

        All missing import-related module attributes will be set.  Here
        is how the spec attributes map onto the module:

        spec.name -> module.__name__
        spec.loader -> module.__loader__
        spec.parent -> module.__package__
        spec -> module.__spec__

        Optional:
        spec.origin -> module.__file__ (if spec.set_fileattr is true)
        spec.cached -> module.__cached__ (if __file__ also set)
        spec.submodule_search_locations -> module.__path__ (if set)

        """
        spec = self.spec

        # The passed in module may be not support attribute assignment,
        # in which case we simply don't set the attributes.

        # __name__
        if (_override or _force_name or
            getattr(module, '__name__', None) is None):
            try:
                module.__name__ = spec.name
            except AttributeError:
                pass

        # __loader__
        if _override or getattr(module, '__loader__', None) is None:
            loader = spec.loader
            if loader is None:
                # A backward compatibility hack.
                if spec.submodule_search_locations is not None:
                    loader = _NamespaceLoader.__new__(_NamespaceLoader)
                    loader._path = spec.submodule_search_locations
            try:
                module.__loader__ = loader
            except AttributeError:
                pass

        # __package__
        if _override or getattr(module, '__package__', None) is None:
            try:
                module.__package__ = spec.parent
            except AttributeError:
                pass

        # __spec__
        try:
            module.__spec__ = spec
        except AttributeError:
            pass

        # __path__
        if _override or getattr(module, '__path__', None) is None:
            if spec.submodule_search_locations is not None:
                try:
                    module.__path__ = spec.submodule_search_locations
                except AttributeError:
                    pass

        if spec.has_location:
            # __file__
            if _override or getattr(module, '__file__', None) is None:
                try:
                    module.__file__ = spec.origin
                except AttributeError:
                    pass

            # __cached__
            if _override or getattr(module, '__cached__', None) is None:
                if spec.cached is not None:
                    try:
                        module.__cached__ = spec.cached
                    except AttributeError:
                        pass

    def create(self):
        """Return a new module to be loaded.

        The import-related module attributes are also set with the
        appropriate values from the spec.

        """
        spec = self.spec
        # Typically loaders will not implement create_module().
        if hasattr(spec.loader, 'create_module'):
            # If create_module() returns `None` it means the default
            # module creation should be used.
            module = spec.loader.create_module(spec)
        else:
            module = None
        if module is None:
            # This must be done before open() is ever called as the 'io'
            # module implicitly imports 'locale' and would otherwise
            # trigger an infinite loop.
            module = _new_module(spec.name)
        self.init_module_attrs(module)
        return module

    def _exec(self, module):
        """Do everything necessary to execute the module.

        The namespace of `module` is used as the target of execution.
        This method uses the loader's `exec_module()` method.

        """
        self.spec.loader.exec_module(module)

    # Used by importlib.reload() and _load_module_shim().
    def exec(self, module):
        """Execute the spec in an existing module's namespace."""
        name = self.spec.name
        _imp.acquire_lock()
        with _ModuleLockManager(name):
            if sys.modules.get(name) is not module:
                msg = 'module {!r} not in sys.modules'.format(name)
                raise ImportError(msg, name=name)
            if self.spec.loader is None:
                if self.spec.submodule_search_locations is None:
                    raise ImportError('missing loader', name=self.spec.name)
                # namespace package
                self.init_module_attrs(module, _override=True)
                return module
            self.init_module_attrs(module, _override=True)
            if not hasattr(self.spec.loader, 'exec_module'):
                # (issue19713) Once BuiltinImporter and ExtensionFileLoader
                # have exec_module() implemented, we can add a deprecation
                # warning here.
                self.spec.loader.load_module(name)
            else:
                self._exec(module)
        return sys.modules[name]

    def _load_backward_compatible(self):
        # (issue19713) Once BuiltinImporter and ExtensionFileLoader
        # have exec_module() implemented, we can add a deprecation
        # warning here.
        spec = self.spec
        spec.loader.load_module(spec.name)
        # The module must be in sys.modules at this point!
        module = sys.modules[spec.name]
        if getattr(module, '__loader__', None) is None:
            try:
                module.__loader__ = spec.loader
            except AttributeError:
                pass
        if getattr(module, '__package__', None) is None:
            try:
                # Since module.__path__ may not line up with
                # spec.submodule_search_paths, we can't necessarily rely
                # on spec.parent here.
                module.__package__ = module.__name__
                if not hasattr(module, '__path__'):
                    module.__package__ = spec.name.rpartition('.')[0]
            except AttributeError:
                pass
        if getattr(module, '__spec__', None) is None:
            try:
                module.__spec__ = spec
            except AttributeError:
                pass
        return module

    def _load_unlocked(self):
        # A helper for direct use by the import system.
        if self.spec.loader is not None:
            # not a namespace package
            if not hasattr(self.spec.loader, 'exec_module'):
                return self._load_backward_compatible()

        module = self.create()
        with _installed_safely(module):
            if self.spec.loader is None:
                if self.spec.submodule_search_locations is None:
                    raise ImportError('missing loader', name=self.spec.name)
                # A namespace package so do nothing.
            else:
                self._exec(module)

        # We don't ensure that the import-related module attributes get
        # set in the sys.modules replacement case.  Such modules are on
        # their own.
        return sys.modules[self.spec.name]

    # A method used during testing of _load_unlocked() and by
    # _load_module_shim().
    def load(self):
        """Return a new module object, loaded by the spec's loader.

        The module is not added to its parent.

        If a module is already in sys.modules, that existing module gets
        clobbered.

        """
        _imp.acquire_lock()
        with _ModuleLockManager(self.spec.name):
            return self._load_unlocked()


def _fix_up_module(ns, name, pathname, cpathname=None):
    # This function is used by PyImport_ExecCodeModuleObject().
    loader = ns.get('__loader__')
    spec = ns.get('__spec__')
    if not loader:
        if spec:
            loader = spec.loader
        elif pathname == cpathname:
            loader = SourcelessFileLoader(name, pathname)
        else:
            loader = SourceFileLoader(name, pathname)
    if not spec:
        spec = spec_from_file_location(name, pathname, loader=loader)
    try:
        ns['__spec__'] = spec
        ns['__loader__'] = loader
        ns['__file__'] = pathname
        ns['__cached__'] = cpathname
    except Exception:
        # Not important enough to report.
        pass


# Loaders #####################################################################

class BuiltinImporter:

    """Meta path import for built-in modules.

    All methods are either class or static methods to avoid the need to
    instantiate the class.

    """

    @staticmethod
    def module_repr(module):
        """Return repr for the module.

        The method is deprecated.  The import machinery does the job itself.

        """
        return '<module {!r} (built-in)>'.format(module.__name__)

    @classmethod
    def find_spec(cls, fullname, path=None, target=None):
        if path is not None:
            return None
        if _imp.is_builtin(fullname):
            return spec_from_loader(fullname, cls, origin='built-in')
        else:
            return None

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find the built-in module.

        If 'path' is ever specified then the search is considered a failure.

        This method is deprecated.  Use find_spec() instead.

        """
        spec = cls.find_spec(fullname, path)
        return spec.loader if spec is not None else None

    @classmethod
    @_requires_builtin
    def load_module(cls, fullname):
        """Load a built-in module."""
        # Once an exec_module() implementation is added we can also
        # add a deprecation warning here.
        with _ManageReload(fullname):
            module = _call_with_frames_removed(_imp.init_builtin, fullname)
        module.__loader__ = cls
        module.__package__ = ''
        return module

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

    @staticmethod
    def module_repr(m):
        """Return repr for the module.

        The method is deprecated.  The import machinery does the job itself.

        """
        return '<module {!r} (frozen)>'.format(m.__name__)

    @classmethod
    def find_spec(cls, fullname, path=None, target=None):
        if _imp.is_frozen(fullname):
            return spec_from_loader(fullname, cls, origin='frozen')
        else:
            return None

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find a frozen module.

        This method is deprecated.  Use find_spec() instead.

        """
        return cls if _imp.is_frozen(fullname) else None

    @staticmethod
    def exec_module(module):
        name = module.__spec__.name
        if not _imp.is_frozen(name):
            raise ImportError('{!r} is not a frozen module'.format(name),
                              name=name)
        code = _call_with_frames_removed(_imp.get_frozen_object, name)
        exec(code, module.__dict__)

    @classmethod
    def load_module(cls, fullname):
        """Load a frozen module.

        This method is deprecated.  Use exec_module() instead.

        """
        return _load_module_shim(cls, fullname)

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

    """Meta path finder for modules declared in the Windows registry."""

    REGISTRY_KEY = (
        'Software\\Python\\PythonCore\\{sys_version}'
        '\\Modules\\{fullname}')
    REGISTRY_KEY_DEBUG = (
        'Software\\Python\\PythonCore\\{sys_version}'
        '\\Modules\\{fullname}\\Debug')
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
                filepath = _winreg.QueryValue(hkey, '')
        except OSError:
            return None
        return filepath

    @classmethod
    def find_spec(cls, fullname, path=None, target=None):
        filepath = cls._search_registry(fullname)
        if filepath is None:
            return None
        try:
            _path_stat(filepath)
        except OSError:
            return None
        for loader, suffixes in _get_supported_file_loaders():
            if filepath.endswith(tuple(suffixes)):
                spec = spec_from_loader(fullname, loader(fullname, filepath),
                                        origin=filepath)
                return spec

    @classmethod
    def find_module(cls, fullname, path=None):
        """Find module named in the registry.

        This method is deprecated.  Use exec_module() instead.

        """
        spec = cls.find_spec(fullname, path)
        if spec is not None:
            return spec.loader
        else:
            return None


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

    def exec_module(self, module):
        """Execute the module."""
        code = self.get_code(module.__name__)
        if code is None:
            raise ImportError('cannot load module {!r} when get_code() '
                              'returns None'.format(module.__name__))
        _call_with_frames_removed(exec, code, module.__dict__)

    load_module = _load_module_shim


class SourceLoader(_LoaderBasics):

    def path_mtime(self, path):
        """Optional method that returns the modification time (an int) for the
        specified path, where path is a str.

        Raises IOError when the path cannot be handled.
        """
        raise IOError

    def path_stats(self, path):
        """Optional method returning a metadata dict for the specified path
        to by the path (str).
        Possible keys:
        - 'mtime' (mandatory) is the numeric timestamp of last source
          code modification;
        - 'size' (optional) is the size in bytes of the source code.

        Implementing this method allows the loader to read bytecode files.
        Raises IOError when the path cannot be handled.
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


    def get_source(self, fullname):
        """Concrete implementation of InspectLoader.get_source."""
        path = self.get_filename(fullname)
        try:
            source_bytes = self.get_data(path)
        except OSError as exc:
            raise ImportError('source not available through get_data()',
                              name=fullname) from exc
        return decode_source(source_bytes)

    def source_to_code(self, data, path, *, _optimize=-1):
        """Return the code object compiled from source.

        The 'data' argument can be any object type that compile() supports.
        """
        return _call_with_frames_removed(compile, data, path, 'exec',
                                        dont_inherit=True, optimize=_optimize)

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
            except IOError:
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
            data = _code_to_bytecode(code_object, source_mtime,
                    len(source_bytes))
            try:
                self._cache_bytecode(source_path, bytecode_path, data)
                _verbose_message('wrote {!r}', bytecode_path)
            except NotImplementedError:
                pass
        return code_object


class FileLoader:

    """Base file loader class which implements the loader protocol methods that
    require file system usage."""

    def __init__(self, fullname, path):
        """Cache the module name and the path to the file found by the
        finder."""
        self.name = fullname
        self.path = path

    def __eq__(self, other):
        return (self.__class__ == other.__class__ and
                self.__dict__ == other.__dict__)

    def __hash__(self):
        return hash(self.name) ^ hash(self.path)

    @_check_name
    def load_module(self, fullname):
        """Load a module from a file.

        This method is deprecated.  Use exec_module() instead.

        """
        # The only reason for this method is for the name check.
        # Issue #14857: Avoid the zero-argument form of super so the implementation
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
        st = _path_stat(path)
        return {'mtime': st.st_mtime, 'size': st.st_size}

    def _cache_bytecode(self, source_path, bytecode_path, data):
        # Adapt between the two APIs
        mode = _calc_mode(source_path)
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

    def __eq__(self, other):
        return (self.__class__ == other.__class__ and
                self.__dict__ == other.__dict__)

    def __hash__(self):
        return hash(self.name) ^ hash(self.path)

    @_check_name
    def load_module(self, fullname):
        """Load an extension module."""
        # Once an exec_module() implementation is added we can also
        # add a deprecation warning here.
        with _ManageReload(fullname):
            module = _call_with_frames_removed(_imp.load_dynamic,
                                               fullname, self.path)
        _verbose_message('extension module loaded from {!r}', self.path)
        is_package = self.is_package(fullname)
        if is_package and not hasattr(module, '__path__'):
            module.__path__ = [_path_split(self.path)[0]]
        module.__loader__ = self
        module.__package__ = module.__name__
        if not is_package:
            module.__package__ = module.__package__.rpartition('.')[0]
        return module

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

    @_check_name
    def get_filename(self, fullname):
        """Return the path to the source file as found by the finder."""
        return self.path


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
            spec = self._path_finder(self._name, parent_path)
            # Note that no changes are made if a loader is returned, but we
            #  do remember the new parent path
            if spec is not None and spec.loader is None:
                if spec.submodule_search_locations:
                    self._path = spec.submodule_search_locations
            self._last_parent_path = parent_path     # Save the copy
        return self._path

    def __iter__(self):
        return iter(self._recalculate())

    def __len__(self):
        return len(self._recalculate())

    def __repr__(self):
        return '_NamespacePath({!r})'.format(self._path)

    def __contains__(self, item):
        return item in self._recalculate()

    def append(self, item):
        self._path.append(item)


# We use this exclusively in init_module_attrs() for backward-compatibility.
class _NamespaceLoader:
    def __init__(self, name, path, path_finder):
        self._path = _NamespacePath(name, path, path_finder)

    @classmethod
    def module_repr(cls, module):
        """Return repr for the module.

        The method is deprecated.  The import machinery does the job itself.

        """
        return '<module {!r} (namespace)>'.format(module.__name__)

    def is_package(self, fullname):
        return True

    def get_source(self, fullname):
        return ''

    def get_code(self, fullname):
        return compile('', '<string>', 'exec', dont_inherit=True)

    def exec_module(self, module):
        pass

    def load_module(self, fullname):
        """Load a namespace module.

        This method is deprecated.  Use exec_module() instead.

        """
        # The import system never calls this method.
        _verbose_message('namespace module loaded with path {!r}', self._path)
        return _load_module_shim(self, fullname)


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
            path = _os.getcwd()
        try:
            finder = sys.path_importer_cache[path]
        except KeyError:
            finder = cls._path_hooks(path)
            sys.path_importer_cache[path] = finder
        return finder

    @classmethod
    def _legacy_get_spec(cls, fullname, finder):
        # This would be a good place for a DeprecationWarning if
        # we ended up going that route.
        if hasattr(finder, 'find_loader'):
            loader, portions = finder.find_loader(fullname)
        else:
            loader = finder.find_module(fullname)
            portions = []
        if loader is not None:
            return spec_from_loader(fullname, loader)
        spec = ModuleSpec(fullname, None)
        spec.submodule_search_locations = portions
        return spec

    @classmethod
    def _get_spec(cls, fullname, path, target=None):
        """Find the loader or namespace_path for this module/package name."""
        # If this ends up being a namespace package, namespace_path is
        #  the list of paths that will become its __path__
        namespace_path = []
        for entry in path:
            if not isinstance(entry, (str, bytes)):
                continue
            finder = cls._path_importer_cache(entry)
            if finder is not None:
                if hasattr(finder, 'find_spec'):
                    spec = finder.find_spec(fullname, target)
                else:
                    spec = cls._legacy_get_spec(fullname, finder)
                if spec is None:
                    continue
                if spec.loader is not None:
                    return spec
                portions = spec.submodule_search_locations
                if portions is None:
                    raise ImportError('spec missing loader')
                # This is possibly part of a namespace package.
                #  Remember these path entries (if any) for when we
                #  create a namespace package, and continue iterating
                #  on path.
                namespace_path.extend(portions)
        else:
            spec = ModuleSpec(fullname, None)
            spec.submodule_search_locations = namespace_path
            return spec

    @classmethod
    def find_spec(cls, fullname, path=None, target=None):
        """find the module on sys.path or 'path' based on sys.path_hooks and
        sys.path_importer_cache."""
        if path is None:
            path = sys.path
        spec = cls._get_spec(fullname, path, target)
        if spec is None:
            return None
        elif spec.loader is None:
            namespace_path = spec.submodule_search_locations
            if namespace_path:
                # We found at least one namespace path.  Return a
                #  spec which can create the namespace package.
                spec.origin = 'namespace'
                spec.submodule_search_locations = _NamespacePath(fullname, namespace_path, cls._get_spec)
                return spec
            else:
                return None
        else:
            return spec

    @classmethod
    def find_module(cls, fullname, path=None):
        """find the module on sys.path or 'path' based on sys.path_hooks and
        sys.path_importer_cache.

        This method is deprecated.  Use find_spec() instead.

        """
        spec = cls.find_spec(fullname, path)
        if spec is None:
            return None
        return spec.loader


class FileFinder:

    """File-based finder.

    Interactions with the file system are cached for performance, being
    refreshed when the directory the finder is handling has been modified.

    """

    def __init__(self, path, *loader_details):
        """Initialize with the path to search on and a variable number of
        2-tuples containing the loader and the file suffixes the loader
        recognizes."""
        loaders = []
        for loader, suffixes in loader_details:
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
        package portions. Returns (loader, list-of-portions).

        This method is deprecated.  Use find_spec() instead.

        """
        spec = self.find_spec(fullname)
        if spec is None:
            return None, []
        return spec.loader, spec.submodule_search_locations or []

    def _get_spec(self, loader_class, fullname, path, smsl, target):
        loader = loader_class(fullname, path)
        return spec_from_file_location(fullname, path, loader=loader,
                                       submodule_search_locations=smsl)

    def find_spec(self, fullname, target=None):
        """Try to find a loader for the specified module, or the namespace
        package portions. Returns (loader, list-of-portions)."""
        is_namespace = False
        tail_module = fullname.rpartition('.')[2]
        try:
            mtime = _path_stat(self.path or _os.getcwd()).st_mtime
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
            for suffix, loader_class in self._loaders:
                init_filename = '__init__' + suffix
                full_path = _path_join(base_path, init_filename)
                if _path_isfile(full_path):
                    return self._get_spec(loader_class, fullname, full_path, [base_path], target)
            else:
                # If a namespace package, return the path if we don't
                #  find a module in the next section.
                is_namespace = _path_isdir(base_path)
        # Check for a file w/ a proper suffix exists.
        for suffix, loader_class in self._loaders:
            full_path = _path_join(self.path, tail_module + suffix)
            _verbose_message('trying {}'.format(full_path), verbosity=2)
            if cache_module + suffix in cache:
                if _path_isfile(full_path):
                    return self._get_spec(loader_class, fullname, full_path, None, target)
        if is_namespace:
            _verbose_message('possible namespace for {}'.format(base_path))
            spec = ModuleSpec(fullname, None)
            spec.submodule_search_locations = [base_path]
            return spec
        return None

    def _fill_cache(self):
        """Fill the cache of potential modules and packages for this directory."""
        path = self.path
        try:
            contents = _os.listdir(path or _os.getcwd())
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
            self._relaxed_path_cache = {fn.lower() for fn in contents}

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
                raise ImportError('only directories are supported', path=path)
            return cls(path, *loader_details)

        return path_hook_for_FileFinder

    def __repr__(self):
        return 'FileFinder({!r})'.format(self.path)


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


def _find_spec_legacy(finder, name, path):
    # This would be a good place for a DeprecationWarning if
    # we ended up going that route.
    loader = finder.find_module(name, path)
    if loader is None:
        return None
    return spec_from_loader(name, loader)


def _find_spec(name, path, target=None):
    """Find a module's loader."""
    if not sys.meta_path:
        _warnings.warn('sys.meta_path is empty', ImportWarning)
    # We check sys.modules here for the reload case.  While a passed-in
    # target will usually indicate a reload there is no guarantee, whereas
    # sys.modules provides one.
    is_reload = name in sys.modules
    for finder in sys.meta_path:
        with _ImportLockContext():
            try:
                find_spec = finder.find_spec
            except AttributeError:
                spec = _find_spec_legacy(finder, name, path)
                if spec is None:
                    continue
            else:
                spec = find_spec(name, path, target)
        if spec is not None:
            # The parent import may have already imported this module.
            if not is_reload and name in sys.modules:
                module = sys.modules[name]
                try:
                    __spec__ = module.__spec__
                except AttributeError:
                    # We use the found spec since that is the one that
                    # we would have used if the parent module hadn't
                    # beaten us to the punch.
                    return spec
                else:
                    if __spec__ is None:
                        return spec
                    else:
                        return __spec__
            else:
                return spec
    else:
        return None


def _sanity_check(name, package, level):
    """Verify arguments are "sane"."""
    if not isinstance(name, str):
        raise TypeError('module name must be str, not {}'.format(type(name)))
    if level < 0:
        raise ValueError('level must be >= 0')
    if package:
        if not isinstance(package, str):
            raise TypeError('__package__ not set to a string')
        elif package not in sys.modules:
            msg = ('Parent module {!r} not loaded, cannot perform relative '
                   'import')
            raise SystemError(msg.format(package))
    if not name and level == 0:
        raise ValueError('Empty module name')


_ERR_MSG_PREFIX = 'No module named '
_ERR_MSG = _ERR_MSG_PREFIX + '{!r}'

def _find_and_load_unlocked(name, import_):
    path = None
    parent = name.rpartition('.')[0]
    if parent:
        if parent not in sys.modules:
            _call_with_frames_removed(import_, parent)
        # Crazy side-effects!
        if name in sys.modules:
            return sys.modules[name]
        parent_module = sys.modules[parent]
        try:
            path = parent_module.__path__
        except AttributeError:
            msg = (_ERR_MSG + '; {!r} is not a package').format(name, parent)
            raise ImportError(msg, name=name)
    spec = _find_spec(name, path)
    if spec is None:
        raise ImportError(_ERR_MSG.format(name), name=name)
    else:
        module = _SpecMethods(spec)._load_unlocked()
    if parent:
        # Set the module as an attribute on its parent.
        parent_module = sys.modules[parent]
        setattr(parent_module, name.rpartition('.')[2], module)
    return module


def _find_and_load(name, import_):
    """Find and load the module, and release the import lock."""
    with _ModuleLockManager(name):
        return _find_and_load_unlocked(name, import_)


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
        message = ('import of {} halted; '
                   'None in sys.modules'.format(name))
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
                    if str(exc).startswith(_ERR_MSG_PREFIX):
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

    Each item is a tuple (loader, suffixes).
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


def _builtin_from_name(name):
    spec = BuiltinImporter.find_spec(name)
    if spec is None:
        raise ImportError('no built-in module named ' + name)
    methods = _SpecMethods(spec)
    return methods._load_unlocked()


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

    # Set up the spec for existing builtin/frozen modules.
    module_type = type(sys)
    for name, module in sys.modules.items():
        if isinstance(module, module_type):
            if name in sys.builtin_module_names:
                loader = BuiltinImporter
            elif _imp.is_frozen(name):
                loader = FrozenImporter
            else:
                continue
            spec = _spec_from_module(module, loader)
            methods = _SpecMethods(spec)
            methods.init_module_attrs(module)

    # Directly load built-in modules needed during bootstrap.
    self_module = sys.modules[__name__]
    for builtin_name in ('_io', '_warnings', 'builtins', 'marshal'):
        if builtin_name not in sys.modules:
            builtin_module = _builtin_from_name(builtin_name)
        else:
            builtin_module = sys.modules[builtin_name]
        setattr(self_module, builtin_name, builtin_module)

    # Directly load the os module (needed during bootstrap).
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
                os_module = _builtin_from_name(builtin_os)
                break
            except ImportError:
                continue
    else:
        raise ImportError('importlib requires posix or nt')
    setattr(self_module, '_os', os_module)
    setattr(self_module, 'path_sep', path_sep)
    setattr(self_module, 'path_separators', ''.join(path_separators))

    # Directly load the _thread module (needed during bootstrap).
    try:
        thread_module = _builtin_from_name('_thread')
    except ImportError:
        # Python was built without threads
        thread_module = None
    setattr(self_module, '_thread', thread_module)

    # Directly load the _weakref module (needed during bootstrap).
    weakref_module = _builtin_from_name('_weakref')
    setattr(self_module, '_weakref', weakref_module)

    # Directly load the winreg module (needed during bootstrap).
    if builtin_os == 'nt':
        winreg_module = _builtin_from_name('winreg')
        setattr(self_module, '_winreg', winreg_module)

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
