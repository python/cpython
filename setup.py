# Autodetecting setup.py script for building the Python extensions

import argparse
import importlib._bootstrap
import importlib.machinery
import importlib.util
import logging
import os
import re
import shlex
import sys
import sysconfig
import warnings
from glob import glob, escape
import _osx_support


try:
    import subprocess
    del subprocess
    SUBPROCESS_BOOTSTRAP = False
except ImportError:
    # Bootstrap Python: distutils.spawn uses subprocess to build C extensions,
    # subprocess requires C extensions built by setup.py like _posixsubprocess.
    #
    # Use _bootsubprocess which only uses the os module.
    #
    # It is dropped from sys.modules as soon as all C extension modules
    # are built.
    import _bootsubprocess
    sys.modules['subprocess'] = _bootsubprocess
    del _bootsubprocess
    SUBPROCESS_BOOTSTRAP = True


with warnings.catch_warnings():
    # bpo-41282 (PEP 632) deprecated distutils but setup.py still uses it
    warnings.filterwarnings(
        "ignore",
        "The distutils package is deprecated",
        DeprecationWarning
    )
    warnings.filterwarnings(
        "ignore",
        "The distutils.sysconfig module is deprecated, use sysconfig instead",
        DeprecationWarning
    )

    from distutils.command.build_ext import build_ext
    from distutils.command.build_scripts import build_scripts
    from distutils.command.install import install
    from distutils.command.install_lib import install_lib
    from distutils.core import Extension, setup
    from distutils.errors import CCompilerError, DistutilsError
    from distutils.spawn import find_executable


# This global variable is used to hold the list of modules to be disabled.
DISABLED_MODULE_LIST = []

# --list-module-names option used by Tools/scripts/generate_module_names.py
LIST_MODULE_NAMES = False


logging.basicConfig(format='%(message)s', level=logging.INFO)
log = logging.getLogger('setup')


def get_platform():
    # Cross compiling
    if "_PYTHON_HOST_PLATFORM" in os.environ:
        return os.environ["_PYTHON_HOST_PLATFORM"]

    # Get value of sys.platform
    if sys.platform.startswith('osf1'):
        return 'osf1'
    return sys.platform


CROSS_COMPILING = ("_PYTHON_HOST_PLATFORM" in os.environ)
HOST_PLATFORM = get_platform()
MS_WINDOWS = (HOST_PLATFORM == 'win32')
CYGWIN = (HOST_PLATFORM == 'cygwin')
MACOS = (HOST_PLATFORM == 'darwin')
AIX = (HOST_PLATFORM.startswith('aix'))
VXWORKS = ('vxworks' in HOST_PLATFORM)
EMSCRIPTEN = HOST_PLATFORM == 'emscripten-wasm32'
CC = os.environ.get("CC")
if not CC:
    CC = sysconfig.get_config_var("CC")

if EMSCRIPTEN:
    # emcc is a Python script from a different Python interpreter.
    os.environ.pop("PYTHONPATH", None)


SUMMARY = """
Python is an interpreted, interactive, object-oriented programming
language. It is often compared to Tcl, Perl, Scheme or Java.

Python combines remarkable power with very clear syntax. It has
modules, classes, exceptions, very high level dynamic data types, and
dynamic typing. There are interfaces to many system calls and
libraries, as well as to various windowing systems (X11, Motif, Tk,
Mac, MFC). New built-in modules are easily written in C or C++. Python
is also usable as an extension language for applications that need a
programmable interface.

The Python implementation is portable: it runs on many brands of UNIX,
on Windows, DOS, Mac, Amiga... If your favorite system isn't
listed here, it may still be supported, if there's a C compiler for
it. Ask around on comp.lang.python -- or just try compiling Python
yourself.
"""

CLASSIFIERS = """
Development Status :: 6 - Mature
License :: OSI Approved :: Python Software Foundation License
Natural Language :: English
Programming Language :: C
Programming Language :: Python
Topic :: Software Development
"""


def run_command(cmd):
    status = os.system(cmd)
    return os.waitstatus_to_exitcode(status)


# Set common compiler and linker flags derived from the Makefile,
# reserved for building the interpreter and the stdlib modules.
# See bpo-21121 and bpo-35257
def set_compiler_flags(compiler_flags, compiler_py_flags_nodist):
    flags = sysconfig.get_config_var(compiler_flags)
    py_flags_nodist = sysconfig.get_config_var(compiler_py_flags_nodist)
    sysconfig.get_config_vars()[compiler_flags] = flags + ' ' + py_flags_nodist


def add_dir_to_list(dirlist, dir):
    """Add the directory 'dir' to the list 'dirlist' (after any relative
    directories) if:

    1) 'dir' is not already in 'dirlist'
    2) 'dir' actually exists, and is a directory.
    """
    if dir is None or not os.path.isdir(dir) or dir in dirlist:
        return
    for i, path in enumerate(dirlist):
        if not os.path.isabs(path):
            dirlist.insert(i + 1, dir)
            return
    dirlist.insert(0, dir)


def sysroot_paths(make_vars, subdirs):
    """Get the paths of sysroot sub-directories.

    * make_vars: a sequence of names of variables of the Makefile where
      sysroot may be set.
    * subdirs: a sequence of names of subdirectories used as the location for
      headers or libraries.
    """

    dirs = []
    for var_name in make_vars:
        var = sysconfig.get_config_var(var_name)
        if var is not None:
            m = re.search(r'--sysroot=([^"]\S*|"[^"]+")', var)
            if m is not None:
                sysroot = m.group(1).strip('"')
                for subdir in subdirs:
                    if os.path.isabs(subdir):
                        subdir = subdir[1:]
                    path = os.path.join(sysroot, subdir)
                    if os.path.isdir(path):
                        dirs.append(path)
                break
    return dirs


MACOS_SDK_ROOT = None
MACOS_SDK_SPECIFIED = None

def macosx_sdk_root():
    """Return the directory of the current macOS SDK.

    If no SDK was explicitly configured, call the compiler to find which
    include files paths are being searched by default.  Use '/' if the
    compiler is searching /usr/include (meaning system header files are
    installed) or use the root of an SDK if that is being searched.
    (The SDK may be supplied via Xcode or via the Command Line Tools).
    The SDK paths used by Apple-supplied tool chains depend on the
    setting of various variables; see the xcrun man page for more info.
    Also sets MACOS_SDK_SPECIFIED for use by macosx_sdk_specified().
    """
    global MACOS_SDK_ROOT, MACOS_SDK_SPECIFIED

    # If already called, return cached result.
    if MACOS_SDK_ROOT:
        return MACOS_SDK_ROOT

    cflags = sysconfig.get_config_var('CFLAGS')
    m = re.search(r'-isysroot\s*(\S+)', cflags)
    if m is not None:
        MACOS_SDK_ROOT = m.group(1)
        MACOS_SDK_SPECIFIED = MACOS_SDK_ROOT != '/'
    else:
        MACOS_SDK_ROOT = _osx_support._default_sysroot(
            sysconfig.get_config_var('CC'))
        MACOS_SDK_SPECIFIED = False

    return MACOS_SDK_ROOT


def is_macosx_sdk_path(path):
    """
    Returns True if 'path' can be located in a macOS SDK
    """
    return ( (path.startswith('/usr/') and not path.startswith('/usr/local'))
                or path.startswith('/System/Library')
                or path.startswith('/System/iOSSupport') )


def grep_headers_for(function, headers):
    for header in headers:
        if not os.path.exists(header): continue
        with open(header, 'r', errors='surrogateescape') as f:
            if function in f.read():
                return True
    return False


def find_file(filename, std_dirs, paths):
    """Searches for the directory where a given file is located,
    and returns a possibly-empty list of additional directories, or None
    if the file couldn't be found at all.

    'filename' is the name of a file, such as readline.h or libcrypto.a.
    'std_dirs' is the list of standard system directories; if the
        file is found in one of them, no additional directives are needed.
    'paths' is a list of additional locations to check; if the file is
        found in one of them, the resulting list will contain the directory.
    """
    if MACOS:
        # Honor the MacOSX SDK setting when one was specified.
        # An SDK is a directory with the same structure as a real
        # system, but with only header files and libraries.
        sysroot = macosx_sdk_root()

    # Check the standard locations
    for dir_ in std_dirs:
        f = os.path.join(dir_, filename)

        if MACOS and is_macosx_sdk_path(dir_):
            f = os.path.join(sysroot, dir_[1:], filename)

        if os.path.exists(f): return []

    # Check the additional directories
    for dir_ in paths:
        f = os.path.join(dir_, filename)

        if MACOS and is_macosx_sdk_path(dir_):
            f = os.path.join(sysroot, dir_[1:], filename)

        if os.path.exists(f):
            return [dir_]

    # Not found anywhere
    return None


def validate_tzpath():
    base_tzpath = sysconfig.get_config_var('TZPATH')
    if not base_tzpath:
        return

    tzpaths = base_tzpath.split(os.pathsep)
    bad_paths = [tzpath for tzpath in tzpaths if not os.path.isabs(tzpath)]
    if bad_paths:
        raise ValueError('TZPATH must contain only absolute paths, '
                         + f'found:\n{tzpaths!r}\nwith invalid paths:\n'
                         + f'{bad_paths!r}')


def find_module_file(module, dirlist):
    """Find a module in a set of possible folders. If it is not found
    return the unadorned filename"""
    dirs = find_file(module, [], dirlist)
    if not dirs:
        return module
    if len(dirs) > 1:
        log.info(f"WARNING: multiple copies of {module} found")
    return os.path.abspath(os.path.join(dirs[0], module))


class PyBuildExt(build_ext):

    def __init__(self, dist):
        build_ext.__init__(self, dist)
        self.srcdir = None
        self.lib_dirs = None
        self.inc_dirs = None
        self.config_h_vars = None
        self.failed = []
        self.failed_on_import = []
        self.missing = []
        self.disabled_configure = []
        if '-j' in os.environ.get('MAKEFLAGS', ''):
            self.parallel = True

    def add(self, ext):
        self.extensions.append(ext)

    def addext(self, ext, *, update_flags=True):
        """Add extension with Makefile MODULE_{name} support
        """
        if update_flags:
            self.update_extension_flags(ext)

        state = sysconfig.get_config_var(f"MODULE_{ext.name.upper()}_STATE")
        if state == "yes":
            self.extensions.append(ext)
        elif state == "disabled":
            self.disabled_configure.append(ext.name)
        elif state == "missing":
            self.missing.append(ext.name)
        elif state == "n/a":
            # not available on current platform
            pass
        else:
            # not migrated to MODULE_{name}_STATE yet.
            self.announce(
                f'WARNING: Makefile is missing module variable for "{ext.name}"',
                level=2
            )
            self.extensions.append(ext)

    def update_extension_flags(self, ext):
        """Update extension flags with module CFLAGS and LDFLAGS

        Reads MODULE_{name}_CFLAGS and _LDFLAGS

        Distutils appends extra args to the compiler arguments. Some flags like
        -I must appear earlier, otherwise the pre-processor picks up files
        from system include directories.
        """
        upper_name = ext.name.upper()
        # Parse compiler flags (-I, -D, -U, extra args)
        cflags = sysconfig.get_config_var(f"MODULE_{upper_name}_CFLAGS")
        if cflags:
            for token in shlex.split(cflags):
                switch = token[0:2]
                value = token[2:]
                if switch == '-I':
                    ext.include_dirs.append(value)
                elif switch == '-D':
                    key, _, val = value.partition("=")
                    if not val:
                        val = None
                    ext.define_macros.append((key, val))
                elif switch == '-U':
                    ext.undef_macros.append(value)
                else:
                    ext.extra_compile_args.append(token)

        # Parse linker flags (-L, -l, extra objects, extra args)
        ldflags = sysconfig.get_config_var(f"MODULE_{upper_name}_LDFLAGS")
        if ldflags:
            for token in shlex.split(ldflags):
                switch = token[0:2]
                value = token[2:]
                if switch == '-L':
                    ext.library_dirs.append(value)
                elif switch == '-l':
                    ext.libraries.append(value)
                elif (
                    token[0] != '-' and
                    token.endswith(('.a', '.o', '.so', '.sl', '.dylib'))
                ):
                    ext.extra_objects.append(token)
                else:
                    ext.extra_link_args.append(token)

        return ext

    def set_srcdir(self):
        self.srcdir = sysconfig.get_config_var('srcdir')
        if not self.srcdir:
            # Maybe running on Windows but not using CYGWIN?
            raise ValueError("No source directory; cannot proceed.")
        self.srcdir = os.path.abspath(self.srcdir)

    def remove_disabled(self):
        # Remove modules that are present on the disabled list
        extensions = [ext for ext in self.extensions
                      if ext.name not in DISABLED_MODULE_LIST]
        # move ctypes to the end, it depends on other modules
        ext_map = dict((ext.name, i) for i, ext in enumerate(extensions))
        if "_ctypes" in ext_map:
            ctypes = extensions.pop(ext_map["_ctypes"])
            extensions.append(ctypes)
        self.extensions = extensions

    def update_sources_depends(self):
        # Fix up the autodetected modules, prefixing all the source files
        # with Modules/.
        # Add dependencies from MODULE_{name}_DEPS variable
        moddirlist = [
            # files in Modules/ directory
            os.path.join(self.srcdir, 'Modules'),
            # files relative to build base, e.g. libmpdec.a, libexpat.a
            os.getcwd()
        ]

        # Fix up the paths for scripts, too
        self.distribution.scripts = [os.path.join(self.srcdir, filename)
                                     for filename in self.distribution.scripts]

        # Python header files
        include_dir = escape(sysconfig.get_path('include'))
        headers = [sysconfig.get_config_h_filename()]
        headers.extend(glob(os.path.join(include_dir, "*.h")))
        headers.extend(glob(os.path.join(include_dir, "cpython", "*.h")))
        headers.extend(glob(os.path.join(include_dir, "internal", "*.h")))

        for ext in self.extensions:
            ext.sources = [ find_module_file(filename, moddirlist)
                            for filename in ext.sources ]
            # Update dependencies from Makefile
            makedeps = sysconfig.get_config_var(f"MODULE_{ext.name.upper()}_DEPS")
            if makedeps:
                # remove backslashes from line break continuations
                ext.depends.extend(
                    dep for dep in makedeps.split() if dep != "\\"
                )
            ext.depends = [
                find_module_file(filename, moddirlist) for filename in ext.depends
            ]
            # re-compile extensions if a header file has been changed
            ext.depends.extend(headers)

    def handle_configured_extensions(self):
        # The sysconfig variables built by makesetup that list the already
        # built modules and the disabled modules as configured by the Setup
        # files.
        sysconf_built = set(sysconfig.get_config_var('MODBUILT_NAMES').split())
        sysconf_shared = set(sysconfig.get_config_var('MODSHARED_NAMES').split())
        sysconf_dis = set(sysconfig.get_config_var('MODDISABLED_NAMES').split())

        mods_built = []
        mods_disabled = []
        for ext in self.extensions:
            # If a module has already been built or has been disabled in the
            # Setup files, don't build it here.
            if ext.name in sysconf_built:
                mods_built.append(ext)
            if ext.name in sysconf_dis:
                mods_disabled.append(ext)

        mods_configured = mods_built + mods_disabled
        if mods_configured:
            self.extensions = [x for x in self.extensions if x not in
                               mods_configured]
            # Remove the shared libraries built by a previous build.
            for ext in mods_configured:
                # Don't remove shared extensions which have been built
                # by Modules/Setup
                if ext.name in sysconf_shared:
                    continue
                fullpath = self.get_ext_fullpath(ext.name)
                if os.path.lexists(fullpath):
                    os.unlink(fullpath)

        return mods_built, mods_disabled

    def set_compiler_executables(self):
        # When you run "make CC=altcc" or something similar, you really want
        # those environment variables passed into the setup.py phase.  Here's
        # a small set of useful ones.
        compiler = os.environ.get('CC')
        args = {}
        # unfortunately, distutils doesn't let us provide separate C and C++
        # compilers
        if compiler is not None:
            (ccshared,cflags) = sysconfig.get_config_vars('CCSHARED','CFLAGS')
            args['compiler_so'] = compiler + ' ' + ccshared + ' ' + cflags
        self.compiler.set_executables(**args)

    def build_extensions(self):
        self.set_srcdir()
        self.set_compiler_executables()
        self.configure_compiler()
        self.init_inc_lib_dirs()

        # Detect which modules should be compiled
        self.detect_modules()

        if not LIST_MODULE_NAMES:
            self.remove_disabled()

        self.update_sources_depends()
        mods_built, mods_disabled = self.handle_configured_extensions()

        if LIST_MODULE_NAMES:
            for ext in self.extensions:
                print(ext.name)
            for name in self.missing:
                print(name)
            return

        build_ext.build_extensions(self)

        if SUBPROCESS_BOOTSTRAP:
            # Drop our custom subprocess module:
            # use the newly built subprocess module
            del sys.modules['subprocess']

        for ext in self.extensions:
            self.check_extension_import(ext)

        self.summary(mods_built, mods_disabled)

    def summary(self, mods_built, mods_disabled):
        longest = max([len(e.name) for e in self.extensions], default=0)
        if self.failed or self.failed_on_import:
            all_failed = self.failed + self.failed_on_import
            longest = max(longest, max([len(name) for name in all_failed]))

        def print_three_column(lst):
            lst.sort(key=str.lower)
            # guarantee zip() doesn't drop anything
            while len(lst) % 3:
                lst.append("")
            for e, f, g in zip(lst[::3], lst[1::3], lst[2::3]):
                print("%-*s   %-*s   %-*s" % (longest, e, longest, f,
                                              longest, g))

        if self.missing:
            print()
            print("The necessary bits to build these optional modules were not "
                  "found:")
            print_three_column(self.missing)
            print("To find the necessary bits, look in setup.py in"
                  " detect_modules() for the module's name.")
            print()

        if mods_built:
            print()
            print("The following modules found by detect_modules() in"
            " setup.py, have been")
            print("built by the Makefile instead, as configured by the"
            " Setup files:")
            print_three_column([ext.name for ext in mods_built])
            print()

        if mods_disabled:
            print()
            print("The following modules found by detect_modules() in"
            " setup.py have not")
            print("been built, they are *disabled* in the Setup files:")
            print_three_column([ext.name for ext in mods_disabled])
            print()

        if self.disabled_configure:
            print()
            print("The following modules found by detect_modules() in"
            " setup.py have not")
            print("been built, they are *disabled* by configure:")
            print_three_column(self.disabled_configure)
            print()

        if self.failed:
            failed = self.failed[:]
            print()
            print("Failed to build these modules:")
            print_three_column(failed)
            print()

        if self.failed_on_import:
            failed = self.failed_on_import[:]
            print()
            print("Following modules built successfully"
                  " but were removed because they could not be imported:")
            print_three_column(failed)
            print()

        if any('_ssl' in l
               for l in (self.missing, self.failed, self.failed_on_import)):
            print()
            print("Could not build the ssl module!")
            print("Python requires a OpenSSL 1.1.1 or newer")
            if sysconfig.get_config_var("OPENSSL_LDFLAGS"):
                print("Custom linker flags may require --with-openssl-rpath=auto")
            print()

        if os.environ.get("PYTHONSTRICTEXTENSIONBUILD") and (
            self.failed or self.failed_on_import or self.missing
        ):
            raise RuntimeError("Failed to build some stdlib modules")

    def build_extension(self, ext):

        if ext.name == '_ctypes':
            if not self.configure_ctypes(ext):
                self.failed.append(ext.name)
                return

        try:
            build_ext.build_extension(self, ext)
        except (CCompilerError, DistutilsError) as why:
            self.announce('WARNING: building of extension "%s" failed: %s' %
                          (ext.name, why))
            self.failed.append(ext.name)
            return

    def check_extension_import(self, ext):
        # Don't try to import an extension that has failed to compile
        if ext.name in self.failed:
            self.announce(
                'WARNING: skipping import check for failed build "%s"' %
                ext.name, level=1)
            return

        # Workaround for Mac OS X: The Carbon-based modules cannot be
        # reliably imported into a command-line Python
        if 'Carbon' in ext.extra_link_args:
            self.announce(
                'WARNING: skipping import check for Carbon-based "%s"' %
                ext.name)
            return

        if MACOS and (
                sys.maxsize > 2**32 and '-arch' in ext.extra_link_args):
            # Don't bother doing an import check when an extension was
            # build with an explicit '-arch' flag on OSX. That's currently
            # only used to build 32-bit only extensions in a 4-way
            # universal build and loading 32-bit code into a 64-bit
            # process will fail.
            self.announce(
                'WARNING: skipping import check for "%s"' %
                ext.name)
            return

        # Workaround for Cygwin: Cygwin currently has fork issues when many
        # modules have been imported
        if CYGWIN:
            self.announce('WARNING: skipping import check for Cygwin-based "%s"'
                % ext.name)
            return
        ext_filename = os.path.join(
            self.build_lib,
            self.get_ext_filename(self.get_ext_fullname(ext.name)))

        # If the build directory didn't exist when setup.py was
        # started, sys.path_importer_cache has a negative result
        # cached.  Clear that cache before trying to import.
        sys.path_importer_cache.clear()

        # Don't try to load extensions for cross builds
        if CROSS_COMPILING:
            return

        loader = importlib.machinery.ExtensionFileLoader(ext.name, ext_filename)
        spec = importlib.util.spec_from_file_location(ext.name, ext_filename,
                                                      loader=loader)
        try:
            importlib._bootstrap._load(spec)
        except ImportError as why:
            self.failed_on_import.append(ext.name)
            self.announce('*** WARNING: renaming "%s" since importing it'
                          ' failed: %s' % (ext.name, why), level=3)
            assert not self.inplace
            basename, tail = os.path.splitext(ext_filename)
            newname = basename + "_failed" + tail
            if os.path.exists(newname):
                os.remove(newname)
            os.rename(ext_filename, newname)

        except:
            exc_type, why, tb = sys.exc_info()
            self.announce('*** WARNING: importing extension "%s" '
                          'failed with %s: %s' % (ext.name, exc_type, why),
                          level=3)
            self.failed.append(ext.name)

    def add_multiarch_paths(self):
        # Debian/Ubuntu multiarch support.
        # https://wiki.ubuntu.com/MultiarchSpec
        tmpfile = os.path.join(self.build_temp, 'multiarch')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        ret = run_command(
            '%s -print-multiarch > %s 2> /dev/null' % (CC, tmpfile))
        multiarch_path_component = ''
        try:
            if ret == 0:
                with open(tmpfile) as fp:
                    multiarch_path_component = fp.readline().strip()
        finally:
            os.unlink(tmpfile)

        if multiarch_path_component != '':
            add_dir_to_list(self.compiler.library_dirs,
                            '/usr/lib/' + multiarch_path_component)
            add_dir_to_list(self.compiler.include_dirs,
                            '/usr/include/' + multiarch_path_component)
            return

        if not find_executable('dpkg-architecture'):
            return
        opt = ''
        if CROSS_COMPILING:
            opt = '-t' + sysconfig.get_config_var('HOST_GNU_TYPE')
        tmpfile = os.path.join(self.build_temp, 'multiarch')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        ret = run_command(
            'dpkg-architecture %s -qDEB_HOST_MULTIARCH > %s 2> /dev/null' %
            (opt, tmpfile))
        try:
            if ret == 0:
                with open(tmpfile) as fp:
                    multiarch_path_component = fp.readline().strip()
                add_dir_to_list(self.compiler.library_dirs,
                                '/usr/lib/' + multiarch_path_component)
                add_dir_to_list(self.compiler.include_dirs,
                                '/usr/include/' + multiarch_path_component)
        finally:
            os.unlink(tmpfile)

    def add_wrcc_search_dirs(self):
        # add library search path by wr-cc, the compiler wrapper

        def convert_mixed_path(path):
            # convert path like C:\folder1\folder2/folder3/folder4
            # to msys style /c/folder1/folder2/folder3/folder4
            drive = path[0].lower()
            left = path[2:].replace("\\", "/")
            return "/" + drive + left

        def add_search_path(line):
            # On Windows building machine, VxWorks does
            # cross builds under msys2 environment.
            pathsep = (";" if sys.platform == "msys" else ":")
            for d in line.strip().split("=")[1].split(pathsep):
                d = d.strip()
                if sys.platform == "msys":
                    # On Windows building machine, compiler
                    # returns mixed style path like:
                    # C:\folder1\folder2/folder3/folder4
                    d = convert_mixed_path(d)
                d = os.path.normpath(d)
                add_dir_to_list(self.compiler.library_dirs, d)

        tmpfile = os.path.join(self.build_temp, 'wrccpaths')
        os.makedirs(self.build_temp, exist_ok=True)
        try:
            ret = run_command('%s --print-search-dirs >%s' % (CC, tmpfile))
            if ret:
                return
            with open(tmpfile) as fp:
                # Parse paths in libraries line. The line is like:
                # On Linux, "libraries: = path1:path2:path3"
                # On Windows, "libraries: = path1;path2;path3"
                for line in fp:
                    if not line.startswith("libraries"):
                        continue
                    add_search_path(line)
        finally:
            try:
                os.unlink(tmpfile)
            except OSError:
                pass

    def add_cross_compiling_paths(self):
        tmpfile = os.path.join(self.build_temp, 'ccpaths')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        # bpo-38472: With a German locale, GCC returns "gcc-Version 9.1.0
        # (GCC)", whereas it returns "gcc version 9.1.0" with the C locale.
        ret = run_command('LC_ALL=C %s -E -v - </dev/null 2>%s 1>/dev/null' % (CC, tmpfile))
        is_gcc = False
        is_clang = False
        in_incdirs = False
        try:
            if ret == 0:
                with open(tmpfile) as fp:
                    for line in fp.readlines():
                        if line.startswith("gcc version"):
                            is_gcc = True
                        elif line.startswith("clang version"):
                            is_clang = True
                        elif line.startswith("#include <...>"):
                            in_incdirs = True
                        elif line.startswith("End of search list"):
                            in_incdirs = False
                        elif (is_gcc or is_clang) and line.startswith("LIBRARY_PATH"):
                            for d in line.strip().split("=")[1].split(":"):
                                d = os.path.normpath(d)
                                if '/gcc/' not in d:
                                    add_dir_to_list(self.compiler.library_dirs,
                                                    d)
                        elif (is_gcc or is_clang) and in_incdirs and '/gcc/' not in line and '/clang/' not in line:
                            add_dir_to_list(self.compiler.include_dirs,
                                            line.strip())
        finally:
            os.unlink(tmpfile)

        if VXWORKS:
            self.add_wrcc_search_dirs()

    def add_ldflags_cppflags(self):
        # Add paths specified in the environment variables LDFLAGS and
        # CPPFLAGS for header and library files.
        # We must get the values from the Makefile and not the environment
        # directly since an inconsistently reproducible issue comes up where
        # the environment variable is not set even though the value were passed
        # into configure and stored in the Makefile (issue found on OS X 10.3).
        for env_var, arg_name, dir_list in (
                ('LDFLAGS', '-R', self.compiler.runtime_library_dirs),
                ('LDFLAGS', '-L', self.compiler.library_dirs),
                ('CPPFLAGS', '-I', self.compiler.include_dirs)):
            env_val = sysconfig.get_config_var(env_var)
            if env_val:
                parser = argparse.ArgumentParser()
                parser.add_argument(arg_name, dest="dirs", action="append")

                # To prevent argparse from raising an exception about any
                # options in env_val that it mistakes for known option, we
                # strip out all double dashes and any dashes followed by a
                # character that is not for the option we are dealing with.
                #
                # Please note that order of the regex is important!  We must
                # strip out double-dashes first so that we don't end up with
                # substituting "--Long" to "-Long" and thus lead to "ong" being
                # used for a library directory.
                env_val = re.sub(r'(^|\s+)-(-|(?!%s))' % arg_name[1],
                                 ' ', env_val)
                options, _ = parser.parse_known_args(env_val.split())
                if options.dirs:
                    for directory in reversed(options.dirs):
                        add_dir_to_list(dir_list, directory)

    def configure_compiler(self):
        # Ensure that /usr/local is always used, but the local build
        # directories (i.e. '.' and 'Include') must be first.  See issue
        # 10520.
        if not CROSS_COMPILING:
            add_dir_to_list(self.compiler.library_dirs, '/usr/local/lib')
            add_dir_to_list(self.compiler.include_dirs, '/usr/local/include')
        # only change this for cross builds for 3.3, issues on Mageia
        if CROSS_COMPILING:
            self.add_cross_compiling_paths()
        self.add_multiarch_paths()
        self.add_ldflags_cppflags()

    def init_inc_lib_dirs(self):
        if (not CROSS_COMPILING and
                os.path.normpath(sys.base_prefix) != '/usr' and
                not sysconfig.get_config_var('PYTHONFRAMEWORK')):
            # OSX note: Don't add LIBDIR and INCLUDEDIR to building a framework
            # (PYTHONFRAMEWORK is set) to avoid # linking problems when
            # building a framework with different architectures than
            # the one that is currently installed (issue #7473)
            add_dir_to_list(self.compiler.library_dirs,
                            sysconfig.get_config_var("LIBDIR"))
            add_dir_to_list(self.compiler.include_dirs,
                            sysconfig.get_config_var("INCLUDEDIR"))

        system_lib_dirs = ['/lib64', '/usr/lib64', '/lib', '/usr/lib']
        system_include_dirs = ['/usr/include']
        # lib_dirs and inc_dirs are used to search for files;
        # if a file is found in one of those directories, it can
        # be assumed that no additional -I,-L directives are needed.
        if not CROSS_COMPILING:
            self.lib_dirs = self.compiler.library_dirs + system_lib_dirs
            self.inc_dirs = self.compiler.include_dirs + system_include_dirs
        else:
            # Add the sysroot paths. 'sysroot' is a compiler option used to
            # set the logical path of the standard system headers and
            # libraries.
            self.lib_dirs = (self.compiler.library_dirs +
                             sysroot_paths(('LDFLAGS', 'CC'), system_lib_dirs))
            self.inc_dirs = (self.compiler.include_dirs +
                             sysroot_paths(('CPPFLAGS', 'CFLAGS', 'CC'),
                                           system_include_dirs))

        config_h = sysconfig.get_config_h_filename()
        with open(config_h) as file:
            self.config_h_vars = sysconfig.parse_config_h(file)

        # OSF/1 and Unixware have some stuff in /usr/ccs/lib (like -ldb)
        if HOST_PLATFORM in ['osf1', 'unixware7', 'openunix8']:
            self.lib_dirs += ['/usr/ccs/lib']

        # HP-UX11iv3 keeps files in lib/hpux folders.
        if HOST_PLATFORM == 'hp-ux11':
            self.lib_dirs += ['/usr/lib/hpux64', '/usr/lib/hpux32']

        if MACOS:
            # This should work on any unixy platform ;-)
            # If the user has bothered specifying additional -I and -L flags
            # in OPT and LDFLAGS we might as well use them here.
            #
            # NOTE: using shlex.split would technically be more correct, but
            # also gives a bootstrap problem. Let's hope nobody uses
            # directories with whitespace in the name to store libraries.
            cflags, ldflags = sysconfig.get_config_vars(
                    'CFLAGS', 'LDFLAGS')
            for item in cflags.split():
                if item.startswith('-I'):
                    self.inc_dirs.append(item[2:])

            for item in ldflags.split():
                if item.startswith('-L'):
                    self.lib_dirs.append(item[2:])

    def detect_simple_extensions(self):
        #
        # The following modules are all pretty straightforward, and compile
        # on pretty much any POSIXish platform.
        #

        # array objects
        self.addext(Extension('array', ['arraymodule.c']))

        # Context Variables
        self.addext(Extension('_contextvars', ['_contextvarsmodule.c']))

        # math library functions, e.g. sin()
        self.addext(Extension('math',  ['mathmodule.c']))

        # complex math library functions
        self.addext(Extension('cmath', ['cmathmodule.c']))

        # libm is needed by delta_new() that uses round() and by accum() that
        # uses modf().
        self.addext(Extension('_datetime', ['_datetimemodule.c']))
        self.addext(Extension('_zoneinfo', ['_zoneinfo.c']))
        # random number generator implemented in C
        self.addext(Extension("_random", ["_randommodule.c"]))
        self.addext(Extension("_bisect", ["_bisectmodule.c"]))
        self.addext(Extension("_heapq", ["_heapqmodule.c"]))
        # C-optimized pickle replacement
        self.addext(Extension("_pickle", ["_pickle.c"]))
        # _json speedups
        self.addext(Extension("_json", ["_json.c"]))

        # profiler (_lsprof is for cProfile.py)
        self.addext(Extension('_lsprof', ['_lsprof.c', 'rotatingtree.c']))
        # static Unicode character database
        self.addext(Extension('unicodedata', ['unicodedata.c']))
        self.addext(Extension('_opcode', ['_opcode.c']))

        # asyncio speedups
        self.addext(Extension("_asyncio", ["_asynciomodule.c"]))

        self.addext(Extension("_queue", ["_queuemodule.c"]))
        self.addext(Extension("_statistics", ["_statisticsmodule.c"]))
        self.addext(Extension("_struct", ["_struct.c"]))
        self.addext(Extension("_typing", ["_typingmodule.c"]))

        # Modules with some UNIX dependencies -- on by default:
        # (If you have a really backward UNIX, select and socket may not be
        # supported...)

        # fcntl(2) and ioctl(2)
        self.addext(Extension('fcntl', ['fcntlmodule.c']))
        # grp(3)
        self.addext(Extension('grp', ['grpmodule.c']))

        self.addext(Extension('_socket', ['socketmodule.c']))
        self.addext(Extension('spwd', ['spwdmodule.c']))

        # select(2); not on ancient System V
        self.addext(Extension('select', ['selectmodule.c']))

        # Memory-mapped files (also works on Win32).
        self.addext(Extension('mmap', ['mmapmodule.c']))

        # Lance Ellinghaus's syslog module
        # syslog daemon interface
        self.addext(Extension('syslog', ['syslogmodule.c']))

        # Python interface to subinterpreter C-API.
        self.addext(Extension('_xxsubinterpreters', ['_xxsubinterpretersmodule.c']))

        #
        # Here ends the simple stuff.  From here on, modules need certain
        # libraries, are platform-specific, or present other surprises.
        #

        # Multimedia modules
        # These don't work for 64-bit platforms!!!
        # These represent audio samples or images as strings:
        #
        # Operations on audio samples
        # According to #993173, this one should actually work fine on
        # 64-bit platforms.
        #
        # audioop needs libm for floor() in multiple functions.
        self.addext(Extension('audioop', ['audioop.c']))

        # CSV files
        self.addext(Extension('_csv', ['_csv.c']))

        # POSIX subprocess module helper.
        self.addext(Extension('_posixsubprocess', ['_posixsubprocess.c']))

    def detect_test_extensions(self):
        # Python C API test module
        self.addext(Extension('_testcapi', ['_testcapimodule.c']))

        # Python Argument Clinc functional test module
        self.addext(Extension('_testclinic', ['_testclinic.c']))

        # Python Internal C API test module
        self.addext(Extension('_testinternalcapi', ['_testinternalcapi.c']))

        # Python PEP-3118 (buffer protocol) test module
        self.addext(Extension('_testbuffer', ['_testbuffer.c']))

        # Test loading multiple modules from one compiled file (https://bugs.python.org/issue16421)
        self.addext(Extension('_testimportmultiple', ['_testimportmultiple.c']))

        # Test multi-phase extension module init (PEP 489)
        self.addext(Extension('_testmultiphase', ['_testmultiphase.c']))

        # Fuzz tests.
        self.addext(Extension(
            '_xxtestfuzz',
            ['_xxtestfuzz/_xxtestfuzz.c', '_xxtestfuzz/fuzzer.c']
        ))

    def detect_readline_curses(self):
        # readline
        readline_termcap_library = ""
        curses_library = ""
        # Cannot use os.popen here in py3k.
        tmpfile = os.path.join(self.build_temp, 'readline_termcap_lib')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        # Determine if readline is already linked against curses or tinfo.
        if sysconfig.get_config_var('HAVE_LIBREADLINE'):
            if sysconfig.get_config_var('WITH_EDITLINE'):
                readline_lib = 'edit'
            else:
                readline_lib = 'readline'
            do_readline = self.compiler.find_library_file(self.lib_dirs,
                readline_lib)
            if CROSS_COMPILING:
                ret = run_command("%s -d %s | grep '(NEEDED)' > %s"
                                % (sysconfig.get_config_var('READELF'),
                                   do_readline, tmpfile))
            elif find_executable('ldd'):
                ret = run_command("ldd %s > %s" % (do_readline, tmpfile))
            else:
                ret = 1
            if ret == 0:
                with open(tmpfile) as fp:
                    for ln in fp:
                        if 'curses' in ln:
                            readline_termcap_library = re.sub(
                                r'.*lib(n?cursesw?)\.so.*', r'\1', ln
                            ).rstrip()
                            break
                        # termcap interface split out from ncurses
                        if 'tinfo' in ln:
                            readline_termcap_library = 'tinfo'
                            break
            if os.path.exists(tmpfile):
                os.unlink(tmpfile)
        else:
            do_readline = False
        # Issue 7384: If readline is already linked against curses,
        # use the same library for the readline and curses modules.
        if 'curses' in readline_termcap_library:
            curses_library = readline_termcap_library
        elif self.compiler.find_library_file(self.lib_dirs, 'ncursesw'):
            curses_library = 'ncursesw'
        # Issue 36210: OSS provided ncurses does not link on AIX
        # Use IBM supplied 'curses' for successful build of _curses
        elif AIX and self.compiler.find_library_file(self.lib_dirs, 'curses'):
            curses_library = 'curses'
        elif self.compiler.find_library_file(self.lib_dirs, 'ncurses'):
            curses_library = 'ncurses'
        elif self.compiler.find_library_file(self.lib_dirs, 'curses'):
            curses_library = 'curses'

        if MACOS:
            os_release = int(os.uname()[2].split('.')[0])
            dep_target = sysconfig.get_config_var('MACOSX_DEPLOYMENT_TARGET')
            if (dep_target and
                    (tuple(int(n) for n in dep_target.split('.')[0:2])
                        < (10, 5) ) ):
                os_release = 8
            if os_release < 9:
                # MacOSX 10.4 has a broken readline. Don't try to build
                # the readline module unless the user has installed a fixed
                # readline package
                if find_file('readline/rlconf.h', self.inc_dirs, []) is None:
                    do_readline = False
        if do_readline:
            readline_libs = [readline_lib]
            if readline_termcap_library:
                pass # Issue 7384: Already linked against curses or tinfo.
            elif curses_library:
                readline_libs.append(curses_library)
            elif self.compiler.find_library_file(self.lib_dirs +
                                                     ['/usr/lib/termcap'],
                                                     'termcap'):
                readline_libs.append('termcap')
            self.add(Extension('readline', ['readline.c'],
                               library_dirs=['/usr/lib/termcap'],
                               libraries=readline_libs))
        else:
            self.missing.append('readline')

        # Curses support, requiring the System V version of curses, often
        # provided by the ncurses library.
        curses_defines = []
        curses_includes = []
        panel_library = 'panel'
        if curses_library == 'ncursesw':
            curses_defines.append(('HAVE_NCURSESW', '1'))
            if not CROSS_COMPILING:
                curses_includes.append('/usr/include/ncursesw')
            # Bug 1464056: If _curses.so links with ncursesw,
            # _curses_panel.so must link with panelw.
            panel_library = 'panelw'
            if MACOS:
                # On OS X, there is no separate /usr/lib/libncursesw nor
                # libpanelw.  If we are here, we found a locally-supplied
                # version of libncursesw.  There should also be a
                # libpanelw.  _XOPEN_SOURCE defines are usually excluded
                # for OS X but we need _XOPEN_SOURCE_EXTENDED here for
                # ncurses wide char support
                curses_defines.append(('_XOPEN_SOURCE_EXTENDED', '1'))
        elif MACOS and curses_library == 'ncurses':
            # Building with the system-suppied combined libncurses/libpanel
            curses_defines.append(('HAVE_NCURSESW', '1'))
            curses_defines.append(('_XOPEN_SOURCE_EXTENDED', '1'))

        curses_enabled = True
        if curses_library.startswith('ncurses'):
            curses_libs = [curses_library]
            self.add(Extension('_curses', ['_cursesmodule.c'],
                               include_dirs=curses_includes,
                               define_macros=curses_defines,
                               libraries=curses_libs))
        elif curses_library == 'curses' and not MACOS:
                # OSX has an old Berkeley curses, not good enough for
                # the _curses module.
            if (self.compiler.find_library_file(self.lib_dirs, 'terminfo')):
                curses_libs = ['curses', 'terminfo']
            elif (self.compiler.find_library_file(self.lib_dirs, 'termcap')):
                curses_libs = ['curses', 'termcap']
            else:
                curses_libs = ['curses']

            self.add(Extension('_curses', ['_cursesmodule.c'],
                               define_macros=curses_defines,
                               libraries=curses_libs))
        else:
            curses_enabled = False
            self.missing.append('_curses')

        # If the curses module is enabled, check for the panel module
        # _curses_panel needs some form of ncurses
        skip_curses_panel = True if AIX else False
        if (curses_enabled and not skip_curses_panel and
                self.compiler.find_library_file(self.lib_dirs, panel_library)):
            self.add(Extension('_curses_panel', ['_curses_panel.c'],
                           include_dirs=curses_includes,
                           define_macros=curses_defines,
                           libraries=[panel_library, *curses_libs]))
        elif not skip_curses_panel:
            self.missing.append('_curses_panel')

    def detect_crypt(self):
        self.addext(Extension('_crypt', ['_cryptmodule.c']))

    def detect_dbm_gdbm(self):
        # Modules that provide persistent dictionary-like semantics.  You will
        # probably want to arrange for at least one of them to be available on
        # your machine, though none are defined by default because of library
        # dependencies.  The Python module dbm/__init__.py provides an
        # implementation independent wrapper for these; dbm/dumb.py provides
        # similar functionality (but slower of course) implemented in Python.

        dbm_setup_debug = False   # verbose debug prints from this script?
        dbm_order = ['gdbm']

        # libdb, gdbm and ndbm headers and libraries
        have_ndbm_h = sysconfig.get_config_var("HAVE_NDBM_H")
        have_gdbm_ndbm_h = sysconfig.get_config_var("HAVE_GDBM_NDBM_H")
        have_gdbm_dash_ndbm_h = sysconfig.get_config_var("HAVE_GDBM_DASH_NDBM_H")
        have_libndbm = sysconfig.get_config_var("HAVE_LIBNDBM")
        have_libgdbm_compat = sysconfig.get_config_var("HAVE_LIBGDBM_COMPAT")
        have_libdb = sysconfig.get_config_var("HAVE_LIBDB")

        # The standard Unix dbm module:
        if not CYGWIN:
            config_args = [arg.strip("'")
                           for arg in sysconfig.get_config_var("CONFIG_ARGS").split()]
            dbm_args = [arg for arg in config_args
                        if arg.startswith('--with-dbmliborder=')]
            if dbm_args:
                dbm_order = [arg.split('=')[-1] for arg in dbm_args][-1].split(":")
            else:
                dbm_order = "gdbm:ndbm:bdb".split(":")
            dbmext = None
            for cand in dbm_order:
                if cand == "ndbm":
                    if have_ndbm_h:
                        # Some systems have -lndbm, others have -lgdbm_compat,
                        # others don't have either
                        if have_libndbm:
                            ndbm_libs = ['ndbm']
                        elif have_libgdbm_compat:
                            ndbm_libs = ['gdbm_compat']
                        else:
                            ndbm_libs = []
                        if dbm_setup_debug: print("building dbm using ndbm")
                        dbmext = Extension(
                            '_dbm', ['_dbmmodule.c'],
                            define_macros=[('USE_NDBM', None)],
                            libraries=ndbm_libs
                        )
                        break
                elif cand == "gdbm":
                    # dbm_open() is provided by libgdbm_compat, which wraps libgdbm
                    if have_libgdbm_compat and (have_gdbm_ndbm_h or have_gdbm_dash_ndbm_h):
                        if dbm_setup_debug: print("building dbm using gdbm")
                        dbmext = Extension(
                            '_dbm', ['_dbmmodule.c'],
                            define_macros=[('USE_GDBM_COMPAT', None)],
                            libraries=['gdbm_compat']
                        )
                        break
                elif cand == "bdb":
                    if have_libdb:
                        if dbm_setup_debug: print("building dbm using bdb")
                        dbmext = Extension(
                            '_dbm', ['_dbmmodule.c'],
                            define_macros=[('USE_BERKDB', None)],
                            libraries=['db']
                        )
                        break
            if dbmext is not None:
                self.add(dbmext)
            else:
                self.missing.append('_dbm')

        # Anthony Baxter's gdbm module.  GNU dbm(3) will require -lgdbm:
        self.addext(Extension('_gdbm', ['_gdbmmodule.c']))

    def detect_sqlite(self):
        sources = [
            "_sqlite/blob.c",
            "_sqlite/connection.c",
            "_sqlite/cursor.c",
            "_sqlite/microprotocols.c",
            "_sqlite/module.c",
            "_sqlite/prepare_protocol.c",
            "_sqlite/row.c",
            "_sqlite/statement.c",
            "_sqlite/util.c",
        ]
        self.addext(Extension("_sqlite3", sources=sources))

    def detect_platform_specific_exts(self):
        # Unix-only modules
        # Steen Lumholt's termios module
        self.addext(Extension('termios', ['termios.c']))
        # Jeremy Hylton's rlimit interface
        self.addext(Extension('resource', ['resource.c']))
        # linux/soundcard.h or sys/soundcard.h
        self.addext(Extension('ossaudiodev', ['ossaudiodev.c']))

        # macOS-only, needs SystemConfiguration and CoreFoundation framework
        self.addext(Extension('_scproxy', ['_scproxy.c']))

    def detect_compress_exts(self):
        # Andrew Kuchling's zlib module.
        self.addext(Extension('zlib', ['zlibmodule.c']))

        # Helper module for various ascii-encoders.  Uses zlib for an optimized
        # crc32 if we have it.  Otherwise binascii uses its own.
        self.addext(Extension('binascii', ['binascii.c']))

        # Gustavo Niemeyer's bz2 module.
        self.addext(Extension('_bz2', ['_bz2module.c']))

        # LZMA compression support.
        self.addext(Extension('_lzma', ['_lzmamodule.c']))

    def detect_expat_elementtree(self):
        # Interface to the Expat XML parser
        #
        # Expat was written by James Clark and is now maintained by a group of
        # developers on SourceForge; see www.libexpat.org for more information.
        # The pyexpat module was written by Paul Prescod after a prototype by
        # Jack Jansen.  The Expat source is included in Modules/expat/.  Usage
        # of a system shared libexpat.so is possible with --with-system-expat
        # configure option.
        #
        # More information on Expat can be found at www.libexpat.org.
        #
        self.addext(Extension('pyexpat', sources=['pyexpat.c']))

        # Fredrik Lundh's cElementTree module.  Note that this also
        # uses expat (via the CAPI hook in pyexpat).
        self.addext(Extension('_elementtree', sources=['_elementtree.c']))

    def detect_multibytecodecs(self):
        # Hye-Shik Chang's CJKCodecs modules.
        self.addext(Extension('_multibytecodec',
                              ['cjkcodecs/multibytecodec.c']))
        for loc in ('kr', 'jp', 'cn', 'tw', 'hk', 'iso2022'):
            self.addext(Extension(
                f'_codecs_{loc}', [f'cjkcodecs/_codecs_{loc}.c']
            ))

    def detect_multiprocessing(self):
        # Richard Oudkerk's multiprocessing module
        multiprocessing_srcs = ['_multiprocessing/multiprocessing.c']
        if (
            sysconfig.get_config_var('HAVE_SEM_OPEN') and not
            sysconfig.get_config_var('POSIX_SEMAPHORES_NOT_ENABLED')
        ):
            multiprocessing_srcs.append('_multiprocessing/semaphore.c')
        self.addext(Extension('_multiprocessing', multiprocessing_srcs))
        self.addext(Extension('_posixshmem', ['_multiprocessing/posixshmem.c']))

    def detect_uuid(self):
        # Build the _uuid module if possible
        self.addext(Extension('_uuid', ['_uuidmodule.c']))

    def detect_modules(self):
        # remove dummy extension
        self.extensions = []

        # Some C extensions are built by entries in Modules/Setup.bootstrap.
        # These are extensions are required to bootstrap the interpreter or
        # build process.
        self.detect_simple_extensions()
        self.detect_test_extensions()
        self.detect_readline_curses()
        self.detect_crypt()
        self.detect_openssl_hashlib()
        self.detect_hash_builtins()
        self.detect_dbm_gdbm()
        self.detect_sqlite()
        self.detect_platform_specific_exts()
        self.detect_nis()
        self.detect_compress_exts()
        self.detect_expat_elementtree()
        self.detect_multibytecodecs()
        self.detect_decimal()
        self.detect_ctypes()
        self.detect_multiprocessing()
        self.detect_tkinter()
        self.detect_uuid()

        # Uncomment the next line if you want to play with xxmodule.c
#        self.add(Extension('xx', ['xxmodule.c']))

        self.addext(Extension('xxlimited', ['xxlimited.c']))
        self.addext(Extension('xxlimited_35', ['xxlimited_35.c']))

    def detect_tkinter(self):
        self.addext(Extension('_tkinter', ['_tkinter.c', 'tkappinit.c']))

    def configure_ctypes(self, ext):
        return True

    def detect_ctypes(self):
        # Thomas Heller's _ctypes module

        if (not sysconfig.get_config_var("LIBFFI_INCLUDEDIR") and MACOS):
            self.use_system_libffi = True
        else:
            self.use_system_libffi = '--with-system-ffi' in sysconfig.get_config_var("CONFIG_ARGS")

        include_dirs = []
        extra_compile_args = []
        extra_link_args = []
        sources = ['_ctypes/_ctypes.c',
                   '_ctypes/callbacks.c',
                   '_ctypes/callproc.c',
                   '_ctypes/stgdict.c',
                   '_ctypes/cfield.c']

        if MACOS:
            sources.append('_ctypes/malloc_closure.c')
            extra_compile_args.append('-DUSING_MALLOC_CLOSURE_DOT_C=1')
            extra_compile_args.append('-DMACOSX')
            include_dirs.append('_ctypes/darwin')

        elif HOST_PLATFORM == 'sunos5':
            # XXX This shouldn't be necessary; it appears that some
            # of the assembler code is non-PIC (i.e. it has relocations
            # when it shouldn't. The proper fix would be to rewrite
            # the assembler code to be PIC.
            # This only works with GCC; the Sun compiler likely refuses
            # this option. If you want to compile ctypes with the Sun
            # compiler, please research a proper solution, instead of
            # finding some -z option for the Sun compiler.
            extra_link_args.append('-mimpure-text')

        ext = Extension('_ctypes',
                        include_dirs=include_dirs,
                        extra_compile_args=extra_compile_args,
                        extra_link_args=extra_link_args,
                        libraries=[],
                        sources=sources)
        self.add(ext)
        # function my_sqrt() needs libm for sqrt()
        self.addext(Extension('_ctypes_test', ['_ctypes/_ctypes_test.c']))

        ffi_inc = sysconfig.get_config_var("LIBFFI_INCLUDEDIR")
        ffi_lib = None

        ffi_inc_dirs = self.inc_dirs.copy()
        if MACOS:
            ffi_in_sdk = os.path.join(macosx_sdk_root(), "usr/include/ffi")

            if not ffi_inc:
                if os.path.exists(ffi_in_sdk):
                    ext.extra_compile_args.append("-DUSING_APPLE_OS_LIBFFI=1")
                    ffi_inc = ffi_in_sdk
                    ffi_lib = 'ffi'
                else:
                    # OS X 10.5 comes with libffi.dylib; the include files are
                    # in /usr/include/ffi
                    ffi_inc_dirs.append('/usr/include/ffi')

        if not ffi_inc:
            found = find_file('ffi.h', [], ffi_inc_dirs)
            if found:
                ffi_inc = found[0]
        if ffi_inc:
            ffi_h = ffi_inc + '/ffi.h'
            if not os.path.exists(ffi_h):
                ffi_inc = None
                print('Header file {} does not exist'.format(ffi_h))
        if ffi_lib is None and ffi_inc:
            for lib_name in ('ffi', 'ffi_pic'):
                if (self.compiler.find_library_file(self.lib_dirs, lib_name)):
                    ffi_lib = lib_name
                    break

        if ffi_inc and ffi_lib:
            ffi_headers = glob(os.path.join(ffi_inc, '*.h'))
            if grep_headers_for('ffi_prep_cif_var', ffi_headers):
                ext.extra_compile_args.append("-DHAVE_FFI_PREP_CIF_VAR=1")
            if grep_headers_for('ffi_prep_closure_loc', ffi_headers):
                ext.extra_compile_args.append("-DHAVE_FFI_PREP_CLOSURE_LOC=1")
            if grep_headers_for('ffi_closure_alloc', ffi_headers):
                ext.extra_compile_args.append("-DHAVE_FFI_CLOSURE_ALLOC=1")

            ext.include_dirs.append(ffi_inc)
            ext.libraries.append(ffi_lib)
            self.use_system_libffi = True

        if sysconfig.get_config_var('HAVE_LIBDL'):
            # for dlopen, see bpo-32647
            ext.libraries.append('dl')

    def detect_decimal(self):
        # Stefan Krah's _decimal module
        self.addext(
            Extension(
                '_decimal',
                ['_decimal/_decimal.c'],
                # Uncomment for extra functionality:
                # define_macros=[('EXTRA_FUNCTIONALITY', 1)]
            )
        )

    def detect_openssl_hashlib(self):
        self.addext(Extension('_ssl', ['_ssl.c']))
        self.addext(Extension('_hashlib', ['_hashopenssl.c']))

    def detect_hash_builtins(self):
        # By default we always compile these even when OpenSSL is available
        # (issue #14693). It's harmless and the object code is tiny
        # (40-50 KiB per module, only loaded when actually used).  Modules can
        # be disabled via the --with-builtin-hashlib-hashes configure flag.

        self.addext(Extension('_md5', ['md5module.c']))
        self.addext(Extension('_sha1', ['sha1module.c']))
        self.addext(Extension('_sha256', ['sha256module.c']))
        self.addext(Extension('_sha512', ['sha512module.c']))
        self.addext(Extension('_sha3', ['_sha3/sha3module.c']))
        self.addext(Extension('_blake2',
            [
                '_blake2/blake2module.c',
                '_blake2/blake2b_impl.c',
                '_blake2/blake2s_impl.c'
            ]
        ))

    def detect_nis(self):
        self.addext(Extension('nis', ['nismodule.c']))


class PyBuildInstall(install):
    # Suppress the warning about installation into the lib_dynload
    # directory, which is not in sys.path when running Python during
    # installation:
    def initialize_options (self):
        install.initialize_options(self)
        self.warn_dir=0

    # Customize subcommands to not install an egg-info file for Python
    sub_commands = [('install_lib', install.has_lib),
                    ('install_headers', install.has_headers),
                    ('install_scripts', install.has_scripts),
                    ('install_data', install.has_data)]


class PyBuildInstallLib(install_lib):
    # Do exactly what install_lib does but make sure correct access modes get
    # set on installed directories and files. All installed files with get
    # mode 644 unless they are a shared library in which case they will get
    # mode 755. All installed directories will get mode 755.

    # this is works for EXT_SUFFIX too, which ends with SHLIB_SUFFIX
    shlib_suffix = sysconfig.get_config_var("SHLIB_SUFFIX")

    def install(self):
        outfiles = install_lib.install(self)
        self.set_file_modes(outfiles, 0o644, 0o755)
        self.set_dir_modes(self.install_dir, 0o755)
        return outfiles

    def set_file_modes(self, files, defaultMode, sharedLibMode):
        if not files: return

        for filename in files:
            if os.path.islink(filename): continue
            mode = defaultMode
            if filename.endswith(self.shlib_suffix): mode = sharedLibMode
            log.info("changing mode of %s to %o", filename, mode)
            if not self.dry_run: os.chmod(filename, mode)

    def set_dir_modes(self, dirname, mode):
        for dirpath, dirnames, fnames in os.walk(dirname):
            if os.path.islink(dirpath):
                continue
            log.info("changing mode of %s to %o", dirpath, mode)
            if not self.dry_run: os.chmod(dirpath, mode)


class PyBuildScripts(build_scripts):
    def copy_scripts(self):
        outfiles, updated_files = build_scripts.copy_scripts(self)
        fullversion = '-{0[0]}.{0[1]}'.format(sys.version_info)
        minoronly = '.{0[1]}'.format(sys.version_info)
        newoutfiles = []
        newupdated_files = []
        for filename in outfiles:
            if filename.endswith('2to3'):
                newfilename = filename + fullversion
            else:
                newfilename = filename + minoronly
            log.info(f'renaming {filename} to {newfilename}')
            os.rename(filename, newfilename)
            newoutfiles.append(newfilename)
            if filename in updated_files:
                newupdated_files.append(newfilename)
        return newoutfiles, newupdated_files


def main():
    global LIST_MODULE_NAMES

    if "--list-module-names" in sys.argv:
        LIST_MODULE_NAMES = True
        sys.argv.remove("--list-module-names")

    set_compiler_flags('CFLAGS', 'PY_CFLAGS_NODIST')
    set_compiler_flags('LDFLAGS', 'PY_LDFLAGS_NODIST')

    class DummyProcess:
        """Hack for parallel build"""
        ProcessPoolExecutor = None

    sys.modules['concurrent.futures.process'] = DummyProcess
    validate_tzpath()

    # turn off warnings when deprecated modules are imported
    import warnings
    warnings.filterwarnings("ignore",category=DeprecationWarning)
    setup(# PyPI Metadata (PEP 301)
          name = "Python",
          version = sys.version.split()[0],
          url = "https://www.python.org/%d.%d" % sys.version_info[:2],
          maintainer = "Guido van Rossum and the Python community",
          maintainer_email = "python-dev@python.org",
          description = "A high-level object-oriented programming language",
          long_description = SUMMARY.strip(),
          license = "PSF license",
          classifiers = [x for x in CLASSIFIERS.split("\n") if x],
          platforms = ["Many"],

          # Build info
          cmdclass = {'build_ext': PyBuildExt,
                      'build_scripts': PyBuildScripts,
                      'install': PyBuildInstall,
                      'install_lib': PyBuildInstallLib},
          # A dummy module is defined here, because build_ext won't be
          # called unless there's at least one extension module defined.
          ext_modules=[Extension('_dummy', ['_dummy.c'])],

          # If you change the scripts installed here, you also need to
          # check the PyBuildScripts command above, and change the links
          # created by the bininstall target in Makefile.pre.in
          scripts = ["Tools/scripts/pydoc3", "Tools/scripts/idle3",
                     "Tools/scripts/2to3"]
        )

# --install-platlib
if __name__ == '__main__':
    main()
