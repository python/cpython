"""
Ruleset to check if a test is expected to be skipped.
"""

import importlib
import os
import sys
from test import support


# List of tests expected to be skipped
EXPECTED_SKIP = []
if support.MS_WINDOWS:
    EXPECTED_SKIP.extend((
        'test_asyncio.test_unix_events',
        'test_gdb',
        'test_threadsignals',
    ))
else:
    EXPECTED_SKIP.extend((
        'test_asyncio.test_windows_events',
        'test_asyncio.test_windows_utils',
        'test_msvcrt',
        'test_os.test_windows',
        'test_winapi',
        'test_winconsoleio',
        'test_winreg',
        'test_winsound',
        'test_wmi',
    ))
if not hasattr(os, 'startfile'):
    EXPECTED_SKIP.append('test_startfile')
if not hasattr(os, "openpty"):
    EXPECTED_SKIP.append('test_openpty')
if not hasattr(os, "wait3"):
    EXPECTED_SKIP.append('test_wait3')
if not hasattr(os, "wait4"):
    EXPECTED_SKIP.append('test_wait4')
if not support.is_android:
    EXPECTED_SKIP.append('test_android')
if not support.is_apple:
    EXPECTED_SKIP.append('test_apple')
if not support.Py_GIL_DISABLED:
    EXPECTED_SKIP.append('test_free_threading')
if sys.platform in ("win32", 'darwin'):
    EXPECTED_SKIP.append('test_multiprocessing_fork')
if sys.platform in ("win32", "cygwin"):
    EXPECTED_SKIP.append('test_multiprocessing_forkserver')
if not support.has_socket_support:
    EXPECTED_SKIP.extend((
        'test_asyncio',
        'test_docxmlrpc',
        'test_ftplib',
        'test_httplib',
        'test_httpservers',
        'test_imaplib',
        'test_poplib',
        'test_select',
        'test_smtplib',
        'test_socket',
        'test_urllib2',
        'test_urllib2_localnet',
        'test_wsgiref',
        'test_xml_dom_xmlbuilder',
        'test_xmlrpc',
    ))
if not support.has_subprocess_support:
    EXPECTED_SKIP.extend((
        'test_argparse',
        'test_cext',
        'test_clinic',
        'test_cmd_line',
        'test_embed',
        'test_faulthandler',
        'test_file_eintr',
        'test_generated_cases',
        'test_getopt',
        'test_optparse',
        'test_perf_profiler',
        'test_regrtest',
        'test_repl',
        'test_samply_profiler',
        'test_subprocess',
        'test_tools',
    ))
if support.is_emscripten:
    EXPECTED_SKIP.extend((
        'test_mmap',
        'test_selectors',
    ))
if support.is_android or support.is_apple_mobile or support.is_wasm32:
    EXPECTED_SKIP.append('test_venv')
if not sys.is_remote_debug_enabled():
    EXPECTED_SKIP.append('test_remote_pdb')


# Modules needed by a test.
# Used to check if a test is expected to be skipped.
NEED_MODULES = {
    'test__interpchannels': '_interpchannels',
    'test_crossinterp': '_interpreters',
    'test_curses': ('_curses', 'curses', 'curses.ascii', 'curses.textpad'),
    'test_dbm_gnu': 'dbm.gnu',
    'test_dbm_ndbm': 'dbm.ndbm',
    'test_fcntl': 'fcntl',
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
    'test_xxtestfuzz': '_xxtestfuzz',
    'test_zstd': '_zstd',
}
for name in (
    'test_multiprocessing_fork',
    'test_multiprocessing_forkserver',
    'test_multiprocessing_main_handling',
    'test_multiprocessing_spawn',
):
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
for name in (
    'test_smtpnet',
    'test_ssl',
):
    NEED_MODULES[name] = '_ssl'


NEED_WORKING_THREADING = (
    'test_concurrent_futures',
    'test_queue',
    'test_thread',
    'test_threadedtempfile',
    'test_threading',
    'test_threading_local',
    'test_webbrowser',
)


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
    test_name = test_name.removeprefix('test.')
    if test_name in EXPECTED_SKIP:
        return True

    package = test_name.split('.')[0]
    if package in EXPECTED_SKIP:
        return True

    if test_name in ('test_poll', 'test_epoll',
                     'test_kqueue', 'test_devpoll'):
        import select
        name = test_name.removeprefix('test_')
        return not hasattr(select, name)

    if test_name in NEED_MODULES:
        if miss_modules(NEED_MODULES[test_name]):
            return True
    elif package in NEED_MODULES:
        if miss_modules(NEED_MODULES[package]):
            return True

    if test_name == 'test_fork1':
        return not support.has_fork_support
    if test_name == 'test_dtrace':
        import sysconfig
        return not sysconfig.get_config_var('WITH_DTRACE')
    if (test_name in NEED_WORKING_THREADING
        or package in NEED_WORKING_THREADING):
        from test.support import threading_helper
        return not threading_helper.can_start_thread

    return False
