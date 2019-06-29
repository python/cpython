# Autodetecting setup.py script for building the Python extensions
#

import sys, os, importlib.machinery, re, optparse
from glob import glob
import importlib._bootstrap
import importlib.util
import sysconfig

from distutils import log
from distutils.errors import *
from distutils.core import Extension, setup
from distutils.command.build_ext import build_ext
from distutils.command.install import install
from distutils.command.install_lib import install_lib
from distutils.command.build_scripts import build_scripts
from distutils.spawn import find_executable

cross_compiling = "_PYTHON_HOST_PLATFORM" in os.environ

# Set common compiler and linker flags derived from the Makefile,
# reserved for building the interpreter and the stdlib modules.
# See bpo-21121 and bpo-35257
def set_compiler_flags(compiler_flags, compiler_py_flags_nodist):
    flags = sysconfig.get_config_var(compiler_flags)
    py_flags_nodist = sysconfig.get_config_var(compiler_py_flags_nodist)
    sysconfig.get_config_vars()[compiler_flags] = flags + ' ' + py_flags_nodist

set_compiler_flags('CFLAGS', 'PY_CFLAGS_NODIST')
set_compiler_flags('LDFLAGS', 'PY_LDFLAGS_NODIST')

class Dummy:
    """Hack for parallel build"""
    ProcessPoolExecutor = None
sys.modules['concurrent.futures.process'] = Dummy

def get_platform():
    # cross build
    if "_PYTHON_HOST_PLATFORM" in os.environ:
        return os.environ["_PYTHON_HOST_PLATFORM"]
    # Get value of sys.platform
    if sys.platform.startswith('osf1'):
        return 'osf1'
    return sys.platform
host_platform = get_platform()

# Were we compiled --with-pydebug or with #define Py_DEBUG?
COMPILED_WITH_PYDEBUG = ('--with-pydebug' in sysconfig.get_config_var("CONFIG_ARGS"))

# This global variable is used to hold the list of modules to be disabled.
disabled_module_list = []

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

def macosx_sdk_root():
    """Return the directory of the current macOS SDK.

    If no SDK was explicitly configured, call the compiler to find which
    include files paths are being searched by default.  Use '/' if the
    compiler is searching /usr/include (meaning system header files are
    installed) or use the root of an SDK if that is being searched.
    (The SDK may be supplied via Xcode or via the Command Line Tools).
    The SDK paths used by Apple-supplied tool chains depend on the
    setting of various variables; see the xcrun man page for more info.
    """
    global MACOS_SDK_ROOT

    # If already called, return cached result.
    if MACOS_SDK_ROOT:
        return MACOS_SDK_ROOT

    cflags = sysconfig.get_config_var('CFLAGS')
    m = re.search(r'-isysroot\s+(\S+)', cflags)
    if m is not None:
        MACOS_SDK_ROOT = m.group(1)
    else:
        MACOS_SDK_ROOT = '/'
        cc = sysconfig.get_config_var('CC')
        tmpfile = '/tmp/setup_sdk_root.%d' % os.getpid()
        try:
            os.unlink(tmpfile)
        except:
            pass
        ret = os.system('%s -E -v - </dev/null 2>%s 1>/dev/null' % (cc, tmpfile))
        in_incdirs = False
        try:
            if ret >> 8 == 0:
                with open(tmpfile) as fp:
                    for line in fp.readlines():
                        if line.startswith("#include <...>"):
                            in_incdirs = True
                        elif line.startswith("End of search list"):
                            in_incdirs = False
                        elif in_incdirs:
                            line = line.strip()
                            if line == '/usr/include':
                                MACOS_SDK_ROOT = '/'
                            elif line.endswith(".sdk/usr/include"):
                                MACOS_SDK_ROOT = line[:-12]
        finally:
            os.unlink(tmpfile)

    return MACOS_SDK_ROOT

def is_macosx_sdk_path(path):
    """
    Returns True if 'path' can be located in an OSX SDK
    """
    return ( (path.startswith('/usr/') and not path.startswith('/usr/local'))
                or path.startswith('/System/')
                or path.startswith('/Library/') )

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
    if host_platform == 'darwin':
        # Honor the MacOSX SDK setting when one was specified.
        # An SDK is a directory with the same structure as a real
        # system, but with only header files and libraries.
        sysroot = macosx_sdk_root()

    # Check the standard locations
    for dir in std_dirs:
        f = os.path.join(dir, filename)

        if host_platform == 'darwin' and is_macosx_sdk_path(dir):
            f = os.path.join(sysroot, dir[1:], filename)

        if os.path.exists(f): return []

    # Check the additional directories
    for dir in paths:
        f = os.path.join(dir, filename)

        if host_platform == 'darwin' and is_macosx_sdk_path(dir):
            f = os.path.join(sysroot, dir[1:], filename)

        if os.path.exists(f):
            return [dir]

    # Not found anywhere
    return None

def find_library_file(compiler, libname, std_dirs, paths):
    result = compiler.find_library_file(std_dirs + paths, libname)
    if result is None:
        return None

    if host_platform == 'darwin':
        sysroot = macosx_sdk_root()

    # Check whether the found file is in one of the standard directories
    dirname = os.path.dirname(result)
    for p in std_dirs:
        # Ensure path doesn't end with path separator
        p = p.rstrip(os.sep)

        if host_platform == 'darwin' and is_macosx_sdk_path(p):
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

        if host_platform == 'darwin' and is_macosx_sdk_path(p):
            if os.path.join(sysroot, p[1:]) == dirname:
                return [ p ]

        if p == dirname:
            return [p]
    else:
        assert False, "Internal error: Path not found in std_dirs or paths"

def module_enabled(extlist, modname):
    """Returns whether the module 'modname' is present in the list
    of extensions 'extlist'."""
    extlist = [ext for ext in extlist if ext.name == modname]
    return len(extlist)

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
        self.failed = []
        self.failed_on_import = []
        if '-j' in os.environ.get('MAKEFLAGS', ''):
            self.parallel = True

    def build_extensions(self):

        # Detect which modules should be compiled
        missing = self.detect_modules()

        # Remove modules that are present on the disabled list
        extensions = [ext for ext in self.extensions
                      if ext.name not in disabled_module_list]
        # move ctypes to the end, it depends on other modules
        ext_map = dict((ext.name, i) for i, ext in enumerate(extensions))
        if "_ctypes" in ext_map:
            ctypes = extensions.pop(ext_map["_ctypes"])
            extensions.append(ctypes)
        self.extensions = extensions

        # Fix up the autodetected modules, prefixing all the source files
        # with Modules/.
        srcdir = sysconfig.get_config_var('srcdir')
        if not srcdir:
            # Maybe running on Windows but not using CYGWIN?
            raise ValueError("No source directory; cannot proceed.")
        srcdir = os.path.abspath(srcdir)
        moddirlist = [os.path.join(srcdir, 'Modules')]

        # Fix up the paths for scripts, too
        self.distribution.scripts = [os.path.join(srcdir, filename)
                                     for filename in self.distribution.scripts]

        # Python header files
        headers = [sysconfig.get_config_h_filename()]
        headers += glob(os.path.join(sysconfig.get_path('include'), "*.h"))

        # The sysconfig variables built by makesetup that list the already
        # built modules and the disabled modules as configured by the Setup
        # files.
        sysconf_built = sysconfig.get_config_var('MODBUILT_NAMES').split()
        sysconf_dis = sysconfig.get_config_var('MODDISABLED_NAMES').split()

        mods_built = []
        mods_disabled = []
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

        build_ext.build_extensions(self)

        for ext in self.extensions:
            self.check_extension_import(ext)

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

        if missing:
            print()
            print("Python build finished successfully!")
            print("The necessary bits to build these optional modules were not "
                  "found:")
            print_three_column(missing)
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
               for l in (missing, self.failed, self.failed_on_import)):
            print()
            print("Could not build the ssl module!")
            print("Python requires an OpenSSL 1.0.2 or 1.1 compatible "
                  "libssl with X509_VERIFY_PARAM_set1_host().")
            print("LibreSSL 2.6.4 and earlier do not provide the necessary "
                  "APIs, https://github.com/libressl-portable/portable/issues/381")
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
                          (ext.name, sys.exc_info()[1]))
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

        if host_platform == 'darwin' and (
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
        if host_platform == 'cygwin':
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
        if cross_compiling:
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
        ret = os.system(
            '%s -print-multiarch > %s 2> /dev/null' % (cc, tmpfile))
        multiarch_path_component = ''
        try:
            if ret >> 8 == 0:
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
        if cross_compiling:
            opt = '-t' + sysconfig.get_config_var('HOST_GNU_TYPE')
        tmpfile = os.path.join(self.build_temp, 'multiarch')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        ret = os.system(
            'dpkg-architecture %s -qDEB_HOST_MULTIARCH > %s 2> /dev/null' %
            (opt, tmpfile))
        try:
            if ret >> 8 == 0:
                with open(tmpfile) as fp:
                    multiarch_path_component = fp.readline().strip()
                add_dir_to_list(self.compiler.library_dirs,
                                '/usr/lib/' + multiarch_path_component)
                add_dir_to_list(self.compiler.include_dirs,
                                '/usr/include/' + multiarch_path_component)
        finally:
            os.unlink(tmpfile)

    def add_gcc_paths(self):
        gcc = sysconfig.get_config_var('CC')
        tmpfile = os.path.join(self.build_temp, 'gccpaths')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        ret = os.system('%s -E -v - </dev/null 2>%s 1>/dev/null' % (gcc, tmpfile))
        is_gcc = False
        in_incdirs = False
        inc_dirs = []
        lib_dirs = []
        try:
            if ret >> 8 == 0:
                with open(tmpfile) as fp:
                    for line in fp.readlines():
                        if line.startswith("gcc version"):
                            is_gcc = True
                        elif line.startswith("#include <...>"):
                            in_incdirs = True
                        elif line.startswith("End of search list"):
                            in_incdirs = False
                        elif is_gcc and line.startswith("LIBRARY_PATH"):
                            for d in line.strip().split("=")[1].split(":"):
                                d = os.path.normpath(d)
                                if '/gcc/' not in d:
                                    add_dir_to_list(self.compiler.library_dirs,
                                                    d)
                        elif is_gcc and in_incdirs and '/gcc/' not in line:
                            add_dir_to_list(self.compiler.include_dirs,
                                            line.strip())
        finally:
            os.unlink(tmpfile)

    def detect_modules(self):
        # Ensure that /usr/local is always used, but the local build
        # directories (i.e. '.' and 'Include') must be first.  See issue
        # 10520.
        if not cross_compiling:
            add_dir_to_list(self.compiler.library_dirs, '/usr/local/lib')
            add_dir_to_list(self.compiler.include_dirs, '/usr/local/include')
        # only change this for cross builds for 3.3, issues on Mageia
        if cross_compiling:
            self.add_gcc_paths()
        self.add_multiarch_paths()

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
                # To prevent optparse from raising an exception about any
                # options in env_val that it doesn't know about we strip out
                # all double dashes and any dashes followed by a character
                # that is not for the option we are dealing with.
                #
                # Please note that order of the regex is important!  We must
                # strip out double-dashes first so that we don't end up with
                # substituting "--Long" to "-Long" and thus lead to "ong" being
                # used for a library directory.
                env_val = re.sub(r'(^|\s+)-(-|(?!%s))' % arg_name[1],
                                 ' ', env_val)
                parser = optparse.OptionParser()
                # Make sure that allowing args interspersed with options is
                # allowed
                parser.allow_interspersed_args = True
                parser.error = lambda msg: None
                parser.add_option(arg_name, dest="dirs", action="append")
                options = parser.parse_args(env_val.split())[0]
                if options.dirs:
                    for directory in reversed(options.dirs):
                        add_dir_to_list(dir_list, directory)

        if (not cross_compiling and
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
        if not cross_compiling:
            lib_dirs = self.compiler.library_dirs + system_lib_dirs
            inc_dirs = self.compiler.include_dirs + system_include_dirs
        else:
            # Add the sysroot paths. 'sysroot' is a compiler option used to
            # set the logical path of the standard system headers and
            # libraries.
            lib_dirs = (self.compiler.library_dirs +
                        sysroot_paths(('LDFLAGS', 'CC'), system_lib_dirs))
            inc_dirs = (self.compiler.include_dirs +
                        sysroot_paths(('CPPFLAGS', 'CFLAGS', 'CC'),
                                      system_include_dirs))
        exts = []
        missing = []

        config_h = sysconfig.get_config_h_filename()
        with open(config_h) as file:
            config_h_vars = sysconfig.parse_config_h(file)

        srcdir = sysconfig.get_config_var('srcdir')

        # OSF/1 and Unixware have some stuff in /usr/ccs/lib (like -ldb)
        if host_platform in ['osf1', 'unixware7', 'openunix8']:
            lib_dirs += ['/usr/ccs/lib']

        # HP-UX11iv3 keeps files in lib/hpux folders.
        if host_platform == 'hp-ux11':
            lib_dirs += ['/usr/lib/hpux64', '/usr/lib/hpux32']

        if host_platform == 'darwin':
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
                    inc_dirs.append(item[2:])

            for item in ldflags.split():
                if item.startswith('-L'):
                    lib_dirs.append(item[2:])

        #
        # The following modules are all pretty straightforward, and compile
        # on pretty much any POSIXish platform.
        #

        # array objects
        exts.append( Extension('array', ['arraymodule.c']) )

        # Context Variables
        exts.append( Extension('_contextvars', ['_contextvarsmodule.c']) )

        shared_math = 'Modules/_math.o'
        # complex math library functions
        exts.append( Extension('cmath', ['cmathmodule.c'],
                               extra_objects=[shared_math],
                               depends=['_math.h', shared_math],
                               libraries=['m']) )
        # math library functions, e.g. sin()
        exts.append( Extension('math',  ['mathmodule.c'],
                               extra_objects=[shared_math],
                               depends=['_math.h', shared_math],
                               libraries=['m']) )

        # time libraries: librt may be needed for clock_gettime()
        time_libs = []
        lib = sysconfig.get_config_var('TIMEMODULE_LIB')
        if lib:
            time_libs.append(lib)

        # time operations and variables
        exts.append( Extension('time', ['timemodule.c'],
                               libraries=time_libs) )
        # libm is needed by delta_new() that uses round() and by accum() that
        # uses modf().
        exts.append( Extension('_datetime', ['_datetimemodule.c'],
                               libraries=['m']) )
        # random number generator implemented in C
        exts.append( Extension("_random", ["_randommodule.c"]) )
        # bisect
        exts.append( Extension("_bisect", ["_bisectmodule.c"]) )
        # heapq
        exts.append( Extension("_heapq", ["_heapqmodule.c"]) )
        # C-optimized pickle replacement
        exts.append( Extension("_pickle", ["_pickle.c"]) )
        # atexit
        exts.append( Extension("atexit", ["atexitmodule.c"]) )
        # _json speedups
        exts.append( Extension("_json", ["_json.c"]) )
        # Python C API test module
        exts.append( Extension('_testcapi', ['_testcapimodule.c'],
                               depends=['testcapi_long.h']) )
        # Python PEP-3118 (buffer protocol) test module
        exts.append( Extension('_testbuffer', ['_testbuffer.c']) )
        # Test loading multiple modules from one compiled file (http://bugs.python.org/issue16421)
        exts.append( Extension('_testimportmultiple', ['_testimportmultiple.c']) )
        # Test multi-phase extension module init (PEP 489)
        exts.append( Extension('_testmultiphase', ['_testmultiphase.c']) )
        # profiler (_lsprof is for cProfile.py)
        exts.append( Extension('_lsprof', ['_lsprof.c', 'rotatingtree.c']) )
        # static Unicode character database
        exts.append( Extension('unicodedata', ['unicodedata.c'],
                               depends=['unicodedata_db.h', 'unicodename_db.h']) )
        # _opcode module
        exts.append( Extension('_opcode', ['_opcode.c']) )
        # asyncio speedups
        exts.append( Extension("_asyncio", ["_asynciomodule.c"]) )
        # _abc speedups
        exts.append( Extension("_abc", ["_abc.c"]) )
        # _queue module
        exts.append( Extension("_queue", ["_queuemodule.c"]) )

        # Modules with some UNIX dependencies -- on by default:
        # (If you have a really backward UNIX, select and socket may not be
        # supported...)

        # fcntl(2) and ioctl(2)
        libs = []
        if (config_h_vars.get('FLOCK_NEEDS_LIBBSD', False)):
            # May be necessary on AIX for flock function
            libs = ['bsd']
        exts.append( Extension('fcntl', ['fcntlmodule.c'], libraries=libs) )
        # pwd(3)
        exts.append( Extension('pwd', ['pwdmodule.c']) )
        # grp(3)
        exts.append( Extension('grp', ['grpmodule.c']) )
        # spwd, shadow passwords
        if (config_h_vars.get('HAVE_GETSPNAM', False) or
                config_h_vars.get('HAVE_GETSPENT', False)):
            exts.append( Extension('spwd', ['spwdmodule.c']) )
        else:
            missing.append('spwd')

        # select(2); not on ancient System V
        exts.append( Extension('select', ['selectmodule.c']) )

        # Fred Drake's interface to the Python parser
        exts.append( Extension('parser', ['parsermodule.c']) )

        # Memory-mapped files (also works on Win32).
        exts.append( Extension('mmap', ['mmapmodule.c']) )

        # Lance Ellinghaus's syslog module
        # syslog daemon interface
        exts.append( Extension('syslog', ['syslogmodule.c']) )

        # Fuzz tests.
        exts.append( Extension(
            '_xxtestfuzz',
            ['_xxtestfuzz/_xxtestfuzz.c', '_xxtestfuzz/fuzzer.c'])
        )

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
        exts.append( Extension('audioop', ['audioop.c'],
                               libraries=['m']) )

        # readline
        do_readline = self.compiler.find_library_file(lib_dirs, 'readline')
        readline_termcap_library = ""
        curses_library = ""
        # Cannot use os.popen here in py3k.
        tmpfile = os.path.join(self.build_temp, 'readline_termcap_lib')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        # Determine if readline is already linked against curses or tinfo.
        if do_readline:
            if cross_compiling:
                ret = os.system("%s -d %s | grep '(NEEDED)' > %s" \
                                % (sysconfig.get_config_var('READELF'),
                                   do_readline, tmpfile))
            elif find_executable('ldd'):
                ret = os.system("ldd %s > %s" % (do_readline, tmpfile))
            else:
                ret = 256
            if ret >> 8 == 0:
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
        # Issue 7384: If readline is already linked against curses,
        # use the same library for the readline and curses modules.
        if 'curses' in readline_termcap_library:
            curses_library = readline_termcap_library
        elif self.compiler.find_library_file(lib_dirs, 'ncursesw'):
            curses_library = 'ncursesw'
        elif self.compiler.find_library_file(lib_dirs, 'ncurses'):
            curses_library = 'ncurses'
        elif self.compiler.find_library_file(lib_dirs, 'curses'):
            curses_library = 'curses'

        if host_platform == 'darwin':
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
                if find_file('readline/rlconf.h', inc_dirs, []) is None:
                    do_readline = False
        if do_readline:
            if host_platform == 'darwin' and os_release < 9:
                # In every directory on the search path search for a dynamic
                # library and then a static library, instead of first looking
                # for dynamic libraries on the entire path.
                # This way a statically linked custom readline gets picked up
                # before the (possibly broken) dynamic library in /usr/lib.
                readline_extra_link_args = ('-Wl,-search_paths_first',)
            else:
                readline_extra_link_args = ()

            readline_libs = ['readline']
            if readline_termcap_library:
                pass # Issue 7384: Already linked against curses or tinfo.
            elif curses_library:
                readline_libs.append(curses_library)
            elif self.compiler.find_library_file(lib_dirs +
                                                     ['/usr/lib/termcap'],
                                                     'termcap'):
                readline_libs.append('termcap')
            exts.append( Extension('readline', ['readline.c'],
                                   library_dirs=['/usr/lib/termcap'],
                                   extra_link_args=readline_extra_link_args,
                                   libraries=readline_libs) )
        else:
            missing.append('readline')

        # crypt module.

        if self.compiler.find_library_file(lib_dirs, 'crypt'):
            libs = ['crypt']
        else:
            libs = []
        exts.append( Extension('_crypt', ['_cryptmodule.c'], libraries=libs) )

        # CSV files
        exts.append( Extension('_csv', ['_csv.c']) )

        # POSIX subprocess module helper.
        exts.append( Extension('_posixsubprocess', ['_posixsubprocess.c']) )

        # socket(2)
        exts.append( Extension('_socket', ['socketmodule.c'],
                               depends = ['socketmodule.h']) )
        # Detect SSL support for the socket module (via _ssl)
        ssl_ext, hashlib_ext = self._detect_openssl(inc_dirs, lib_dirs)
        if ssl_ext is not None:
            exts.append(ssl_ext)
        else:
            missing.append('_ssl')
        if hashlib_ext is not None:
            exts.append(hashlib_ext)
        else:
            missing.append('_hashlib')

        # We always compile these even when OpenSSL is available (issue #14693).
        # It's harmless and the object code is tiny (40-50 KiB per module,
        # only loaded when actually used).
        exts.append( Extension('_sha256', ['sha256module.c'],
                               depends=['hashlib.h']) )
        exts.append( Extension('_sha512', ['sha512module.c'],
                               depends=['hashlib.h']) )
        exts.append( Extension('_md5', ['md5module.c'],
                               depends=['hashlib.h']) )
        exts.append( Extension('_sha1', ['sha1module.c'],
                               depends=['hashlib.h']) )

        blake2_deps = glob(os.path.join(os.getcwd(), srcdir,
                                        'Modules/_blake2/impl/*'))
        blake2_deps.append('hashlib.h')

        exts.append( Extension('_blake2',
                               ['_blake2/blake2module.c',
                                '_blake2/blake2b_impl.c',
                                '_blake2/blake2s_impl.c'],
                               depends=blake2_deps) )

        sha3_deps = glob(os.path.join(os.getcwd(), srcdir,
                                      'Modules/_sha3/kcp/*'))
        sha3_deps.append('hashlib.h')
        exts.append( Extension('_sha3',
                               ['_sha3/sha3module.c'],
                               depends=sha3_deps))

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

        if cross_compiling:
            db_inc_paths = []

        # Add some common subdirectories for Sleepycat DB to the list,
        # based on the standard include directories. This way DB3/4 gets
        # picked up when it is installed in a non-standard prefix and
        # the user has added that prefix into inc_dirs.
        std_variants = []
        for dn in inc_dirs:
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

        if host_platform == 'darwin':
            sysroot = macosx_sdk_root()

        class db_found(Exception): pass
        try:
            # See whether there is a Sleepycat header in the standard
            # search path.
            for d in inc_dirs + db_inc_paths:
                f = os.path.join(d, "db.h")
                if host_platform == 'darwin' and is_macosx_sdk_path(d):
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

                if host_platform != 'darwin':
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
                                    db_dirs_to_check + lib_dirs, dblib )
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
            if db_incdir in inc_dirs:
                db_incs = None
            else:
                db_incs = [db_incdir]
            if dblib_dir[0] in lib_dirs:
                dblib_dir = None
        else:
            if db_setup_debug: print("db: no appropriate library found")
            db_incs = None
            dblibs = []
            dblib_dir = None

        # The sqlite interface
        sqlite_setup_debug = False   # verbose debug prints from this script?

        # We hunt for #define SQLITE_VERSION "n.n.n"
        # We need to find >= sqlite version 3.3.9, for sqlite3_prepare_v2
        sqlite_incdir = sqlite_libdir = None
        sqlite_inc_paths = [ '/usr/include',
                             '/usr/include/sqlite',
                             '/usr/include/sqlite3',
                             '/usr/local/include',
                             '/usr/local/include/sqlite',
                             '/usr/local/include/sqlite3',
                             ]
        if cross_compiling:
            sqlite_inc_paths = []
        MIN_SQLITE_VERSION_NUMBER = (3, 3, 9)
        MIN_SQLITE_VERSION = ".".join([str(x)
                                    for x in MIN_SQLITE_VERSION_NUMBER])

        # Scan the default include directories before the SQLite specific
        # ones. This allows one to override the copy of sqlite on OSX,
        # where /usr/include contains an old version of sqlite.
        if host_platform == 'darwin':
            sysroot = macosx_sdk_root()

        for d_ in inc_dirs + sqlite_inc_paths:
            d = d_
            if host_platform == 'darwin' and is_macosx_sdk_path(d):
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
                                sqlite_dirs_to_check + lib_dirs, 'sqlite3')
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
            if host_platform != "win32":
                sqlite_defines.append(('MODULE_NAME', '"sqlite3"'))
            else:
                sqlite_defines.append(('MODULE_NAME', '\\"sqlite3\\"'))

            # Enable support for loadable extensions in the sqlite3 module
            # if --enable-loadable-sqlite-extensions configure option is used.
            if '--enable-loadable-sqlite-extensions' not in sysconfig.get_config_var("CONFIG_ARGS"):
                sqlite_defines.append(("SQLITE_OMIT_LOAD_EXTENSION", "1"))

            if host_platform == 'darwin':
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
            if sqlite_libdir and sqlite_libdir[0] in lib_dirs:
                sqlite_libdir = None
            exts.append(Extension('_sqlite3', sqlite_srcs,
                                  define_macros=sqlite_defines,
                                  include_dirs=include_dirs,
                                  library_dirs=sqlite_libdir,
                                  extra_link_args=sqlite_extra_link_args,
                                  libraries=["sqlite3",]))
        else:
            missing.append('_sqlite3')

        dbm_setup_debug = False   # verbose debug prints from this script?
        dbm_order = ['gdbm']
        # The standard Unix dbm module:
        if host_platform not in ['cygwin']:
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
                    if find_file("ndbm.h", inc_dirs, []) is not None:
                        # Some systems have -lndbm, others have -lgdbm_compat,
                        # others don't have either
                        if self.compiler.find_library_file(lib_dirs,
                                                               'ndbm'):
                            ndbm_libs = ['ndbm']
                        elif self.compiler.find_library_file(lib_dirs,
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
                    if self.compiler.find_library_file(lib_dirs, 'gdbm'):
                        gdbm_libs = ['gdbm']
                        if self.compiler.find_library_file(lib_dirs,
                                                               'gdbm_compat'):
                            gdbm_libs.append('gdbm_compat')
                        if find_file("gdbm/ndbm.h", inc_dirs, []) is not None:
                            if dbm_setup_debug: print("building dbm using gdbm")
                            dbmext = Extension(
                                '_dbm', ['_dbmmodule.c'],
                                define_macros=[
                                    ('HAVE_GDBM_NDBM_H', None),
                                    ],
                                libraries = gdbm_libs)
                            break
                        if find_file("gdbm-ndbm.h", inc_dirs, []) is not None:
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
                exts.append(dbmext)
            else:
                missing.append('_dbm')

        # Anthony Baxter's gdbm module.  GNU dbm(3) will require -lgdbm:
        if ('gdbm' in dbm_order and
            self.compiler.find_library_file(lib_dirs, 'gdbm')):
            exts.append( Extension('_gdbm', ['_gdbmmodule.c'],
                                   libraries = ['gdbm'] ) )
        else:
            missing.append('_gdbm')

        # Unix-only modules
        if host_platform != 'win32':
            # Steen Lumholt's termios module
            exts.append( Extension('termios', ['termios.c']) )
            # Jeremy Hylton's rlimit interface
            exts.append( Extension('resource', ['resource.c']) )
        else:
            missing.extend(['resource', 'termios'])

        nis = self._detect_nis(inc_dirs, lib_dirs)
        if nis is not None:
            exts.append(nis)
        else:
            missing.append('nis')

        # Curses support, requiring the System V version of curses, often
        # provided by the ncurses library.
        curses_defines = []
        curses_includes = []
        panel_library = 'panel'
        if curses_library == 'ncursesw':
            curses_defines.append(('HAVE_NCURSESW', '1'))
            if not cross_compiling:
                curses_includes.append('/usr/include/ncursesw')
            # Bug 1464056: If _curses.so links with ncursesw,
            # _curses_panel.so must link with panelw.
            panel_library = 'panelw'
            if host_platform == 'darwin':
                # On OS X, there is no separate /usr/lib/libncursesw nor
                # libpanelw.  If we are here, we found a locally-supplied
                # version of libncursesw.  There should also be a
                # libpanelw.  _XOPEN_SOURCE defines are usually excluded
                # for OS X but we need _XOPEN_SOURCE_EXTENDED here for
                # ncurses wide char support
                curses_defines.append(('_XOPEN_SOURCE_EXTENDED', '1'))
        elif host_platform == 'darwin' and curses_library == 'ncurses':
            # Building with the system-suppied combined libncurses/libpanel
            curses_defines.append(('HAVE_NCURSESW', '1'))
            curses_defines.append(('_XOPEN_SOURCE_EXTENDED', '1'))

        if curses_library.startswith('ncurses'):
            curses_libs = [curses_library]
            exts.append( Extension('_curses', ['_cursesmodule.c'],
                                   include_dirs=curses_includes,
                                   define_macros=curses_defines,
                                   libraries = curses_libs) )
        elif curses_library == 'curses' and host_platform != 'darwin':
                # OSX has an old Berkeley curses, not good enough for
                # the _curses module.
            if (self.compiler.find_library_file(lib_dirs, 'terminfo')):
                curses_libs = ['curses', 'terminfo']
            elif (self.compiler.find_library_file(lib_dirs, 'termcap')):
                curses_libs = ['curses', 'termcap']
            else:
                curses_libs = ['curses']

            exts.append( Extension('_curses', ['_cursesmodule.c'],
                                   define_macros=curses_defines,
                                   libraries = curses_libs) )
        else:
            missing.append('_curses')

        # If the curses module is enabled, check for the panel module
        if (module_enabled(exts, '_curses') and
            self.compiler.find_library_file(lib_dirs, panel_library)):
            exts.append( Extension('_curses_panel', ['_curses_panel.c'],
                                   include_dirs=curses_includes,
                                   define_macros=curses_defines,
                                   libraries = [panel_library] + curses_libs) )
        else:
            missing.append('_curses_panel')

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
        zlib_inc = find_file('zlib.h', [], inc_dirs)
        have_zlib = False
        if zlib_inc is not None:
            zlib_h = zlib_inc[0] + '/zlib.h'
            version = '"0.0.0"'
            version_req = '"1.1.3"'
            if host_platform == 'darwin' and is_macosx_sdk_path(zlib_h):
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
                if (self.compiler.find_library_file(lib_dirs, 'z')):
                    if host_platform == "darwin":
                        zlib_extra_link_args = ('-Wl,-search_paths_first',)
                    else:
                        zlib_extra_link_args = ()
                    exts.append( Extension('zlib', ['zlibmodule.c'],
                                           libraries = ['z'],
                                           extra_link_args = zlib_extra_link_args))
                    have_zlib = True
                else:
                    missing.append('zlib')
            else:
                missing.append('zlib')
        else:
            missing.append('zlib')

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
        exts.append( Extension('binascii', ['binascii.c'],
                               extra_compile_args = extra_compile_args,
                               libraries = libraries,
                               extra_link_args = extra_link_args) )

        # Gustavo Niemeyer's bz2 module.
        if (self.compiler.find_library_file(lib_dirs, 'bz2')):
            if host_platform == "darwin":
                bz2_extra_link_args = ('-Wl,-search_paths_first',)
            else:
                bz2_extra_link_args = ()
            exts.append( Extension('_bz2', ['_bz2module.c'],
                                   libraries = ['bz2'],
                                   extra_link_args = bz2_extra_link_args) )
        else:
            missing.append('_bz2')

        # LZMA compression support.
        if self.compiler.find_library_file(lib_dirs, 'lzma'):
            exts.append( Extension('_lzma', ['_lzmamodule.c'],
                                   libraries = ['lzma']) )
        else:
            missing.append('_lzma')

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
            expat_inc = [os.path.join(os.getcwd(), srcdir, 'Modules', 'expat')]
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
            ret = os.system(
                      '"%s" -Werror -Wno-unreachable-code -E -xc /dev/null >/dev/null 2>&1' % cc)
            if ret >> 8 == 0:
                extra_compile_args.append('-Wno-unreachable-code')

        exts.append(Extension('pyexpat',
                              define_macros = define_macros,
                              extra_compile_args = extra_compile_args,
                              include_dirs = expat_inc,
                              libraries = expat_lib,
                              sources = ['pyexpat.c'] + expat_sources,
                              depends = expat_depends,
                              ))

        # Fredrik Lundh's cElementTree module.  Note that this also
        # uses expat (via the CAPI hook in pyexpat).

        if os.path.isfile(os.path.join(srcdir, 'Modules', '_elementtree.c')):
            define_macros.append(('USE_PYEXPAT_CAPI', None))
            exts.append(Extension('_elementtree',
                                  define_macros = define_macros,
                                  include_dirs = expat_inc,
                                  libraries = expat_lib,
                                  sources = ['_elementtree.c'],
                                  depends = ['pyexpat.c'] + expat_sources +
                                      expat_depends,
                                  ))
        else:
            missing.append('_elementtree')

        # Hye-Shik Chang's CJKCodecs modules.
        exts.append(Extension('_multibytecodec',
                              ['cjkcodecs/multibytecodec.c']))
        for loc in ('kr', 'jp', 'cn', 'tw', 'hk', 'iso2022'):
            exts.append(Extension('_codecs_%s' % loc,
                                  ['cjkcodecs/_codecs_%s.c' % loc]))

        # Stefan Krah's _decimal module
        exts.append(self._decimal_ext())

        # Thomas Heller's _ctypes module
        self.detect_ctypes(inc_dirs, lib_dirs)

        # Richard Oudkerk's multiprocessing module
        if host_platform == 'win32':        # Windows
            macros = dict()
            libraries = ['ws2_32']

        elif host_platform == 'darwin':     # Mac OSX
            macros = dict()
            libraries = []

        elif host_platform == 'cygwin':     # Cygwin
            macros = dict()
            libraries = []

        elif host_platform.startswith('openbsd'):
            macros = dict()
            libraries = []

        elif host_platform.startswith('netbsd'):
            macros = dict()
            libraries = []

        else:                                   # Linux and other unices
            macros = dict()
            libraries = ['rt']

        if host_platform == 'win32':
            multiprocessing_srcs = [ '_multiprocessing/multiprocessing.c',
                                     '_multiprocessing/semaphore.c',
                                   ]

        else:
            multiprocessing_srcs = [ '_multiprocessing/multiprocessing.c',
                                   ]
            if (sysconfig.get_config_var('HAVE_SEM_OPEN') and not
                sysconfig.get_config_var('POSIX_SEMAPHORES_NOT_ENABLED')):
                multiprocessing_srcs.append('_multiprocessing/semaphore.c')

        exts.append ( Extension('_multiprocessing', multiprocessing_srcs,
                                define_macros=list(macros.items()),
                                include_dirs=["Modules/_multiprocessing"]))
        # End multiprocessing

        # Platform-specific libraries
        if host_platform.startswith(('linux', 'freebsd', 'gnukfreebsd')):
            exts.append( Extension('ossaudiodev', ['ossaudiodev.c']) )
        else:
            missing.append('ossaudiodev')

        if host_platform == 'darwin':
            exts.append(
                       Extension('_scproxy', ['_scproxy.c'],
                       extra_link_args=[
                           '-framework', 'SystemConfiguration',
                           '-framework', 'CoreFoundation',
                        ]))

        self.extensions.extend(exts)

        # Call the method for detecting whether _tkinter can be compiled
        self.detect_tkinter(inc_dirs, lib_dirs)

        if '_tkinter' not in [e.name for e in self.extensions]:
            missing.append('_tkinter')

        # Build the _uuid module if possible
        uuid_incs = find_file("uuid.h", inc_dirs, ["/usr/include/uuid"])
        if uuid_incs is not None:
            if self.compiler.find_library_file(lib_dirs, 'uuid'):
                uuid_libs = ['uuid']
            else:
                uuid_libs = []
            self.extensions.append(Extension('_uuid', ['_uuidmodule.c'],
                                   libraries=uuid_libs,
                                   include_dirs=uuid_incs))
        else:
            missing.append('_uuid')

##         # Uncomment these lines if you want to play with xxmodule.c
##         ext = Extension('xx', ['xxmodule.c'])
##         self.extensions.append(ext)

        if 'd' not in sysconfig.get_config_var('ABIFLAGS'):
            ext = Extension('xxlimited', ['xxlimited.c'],
                            define_macros=[('Py_LIMITED_API', '0x03050000')])
            self.extensions.append(ext)

        return missing

    def detect_tkinter_explicitly(self):
        # Build _tkinter using explicit locations for Tcl/Tk.
        #
        # This is enabled when both arguments are given to ./configure:
        #
        #     --with-tcltk-includes="-I/path/to/tclincludes \
        #                            -I/path/to/tkincludes"
        #     --with-tcltk-libs="-L/path/to/tcllibs -ltclm.n \
        #                        -L/path/to/tklibs -ltkm.n"
        #
        # These values can also be specified or overridden via make:
        #    make TCLTK_INCLUDES="..." TCLTK_LIBS="..."
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
            return 0

        extra_compile_args = tcltk_includes.split()
        extra_link_args = tcltk_libs.split()
        ext = Extension('_tkinter', ['_tkinter.c', 'tkappinit.c'],
                        define_macros=[('WITH_APPINIT', 1)],
                        extra_compile_args = extra_compile_args,
                        extra_link_args = extra_link_args,
                        )
        self.extensions.append(ext)
        return 1

    def detect_tkinter_darwin(self, inc_dirs, lib_dirs):
        # The _tkinter module, using frameworks. Since frameworks are quite
        # different the UNIX search logic is not sharable.
        from os.path import join, exists
        framework_dirs = [
            '/Library/Frameworks',
            '/System/Library/Frameworks/',
            join(os.getenv('HOME'), '/Library/Frameworks')
        ]

        sysroot = macosx_sdk_root()

        # Find the directory that contains the Tcl.framework and Tk.framework
        # bundles.
        # XXX distutils should support -F!
        for F in framework_dirs:
            # both Tcl.framework and Tk.framework should be present


            for fw in 'Tcl', 'Tk':
                if is_macosx_sdk_path(F):
                    if not exists(join(sysroot, F[1:], fw + '.framework')):
                        break
                else:
                    if not exists(join(F, fw + '.framework')):
                        break
            else:
                # ok, F is now directory with both frameworks. Continure
                # building
                break
        else:
            # Tk and Tcl frameworks not found. Normal "unix" tkinter search
            # will now resume.
            return 0

        # For 8.4a2, we must add -I options that point inside the Tcl and Tk
        # frameworks. In later release we should hopefully be able to pass
        # the -F option to gcc, which specifies a framework lookup path.
        #
        include_dirs = [
            join(F, fw + '.framework', H)
            for fw in ('Tcl', 'Tk')
            for H in ('Headers', 'Versions/Current/PrivateHeaders')
        ]

        # For 8.4a2, the X11 headers are not included. Rather than include a
        # complicated search, this is a hard-coded path. It could bail out
        # if X11 libs are not found...
        include_dirs.append('/usr/X11R6/include')
        frameworks = ['-framework', 'Tcl', '-framework', 'Tk']

        # All existing framework builds of Tcl/Tk don't support 64-bit
        # architectures.
        cflags = sysconfig.get_config_vars('CFLAGS')[0]
        archs = re.findall(r'-arch\s+(\w+)', cflags)

        tmpfile = os.path.join(self.build_temp, 'tk.arch')
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        # Note: cannot use os.popen or subprocess here, that
        # requires extensions that are not available here.
        if is_macosx_sdk_path(F):
            os.system("file %s/Tk.framework/Tk | grep 'for architecture' > %s"%(os.path.join(sysroot, F[1:]), tmpfile))
        else:
            os.system("file %s/Tk.framework/Tk | grep 'for architecture' > %s"%(F, tmpfile))

        with open(tmpfile) as fp:
            detected_archs = []
            for ln in fp:
                a = ln.split()[-1]
                if a in archs:
                    detected_archs.append(ln.split()[-1])
        os.unlink(tmpfile)

        for a in detected_archs:
            frameworks.append('-arch')
            frameworks.append(a)

        ext = Extension('_tkinter', ['_tkinter.c', 'tkappinit.c'],
                        define_macros=[('WITH_APPINIT', 1)],
                        include_dirs = include_dirs,
                        libraries = [],
                        extra_compile_args = frameworks[2:],
                        extra_link_args = frameworks,
                        )
        self.extensions.append(ext)
        return 1

    def detect_tkinter(self, inc_dirs, lib_dirs):
        # The _tkinter module.

        # Check whether --with-tcltk-includes and --with-tcltk-libs were
        # configured or passed into the make target.  If so, use these values
        # to build tkinter and bypass the searches for Tcl and TK in standard
        # locations.
        if self.detect_tkinter_explicitly():
            return

        # Rather than complicate the code below, detecting and building
        # AquaTk is a separate method. Only one Tkinter will be built on
        # Darwin - either AquaTk, if it is found, or X11 based Tk.
        if (host_platform == 'darwin' and
            self.detect_tkinter_darwin(inc_dirs, lib_dirs)):
            return

        # Assume we haven't found any of the libraries or include files
        # The versions with dots are used on Unix, and the versions without
        # dots on Windows, for detection by cygwin.
        tcllib = tklib = tcl_includes = tk_includes = None
        for version in ['8.6', '86', '8.5', '85', '8.4', '84', '8.3', '83',
                        '8.2', '82', '8.1', '81', '8.0', '80']:
            tklib = self.compiler.find_library_file(lib_dirs,
                                                        'tk' + version)
            tcllib = self.compiler.find_library_file(lib_dirs,
                                                         'tcl' + version)
            if tklib and tcllib:
                # Exit the loop when we've found the Tcl/Tk libraries
                break

        # Now check for the header files
        if tklib and tcllib:
            # Check for the include files on Debian and {Free,Open}BSD, where
            # they're put in /usr/include/{tcl,tk}X.Y
            dotversion = version
            if '.' not in dotversion and "bsd" in host_platform.lower():
                # OpenBSD and FreeBSD use Tcl/Tk library names like libtcl83.a,
                # but the include subdirs are named like .../include/tcl8.3.
                dotversion = dotversion[:-1] + '.' + dotversion[-1]
            tcl_include_sub = []
            tk_include_sub = []
            for dir in inc_dirs:
                tcl_include_sub += [dir + os.sep + "tcl" + dotversion]
                tk_include_sub += [dir + os.sep + "tk" + dotversion]
            tk_include_sub += tcl_include_sub
            tcl_includes = find_file('tcl.h', inc_dirs, tcl_include_sub)
            tk_includes = find_file('tk.h', inc_dirs, tk_include_sub)

        if (tcllib is None or tklib is None or
            tcl_includes is None or tk_includes is None):
            self.announce("INFO: Can't locate Tcl/Tk libs and/or headers", 2)
            return

        # OK... everything seems to be present for Tcl/Tk.

        include_dirs = [] ; libs = [] ; defs = [] ; added_lib_dirs = []
        for dir in tcl_includes + tk_includes:
            if dir not in include_dirs:
                include_dirs.append(dir)

        # Check for various platform-specific directories
        if host_platform == 'sunos5':
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
        if host_platform == 'cygwin':
            x11_inc = find_file('X11/Xlib.h', [], include_dirs)
            if x11_inc is None:
                return

        # Check for BLT extension
        if self.compiler.find_library_file(lib_dirs + added_lib_dirs,
                                               'BLT8.0'):
            defs.append( ('WITH_BLT', 1) )
            libs.append('BLT8.0')
        elif self.compiler.find_library_file(lib_dirs + added_lib_dirs,
                                                'BLT'):
            defs.append( ('WITH_BLT', 1) )
            libs.append('BLT')

        # Add the Tcl/Tk libraries
        libs.append('tk'+ version)
        libs.append('tcl'+ version)

        if host_platform in ['aix3', 'aix4']:
            libs.append('ld')

        # Finally, link with the X11 libraries (not appropriate on cygwin)
        if host_platform != "cygwin":
            libs.append('X11')

        ext = Extension('_tkinter', ['_tkinter.c', 'tkappinit.c'],
                        define_macros=[('WITH_APPINIT', 1)] + defs,
                        include_dirs = include_dirs,
                        libraries = libs,
                        library_dirs = added_lib_dirs,
                        )
        self.extensions.append(ext)

        # XXX handle these, but how to detect?
        # *** Uncomment and edit for PIL (TkImaging) extension only:
        #       -DWITH_PIL -I../Extensions/Imaging/libImaging  tkImaging.c \
        # *** Uncomment and edit for TOGL extension only:
        #       -DWITH_TOGL togl.c \
        # *** Uncomment these for TOGL extension only:
        #       -lGL -lGLU -lXext -lXmu \

    def configure_ctypes_darwin(self, ext):
        # Darwin (OS X) uses preconfigured files, in
        # the Modules/_ctypes/libffi_osx directory.
        srcdir = sysconfig.get_config_var('srcdir')
        ffi_srcdir = os.path.abspath(os.path.join(srcdir, 'Modules',
                                                  '_ctypes', 'libffi_osx'))
        sources = [os.path.join(ffi_srcdir, p)
                   for p in ['ffi.c',
                             'x86/darwin64.S',
                             'x86/x86-darwin.S',
                             'x86/x86-ffi_darwin.c',
                             'x86/x86-ffi64.c',
                             'powerpc/ppc-darwin.S',
                             'powerpc/ppc-darwin_closure.S',
                             'powerpc/ppc-ffi_darwin.c',
                             'powerpc/ppc64-darwin_closure.S',
                             ]]

        # Add .S (preprocessed assembly) to C compiler source extensions.
        self.compiler.src_extensions.append('.S')

        include_dirs = [os.path.join(ffi_srcdir, 'include'),
                        os.path.join(ffi_srcdir, 'powerpc')]
        ext.include_dirs.extend(include_dirs)
        ext.sources.extend(sources)
        return True

    def configure_ctypes(self, ext):
        if not self.use_system_libffi:
            if host_platform == 'darwin':
                return self.configure_ctypes_darwin(ext)
            print('INFO: Could not locate ffi libs and/or headers')
            return False
        return True

    def detect_ctypes(self, inc_dirs, lib_dirs):
        self.use_system_libffi = False
        include_dirs = []
        extra_compile_args = []
        extra_link_args = []
        sources = ['_ctypes/_ctypes.c',
                   '_ctypes/callbacks.c',
                   '_ctypes/callproc.c',
                   '_ctypes/stgdict.c',
                   '_ctypes/cfield.c']
        depends = ['_ctypes/ctypes.h']

        if host_platform == 'darwin':
            sources.append('_ctypes/malloc_closure.c')
            sources.append('_ctypes/darwin/dlfcn_simple.c')
            extra_compile_args.append('-DMACOSX')
            include_dirs.append('_ctypes/darwin')
# XXX Is this still needed?
##            extra_link_args.extend(['-read_only_relocs', 'warning'])

        elif host_platform == 'sunos5':
            # XXX This shouldn't be necessary; it appears that some
            # of the assembler code is non-PIC (i.e. it has relocations
            # when it shouldn't. The proper fix would be to rewrite
            # the assembler code to be PIC.
            # This only works with GCC; the Sun compiler likely refuses
            # this option. If you want to compile ctypes with the Sun
            # compiler, please research a proper solution, instead of
            # finding some -z option for the Sun compiler.
            extra_link_args.append('-mimpure-text')

        elif host_platform.startswith('hp-ux'):
            extra_link_args.append('-fPIC')

        ext = Extension('_ctypes',
                        include_dirs=include_dirs,
                        extra_compile_args=extra_compile_args,
                        extra_link_args=extra_link_args,
                        libraries=[],
                        sources=sources,
                        depends=depends)
        # function my_sqrt() needs libm for sqrt()
        ext_test = Extension('_ctypes_test',
                     sources=['_ctypes/_ctypes_test.c'],
                     libraries=['m'])
        self.extensions.extend([ext, ext_test])

        if host_platform == 'darwin':
            if '--with-system-ffi' not in sysconfig.get_config_var("CONFIG_ARGS"):
                return
            # OS X 10.5 comes with libffi.dylib; the include files are
            # in /usr/include/ffi
            inc_dirs.append('/usr/include/ffi')

        ffi_inc = [sysconfig.get_config_var("LIBFFI_INCLUDEDIR")]
        if not ffi_inc or ffi_inc[0] == '':
            ffi_inc = find_file('ffi.h', [], inc_dirs)
        if ffi_inc is not None:
            ffi_h = ffi_inc[0] + '/ffi.h'
            if not os.path.exists(ffi_h):
                ffi_inc = None
                print('Header file {} does not exist'.format(ffi_h))
        ffi_lib = None
        if ffi_inc is not None:
            for lib_name in ('ffi', 'ffi_pic'):
                if (self.compiler.find_library_file(lib_dirs, lib_name)):
                    ffi_lib = lib_name
                    break

        if ffi_inc and ffi_lib:
            ext.include_dirs.extend(ffi_inc)
            ext.libraries.append(ffi_lib)
            self.use_system_libffi = True

        if sysconfig.get_config_var('HAVE_LIBDL'):
            # for dlopen, see bpo-32647
            ext.libraries.append('dl')

    def _decimal_ext(self):
        extra_compile_args = []
        undef_macros = []
        if '--with-system-libmpdec' in sysconfig.get_config_var("CONFIG_ARGS"):
            include_dirs = []
            libraries = [':libmpdec.so.2']
            sources = ['_decimal/_decimal.c']
            depends = ['_decimal/docstrings.h']
        else:
            srcdir = sysconfig.get_config_var('srcdir')
            include_dirs = [os.path.abspath(os.path.join(srcdir,
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
              '_decimal/libmpdec/memory.c',
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
        elif host_platform == 'darwin':
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
               not 'sunos' in host_platform:
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
        ext = Extension (
            '_decimal',
            include_dirs=include_dirs,
            libraries=libraries,
            define_macros=define_macros,
            undef_macros=undef_macros,
            extra_compile_args=extra_compile_args,
            sources=sources,
            depends=depends
        )
        return ext

    def _detect_openssl(self, inc_dirs, lib_dirs):
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
        if not openssl_libs:
            # libssl and libcrypto not found
            return None, None

        # Find OpenSSL includes
        ssl_incs = find_file(
            'openssl/ssl.h', inc_dirs, openssl_includes
        )
        if ssl_incs is None:
            return None, None

        # OpenSSL 1.0.2 uses Kerberos for KRB5 ciphers
        krb5_h = find_file(
            'krb5.h', inc_dirs,
            ['/usr/kerberos/include']
        )
        if krb5_h:
            ssl_incs.extend(krb5_h)

        if config_vars.get("HAVE_X509_VERIFY_PARAM_SET1_HOST"):
            ssl_ext = Extension(
                '_ssl', ['_ssl.c'],
                include_dirs=openssl_includes,
                library_dirs=openssl_libdirs,
                libraries=openssl_libs,
                depends=['socketmodule.h']
            )
        else:
            ssl_ext = None

        hashlib_ext = Extension(
            '_hashlib', ['_hashopenssl.c'],
            depends=['hashlib.h'],
            include_dirs=openssl_includes,
            library_dirs=openssl_libdirs,
            libraries=openssl_libs,
        )

        return ssl_ext, hashlib_ext

    def _detect_nis(self, inc_dirs, lib_dirs):
        if host_platform in {'win32', 'cygwin', 'qnx6'}:
            return None

        libs = []
        library_dirs = []
        includes_dirs = []

        # bpo-32521: glibc has deprecated Sun RPC for some time. Fedora 28
        # moved headers and libraries to libtirpc and libnsl. The headers
        # are in tircp and nsl sub directories.
        rpcsvc_inc = find_file(
            'rpcsvc/yp_prot.h', inc_dirs,
            [os.path.join(inc_dir, 'nsl') for inc_dir in inc_dirs]
        )
        rpc_inc = find_file(
            'rpc/rpc.h', inc_dirs,
            [os.path.join(inc_dir, 'tirpc') for inc_dir in inc_dirs]
        )
        if rpcsvc_inc is None or rpc_inc is None:
            # not found
            return None
        includes_dirs.extend(rpcsvc_inc)
        includes_dirs.extend(rpc_inc)

        if self.compiler.find_library_file(lib_dirs, 'nsl'):
            libs.append('nsl')
        else:
            # libnsl-devel: check for libnsl in nsl/ subdirectory
            nsl_dirs = [os.path.join(lib_dir, 'nsl') for lib_dir in lib_dirs]
            libnsl = self.compiler.find_library_file(nsl_dirs, 'nsl')
            if libnsl is not None:
                library_dirs.append(os.path.dirname(libnsl))
                libs.append('nsl')

        if self.compiler.find_library_file(lib_dirs, 'tirpc'):
            libs.append('tirpc')

        return Extension(
            'nis', ['nismodule.c'],
            libraries=libs,
            library_dirs=library_dirs,
            include_dirs=includes_dirs
        )


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
        if not self.is_chmod_supported(): return
        if not files: return

        for filename in files:
            if os.path.islink(filename): continue
            mode = defaultMode
            if filename.endswith(self.shlib_suffix): mode = sharedLibMode
            log.info("changing mode of %s to %o", filename, mode)
            if not self.dry_run: os.chmod(filename, mode)

    def set_dir_modes(self, dirname, mode):
        if not self.is_chmod_supported(): return
        for dirpath, dirnames, fnames in os.walk(dirname):
            if os.path.islink(dirpath):
                continue
            log.info("changing mode of %s to %o", dirpath, mode)
            if not self.dry_run: os.chmod(dirpath, mode)

    def is_chmod_supported(self):
        return hasattr(os, 'chmod')

class PyBuildScripts(build_scripts):
    def copy_scripts(self):
        outfiles, updated_files = build_scripts.copy_scripts(self)
        fullversion = '-{0[0]}.{0[1]}'.format(sys.version_info)
        minoronly = '.{0[1]}'.format(sys.version_info)
        newoutfiles = []
        newupdated_files = []
        for filename in outfiles:
            if filename.endswith(('2to3', 'pyvenv')):
                newfilename = filename + fullversion
            else:
                newfilename = filename + minoronly
            log.info('renaming %s to %s', filename, newfilename)
            os.rename(filename, newfilename)
            newoutfiles.append(newfilename)
            if filename in updated_files:
                newupdated_files.append(newfilename)
        return newoutfiles, newupdated_files

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

def main():
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
                     "Tools/scripts/2to3", "Tools/scripts/pyvenv"]
        )

# --install-platlib
if __name__ == '__main__':
    main()
