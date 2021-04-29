import os
import sys

_INSTALL_SCHEMES = {
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

_get_preferred_schemes = None


def _load_vendor_schemes():
    # add vendor defined schemes
    try:
        import _vendor.config

        extra_schemes = _vendor.config.EXTRA_INSTALL_SCHEMES
        _INSTALL_SCHEMES.update({
            name: scheme
            for name, scheme in extra_schemes.items()
            if name not in _INSTALL_SCHEMES
        })
    except (ModuleNotFoundError, AttributeError):
        pass


_load_vendor_schemes()


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


_HAS_USER_BASE = (_getuserbase() is not None)

if _HAS_USER_BASE:
    _INSTALL_SCHEMES |= {
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
_USER_BASE = None


def _safe_realpath(path):
    try:
        return os.path.realpath(path)
    except OSError:
        return path


if sys.executable:
    _PROJECT_BASE = os.path.dirname(_safe_realpath(sys.executable))
else:
    # sys.executable can be empty if argv[0] has been changed and Python is
    # unable to retrieve the real program name
    _PROJECT_BASE = _safe_realpath(os.getcwd())

if (os.name == 'nt' and
    _PROJECT_BASE.lower().endswith(('\\pcbuild\\win32', '\\pcbuild\\amd64'))):
    _PROJECT_BASE = _safe_realpath(os.path.join(_PROJECT_BASE, os.path.pardir, os.path.pardir))

# set for cross builds
if "_PYTHON_PROJECT_BASE" in os.environ:
    _PROJECT_BASE = _safe_realpath(os.environ["_PYTHON_PROJECT_BASE"])


def _is_python_source_dir(d):
    for fn in ("Setup", "Setup.local"):
        if os.path.isfile(os.path.join(d, "Modules", fn)):
            return True
    return False


_SYS_HOME = getattr(sys, '_home', None)


if os.name == 'nt':
    def _fix_pcbuild(d):
        if d and os.path.normcase(d).startswith(
                os.path.normcase(os.path.join(_PREFIX, "PCbuild"))):
            return _PREFIX
        return d
    _PROJECT_BASE = _fix_pcbuild(_PROJECT_BASE)
    _SYS_HOME = _fix_pcbuild(_SYS_HOME)


def is_python_build(check_home=False):
    if check_home and _SYS_HOME:
        return _is_python_source_dir(_SYS_HOME)
    return _is_python_source_dir(getattr(sys.modules[__name__], '_PROJECT_BASE'))


_PYTHON_BUILD = is_python_build(True)

if _PYTHON_BUILD:
    for scheme in ('posix_prefix', 'posix_home'):
        # On POSIX-y platforms, Python will:
        # - Build from .h files in 'headers' (which is only added to the
        #   scheme when building CPython)
        # - Install .h files to 'include'
        scheme = _INSTALL_SCHEMES[scheme]
        scheme['headers'] = scheme['include']
        scheme['include'] = '{srcdir}/Include'
        scheme['platinclude'] = '{projectbase}/.'


_SCHEME_CONFIG_VARS = {
    'prefix': _PREFIX,
    'exec_prefix': _EXEC_PREFIX,
    'py_version': _PY_VERSION,
    'py_version_short': _PY_VERSION_SHORT,
    'py_version_nodot': _PY_VERSION_SHORT_NO_DOT,
    'installed_base': _BASE_PREFIX,
    'base': _PREFIX,
    'installed_platbase': _BASE_EXEC_PREFIX,
    'platbase': _EXEC_PREFIX,
    'projectbase': _PROJECT_BASE,
    'platlibdir': sys.platlibdir,
    'abiflags': getattr(sys, 'abiflags', ''),
    'py_version_nodot_plat': getattr(sys, 'winver', '').replace('.', '')
}


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
        vars = _SCHEME_CONFIG_VARS
    res = {}
    for key, value in _INSTALL_SCHEMES[scheme].items():
        if os.name in ('posix', 'nt'):
            value = os.path.expanduser(value)
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
    if scheme not in _INSTALL_SCHEMES:
        raise ValueError(
            f"{key!r} returned {scheme!r}, which is not a valid scheme "
            f"on this platform"
        )
    return scheme


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
        return _INSTALL_SCHEMES[scheme]
