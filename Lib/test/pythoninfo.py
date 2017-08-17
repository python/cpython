"""
Collect various informations about Python to help debugging test failures.
"""
from __future__ import print_function
import re
import sys
import traceback


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

        if isinstance(value, str):
            value = value.strip()
            if not value:
                return
        elif value is None:
            return
        elif not isinstance(value, int):
            raise TypeError("value type must be str, int or None")

        self.info[key] = value

    def get_infos(self):
        """
        Get informations as a key:value dictionary where values are strings.
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


def collect_sys(info_add):
    def format_attr(attr, value):
        if attr == 'flags':
            # convert sys.flags tuple to string
            return str(value)
        else:
            return value

    attributes = (
        '_framework',
        'byteorder',
        'executable',
        'flags',
        'maxsize',
        'maxunicode',
        'version',
    )
    copy_attributes(info_add, sys, 'sys.%s', attributes,
                    formatter=format_attr)

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

    if hasattr(sys, 'hash_info'):
        alg = sys.hash_info.algorithm
        bits = 64 if sys.maxsize > 2**32 else 32
        alg = '%s (%s bits)' % (alg, bits)
        info_add('sys.hash_info', alg)

    if hasattr(sys, 'getandroidapilevel'):
        info_add('sys.androidapilevel', sys.getandroidapilevel())


def collect_platform(info_add):
    import platform

    arch = platform.architecture()
    arch = ' '.join(filter(bool, arch))
    info_add('platform.architecture', arch)

    info_add('platform.python_implementation',
             platform.python_implementation())
    info_add('platform.platform',
             platform.platform(aliased=True))


def collect_locale(info_add):
    import locale

    info_add('locale.encoding', locale.getpreferredencoding(False))


def collect_os(info_add):
    import os

    if hasattr(os, 'getrandom'):
        # PEP 524: Check is system urandom is initialized
        try:
            os.getrandom(1, os.GRND_NONBLOCK)
            state = 'ready (initialized)'
        except BlockingIOError as exc:
            state = 'not seeded yet (%s)' % exc
        info_add('os.getrandom', state)

    info_add("os.cwd", os.getcwd())

    if hasattr(os, 'getuid'):
        info_add("os.uid", os.getuid())
        info_add("os.gid", os.getgid())

    if hasattr(os, 'getgroups'):
        groups = os.getgroups()
        groups = map(str, groups)
        groups = ', '.join(groups)
        info_add("os.groups", groups)

    if hasattr(os, 'getlogin'):
        try:
            login = os.getlogin()
        except OSError:
            # getlogin() fails with "OSError: [Errno 25] Inappropriate ioctl
            # for device" on Travis CI
            pass
        else:
            info_add("os.login", login)

    if hasattr(os, 'cpu_count'):
        cpu_count = os.cpu_count()
        if cpu_count:
            info_add('os.cpu_count', cpu_count)

    if hasattr(os, 'getloadavg'):
        load = os.getloadavg()
        info_add('os.loadavg', str(load))

    # Get environment variables: filter to list
    # to not leak sensitive information
    ENV_VARS = (
        "CC",
        "COMSPEC",
        "DISPLAY",
        "DISTUTILS_USE_SDK",
        "DYLD_LIBRARY_PATH",
        "HOME",
        "HOMEDRIVE",
        "HOMEPATH",
        "LANG",
        "LD_LIBRARY_PATH",
        "MACOSX_DEPLOYMENT_TARGET",
        "MAKEFLAGS",
        "MSSDK",
        "PATH",
        "SDK_TOOLS_BIN",
        "SHELL",
        "TEMP",
        "TERM",
        "TMP",
        "TMPDIR",
        "USERPROFILE",
        "WAYLAND_DISPLAY",
    )
    for name, value in os.environ.items():
        uname = name.upper()
        if (uname in ENV_VARS or uname.startswith(("PYTHON", "LC_"))
           # Visual Studio: VS140COMNTOOLS
           or (uname.startswith("VS") and uname.endswith("COMNTOOLS"))):
            info_add('os.environ[%s]' % name, value)


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

    if not hasattr(time, 'get_clock_info'):
        return

    for clock in ('time', 'perf_counter'):
        tinfo = time.get_clock_info(clock)
        info_add('time.%s' % clock, str(tinfo))


def collect_sysconfig(info_add):
    import sysconfig

    for name in (
        'ABIFLAGS',
        'ANDROID_API_LEVEL',
        'CC',
        'CCSHARED',
        'CFLAGS',
        'CFLAGSFORSHARED',
        'PY_LDFLAGS',
        'CONFIG_ARGS',
        'HOST_GNU_TYPE',
        'MACHDEP',
        'MULTIARCH',
        'OPT',
        'PY_CFLAGS',
        'PY_CFLAGS_NODIST',
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
            # Convert OPENSSL_VERSION_INFO tuple to str
            return str(value)

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


def collect_info(info):
    error = False
    info_add = info.add

    for collect_func in (
        # collect_os() should be the first, to check the getrandom() status
        collect_os,

        collect_gdb,
        collect_locale,
        collect_platform,
        collect_readline,
        collect_socket,
        collect_sqlite,
        collect_ssl,
        collect_sys,
        collect_sysconfig,
        collect_time,
        collect_tkinter,
        collect_zlib,
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
