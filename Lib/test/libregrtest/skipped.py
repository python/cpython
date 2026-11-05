"""
Ruleset to check if a test is expected to be skipped.
"""

import importlib
import os
import sys
from test import support


# On a WASI CI, the main regrtest process is run on the build Python and test
# worker processes are run on the host Python. Detect WASI using
# _PYTHON_HOST_PLATFORM environment variable. Below, IS_WASI should be checked
# explicitly since test.support constants (such as support.has_socket_support)
# are set by the build Python which is different.
HOST_PLATFORM = os.environ.get('_PYTHON_HOST_PLATFORM', '')
IS_WASI = support.is_wasi or HOST_PLATFORM.startswith('wasi')


NEED_MULTIPROCESSING = {
    'test_multiprocessing_fork',
    'test_multiprocessing_forkserver',
    'test_multiprocessing_main_handling',
    'test_multiprocessing_spawn',
}
SELECT_TESTS = {
    'test_devpoll',
    'test_epoll',
    'test_kqueue',
    'test_poll',
}
NEED_SSL = {
    'test_smtpnet',
    'test_ssl',
}
NEED_TESTMULTIPHASE = {
    # Lib/test/test_importlib/util.py requires _testmultiphase
    'test_compileall',
    'test_import',
    'test_pkgutil',
    'test_pyclbr',
    'test_runpy',
}


# List of tests expected to be skipped
EXPECTED_SKIP = set()
if support.MS_WINDOWS:
    EXPECTED_SKIP |= {
        # UNIX only
        'test_asyncio.test_unix_events',
        # test_gdb doesn't work on Windows
        'test_gdb',
        # Can't test signal on Windows
        'test_threadsignals',
    }
else:
    EXPECTED_SKIP |= {
        # Tests specific to Windows
        'test_asyncio.test_windows_events',
        'test_asyncio.test_windows_utils',
        'test_msvcrt',
        'test_os.test_windows',
        'test_winapi',
        'test_winconsoleio',
        'test_winreg',
        'test_winsound',
        'test_wmi',
    }

if sys.platform in ("win32", 'darwin'):
    EXPECTED_SKIP.add('test_multiprocessing_fork')
if sys.platform in ("win32", "cygwin"):
    EXPECTED_SKIP.add('test_multiprocessing_forkserver')

if support.Py_GIL_DISABLED:
    EXPECTED_SKIP.add('test_interpreters')
else:
    EXPECTED_SKIP.add('test_free_threading')

if not hasattr(os, 'startfile'):
    EXPECTED_SKIP.add('test_startfile')
if not hasattr(os, "openpty"):
    EXPECTED_SKIP.add('test_openpty')
if not hasattr(os, "wait3"):
    EXPECTED_SKIP.add('test_wait3')
if not hasattr(os, "wait4"):
    EXPECTED_SKIP.add('test_wait4')

if not support.is_android:
    EXPECTED_SKIP.add('test_android')
if not support.is_apple:
    EXPECTED_SKIP.add('test_apple')

if not support.has_socket_support or IS_WASI:
    # Need socket support
    EXPECTED_SKIP |= {
        'test_asyncgen',
        'test_asyncio',
        'test_docxmlrpc',
        'test_ftplib',
        'test_httplib',
        'test_httpservers',
        'test_imaplib',
        'test_mailbox',
        'test_poplib',
        'test_select',
        'test_selectors',
        'test_smtplib',
        'test_socket',
        'test_urllib',
        'test_urllib2',
        'test_urllib2_localnet',
        'test_urllib_response',
        'test_wsgiref',
        'test_xml_dom_xmlbuilder',
        'test_xmlrpc',
    }
    EXPECTED_SKIP |= SELECT_TESTS
if not support.has_subprocess_support or IS_WASI:
    # Need subprocess support
    EXPECTED_SKIP |= {
        'test_argparse',
        'test_c_stack_unwind',
        'test_cext',
        'test_clinic',
        'test_cmd_line',
        'test_embed',
        'test_faulthandler',
        'test_file_eintr',
        'test_gdb',
        'test_generated_cases',
        'test_getopt',
        'test_optparse',
        'test_perf_profiler',
        'test_regrtest',
        'test_repl',
        'test_samply_profiler',
        'test_subprocess',
        'test_tools',
    }
if support.is_emscripten:
    EXPECTED_SKIP |= {
        # Incompatible with Emscripten's mmap emulation
        'test_mmap',
        # Cannot create socketpair on Emscripten/WASI
        'test_selectors',
    }
if support.is_android or support.is_apple_mobile or support.is_wasm32 or IS_WASI:
    EXPECTED_SKIP |= {
        # pty is not available on these platforms
        'test_pty',
        # venv is not available on these platforms
        'test_venv',
    }
# On FreeBSD, is_remote_debug_enabled() is always false
if not sys.is_remote_debug_enabled():
    EXPECTED_SKIP.add('test_remote_pdb')
if not support.has_fork_support or IS_WASI:
    EXPECTED_SKIP |= {
        'test_fork1',
        'test_multiprocessing_forkserver',
        'test_wait3',
        'test_wait4',
    }
if IS_WASI:
    EXPECTED_SKIP |= {
        # configure disables multiple extension modules on WASI
        'test__interpchannels',        # need _interpchannels
        'test__interpreters',          # need _interpreters
        'test_crossinterp',            # need _interpreters
        'test_curses',                 # need _curses
        'test_external_inspection',    # need _remote_debugging
        'test_fcntl',                  # need fcntl
        'test_gc_stats',               # need _remote_debugging
        'test_grp',                    # need grp
        'test_ioctl',                  # need fcntl
        'test_mmap',                   # need mmap
        'test_pwd',                    # need pwd
        'test_pyrepl',                 # need termios
        'test_remote_pdb',             # need _remote_debugging
        'test_resource',               # need resource
        'test_syslog',                 # need syslog
        'test_termios',                # need termios
        'test_tty',                    # need termios
        'test_xxlimited',              # need xxlimited

        # Missing functions on WASI
        'test_openpty',                # need os.openpty()

        # Missing dependencies on WASI
        'test_bz2',                    # need _bz2
        'test_ctypes',                 # need _ctypes
        'test_dbm_sqlite3',            # need _sqlite3
        'test_gzip',                   # need zlib
        'test_lzma',                   # need _lzma
        'test_sqlite3',                # need _sqlite3
        'test_stable_abi_ctypes',      # need _ctypes
        'test_zlib',                   # need zlib
        'test_zstd',                   # need _zstd
    }
    # configure disables _multiprocessing and _testmultiphase
    # extension modules on WASI
    EXPECTED_SKIP |= NEED_TESTMULTIPHASE
    EXPECTED_SKIP |= NEED_MULTIPROCESSING
    # Missing OpenSSL dependency on WASI
    EXPECTED_SKIP |= NEED_SSL


# Modules needed by a test.
# Used to check if a test is expected to be skipped.
# NEED_MODULES doesn't work on WASI, since the main regrtest process is run
# on the build Python which is different than the host (WASI) Python.
NEED_MODULES = {
    'test__interpchannels': '_interpchannels',
    'test__interpreters': '_interpreters',
    'test_crossinterp': '_interpreters',
    'test_curses': ('_curses', 'curses', 'curses.ascii', 'curses.textpad'),
    'test_dbm_gnu': 'dbm.gnu',
    'test_dbm_ndbm': 'dbm.ndbm',
    'test_fcntl': 'fcntl',
    'test_free_threading.test_dbm_gnu': 'dbm.gnu',
    'test_free_threading.test_grp': 'grp',
    'test_free_threading.test_pwd': 'pwd',
    'test_free_threading.test_readline': 'readline',
    'test_free_threading.test_resource': 'resource',
    'test_free_threading.test_syslog': 'syslog',
    'test_grp': 'grp',
    'test_ioctl': ('fcntl', 'termios'),
    'test_lzma': '_lzma',
    'test_pty': 'termios',
    'test_pwd': 'pwd',
    'test_readline': 'readline',
    'test_resource': 'resource',
    'test_syslog': 'syslog',
    'test_termios': 'termios',
    'test_tty': ('termios', 'tty'),
    'test_xxlimited': 'xxlimited',
    'test_xxtestfuzz': '_xxtestfuzz',
    'test_zstd': '_zstd',
}
for name in NEED_MULTIPROCESSING:
    NEED_MODULES[name] = '_multiprocessing'
for name in (
    'test_idle',
    'test_tcl',
    'test_tkinter',
    'test_ttk',
    'test_ttk_textonly',
    'test_turtle',
):
    NEED_MODULES[name] = '_tkinter'
for name in NEED_SSL:
    NEED_MODULES[name] = '_ssl'


NEED_WORKING_THREADING = {
    'test_concurrent_futures',
    'test_queue',
    'test_thread',
    'test_threadedtempfile',
    'test_threading',
    'test_threading_local',
    'test_webbrowser',
}


def miss_modules(modules):
    if isinstance(modules, str):
        modules = (modules,)
    try:
        for module in modules:
            importlib.import_module(module)
    except ImportError:
        return True
    return False


def expected_to_be_skipped(test_name):
    # Test EXPECTED_SKIP
    test_name = test_name.removeprefix('test.')
    if test_name in EXPECTED_SKIP:
        return True
    package = test_name.split('.')[0]
    if package in EXPECTED_SKIP:
        return True

    # Test NEED_MODULES
    if test_name in NEED_MODULES:
        if miss_modules(NEED_MODULES[test_name]):
            return True
    elif package in NEED_MODULES:
        if miss_modules(NEED_MODULES[package]):
            return True

    # Custom ruleset
    if (test_name in NEED_WORKING_THREADING
        or package in NEED_WORKING_THREADING):
        # Detect whether Python can start new threads
        # - wasm32-wasi does not have threading.
        # - wasm32-emscripten can be compiled with or without pthread
        #   support (-s USE_PTHREADS / __EMSCRIPTEN_PTHREADS__).
        if IS_WASI:
            return True
        from test.support import threading_helper
        return not threading_helper.can_start_thread

    if test_name in SELECT_TESTS:
        # For example, test_epoll needs select.epoll()
        import select
        name = test_name.removeprefix('test_')
        return not hasattr(select, name)

    if test_name == 'test_dtrace':
        # Need ./configure --with-dtrace
        import sysconfig
        return not sysconfig.get_config_var('WITH_DTRACE')

    if test_name in ('test_perf_profiler', 'test_perfmaps', 'test_samply_profiler'):
        # Need perf trampoline profiling: configure defines the
        # PY_HAVE_PERF_TRAMPOLINE macro on Linux (x86-64 and AArch64)
        # and on macOS.
        return not support.supports_perf_trampoline_profiling()

    if package == 'test_gdb':
        # Need gdb program
        import shutil
        return not shutil.which('gdb')

    if test_name == 'test_asdl_parser':
        # Test only relevant for from-source Python builds
        import sysconfig
        return not sysconfig.is_python_build()

    return False
