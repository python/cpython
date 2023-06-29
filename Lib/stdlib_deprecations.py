import dataclasses
import re


@dataclasses.dataclass(slots=True, frozen=True)
class Deprecated:
    name: str
    version: tuple[int]
    remove: tuple[int] | None
    message: str | None


def _parse_version(version_str):
    version = tuple(int(part) for part in version_str.split('.'))
    if len(version) < 2:
        raise ValueError(f"invalid Python version: {version_str!r}")
    return version


_DEPRECATIONS = {}
_DEPRECATIONS_CAPI = {}
_REGEX_NAME = r'[A-Za-z_][A-Za-z0-9_]*'
_REGEX_QUALNAME = fr'^{_REGEX_NAME}(\.{_REGEX_NAME})*$'


def _deprecate_api(api_dict, name, version, remove, replace):
    if not re.match(_REGEX_QUALNAME, name):
        raise ValueError(f"invalid name: {name!a}")
    version = _parse_version(version)
    if remove is not None:
        remove = _parse_version(remove)
    if replace is not None:
        msg = f'use {replace}'
    else:
        msg = None
    deprecated = Deprecated(name, version, remove, msg)
    api_dict[deprecated.name] = deprecated


def _deprecate(name, version, *, remove=None, replace=None):
    global _DEPRECATIONS
    _deprecate_api(_DEPRECATIONS, name, version, remove, replace)


def _deprecate_capi(name, version, *, remove=None, replace=None):
    global _DEPRECATIONS_CAPI
    _deprecate_api(_DEPRECATIONS_CAPI, name, version, remove, replace)


# Python 2.6
_deprecate('gzip.GzipFile.filename', '2.6', remove='3.12',
    replace='gzip.GzipFile.name'),

# Python 3.6
_deprecate('asyncore', '3.6', remove='3.12', replace='asyncio'),
_deprecate('asynchat', '3.6', remove='3.12', replace='asyncio'),
_deprecate('smtpd', '3.6', remove='3.12', replace='aiosmtp'),
_deprecate('ssl.RAND_pseudo_bytes', '3.6', remove='3.12',
    replace='os.urandom()'),

# Python 3.7
_deprecate('ssl.match_hostname', '3.7', remove='3.12'),
_deprecate('ssl.wrap_socket', '3.7', remove='3.12',
    replace='ssl.SSLContext.wrap_socket()'),
_deprecate('locale.format', '3.7', remove='3.12',
    replace='locale.format_string()'),

# Python 3.10
_deprecate('io.OpenWrapper', '3.10', remove='3.12', replace='open()'),
_deprecate('_pyio.OpenWrapper', '3.10', remove='3.12', replace='open()'),
_deprecate('xml.etree.ElementTree.copy', '3.10', remove='3.12',
    replace='copy.copy()'),
_deprecate('zipimport.zipimporter.find_loader', '3.10', remove='3.12',
    replace='find_spec() method: PEP 451'),
_deprecate('zipimport.zipimporter.find_module', '3.10', remove='3.12',
    replace='find_spec() method: PEP 451'),

# Python 3.11
_deprecate('aifc', '3.11', remove='3.13'),
_deprecate('audioop', '3.11', remove='3.13'),
_deprecate('cgi', '3.11', remove='3.13'),
_deprecate('cgitb', '3.11', remove='3.13'),
_deprecate('chunk', '3.11', remove='3.13'),
_deprecate('crypt', '3.11', remove='3.13'),
_deprecate('imghdr', '3.11', remove='3.13'),
_deprecate('mailcap', '3.11', remove='3.13'),
_deprecate('msilib', '3.11', remove='3.13'),
_deprecate('nis', '3.11', remove='3.13'),
_deprecate('nntplib', '3.11', remove='3.13'),
_deprecate('ossaudiodev', '3.11', remove='3.13'),
_deprecate('pipes', '3.11', remove='3.13'),
_deprecate('sndhdr', '3.11', remove='3.13'),
_deprecate('spwd', '3.11', remove='3.13'),
_deprecate('sunau', '3.11', remove='3.13'),
_deprecate('telnetlib', '3.11', remove='3.13'),
_deprecate('uu', '3.11', remove='3.13'),
_deprecate('xdrlib', '3.11', remove='3.13'),

# Python 3.12
_deprecate('datetime.datetime.utcnow', '3.12',
    replace='datetime.datetime.now(tz=datetime.UTC)'),
_deprecate('datetime.datetime.utcfromtimestamp', '3.12',
    replace='datetime.datetime.fromtimestamp(tz=datetime.UTC)'),
_deprecate('calendar.January', '3.12'),
_deprecate('calendar.February', '3.12'),
_deprecate('sys.last_value', '3.12'),
_deprecate('sys.last_traceback', '3.12'),
_deprecate('sys.last_exc', '3.12'),
_deprecate('xml.etree.ElementTree.__bool__', '3.12'),

# Python 3.13
_deprecate('ctypes.SetPointerType', '3.13', remove='3.15'),
_deprecate('ctypes.ARRAY', '3.13', remove='3.15'),
_deprecate('wave.Wave_read.getmark', '3.13', remove='3.15'),
_deprecate('wave.Wave_read.getmarkers', '3.13', remove='3.15'),
_deprecate('wave.Wave_read.setmark', '3.13', remove='3.15'),


# C API: Python 3.10
for name in (
    'PyUnicode_AS_DATA',
    'PyUnicode_AS_UNICODE',
    'PyUnicode_AsUnicodeAndSize',
    'PyUnicode_AsUnicode',
    'PyUnicode_FromUnicode',
    'PyUnicode_GET_DATA_SIZE',
    'PyUnicode_GET_SIZE',
    'PyUnicode_GetSize',
    'PyUnicode_IS_COMPACT',
    'PyUnicode_IS_READY',
    'PyUnicode_READY',
    'Py_UNICODE_WSTR_LENGTH',
    '_PyUnicode_AsUnicode',
    'PyUnicode_WCHAR_KIND',
    'PyUnicodeObject',
    'PyUnicode_InternImmortal',
):
    _deprecate_capi(name, '3.10', remove='3.12')

# C API: Python 3.12
_deprecate_capi('PyDictObject.ma_version_tag', '3.12', remove='3.14'),
for name, replace in (
    ('Py_DebugFlag', 'PyConfig.parser_debug'),
    ('Py_VerboseFlag', 'PyConfig.verbose'),
    ('Py_QuietFlag', 'PyConfig.quiet'),
    ('Py_InteractiveFlag', 'PyConfig.interactive'),
    ('Py_InspectFlag', 'PyConfig.inspect'),
    ('Py_OptimizeFlag', 'PyConfig.optimization_level'),
    ('Py_NoSiteFlag', 'PyConfig.site_import'),
    ('Py_BytesWarningFlag', 'PyConfig.bytes_warning'),
    ('Py_FrozenFlag', 'PyConfig.pathconfig_warnings'),
    ('Py_IgnoreEnvironmentFlag', 'PyConfig.use_environment'),
    ('Py_DontWriteBytecodeFlag', 'PyConfig.write_bytecode'),
    ('Py_NoUserSiteDirectory', 'PyConfig.user_site_directory'),
    ('Py_UnbufferedStdioFlag', 'PyConfig.buffered_stdio'),
    ('Py_HashRandomizationFlag', 'PyConfig.hash_seed'),
    ('Py_IsolatedFlag', 'PyConfig.isolated'),
    ('Py_LegacyWindowsFSEncodingFlag', 'PyPreConfig.legacy_windows_fs_encoding'),
    ('Py_LegacyWindowsStdioFlag', 'PyConfig.legacy_windows_stdio'),
    ('Py_FileSystemDefaultEncoding', 'PyConfig.filesystem_encoding'),
    ('Py_FileSystemDefaultEncodeErrors', 'PyConfig.filesystem_errors'),
    ('Py_UTF8Mode', 'PyPreConfig.utf8_mode'),
):
    _deprecate_capi(name, '3.12', replace=replace)

_deprecate_capi('PyErr_Display', '3.12', replace='PyErr_DisplayException()'),
_deprecate_capi('_PyErr_ChainExceptions', '3.12', replace='_PyErr_ChainExceptions1()'),


def get_deprecated(name):
    try:
        return _DEPRECATIONS[name]
    except KeyError:
        pass

    parts = name.split('.')
    if len(parts) == 1:
        return False

    module_name = parts[0]
    return _DEPRECATIONS.get(module_name)


def get_capi_deprecated(name):
    return _DEPRECATIONS_CAPI.get(name)
