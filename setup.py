# Autodetecting setup.py script for building the Python extensions

import argparse
import importlib._bootstrap
import importlib.machinery
import importlib.util
import os
import re
import sys
import sysconfig
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


from distutils import log
from distutils.command.build_ext import build_ext
from distutils.command.build_scripts import build_scripts
from distutils.command.install import install
from distutils.command.install_lib import install_lib
from distutils.core import Extension, setup
from distutils.errors import CCompilerError, DistutilsError
from distutils.spawn import find_executable


# Compile extensions used to test Python?
TEST_EXTENSIONS = (sysconfig.get_config_var('TEST_MODULES') == 'yes')

# This global variable is used to hold the list of modules to be disabled.
DISABLED_MODULE_LIST = []

# --list-module-names option used by Tools/scripts/generate_module_names.py
LIST_MODULE_NAMES = False


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


def macosx_sdk_specified():
    """Returns true if an SDK was explicitly configured.

    True if an SDK was selected at configure time, either by specifying
    --enable-universalsdk=(something other than no or /) or by adding a
    -isysroot option to CFLAGS.  In some cases, like when making
    decisions about macOS Tk framework paths, we need to be able to
    know whether the user explicitly asked to build with an SDK versus
    the implicit use of an SDK when header files are no longer
    installed on a running system by the Command Line Tools.
    """
    global MACOS_SDK_SPECIFIED

    # If already called, return cached result.
    if MACOS_SDK_SPECIFIED:
        return MACOS_SDK_SPECIFIED

    # Find the sdk root and set MACOS_SDK_SPECIFIED
    macosx_sdk_root()
    return MACOS_SDK_SPECIFIED


def is_macosx_sdk_path(path):
    """
    Returns True if 'path' can be located in an OSX SDK
    """
    return ( (path.startswith('/usr/') and not path.startswith('/usr/local'))
                or path.startswith('/System/')
                or path.startswith('/Library/') )


def grep_headers_for(function, headers):
    for header in headers:
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
    for dir in std_dirs:
        f = os.path.join(dir, filename)

        if MACOS and is_macosx_sdk_path(dir):
            f = os.path.join(sysroot, dir[1:], filename)

        if os.path.exists(f): return []

    # Check the additional directories
    for dir in paths:
        f = os.path.join(dir, filename)

        if MACOS and is_macosx_sdk_path(dir):
            f = os.path.join(sysroot, dir[1:], filename)

        if os.path.exists(f):
            return [dir]

    # Not found anywhere
    return None


def find_library_file(compiler, libname, std_dirs, paths):
    result = compiler.find_library_file(std_dirs + paths, libname)
    if result is None:
        return None

    if MACOS:
        sysroot = macosx_sdk_root()

    # Check whether the found file is in one of the standard directories
    dirname = os.path.dirname(result)
    for p in std_dirs:
        # Ensure path doesn't end with path separator
        p = p.rstrip(os.sep)

        if MACOS and is_macosx_sdk_path(p):
            # Note that, as of Xcode 7, Apple SDKs may contain textual stub
            # libraries with .tbd extensions rather than the normal .dylib
            # shared libraries installed in /.  The Apple compiler tool
            # chain handles this transparently but it can cause problems
            # for programs that are being built with an SDK and searching
            # for specific libraries.  Distutils find_library_file() now
            # knows to also search for and return .tbd files.  But callers
            # of find_library_file need to keep in mind that the base filename
            # of the returned SDK library file might have a different extension
            # from that of the library file installed on the running system,
            # for example:
            #   /Applications/Xcode.app/Contents/Developer/Platforms/
            #       MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/
            #       usr/lib/libedit.tbd
            # vs
            #   /usr/lib/libedit.dylib
            if os.path.join(sysroot, p[1:]) == dirname:
                return [ ]

        if p == dirname:
            return [ ]

    # Otherwise, it must have been in one of the additional directories,
    # so we have to figure out which one.
    for p in paths:
        # Ensure path doesn't end with path separator
        p = p.rstrip(os.sep)

        if MACOS and is_macosx_sdk_path(p):
            if os.path.join(sysroot, p[1:]) == dirname:
                return [ p ]

        if p == dirname:
            return [p]
    else:
        assert False, "Internal error: Path not found in std_dirs or paths"

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
    list = find_file(module, [], dirlist)
    if not list:
        return module
    if len(list) > 1:
        log.info("WARNING: multiple copies of %s found", module)
    return os.path.join(list[0], module)


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
        moddirlist = [os.path.join(self.srcdir, 'Modules')]

        # Fix up the paths for scripts, too
        self.distribution.scripts = [os.path.join(self.srcdir, filename)
                                     for filename in self.distribution.scripts]

        # Python header files
        headers = [sysconfig.get_config_h_filename()]
        headers += glob(os.path.join(escape(sysconfig.get_path('include')), "*.h"))

        for ext in self.extensions:
            ext.sources = [ find_module_file(filename, moddirlist)
                            for filename in ext.sources ]
            if ext.depends is not None:
                ext.depends = [find_module_file(filename, moddirlist)
                               for filename in ext.depends]
            else:
                ext.depends = []
            # re-compile extensions if a header file has been changed
            ext.depends.extend(headers)

    def remove_configured_extensions(self):
        # The sysconfig variables built by makesetup that list the already
        # built modules and the disabled modules as configured by the Setup
        # files.
        sysconf_built = sysconfig.get_config_var('MODBUILT_NAMES').split()
        sysconf_dis = sysconfig.get_config_var('MODDISABLED_NAMES').split()

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
                fullpath = self.get_ext_fullpath(ext.name)
                if os.path.exists(fullpath):
                    os.unlink(fullpath)

        return (mods_built, mods_disabled)

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

        # Detect which modules should be compiled
        self.detect_modules()

        if not LIST_MODULE_NAMES:
            self.remove_disabled()

        self.update_sources_depends()
        mods_built, mods_disabled = self.remove_configured_extensions()
        self.set_compiler_executables()

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
            print("Python build finished successfully!")
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
            print("Python requires an OpenSSL 1.0.2 or 1.1 compatible "
                  "libssl with X509_VERIFY_PARAM_set1_host().")
            print("LibreSSL 2.6.4 and earlier do not provide the necessary "
                  "APIs, https://github.com/libressl-portable/portable/issues/381")
            if sysconfig.get_config_var("OPENSSL_LDFLAGS"):
                print("Custom linker flags may require --with-openssl-rpath=auto")
            print()

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
        cc = sysconfig.get_config_var('CC')
        tmpfile = os.path.join(self.build_temp, 'multiarch')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        ret = run_command(
            '%s -print-multiarch > %s 2> /dev/null' % (cc, tmpfile))
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

        cc = sysconfig.get_config_var('CC')
        tmpfile = os.path.join(self.build_temp, 'wrccpaths')
        os.makedirs(self.build_temp, exist_ok=True)
        try:
            ret = run_command('%s --print-search-dirs >%s' % (cc, tmpfile))
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
        cc = sysconfig.get_config_var('CC')
        tmpfile = os.path.join(self.build_temp, 'ccpaths')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        ret = run_command('%s -E -v - </dev/null 2>%s 1>/dev/null' % (cc, tmpfile))
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
        self.add(Extension('array', ['arraymodule.c']))

        # Context Variables
        self.add(Extension('_contextvars', ['_contextvarsmodule.c']))

        shared_math = 'Modules/_math.o'

        # math library functions, e.g. sin()
        self.add(Extension('math',  ['mathmodule.c'],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE'],
                           extra_objects=[shared_math],
                           depends=['_math.h', shared_math],
                           libraries=['m']))

        # complex math library functions
        self.add(Extension('cmath', ['cmathmodule.c'],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE'],
                           extra_objects=[shared_math],
                           depends=['_math.h', shared_math],
                           libraries=['m']))

        # time libraries: librt may be needed for clock_gettime()
        time_libs = []
        lib = sysconfig.get_config_var('TIMEMODULE_LIB')
        if lib:
            time_libs.append(lib)

        # time operations and variables
        self.add(Extension('time', ['timemodule.c'],
                           libraries=time_libs))
        # libm is needed by delta_new() that uses round() and by accum() that
        # uses modf().
        self.add(Extension('_datetime', ['_datetimemodule.c'],
                           libraries=['m'],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))
        # zoneinfo module
        self.add(Extension('_zoneinfo', ['_zoneinfo.c'],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))
        # random number generator implemented in C
        self.add(Extension("_random", ["_randommodule.c"],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))
        # bisect
        self.add(Extension("_bisect", ["_bisectmodule.c"]))
        # heapq
        self.add(Extension("_heapq", ["_heapqmodule.c"],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))
        # C-optimized pickle replacement
        self.add(Extension("_pickle", ["_pickle.c"],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))
        # _json speedups
        self.add(Extension("_json", ["_json.c"],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))

        # profiler (_lsprof is for cProfile.py)
        self.add(Extension('_lsprof', ['_lsprof.c', 'rotatingtree.c']))
        # static Unicode character database
        self.add(Extension('unicodedata', ['unicodedata.c'],
                           depends=['unicodedata_db.h', 'unicodename_db.h'],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))
        # _opcode module
        self.add(Extension('_opcode', ['_opcode.c']))
        # asyncio speedups
        self.add(Extension("_asyncio", ["_asynciomodule.c"],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))
        # _abc speedups
        self.add(Extension("_abc", ["_abc.c"]))
        # _queue module
        self.add(Extension("_queue", ["_queuemodule.c"]))
        # _statistics module
        self.add(Extension("_statistics", ["_statisticsmodule.c"]))

        # Modules with some UNIX dependencies -- on by default:
        # (If you have a really backward UNIX, select and socket may not be
        # supported...)

        # fcntl(2) and ioctl(2)
        libs = []
        if (self.config_h_vars.get('FLOCK_NEEDS_LIBBSD', False)):
            # May be necessary on AIX for flock function
            libs = ['bsd']
        self.add(Extension('fcntl', ['fcntlmodule.c'],
                           libraries=libs))
        # pwd(3)
        self.add(Extension('pwd', ['pwdmodule.c']))
        # grp(3)
        if not VXWORKS:
            self.add(Extension('grp', ['grpmodule.c']))
        # spwd, shadow passwords
        if (self.config_h_vars.get('HAVE_GETSPNAM', False) or
                self.config_h_vars.get('HAVE_GETSPENT', False)):
            self.add(Extension('spwd', ['spwdmodule.c']))
        # AIX has shadow passwords, but access is not via getspent(), etc.
        # module support is not expected so it not 'missing'
        elif not AIX:
            self.missing.append('spwd')

        # select(2); not on ancient System V
        self.add(Extension('select', ['selectmodule.c']))

        # Memory-mapped files (also works on Win32).
        self.add(Extension('mmap', ['mmapmodule.c']))

        # Lance Ellinghaus's syslog module
        # syslog daemon interface
        self.add(Extension('syslog', ['syslogmodule.c']))

        # Python interface to subinterpreter C-API.
        self.add(Extension('_xxsubinterpreters', ['_xxsubinterpretersmodule.c']))

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
        self.add(Extension('audioop', ['audioop.c'],
                           libraries=['m']))

        # CSV files
        self.add(Extension('_csv', ['_csv.c']))

        # POSIX subprocess module helper.
        self.add(Extension('_posixsubprocess', ['_posixsubprocess.c'],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))

    def detect_test_extensions(self):
        # Python C API test module
        self.add(Extension('_testcapi', ['_testcapimodule.c'],
                           depends=['testcapi_long.h']))

        # Python Internal C API test module
        self.add(Extension('_testinternalcapi', ['_testinternalcapi.c'],
                           extra_compile_args=['-DPy_BUILD_CORE_MODULE']))

        # Python PEP-3118 (buffer protocol) test module
        self.add(Extension('_testbuffer', ['_testbuffer.c']))

        # Test loading multiple modules from one compiled file (http://bugs.python.org/issue16421)
        self.add(Extension('_testimportmultiple', ['_testimportmultiple.c']))

        # Test multi-phase extension module init (PEP 489)
        self.add(Extension('_testmultiphase', ['_testmultiphase.c']))

        # Fuzz tests.
        self.add(Extension('_xxtestfuzz',
                           ['_xxtestfuzz/_xxtestfuzz.c',
                            '_xxtestfuzz/fuzzer.c']))

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
            if MACOS and os_release < 9:
                # In every directory on the search path search for a dynamic
                # library and then a static library, instead of first looking
                # for dynamic libraries on the entire path.
                # This way a statically linked custom readline gets picked up
                # before the (possibly broken) dynamic library in /usr/lib.
                readline_extra_link_args = ('-Wl,-search_paths_first',)
            else:
                readline_extra_link_args = ()

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
                               extra_link_args=readline_extra_link_args,
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
                               extra_compile_args=['-DPy_BUILD_CORE_MODULE'],
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
                               extra_compile_args=['-DPy_BUILD_CORE_MODULE'],
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
        # crypt module.
        if VXWORKS:
            # bpo-31904: crypt() function is not provided by VxWorks.
            # DES_crypt() OpenSSL provides is too weak to implement
            # the encryption.
            self.missing.append('_crypt')
            return

        if self.compiler.find_library_file(self.lib_dirs, 'crypt'):
            libs = ['crypt']
        else:
            libs = []

        self.add(Extension('_crypt', ['_cryptmodule.c'], libraries=libs))

    def detect_socket(self):
        # socket(2)
        kwargs = {'depends': ['socketmodule.h']}
        if MACOS:
            # Issue #35569: Expose RFC 3542 socket options.
            kwargs['extra_compile_args'] = ['-D__APPLE_USE_RFC_3542']

        self.add(Extension('_socket', ['socketmodule.c'], **kwargs))

    def detect_dbm_gdbm(self):
        # Modules that provide persistent dictionary-like semantics.  You will
        # probably want to arrange for at least one of them to be available on
        # your machine, though none are defined by default because of library
        # dependencies.  The Python module dbm/__init__.py provides an
        # implementation independent wrapper for these; dbm/dumb.py provides
        # similar functionality (but slower of course) implemented in Python.

        # Sleepycat^WOracle Berkeley DB interface.
        #  http://www.oracle.com/database/berkeley-db/db/index.html
        #
        # This requires the Sleepycat^WOracle DB code. The supported versions
        # are set below.  Visit the URL above to download
        # a release.  Most open source OSes come with one or more
        # versions of BerkeleyDB already installed.

        max_db_ver = (5, 3)
        min_db_ver = (3, 3)
        db_setup_debug = False   # verbose debug prints from this script?

        def allow_db_ver(db_ver):
            """Returns a boolean if the given BerkeleyDB version is acceptable.

            Args:
              db_ver: A tuple of the version to verify.
            """
            if not (min_db_ver <= db_ver <= max_db_ver):
                return False
            return True

        def gen_db_minor_ver_nums(major):
            if major == 4:
                for x in range(max_db_ver[1]+1):
                    if allow_db_ver((4, x)):
                        yield x
            elif major == 3:
                for x in (3,):
                    if allow_db_ver((3, x)):
                        yield x
            else:
                raise ValueError("unknown major BerkeleyDB version", major)

        # construct a list of paths to look for the header file in on
        # top of the normal inc_dirs.
        db_inc_paths = [
            '/usr/include/db4',
            '/usr/local/include/db4',
            '/opt/sfw/include/db4',
            '/usr/include/db3',
            '/usr/local/include/db3',
            '/opt/sfw/include/db3',
            # Fink defaults (http://fink.sourceforge.net/)
            '/sw/include/db4',
            '/sw/include/db3',
        ]
        # 4.x minor number specific paths
        for x in gen_db_minor_ver_nums(4):
            db_inc_paths.append('/usr/include/db4%d' % x)
            db_inc_paths.append('/usr/include/db4.%d' % x)
            db_inc_paths.append('/usr/local/BerkeleyDB.4.%d/include' % x)
            db_inc_paths.append('/usr/local/include/db4%d' % x)
            db_inc_paths.append('/pkg/db-4.%d/include' % x)
            db_inc_paths.append('/opt/db-4.%d/include' % x)
            # MacPorts default (http://www.macports.org/)
            db_inc_paths.append('/opt/local/include/db4%d' % x)
        # 3.x minor number specific paths
        for x in gen_db_minor_ver_nums(3):
            db_inc_paths.append('/usr/include/db3%d' % x)
            db_inc_paths.append('/usr/local/BerkeleyDB.3.%d/include' % x)
            db_inc_paths.append('/usr/local/include/db3%d' % x)
            db_inc_paths.append('/pkg/db-3.%d/include' % x)
            db_inc_paths.append('/opt/db-3.%d/include' % x)

        if CROSS_COMPILING:
            db_inc_paths = []

        # Add some common subdirectories for Sleepycat DB to the list,
        # based on the standard include directories. This way DB3/4 gets
        # picked up when it is installed in a non-standard prefix and
        # the user has added that prefix into inc_dirs.
        std_variants = []
        for dn in self.inc_dirs:
            std_variants.append(os.path.join(dn, 'db3'))
            std_variants.append(os.path.join(dn, 'db4'))
            for x in gen_db_minor_ver_nums(4):
                std_variants.append(os.path.join(dn, "db4%d"%x))
                std_variants.append(os.path.join(dn, "db4.%d"%x))
            for x in gen_db_minor_ver_nums(3):
                std_variants.append(os.path.join(dn, "db3%d"%x))
                std_variants.append(os.path.join(dn, "db3.%d"%x))

        db_inc_paths = std_variants + db_inc_paths
        db_inc_paths = [p for p in db_inc_paths if os.path.exists(p)]

        db_ver_inc_map = {}

        if MACOS:
            sysroot = macosx_sdk_root()

        class db_found(Exception): pass
        try:
            # See whether there is a Sleepycat header in the standard
            # search path.
            for d in self.inc_dirs + db_inc_paths:
                f = os.path.join(d, "db.h")
                if MACOS and is_macosx_sdk_path(d):
                    f = os.path.join(sysroot, d[1:], "db.h")

                if db_setup_debug: print("db: looking for db.h in", f)
                if os.path.exists(f):
                    with open(f, 'rb') as file:
                        f = file.read()
                    m = re.search(br"#define\WDB_VERSION_MAJOR\W(\d+)", f)
                    if m:
                        db_major = int(m.group(1))
                        m = re.search(br"#define\WDB_VERSION_MINOR\W(\d+)", f)
                        db_minor = int(m.group(1))
                        db_ver = (db_major, db_minor)

                        # Avoid 4.6 prior to 4.6.21 due to a BerkeleyDB bug
                        if db_ver == (4, 6):
                            m = re.search(br"#define\WDB_VERSION_PATCH\W(\d+)", f)
                            db_patch = int(m.group(1))
                            if db_patch < 21:
                                print("db.h:", db_ver, "patch", db_patch,
                                      "being ignored (4.6.x must be >= 4.6.21)")
                                continue

                        if ( (db_ver not in db_ver_inc_map) and
                            allow_db_ver(db_ver) ):
                            # save the include directory with the db.h version
                            # (first occurrence only)
                            db_ver_inc_map[db_ver] = d
                            if db_setup_debug:
                                print("db.h: found", db_ver, "in", d)
                        else:
                            # we already found a header for this library version
                            if db_setup_debug: print("db.h: ignoring", d)
                    else:
                        # ignore this header, it didn't contain a version number
                        if db_setup_debug:
                            print("db.h: no version number version in", d)

            db_found_vers = list(db_ver_inc_map.keys())
            db_found_vers.sort()

            while db_found_vers:
                db_ver = db_found_vers.pop()
                db_incdir = db_ver_inc_map[db_ver]

                # check lib directories parallel to the location of the header
                db_dirs_to_check = [
                    db_incdir.replace("include", 'lib64'),
                    db_incdir.replace("include", 'lib'),
                ]

                if not MACOS:
                    db_dirs_to_check = list(filter(os.path.isdir, db_dirs_to_check))

                else:
                    # Same as other branch, but takes OSX SDK into account
                    tmp = []
                    for dn in db_dirs_to_check:
                        if is_macosx_sdk_path(dn):
                            if os.path.isdir(os.path.join(sysroot, dn[1:])):
                                tmp.append(dn)
                        else:
                            if os.path.isdir(dn):
                                tmp.append(dn)
                    db_dirs_to_check = tmp

                    db_dirs_to_check = tmp

                # Look for a version specific db-X.Y before an ambiguous dbX
                # XXX should we -ever- look for a dbX name?  Do any
                # systems really not name their library by version and
                # symlink to more general names?
                for dblib in (('db-%d.%d' % db_ver),
                              ('db%d%d' % db_ver),
                              ('db%d' % db_ver[0])):
                    dblib_file = self.compiler.find_library_file(
                                    db_dirs_to_check + self.lib_dirs, dblib )
                    if dblib_file:
                        dblib_dir = [ os.path.abspath(os.path.dirname(dblib_file)) ]
                        raise db_found
                    else:
                        if db_setup_debug: print("db lib: ", dblib, "not found")

        except db_found:
            if db_setup_debug:
                print("bsddb using BerkeleyDB lib:", db_ver, dblib)
                print("bsddb lib dir:", dblib_dir, " inc dir:", db_incdir)
            dblibs = [dblib]
            # Only add the found library and include directories if they aren't
            # already being searched. This avoids an explicit runtime library
            # dependency.
            if db_incdir in self.inc_dirs:
                db_incs = None
            else:
                db_incs = [db_incdir]
            if dblib_dir[0] in self.lib_dirs:
                dblib_dir = None
        else:
            if db_setup_debug: print("db: no appropriate library found")
            db_incs = None
            dblibs = []
            dblib_dir = None

        dbm_setup_debug = False   # verbose debug prints from this script?
        dbm_order = ['gdbm']
        # The standard Unix dbm module:
        if not CYGWIN:
            config_args = [arg.strip("'")
                           for arg in sysconfig.get_config_var("CONFIG_ARGS").split()]
            dbm_args = [arg for arg in config_args
                        if arg.startswith('--with-dbmliborder=')]
            if dbm_args:
                dbm_order = [arg.split('=')[-1] for arg in dbm_args][-1].split(":")
            else:
                dbm_order = "ndbm:gdbm:bdb".split(":")
            dbmext = None
            for cand in dbm_order:
                if cand == "ndbm":
                    if find_file("ndbm.h", self.inc_dirs, []) is not None:
                        # Some systems have -lndbm, others have -lgdbm_compat,
                        # others don't have either
                        if self.compiler.find_library_file(self.lib_dirs,
                                                               'ndbm'):
                            ndbm_libs = ['ndbm']
                        elif self.compiler.find_library_file(self.lib_dirs,
                                                             'gdbm_compat'):
                            ndbm_libs = ['gdbm_compat']
                        else:
                            ndbm_libs = []
                        if dbm_setup_debug: print("building dbm using ndbm")
                        dbmext = Extension('_dbm', ['_dbmmodule.c'],
                                           define_macros=[
                                               ('HAVE_NDBM_H',None),
                                               ],
                                           libraries=ndbm_libs)
                        break

                elif cand == "gdbm":
                    if self.compiler.find_library_file(self.lib_dirs, 'gdbm'):
                        gdbm_libs = ['gdbm']
                        if self.compiler.find_library_file(self.lib_dirs,
                                                               'gdbm_compat'):
                            gdbm_libs.append('gdbm_compat')
                        if find_file("gdbm/ndbm.h", self.inc_dirs, []) is not None:
                            if dbm_setup_debug: print("building dbm using gdbm")
                            dbmext = Extension(
                                '_dbm', ['_dbmmodule.c'],
                                define_macros=[
                                    ('HAVE_GDBM_NDBM_H', None),
                                    ],
                                libraries = gdbm_libs)
                            break
                        if find_file("gdbm-ndbm.h", self.inc_dirs, []) is not None:
                            if dbm_setup_debug: print("building dbm using gdbm")
                            dbmext = Extension(
                                '_dbm', ['_dbmmodule.c'],
                                define_macros=[
                                    ('HAVE_GDBM_DASH_NDBM_H', None),
                                    ],
                                libraries = gdbm_libs)
                            break
                elif cand == "bdb":
                    if dblibs:
                        if dbm_setup_debug: print("building dbm using bdb")
                        dbmext = Extension('_dbm', ['_dbmmodule.c'],
                                           library_dirs=dblib_dir,
                                           runtime_library_dirs=dblib_dir,
                                           include_dirs=db_incs,
                                           define_macros=[
                                               ('HAVE_BERKDB_H', None),
                                               ('DB_DBM_HSEARCH', None),
                                               ],
                                           libraries=dblibs)
                        break
            if dbmext is not None:
                self.add(dbmext)
            else:
                self.missing.append('_dbm')

        # Anthony Baxter's gdbm module.  GNU dbm(3) will require -lgdbm:
        if ('gdbm' in dbm_order and
            self.compiler.find_library_file(self.lib_dirs, 'gdbm')):
            self.add(Extension('_gdbm', ['_gdbmmodule.c'],
                               libraries=['gdbm']))
        else:
            self.missing.append('_gdbm')

    def detect_sqlite(self):
        # The sqlite interface
        sqlite_setup_debug = False   # verbose debug prints from this script?

        # We hunt for #define SQLITE_VERSION "n.n.n"
        sqlite_incdir = sqlite_libdir = None
        sqlite_inc_paths = [ '/usr/include',
                             '/usr/include/sqlite',
                             '/usr/include/sqlite3',
                             '/usr/local/include',
                             '/usr/local/include/sqlite',
                             '/usr/local/include/sqlite3',
                             ]
        if CROSS_COMPILING:
            sqlite_inc_paths = []
        MIN_SQLITE_VERSION_NUMBER = (3, 7, 15)  # Issue 40810
        MIN_SQLITE_VERSION = ".".join([str(x)
                                    for x in MIN_SQLITE_VERSION_NUMBER])

        # Scan the default include directories before the SQLite specific
        # ones. This allows one to override the copy of sqlite on OSX,
        # where /usr/include contains an old version of sqlite.
        if MACOS:
            sysroot = macosx_sdk_root()

        for d_ in self.inc_dirs + sqlite_inc_paths:
            d = d_
            if MACOS and is_macosx_sdk_path(d):
                d = os.path.join(sysroot, d[1:])

            f = os.path.join(d, "sqlite3.h")
            if os.path.exists(f):
                if sqlite_setup_debug: print("sqlite: found %s"%f)
                with open(f) as file:
                    incf = file.read()
                m = re.search(
                    r'\s*.*#\s*.*define\s.*SQLITE_VERSION\W*"([\d\.]*)"', incf)
                if m:
                    sqlite_version = m.group(1)
                    sqlite_version_tuple = tuple([int(x)
                                        for x in sqlite_version.split(".")])
                    if sqlite_version_tuple >= MIN_SQLITE_VERSION_NUMBER:
                        # we win!
                        if sqlite_setup_debug:
                            print("%s/sqlite3.h: version %s"%(d, sqlite_version))
                        sqlite_incdir = d
                        break
                    else:
                        if sqlite_setup_debug:
                            print("%s: version %s is too old, need >= %s"%(d,
                                        sqlite_version, MIN_SQLITE_VERSION))
                elif sqlite_setup_debug:
                    print("sqlite: %s had no SQLITE_VERSION"%(f,))

        if sqlite_incdir:
            sqlite_dirs_to_check = [
                os.path.join(sqlite_incdir, '..', 'lib64'),
                os.path.join(sqlite_incdir, '..', 'lib'),
                os.path.join(sqlite_incdir, '..', '..', 'lib64'),
                os.path.join(sqlite_incdir, '..', '..', 'lib'),
            ]
            sqlite_libfile = self.compiler.find_library_file(
                                sqlite_dirs_to_check + self.lib_dirs, 'sqlite3')
            if sqlite_libfile:
                sqlite_libdir = [os.path.abspath(os.path.dirname(sqlite_libfile))]

        if sqlite_incdir and sqlite_libdir:
            sqlite_srcs = ['_sqlite/cache.c',
                '_sqlite/connection.c',
                '_sqlite/cursor.c',
                '_sqlite/microprotocols.c',
                '_sqlite/module.c',
                '_sqlite/prepare_protocol.c',
                '_sqlite/row.c',
                '_sqlite/statement.c',
                '_sqlite/util.c', ]
            sqlite_defines = []

            # Enable support for loadable extensions in the sqlite3 module
            # if --enable-loadable-sqlite-extensions configure option is used.
            if '--enable-loadable-sqlite-extensions' not in sysconfig.get_config_var("CONFIG_ARGS"):
                sqlite_defines.append(("SQLITE_OMIT_LOAD_EXTENSION", "1"))

            if MACOS:
                # In every directory on the search path search for a dynamic
                # library and then a static library, instead of first looking
                # for dynamic libraries on the entire path.
                # This way a statically linked custom sqlite gets picked up
                # before the dynamic library in /usr/lib.
                sqlite_extra_link_args = ('-Wl,-search_paths_first',)
            else:
                sqlite_extra_link_args = ()

            include_dirs = ["Modules/_sqlite"]
            # Only include the directory where sqlite was found if it does
            # not already exist in set include directories, otherwise you
            # can end up with a bad search path order.
            if sqlite_incdir not in self.compiler.include_dirs:
                include_dirs.append(sqlite_incdir)
            # avoid a runtime library path for a system library dir
            if sqlite_libdir and sqlite_libdir[0] in self.lib_dirs:
                sqlite_libdir = None
            self.add(Extension('_sqlite3', sqlite_srcs,
                               define_macros=sqlite_defines,
                               include_dirs=include_dirs,
                               library_dirs=sqlite_libdir,
                               extra_link_args=sqlite_extra_link_args,
                               libraries=["sqlite3",]))
        else:
            self.missing.append('_sqlite3')

    def detect_platform_specific_exts(self):
        # Unix-only modules
        if not MS_WINDOWS:
            if not VXWORKS:
                # Steen Lumholt's termios module
                self.add(Extension('termios', ['termios.c']))
                # Jeremy Hylton's rlimit interface
            self.add(Extension('resource', ['resource.c']))
        else:
            self.missing.extend(['resource', 'termios'])

        # Platform-specific libraries
        if HOST_PLATFORM.startswith(('linux', 'freebsd', 'gnukfreebsd')):
            self.add(Extension('ossaudiodev', ['ossaudiodev.c']))
        elif not AIX:
            self.missing.append('ossaudiodev')

        if MACOS:
            self.add(Extension('_scproxy', ['_scproxy.c'],
                               extra_link_args=[
                                   '-framework', 'SystemConfiguration',
                                   '-framework', 'CoreFoundation']))

    def detect_compress_exts(self):
        # Andrew Kuchling's zlib module.  Note that some versions of zlib
        # 1.1.3 have security problems.  See CERT Advisory CA-2002-07:
        # http://www.cert.org/advisories/CA-2002-07.html
        #
        # zlib 1.1.4 is fixed, but at least one vendor (RedHat) has decided to
        # patch its zlib 1.1.3 package instead of upgrading to 1.1.4.  For
        # now, we still accept 1.1.3, because we think it's difficult to
        # exploit this in Python, and we'd rather make it RedHat's problem
        # than our problem <wink>.
        #
        # You can upgrade zlib to version 1.1.4 yourself by going to
        # http://www.gzip.org/zlib/
        zlib_inc = find_file('zlib.h', [], self.inc_dirs)
        have_zlib = False
        if zlib_inc is not None:
            zlib_h = zlib_inc[0] + '/zlib.h'
            version = '"0.0.0"'
            version_req = '"1.1.3"'
            if MACOS and is_macosx_sdk_path(zlib_h):
                zlib_h = os.path.join(macosx_sdk_root(), zlib_h[1:])
            with open(zlib_h) as fp:
                while 1:
                    line = fp.readline()
                    if not line:
                        break
                    if line.startswith('#define ZLIB_VERSION'):
                        version = line.split()[2]
                        break
            if version >= version_req:
                if (self.compiler.find_library_file(self.lib_dirs, 'z')):
                    if MACOS:
                        zlib_extra_link_args = ('-Wl,-search_paths_first',)
                    else:
                        zlib_extra_link_args = ()
                    self.add(Extension('zlib', ['zlibmodule.c'],
                                       libraries=['z'],
                                       extra_link_args=zlib_extra_link_args))
                    have_zlib = True
                else:
                    self.missing.append('zlib')
            else:
                self.missing.append('zlib')
        else:
            self.missing.append('zlib')

        # Helper module for various ascii-encoders.  Uses zlib for an optimized
        # crc32 if we have it.  Otherwise binascii uses its own.
        if have_zlib:
            extra_compile_args = ['-DUSE_ZLIB_CRC32']
            libraries = ['z']
            extra_link_args = zlib_extra_link_args
        else:
            extra_compile_args = []
            libraries = []
            extra_link_args = []
        self.add(Extension('binascii', ['binascii.c'],
                           extra_compile_args=extra_compile_args,
                           libraries=libraries,
                           extra_link_args=extra_link_args))

        # Gustavo Niemeyer's bz2 module.
        if (self.compiler.find_library_file(self.lib_dirs, 'bz2')):
            if MACOS:
                bz2_extra_link_args = ('-Wl,-search_paths_first',)
            else:
                bz2_extra_link_args = ()
            self.add(Extension('_bz2', ['_bz2module.c'],
                               libraries=['bz2'],
                               extra_link_args=bz2_extra_link_args))
        else:
            self.missing.append('_bz2')

        # LZMA compression support.
        if self.compiler.find_library_file(self.lib_dirs, 'lzma'):
            self.add(Extension('_lzma', ['_lzmamodule.c'],
                               libraries=['lzma']))
        else:
            self.missing.append('_lzma')

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
        if '--with-system-expat' in sysconfig.get_config_var("CONFIG_ARGS"):
            expat_inc = []
            define_macros = []
            extra_compile_args = []
            expat_lib = ['expat']
            expat_sources = []
            expat_depends = []
        else:
            expat_inc = [os.path.join(self.srcdir, 'Modules', 'expat')]
            define_macros = [
                ('HAVE_EXPAT_CONFIG_H', '1'),
                # bpo-30947: Python uses best available entropy sources to
                # call XML_SetHashSalt(), expat entropy sources are not needed
                ('XML_POOR_ENTROPY', '1'),
            ]
            extra_compile_args = []
            expat_lib = []
            expat_sources = ['expat/xmlparse.c',
                             'expat/xmlrole.c',
                             'expat/xmltok.c']
            expat_depends = ['expat/ascii.h',
                             'expat/asciitab.h',
                             'expat/expat.h',
                             'expat/expat_config.h',
                             'expat/expat_external.h',
                             'expat/internal.h',
                             'expat/latin1tab.h',
                             'expat/utf8tab.h',
                             'expat/xmlrole.h',
                             'expat/xmltok.h',
                             'expat/xmltok_impl.h'
                             ]

            cc = sysconfig.get_config_var('CC').split()[0]
            ret = run_command(
                      '"%s" -Werror -Wno-unreachable-code -E -xc /dev/null >/dev/null 2>&1' % cc)
            if ret == 0:
                extra_compile_args.append('-Wno-unreachable-code')

        self.add(Extension('pyexpat',
                           define_macros=define_macros,
                           extra_compile_args=extra_compile_args,
                           include_dirs=expat_inc,
                           libraries=expat_lib,
                           sources=['pyexpat.c'] + expat_sources,
                           depends=expat_depends))

        # Fredrik Lundh's cElementTree module.  Note that this also
        # uses expat (via the CAPI hook in pyexpat).

        if os.path.isfile(os.path.join(self.srcdir, 'Modules', '_elementtree.c')):
            define_macros.append(('USE_PYEXPAT_CAPI', None))
            self.add(Extension('_elementtree',
                               define_macros=define_macros,
                               include_dirs=expat_inc,
                               libraries=expat_lib,
                               sources=['_elementtree.c'],
                               depends=['pyexpat.c', *expat_sources,
                                        *expat_depends]))
        else:
            self.missing.append('_elementtree')

    def detect_multibytecodecs(self):
        # Hye-Shik Chang's CJKCodecs modules.
        self.add(Extension('_multibytecodec',
                           ['cjkcodecs/multibytecodec.c']))
        for loc in ('kr', 'jp', 'cn', 'tw', 'hk', 'iso2022'):
            self.add(Extension('_codecs_%s' % loc,
                               ['cjkcodecs/_codecs_%s.c' % loc]))

    def detect_multiprocessing(self):
        # Richard Oudkerk's multiprocessing module
        if MS_WINDOWS:
            multiprocessing_srcs = ['_multiprocessing/multiprocessing.c',
                                    '_multiprocessing/semaphore.c']
        else:
            multiprocessing_srcs = ['_multiprocessing/multiprocessing.c']
            if (sysconfig.get_config_var('HAVE_SEM_OPEN') and not
                sysconfig.get_config_var('POSIX_SEMAPHORES_NOT_ENABLED')):
                multiprocessing_srcs.append('_multiprocessing/semaphore.c')
        self.add(Extension('_multiprocessing', multiprocessing_srcs,
                           include_dirs=["Modules/_multiprocessing"]))

        if (not MS_WINDOWS and
           sysconfig.get_config_var('HAVE_SHM_OPEN') and
           sysconfig.get_config_var('HAVE_SHM_UNLINK')):
            posixshmem_srcs = ['_multiprocessing/posixshmem.c']
            libs = []
            if sysconfig.get_config_var('SHM_NEEDS_LIBRT'):
                # need to link with librt to get shm_open()
                libs.append('rt')
            self.add(Extension('_posixshmem', posixshmem_srcs,
                               define_macros={},
                               libraries=libs,
                               include_dirs=["Modules/_multiprocessing"]))
        else:
            self.missing.append('_posixshmem')

    def detect_uuid(self):
        # Build the _uuid module if possible
        uuid_incs = find_file("uuid.h", self.inc_dirs, ["/usr/include/uuid"])
        if uuid_incs is not None:
            if self.compiler.find_library_file(self.lib_dirs, 'uuid'):
                uuid_libs = ['uuid']
            else:
                uuid_libs = []
            self.add(Extension('_uuid', ['_uuidmodule.c'],
                               libraries=uuid_libs,
                               include_dirs=uuid_incs))
        else:
            self.missing.append('_uuid')

    def detect_modules(self):
        self.configure_compiler()
        self.init_inc_lib_dirs()

        self.detect_simple_extensions()
        if TEST_EXTENSIONS:
            self.detect_test_extensions()
        self.detect_readline_curses()
        self.detect_crypt()
        self.detect_socket()
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
        if not self.detect_tkinter():
            self.missing.append('_tkinter')
        self.detect_uuid()

##         # Uncomment these lines if you want to play with xxmodule.c
##         self.add(Extension('xx', ['xxmodule.c']))

        if 'd' not in sysconfig.get_config_var('ABIFLAGS'):
            # Non-debug mode: Build xxlimited with limited API
            self.add(Extension('xxlimited', ['xxlimited.c'],
                               define_macros=[('Py_LIMITED_API', '0x03100000')]))
            self.add(Extension('xxlimited_35', ['xxlimited_35.c'],
                               define_macros=[('Py_LIMITED_API', '0x03050000')]))
        else:
            # Debug mode: Build xxlimited with the full API
            # (which is compatible with the limited one)
            self.add(Extension('xxlimited', ['xxlimited.c']))
            self.add(Extension('xxlimited_35', ['xxlimited_35.c']))

    def detect_tkinter_fromenv(self):
        # Build _tkinter using the Tcl/Tk locations specified by
        # the _TCLTK_INCLUDES and _TCLTK_LIBS environment variables.
        # This method is meant to be invoked by detect_tkinter().
        #
        # The variables can be set via one of the following ways.
        #
        # - Automatically, at configuration time, by using pkg-config.
        #   The tool is called by the configure script.
        #   Additional pkg-config configuration paths can be set via the
        #   PKG_CONFIG_PATH environment variable.
        #
        #     PKG_CONFIG_PATH=".../lib/pkgconfig" ./configure ...
        #
        # - Explicitly, at configuration time by setting both
        #   --with-tcltk-includes and --with-tcltk-libs.
        #
        #     ./configure ... \
        #     --with-tcltk-includes="-I/path/to/tclincludes \
        #                            -I/path/to/tkincludes"
        #     --with-tcltk-libs="-L/path/to/tcllibs -ltclm.n \
        #                        -L/path/to/tklibs -ltkm.n"
        #
        #  - Explicitly, at compile time, by passing TCLTK_INCLUDES and
        #    TCLTK_LIBS to the make target.
        #    This will override any configuration-time option.
        #
        #      make TCLTK_INCLUDES="..." TCLTK_LIBS="..."
        #
        # This can be useful for building and testing tkinter with multiple
        # versions of Tcl/Tk.  Note that a build of Tk depends on a particular
        # build of Tcl so you need to specify both arguments and use care when
        # overriding.

        # The _TCLTK variables are created in the Makefile sharedmods target.
        tcltk_includes = os.environ.get('_TCLTK_INCLUDES')
        tcltk_libs = os.environ.get('_TCLTK_LIBS')
        if not (tcltk_includes and tcltk_libs):
            # Resume default configuration search.
            return False

        extra_compile_args = tcltk_includes.split()
        extra_link_args = tcltk_libs.split()
        self.add(Extension('_tkinter', ['_tkinter.c', 'tkappinit.c'],
                           define_macros=[('WITH_APPINIT', 1)],
                           extra_compile_args = extra_compile_args,
                           extra_link_args = extra_link_args))
        return True

    def detect_tkinter_darwin(self):
        # Build default _tkinter on macOS using Tcl and Tk frameworks.
        # This method is meant to be invoked by detect_tkinter().
        #
        # The macOS native Tk (AKA Aqua Tk) and Tcl are most commonly
        # built and installed as macOS framework bundles.  However,
        # for several reasons, we cannot take full advantage of the
        # Apple-supplied compiler chain's -framework options here.
        # Instead, we need to find and pass to the compiler the
        # absolute paths of the Tcl and Tk headers files we want to use
        # and the absolute path to the directory containing the Tcl
        # and Tk frameworks for linking.
        #
        # We want to handle here two common use cases on macOS:
        # 1. Build and link with system-wide third-party or user-built
        #    Tcl and Tk frameworks installed in /Library/Frameworks.
        # 2. Build and link using a user-specified macOS SDK so that the
        #    built Python can be exported to other systems.  In this case,
        #    search only the SDK's /Library/Frameworks (normally empty)
        #    and /System/Library/Frameworks.
        #
        # Any other use cases are handled either by detect_tkinter_fromenv(),
        # or detect_tkinter(). The former handles non-standard locations of
        # Tcl/Tk, defined via the _TCLTK_INCLUDES and _TCLTK_LIBS environment
        # variables. The latter handles any Tcl/Tk versions installed in
        # standard Unix directories.
        #
        # It would be desirable to also handle here the case where
        # you want to build and link with a framework build of Tcl and Tk
        # that is not in /Library/Frameworks, say, in your private
        # $HOME/Library/Frameworks directory or elsewhere. It turns
        # out to be difficult to make that work automatically here
        # without bringing into play more tools and magic. That case
        # can be handled using a recipe with the right arguments
        # to detect_tkinter_fromenv().
        #
        # Note also that the fallback case here is to try to use the
        # Apple-supplied Tcl and Tk frameworks in /System/Library but
        # be forewarned that they are deprecated by Apple and typically
        # out-of-date and buggy; their use should be avoided if at
        # all possible by installing a newer version of Tcl and Tk in
        # /Library/Frameworks before building Python without
        # an explicit SDK or by configuring build arguments explicitly.

        from os.path import join, exists

        sysroot = macosx_sdk_root() # path to the SDK or '/'

        if macosx_sdk_specified():
            # Use case #2: an SDK other than '/' was specified.
            # Only search there.
            framework_dirs = [
                join(sysroot, 'Library', 'Frameworks'),
                join(sysroot, 'System', 'Library', 'Frameworks'),
            ]
        else:
            # Use case #1: no explicit SDK selected.
            # Search the local system-wide /Library/Frameworks,
            # not the one in the default SDK, otherwise fall back to
            # /System/Library/Frameworks whose header files may be in
            # the default SDK or, on older systems, actually installed.
            framework_dirs = [
                join('/', 'Library', 'Frameworks'),
                join(sysroot, 'System', 'Library', 'Frameworks'),
            ]

        # Find the directory that contains the Tcl.framework and
        # Tk.framework bundles.
        for F in framework_dirs:
            # both Tcl.framework and Tk.framework should be present
            for fw in 'Tcl', 'Tk':
                if not exists(join(F, fw + '.framework')):
                    break
            else:
                # ok, F is now directory with both frameworks. Continue
                # building
                break
        else:
            # Tk and Tcl frameworks not found. Normal "unix" tkinter search
            # will now resume.
            return False

        include_dirs = [
            join(F, fw + '.framework', H)
            for fw in ('Tcl', 'Tk')
            for H in ('Headers',)
        ]

        # Add the base framework directory as well
        compile_args = ['-F', F]

        # Do not build tkinter for archs that this Tk was not built with.
        cflags = sysconfig.get_config_vars('CFLAGS')[0]
        archs = re.findall(r'-arch\s+(\w+)', cflags)

        tmpfile = os.path.join(self.build_temp, 'tk.arch')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        run_command(
            "file {}/Tk.framework/Tk | grep 'for architecture' > {}".format(F, tmpfile)
        )
        with open(tmpfile) as fp:
            detected_archs = []
            for ln in fp:
                a = ln.split()[-1]
                if a in archs:
                    detected_archs.append(ln.split()[-1])
        os.unlink(tmpfile)

        arch_args = []
        for a in detected_archs:
            arch_args.append('-arch')
            arch_args.append(a)

        compile_args += arch_args
        link_args = [','.join(['-Wl', '-F', F, '-framework', 'Tcl', '-framework', 'Tk']), *arch_args]

        # The X11/xlib.h file bundled in the Tk sources can cause function
        # prototype warnings from the compiler. Since we cannot easily fix
        # that, suppress the warnings here instead.
        if '-Wstrict-prototypes' in cflags.split():
            compile_args.append('-Wno-strict-prototypes')

        self.add(Extension('_tkinter', ['_tkinter.c', 'tkappinit.c'],
                           define_macros=[('WITH_APPINIT', 1)],
                           include_dirs=include_dirs,
                           libraries=[],
                           extra_compile_args=compile_args,
                           extra_link_args=link_args))
        return True

    def detect_tkinter(self):
        # The _tkinter module.
        #
        # Detection of Tcl/Tk is attempted in the following order:
        #   - Through environment variables.
        #   - Platform specific detection of Tcl/Tk (currently only macOS).
        #   - Search of various standard Unix header/library paths.
        #
        # Detection stops at the first successful method.

        # Check for Tcl and Tk at the locations indicated by _TCLTK_INCLUDES
        # and _TCLTK_LIBS environment variables.
        if self.detect_tkinter_fromenv():
            return True

        # Rather than complicate the code below, detecting and building
        # AquaTk is a separate method. Only one Tkinter will be built on
        # Darwin - either AquaTk, if it is found, or X11 based Tk.
        if (MACOS and self.detect_tkinter_darwin()):
            return True

        # Assume we haven't found any of the libraries or include files
        # The versions with dots are used on Unix, and the versions without
        # dots on Windows, for detection by cygwin.
        tcllib = tklib = tcl_includes = tk_includes = None
        for version in ['8.6', '86', '8.5', '85', '8.4', '84', '8.3', '83',
                        '8.2', '82', '8.1', '81', '8.0', '80']:
            tklib = self.compiler.find_library_file(self.lib_dirs,
                                                        'tk' + version)
            tcllib = self.compiler.find_library_file(self.lib_dirs,
                                                         'tcl' + version)
            if tklib and tcllib:
                # Exit the loop when we've found the Tcl/Tk libraries
                break

        # Now check for the header files
        if tklib and tcllib:
            # Check for the include files on Debian and {Free,Open}BSD, where
            # they're put in /usr/include/{tcl,tk}X.Y
            dotversion = version
            if '.' not in dotversion and "bsd" in HOST_PLATFORM.lower():
                # OpenBSD and FreeBSD use Tcl/Tk library names like libtcl83.a,
                # but the include subdirs are named like .../include/tcl8.3.
                dotversion = dotversion[:-1] + '.' + dotversion[-1]
            tcl_include_sub = []
            tk_include_sub = []
            for dir in self.inc_dirs:
                tcl_include_sub += [dir + os.sep + "tcl" + dotversion]
                tk_include_sub += [dir + os.sep + "tk" + dotversion]
            tk_include_sub += tcl_include_sub
            tcl_includes = find_file('tcl.h', self.inc_dirs, tcl_include_sub)
            tk_includes = find_file('tk.h', self.inc_dirs, tk_include_sub)

        if (tcllib is None or tklib is None or
            tcl_includes is None or tk_includes is None):
            self.announce("INFO: Can't locate Tcl/Tk libs and/or headers", 2)
            return False

        # OK... everything seems to be present for Tcl/Tk.

        include_dirs = []
        libs = []
        defs = []
        added_lib_dirs = []
        for dir in tcl_includes + tk_includes:
            if dir not in include_dirs:
                include_dirs.append(dir)

        # Check for various platform-specific directories
        if HOST_PLATFORM == 'sunos5':
            include_dirs.append('/usr/openwin/include')
            added_lib_dirs.append('/usr/openwin/lib')
        elif os.path.exists('/usr/X11R6/include'):
            include_dirs.append('/usr/X11R6/include')
            added_lib_dirs.append('/usr/X11R6/lib64')
            added_lib_dirs.append('/usr/X11R6/lib')
        elif os.path.exists('/usr/X11R5/include'):
            include_dirs.append('/usr/X11R5/include')
            added_lib_dirs.append('/usr/X11R5/lib')
        else:
            # Assume default location for X11
            include_dirs.append('/usr/X11/include')
            added_lib_dirs.append('/usr/X11/lib')

        # If Cygwin, then verify that X is installed before proceeding
        if CYGWIN:
            x11_inc = find_file('X11/Xlib.h', [], include_dirs)
            if x11_inc is None:
                return False

        # Check for BLT extension
        if self.compiler.find_library_file(self.lib_dirs + added_lib_dirs,
                                               'BLT8.0'):
            defs.append( ('WITH_BLT', 1) )
            libs.append('BLT8.0')
        elif self.compiler.find_library_file(self.lib_dirs + added_lib_dirs,
                                                'BLT'):
            defs.append( ('WITH_BLT', 1) )
            libs.append('BLT')

        # Add the Tcl/Tk libraries
        libs.append('tk'+ version)
        libs.append('tcl'+ version)

        # Finally, link with the X11 libraries (not appropriate on cygwin)
        if not CYGWIN:
            libs.append('X11')

        # XXX handle these, but how to detect?
        # *** Uncomment and edit for PIL (TkImaging) extension only:
        #       -DWITH_PIL -I../Extensions/Imaging/libImaging  tkImaging.c \
        # *** Uncomment and edit for TOGL extension only:
        #       -DWITH_TOGL togl.c \
        # *** Uncomment these for TOGL extension only:
        #       -lGL -lGLU -lXext -lXmu \

        self.add(Extension('_tkinter', ['_tkinter.c', 'tkappinit.c'],
                           define_macros=[('WITH_APPINIT', 1)] + defs,
                           include_dirs=include_dirs,
                           libraries=libs,
                           library_dirs=added_lib_dirs))
        return True

    def configure_ctypes(self, ext):
        return True

    def detect_ctypes(self):
        # Thomas Heller's _ctypes module

        if (not sysconfig.get_config_var("LIBFFI_INCLUDEDIR") and MACOS):
            self.use_system_libffi = True
        else:
            self.use_system_libffi = '--with-system-ffi' in sysconfig.get_config_var("CONFIG_ARGS")

        include_dirs = []
        extra_compile_args = ['-DPy_BUILD_CORE_MODULE']
        extra_link_args = []
        sources = ['_ctypes/_ctypes.c',
                   '_ctypes/callbacks.c',
                   '_ctypes/callproc.c',
                   '_ctypes/stgdict.c',
                   '_ctypes/cfield.c']
        depends = ['_ctypes/ctypes.h']

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

        elif HOST_PLATFORM.startswith('hp-ux'):
            extra_link_args.append('-fPIC')

        ext = Extension('_ctypes',
                        include_dirs=include_dirs,
                        extra_compile_args=extra_compile_args,
                        extra_link_args=extra_link_args,
                        libraries=[],
                        sources=sources,
                        depends=depends)
        self.add(ext)
        if TEST_EXTENSIONS:
            # function my_sqrt() needs libm for sqrt()
            self.add(Extension('_ctypes_test',
                               sources=['_ctypes/_ctypes_test.c'],
                               libraries=['m']))

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
        extra_compile_args = []
        undef_macros = []
        if '--with-system-libmpdec' in sysconfig.get_config_var("CONFIG_ARGS"):
            include_dirs = []
            libraries = [':libmpdec.so.2']
            sources = ['_decimal/_decimal.c']
            depends = ['_decimal/docstrings.h']
        else:
            include_dirs = [os.path.abspath(os.path.join(self.srcdir,
                                                         'Modules',
                                                         '_decimal',
                                                         'libmpdec'))]
            libraries = ['m']
            sources = [
              '_decimal/_decimal.c',
              '_decimal/libmpdec/basearith.c',
              '_decimal/libmpdec/constants.c',
              '_decimal/libmpdec/context.c',
              '_decimal/libmpdec/convolute.c',
              '_decimal/libmpdec/crt.c',
              '_decimal/libmpdec/difradix2.c',
              '_decimal/libmpdec/fnt.c',
              '_decimal/libmpdec/fourstep.c',
              '_decimal/libmpdec/io.c',
              '_decimal/libmpdec/mpalloc.c',
              '_decimal/libmpdec/mpdecimal.c',
              '_decimal/libmpdec/numbertheory.c',
              '_decimal/libmpdec/sixstep.c',
              '_decimal/libmpdec/transpose.c',
              ]
            depends = [
              '_decimal/docstrings.h',
              '_decimal/libmpdec/basearith.h',
              '_decimal/libmpdec/bits.h',
              '_decimal/libmpdec/constants.h',
              '_decimal/libmpdec/convolute.h',
              '_decimal/libmpdec/crt.h',
              '_decimal/libmpdec/difradix2.h',
              '_decimal/libmpdec/fnt.h',
              '_decimal/libmpdec/fourstep.h',
              '_decimal/libmpdec/io.h',
              '_decimal/libmpdec/mpalloc.h',
              '_decimal/libmpdec/mpdecimal.h',
              '_decimal/libmpdec/numbertheory.h',
              '_decimal/libmpdec/sixstep.h',
              '_decimal/libmpdec/transpose.h',
              '_decimal/libmpdec/typearith.h',
              '_decimal/libmpdec/umodarith.h',
              ]

        config = {
          'x64':     [('CONFIG_64','1'), ('ASM','1')],
          'uint128': [('CONFIG_64','1'), ('ANSI','1'), ('HAVE_UINT128_T','1')],
          'ansi64':  [('CONFIG_64','1'), ('ANSI','1')],
          'ppro':    [('CONFIG_32','1'), ('PPRO','1'), ('ASM','1')],
          'ansi32':  [('CONFIG_32','1'), ('ANSI','1')],
          'ansi-legacy': [('CONFIG_32','1'), ('ANSI','1'),
                          ('LEGACY_COMPILER','1')],
          'universal':   [('UNIVERSAL','1')]
        }

        cc = sysconfig.get_config_var('CC')
        sizeof_size_t = sysconfig.get_config_var('SIZEOF_SIZE_T')
        machine = os.environ.get('PYTHON_DECIMAL_WITH_MACHINE')

        if machine:
            # Override automatic configuration to facilitate testing.
            define_macros = config[machine]
        elif MACOS:
            # Universal here means: build with the same options Python
            # was built with.
            define_macros = config['universal']
        elif sizeof_size_t == 8:
            if sysconfig.get_config_var('HAVE_GCC_ASM_FOR_X64'):
                define_macros = config['x64']
            elif sysconfig.get_config_var('HAVE_GCC_UINT128_T'):
                define_macros = config['uint128']
            else:
                define_macros = config['ansi64']
        elif sizeof_size_t == 4:
            ppro = sysconfig.get_config_var('HAVE_GCC_ASM_FOR_X87')
            if ppro and ('gcc' in cc or 'clang' in cc) and \
               not 'sunos' in HOST_PLATFORM:
                # solaris: problems with register allocation.
                # icc >= 11.0 works as well.
                define_macros = config['ppro']
                extra_compile_args.append('-Wno-unknown-pragmas')
            else:
                define_macros = config['ansi32']
        else:
            raise DistutilsError("_decimal: unsupported architecture")

        # Workarounds for toolchain bugs:
        if sysconfig.get_config_var('HAVE_IPA_PURE_CONST_BUG'):
            # Some versions of gcc miscompile inline asm:
            # http://gcc.gnu.org/bugzilla/show_bug.cgi?id=46491
            # http://gcc.gnu.org/ml/gcc/2010-11/msg00366.html
            extra_compile_args.append('-fno-ipa-pure-const')
        if sysconfig.get_config_var('HAVE_GLIBC_MEMMOVE_BUG'):
            # _FORTIFY_SOURCE wrappers for memmove and bcopy are incorrect:
            # http://sourceware.org/ml/libc-alpha/2010-12/msg00009.html
            undef_macros.append('_FORTIFY_SOURCE')

        # Uncomment for extra functionality:
        #define_macros.append(('EXTRA_FUNCTIONALITY', 1))
        self.add(Extension('_decimal',
                           include_dirs=include_dirs,
                           libraries=libraries,
                           define_macros=define_macros,
                           undef_macros=undef_macros,
                           extra_compile_args=extra_compile_args,
                           sources=sources,
                           depends=depends))

    def detect_openssl_hashlib(self):
        # Detect SSL support for the socket module (via _ssl)
        config_vars = sysconfig.get_config_vars()

        def split_var(name, sep):
            # poor man's shlex, the re module is not available yet.
            value = config_vars.get(name)
            if not value:
                return ()
            # This trick works because ax_check_openssl uses --libs-only-L,
            # --libs-only-l, and --cflags-only-I.
            value = ' ' + value
            sep = ' ' + sep
            return [v.strip() for v in value.split(sep) if v.strip()]

        openssl_includes = split_var('OPENSSL_INCLUDES', '-I')
        openssl_libdirs = split_var('OPENSSL_LDFLAGS', '-L')
        openssl_libs = split_var('OPENSSL_LIBS', '-l')
        openssl_rpath = config_vars.get('OPENSSL_RPATH')
        if not openssl_libs:
            # libssl and libcrypto not found
            self.missing.extend(['_ssl', '_hashlib'])
            return None, None

        # Find OpenSSL includes
        ssl_incs = find_file(
            'openssl/ssl.h', self.inc_dirs, openssl_includes
        )
        if ssl_incs is None:
            self.missing.extend(['_ssl', '_hashlib'])
            return None, None

        # OpenSSL 1.0.2 uses Kerberos for KRB5 ciphers
        krb5_h = find_file(
            'krb5.h', self.inc_dirs,
            ['/usr/kerberos/include']
        )
        if krb5_h:
            ssl_incs.extend(krb5_h)

        if openssl_rpath == 'auto':
            runtime_library_dirs = openssl_libdirs[:]
        elif not openssl_rpath:
            runtime_library_dirs = []
        else:
            runtime_library_dirs = [openssl_rpath]

        openssl_extension_kwargs = dict(
            include_dirs=openssl_includes,
            library_dirs=openssl_libdirs,
            libraries=openssl_libs,
            runtime_library_dirs=runtime_library_dirs,
        )

        # This static linking is NOT OFFICIALLY SUPPORTED.
        # Requires static OpenSSL build with position-independent code. Some
        # features like DSO engines or external OSSL providers don't work.
        # Only tested on GCC and clang on X86_64.
        if os.environ.get("PY_UNSUPPORTED_OPENSSL_BUILD") == "static":
            extra_linker_args = []
            for lib in openssl_extension_kwargs["libraries"]:
                # link statically
                extra_linker_args.append(f"-l:lib{lib}.a")
                # don't export symbols
                extra_linker_args.append(f"-Wl,--exclude-libs,lib{lib}.a")
            openssl_extension_kwargs["extra_link_args"] = extra_linker_args
            # don't link OpenSSL shared libraries.
            openssl_extension_kwargs["libraries"] = []

        if config_vars.get("HAVE_X509_VERIFY_PARAM_SET1_HOST"):
            self.add(
                Extension(
                    '_ssl',
                    ['_ssl.c'],
                    depends=['socketmodule.h', '_ssl/debughelpers.c'],
                    **openssl_extension_kwargs
                )
            )
        else:
            self.missing.append('_ssl')

        self.add(
            Extension(
                '_hashlib',
                ['_hashopenssl.c'],
                depends=['hashlib.h'],
                **openssl_extension_kwargs,
            )
        )

    def detect_hash_builtins(self):
        # By default we always compile these even when OpenSSL is available
        # (issue #14693). It's harmless and the object code is tiny
        # (40-50 KiB per module, only loaded when actually used).  Modules can
        # be disabled via the --with-builtin-hashlib-hashes configure flag.
        supported = {"md5", "sha1", "sha256", "sha512", "sha3", "blake2"}

        configured = sysconfig.get_config_var("PY_BUILTIN_HASHLIB_HASHES")
        configured = configured.strip('"').lower()
        configured = {
            m.strip() for m in configured.split(",")
        }

        self.disabled_configure.extend(
            sorted(supported.difference(configured))
        )

        if "sha256" in configured:
            self.add(Extension(
                '_sha256', ['sha256module.c'],
                extra_compile_args=['-DPy_BUILD_CORE_MODULE'],
                depends=['hashlib.h']
            ))

        if "sha512" in configured:
            self.add(Extension(
                '_sha512', ['sha512module.c'],
                extra_compile_args=['-DPy_BUILD_CORE_MODULE'],
                depends=['hashlib.h']
            ))

        if "md5" in configured:
            self.add(Extension(
                '_md5', ['md5module.c'],
                depends=['hashlib.h']
            ))

        if "sha1" in configured:
            self.add(Extension(
                '_sha1', ['sha1module.c'],
                depends=['hashlib.h']
            ))

        if "blake2" in configured:
            blake2_deps = glob(
                os.path.join(escape(self.srcdir), 'Modules/_blake2/impl/*')
            )
            blake2_deps.append('hashlib.h')
            self.add(Extension(
                '_blake2',
                [
                    '_blake2/blake2module.c',
                    '_blake2/blake2b_impl.c',
                    '_blake2/blake2s_impl.c'
                ],
                depends=blake2_deps
            ))

        if "sha3" in configured:
            sha3_deps = glob(
                os.path.join(escape(self.srcdir), 'Modules/_sha3/kcp/*')
            )
            sha3_deps.append('hashlib.h')
            self.add(Extension(
                '_sha3',
                ['_sha3/sha3module.c'],
                depends=sha3_deps
            ))

    def detect_nis(self):
        if MS_WINDOWS or CYGWIN or HOST_PLATFORM == 'qnx6':
            self.missing.append('nis')
            return

        libs = []
        library_dirs = []
        includes_dirs = []

        # bpo-32521: glibc has deprecated Sun RPC for some time. Fedora 28
        # moved headers and libraries to libtirpc and libnsl. The headers
        # are in tircp and nsl sub directories.
        rpcsvc_inc = find_file(
            'rpcsvc/yp_prot.h', self.inc_dirs,
            [os.path.join(inc_dir, 'nsl') for inc_dir in self.inc_dirs]
        )
        rpc_inc = find_file(
            'rpc/rpc.h', self.inc_dirs,
            [os.path.join(inc_dir, 'tirpc') for inc_dir in self.inc_dirs]
        )
        if rpcsvc_inc is None or rpc_inc is None:
            # not found
            self.missing.append('nis')
            return
        includes_dirs.extend(rpcsvc_inc)
        includes_dirs.extend(rpc_inc)

        if self.compiler.find_library_file(self.lib_dirs, 'nsl'):
            libs.append('nsl')
        else:
            # libnsl-devel: check for libnsl in nsl/ subdirectory
            nsl_dirs = [os.path.join(lib_dir, 'nsl') for lib_dir in self.lib_dirs]
            libnsl = self.compiler.find_library_file(nsl_dirs, 'nsl')
            if libnsl is not None:
                library_dirs.append(os.path.dirname(libnsl))
                libs.append('nsl')

        if self.compiler.find_library_file(self.lib_dirs, 'tirpc'):
            libs.append('tirpc')

        self.add(Extension('nis', ['nismodule.c'],
                           libraries=libs,
                           library_dirs=library_dirs,
                           include_dirs=includes_dirs))


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
            log.info('renaming %s to %s', filename, newfilename)
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
          url = "http://www.python.org/%d.%d" % sys.version_info[:2],
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
          # The struct module is defined here, because build_ext won't be
          # called unless there's at least one extension module defined.
          ext_modules=[Extension('_struct', ['_struct.c'])],

          # If you change the scripts installed here, you also need to
          # check the PyBuildScripts command above, and change the links
          # created by the bininstall target in Makefile.pre.in
          scripts = ["Tools/scripts/pydoc3", "Tools/scripts/idle3",
                     "Tools/scripts/2to3"]
        )

# --install-platlib
if __name__ == '__main__':
    main()
