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


PREFIX = os.path.normpath(sys.prefix)
EXEC_PREFIX = os.path.normpath(sys.exec_prefix)


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
        if platform_specific:
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
        

def get_config_h_filename():
    """Return full pathname of installed config.h file."""
    inc_dir = get_python_inc(plat_specific=1)
    return os.path.join(inc_dir, "config.h")


def get_makefile_filename():
    """Return full pathname of installed Makefile from the Python build."""
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

def parse_makefile(fp, g=None):
    """Parse a Makefile-style file.

    A dictionary containing name/value pairs is returned.  If an
    optional dictionary is passed in as the second argument, it is
    used instead of a new dictionary.

    """
    if g is None:
        g = {}
    variable_rx = re.compile("([a-zA-Z][a-zA-Z0-9_]+)\s*=\s*(.*)\n")
    done = {}
    notdone = {}
    #
    while 1:
        line = fp.readline()
        if not line:
            break
        m = variable_rx.match(line)
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
    findvar1_rx = re.compile(r"\$\(([A-Za-z][A-Za-z0-9_]*)\)")
    findvar2_rx = re.compile(r"\${([A-Za-z][A-Za-z0-9_]*)}")
    while notdone:
        for name in notdone.keys():
            value = notdone[name]
            m = findvar1_rx.search(value)
            if not m:
                m = findvar2_rx.search(value)
            if m:
                n = m.group(1)
                if done.has_key(n):
                    after = value[m.end():]
                    value = value[:m.start()] + done[n] + after
                    if "$" in after:
                        notdone[name] = value
                    else:
                        try: value = string.atoi(value)
                        except ValueError: pass
                        done[name] = string.strip(value)
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
                        except ValueError: pass
                        done[name] = string.strip(value)
                        del notdone[name]
            else:
                # bogus variable reference; just drop it since we can't deal
                del notdone[name]

    # "Fix" all pathnames in the Makefile that are explicitly relative,
    # ie. that start with "./".  This is a kludge to fix the "./ld_so_aix"
    # problem, the nature of which is that Python's installed Makefile
    # refers to "./ld_so_aix", but when we are building extensions we are
    # far from the directory where Python's Makefile (and ld_so_aix, for
    # that matter) is installed.  Unfortunately, there are several other
    # relative pathnames in the Makefile, and this fix doesn't fix them,
    # because the layout of Python's source tree -- which is what the
    # Makefile refers to -- is not fully preserved in the Python
    # installation.  Grumble.
    from os.path import normpath, join, dirname
    for (name, value) in done.items():
        if value[0:2] == "./":
            done[name] = normpath(join(dirname(fp.name), value))

    # save the results in the global dictionary
    g.update(done)
    return g


def _init_posix():
    """Initialize the module as appropriate for POSIX systems."""
    g = globals()
    # load the installed Makefile:
    try:
        filename = get_makefile_filename()
        file = open(filename)
    except IOError, msg:
        my_msg = "invalid Python installation: unable to open %s" % filename
        if hasattr(msg, "strerror"):
            my_msg = my_msg + " (%s)" % msg.strerror

        raise DistutilsPlatformError, my_msg
              
    parse_makefile(file, g)


def _init_nt():
    """Initialize the module as appropriate for NT"""
    g = globals()
    # set basic install directories
    g['LIBDEST'] = get_python_lib(plat_specific=0, standard_lib=1)
    g['BINLIBDEST'] = get_python_lib(plat_specific=1, standard_lib=1)

    # XXX hmmm.. a normal install puts include files here
    g['INCLUDEPY'] = get_python_inc(plat_specific=0)

    g['SO'] = '.pyd'
    g['exec_prefix'] = EXEC_PREFIX

    # These are needed for the CygwinCCompiler and Mingw32CCompiler
    # classes, which are just UnixCCompiler classes that happen to work on
    # Windows.  UnixCCompiler expects to find these values in sysconfig, so
    # here they are.  The fact that other Windows compilers don't need
    # these values is pure luck (hmmm).
    g['CC'] = "cc"                      # not gcc?
    g['RANLIB'] = "ranlib"
    g['AR'] = "ar"
    g['OPT'] = "-O2"
    g['SO'] = ".pyd"
    g['LDSHARED'] = "ld"
    g['CCSHARED'] = ""
    g['EXE'] = ".exe"


def _init_mac():
    """Initialize the module as appropriate for Macintosh systems"""
    g = globals()
    # set basic install directories
    g['LIBDEST'] = get_python_lib(plat_specific=0, standard_lib=1)
    g['BINLIBDEST'] = get_python_lib(plat_specific=1, standard_lib=1)

    # XXX hmmm.. a normal install puts include files here
    g['INCLUDEPY'] = get_python_inc(plat_specific=0)

    g['SO'] = '.ppc.slb'
    g['exec_prefix'] = EXEC_PREFIX
    print sys.prefix, PREFIX

    # XXX are these used anywhere?
    g['install_lib'] = os.path.join(EXEC_PREFIX, "Lib")
    g['install_platlib'] = os.path.join(EXEC_PREFIX, "Mac", "Lib")


try:
    exec "_init_" + os.name
except NameError:
    # not needed for this platform
    pass
else:
    exec "_init_%s()" % os.name


del _init_posix
del _init_nt
del _init_mac
