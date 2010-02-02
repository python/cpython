"""Provide access to Python's configuration information.  The specific
configuration variables available depend heavily on the platform and
configuration.  The values may be retrieved using
get_config_var(name), and the list of variables is available via
get_config_vars().keys().  Additional convenience functions are also
available.

Written by:   Fred L. Drake, Jr.
Email:        <fdrake@acm.org>

**This module has been moved out of Distutils and will be removed from
Python in the next version (3.3)**
"""

__revision__ = "$Id$"

import re
from warnings import warn

# importing sysconfig from Lib
# to avoid this module to shadow it
_sysconfig = __import__('sysconfig')

# names defined here to keep backward compatibility
# for APIs that were relocated
get_python_version = _sysconfig.get_python_version
get_config_h_filename = _sysconfig.get_config_h_filename
parse_config_h = _sysconfig.parse_config_h
get_config_vars = _sysconfig.get_config_vars
get_config_var = _sysconfig.get_config_var
from distutils.ccompiler import customize_compiler

_DEPRECATION_MSG = ("distutils.sysconfig.%s is deprecated. "
                    "Use the APIs provided by the sysconfig module instead")

def _get_project_base():
    return _sysconfig._PROJECT_BASE

project_base = _get_project_base()

class _DeprecatedBool(int):
    def __nonzero__(self):
        warn(_DEPRECATION_MSG % 'get_python_version', DeprecationWarning)
        return super(_DeprecatedBool, self).__nonzero__()

def _python_build():
    return _DeprecatedBool(_sysconfig.is_python_build())

python_build = _python_build()

def get_python_inc(plat_specific=0, prefix=None):
    """This function is deprecated.

    Return the directory containing installed Python header files.

    If 'plat_specific' is false (the default), this is the path to the
    non-platform-specific header files, i.e. Python.h and so on;
    otherwise, this is the path to platform-specific header files
    (namely pyconfig.h).

    If 'prefix' is supplied, use it instead of sys.prefix or
    sys.exec_prefix -- i.e., ignore 'plat_specific'.
    """
    warn(_DEPRECATION_MSG % 'get_python_inc', DeprecationWarning)
    get_path = _sysconfig.get_path

    if prefix is not None:
        vars = {'base': prefix}
        return get_path('include', vars=vars)

    if not plat_specific:
        return get_path('include')
    else:
        return get_path('platinclude')

def get_python_lib(plat_specific=False, standard_lib=False, prefix=None):
    """This function is deprecated.

    Return the directory containing the Python library (standard or
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
    warn(_DEPRECATION_MSG % 'get_python_lib', DeprecationWarning)
    vars = {}
    get_path = _sysconfig.get_path
    if prefix is not None:
        if plat_specific:
            vars['platbase'] = prefix
        else:
            vars['base'] = prefix

    if standard_lib:
        if plat_specific:
            return get_path('platstdlib', vars=vars)
        else:
            return get_path('stdlib', vars=vars)
    else:
        if plat_specific:
            return get_path('platlib', vars=vars)
        else:
            return get_path('purelib', vars=vars)

def get_makefile_filename():
    """This function is deprecated.

    Return full pathname of installed Makefile from the Python build.
    """
    warn(_DEPRECATION_MSG % 'get_makefile_filename', DeprecationWarning)
    return _sysconfig._get_makefile_filename()

# Regexes needed for parsing Makefile (and similar syntaxes,
# like old-style Setup files).
_variable_rx = re.compile("([a-zA-Z][a-zA-Z0-9_]+)\s*=\s*(.*)")
_findvar1_rx = re.compile(r"\$\(([A-Za-z][A-Za-z0-9_]*)\)")
_findvar2_rx = re.compile(r"\${([A-Za-z][A-Za-z0-9_]*)}")

def parse_makefile(fn, g=None):
    """This function is deprecated.

    Parse a Makefile-style file.

    A dictionary containing name/value pairs is returned.  If an
    optional dictionary is passed in as the second argument, it is
    used instead of a new dictionary.
    """
    warn(_DEPRECATION_MSG % 'parse_makefile', DeprecationWarning)
    return _sysconfig._parse_makefile(fn, g)

def expand_makefile_vars(s, vars):
    """This function is deprecated.

    Expand Makefile-style variables -- "${foo}" or "$(foo)" -- in
    'string' according to 'vars' (a dictionary mapping variable names to
    values).  Variables not present in 'vars' are silently expanded to the
    empty string.  The variable values in 'vars' should not contain further
    variable expansions; if 'vars' is the output of 'parse_makefile()',
    you're fine.  Returns a variable-expanded version of 's'.
    """
    warn('this function will be removed in then next version of Python',
         DeprecationWarning)

    # This algorithm does multiple expansion, so if vars['foo'] contains
    # "${bar}", it will expand ${foo} to ${bar}, and then expand
    # ${bar}... and so forth.  This is fine as long as 'vars' comes from
    # 'parse_makefile()', which takes care of such expansions eagerly,
    # according to make's variable expansion semantics.

    while 1:
        m = _findvar1_rx.search(s) or _findvar2_rx.search(s)
        if m:
            (beg, end) = m.span()
            s = s[0:beg] + vars.get(m.group(1)) + s[end:]
        else:
            break
    return s
