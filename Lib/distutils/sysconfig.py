"""Provide access to Python's configuration information.  The specific names
defined in the module depend heavily on the platform and configuration.

Written by:   Fred L. Drake, Jr.
Email:        <fdrake@acm.org>
Initial date: 17-Dec-1998
"""

__revision__ = "$Id$"

import os
import re
import string
import sys

from errors import DistutilsPlatformError

# These are needed in a couple of spots, so just compute them once.
PREFIX = os.path.normpath(sys.prefix)
EXEC_PREFIX = os.path.normpath(sys.exec_prefix)

# Boolean; if it's true, we're still building Python, so
# we use different (hard-wired) directories.

python_build = 0

def set_python_build():
    """Set the python_build flag to true; this means that we're
    building Python itself.  Only called from the setup.py script
    shipped with Python.
    """
    
    global python_build
    python_build = 1

def get_python_inc(plat_specific=0, prefix=None):
    """Return the directory containing installed Python header files.

    If 'plat_specific' is false (the default), this is the path to the
    non-platform-specific header files, i.e. Python.h and so on;
    otherwise, this is the path to platform-specific header files
    (namely config.h).

    If 'prefix' is supplied, use it instead of sys.prefix or
    sys.exec_prefix -- i.e., ignore 'plat_specific'.
    """    
    if prefix is None:
        prefix = (plat_specific and EXEC_PREFIX or PREFIX)
    if os.name == "posix":
        if python_build:
            return "Include/"
        return os.path.join(prefix, "include", "python" + sys.version[:3])
    elif os.name == "nt":
        return os.path.join(prefix, "Include") # include or Include?
    elif os.name == "mac":
        return os.path.join(prefix, "Include")
    else:
        raise DistutilsPlatformError, \
              ("I don't know where Python installs its C header files " +
               "on platform '%s'") % os.name


def get_python_lib(plat_specific=0, standard_lib=0, prefix=None):
    """Return the directory containing the Python library (standard or
    site additions).

    If 'plat_specific' is true, return the directory containing
    platform-specific modules, i.e. any module from a non-pure-Python
    module distribution; otherwise, return the platform-shared library
    directory.  If 'standard_lib' is true, return the directory
    containing standard Python library modules; otherwise, return the
    directory for site-specific modules.

    If 'prefix' is supplied, use it instead of sys.prefix or
    sys.exec_prefix -- i.e., ignore 'plat_specific'.
    """
    if prefix is None:
        prefix = (plat_specific and EXEC_PREFIX or PREFIX)
       
    if os.name == "posix":
        libpython = os.path.join(prefix,
                                 "lib", "python" + sys.version[:3])
        if standard_lib:
            return libpython
        else:
            return os.path.join(libpython, "site-packages")

    elif os.name == "nt":
        if standard_lib:
            return os.path.join(PREFIX, "Lib")
        else:
            return prefix

    elif os.name == "mac":
        if plat_specific:
            if standard_lib:
                return os.path.join(EXEC_PREFIX, "Mac", "Plugins")
            else:
                raise DistutilsPlatformError, \
                      "OK, where DO site-specific extensions go on the Mac?"
        else:
            if standard_lib:
                return os.path.join(PREFIX, "Lib")
            else:
                raise DistutilsPlatformError, \
                      "OK, where DO site-specific modules go on the Mac?"
    else:
        raise DistutilsPlatformError, \
              ("I don't know where Python installs its library " +
               "on platform '%s'") % os.name

# get_python_lib()
        

def customize_compiler (compiler):
    """Do any platform-specific customization of the CCompiler instance
    'compiler'.  Mainly needed on Unix, so we can plug in the information
    that varies across Unices and is stored in Python's Makefile.
    """
    if compiler.compiler_type == "unix":
        (cc, opt, ccshared, ldshared, so_ext) = \
            get_config_vars('CC', 'OPT', 'CCSHARED', 'LDSHARED', 'SO')

        cc_cmd = cc + ' ' + opt
        compiler.set_executables(
            preprocessor=cc + " -E",    # not always!
            compiler=cc_cmd,
            compiler_so=cc_cmd + ' ' + ccshared,
            linker_so=ldshared,
            linker_exe=cc)

        compiler.shared_lib_extension = so_ext


def get_config_h_filename():
    """Return full pathname of installed config.h file."""
    if python_build: inc_dir = '.'
    else:            inc_dir = get_python_inc(plat_specific=1)
    return os.path.join(inc_dir, "config.h")


def get_makefile_filename():
    """Return full pathname of installed Makefile from the Python build."""
    if python_build:
        return './Makefile'
    lib_dir = get_python_lib(plat_specific=1, standard_lib=1)
    return os.path.join(lib_dir, "config", "Makefile")


def parse_config_h(fp, g=None):
    """Parse a config.h-style file.

    A dictionary containing name/value pairs is returned.  If an
    optional dictionary is passed in as the second argument, it is
    used instead of a new dictionary.
    """
    if g is None:
        g = {}
    define_rx = re.compile("#define ([A-Z][A-Z0-9_]+) (.*)\n")
    undef_rx = re.compile("/[*] #undef ([A-Z][A-Z0-9_]+) [*]/\n")
    #
    while 1:
        line = fp.readline()
        if not line:
            break
        m = define_rx.match(line)
        if m:
            n, v = m.group(1, 2)
            try: v = string.atoi(v)
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
    fp = TextFile(fn, strip_comments=1, skip_blanks=1, join_lines=1)

    if g is None:
        g = {}
    done = {}
    notdone = {}

    while 1:
        line = fp.readline()
        if line is None:                # eof
            break
        m = _variable_rx.match(line)
        if m:
            n, v = m.group(1, 2)
            v = string.strip(v)
            if "$" in v:
                notdone[n] = v
            else:
                try: v = string.atoi(v)
                except ValueError: pass
                done[n] = v

    # do variable interpolation here
    while notdone:
        for name in notdone.keys():
            value = notdone[name]
            m = _findvar1_rx.search(value) or _findvar2_rx.search(value)
            if m:
                n = m.group(1)
                if done.has_key(n):
                    after = value[m.end():]
                    value = value[:m.start()] + str(done[n]) + after
                    if "$" in after:
                        notdone[name] = value
                    else:
                        try: value = string.atoi(value)
                        except ValueError:
                            done[name] = string.strip(value)
                        else:
                            done[name] = value
                        del notdone[name]
                elif notdone.has_key(n):
                    # get it on a subsequent round
                    pass
                else:
                    done[n] = ""
                    after = value[m.end():]
                    value = value[:m.start()] + after
                    if "$" in after:
                        notdone[name] = value
                    else:
                        try: value = string.atoi(value)
                        except ValueError:
                            done[name] = string.strip(value)
                        else:
                            done[name] = value
                        del notdone[name]
            else:
                # bogus variable reference; just drop it since we can't deal
                del notdone[name]

    fp.close()

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

    while 1:
        m = _findvar1_rx.search(s) or _findvar2_rx.search(s)
        if m:
            name = m.group(1)
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
    except IOError, msg:
        my_msg = "invalid Python installation: unable to open %s" % filename
        if hasattr(msg, "strerror"):
            my_msg = my_msg + " (%s)" % msg.strerror

        raise DistutilsPlatformError, my_msg
              
    
    # On AIX, there are wrong paths to the linker scripts in the Makefile
    # -- these paths are relative to the Python source, but when installed
    # the scripts are in another directory.
    if python_build:
        g['LDSHARED'] = g['BLDSHARED']

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

    global _config_vars
    _config_vars = g


def _init_mac():
    """Initialize the module as appropriate for Macintosh systems"""
    g = {}
    # set basic install directories
    g['LIBDEST'] = get_python_lib(plat_specific=0, standard_lib=1)
    g['BINLIBDEST'] = get_python_lib(plat_specific=1, standard_lib=1)

    # XXX hmmm.. a normal install puts include files here
    g['INCLUDEPY'] = get_python_inc(plat_specific=0)

    import MacOS
    if not hasattr(MacOS, 'runtimemodel'):
        g['SO'] = '.ppc.slb'
    else:
        g['SO'] = '.%s.slb' % MacOS.runtimemodel

    # XXX are these used anywhere?
    g['install_lib'] = os.path.join(EXEC_PREFIX, "Lib")
    g['install_platlib'] = os.path.join(EXEC_PREFIX, "Mac", "Lib")

    global _config_vars
    _config_vars = g


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
