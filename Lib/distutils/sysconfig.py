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

from sysconfig import _PREFIX as PREFIX
from sysconfig import _BASE_PREFIX as BASE_PREFIX
from sysconfig import _EXEC_PREFIX as EXEC_PREFIX
from sysconfig import _BASE_EXEC_PREFIX as BASE_EXEC_PREFIX
from sysconfig import _PROJECT_BASE as project_base
from sysconfig import _PYTHON_BUILD as python_build
from sysconfig import _CONFIG_VARS as _config_vars
from sysconfig import _init_posix as sysconfig_init_posix
from sysconfig import parse_config_h as sysconfig_parse_config_h
from sysconfig import _parse_makefile as sysconfig_parse_makefile

from sysconfig import _init_non_posix
from sysconfig import _is_python_source_dir
from sysconfig import _sys_home

from sysconfig import _variable_rx
from sysconfig import _findvar1_rx
from sysconfig import _findvar2_rx

from sysconfig import build_flags
from sysconfig import customize_compiler
from sysconfig import expand_makefile_vars
from sysconfig import is_python_build
from sysconfig import get_config_h_filename
from sysconfig import get_config_var
from sysconfig import get_config_vars
from sysconfig import get_makefile_filename
from sysconfig import get_python_inc
from sysconfig import get_python_version
from sysconfig import get_python_lib

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
