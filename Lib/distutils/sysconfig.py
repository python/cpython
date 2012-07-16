"""Provide access to Python's configuration information.  The specific
configuration variables available depend heavily on the platform and
configuration.  The values may be retrieved using
get_config_var(name), and the list of variables is available via
get_config_vars().keys().  Additional convenience functions are also
available.

Written by:   Fred L. Drake, Jr.
Email:        <fdrake@acm.org>
"""

import os
import re
import sys

from .errors import DistutilsPlatformError

# These are needed in a couple of spots, so just compute them once.
PREFIX = os.path.normpath(sys.prefix)
EXEC_PREFIX = os.path.normpath(sys.exec_prefix)
BASE_PREFIX = os.path.normpath(sys.base_prefix)
BASE_EXEC_PREFIX = os.path.normpath(sys.base_exec_prefix)

# Path to the base directory of the project. On Windows the binary may
# live in project/PCBuild9.  If we're dealing with an x64 Windows build,
# it'll live in project/PCbuild/amd64.
project_base = os.path.dirname(os.path.abspath(sys.executable))
if os.name == "nt" and "pcbuild" in project_base[-8:].lower():
    project_base = os.path.abspath(os.path.join(project_base, os.path.pardir))
# PC/VS7.1
if os.name == "nt" and "\\pc\\v" in project_base[-10:].lower():
    project_base = os.path.abspath(os.path.join(project_base, os.path.pardir,
                                                os.path.pardir))
# PC/AMD64
if os.name == "nt" and "\\pcbuild\\amd64" in project_base[-14:].lower():
    project_base = os.path.abspath(os.path.join(project_base, os.path.pardir,
                                                os.path.pardir))

# python_build: (Boolean) if true, we're either building Python or
# building an extension with an un-installed Python, so we use
# different (hard-wired) directories.
# Setup.local is available for Makefile builds including VPATH builds,
# Setup.dist is available on Windows
def _is_python_source_dir(d):
    for fn in ("Setup.dist", "Setup.local"):
        if os.path.isfile(os.path.join(d, "Modules", fn)):
            return True
    return False
_sys_home = getattr(sys, '_home', None)
if _sys_home and os.name == 'nt' and \
    _sys_home.lower().endswith(('pcbuild', 'pcbuild\\amd64')):
    _sys_home = os.path.dirname(_sys_home)
    if _sys_home.endswith('pcbuild'):   # must be amd64
        _sys_home = os.path.dirname(_sys_home)
def _python_build():
    if _sys_home:
        return _is_python_source_dir(_sys_home)
    return _is_python_source_dir(project_base)
python_build = _python_build()

# Calculate the build qualifier flags if they are defined.  Adding the flags
# to the include and lib directories only makes sense for an installation, not
# an in-source build.
build_flags = ''
try:
    if not python_build:
        build_flags = sys.abiflags
except AttributeError:
    # It's not a configure-based build, so the sys module doesn't have
    # this attribute, which is fine.
    pass

def get_python_version():
    """Return a string containing the major and minor Python version,
    leaving off the patchlevel.  Sample return values could be '1.5'
    or '2.2'.
    """
    return sys.version[:3]


def get_python_inc(plat_specific=0, prefix=None):
    """Return the directory containing installed Python header files.

    If 'plat_specific' is false (the default), this is the path to the
    non-platform-specific header files, i.e. Python.h and so on;
    otherwise, this is the path to platform-specific header files
    (namely pyconfig.h).

    If 'prefix' is supplied, use it instead of sys.base_prefix or
    sys.base_exec_prefix -- i.e., ignore 'plat_specific'.
    """
    if prefix is None:
        prefix = plat_specific and BASE_EXEC_PREFIX or BASE_PREFIX
    if os.name == "posix":
        if python_build:
            # Assume the executable is in the build directory.  The
            # pyconfig.h file should be in the same directory.  Since
            # the build directory may not be the source directory, we
            # must use "srcdir" from the makefile to find the "Include"
            # directory.
            base = _sys_home or os.path.dirname(os.path.abspath(sys.executable))
            if plat_specific:
                return base
            else:
                incdir = os.path.join(_sys_home or get_config_var('srcdir'),
                                      'Include')
                return os.path.normpath(incdir)
        python_dir = 'python' + get_python_version() + build_flags
        return os.path.join(prefix, "include", python_dir)
    elif os.name == "nt":
        return os.path.join(prefix, "include")
    elif os.name == "os2":
        return os.path.join(prefix, "Include")
    else:
        raise DistutilsPlatformError(
            "I don't know where Python installs its C header files "
            "on platform '%s'" % os.name)


def get_python_lib(plat_specific=0, standard_lib=0, prefix=None):
    """Return the directory containing the Python library (standard or
    site additions).

    If 'plat_specific' is true, return the directory containing
    platform-specific modules, i.e. any module from a non-pure-Python
    module distribution; otherwise, return the platform-shared library
    directory.  If 'standard_lib' is true, return the directory
    containing standard Python library modules; otherwise, return the
    directory for site-specific modules.

    If 'prefix' is supplied, use it instead of sys.base_prefix or
    sys.base_exec_prefix -- i.e., ignore 'plat_specific'.
    """
    if prefix is None:
        if standard_lib:
            prefix = plat_specific and BASE_EXEC_PREFIX or BASE_PREFIX
        else:
            prefix = plat_specific and EXEC_PREFIX or PREFIX

    if os.name == "posix":
        libpython = os.path.join(prefix,
                                 "lib", "python" + get_python_version())
        if standard_lib:
            return libpython
        else:
            return os.path.join(libpython, "site-packages")
    elif os.name == "nt":
        if standard_lib:
            return os.path.join(prefix, "Lib")
        else:
            if get_python_version() < "2.2":
                return prefix
            else:
                return os.path.join(prefix, "Lib", "site-packages")
    elif os.name == "os2":
        if standard_lib:
            return os.path.join(prefix, "Lib")
        else:
            return os.path.join(prefix, "Lib", "site-packages")
    else:
        raise DistutilsPlatformError(
            "I don't know where Python installs its library "
            "on platform '%s'" % os.name)



def customize_compiler(compiler):
    """Do any platform-specific customization of a CCompiler instance.

    Mainly needed on Unix, so we can plug in the information that
    varies across Unices and is stored in Python's Makefile.
    """
    if compiler.compiler_type == "unix":
        (cc, cxx, opt, cflags, ccshared, ldshared, so_ext, ar, ar_flags) = \
            get_config_vars('CC', 'CXX', 'OPT', 'CFLAGS',
                            'CCSHARED', 'LDSHARED', 'SO', 'AR', 'ARFLAGS')

        newcc = None
        if 'CC' in os.environ:
            cc = os.environ['CC']
        if 'CXX' in os.environ:
            cxx = os.environ['CXX']
        if 'LDSHARED' in os.environ:
            ldshared = os.environ['LDSHARED']
        if 'CPP' in os.environ:
            cpp = os.environ['CPP']
        else:
            cpp = cc + " -E"           # not always
        if 'LDFLAGS' in os.environ:
            ldshared = ldshared + ' ' + os.environ['LDFLAGS']
        if 'CFLAGS' in os.environ:
            cflags = opt + ' ' + os.environ['CFLAGS']
            ldshared = ldshared + ' ' + os.environ['CFLAGS']
        if 'CPPFLAGS' in os.environ:
            cpp = cpp + ' ' + os.environ['CPPFLAGS']
            cflags = cflags + ' ' + os.environ['CPPFLAGS']
            ldshared = ldshared + ' ' + os.environ['CPPFLAGS']
        if 'AR' in os.environ:
            ar = os.environ['AR']
        if 'ARFLAGS' in os.environ:
            archiver = ar + ' ' + os.environ['ARFLAGS']
        else:
            archiver = ar + ' ' + ar_flags

        cc_cmd = cc + ' ' + cflags
        compiler.set_executables(
            preprocessor=cpp,
            compiler=cc_cmd,
            compiler_so=cc_cmd + ' ' + ccshared,
            compiler_cxx=cxx,
            linker_so=ldshared,
            linker_exe=cc,
            archiver=archiver)

        compiler.shared_lib_extension = so_ext


def get_config_h_filename():
    """Return full pathname of installed pyconfig.h file."""
    if python_build:
        if os.name == "nt":
            inc_dir = os.path.join(_sys_home or project_base, "PC")
        else:
            inc_dir = _sys_home or project_base
    else:
        inc_dir = get_python_inc(plat_specific=1)
    if get_python_version() < '2.2':
        config_h = 'config.h'
    else:
        # The name of the config.h file changed in 2.2
        config_h = 'pyconfig.h'
    return os.path.join(inc_dir, config_h)


def get_makefile_filename():
    """Return full pathname of installed Makefile from the Python build."""
    if python_build:
        return os.path.join(_sys_home or os.path.dirname(sys.executable),
                                                         "Makefile")
    lib_dir = get_python_lib(plat_specific=0, standard_lib=1)
    config_file = 'config-{}{}'.format(get_python_version(), build_flags)
    return os.path.join(lib_dir, config_file, 'Makefile')


def parse_config_h(fp, g=None):
    """Parse a config.h-style file.

    A dictionary containing name/value pairs is returned.  If an
    optional dictionary is passed in as the second argument, it is
    used instead of a new dictionary.
    """
    if g is None:
        g = {}
    define_rx = re.compile("#define ([A-Z][A-Za-z0-9_]+) (.*)\n")
    undef_rx = re.compile("/[*] #undef ([A-Z][A-Za-z0-9_]+) [*]/\n")
    #
    while True:
        line = fp.readline()
        if not line:
            break
        m = define_rx.match(line)
        if m:
            n, v = m.group(1, 2)
            try: v = int(v)
            except ValueError: pass
            g[n] = v
        else:
            m = undef_rx.match(line)
            if m:
                g[m.group(1)] = 0
    return g


# Regexes needed for parsing Makefile (and similar syntaxes,
# like old-style Setup files).
_variable_rx = re.compile("([a-zA-Z][a-zA-Z0-9_]+)\s*=\s*(.*)")
_findvar1_rx = re.compile(r"\$\(([A-Za-z][A-Za-z0-9_]*)\)")
_findvar2_rx = re.compile(r"\${([A-Za-z][A-Za-z0-9_]*)}")

def parse_makefile(fn, g=None):
    """Parse a Makefile-style file.

    A dictionary containing name/value pairs is returned.  If an
    optional dictionary is passed in as the second argument, it is
    used instead of a new dictionary.
    """
    from distutils.text_file import TextFile
    fp = TextFile(fn, strip_comments=1, skip_blanks=1, join_lines=1, errors="surrogateescape")

    if g is None:
        g = {}
    done = {}
    notdone = {}

    while True:
        line = fp.readline()
        if line is None: # eof
            break
        m = _variable_rx.match(line)
        if m:
            n, v = m.group(1, 2)
            v = v.strip()
            # `$$' is a literal `$' in make
            tmpv = v.replace('$$', '')

            if "$" in tmpv:
                notdone[n] = v
            else:
                try:
                    v = int(v)
                except ValueError:
                    # insert literal `$'
                    done[n] = v.replace('$$', '$')
                else:
                    done[n] = v

    # Variables with a 'PY_' prefix in the makefile. These need to
    # be made available without that prefix through sysconfig.
    # Special care is needed to ensure that variable expansion works, even
    # if the expansion uses the name without a prefix.
    renamed_variables = ('CFLAGS', 'LDFLAGS', 'CPPFLAGS')

    # do variable interpolation here
    while notdone:
        for name in list(notdone):
            value = notdone[name]
            m = _findvar1_rx.search(value) or _findvar2_rx.search(value)
            if m:
                n = m.group(1)
                found = True
                if n in done:
                    item = str(done[n])
                elif n in notdone:
                    # get it on a subsequent round
                    found = False
                elif n in os.environ:
                    # do it like make: fall back to environment
                    item = os.environ[n]

                elif n in renamed_variables:
                    if name.startswith('PY_') and name[3:] in renamed_variables:
                        item = ""

                    elif 'PY_' + n in notdone:
                        found = False

                    else:
                        item = str(done['PY_' + n])
                else:
                    done[n] = item = ""
                if found:
                    after = value[m.end():]
                    value = value[:m.start()] + item + after
                    if "$" in after:
                        notdone[name] = value
                    else:
                        try: value = int(value)
                        except ValueError:
                            done[name] = value.strip()
                        else:
                            done[name] = value
                        del notdone[name]

                        if name.startswith('PY_') \
                            and name[3:] in renamed_variables:

                            name = name[3:]
                            if name not in done:
                                done[name] = value
            else:
                # bogus variable reference; just drop it since we can't deal
                del notdone[name]

    fp.close()

    # strip spurious spaces
    for k, v in done.items():
        if isinstance(v, str):
            done[k] = v.strip()

    # save the results in the global dictionary
    g.update(done)
    return g


def expand_makefile_vars(s, vars):
    """Expand Makefile-style variables -- "${foo}" or "$(foo)" -- in
    'string' according to 'vars' (a dictionary mapping variable names to
    values).  Variables not present in 'vars' are silently expanded to the
    empty string.  The variable values in 'vars' should not contain further
    variable expansions; if 'vars' is the output of 'parse_makefile()',
    you're fine.  Returns a variable-expanded version of 's'.
    """

    # This algorithm does multiple expansion, so if vars['foo'] contains
    # "${bar}", it will expand ${foo} to ${bar}, and then expand
    # ${bar}... and so forth.  This is fine as long as 'vars' comes from
    # 'parse_makefile()', which takes care of such expansions eagerly,
    # according to make's variable expansion semantics.

    while True:
        m = _findvar1_rx.search(s) or _findvar2_rx.search(s)
        if m:
            (beg, end) = m.span()
            s = s[0:beg] + vars.get(m.group(1)) + s[end:]
        else:
            break
    return s


_config_vars = None

def _init_posix():
    """Initialize the module as appropriate for POSIX systems."""
    g = {}
    # load the installed Makefile:
    try:
        filename = get_makefile_filename()
        parse_makefile(filename, g)
    except IOError as msg:
        my_msg = "invalid Python installation: unable to open %s" % filename
        if hasattr(msg, "strerror"):
            my_msg = my_msg + " (%s)" % msg.strerror

        raise DistutilsPlatformError(my_msg)

    # load the installed pyconfig.h:
    try:
        filename = get_config_h_filename()
        with open(filename) as file:
            parse_config_h(file, g)
    except IOError as msg:
        my_msg = "invalid Python installation: unable to open %s" % filename
        if hasattr(msg, "strerror"):
            my_msg = my_msg + " (%s)" % msg.strerror

        raise DistutilsPlatformError(my_msg)

    # On AIX, there are wrong paths to the linker scripts in the Makefile
    # -- these paths are relative to the Python source, but when installed
    # the scripts are in another directory.
    if python_build:
        g['LDSHARED'] = g['BLDSHARED']

    elif get_python_version() < '2.1':
        # The following two branches are for 1.5.2 compatibility.
        if sys.platform == 'aix4':          # what about AIX 3.x ?
            # Linker script is in the config directory, not in Modules as the
            # Makefile says.
            python_lib = get_python_lib(standard_lib=1)
            ld_so_aix = os.path.join(python_lib, 'config', 'ld_so_aix')
            python_exp = os.path.join(python_lib, 'config', 'python.exp')

            g['LDSHARED'] = "%s %s -bI:%s" % (ld_so_aix, g['CC'], python_exp)

    global _config_vars
    _config_vars = g


def _init_nt():
    """Initialize the module as appropriate for NT"""
    g = {}
    # set basic install directories
    g['LIBDEST'] = get_python_lib(plat_specific=0, standard_lib=1)
    g['BINLIBDEST'] = get_python_lib(plat_specific=1, standard_lib=1)

    # XXX hmmm.. a normal install puts include files here
    g['INCLUDEPY'] = get_python_inc(plat_specific=0)

    g['SO'] = '.pyd'
    g['EXE'] = ".exe"
    g['VERSION'] = get_python_version().replace(".", "")
    g['BINDIR'] = os.path.dirname(os.path.abspath(sys.executable))

    global _config_vars
    _config_vars = g


def _init_os2():
    """Initialize the module as appropriate for OS/2"""
    g = {}
    # set basic install directories
    g['LIBDEST'] = get_python_lib(plat_specific=0, standard_lib=1)
    g['BINLIBDEST'] = get_python_lib(plat_specific=1, standard_lib=1)

    # XXX hmmm.. a normal install puts include files here
    g['INCLUDEPY'] = get_python_inc(plat_specific=0)

    g['SO'] = '.pyd'
    g['EXE'] = ".exe"

    global _config_vars
    _config_vars = g


def _read_output(commandstring):
    """
    Returns os.popen(commandstring, "r").read(), but
    without actually using os.popen because that
    function is not usable during python bootstrap
    """
    # NOTE: tempfile is also not useable during
    # bootstrap
    import contextlib
    try:
        import tempfile
        fp = tempfile.NamedTemporaryFile()
    except ImportError:
        fp = open("/tmp/distutils.%s"%(
            os.getpid(),), "w+b")

    with contextlib.closing(fp) as fp:
        cmd = "%s >'%s'"%(commandstring, fp.name)
        os.system(cmd)
        data = fp.read()

    return data.decode('utf-8')

def get_config_vars(*args):
    """With no arguments, return a dictionary of all configuration
    variables relevant for the current platform.  Generally this includes
    everything needed to build extensions and install both pure modules and
    extensions.  On Unix, this means every variable defined in Python's
    installed Makefile; on Windows and Mac OS it's a much smaller set.

    With arguments, return a list of values that result from looking up
    each argument in the configuration variable dictionary.
    """
    global _config_vars
    if _config_vars is None:
        func = globals().get("_init_" + os.name)
        if func:
            func()
        else:
            _config_vars = {}

        # Normalized versions of prefix and exec_prefix are handy to have;
        # in fact, these are the standard versions used most places in the
        # Distutils.
        _config_vars['prefix'] = PREFIX
        _config_vars['exec_prefix'] = EXEC_PREFIX

        # Convert srcdir into an absolute path if it appears necessary.
        # Normally it is relative to the build directory.  However, during
        # testing, for example, we might be running a non-installed python
        # from a different directory.
        if python_build and os.name == "posix":
            base = os.path.dirname(os.path.abspath(sys.executable))
            if (not os.path.isabs(_config_vars['srcdir']) and
                base != os.getcwd()):
                # srcdir is relative and we are not in the same directory
                # as the executable. Assume executable is in the build
                # directory and make srcdir absolute.
                srcdir = os.path.join(base, _config_vars['srcdir'])
                _config_vars['srcdir'] = os.path.normpath(srcdir)

        if sys.platform == 'darwin':
            from distutils.spawn import find_executable

            kernel_version = os.uname()[2] # Kernel version (8.4.3)
            major_version = int(kernel_version.split('.')[0])

            # Issue #13590:
            #    The OSX location for the compiler varies between OSX
            #    (or rather Xcode) releases.  With older releases (up-to 10.5)
            #    the compiler is in /usr/bin, with newer releases the compiler
            #    can only be found inside Xcode.app if the "Command Line Tools"
            #    are not installed.
            #
            #    Futhermore, the compiler that can be used varies between
            #    Xcode releases. Upto Xcode 4 it was possible to use 'gcc-4.2'
            #    as the compiler, after that 'clang' should be used because
            #    gcc-4.2 is either not present, or a copy of 'llvm-gcc' that
            #    miscompiles Python.

            # skip checks if the compiler was overriden with a CC env variable
            if 'CC' not in os.environ:
                cc = oldcc = _config_vars['CC']
                if not find_executable(cc):
                    # Compiler is not found on the shell search PATH.
                    # Now search for clang, first on PATH (if the Command LIne
                    # Tools have been installed in / or if the user has provided
                    # another location via CC).  If not found, try using xcrun
                    # to find an uninstalled clang (within a selected Xcode).

                    # NOTE: Cannot use subprocess here because of bootstrap
                    # issues when building Python itself (and os.popen is
                    # implemented on top of subprocess and is therefore not
                    # usable as well)

                    data = (find_executable('clang') or
                            _read_output(
                                "/usr/bin/xcrun -find clang 2>/dev/null").strip())
                    if not data:
                        raise DistutilsPlatformError(
                               "Cannot locate working compiler")

                    _config_vars['CC'] = cc = data
                    _config_vars['CXX'] = cc + '++'

                elif os.path.basename(cc).startswith('gcc'):
                    # Compiler is GCC, check if it is LLVM-GCC
                    data = _read_output("'%s' --version 2>/dev/null"
                                         % (cc.replace("'", "'\"'\"'"),))
                    if 'llvm-gcc' in data:
                        # Found LLVM-GCC, fall back to clang
                        data = (find_executable('clang') or
                                _read_output(
                                    "/usr/bin/xcrun -find clang 2>/dev/null").strip())
                        if find_executable(data):
                            _config_vars['CC'] = cc = data
                            _config_vars['CXX'] = cc + '++'

                if (cc != oldcc
                        and 'LDSHARED' in _config_vars
                        and 'LDSHARED' not in os.environ):
                    # modify LDSHARED if we modified CC
                    ldshared = _config_vars['LDSHARED']
                    if ldshared.startswith(oldcc):
                        _config_vars['LDSHARED'] = cc + ldshared[len(oldcc):]

            if major_version < 8:
                # On Mac OS X before 10.4, check if -arch and -isysroot
                # are in CFLAGS or LDFLAGS and remove them if they are.
                # This is needed when building extensions on a 10.3 system
                # using a universal build of python.
                for key in ('LDFLAGS', 'BASECFLAGS', 'LDSHARED',
                        # a number of derived variables. These need to be
                        # patched up as well.
                        'CFLAGS', 'PY_CFLAGS', 'BLDSHARED'):
                    flags = _config_vars[key]
                    flags = re.sub('-arch\s+\w+\s', ' ', flags, re.ASCII)
                    flags = re.sub('-isysroot [^ \t]*', ' ', flags)
                    _config_vars[key] = flags

            else:
                # Different Xcode releases support different sets for '-arch'
                # flags. In particular, Xcode 4.x no longer supports the
                # PPC architectures.
                #
                # This code automatically removes '-arch ppc' and '-arch ppc64'
                # when these are not supported. That makes it possible to
                # build extensions on OSX 10.7 and later with the prebuilt
                # 32-bit installer on the python.org website.
                flags = _config_vars['CFLAGS']
                if re.search('-arch\s+ppc', flags) is not None:
                    # NOTE: Cannot use subprocess here because of bootstrap
                    # issues when building Python itself
                    status = os.system("'%s' -arch ppc -x c /dev/null 2>/dev/null"%(
                        _config_vars['CC'].replace("'", "'\"'\"'"),))

                    if status != 0:
                        # Compiler doesn't support PPC, remove the related
                        # '-arch' flags.
                        for key in ('LDFLAGS', 'BASECFLAGS',
                            # a number of derived variables. These need to be
                            # patched up as well.
                            'CFLAGS', 'PY_CFLAGS', 'BLDSHARED', 'LDSHARED'):

                            flags = _config_vars[key]
                            flags = re.sub('-arch\s+ppc\w*\s', ' ', flags)
                            _config_vars[key] = flags


                # Allow the user to override the architecture flags using
                # an environment variable.
                # NOTE: This name was introduced by Apple in OSX 10.5 and
                # is used by several scripting languages distributed with
                # that OS release.
                if 'ARCHFLAGS' in os.environ:
                    arch = os.environ['ARCHFLAGS']
                    for key in ('LDFLAGS', 'BASECFLAGS', 'LDSHARED',
                        # a number of derived variables. These need to be
                        # patched up as well.
                        'CFLAGS', 'PY_CFLAGS', 'BLDSHARED'):

                        flags = _config_vars[key]
                        flags = re.sub('-arch\s+\w+\s', ' ', flags)
                        flags = flags + ' ' + arch
                        _config_vars[key] = flags

                # If we're on OSX 10.5 or later and the user tries to
                # compiles an extension using an SDK that is not present
                # on the current machine it is better to not use an SDK
                # than to fail.
                #
                # The major usecase for this is users using a Python.org
                # binary installer  on OSX 10.6: that installer uses
                # the 10.4u SDK, but that SDK is not installed by default
                # when you install Xcode.
                #
                m = re.search('-isysroot\s+(\S+)', _config_vars['CFLAGS'])
                if m is not None:
                    sdk = m.group(1)
                    if not os.path.exists(sdk):
                        for key in ('LDFLAGS', 'BASECFLAGS', 'LDSHARED',
                             # a number of derived variables. These need to be
                             # patched up as well.
                            'CFLAGS', 'PY_CFLAGS', 'BLDSHARED'):

                            flags = _config_vars[key]
                            flags = re.sub('-isysroot\s+\S+(\s|$)', ' ', flags)
                            _config_vars[key] = flags

    if args:
        vals = []
        for name in args:
            vals.append(_config_vars.get(name))
        return vals
    else:
        return _config_vars

def get_config_var(name):
    """Return the value of a single variable using the dictionary
    returned by 'get_config_vars()'.  Equivalent to
    get_config_vars().get(name)
    """
    return get_config_vars().get(name)
