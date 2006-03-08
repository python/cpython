# WORK IN PROGRESS!  DO NOT (yet) USE!
import sys, os
import ctypes

__all__ = ["LibraryLoader", "RTLD_LOCAL", "RTLD_GLOBAL"]

if os.name in ("nt", "ce"):
    from _ctypes import LoadLibrary as dlopen
    RTLD_LOCAL = RTLD_GLOBAL = None
else:
    from _ctypes import dlopen, RTLD_LOCAL, RTLD_GLOBAL

# _findLib(name) returns an iterable of possible names for a library.
if os.name in ("nt", "ce"):
    def _findLib(name):
        return [name]

if os.name == "posix" and sys.platform == "darwin":
    from ctypes.macholib.dyld import dyld_find as _dyld_find
    def _findLib(name):
        possible = ['lib%s.dylib' % name,
                    '%s.dylib' % name,
                    '%s.framework/%s' % (name, name)]
        for name in possible:
            try:
                return [_dyld_find(name)]
            except ValueError:
                continue
        return []

elif os.name == "posix":
    # Andreas Degert's find functions, using gcc, /sbin/ldconfig, objdump
    import re, tempfile

    def _findLib_gcc(name):
        expr = '[^\(\)\s]*lib%s\.[^\(\)\s]*' % name
        cmd = 'if type gcc &>/dev/null; then CC=gcc; else CC=cc; fi;' \
              '$CC -Wl,-t -o /dev/null 2>&1 -l' + name
        try:
            fdout, outfile =  tempfile.mkstemp()
            fd = os.popen(cmd)
            trace = fd.read()
            err = fd.close()
        finally:
            try:
                os.unlink(outfile)
            except OSError, e:
                if e.errno != errno.ENOENT:
                    raise
        res = re.search(expr, trace)
        if not res:
            return None
        return res.group(0)

    def _findLib_ld(name):
        expr = '/[^\(\)\s]*lib%s\.[^\(\)\s]*' % name
        res = re.search(expr, os.popen('/sbin/ldconfig -p 2>/dev/null').read())
        if not res:
            return None
        return res.group(0)

    def _get_soname(f):
        cmd = "objdump -p -j .dynamic 2>/dev/null " + f
        res = re.search(r'\sSONAME\s+([^\s]+)', os.popen(cmd).read())
        if not res:
            return f
        return res.group(1)

    def _findLib(name):
        lib = _findLib_ld(name)
        if not lib:
            lib = _findLib_gcc(name)
            if not lib:
                return [name]
        return [_get_soname(lib)]

class LibraryLoader(object):
    """Loader for shared libraries.

    Shared libraries are accessed when compiling/linking a program,
    and when the program is run.  The purpose of the 'find' method is
    to locate a library similar to what the compiler does (on machines
    with several versions of a shared library the most recent should
    be loaded), while 'load' acts like when the program is run, and
    uses the runtime loader directly.  'load_version' works like
    'load' but tries to be platform independend (for cases where this
    makes sense).  Loading via attribute access is a shorthand
    notation especially useful for interactive use."""


    def __init__(self, dlltype, mode=RTLD_LOCAL):
        """Create a library loader instance which loads libraries by
        creating an instance of 'dlltype'.  'mode' can be RTLD_LOCAL
        or RTLD_GLOBAL, it is ignored on Windows.
        """
        self._dlltype = dlltype
        self._mode = mode

    def load(self, libname, mode=None):
        """Load and return the library with the given libname.  On
        most systems 'libname' is the filename of the shared library;
        when it's not a pathname it will be searched in a system
        dependend list of locations (on many systems additional search
        paths can be specified by an environment variable).  Sometimes
        the extension (like '.dll' on Windows) can be omitted.

        'mode' allows to override the default flags specified in the
        constructor, it is ignored on Windows.
        """
        if mode is None:
            mode = self._mode
        return self._load(libname, mode)

    def load_library(self, libname, mode=None):
        """Load and return the library with the given libname.  This
        method passes the specified 'libname' directly to the
        platform's library loading function (dlopen, or LoadLibrary).

        'mode' allows to override the default flags specified in the
        constructor, it is ignored on Windows.
        """
        if mode is None:
            mode = self._mode
        return self._dlltype(libname, mode)

    # alias name for backwards compatiblity
    LoadLibrary = load_library

    # Helpers for load and load_version - assembles a filename from name and filename
    if os.name in ("nt", "ce"):
        # Windows (XXX what about cygwin?)
        def _plat_load_version(self, name, version, mode):
            # not sure if this makes sense
            if version is not None:
                return self.load(name + version, mode)
            return self.load(name, mode)

        _load = load_library

    elif os.name == "posix" and sys.platform == "darwin":
        # Mac OS X
        def _plat_load_version(self, name, version, mode):
            if version:
                return self.load("lib%s.%s.dylib" % (name, version), mode)
            return self.load("lib%s.dylib" % name, mode)

        def _load(self, libname, mode):
            # _dyld_find raises ValueError, convert this into OSError
            try:
                pathname = _dyld_find(libname)
            except ValueError:
                raise OSError("Library %s could not be found" % libname)
            return self.load_library(pathname, mode)
            
    elif os.name == "posix":
        # Posix
        def _plat_load_version(self, name, version, mode):
            if version:
                return self.load("lib%s.so.%s" % (name, version), mode)
            return self.load("lib%s.so" % name, mode)

        _load = load_library

    else:
        # Others, TBD
        def _plat_load_version(self, name, version, mode=None):
            return self.load(name, mode)

        _load = load_library

    def load_version(self, name, version=None, mode=None):
        """Build a (system dependend) filename from 'name' and
        'version', then load and return it.  'name' is the library
        name without any prefix like 'lib' and suffix like '.so' or
        '.dylib'.  This method should be used if a library is
        available on different platforms, using the particular naming
        convention of each platform.

        'mode' allows to override the default flags specified in the
        constructor, it is ignored on Windows.
        """
        return self._plat_load_version(name, version, mode)

    def find(self, name, mode=None):
        """Try to find a library, load and return it.  'name' is the
        library name without any prefix like 'lib', suffix like '.so',
        '.dylib' or version number (this is the form used for the
        posix linker option '-l').

        'mode' allows to override the default flags specified in the
        constructor, it is ignored on Windows.

        On windows, this method does the same as the 'load' method.

        On other platforms, this function might call other programs
        like the compiler to find the library.  When using ctypes to
        write a shared library wrapping, consider using .load() or
        .load_version() instead.
        """
        for libname in _findLib(name):
            try:
                return self.load(libname, mode)
            except OSError:
                continue
        raise OSError("Library %r not found" % name)

    def __getattr__(self, name):
        """Load a library via attribute access.  Calls
        .load_version().  The result is cached."""
        if name.startswith("_"):
            raise AttributeError(name)
        dll = self.load_version(name)
        setattr(self, name, dll)
        return dll

################################################################
# test code

class CDLL(object):
    def __init__(self, name, mode):
        self._handle = dlopen(name, mode)
        self._name = name

    def __repr__(self):
        return "<%s '%s', handle %x at %x>" % \
               (self.__class__.__name__, self._name,
                (self._handle & (sys.maxint*2 + 1)),
                id(self))

cdll = LibraryLoader(CDLL)

def test():
    if os.name == "nt":
        print cdll.msvcrt
        print cdll.load("msvcrt")
        # load_version looks more like an artefact:
        print cdll.load_version("msvcr", "t")
        print cdll.find("msvcrt")

    if os.name == "posix":
        # find and load_version
        print cdll.find("m")
        print cdll.find("c")
        print cdll.load_version("crypto", "0.9.7")

        # getattr
        print cdll.m
        print cdll.bz2

        # load
        if sys.platform == "darwin":
            print cdll.load("libm.dylib")
            print cdll.load("libcrypto.dylib")
            print cdll.load("libSystem.dylib")
            print cdll.load("System.framework/System")
        else:
            print cdll.load("libm.so")
            print cdll.load("libcrypt.so")
            print cdll.find("crypt")

if __name__ == "__main__":
    test()
