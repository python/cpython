# Compute the Python Path Configuration on Unix.

import posix
from _stat import S_ISREG, S_ISDIR
import _cgetpath
import sys

ENV_CFG = "pyvenv.cfg"
LANDMARK = "os.py"
BUILD_LANDMARK = "Modules/Setup.local"

PY_MAJOR_VERSION = sys.version_info.major
PY_MINOR_VERSION = sys.version_info.minor

SEP = _cgetpath.SEP
DELIM = _cgetpath.DELIM
WITH_NEXT_FRAMEWORK = _cgetpath.WITH_NEXT_FRAMEWORK
__APPLE__ = (sys.platform == 'darwin')
MS_WINDOWS = (sys.platform == 'win32')
# FIXME: Export these constants in _cgetpath
__CYGWIN__ = False
__MINGW32__ = False


# Py_DecodeLocale() function: decode a byte string from the filesystem encoding
# and error handler.
decode_locale = _cgetpath.decode_locale


PYTHONPATH = decode_locale(_cgetpath.PYTHONPATH)
PREFIX = decode_locale(_cgetpath.PREFIX)
EXEC_PREFIX = decode_locale(_cgetpath.EXEC_PREFIX)
VPATH = decode_locale(_cgetpath.VPATH)
VERSION = decode_locale(_cgetpath.VERSION)


def isabs(path):
    # an empty path is not considered as absolute
    return (path and path.startswith(SEP))


def dirname(path):
    # FIXME: use str methods
    i = len(path) - 1
    while i > 0 and path[i] != SEP:
        i -= 1
    return path[:i]


# FIXME: support more than 2 arguments
def joinpath(path, path2):
    if isabs(path2):
        return path2

    if not path:
        return path2

    if not path.endswith(SEP):
        return f"{path}{SEP}{path2}"
    else:
        return path + path2


# Is a file, not a directory?
def isfile(filename):
    try:
        st = posix.stat(filename)
    except OSError:
        return False
    if not S_ISREG(st.st_mode):
        return False
    return True


# Is executable file?
def isxfile(filename):
    try:
        st = posix.stat(filename)
    except OSError:
        return False
    if not S_ISREG(st.st_mode):
        return False
    return ((st.st_mode & 0o111) != 0)


# Is a directory?
def isdir(filename):
    try:
        st = posix.stat(filename)
    except OSError:
        return False
    return S_ISDIR(st.st_mode)


# Is a module? Check for .pyc too.
def ismodule(path):
    filename = joinpath(path, LANDMARK)
    if isfile(filename):
        return True

    # Check for the compiled version of prefix.
    return isfile(filename + 'c')

def abspath(path):
    if isabs(path):
        return path

    try:
        cwd = posix.getcwd()
    except OSError:
        return path

    # ./foo => foo
    path = path.removeprefix(f'.{SEP}')
    return joinpath(cwd, path)


if __CYGWIN__ or __MINGW32__:
    EXE_SUFFIX = ".exe"

    def add_exe_suffix(progpath):
        # Check for already have an executable suffix
        if progpath[-len(EXE_SUFFIX):].lower() == EXE_SUFFIX:
            return progpath

        progpath_exe = progpath + EXE_SUFFIX
        if isxfile(progpath_exe):
            return progpath_exe

        return progpath


# Similar to realpath() but without the normalization.
def resolve_symlinks(path):
    nlink = 0
    while True:
        try:
            new_path = posix.readlink(path)
        except OSError:
            # Not a symbolic link: we are done.
            break

        if isabs(new_path):
            path = new_path
        else:
            # new_path is relative to path
            path = joinpath(dirname(path), new_path)

        nlink += 1
        # 40 is the Linux kernel 4.2 limit
        if nlink >= 40:
            raise Exception("maximum number of symbolic links reached")

    return path


class PathConfig:
    def __init__(self):
        self.module_search_path = None
        self.program_full_path = None
        self.prefix = None
        self.exec_prefix = None
        self.program_name = None
        self.home = None
        if MS_WINDOWS:
            self.base_executable = None

    def _get_config(self, config, attr, config_key=None):
        if config_key is None:
            config_key = attr
        setattr(self, attr, config[config_key])

    def set_from_config(self, config):
        if config['module_search_paths_set']:
            self.module_search_path = config['module_search_paths']
        self._get_config(config, 'program_full_path', 'executable')
        self._get_config(config, 'prefix')
        self._get_config(config, 'exec_prefix')
        self._get_config(config, 'program_name')
        self._get_config(config, 'home')
        if MS_WINDOWS:
            self._get_config(config, 'base_executable')

    def _set_config(self, config, attr, config_key=None):
        if config_key is None:
            config_key = attr
        if config[config_key] is not None:
            return
        config[config_key] = getattr(self, attr)

    def set_to_config(self, config):
        if not config['module_search_paths_set']:
            config['module_search_paths'] = self.module_search_path
            config['module_search_paths_set'] = 1

        if MS_WINDOWS:
            if config['executable'] is not None and config['base_executable'] is None:
                # If executable is set explicitly in the configuration,
                # ignore calculated base_executable: _PyConfig_InitPathConfig()
                # will copy executable to base_executable
                pass
            else:
                self._set_config(config, 'base_executable')

        self._set_config(config, 'program_full_path', 'executable')
        self._set_config(config, 'prefix')
        self._set_config(config, 'exec_prefix')

        if MS_WINDOWS:
            # If a ._pth file is found: isolated and site_import are overriden
            if self.isolated != -1:
                config['isolated'] = self.isolated
            if self.site_import != -1:
                config['site_import'] = self.site_import


def readline(fd):
    # FIXME: write better code
    # FIXME: support '\r' newline (macOS)
    nl = b'\n'
    buf = bytearray()
    while True:
        chunk = posix.read(fd, 4096)
        if not chunk:
            break
        if nl in chunk:
            chunk = chunk.split(nl, 1)[0]
            buf += chunk
            break
        buf += chunk
    return bytes(buf)


# Search for a prefix value in an environment file (pyvenv.cfg).
def find_env_config_value(fd, key):
    while True:
        line = readline(fd)
        if not line:
            break
        if line.startswith(b'#'):
            # Comment - skip
            continue

        line = line.decode('utf-8', 'surrogateescape')

        # FIXME: rewrite this code
        tok = line.split() # FIXME: only split at " \t\r\n"
        if tok[0] == key and tok[1] == '=':
            value = tok[2]
            return value


class CalculatePath:
    def __init__(self, pathconfig, config):
        self.pathconfig = pathconfig

        self.prefix_found = 0
        self.exec_prefix_found = 0

        self.warnings = config['pathconfig_warnings']
        self.pythonpath_env = config['pythonpath_env']
        self.platlibdir = config['platlibdir']
        path = posix.environ.get(b'PATH', None)
        if path is not None:
            self.path_env = decode_locale(path)
        else:
            self.path_env = None
        self.pythonpath_macro = PYTHONPATH
        self.prefix_macro = PREFIX
        self.exec_prefix_macro = EXEC_PREFIX
        self.vpath_macro = VPATH

        self.lib_python = joinpath(self.platlibdir, f"python{VERSION}")

    def calculate_program_macos(self):
        # On Mac OS X, if a script uses an interpreter of the form
        # "#!/opt/python2.3/bin/python", the kernel only passes "python"
        # as argv[0], which falls through to the $PATH search below.
        # If /opt/python2.3/bin isn't in your path, or is near the end,
        # this algorithm may incorrectly find /usr/bin/python. To work
        # around this, we can use _NSGetExecutablePath to get a better
        # hint of what the intended interpreter was, although this
        # will fail if a relative path was used. but in that case,
        # abspath() should help us out below
        exec_path = _cgetpath._NSGetExecutablePath()
        if not isabs(exec_path):
            return

        return exec_path

    def calculate_argv0_path_framework(self):
        mod_path = _cgetpath.NSLibraryName()
        if mod_path is None:
            return

        # We're in a framework.
        # See if we might be in the build directory. The framework in the
        # build directory is incomplete, it only has the .dylib and a few
        # needed symlinks, it doesn't have the Lib directories and such.
        # If we're running with the framework from the build directory we must
        # be running the interpreter in the build directory, so we use the
        # build-directory-specific logic to find Lib and such.

        lib_python = joinpath(dirname(mod_path), self.lib_python)
        if not ismodule(lib_python):
            # We are in the build directory so use the name of the
            # executable - we know that the absolute path is passed
            self.argv0_path = self.pathconfig.program_full_path
            return

        # Use the location of the library as argv0_path
        self.argv0_path = mod_path

    # Similar to shutil.which().
    @staticmethod
    def calculate_which(path_env, program_name):
        for path in path_env.split(DELIM):
            abs_path = joinpath(path, program_name)
            if isxfile(abs_path):
                return abs_path

        # not found
        return None

    def calculate_program_impl(self):
        pathconfig = self.pathconfig
        assert pathconfig.program_full_path is None

        # If there is no slash in the argv0 path, then we have to
        # assume python is on the user's $PATH, since there's no
        # other way to find a directory to start the search from.  If
        # $PATH isn't exported, you lose.
        if SEP in pathconfig.program_name:
            pathconfig.program_full_path = pathconfig.program_name
            return

        if __APPLE__:
            abs_path = self.calculate_program_macos()
            if abs_path is not None:
                pathconfig.program_full_path = abs_path
                return

        if self.path_env:
            abs_path = self.calculate_which(self.path_env, pathconfig.program_name)
            if abs_path is not None:
                pathconfig.program_full_path = abs_path
                return

        # In the last resort, use an empty string
        pathconfig.program_full_path = ""

    def calculate_program(self):
        pathconfig = self.pathconfig
        self.calculate_program_impl()

        if not pathconfig.program_full_path:
            return
        # program_full_path is not empty

        # Make sure that program_full_path is an absolute path
        pathconfig.program_full_path = abspath(pathconfig.program_full_path)

        if __CYGWIN__ or __MINGW32__:
            # For these platforms it is necessary to ensure that the .exe
            # suffix is appended to the filename, otherwise there is potential
            # for sys.executable to return the name of a directory under the
            # same path (bpo-28441).
            pathconfig.program_full_path = add_exe_suffix(pathconfig.program_full_path)

    def calculate_argv0_path(self):
        self.argv0_path = self.pathconfig.program_full_path

        if WITH_NEXT_FRAMEWORK:
            self.calculate_argv0_path_framework()

        self.argv0_path = resolve_symlinks(self.argv0_path)
        self.argv0_path = dirname(self.argv0_path)

    def calculate_open_pyenv(self):
        # Filename: <argv0_path> / "pyvenv.cfg"
        filename = joinpath(self.argv0_path, ENV_CFG)
        try:
            return posix.open(filename, posix.O_RDONLY)
        except OSError:
            pass

        # Path: <dirname(argv0_path)> / "pyvenv.cfg"
        filename = joinpath(dirname(self.argv0_path), ENV_CFG)
        try:
            return posix.open(filename, posix.O_RDONLY)
        except OSError:
            return None

    # Search for an "pyvenv.cfg" environment configuration file, first in the
    # executable's directory and then in the parent directory.
    # If found, open it for use when searching for prefixes.
    #
    # Write the 'home' variable of pyvenv.cfg into self.argv0_path.
    def calculate_read_pyenv(self):
        env_file = self.calculate_open_pyenv()
        if env_file is None:
            # pyvenv.cfg not found
            return

        # Look for a 'home' variable and set argv0_path to it, if found
        try:
            home = find_env_config_value(env_file, "home")
            if home is not None:
                self.argv0_path = home
        finally:
            posix.close(env_file)

    def search_for_prefix(self):
        # If PYTHONHOME is set, we believe it unconditionally
        home = self.pathconfig.home
        if home is not None:
            # Path: <home> / <lib_python>
            prefix = home.partition(DELIM)[0]
            prefix = joinpath(prefix, self.lib_python)
            self.prefix_found = 1
            return prefix

        # Check to see if argv0_path is in the build directory
        path = joinpath(self.argv0_path, BUILD_LANDMARK)
        if isfile(path):
            # argv0_path is the build directory (BUILD_LANDMARK exists),
            # now also check LANDMARK using ismodule().

            # Path: <argv0_path> / <VPATH macro> / Lib
            # or if VPATH is empty: <argv0_path> / Lib
            prefix = joinpath(self.argv0_path, self.vpath_macro)
            prefix = joinpath(prefix, "Lib")
            if ismodule(prefix):
                # BUILD_LANDMARK and LANDMARK found
                self.prefix_found = -1
                return prefix

        # Search from argv0_path, until root is found
        prefix = abspath(self.argv0_path)
        while prefix:
            # Path: <argv0_path or substring> / <lib_python> / LANDMARK
            new_prefix = joinpath(prefix, self.lib_python)
            if ismodule(new_prefix):
                self.prefix_found = 1
                return new_prefix

            prefix = dirname(prefix)

        # Look at configure's PREFIX.
        # Path: <PREFIX macro> / <lib_python> / LANDMARK
        prefix = joinpath(self.prefix_macro, self.lib_python)
        if ismodule(prefix):
            self.prefix_found = 1
            return prefix

        # Fail
        self.prefix_found = 0
        return None

    def calculate_prefix(self):
        prefix = self.search_for_prefix()
        if self.prefix_found == 0:
            if self.warnings:
                print("Could not find platform independent libraries <prefix>",
                      file=sys.stderr)
            self.prefix = joinpath(self.prefix_macro, self.lib_python)
        else:
            self.prefix = prefix

    def calculate_zip_path(self):
        path = joinpath(self.platlibdir,
                        f"python{PY_MAJOR_VERSION}{PY_MINOR_VERSION}.zip")
        if self.prefix_found > 0:
            # Use the reduced prefix returned by Py_GetPrefix()
            self.zip_path = joinpath(dirname(dirname(self.prefix)), path)
        else:
            self.zip_path = joinpath(self.prefix_macro, path)

    def calculate_pybuilddir(self):
        # Check to see if argv[0] is in the build directory. "pybuilddir.txt"
        # is written by setup.py and contains the relative path to the location
        # of shared library modules.
        #
        # Filename: <argv0_path> / "pybuilddir.txt"
        filename = joinpath(self.argv0_path, "pybuilddir.txt")
        try:
            fd = posix.open(filename, posix.O_RDONLY)
        except OSError:
            return

        try:
            line = readline(fd)
        finally:
            posix.close(fd)
        pybuilddir = line.decode('utf-8', 'surrogateescape')

        exec_prefix = joinpath(self.argv0_path, pybuilddir)
        self.exec_prefix_found = -1
        return exec_prefix

    def search_for_exec_prefix(self):
        # If PYTHONHOME is set, we believe it unconditionally
        home = self.pathconfig.home
        if home:
            # Path: <home> / <lib_python> / "lib-dynload"
            paths = home.split(DELIM)
            if len(paths) >= 2:
                exec_prefix = paths[1]
            else:
                exec_prefix = home

            exec_prefix = joinpath(exec_prefix, self.lib_python)
            exec_prefix = joinpath(exec_prefix, "lib-dynload")
            self.exec_prefix_found = 1
            return exec_prefix

        # Check for pybuilddir.txt
        assert self.exec_prefix_found == 0
        exec_prefix = self.calculate_pybuilddir()
        if self.exec_prefix_found != 0:
            return exec_prefix

        # Search from argv0_path, until root is found
        exec_prefix = abspath(self.argv0_path)
        while exec_prefix:
            # Path: <argv0_path or substring> / <lib_python> / "lib-dynload"
            new_exec_prefix = joinpath(exec_prefix, self.lib_python)
            new_exec_prefix = joinpath(new_exec_prefix, "lib-dynload")
            if isdir(new_exec_prefix):
                self.exec_prefix_found = 1
                return new_exec_prefix

            exec_prefix = dirname(exec_prefix)

        # Look at configure's EXEC_PREFIX.
        exec_prefix = joinpath(self.exec_prefix_macro, self.lib_python)
        exec_prefix = joinpath(exec_prefix, "lib-dynload")
        if isdir(exec_prefix):
            self.exec_prefix_found = 1
            return exec_prefix

        # Fail
        self.exec_prefix_found = 0

    def calculate_exec_prefix(self):
        exec_prefix = self.search_for_exec_prefix()

        if self.exec_prefix_found == 0:
            if self.warnings:
                print("Could not find platform dependent libraries <exec_prefix>\n",
                      file=sys.stderr)

            lib_dynload = joinpath(self.platlibdir, "lib-dynload")
            self.exec_prefix = joinpath(self.exec_prefix_macro, lib_dynload)
        else:
            # If we found EXEC_PREFIX do *not* reduce it!  (Yet.)
            self.exec_prefix = exec_prefix

    def calculate_module_search_path(self):
        paths = []

        # Run-time value of $PYTHONPATH environment variable goes first
        if self.pythonpath_env:
            paths.extend(self.pythonpath_env.split(DELIM))

        # Next is the default zip path
        paths.append(self.zip_path)

        # Next goes merge of compile-time $PYTHONPATH with
        # dynamically located prefix.
        for path in self.pythonpath_macro.split(DELIM):
            if path:
                path = joinpath(self.prefix, path)
            else:
                path = self.prefix
            paths.append(path)

        # Finally, on goes the directory for dynamic-load modules
        paths.append(self.exec_prefix)

        self.pathconfig.module_search_path = paths

    def calculate_set_prefix(self):
        # Reduce prefix and exec_prefix to their essence,
        # e.g. /usr/local/lib/python1.5 is reduced to /usr/local.
        # If we're loading relative to the build directory,
        # return the compiled-in defaults instead.
        if self.prefix_found > 0:
            prefix = dirname(dirname(self.prefix))
            if not prefix:
                # The prefix is the root directory, but dirname() chopped
                # off the "/".
                prefix = SEP
            self.pathconfig.prefix = prefix
        else:
            self.pathconfig.prefix = self.prefix_macro

    def calculate_set_exec_prefix(self):
        if self.exec_prefix_found > 0:
            exec_prefix = dirname(dirname(dirname(self.exec_prefix)))
            if not exec_prefix:
                # The exec_prefix is the root directory, but dirname() chopped
                # off the "/".
                exec_prefix = SEP
            self.pathconfig.exec_prefix = exec_prefix
        else:
            self.pathconfig.exec_prefix = self.exec_prefix_macro

    # Calculate the Python path configuration on Unix.
    #
    # Inputs:
    #
    # - PATH environment variable
    # - Macros: PYTHONPATH, PREFIX, EXEC_PREFIX, VERSION (ex: "3.9").
    #   PREFIX and EXEC_PREFIX are generated by the configure script.
    #   PYTHONPATH macro is the default search path.
    # - pybuilddir.txt file
    # - pyvenv.cfg configuration file
    # - PyConfig fields ('config' function argument):
    #
    #   - pathconfig_warnings
    #   - pythonpath_env (PYTHONPATH environment variable)
    #
    # - _PyPathConfig fields ('pathconfig' function argument):
    #
    #   - program_name: see config_init_program_name()
    #   - home: Py_SetPythonHome() or PYTHONHOME environment variable
    #
    # - current working directory: see abspath()
    #
    # Outputs, 'pathconfig' fields:
    #
    # - program_full_path
    # - module_search_path
    # - prefix
    # - exec_prefix
    #
    # If a field is already set (non NULL), it is left unchanged.
    def calculate(self):
        if self.pathconfig.program_full_path is None:
            self.calculate_program()

        self.calculate_argv0_path()

        # If a pyvenv.cfg configure file is found,
        # argv0_path is overriden with its 'home' variable.
        self.calculate_read_pyenv()

        self.calculate_prefix()
        self.calculate_zip_path()
        self.calculate_exec_prefix()

        if ((self.prefix_found == 0 or self.exec_prefix_found == 0)
           and self.warnings):
            print("Consider setting $PYTHONHOME to <prefix>[:<exec_prefix>]",
                  file=sys.stderr)

        if self.pathconfig.module_search_path is None:
            self.calculate_module_search_path()
        if self.pathconfig.prefix is None:
            self.calculate_set_prefix()
        if self.pathconfig.exec_prefix is None:
            self.calculate_set_exec_prefix()


def computed(config):
    return (config['module_search_paths_set']
            and config['executable'] is not None
            and config['prefix'] is not None
            and config['exec_prefix'] is not None)


def main():
    config = _cgetpath.get_config()
    if computed(config):
        return

    pathconfig = PathConfig()
    pathconfig.set_from_config(config)
    CalculatePath(pathconfig, config).calculate()
    pathconfig.set_to_config(config)

    _cgetpath.set_config(config)


if __name__ == "__main__":
    main()
