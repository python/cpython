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

    customize_compiler,
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
