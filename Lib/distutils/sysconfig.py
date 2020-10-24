"""Provide access to Python's configuration information.  The specific
configuration variables available depend heavily on the platform and
configuration.  The values may be retrieved using
get_config_var(name), and the list of variables is available via
get_config_vars().keys().  Additional convenience functions are also
available.

Written by:   Fred L. Drake, Jr.
Email:        <fdrake@acm.org>
"""

import _imp
import os
import re
import sys
import warnings

from functools import partial

from .errors import DistutilsPlatformError

from sysconfig import (
    _PREFIX as PREFIX,
    _BASE_PREFIX as BASE_PREFIX,
    _EXEC_PREFIX as EXEC_PREFIX,
    _BASE_EXEC_PREFIX as BASE_EXEC_PREFIX,
    _PROJECT_BASE as project_base,
    _PYTHON_BUILD as python_build,
    _CONFIG_VARS as _config_vars,
    _init_posix as sysconfig_init_posix,
    parse_config_h as sysconfig_parse_config_h,
    _parse_makefile as sysconfig_parse_makefile,

    _init_non_posix,
    _is_python_source_dir,
    _sys_home,

    _variable_rx,
    _findvar1_rx,
    _findvar2_rx,

    expand_makefile_vars,
    is_python_build,
    get_config_h_filename,
    get_config_var,
    get_config_vars,
    get_makefile_filename,
    get_python_inc,
    get_python_version,
    get_python_lib,
)

if os.name == "nt":
    from sysconfig import _fix_pcbuild

warnings.warn(
    'the distutils.sysconfig module is deprecated, use sysconfig instead',
    DeprecationWarning,
    stacklevel=2
)


# Following functions are the same as in sysconfig but with different API
def parse_config_h(fp, g=None):
    return sysconfig_parse_config_h(fp, vars=g)


def parse_makefile(fn, g=None):
    return sysconfig_parse_makefile(fn, vars=g, keep_unresolved=False)

_python_build = partial(is_python_build, check_home=True)
_init_posix = partial(sysconfig_init_posix, _config_vars)
_init_nt = partial(_init_non_posix, _config_vars)


# customize_compiler is deprecated and won't be moved to another module
def customize_compiler(compiler):
    """Do any platform-specific customization of a CCompiler instance.

    Mainly needed on Unix, so we can plug in the information that
    varies across Unices and is stored in Python's Makefile.
    """
    if compiler.compiler_type == "unix":
        if sys.platform == "darwin":
            # Perform first-time customization of compiler-related
            # config vars on OS X now that we know we need a compiler.
            # This is primarily to support Pythons from binary
            # installers.  The kind and paths to build tools on
            # the user system may vary significantly from the system
            # that Python itself was built on.  Also the user OS
            # version and build tools may not support the same set
            # of CPU architectures for universal builds.
            global _config_vars
            # Use get_config_var() to ensure _config_vars is initialized.
            if not get_config_var('CUSTOMIZED_OSX_COMPILER'):
                import _osx_support
                _osx_support.customize_compiler(_config_vars)
                _config_vars['CUSTOMIZED_OSX_COMPILER'] = 'True'

        (cc, cxx, cflags, ccshared, ldshared, shlib_suffix, ar, ar_flags) = \
            get_config_vars('CC', 'CXX', 'CFLAGS',
                            'CCSHARED', 'LDSHARED', 'SHLIB_SUFFIX', 'AR', 'ARFLAGS')

        if 'CC' in os.environ:
            newcc = os.environ['CC']
            if (sys.platform == 'darwin'
                    and 'LDSHARED' not in os.environ
                    and ldshared.startswith(cc)):
                # On OS X, if CC is overridden, use that as the default
                #       command for LDSHARED as well
                ldshared = newcc + ldshared[len(cc):]
            cc = newcc
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
            cflags = cflags + ' ' + os.environ['CFLAGS']
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

        compiler.shared_lib_extension = shlib_suffix
