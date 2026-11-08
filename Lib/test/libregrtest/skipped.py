"""
Ruleset to check if a test is expected to be skipped.
Tests are mostly filtered using sys.platform, except a few exceptions.
"""

import os
import sys
import sysconfig
from test import support
from .utils import cleanup_test_name


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
NEED_TESTMULTIPHASE = {
    # Lib/test/test_importlib/util.py requires _testmultiphase
    'test_compileall',
    'test_import',
    'test_pkgutil',
    'test_pyclbr',
    'test_runpy',
}
NEED_WORKING_THREADING = {
    'test_concurrent_futures',
    'test_queue',
    'test_thread',
    'test_threadedtempfile',
    'test_threading',
    'test_threading_local',
    'test_webbrowser',
}
NEED_TKINTER = {
    'test_idle',
    'test_tcl',
    'test_tkinter',
    'test_ttk',
    'test_ttk_textonly',
    'test_turtle',
}


# List of tests expected to be skipped on sys.platform
EXPECTED_SKIP = set()
if support.Py_GIL_DISABLED:
    # Need GIL build
    EXPECTED_SKIP.add('test_interpreters')
else:
    # Need Free Threading build
    EXPECTED_SKIP |= {
        'test_free_threading',
        'test_io.test_free_threading',
    }

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
        'test_asyncio.test_subprocess',
        'test_c_stack_unwind',
        'test_cext',
        'test_clinic',
        'test_cmd_line',
        'test_embed',
        'test_faulthandler',
        'test_gdb',
        'test_generated_cases',
        'test_getopt',
        'test_io.test_file_eintr',
        'test_optparse',
        'test_perf_profiler',
        'test_regrtest',
        'test_repl',
        'test_samply_profiler',
        'test_subprocess',
        'test_tools',
    }
if not support.has_fork_support or IS_WASI:
    # Need fork
    EXPECTED_SKIP |= {
        'test_fork1',
        'test_multiprocessing_forkserver',
        'test_wait3',
        'test_wait4',
    }
if not(sys.platform in ("linux", "darwin") or support.MS_WINDOWS):
    # Need Py_SUPPORTS_REMOTE_DEBUG macro
    EXPECTED_SKIP.add('test_remote_pdb')
if not sysconfig.is_python_build():
    # Skip tests on installed Python.
    # Tests only relevant for from-source Python builds
    EXPECTED_SKIP |= {
        # Parser/asdl.py is not installed
        'test_asdl_parser',
        # Tools/clinic/ is not installed
        'test_clinic',
        # python-gdb.py is not installed
        'test_gdb',
        # Tools/cases_generator is not installed
        'test_generated_cases',
    }
if not sysconfig.get_config_var('WITH_DTRACE'):
    # Need ./configure --with-dtrace
    EXPECTED_SKIP.add('test_dtrace')


if support.MS_WINDOWS:
    # Tests specific to non-Windows platforms
    EXPECTED_SKIP |= {
        'test_asyncio.test_unix_events',  # UNIX only
        'test_dbm_gnu',        # Need dbm.gnu module
        'test_dbm_ndbm',       # Need dbm.ndbm module
        'test_fcntl',          # Need fcntl module
        'test_free_threading.test_dbm_gnu',   # Need dbm.gnu module
        'test_free_threading.test_grp',       # Need grp module
        'test_free_threading.test_pwd',       # Need pwd module
        'test_free_threading.test_readline',  # Need readline module
        'test_free_threading.test_resource',  # Need resource module
        'test_free_threading.test_syslog',    # Need syslog module
        'test_gdb',            # test_gdb doesn't work on Windows
        'test_grp',            # Need grp module
        'test_ioctl',          # Need fcntl module
        'test_openpty',        # Need os.openpty()
        'test_pty',            # Need termios module
        'test_pwd',            # Need pwd module
        'test_readline',       # Need readline module
        'test_resource',       # Need resource module
        'test_syslog',         # Need syslog module
        'test_termios',        # Need termios module
        'test_threadsignals',  # Can't test signal on Windows
        'test_tty',            # Need termios and tty modules
        'test_wait3',          # Need os.wait3()
        'test_wait4',          # Need os.wait4()
    }
else:
    EXPECTED_SKIP |= {
        # Tests specific to Windows
        'test_asyncio.test_windows_events',
        'test_asyncio.test_windows_utils',
        'test_msvcrt',
        'test_os.test_windows',
        'test_io.test_winconsoleio',
        'test_startfile',      # Need os.startfile()
        'test_winapi',
        'test_winreg',
        'test_winsound',
        'test_wmi',
    }
if sys.platform in ("win32", 'darwin'):
    EXPECTED_SKIP.add('test_multiprocessing_fork')
if sys.platform in ("win32", "cygwin"):
    EXPECTED_SKIP.add('test_multiprocessing_forkserver')

if not support.is_android:
    # Test specific to Android
    EXPECTED_SKIP.add('test_android')


if not support.is_apple:
    # Test specific to Apple
    EXPECTED_SKIP.add('test_apple')


if sys.platform == "darwin":
    # On macOS, running gdb fails badly:
    # https://github.com/python/cpython/pull/161520
    EXPECTED_SKIP.add('test_gdb')


if support.is_apple_mobile:
    # configure disables multiple extension modules on Apple mobile
    EXPECTED_SKIP |= {
        'test_curses',                 # need _curses
        'test_dbm_gnu',                # need _gdbm
        'test_grp',                    # need grp
        'test_pwd',                    # need pwd
        'test_readline',               # need readline
        'test_syslog',                 # need syslog
    }
    # configure disables _multiprocessing on Apple mobile
    EXPECTED_SKIP |= NEED_MULTIPROCESSING
    # configure disables _tkinter on Apple mobile
    EXPECTED_SKIP |= NEED_TKINTER


if support.is_emscripten:
    # configure disables multiple extension modules on Emscripten
    EXPECTED_SKIP |= {
        'test__interpchannels',        # need _interpchannels
        'test__interpreters',          # need _interpreters
        'test_crossinterp',            # need _interpreters
        'test_curses',                 # need _curses
        'test_dbm_gnu',                # need _gdbm
        'test_dbm_ndbm',               # need _dbm
        'test_grp',                    # need grp
        'test_pwd',                    # need pwd
        'test_readline',               # need readline
        'test_resource',               # need resource
        'test_syslog',                 # need syslog
    }
    # configure disables _multiprocessing on Emscripten
    EXPECTED_SKIP |= NEED_MULTIPROCESSING

    # Missing os.openpty() function on Emscripten
    EXPECTED_SKIP.add('test_openpty')

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


if IS_WASI:
    # configure disables multiple extension modules on WASI
    EXPECTED_SKIP |= {
        'test__interpchannels',        # need _interpchannels
        'test__interpreters',          # need _interpreters
        'test_crossinterp',            # need _interpreters
        'test_curses',                 # need _curses
        'test_dbm_gnu',                # need _gdbm
        'test_dbm_ndbm',               # need _dbm
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
    }
    # configure disables _multiprocessing and _testmultiphase
    # extension modules on WASI
    EXPECTED_SKIP |= NEED_MULTIPROCESSING
    EXPECTED_SKIP |= NEED_TESTMULTIPHASE

    # Missing os.openpty() function on WASI
    EXPECTED_SKIP.add('test_openpty')


def expected_to_be_skipped(test_name):
    # Test EXPECTED_SKIP
    test_name = cleanup_test_name(test_name)
    if test_name in EXPECTED_SKIP:
        return True
    package = test_name.split('.')[0]
    if package in EXPECTED_SKIP:
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

    if test_name in ('test_perf_profiler', 'test_perfmaps', 'test_samply_profiler'):
        # Need perf trampoline profiling: configure defines the
        # PY_HAVE_PERF_TRAMPOLINE macro on Linux (x86-64 and AArch64)
        # and on macOS.
        return not support.supports_perf_trampoline_profiling()

    return False
