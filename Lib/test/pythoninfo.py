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

        if value is None:
            return

        if not isinstance(value, (int, long)):
            if not isinstance(value, basestring):
                # convert other objects like sys.flags to string
                value = str(value)

            value = value.strip()
            if not value:
                return

        self.info[key] = value

    def get_infos(self):
        """
        Get informations as a key:value dictionary where values are strings.
        """
        return {key: str(value) for key, value in self.info.items()}


def copy_attributes(info_add, obj, name_fmt, attributes, formatter=None):
    for attr in attributes:
        value = getattr(obj, attr, None)
        if value is None:
            continue
        name = name_fmt % attr
        if formatter is not None:
            value = formatter(attr, value)
        info_add(name, value)


def call_func(info_add, name, mod, func_name, formatter=None):
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
        'api_version',
        'builtin_module_names',
        'byteorder',
        'dont_write_bytecode',
        'executable',
        'flags',
        'float_info',
        'float_repr_style',
        'hexversion',
        'maxint',
        'maxsize',
        'maxunicode',
        'path',
        'platform',
        'prefix',
        'version',
        'version_info',
        'winver',
    )
    copy_attributes(info_add, sys, 'sys.%s', attributes)

    encoding = sys.getfilesystemencoding()
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

    info_add('locale.encoding', locale.getpreferredencoding(True))


def collect_os(info_add):
    import os

    attributes = ('name',)
    copy_attributes(info_add, os, 'os.%s', attributes)

    info_add("os.cwd", os.getcwd())

    call_func(info_add, 'os.uid', os, 'getuid')
    call_func(info_add, 'os.gid', os, 'getgid')
    call_func(info_add, 'os.uname', os, 'uname')

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

    call_func(info_add, 'os.loadavg', os, 'getloadavg')

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

    if hasattr(os, 'umask'):
        mask = os.umask(0)
        os.umask(mask)
        info_add("os.umask", '%03o' % mask)

    try:
        cpu_count = os.sysconf('SC_NPROCESSORS_ONLN')
    except (AttributeError, ValueError):
        pass
    else:
        if cpu_count:
            info_add('os.sysconf(SC_NPROCESSORS_ONLN)', cpu_count)


def collect_readline(info_add):
    try:
        import readline
    except ImportError:
        return

    def format_attr(attr, value):
        if isinstance(value, (int, long)):
            return "%#x" % value
        else:
            return value

    attributes = (
        "_READLINE_VERSION",
        "_READLINE_RUNTIME_VERSION",
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
        import Tkinter
    except ImportError:
        pass
    else:
        tcl = Tkinter.Tcl()
        patchlevel = tcl.call('info', 'patchlevel')
        info_add('tkinter.info_patchlevel', patchlevel)


def collect_time(info_add):
    import time

    attributes = (
        'altzone',
        'daylight',
        'timezone',
        'tzname',
    )
    copy_attributes(info_add, time, 'time.%s', attributes)


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


def collect_multiprocessing(info_add):
    try:
        import multiprocessing
    except ImportError:
        return

    cpu_count = multiprocessing.cpu_count()
    if cpu_count:
        info_add('multiprocessing.cpu_count', cpu_count)


def collect_info(info):
    error = False
    info_add = info.add

    for collect_func in (
        collect_expat,
        collect_gdb,
        collect_locale,
        collect_os,
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
        collect_multiprocessing,
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
