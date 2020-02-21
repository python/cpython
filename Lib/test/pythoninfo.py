"""
Collect various information about Python to help debugging test failures.
"""
from __future__ import print_function
import errno
import re
import sys
import traceback
import warnings


def normalize_text(text):
    if text is None:
        return None
    text = str(text)
    text = re.sub(r'\s+', ' ', text)
    return text.strip()


class PythonInfo:
    def __init__(self):
        self.info = {}

    def add(self, key, value):
        if key in self.info:
            raise ValueError("duplicate key: %r" % key)

        if value is None:
            return

        if not isinstance(value, int):
            if not isinstance(value, str):
                # convert other objects like sys.flags to string
                value = str(value)

            value = value.strip()
            if not value:
                return

        self.info[key] = value

    def get_infos(self):
        """
        Get information as a key:value dictionary where values are strings.
        """
        return {key: str(value) for key, value in self.info.items()}


def copy_attributes(info_add, obj, name_fmt, attributes, *, formatter=None):
    for attr in attributes:
        value = getattr(obj, attr, None)
        if value is None:
            continue
        name = name_fmt % attr
        if formatter is not None:
            value = formatter(attr, value)
        info_add(name, value)


def copy_attr(info_add, name, mod, attr_name):
    try:
        value = getattr(mod, attr_name)
    except AttributeError:
        return
    info_add(name, value)


def call_func(info_add, name, mod, func_name, *, formatter=None):
    try:
        func = getattr(mod, func_name)
    except AttributeError:
        return
    value = func()
    if formatter is not None:
        value = formatter(value)
    info_add(name, value)


def collect_sys(info_add):
    attributes = (
        '_framework',
        'abiflags',
        'api_version',
        'builtin_module_names',
        'byteorder',
        'dont_write_bytecode',
        'executable',
        'flags',
        'float_info',
        'float_repr_style',
        'hash_info',
        'hexversion',
        'implementation',
        'int_info',
        'maxsize',
        'maxunicode',
        'path',
        'platform',
        'prefix',
        'thread_info',
        'version',
        'version_info',
        'winver',
    )
    copy_attributes(info_add, sys, 'sys.%s', attributes)

    call_func(info_add, 'sys.androidapilevel', sys, 'getandroidapilevel')
    call_func(info_add, 'sys.windowsversion', sys, 'getwindowsversion')

    encoding = sys.getfilesystemencoding()
    if hasattr(sys, 'getfilesystemencodeerrors'):
        encoding = '%s/%s' % (encoding, sys.getfilesystemencodeerrors())
    info_add('sys.filesystem_encoding', encoding)

    for name in ('stdin', 'stdout', 'stderr'):
        stream = getattr(sys, name)
        if stream is None:
            continue
        encoding = getattr(stream, 'encoding', None)
        if not encoding:
            continue
        errors = getattr(stream, 'errors', None)
        if errors:
            encoding = '%s/%s' % (encoding, errors)
        info_add('sys.%s.encoding' % name, encoding)

    # Were we compiled --with-pydebug or with #define Py_DEBUG?
    Py_DEBUG = hasattr(sys, 'gettotalrefcount')
    if Py_DEBUG:
        text = 'Yes (sys.gettotalrefcount() present)'
    else:
        text = 'No (sys.gettotalrefcount() missing)'
    info_add('Py_DEBUG', text)


def collect_platform(info_add):
    import platform

    arch = platform.architecture()
    arch = ' '.join(filter(bool, arch))
    info_add('platform.architecture', arch)

    info_add('platform.python_implementation',
             platform.python_implementation())
    info_add('platform.platform',
             platform.platform(aliased=True))

    libc_ver = ('%s %s' % platform.libc_ver()).strip()
    if libc_ver:
        info_add('platform.libc_ver', libc_ver)


def collect_locale(info_add):
    import locale

    info_add('locale.encoding', locale.getpreferredencoding(False))


def collect_builtins(info_add):
    info_add('builtins.float.float_format', float.__getformat__("float"))
    info_add('builtins.float.double_format', float.__getformat__("double"))


def collect_urandom(info_add):
    import os

    if hasattr(os, 'getrandom'):
        # PEP 524: Check if system urandom is initialized
        try:
            try:
                os.getrandom(1, os.GRND_NONBLOCK)
                state = 'ready (initialized)'
            except BlockingIOError as exc:
                state = 'not seeded yet (%s)' % exc
            info_add('os.getrandom', state)
        except OSError as exc:
            # Python was compiled on a more recent Linux version
            # than the current Linux kernel: ignore OSError(ENOSYS)
            if exc.errno != errno.ENOSYS:
                raise


def collect_os(info_add):
    import os

    def format_attr(attr, value):
        if attr in ('supports_follow_symlinks', 'supports_fd',
                    'supports_effective_ids'):
            return str(sorted(func.__name__ for func in value))
        else:
            return value

    attributes = (
        'name',
        'supports_bytes_environ',
        'supports_effective_ids',
        'supports_fd',
        'supports_follow_symlinks',
    )
    copy_attributes(info_add, os, 'os.%s', attributes, formatter=format_attr)

    call_func(info_add, 'os.getcwd', os, 'getcwd')

    call_func(info_add, 'os.getuid', os, 'getuid')
    call_func(info_add, 'os.getgid', os, 'getgid')
    call_func(info_add, 'os.uname', os, 'uname')

    def format_groups(groups):
        return ', '.join(map(str, groups))

    call_func(info_add, 'os.getgroups', os, 'getgroups', formatter=format_groups)

    if hasattr(os, 'getlogin'):
        try:
            login = os.getlogin()
        except OSError:
            # getlogin() fails with "OSError: [Errno 25] Inappropriate ioctl
            # for device" on Travis CI
            pass
        else:
            info_add("os.login", login)

    call_func(info_add, 'os.cpu_count', os, 'cpu_count')
    call_func(info_add, 'os.getloadavg', os, 'getloadavg')

    # Environment variables used by the stdlib and tests. Don't log the full
    # environment: filter to list to not leak sensitive information.
    #
    # HTTP_PROXY is not logged because it can contain a password.
    ENV_VARS = frozenset((
        "APPDATA",
        "AR",
        "ARCHFLAGS",
        "ARFLAGS",
        "AUDIODEV",
        "CC",
        "CFLAGS",
        "COLUMNS",
        "COMPUTERNAME",
        "COMSPEC",
        "CPP",
        "CPPFLAGS",
        "DISPLAY",
        "DISTUTILS_DEBUG",
        "DISTUTILS_USE_SDK",
        "DYLD_LIBRARY_PATH",
        "ENSUREPIP_OPTIONS",
        "HISTORY_FILE",
        "HOME",
        "HOMEDRIVE",
        "HOMEPATH",
        "IDLESTARTUP",
        "LANG",
        "LDFLAGS",
        "LDSHARED",
        "LD_LIBRARY_PATH",
        "LINES",
        "MACOSX_DEPLOYMENT_TARGET",
        "MAILCAPS",
        "MAKEFLAGS",
        "MIXERDEV",
        "MSSDK",
        "PATH",
        "PATHEXT",
        "PIP_CONFIG_FILE",
        "PLAT",
        "POSIXLY_CORRECT",
        "PY_SAX_PARSER",
        "ProgramFiles",
        "ProgramFiles(x86)",
        "RUNNING_ON_VALGRIND",
        "SDK_TOOLS_BIN",
        "SERVER_SOFTWARE",
        "SHELL",
        "SOURCE_DATE_EPOCH",
        "SYSTEMROOT",
        "TEMP",
        "TERM",
        "TILE_LIBRARY",
        "TIX_LIBRARY",
        "TMP",
        "TMPDIR",
        "TRAVIS",
        "TZ",
        "USERPROFILE",
        "VIRTUAL_ENV",
        "WAYLAND_DISPLAY",
        "WINDIR",
        "_PYTHON_HOST_PLATFORM",
        "_PYTHON_PROJECT_BASE",
        "_PYTHON_SYSCONFIGDATA_NAME",
        "__PYVENV_LAUNCHER__",
    ))
    for name, value in os.environ.items():
        uname = name.upper()
        if (uname in ENV_VARS
           # Copy PYTHON* and LC_* variables
           or uname.startswith(("PYTHON", "LC_"))
           # Visual Studio: VS140COMNTOOLS
           or (uname.startswith("VS") and uname.endswith("COMNTOOLS"))):
            info_add('os.environ[%s]' % name, value)

    if hasattr(os, 'umask'):
        mask = os.umask(0)
        os.umask(mask)
        info_add("os.umask", '%03o' % mask)


def collect_pwd(info_add):
    try:
        import pwd
    except ImportError:
        return
    import os

    uid = os.getuid()
    try:
        entry = pwd.getpwuid(uid)
    except KeyError:
        entry = None

    info_add('pwd.getpwuid(%s)'% uid,
             entry if entry is not None else '<KeyError>')

    if entry is None:
        # there is nothing interesting to read if the current user identifier
        # is not the password database
        return

    if hasattr(os, 'getgrouplist'):
        groups = os.getgrouplist(entry.pw_name, entry.pw_gid)
        groups = ', '.join(map(str, groups))
        info_add('os.getgrouplist', groups)


def collect_readline(info_add):
    try:
        import readline
    except ImportError:
        return

    def format_attr(attr, value):
        if isinstance(value, int):
            return "%#x" % value
        else:
            return value

    attributes = (
        "_READLINE_VERSION",
        "_READLINE_RUNTIME_VERSION",
        "_READLINE_LIBRARY_VERSION",
    )
    copy_attributes(info_add, readline, 'readline.%s', attributes,
                    formatter=format_attr)

    if not hasattr(readline, "_READLINE_LIBRARY_VERSION"):
        # _READLINE_LIBRARY_VERSION has been added to CPython 3.7
        doc = getattr(readline, '__doc__', '')
        if 'libedit readline' in doc:
            info_add('readline.library', 'libedit readline')
        elif 'GNU readline' in doc:
            info_add('readline.library', 'GNU readline')


def collect_gdb(info_add):
    import subprocess

    try:
        proc = subprocess.Popen(["gdb", "-nx", "--version"],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                universal_newlines=True)
        version = proc.communicate()[0]
    except OSError:
        return

    # Only keep the first line
    version = version.splitlines()[0]
    info_add('gdb_version', version)


def collect_tkinter(info_add):
    try:
        import _tkinter
    except ImportError:
        pass
    else:
        attributes = ('TK_VERSION', 'TCL_VERSION')
        copy_attributes(info_add, _tkinter, 'tkinter.%s', attributes)

    try:
        import tkinter
    except ImportError:
        pass
    else:
        tcl = tkinter.Tcl()
        patchlevel = tcl.call('info', 'patchlevel')
        info_add('tkinter.info_patchlevel', patchlevel)


def collect_time(info_add):
    import time

    info_add('time.time', time.time())

    attributes = (
        'altzone',
        'daylight',
        'timezone',
        'tzname',
    )
    copy_attributes(info_add, time, 'time.%s', attributes)

    if hasattr(time, 'get_clock_info'):
        for clock in ('clock', 'monotonic', 'perf_counter',
                      'process_time', 'thread_time', 'time'):
            try:
                # prevent DeprecatingWarning on get_clock_info('clock')
                with warnings.catch_warnings(record=True):
                    clock_info = time.get_clock_info(clock)
            except ValueError:
                # missing clock like time.thread_time()
                pass
            else:
                info_add('time.get_clock_info(%s)' % clock, clock_info)


def collect_datetime(info_add):
    try:
        import datetime
    except ImportError:
        return

    info_add('datetime.datetime.now', datetime.datetime.now())


def collect_sysconfig(info_add):
    import sysconfig

    for name in (
        'ABIFLAGS',
        'ANDROID_API_LEVEL',
        'CC',
        'CCSHARED',
        'CFLAGS',
        'CFLAGSFORSHARED',
        'CONFIG_ARGS',
        'HOST_GNU_TYPE',
        'MACHDEP',
        'MULTIARCH',
        'OPT',
        'PY_CFLAGS',
        'PY_CFLAGS_NODIST',
        'PY_CORE_LDFLAGS',
        'PY_LDFLAGS',
        'PY_LDFLAGS_NODIST',
        'PY_STDMODULE_CFLAGS',
        'Py_DEBUG',
        'Py_ENABLE_SHARED',
        'SHELL',
        'SOABI',
        'prefix',
    ):
        value = sysconfig.get_config_var(name)
        if name == 'ANDROID_API_LEVEL' and not value:
            # skip ANDROID_API_LEVEL=0
            continue
        value = normalize_text(value)
        info_add('sysconfig[%s]' % name, value)


def collect_ssl(info_add):
    try:
        import ssl
    except ImportError:
        return

    def format_attr(attr, value):
        if attr.startswith('OP_'):
            return '%#8x' % value
        else:
            return value

    attributes = (
        'OPENSSL_VERSION',
        'OPENSSL_VERSION_INFO',
        'HAS_SNI',
        'OP_ALL',
        'OP_NO_TLSv1_1',
    )
    copy_attributes(info_add, ssl, 'ssl.%s', attributes, formatter=format_attr)


def collect_socket(info_add):
    import socket

    hostname = socket.gethostname()
    info_add('socket.hostname', hostname)


def collect_sqlite(info_add):
    try:
        import sqlite3
    except ImportError:
        return

    attributes = ('version', 'sqlite_version')
    copy_attributes(info_add, sqlite3, 'sqlite3.%s', attributes)


def collect_zlib(info_add):
    try:
        import zlib
    except ImportError:
        return

    attributes = ('ZLIB_VERSION', 'ZLIB_RUNTIME_VERSION')
    copy_attributes(info_add, zlib, 'zlib.%s', attributes)


def collect_expat(info_add):
    try:
        from xml.parsers import expat
    except ImportError:
        return

    attributes = ('EXPAT_VERSION',)
    copy_attributes(info_add, expat, 'expat.%s', attributes)


def collect_decimal(info_add):
    try:
        import _decimal
    except ImportError:
        return

    attributes = ('__libmpdec_version__',)
    copy_attributes(info_add, _decimal, '_decimal.%s', attributes)


def collect_testcapi(info_add):
    try:
        import _testcapi
    except ImportError:
        return

    call_func(info_add, 'pymem.allocator', _testcapi, 'pymem_getallocatorsname')
    copy_attr(info_add, 'pymem.with_pymalloc', _testcapi, 'WITH_PYMALLOC')


def collect_resource(info_add):
    try:
        import resource
    except ImportError:
        return

    limits = [attr for attr in dir(resource) if attr.startswith('RLIMIT_')]
    for name in limits:
        key = getattr(resource, name)
        value = resource.getrlimit(key)
        info_add('resource.%s' % name, value)

    call_func(info_add, 'resource.pagesize', resource, 'getpagesize')


def collect_test_socket(info_add):
    try:
        from test import test_socket
    except ImportError:
        return

    # all check attributes like HAVE_SOCKET_CAN
    attributes = [name for name in dir(test_socket)
                  if name.startswith('HAVE_')]
    copy_attributes(info_add, test_socket, 'test_socket.%s', attributes)


def collect_test_support(info_add):
    try:
        from test import support
    except ImportError:
        return

    attributes = ('IPV6_ENABLED',)
    copy_attributes(info_add, support, 'test_support.%s', attributes)

    call_func(info_add, 'test_support._is_gui_available', support, '_is_gui_available')
    call_func(info_add, 'test_support.python_is_optimized', support, 'python_is_optimized')


def collect_cc(info_add):
    import subprocess
    import sysconfig

    CC = sysconfig.get_config_var('CC')
    if not CC:
        return

    try:
        import shlex
        args = shlex.split(CC)
    except ImportError:
        args = CC.split()
    args.append('--version')
    try:
        proc = subprocess.Popen(args,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,
                                universal_newlines=True)
    except OSError:
        # Cannot run the compiler, for example when Python has been
        # cross-compiled and installed on the target platform where the
        # compiler is missing.
        return

    stdout = proc.communicate()[0]
    if proc.returncode:
        # CC --version failed: ignore error
        return

    text = stdout.splitlines()[0]
    text = normalize_text(text)
    info_add('CC.version', text)


def collect_gdbm(info_add):
    try:
        from _gdbm import _GDBM_VERSION
    except ImportError:
        return

    info_add('gdbm.GDBM_VERSION', '.'.join(map(str, _GDBM_VERSION)))


def collect_get_config(info_add):
    # Get global configuration variables, _PyPreConfig and _PyCoreConfig
    try:
        from _testinternalcapi import get_configs
    except ImportError:
        return

    all_configs = get_configs()
    for config_type in sorted(all_configs):
        config = all_configs[config_type]
        for key in sorted(config):
            info_add('%s[%s]' % (config_type, key), repr(config[key]))


def collect_subprocess(info_add):
    import subprocess
    copy_attributes(info_add, subprocess, 'subprocess.%s', ('_USE_POSIX_SPAWN',))


def collect_windows(info_add):
    try:
        import ctypes
    except ImportError:
        return

    if not hasattr(ctypes, 'WinDLL'):
        return

    ntdll = ctypes.WinDLL('ntdll')
    BOOLEAN = ctypes.c_ubyte

    try:
        RtlAreLongPathsEnabled = ntdll.RtlAreLongPathsEnabled
    except AttributeError:
        res = '<function not available>'
    else:
        RtlAreLongPathsEnabled.restype = BOOLEAN
        RtlAreLongPathsEnabled.argtypes = ()
        res = bool(RtlAreLongPathsEnabled())
    info_add('windows.RtlAreLongPathsEnabled', res)

    try:
        import _winapi
        dll_path = _winapi.GetModuleFileName(sys.dllhandle)
        info_add('windows.dll_path', dll_path)
    except (ImportError, AttributeError):
        pass


def collect_info(info):
    error = False
    info_add = info.add

    for collect_func in (
        # collect_urandom() must be the first, to check the getrandom() status.
        # Other functions may block on os.urandom() indirectly and so change
        # its state.
        collect_urandom,

        collect_builtins,
        collect_cc,
        collect_datetime,
        collect_decimal,
        collect_expat,
        collect_gdb,
        collect_gdbm,
        collect_get_config,
        collect_locale,
        collect_os,
        collect_platform,
        collect_pwd,
        collect_readline,
        collect_resource,
        collect_socket,
        collect_sqlite,
        collect_ssl,
        collect_subprocess,
        collect_sys,
        collect_sysconfig,
        collect_testcapi,
        collect_time,
        collect_tkinter,
        collect_windows,
        collect_zlib,

        # Collecting from tests should be last as they have side effects.
        collect_test_socket,
        collect_test_support,
    ):
        try:
            collect_func(info_add)
        except Exception as exc:
            error = True
            print("ERROR: %s() failed" % (collect_func.__name__),
                  file=sys.stderr)
            traceback.print_exc(file=sys.stderr)
            print(file=sys.stderr)
            sys.stderr.flush()

    return error


def dump_info(info, file=None):
    title = "Python debug information"
    print(title)
    print("=" * len(title))
    print()

    infos = info.get_infos()
    infos = sorted(infos.items())
    for key, value in infos:
        value = value.replace("\n", " ")
        print("%s: %s" % (key, value))
    print()


def main():
    info = PythonInfo()
    error = collect_info(info)
    dump_info(info)

    if error:
        print("Collection failed: exit with error", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
