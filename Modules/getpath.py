# ******************************************************************************
# getpath.py
# ******************************************************************************

# This script is designed to be precompiled to bytecode, frozen into the
# main binary, and then directly evaluated. It is not an importable module,
# and does not import any other modules (besides winreg on Windows).
# Rather, the values listed below must be specified in the globals dict
# used when evaluating the bytecode.

# See _PyConfig_InitPathConfig in Modules/getpath.c for the execution.

# ******************************************************************************
# REQUIRED GLOBALS
# ******************************************************************************

# ** Helper functions **
# abspath(path)     -- make relative paths absolute against CWD
# basename(path)    -- the filename of path
# dirname(path)     -- the directory name of path
# hassuffix(path, suffix) -- returns True if path has suffix
# isabs(path)       -- path is absolute or not
# isdir(path)       -- path exists and is a directory
# isfile(path)      -- path exists and is a file
# isxfile(path)     -- path exists and is an executable file
# joinpath(*paths)  -- combine the paths
# readlines(path)   -- a list of each line of text in the UTF-8 encoded file
# realpath(path)    -- resolves symlinks in path
# warn(message)     -- print a warning (if enabled)

# ** Values known at compile time **
# os_name           -- [in] one of 'nt', 'posix', 'darwin'
# PREFIX            -- [in] sysconfig.get_config_var(...)
# EXEC_PREFIX       -- [in] sysconfig.get_config_var(...)
# PYTHONPATH        -- [in] sysconfig.get_config_var(...)
# WITH_NEXT_FRAMEWORK   -- [in] sysconfig.get_config_var(...)
# VPATH             -- [in] sysconfig.get_config_var(...)
# PLATLIBDIR        -- [in] sysconfig.get_config_var(...)
# PYDEBUGEXT        -- [in, opt] '_d' on Windows for debug builds
# EXE_SUFFIX        -- [in, opt] '.exe' on Windows/Cygwin/similar
# VERSION_MAJOR     -- [in] sys.version_info.major
# VERSION_MINOR     -- [in] sys.version_info.minor
# PYWINVER          -- [in] the Windows platform-specific version (e.g. 3.8-32)

# ** Values read from the environment **
#   There is no need to check the use_environment flag before reading
#   these, as the flag will be tested in this script.
#   Also note that ENV_PYTHONPATH is read from config['pythonpath_env']
#   to allow for embedders who choose to specify it via that struct.
# ENV_PATH                -- [in] getenv(...)
# ENV_PYTHONHOME          -- [in] getenv(...)
# ENV_PYTHONEXECUTABLE    -- [in] getenv(...)
# ENV___PYVENV_LAUNCHER__ -- [in] getenv(...)

# ** Values calculated at runtime **
# config            -- [in/out] dict of the PyConfig structure
# real_executable   -- [in, optional] resolved path to main process
#   On Windows and macOS, read directly from the running process
#   Otherwise, leave None and it will be calculated from executable
# executable_dir    -- [in, optional] real directory containing binary
#   If None, will be calculated from real_executable or executable
# py_setpath        -- [in] argument provided to Py_SetPath
#   If None, 'prefix' and 'exec_prefix' may be updated in config
# library           -- [in, optional] path of dylib/DLL/so
#   Only used for locating ._pth files
# winreg            -- [in, optional] the winreg module (only on Windows)

# ******************************************************************************
# HIGH-LEVEL ALGORITHM
# ******************************************************************************

# IMPORTANT: The code is the actual specification at time of writing.
# This prose description is based on the original comment from the old
# getpath.c to help capture the intent, but should not be considered
# a specification.

# Search in some common locations for the associated Python libraries.

# Two directories must be found, the platform independent directory
# (prefix), containing the common .py and .pyc files, and the platform
# dependent directory (exec_prefix), containing the shared library
# modules.  Note that prefix and exec_prefix can be the same directory,
# but for some installations, they are different.

# This script carries out separate searches for prefix and exec_prefix.
# Each search tries a number of different locations until a ``landmark''
# file or directory is found.  If no prefix or exec_prefix is found, a
# warning message is issued and the preprocessor defined PREFIX and
# EXEC_PREFIX are used (even though they will not work); python carries on
# as best as is possible, but most imports will fail.

# Before any searches are done, the location of the executable is
# determined.  If Py_SetPath() was called, or if we are running on
# Windows, the 'real_executable' path is used (if known).  Otherwise,
# we use the config-specified program name or default to argv[0].
# If this has one or more slashes in it, it is made absolute against
# the current working directory.  If it only contains a name, it must
# have been invoked from the shell's path, so we search $PATH for the
# named executable and use that.  If the executable was not found on
# $PATH (or there was no $PATH environment variable), the original
# argv[0] string is used.

# At this point, provided Py_SetPath was not used, the
# __PYVENV_LAUNCHER__ variable may override the executable (on macOS,
# the PYTHON_EXECUTABLE variable may also override). This allows
# certain launchers that run Python as a subprocess to properly
# specify the executable path. They are not intended for users.

# Next, the executable location is examined to see if it is a symbolic
# link.  If so, the link is realpath-ed and the directory of the link
# target is used for the remaining searches.  The same steps are
# performed for prefix and for exec_prefix, but with different landmarks.

# Step 1. Are we running in a virtual environment? Unless 'home' has
# been specified another way, check for a pyvenv.cfg and use its 'home'
# property to override the executable dir used later for prefix searches.
# We do not activate the venv here - that is performed later by site.py.

# Step 2. Is there a ._pth file? A ._pth file lives adjacent to the
# runtime library (if any) or the actual executable (not the symlink),
# and contains precisely the intended contents of sys.path as relative
# paths (to its own location). Its presence also enables isolated mode
# and suppresses other environment variable usage. Unless already
# specified by Py_SetHome(), the directory containing the ._pth file is
# set as 'home'.

# Step 3. Are we running python out of the build directory?  This is
# checked by looking for the BUILDDIR_TXT file, which contains the
# relative path to the platlib dir. The executable_dir value is
# derived from joining the VPATH preprocessor variable to the
# directory containing pybuilddir.txt. If it is not found, the
# BUILD_LANDMARK file is found, which is part of the source tree.
# prefix is then found by searching up for a file that should only
# exist in the source tree, and the stdlib dir is set to prefix/Lib.

# Step 4. If 'home' is set, either by Py_SetHome(), ENV_PYTHONHOME,
# a pyvenv.cfg file, ._pth file, or by detecting a build directory, it
# is assumed to point to prefix and exec_prefix. $PYTHONHOME can be a
# single directory, which is used for both, or the prefix and exec_prefix
# directories separated by DELIM (colon on POSIX; semicolon on Windows).

# Step 5. Try to find prefix and exec_prefix relative to executable_dir,
# backtracking up the path until it is exhausted.  This is the most common
# step to succeed.  Note that if prefix and exec_prefix are different,
# exec_prefix is more likely to be found; however if exec_prefix is a
# subdirectory of prefix, both will be found.

# Step 6. Search the directories pointed to by the preprocessor variables
# PREFIX and EXEC_PREFIX.  These are supplied by the Makefile but can be
# passed in as options to the configure script.

# That's it!

# Well, almost.  Once we have determined prefix and exec_prefix, the
# preprocessor variable PYTHONPATH is used to construct a path.  Each
# relative path on PYTHONPATH is prefixed with prefix.  Then the directory
# containing the shared library modules is appended.  The environment
# variable $PYTHONPATH is inserted in front of it all. On POSIX, if we are
# in a build directory, both prefix and exec_prefix are reset to the
# corresponding preprocessor variables (so sys.prefix will reflect the
# installation location, even though sys.path points into the build
# directory).  This seems to make more sense given that currently the only
# known use of sys.prefix and sys.exec_prefix is for the ILU installation
# process to find the installed Python tree.

# An embedding application can use Py_SetPath() to override all of
# these automatic path computations.


# ******************************************************************************
# PLATFORM CONSTANTS
# ******************************************************************************

platlibdir = config.get('platlibdir') or PLATLIBDIR

if os_name == 'posix' or os_name == 'darwin':
    BUILDDIR_TXT = 'pybuilddir.txt'
    BUILD_LANDMARK = 'Modules/Setup.local'
    DEFAULT_PROGRAM_NAME = f'python{VERSION_MAJOR}'
    STDLIB_SUBDIR = f'{platlibdir}/python{VERSION_MAJOR}.{VERSION_MINOR}'
    STDLIB_LANDMARKS = [f'{STDLIB_SUBDIR}/os.py', f'{STDLIB_SUBDIR}/os.pyc']
    PLATSTDLIB_LANDMARK = f'{platlibdir}/python{VERSION_MAJOR}.{VERSION_MINOR}/lib-dynload'
    BUILDSTDLIB_LANDMARKS = ['Lib/os.py']
    VENV_LANDMARK = 'pyvenv.cfg'
    ZIP_LANDMARK = f'{platlibdir}/python{VERSION_MAJOR}{VERSION_MINOR}.zip'
    DELIM = ':'
    SEP = '/'

elif os_name == 'nt':
    BUILDDIR_TXT = 'pybuilddir.txt'
    BUILD_LANDMARK = f'{VPATH}\\Modules\\Setup.local'
    DEFAULT_PROGRAM_NAME = f'python'
    STDLIB_SUBDIR = 'Lib'
    STDLIB_LANDMARKS = [f'{STDLIB_SUBDIR}\\os.py', f'{STDLIB_SUBDIR}\\os.pyc']
    PLATSTDLIB_LANDMARK = f'{platlibdir}'
    BUILDSTDLIB_LANDMARKS = ['Lib\\os.py']
    VENV_LANDMARK = 'pyvenv.cfg'
    ZIP_LANDMARK = f'python{VERSION_MAJOR}{VERSION_MINOR}{PYDEBUGEXT or ""}.zip'
    WINREG_KEY = f'SOFTWARE\\Python\\PythonCore\\{PYWINVER}\\PythonPath'
    DELIM = ';'
    SEP = '\\'


# ******************************************************************************
# HELPER FUNCTIONS (note that we prefer C functions for performance)
# ******************************************************************************

def search_up(prefix, *landmarks, test=isfile):
    while prefix:
        if any(test(joinpath(prefix, f)) for f in landmarks):
            return prefix
        prefix = dirname(prefix)


# ******************************************************************************
# READ VARIABLES FROM config
# ******************************************************************************

program_name = config.get('program_name')
home = config.get('home')
executable = config.get('executable')
base_executable = config.get('base_executable')
prefix = config.get('prefix')
exec_prefix = config.get('exec_prefix')
base_prefix = config.get('base_prefix')
base_exec_prefix = config.get('base_exec_prefix')
ENV_PYTHONPATH = config['pythonpath_env']
use_environment = config.get('use_environment', 1)

pythonpath = config.get('module_search_paths')
pythonpath_was_set = config.get('module_search_paths_set')

real_executable_dir = None
stdlib_dir = None
platstdlib_dir = None

# ******************************************************************************
# CALCULATE program_name
# ******************************************************************************

program_name_was_set = bool(program_name)

if not program_name:
    try:
        program_name = config.get('orig_argv', [])[0]
    except IndexError:
        pass

if not program_name:
    program_name = DEFAULT_PROGRAM_NAME

if EXE_SUFFIX and not hassuffix(program_name, EXE_SUFFIX) and isxfile(program_name + EXE_SUFFIX):
    program_name = program_name + EXE_SUFFIX


# ******************************************************************************
# CALCULATE executable
# ******************************************************************************

if py_setpath:
    # When Py_SetPath has been called, executable defaults to
    # the real executable path.
    if not executable:
        executable = real_executable

if not executable and SEP in program_name:
    # Resolve partial path program_name against current directory
    executable = abspath(program_name)

if not executable:
    # All platforms default to real_executable if known at this
    # stage. POSIX does not set this value.
    executable = real_executable
elif os_name == 'darwin':
    # QUIRK: On macOS we may know the real executable path, but
    # if our caller has lied to us about it (e.g. most of
    # test_embed), we need to use their path in order to detect
    # whether we are in a build tree. This is true even if the
    # executable path was provided in the config.
    real_executable = executable

if not executable and program_name and ENV_PATH:
    # Resolve names against PATH.
    # NOTE: The use_environment value is ignored for this lookup.
    # To properly isolate, launch Python with a full path.
    for p in ENV_PATH.split(DELIM):
        p = joinpath(p, program_name)
        if isxfile(p):
            executable = p
            break

if not executable:
    executable = ''
    # When we cannot calculate the executable, subsequent searches
    # look in the current working directory. Here, we emulate that
    # (the former getpath.c would do it apparently by accident).
    executable_dir = abspath('.')
    # Also need to set this fallback in case we are running from a
    # build directory with an invalid argv0 (i.e. test_sys.test_executable)
    real_executable_dir = executable_dir

if ENV_PYTHONEXECUTABLE or ENV___PYVENV_LAUNCHER__:
    # If set, these variables imply that we should be using them as
    # sys.executable and when searching for venvs. However, we should
    # use the argv0 path for prefix calculation

    if os_name == 'darwin' and WITH_NEXT_FRAMEWORK:
        # In a framework build the binary in {sys.exec_prefix}/bin is
        # a stub executable that execs the real interpreter in an
        # embedded app bundle. That bundle is an implementation detail
        # and should not affect base_executable.
        base_executable = f"{dirname(library)}/bin/python{VERSION_MAJOR}.{VERSION_MINOR}"
    else:
        base_executable = executable

    if not real_executable:
        real_executable = base_executable
        #real_executable_dir = dirname(real_executable)
    executable = ENV_PYTHONEXECUTABLE or ENV___PYVENV_LAUNCHER__
    executable_dir = dirname(executable)


# ******************************************************************************
# CALCULATE (default) home
# ******************************************************************************

# Used later to distinguish between Py_SetPythonHome and other
# ways that it may have been set
home_was_set = False

if home:
    home_was_set = True
elif use_environment and ENV_PYTHONHOME and not py_setpath:
    home = ENV_PYTHONHOME


# ******************************************************************************
# READ pyvenv.cfg
# ******************************************************************************

venv_prefix = None

# Calling Py_SetPythonHome(), Py_SetPath() or
# setting $PYTHONHOME will override venv detection.
if not home and not py_setpath:
    try:
        # prefix2 is just to avoid calculating dirname again later,
        # as the path in venv_prefix is the more common case.
        venv_prefix2 = executable_dir or dirname(executable)
        venv_prefix = dirname(venv_prefix2)
        try:
            # Read pyvenv.cfg from one level above executable
            pyvenvcfg = readlines(joinpath(venv_prefix, VENV_LANDMARK))
        except (FileNotFoundError, PermissionError):
            # Try the same directory as executable
            pyvenvcfg = readlines(joinpath(venv_prefix2, VENV_LANDMARK))
            venv_prefix = venv_prefix2
    except (FileNotFoundError, PermissionError):
        venv_prefix = None
        pyvenvcfg = []

    for line in pyvenvcfg:
        key, had_equ, value = line.partition('=')
        if had_equ and key.strip().lower() == 'home':
            executable_dir = real_executable_dir = value.strip()
            if not base_executable:
                # First try to resolve symlinked executables, since that may be
                # more accurate than assuming the executable in 'home'.
                try:
                    base_executable = realpath(executable)
                    if base_executable == executable:
                        # No change, so probably not a link. Clear it and fall back
                        base_executable = ''
                except OSError:
                    pass
                if not base_executable:
                    base_executable = joinpath(executable_dir, basename(executable))
                    # It's possible "python" is executed from within a posix venv but that
                    # "python" is not available in the "home" directory as the standard
                    # `make install` does not create it and distros often do not provide it.
                    #
                    # In this case, try to fall back to known alternatives
                    if os_name != 'nt' and not isfile(base_executable):
                        base_exe = basename(executable)
                        for candidate in (DEFAULT_PROGRAM_NAME, f'python{VERSION_MAJOR}.{VERSION_MINOR}'):
                            candidate += EXE_SUFFIX if EXE_SUFFIX else ''
                            if base_exe == candidate:
                                continue
                            candidate = joinpath(executable_dir, candidate)
                            # Only set base_executable if the candidate exists.
                            # If no candidate succeeds, subsequent errors related to
                            # base_executable (like FileNotFoundError) remain in the
                            # context of the original executable name
                            if isfile(candidate):
                                base_executable = candidate
                                break
            break
    else:
        venv_prefix = None


# ******************************************************************************
# CALCULATE base_executable, real_executable AND executable_dir
# ******************************************************************************

if not base_executable:
    base_executable = executable or real_executable or ''

if not real_executable:
    real_executable = base_executable

try:
    real_executable = realpath(real_executable)
except OSError as ex:
    # Only warn if the file actually exists and was unresolvable
    # Otherwise users who specify a fake executable may get spurious warnings.
    if isfile(real_executable):
        warn(f'Failed to find real location of {base_executable}')

if not executable_dir and os_name == 'darwin' and library:
    # QUIRK: macOS checks adjacent to its library early
    library_dir = dirname(library)
    if any(isfile(joinpath(library_dir, p)) for p in STDLIB_LANDMARKS):
        # Exceptions here should abort the whole process (to match
        # previous behavior)
        executable_dir = realpath(library_dir)
        real_executable_dir = executable_dir

# If we do not have the executable's directory, we can calculate it.
# This is the directory used to find prefix/exec_prefix if necessary.
if not executable_dir:
    executable_dir = real_executable_dir = dirname(real_executable)

# If we do not have the real executable's directory, we calculate it.
# This is the directory used to detect build layouts.
if not real_executable_dir:
    real_executable_dir = dirname(real_executable)

# ******************************************************************************
# DETECT _pth FILE
# ******************************************************************************

# The contents of an optional ._pth file are used to totally override
# sys.path calculation. Its presence also implies isolated mode and
# no-site (unless explicitly requested)
pth = None
pth_dir = None

# Calling Py_SetPythonHome() or Py_SetPath() will override ._pth search,
# but environment variables and command-line options cannot.
if not py_setpath and not home_was_set:
    # 1. Check adjacent to the main DLL/dylib/so (if set)
    # 2. Check adjacent to the original executable
    # 3. Check adjacent to our actual executable
    # This may allow a venv to override the base_executable's
    # ._pth file, but it cannot override the library's one.
    for p in [library, executable, real_executable]:
        if p:
            if os_name == 'nt' and (hassuffix(p, 'exe') or hassuffix(p, 'dll')):
                p = p.rpartition('.')[0]
            p += '._pth'
            try:
                pth = readlines(p)
                pth_dir = dirname(p)
                break
            except OSError:
                pass

    # If we found a ._pth file, disable environment and home
    # detection now. Later, we will do the rest.
    if pth_dir:
        use_environment = 0
        home = pth_dir
        pythonpath = []


# ******************************************************************************
# CHECK FOR BUILD DIRECTORY
# ******************************************************************************

build_prefix = None

if ((not home_was_set and real_executable_dir and not py_setpath)
        or config.get('_is_python_build', 0) > 0):
    # Detect a build marker and use it to infer prefix, exec_prefix,
    # stdlib_dir and the platstdlib_dir directories.
    try:
        platstdlib_dir = joinpath(
            real_executable_dir,
            readlines(joinpath(real_executable_dir, BUILDDIR_TXT))[0],
        )
        build_prefix = joinpath(real_executable_dir, VPATH)
    except IndexError:
        # File exists but is empty
        platstdlib_dir = real_executable_dir
        build_prefix = joinpath(real_executable_dir, VPATH)
    except (FileNotFoundError, PermissionError):
        if isfile(joinpath(real_executable_dir, BUILD_LANDMARK)):
            build_prefix = joinpath(real_executable_dir, VPATH)
            if os_name == 'nt':
                # QUIRK: Windows builds need platstdlib_dir to be the executable
                # dir. Normally the builddir marker handles this, but in this
                # case we need to correct manually.
                platstdlib_dir = real_executable_dir

    if build_prefix:
        if os_name == 'nt':
            # QUIRK: No searching for more landmarks on Windows
            build_stdlib_prefix = build_prefix
        else:
            build_stdlib_prefix = search_up(build_prefix, *BUILDSTDLIB_LANDMARKS)
        # Always use the build prefix for stdlib
        if build_stdlib_prefix:
            stdlib_dir = joinpath(build_stdlib_prefix, 'Lib')
        else:
            stdlib_dir = joinpath(build_prefix, 'Lib')
        # Only use the build prefix for prefix if it hasn't already been set
        if not prefix:
            prefix = build_stdlib_prefix
        # Do not warn, because 'prefix' never equals 'build_prefix' on POSIX
        #elif not venv_prefix and prefix != build_prefix:
        #    warn('Detected development environment but prefix is already set')
        if not exec_prefix:
            exec_prefix = build_prefix
        # Do not warn, because 'exec_prefix' never equals 'build_prefix' on POSIX
        #elif not venv_prefix and exec_prefix != build_prefix:
        #    warn('Detected development environment but exec_prefix is already set')
        config['_is_python_build'] = 1


# ******************************************************************************
# CALCULATE prefix AND exec_prefix
# ******************************************************************************

if py_setpath:
    # As documented, calling Py_SetPath will force both prefix
    # and exec_prefix to the empty string.
    prefix = exec_prefix = ''

else:
    # Read prefix and exec_prefix from explicitly set home
    if home:
        # When multiple paths are listed with ':' or ';' delimiters,
        # split into prefix:exec_prefix
        prefix, had_delim, exec_prefix = home.partition(DELIM)
        if not had_delim:
            exec_prefix = prefix
        # Reset the standard library directory if it was already set
        stdlib_dir = None


    # First try to detect prefix by looking alongside our runtime library, if known
    if library and not prefix:
        library_dir = dirname(library)
        if ZIP_LANDMARK:
            if os_name == 'nt':
                # QUIRK: Windows does not search up for ZIP file
                if isfile(joinpath(library_dir, ZIP_LANDMARK)):
                    prefix = library_dir
            else:
                prefix = search_up(library_dir, ZIP_LANDMARK)
        if STDLIB_SUBDIR and STDLIB_LANDMARKS and not prefix:
            if any(isfile(joinpath(library_dir, f)) for f in STDLIB_LANDMARKS):
                prefix = library_dir
                stdlib_dir = joinpath(prefix, STDLIB_SUBDIR)


    # Detect prefix by looking for zip file
    if ZIP_LANDMARK and executable_dir and not prefix:
        if os_name == 'nt':
            # QUIRK: Windows does not search up for ZIP file
            if isfile(joinpath(executable_dir, ZIP_LANDMARK)):
                prefix = executable_dir
        else:
            prefix = search_up(executable_dir, ZIP_LANDMARK)
        if prefix:
            stdlib_dir = joinpath(prefix, STDLIB_SUBDIR)
            if not isdir(stdlib_dir):
                stdlib_dir = None


    # Detect prefix by searching from our executable location for the stdlib_dir
    if STDLIB_SUBDIR and STDLIB_LANDMARKS and executable_dir and not prefix:
        prefix = search_up(executable_dir, *STDLIB_LANDMARKS)
        if prefix and not stdlib_dir:
            stdlib_dir = joinpath(prefix, STDLIB_SUBDIR)

    if PREFIX and not prefix:
        prefix = PREFIX
        if not any(isfile(joinpath(prefix, f)) for f in STDLIB_LANDMARKS):
            warn('Could not find platform independent libraries <prefix>')

    if not prefix:
        prefix = abspath('')
        warn('Could not find platform independent libraries <prefix>')


    # Detect exec_prefix by searching from executable for the platstdlib_dir
    if PLATSTDLIB_LANDMARK and not exec_prefix:
        if os_name == 'nt':
            # QUIRK: Windows always assumed these were the same
            # gh-100320: Our PYDs are assumed to be relative to the Lib directory
            # (that is, prefix) rather than the executable (that is, executable_dir)
            exec_prefix = prefix
        if not exec_prefix and executable_dir:
            exec_prefix = search_up(executable_dir, PLATSTDLIB_LANDMARK, test=isdir)
        if not exec_prefix and EXEC_PREFIX:
            exec_prefix = EXEC_PREFIX
        if not exec_prefix or not isdir(joinpath(exec_prefix, PLATSTDLIB_LANDMARK)):
            if os_name == 'nt':
                # QUIRK: If DLLs is missing on Windows, don't warn, just assume
                # that they're in exec_prefix
                if not platstdlib_dir:
                    # gh-98790: We set platstdlib_dir here to avoid adding "DLLs" into
                    # sys.path when it doesn't exist in the platstdlib place, which
                    # would give Lib packages precedence over executable_dir where our
                    # PYDs *probably* live. Ideally, whoever changes our layout will tell
                    # us what the layout is, but in the past this worked, so it should
                    # keep working.
                    platstdlib_dir = exec_prefix
            else:
                warn('Could not find platform dependent libraries <exec_prefix>')


    # Fallback: assume exec_prefix == prefix
    if not exec_prefix:
        exec_prefix = prefix


    if not prefix or not exec_prefix:
        warn('Consider setting $PYTHONHOME to <prefix>[:<exec_prefix>]')


# For a venv, update the main prefix/exec_prefix but leave the base ones unchanged
# XXX: We currently do not update prefix here, but it happens in site.py
#if venv_prefix:
#    base_prefix = prefix
#    base_exec_prefix = exec_prefix
#    prefix = exec_prefix = venv_prefix


# ******************************************************************************
# UPDATE pythonpath (sys.path)
# ******************************************************************************

if py_setpath:
    # If Py_SetPath was called then it overrides any existing search path
    config['module_search_paths'] = py_setpath.split(DELIM)
    config['module_search_paths_set'] = 1

elif not pythonpath_was_set:
    # If pythonpath was already explicitly set or calculated, we leave it alone.
    # This won't matter in normal use, but if an embedded host is trying to
    # recalculate paths while running then we do not want to change it.
    pythonpath = []

    # First add entries from the process environment
    if use_environment and ENV_PYTHONPATH:
        for p in ENV_PYTHONPATH.split(DELIM):
            pythonpath.append(abspath(p))

    # Then add the default zip file
    if os_name == 'nt':
        # QUIRK: Windows uses the library directory rather than the prefix
        if library:
            library_dir = dirname(library)
        else:
            library_dir = executable_dir
        pythonpath.append(joinpath(library_dir, ZIP_LANDMARK))
    elif build_prefix:
        # QUIRK: POSIX uses the default prefix when in the build directory
        pythonpath.append(joinpath(PREFIX, ZIP_LANDMARK))
    else:
        pythonpath.append(joinpath(prefix, ZIP_LANDMARK))

    if os_name == 'nt' and use_environment and winreg:
        # QUIRK: Windows also lists paths in the registry. Paths are stored
        # as the default value of each subkey of
        # {HKCU,HKLM}\Software\Python\PythonCore\{winver}\PythonPath
        # where winver is sys.winver (typically '3.x' or '3.x-32')
        for hk in (winreg.HKEY_CURRENT_USER, winreg.HKEY_LOCAL_MACHINE):
            try:
                key = winreg.OpenKeyEx(hk, WINREG_KEY)
                try:
                    i = 0
                    while True:
                        try:
                            v = winreg.QueryValue(key, winreg.EnumKey(key, i))
                        except OSError:
                            break
                        if isinstance(v, str):
                            pythonpath.extend(v.split(DELIM))
                        i += 1
                    # Paths from the core key get appended last, but only
                    # when home was not set and we haven't found our stdlib
                    # some other way.
                    if not home and not stdlib_dir:
                        v = winreg.QueryValue(key, None)
                        if isinstance(v, str):
                            pythonpath.extend(v.split(DELIM))
                finally:
                    winreg.CloseKey(key)
            except OSError:
                pass

    # Then add any entries compiled into the PYTHONPATH macro.
    if PYTHONPATH:
        for p in PYTHONPATH.split(DELIM):
            pythonpath.append(joinpath(prefix, p))

    # Then add stdlib_dir and platstdlib_dir
    if not stdlib_dir and prefix:
        stdlib_dir = joinpath(prefix, STDLIB_SUBDIR)
    if not platstdlib_dir and exec_prefix:
        platstdlib_dir = joinpath(exec_prefix, PLATSTDLIB_LANDMARK)

    if os_name == 'nt':
        # QUIRK: Windows generates paths differently
        if platstdlib_dir:
            pythonpath.append(platstdlib_dir)
        if stdlib_dir:
            pythonpath.append(stdlib_dir)
        if executable_dir and executable_dir not in pythonpath:
            # QUIRK: the executable directory is on sys.path
            # We keep it low priority, so that properly installed modules are
            # found first. It may be earlier in the order if we found some
            # reason to put it there.
            pythonpath.append(executable_dir)
    else:
        if stdlib_dir:
            pythonpath.append(stdlib_dir)
        if platstdlib_dir:
            pythonpath.append(platstdlib_dir)

    config['module_search_paths'] = pythonpath
    config['module_search_paths_set'] = 1


# ******************************************************************************
# POSIX prefix/exec_prefix QUIRKS
# ******************************************************************************

# QUIRK: Non-Windows replaces prefix/exec_prefix with defaults when running
# in build directory. This happens after pythonpath calculation.
if os_name != 'nt' and build_prefix:
    prefix = config.get('prefix') or PREFIX
    exec_prefix = config.get('exec_prefix') or EXEC_PREFIX or prefix


# ******************************************************************************
# SET pythonpath FROM _PTH FILE
# ******************************************************************************

if pth:
    config['isolated'] = 1
    config['use_environment'] = 0
    config['site_import'] = 0
    config['safe_path'] = 1
    pythonpath = []
    for line in pth:
        line = line.partition('#')[0].strip()
        if not line:
            pass
        elif line == 'import site':
            config['site_import'] = 1
        elif line.startswith('import '):
            warn("unsupported 'import' line in ._pth file")
        else:
            pythonpath.append(joinpath(pth_dir, line))
    config['module_search_paths'] = pythonpath
    config['module_search_paths_set'] = 1

# ******************************************************************************
# UPDATE config FROM CALCULATED VALUES
# ******************************************************************************

config['program_name'] = program_name
config['home'] = home
config['executable'] = executable
config['base_executable'] = base_executable
config['prefix'] = prefix
config['exec_prefix'] = exec_prefix
config['base_prefix'] = base_prefix or prefix
config['base_exec_prefix'] = base_exec_prefix or exec_prefix

config['platlibdir'] = platlibdir
# test_embed expects empty strings, not None
config['stdlib_dir'] = stdlib_dir or ''
config['platstdlib_dir'] = platstdlib_dir or ''
