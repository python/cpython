"""Bits of sysconfig that are required at startup (by the site module)"""

import os
import sys


# helpers


def _safe_realpath(path):
    try:
        return os.path.realpath(path)
    except OSError:
        return path


def _is_python_source_dir(d):
    for fn in ("Setup", "Setup.local"):
        if os.path.isfile(os.path.join(d, "Modules", fn)):
            return True
    return False


if os.name == 'nt':
    def _fix_pcbuild(d):
        if d and os.path.normcase(d).startswith(
                os.path.normcase(os.path.join(_PREFIX, "PCbuild"))):
            return _PREFIX
        return d


# constants

_BASE_INSTALL_SCHEMES = {
    'posix_prefix': {
        'stdlib': '{installed_base}/{platlibdir}/python{py_version_short}',
        'platstdlib': '{platbase}/{platlibdir}/python{py_version_short}',
        'purelib': '{base}/lib/python{py_version_short}/site-packages',
        'platlib': '{platbase}/{platlibdir}/python{py_version_short}/site-packages',
        'include':
            '{installed_base}/include/python{py_version_short}{abiflags}',
        'platinclude':
            '{installed_platbase}/include/python{py_version_short}{abiflags}',
        'scripts': '{base}/bin',
        'data': '{base}',
        },
    'posix_home': {
        'stdlib': '{installed_base}/lib/python',
        'platstdlib': '{base}/lib/python',
        'purelib': '{base}/lib/python',
        'platlib': '{base}/lib/python',
        'include': '{installed_base}/include/python',
        'platinclude': '{installed_base}/include/python',
        'scripts': '{base}/bin',
        'data': '{base}',
        },
    'nt': {
        'stdlib': '{installed_base}/Lib',
        'platstdlib': '{base}/Lib',
        'purelib': '{base}/Lib/site-packages',
        'platlib': '{base}/Lib/site-packages',
        'include': '{installed_base}/Include',
        'platinclude': '{installed_base}/Include',
        'scripts': '{base}/Scripts',
        'data': '{base}',
        },
    }
_USER_INSTALL_SCHEMES = {
    # NOTE: When modifying "purelib" scheme, update site._get_path() too.
    'nt_user': {
        'stdlib': '{userbase}/Python{py_version_nodot_plat}',
        'platstdlib': '{userbase}/Python{py_version_nodot_plat}',
        'purelib': '{userbase}/Python{py_version_nodot_plat}/site-packages',
        'platlib': '{userbase}/Python{py_version_nodot_plat}/site-packages',
        'include': '{userbase}/Python{py_version_nodot_plat}/Include',
        'scripts': '{userbase}/Python{py_version_nodot_plat}/Scripts',
        'data': '{userbase}',
        },
    'posix_user': {
        'stdlib': '{userbase}/{platlibdir}/python{py_version_short}',
        'platstdlib': '{userbase}/{platlibdir}/python{py_version_short}',
        'purelib': '{userbase}/lib/python{py_version_short}/site-packages',
        'platlib': '{userbase}/lib/python{py_version_short}/site-packages',
        'include': '{userbase}/include/python{py_version_short}',
        'scripts': '{userbase}/bin',
        'data': '{userbase}',
        },
    'osx_framework_user': {
        'stdlib': '{userbase}/lib/python',
        'platstdlib': '{userbase}/lib/python',
        'purelib': '{userbase}/lib/python/site-packages',
        'platlib': '{userbase}/lib/python/site-packages',
        'include': '{userbase}/include/python{py_version_short}',
        'scripts': '{userbase}/bin',
        'data': '{userbase}',
        },
    }

_SCHEME_KEYS = ('stdlib', 'platstdlib', 'purelib', 'platlib', 'include',
                'scripts', 'data')

_PY_VERSION = sys.version.split()[0]
_PY_VERSION_SHORT = f'{sys.version_info[0]}.{sys.version_info[1]}'
_PY_VERSION_SHORT_NO_DOT = f'{sys.version_info[0]}{sys.version_info[1]}'
_PREFIX = os.path.normpath(sys.prefix)
_BASE_PREFIX = os.path.normpath(sys.base_prefix)
_EXEC_PREFIX = os.path.normpath(sys.exec_prefix)
_BASE_EXEC_PREFIX = os.path.normpath(sys.base_exec_prefix)

_CHEAP_SCHEME_CONFIG_VARS = {
    'prefix': _PREFIX,
    'exec_prefix': _EXEC_PREFIX,
    'py_version': _PY_VERSION,
    'py_version_short': _PY_VERSION_SHORT,
    'py_version_nodot': _PY_VERSION_SHORT_NO_DOT,
    'installed_base': _BASE_PREFIX,
    'base': _PREFIX,
    'installed_platbase': _BASE_EXEC_PREFIX,
    'platbase': _EXEC_PREFIX,
    'platlibdir': sys.platlibdir,
    'abiflags': getattr(sys, 'abiflags', ''),
    'py_version_nodot_plat': getattr(sys, 'winver', '').replace('.', '')
}

_MODULE = sys.modules[__name__]

# uninitialized

_get_preferred_schemes = None


# lazy constants


# NOTE: site.py has copy of this function.
# Sync it when modify this function.
def _getuserbase():
    env_base = os.environ.get("PYTHONUSERBASE", None)
    if env_base:
        return env_base

    # VxWorks has no home directories
    if sys.platform == "vxworks":
        return None

    def joinuser(*args):
        return os.path.expanduser(os.path.join(*args))

    if os.name == "nt":
        base = os.environ.get("APPDATA") or "~"
        return joinuser(base, "Python")

    if sys.platform == "darwin" and sys._framework:
        return joinuser("~", "Library", sys._framework,
                        f"{sys.version_info[0]}.{sys.version_info[1]}")

    return joinuser("~", ".local")


def is_python_build(check_home=False):
    if check_home and _MODULE._SYS_HOME:
        return _is_python_source_dir(_SYS_HOME)
    return _is_python_source_dir(_MODULE._PROJECT_BASE)


def __getattr__(name):
    match name:
        case '_BUILD_TIME_VARS':
            _sysconfigdata = __import__(_get_sysconfigdata_name(), globals(), locals(), ['build_time_vars'], 0)
            value = _sysconfigdata.build_time_vars
        case '_HAS_USER_BASE':
            value = (_getuserbase() is not None)
        case '_PROJECT_BASE':
            # set for cross builds
            if "_PYTHON_PROJECT_BASE" in os.environ:
                value = _safe_realpath(os.environ["_PYTHON_PROJECT_BASE"])
            else:
                if sys.executable:
                    value = os.path.dirname(_safe_realpath(sys.executable))
                else:
                    # sys.executable can be empty if argv[0] has been changed and Python is
                    # unable to retrieve the real program name
                    value = _safe_realpath(os.getcwd())

                if (os.name == 'nt' and
                    value.lower().endswith(('\\pcbuild\\win32', '\\pcbuild\\amd64'))):
                    value = _safe_realpath(os.path.join(_PROJECT_BASE, os.path.pardir, os.path.pardir))

            if os.name == 'nt':
                value = _fix_pcbuild(value)
        case '_SRCDIR':
            if os.name == 'posix' and not _MODULE._PYTHON_BUILD:
                # srcdir is not meaningful since the installation is
                # spread about the filesystem.  We choose the
                # directory containing the Makefile since we know it
                # exists.
                value = os.path.dirname(get_makefile_filename())
            else:
                if 'srcdir' in _MODULE._BUILD_TIME_VARS:
                    value = _MODULE._BUILD_TIME_VARS['srcdir']
                else:
                    value = _MODULE._PROJECT_BASE

                if os.name == 'posix' and _MODULE._PYTHON_BUILD:
                    # If srcdir is a relative path (typically '.' or '..')
                    # then it should be interpreted relative to the directory
                    # containing Makefile.
                    base = os.path.dirname(get_makefile_filename())
                    value = os.path.join(base, value)

            value = _safe_realpath(value)
        case '_SYS_HOME':
            value = getattr(sys, '_home', None)
            if os.name == 'nt':
                value = _fix_pcbuild(value)
        case '_PYTHON_BUILD':
            value = is_python_build(check_home=True)
        case '_SCHEME_CONFIG_VARS':
            value = _CHEAP_SCHEME_CONFIG_VARS.copy()
            value['projectbase'] = _MODULE._PROJECT_BASE
        case _:
            raise AttributeError(f"module '{__name__}' has no attribute '{name}'")
    setattr(_MODULE, name, value)
    return value


# methods


def _get_raw_scheme_paths(scheme):
    # lazy loading of install schemes -- only run the code paths we need to

    # check base schemes
    if scheme in _BASE_INSTALL_SCHEMES:
        if scheme in ('posix_prefix', 'posix_home') and _MODULE._PYTHON_BUILD:
            # On POSIX-y platforms, Python will:
            # - Build from .h files in 'headers' (which is only added to the
            #   scheme when building CPython)
            # - Install .h files to 'include'
            scheme = _BASE_INSTALL_SCHEMES[scheme]
            scheme['headers'] = scheme['include']
            scheme['include'] = '{srcdir}/Include'
            scheme['platinclude'] = '{projectbase}/.'
            return scheme
        return _BASE_INSTALL_SCHEMES[scheme]

    if scheme in _USER_INSTALL_SCHEMES and _MODULE._HAS_USER_BASE:
        return _USER_INSTALL_SCHEMES[scheme]

    # check vendor schemes
    try:
        import _vendor.config

        vendor_schemes = _vendor.config.EXTRA_INSTALL_SCHEMES
    except (ModuleNotFoundError, AttributeError):
        pass
    return vendor_schemes[scheme]


def _get_sysconfigdata_name():
    multiarch = getattr(sys.implementation, '_multiarch', '')
    return os.environ.get(
        '_PYTHON_SYSCONFIGDATA_NAME',
        f'_sysconfigdata_{sys.abiflags}_{sys.platform}_{multiarch}',
    )


def get_makefile_filename():
    """Return the path of the Makefile."""
    if _MODULE._PYTHON_BUILD:
        return os.path.join(_MODULE._SYS_HOME or _MODULE._PROJECT_BASE, "Makefile")
    if hasattr(sys, 'abiflags'):
        config_dir_name = f'config-{_PY_VERSION_SHORT}{sys.abiflags}'
    else:
        config_dir_name = 'config'
    if hasattr(sys.implementation, '_multiarch'):
        config_dir_name += f'-{sys.implementation._multiarch}'
    stdlib = _get_paths(get_default_scheme())['stdlib']
    return os.path.join(stdlib, config_dir_name, 'Makefile')


def _subst_vars(s, local_vars):
    try:
        return s.format(**local_vars)
    except KeyError as var:
        try:
            return s.format(**os.environ)
        except KeyError:
            raise AttributeError(f'{var}') from None


def _expand_vars(scheme, vars):
    if vars is None:
        vars = {}
    vars.update(_CHEAP_SCHEME_CONFIG_VARS)

    res = {}
    for key, value in _get_raw_scheme_paths(scheme).items():
        if os.name in ('posix', 'nt'):
            value = os.path.expanduser(value)

        # these are an expensive and uncommon config vars, let's only load them if we need to
        if '{projectbase}' in value:
            vars['projectbase'] = _MODULE._PROJECT_BASE
        if '{srcdir}' in value:
            vars['srcdir'] = _MODULE._SRCDIR

        res[key] = os.path.normpath(_subst_vars(value, vars))
    return res


def _get_preferred_schemes_default():
    if os.name == 'nt':
        return {
            'prefix': 'nt',
            'home': 'posix_home',
            'user': 'nt_user',
        }
    if sys.platform == 'darwin' and sys._framework:
        return {
            'prefix': 'posix_prefix',
            'home': 'posix_home',
            'user': 'osx_framework_user',
        }
    return {
        'prefix': 'posix_prefix',
        'home': 'posix_home',
        'user': 'posix_user',
    }


def get_preferred_scheme(key):
    global _get_preferred_schemes
    if not _get_preferred_schemes:
        try:
            import _vendor.config

            _get_preferred_schemes = _vendor.config.get_preferred_schemes
        except (ModuleNotFoundError, AttributeError):
            _get_preferred_schemes = _get_preferred_schemes_default

    scheme = _get_preferred_schemes()[key]

    # check our schemes
    if scheme in _BASE_INSTALL_SCHEMES or scheme in _USER_INSTALL_SCHEMES:
        return scheme

    # check vendor schemes
    try:
        import _vendor.config

        vendor_schemes = _vendor.config.EXTRA_INSTALL_SCHEMES
    except (ModuleNotFoundError, AttributeError):
        pass
    else:
        if scheme in vendor_schemes:
            return scheme

    raise ValueError(
        f"{key!r} returned {scheme!r}, which is not a valid scheme "
        f"on this platform"
    )


def get_default_scheme():
    return get_preferred_scheme('prefix')


def _get_paths(scheme=get_default_scheme(), vars=None, expand=True):
    """Return a mapping containing an install scheme.

    ``scheme`` is the install scheme name. If not provided, it will
    return the default scheme for the current platform.
    """
    if expand:
        return _expand_vars(scheme, vars)
    else:
        return _get_raw_scheme_paths(scheme)
