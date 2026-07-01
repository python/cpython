"""Tests for sys.audit and sys.addaudithook

``sys.addaudithook()`` cannot be undone, so each test needs a fresh
interpreter.  ``@isolation.isolated()`` provides one and reports the result, which
lets these be ordinary ``TestCase`` methods using ``self.assert*`` directly in
the subprocess -- no separate driver script, and no parsing of events out of the
subprocess's stdout.
"""

import os
import sys
import unittest
from test import support
from test.support import isolation
from unittest.mock import ANY


if not hasattr(sys, "addaudithook") or not hasattr(sys, "audit"):
    raise unittest.SkipTest("test only relevant when sys.audit is available")


class _Hook:
    """Collect audited events; for use inside @isolated() tests.

    Should be used in a with block to ensure that it has no impact after the
    test completes (although that matters less here, since each test runs in its
    own interpreter).
    """

    def __init__(self, raise_on_events=None, exc_type=RuntimeError):
        self.raise_on_events = raise_on_events or ()
        self.exc_type = exc_type
        self.seen = []
        self.closed = False

    def __enter__(self):
        sys.addaudithook(self)
        return self

    def __exit__(self, *a):
        self.closed = True

    @property
    def seen_events(self):
        return [ev for ev, args in self.seen]

    def __call__(self, event, args):
        if self.closed:
            return
        self.seen.append((event, args))
        if event in self.raise_on_events:
            raise self.exc_type("saw event " + event)


def requires_module(name):
    """Skip the test, in the parent process, if module *name* is unavailable.

    Checking here means a missing module skips the test without first spawning
    the @isolated() subprocess.
    """
    import functools

    def decorator(func):
        @functools.wraps(func)
        def wrapper(self):
            from test.support import import_helper
            import_helper.import_module(name)
            return func(self)
        return wrapper
    return decorator


class AuditTest(unittest.TestCase):
    maxDiff = None

    @isolation.isolated()
    def test_basic(self):
        with _Hook() as hook:
            sys.audit("test_event", 1, 2, 3)
        self.assertEqual(hook.seen[0][0], "test_event")
        self.assertEqual(hook.seen[0][1], (1, 2, 3))

    @isolation.isolated()
    def test_block_add_hook(self):
        # Raising in a hook for "sys.addaudithook" blocks the new hook but does
        # not propagate out.
        with _Hook(raise_on_events="sys.addaudithook") as hook1:
            with _Hook() as hook2:
                sys.audit("test_event")
        self.assertIn("test_event", hook1.seen_events)
        self.assertNotIn("test_event", hook2.seen_events)

    @isolation.isolated()
    def test_block_add_hook_baseexception(self):
        # A BaseException raised while adding a hook propagates out.
        with self.assertRaises(BaseException):
            with _Hook(raise_on_events="sys.addaudithook",
                       exc_type=BaseException):
                with _Hook():
                    pass

    @isolation.isolated()
    def test_marshal(self):
        import marshal
        from test.support import os_helper

        o = ("a", "b", "c", 1, 2, 3)
        payload = marshal.dumps(o)

        with _Hook() as hook:
            self.assertEqual(o, marshal.loads(marshal.dumps(o)))

            try:
                with open(os_helper.TESTFN, "wb") as f:
                    marshal.dump(o, f)
                with open(os_helper.TESTFN, "rb") as f:
                    self.assertEqual(o, marshal.load(f))
            finally:
                os_helper.unlink(os_helper.TESTFN)

        actual = [(a[0], a[1]) for e, a in hook.seen if e == "marshal.dumps"]
        self.assertEqual(actual, [(o, marshal.version)] * 2)

        actual = [a[0] for e, a in hook.seen if e == "marshal.loads"]
        self.assertEqual(actual, [payload])

        actual = [e for e, a in hook.seen if e == "marshal.load"]
        self.assertEqual(actual, ["marshal.load"])

    @isolation.isolated()
    def test_pickle(self):
        import pickle

        class PicklePrint:
            def __reduce_ex__(self, p):
                return str, ("Pwned!",)

        payload_1 = pickle.dumps(PicklePrint())
        payload_2 = pickle.dumps(("a", "b", "c", 1, 2, 3))

        # Before we add the hook, ensure our malicious pickle loads
        self.assertEqual("Pwned!", pickle.loads(payload_1))

        with _Hook(raise_on_events="pickle.find_class"):
            with self.assertRaises(RuntimeError):
                # With the hook enabled, loading globals is not allowed
                pickle.loads(payload_1)
            # pickles with no globals are okay
            pickle.loads(payload_2)

    @isolation.isolated()
    def test_monkeypatch(self):
        class A:
            pass

        class B:
            pass

        class C(A):
            pass

        a = A()

        with _Hook() as hook:
            # Catch name changes
            C.__name__ = "X"
            # Catch type changes
            C.__bases__ = (B,)
            # Ensure bypassing __setattr__ is still caught
            type.__dict__["__bases__"].__set__(C, (B,))
            # Catch attribute replacement
            C.__init__ = B.__init__
            # Catch attribute addition
            C.new_attr = 123
            # Catch class changes
            a.__class__ = B

        actual = [(ar[0], ar[1]) for e, ar in hook.seen
                  if e == "object.__setattr__"]
        self.assertEqual(
            [(C, "__name__"), (C, "__bases__"), (C, "__bases__"),
             (a, "__class__")],
            actual,
        )

    @isolation.isolated()
    def test_open(self):
        from test.support import os_helper

        testfn = os_helper.TESTFN

        # SSLContext.load_dh_params uses Py_fopen() rather than normal open()
        try:
            import ssl

            load_dh_params = ssl.create_default_context().load_dh_params
        except ImportError:
            load_dh_params = None

        try:
            import readline
        except ImportError:
            readline = None

        def rl(name):
            if readline:
                return getattr(readline, name, None)
            else:
                return None

        try:
            import _remote_debugging
        except ImportError:
            _remote_debugging = None

        def rd(name):
            if _remote_debugging:
                return getattr(_remote_debugging, name, None)
            return None

        # Try a range of "open" functions.
        # All of them should fail
        with _Hook(raise_on_events={"open"}) as hook:
            for fn, *args in [
                (open, testfn, "r"),
                (open, sys.executable, "rb"),
                (open, 3, "wb"),
                (open, testfn, "w", -1, None, None, None, False, lambda *a: 1),
                (load_dh_params, testfn),
                (rl("read_history_file"), testfn),
                (rl("read_history_file"), None),
                (rl("write_history_file"), testfn),
                (rl("write_history_file"), None),
                (rl("append_history_file"), 0, testfn),
                (rl("append_history_file"), 0, None),
                (rl("read_init_file"), testfn),
                (rl("read_init_file"), None),
                (rd("BinaryWriter"), testfn, 1000, 0),
                (rd("BinaryReader"), testfn),
            ]:
                if not fn:
                    continue
                with self.assertRaises(RuntimeError):
                    try:
                        fn(*args)
                    except NotImplementedError:
                        if fn == load_dh_params:
                            # Not callable in some builds
                            load_dh_params = None
                            raise RuntimeError
                        else:
                            raise

        actual_mode = [(a[0], a[1]) for e, a in hook.seen
                       if e == "open" and a[1]]
        actual_flag = [(a[0], a[2]) for e, a in hook.seen
                       if e == "open" and not a[1]]
        self.assertEqual(
            [
                i
                for i in [
                    (testfn, "r"),
                    (sys.executable, "r"),
                    (3, "w"),
                    (testfn, "w"),
                    (testfn, "rb") if load_dh_params else None,
                    (testfn, "r") if readline else None,
                    ("~/.history", "r") if readline else None,
                    (testfn, "w") if readline else None,
                    ("~/.history", "w") if readline else None,
                    (testfn, "a") if rl("append_history_file") else None,
                    ("~/.history", "a") if rl("append_history_file") else None,
                    (testfn, "r") if readline else None,
                    ("<readline_init_file>", "r") if readline else None,
                    (testfn, "wb") if rd("BinaryWriter") else None,
                    (testfn, "rb") if rd("BinaryReader") else None,
                ]
                if i is not None
            ],
            actual_mode,
        )
        self.assertEqual([], actual_flag)

    @isolation.isolated()
    def test_cantrace(self):
        traced = []

        def trace(frame, event, *args):
            if frame.f_code == _Hook.__call__.__code__:
                traced.append(event)

        old = sys.settrace(trace)
        try:
            with _Hook() as hook:
                # No traced call
                eval("1")

                # No traced call
                hook.__cantrace__ = False
                eval("2")

                # One traced call
                hook.__cantrace__ = True
                eval("3")

                # Two traced calls (writing to private member, eval)
                hook.__cantrace__ = 1
                eval("4")

                # One traced call (writing to private member)
                hook.__cantrace__ = 0
        finally:
            sys.settrace(old)

        self.assertEqual(["call"] * 4, traced)

    @isolation.isolated()
    def test_mmap(self):
        import mmap

        with _Hook() as hook:
            mmap.mmap(-1, 8)
        self.assertEqual(hook.seen[0][1][:2], (-1, 8))

    @requires_module("ctypes")
    @isolation.isolated()
    def test_ctypes_call_function(self):
        import ctypes
        import _ctypes

        with _Hook() as hook:
            _ctypes.call_function(ctypes._memmove_addr, (0, 0, 0))
            self.assertIn(
                ("ctypes.call_function", (ctypes._memmove_addr, (0, 0, 0))),
                hook.seen)

            ctypes.CFUNCTYPE(ctypes.c_voidp)(ctypes._memset_addr)(1, 0, 0)
            self.assertIn(
                ("ctypes.call_function", (ctypes._memset_addr, (1, 0, 0))),
                hook.seen)

        with _Hook() as hook:
            ctypes.cast(ctypes.c_voidp(0), ctypes.POINTER(ctypes.c_char))
            self.assertIn("ctypes.call_function", hook.seen_events)

        with _Hook() as hook:
            ctypes.string_at(id("ctypes.string_at") + 40)
            self.assertIn("ctypes.call_function", hook.seen_events)
            self.assertIn("ctypes.string_at", hook.seen_events)

    @requires_module("_posixsubprocess")
    @isolation.isolated()
    def test_posixsubprocess(self):
        import multiprocessing.util

        exe = b"xxx"
        args = [b"yyy", b"zzz"]
        with _Hook() as hook:
            multiprocessing.util.spawnv_passfds(exe, args, ())
        self.assertIn(
            ("_posixsubprocess.fork_exec", ([exe], args, None)), hook.seen)

    @support.requires_subprocess()
    def test_excepthook(self):
        import textwrap
        from test.support import script_helper

        # The "sys.excepthook" audit event fires only for an exception that goes
        # uncaught to the top level of the interpreter, so this case cannot use
        # @isolated() (whose test body runs under unittest, which catches the
        # exception).  Run it as a small script instead.
        code = textwrap.dedent("""\
            import sys

            def excepthook(exc_type, exc_value, exc_tb):
                if exc_type is not RuntimeError:
                    sys.__excepthook__(exc_type, exc_value, exc_tb)

            def hook(event, args):
                if event == "sys.excepthook":
                    if not isinstance(args[2], args[1]):
                        raise TypeError(
                            f"Expected isinstance({args[2]!r}, {args[1]!r})")
                    if args[0] != excepthook:
                        raise ValueError(f"Expected {args[0]} == {excepthook}")
                    print(event, repr(args[2]))

            sys.addaudithook(hook)
            sys.excepthook = excepthook
            raise RuntimeError("fatal-error")
            """)
        rc, stdout, stderr = script_helper.assert_python_failure("-c", code)
        self.assertEqual(
            stdout.strip().decode(),
            "sys.excepthook RuntimeError('fatal-error')",
            stderr.decode(),
        )

    @requires_module("_testcapi")
    @isolation.isolated()
    def test_unraisablehook(self):
        import _testcapi

        def unraisablehook(hookargs):
            pass

        events = []

        def hook(event, args):
            if event == "sys.unraisablehook":
                events.append(args)

        sys.addaudithook(hook)
        sys.unraisablehook = unraisablehook
        _testcapi.err_formatunraisable(
            RuntimeError("nonfatal-error"),
            "Exception ignored for audit hook test")

        self.assertEqual(len(events), 1)
        args = events[0]
        self.assertIs(args[0], unraisablehook)
        self.assertEqual(repr(args[1].exc_value), "RuntimeError('nonfatal-error')")
        self.assertEqual(args[1].err_msg,
                         "Exception ignored for audit hook test")

    @requires_module("winreg")
    @isolation.isolated()
    def test_winreg(self):
        import winreg

        events = []

        def hook(event, args):
            if event.startswith("winreg."):
                events.append((event, args))

        sys.addaudithook(hook)

        k = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, "Software")
        winreg.EnumKey(k, 0)
        with self.assertRaises(OSError):
            winreg.EnumKey(k, 10000)
        kv = k.Detach()
        winreg.CloseKey(kv)

        self.assertEqual(events[0][0], "winreg.OpenKey")
        self.assertEqual(events[1][0], "winreg.OpenKey/result")
        hkey = events[1][1][0]
        self.assertTrue(hkey)
        self.assertEqual(events[2], ("winreg.EnumKey", (hkey, 0)))
        self.assertEqual(events[3], ("winreg.EnumKey", (hkey, 10000)))
        self.assertEqual(events[4], ("winreg.PyHKEY.Detach", (hkey,)))

    @requires_module("socket")
    @isolation.isolated()
    def test_socket(self):
        import socket

        events = []

        def hook(event, args):
            if event.startswith("socket."):
                events.append((event, args))

        sys.addaudithook(hook)

        socket.gethostname()

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            # Don't care if this fails, we just want the audit message
            sock.bind(('127.0.0.1', 8080))
        except OSError:
            pass
        finally:
            sock.close()

        names = [e for e, a in events]
        self.assertEqual(names[0], "socket.gethostname")
        self.assertEqual(names[1], "socket.__new__")
        self.assertEqual(names[2], "socket.bind")
        self.assertEqual(events[2][1][-1], ('127.0.0.1', 8080))

    @isolation.isolated()
    def test_gc(self):
        import gc

        events = []

        def hook(event, args):
            if event.startswith("gc."):
                events.append(event)

        sys.addaudithook(hook)

        gc.get_objects(generation=1)

        x = object()
        y = [x]

        gc.get_referrers(x)
        gc.get_referents(y)

        self.assertEqual(
            events,
            ["gc.get_objects", "gc.get_referrers", "gc.get_referents"])

    @support.requires_resource('network')
    @requires_module("http.client")
    @isolation.isolated()
    def test_http(self):
        import http.client

        events = []

        def hook(event, args):
            if event.startswith("http.client."):
                events.append((event, args[1:]))

        sys.addaudithook(hook)

        conn = http.client.HTTPConnection('www.python.org')
        sent = True
        try:
            conn.request('GET', '/')
        except OSError:
            sent = False
        finally:
            conn.close()

        self.assertEqual(events[0][0], "http.client.connect")
        self.assertEqual(events[0][1], ('www.python.org', 80))
        if sent:
            self.assertEqual(events[1][0], "http.client.send")
            self.assertIn(b'HTTP', events[1][1][0])

    @requires_module("sqlite3")
    @isolation.isolated()
    def test_sqlite3(self):
        import sqlite3

        events = []

        def hook(event, args):
            if event.startswith("sqlite3."):
                events.append(event)

        sys.addaudithook(hook)
        cx1 = sqlite3.connect(":memory:")
        cx2 = sqlite3.Connection(":memory:")

        # Configured without --enable-loadable-sqlite-extensions
        try:
            if hasattr(sqlite3.Connection, "enable_load_extension"):
                cx1.enable_load_extension(False)
                try:
                    cx1.load_extension("test")
                except sqlite3.OperationalError:
                    pass
                else:
                    self.fail("Expected sqlite3.load_extension to fail")
        finally:
            cx1.close()
            cx2.close()

        expected = ["sqlite3.connect", "sqlite3.connect/handle"] * 2
        if hasattr(sqlite3.Connection, "enable_load_extension"):
            expected += [
                "sqlite3.enable_load_extension",
                "sqlite3.load_extension",
            ]
        self.assertEqual(events, expected)

    @isolation.isolated()
    def test_sys_getframe(self):
        events = []

        def hook(event, args):
            if event.startswith("sys."):
                events.append((event, args[0].f_code.co_name))

        sys.addaudithook(hook)
        sys._getframe()
        self.assertEqual(events, [("sys._getframe", "test_sys_getframe")])

    @isolation.isolated()
    def test_sys_getframemodulename(self):
        events = []

        def hook(event, args):
            if event.startswith("sys."):
                events.append((event, args))

        sys.addaudithook(hook)
        sys._getframemodulename()
        self.assertEqual(events, [("sys._getframemodulename", (0,))])

    @isolation.isolated()
    def test_threading(self):
        import _thread

        events = []

        def hook(event, args):
            if event.startswith(("_thread.", "cpython.PyThreadState", "test.")):
                events.append(event)

        sys.addaudithook(hook)

        lock = _thread.allocate_lock()
        lock.acquire()

        class test_func:
            def __repr__(self):
                return "<test_func>"

            def __call__(self):
                sys.audit("test.test_func")
                lock.release()

        _thread.start_new_thread(test_func(), ())
        lock.acquire()

        handle = _thread.start_joinable_thread(test_func())
        handle.join()

        self.assertEqual(
            events,
            [
                "_thread.start_new_thread",
                "test.test_func",
                "_thread.start_joinable_thread",
                "test.test_func",
            ],
        )

    @requires_module("_wmi")
    @isolation.isolated()
    def test_wmi_exec_query(self):
        import _wmi

        events = []

        def hook(event, args):
            if event.startswith("_wmi."):
                events.append((event, args))

        sys.addaudithook(hook)
        try:
            _wmi.exec_query("SELECT * FROM Win32_OperatingSystem")
        except OSError as e:
            # gh-112278: WMI may be slow response when first called, but we still
            # get the audit event, so just ignore the timeout
            if e.winerror != 258:
                raise

        self.assertEqual(len(events), 1)
        self.assertEqual(events[0][0], "_wmi.exec_query")
        self.assertEqual(events[0][1][0],
                         "SELECT * FROM Win32_OperatingSystem")

    @requires_module("syslog")
    @isolation.isolated()
    def test_syslog(self):
        import syslog

        events = []

        def hook(event, args):
            if event.startswith("syslog."):
                events.append((event, args))

        sys.addaudithook(hook)

        # The default ident is derived from sys.argv[0].
        default_ident = os.path.basename(sys.argv[0])

        syslog.openlog('python')
        syslog.syslog('test')
        syslog.setlogmask(syslog.LOG_DEBUG)
        syslog.closelog()
        # implicit open
        syslog.syslog('test2')
        # open with default ident
        syslog.openlog(logoption=syslog.LOG_NDELAY, facility=syslog.LOG_LOCAL0)
        sys.argv = None
        syslog.openlog()
        syslog.closelog()

        self.assertEqual(
            events,
            [('syslog.openlog', ('python', 0, syslog.LOG_USER)),
             ('syslog.syslog', (syslog.LOG_INFO, 'test')),
             ('syslog.setlogmask', (syslog.LOG_DEBUG,)),
             ('syslog.closelog', ()),
             ('syslog.syslog', (syslog.LOG_INFO, 'test2')),
             ('syslog.openlog', (default_ident, 0, syslog.LOG_USER)),
             ('syslog.openlog',
              (default_ident, syslog.LOG_NDELAY, syslog.LOG_LOCAL0)),
             ('syslog.openlog', (None, 0, syslog.LOG_USER)),
             ('syslog.closelog', ())],
        )

    @isolation.isolated()
    def test_not_in_gc(self):
        import gc

        hook = lambda *a: None
        sys.addaudithook(hook)

        for o in gc.get_objects():
            if isinstance(o, list):
                self.assertNotIn(hook, o)

    @isolation.isolated()
    def test_time(self):
        import time

        events = []

        def hook(event, args):
            if event.startswith("time."):
                events.append((event, args))

        sys.addaudithook(hook)

        time.sleep(0)
        time.sleep(0.0625)  # 1/16, a small exact float
        with self.assertRaises(ValueError):
            time.sleep(-1)

        self.assertEqual(
            [(ev, args[0]) for ev, args in events],
            [("time.sleep", 0), ("time.sleep", 0.0625), ("time.sleep", -1)],
        )

    @isolation.isolated()
    def test_time_fail(self):
        # A hook raising on an audited event propagates out of the operation.
        import time

        with self.assertRaises(AssertionError):
            with _Hook(raise_on_events={"time.sleep"},
                       exc_type=AssertionError):
                time.sleep(0)

    @isolation.isolated()
    def test_sys_monitoring_register_callback(self):
        events = []

        def hook(event, args):
            if event.startswith("sys.monitoring"):
                events.append((event, args))

        sys.addaudithook(hook)
        sys.monitoring.register_callback(1, 1, None)
        self.assertEqual(
            events, [("sys.monitoring.register_callback", (None,))])

    @requires_module("_winapi")
    @isolation.isolated()
    def test_winapi_createnamedpipe(self):
        import _winapi

        pipe_name = r"\\.\pipe\LOCAL\test_winapi_createnamed_pipe"

        events = []

        def hook(event, args):
            if event == "_winapi.CreateNamedPipe":
                events.append((event, args))

        sys.addaudithook(hook)
        _winapi.CreateNamedPipe(pipe_name, _winapi.PIPE_ACCESS_DUPLEX,
                                8, 2, 0, 0, 0, 0)

        self.assertEqual(events, [("_winapi.CreateNamedPipe", (pipe_name, 3, 8))])

    @isolation.isolated()
    def test_assert_unicode(self):
        # See gh-126018
        sys.addaudithook(lambda *args: None)
        with self.assertRaises(TypeError):
            sys.audit(9)

    @support.support_remote_exec_only
    @support.cpython_only
    @isolation.isolated()
    def test_sys_remote_exec(self):
        import tempfile
        import time

        pid = os.getpid()
        events = []

        def hook(event, args):
            if event in ("sys.remote_exec", "cpython.remote_debugger_script"):
                events.append((event, args))

        sys.addaudithook(hook)
        with tempfile.NamedTemporaryFile(mode='w+', delete=True) as tmp_file:
            tmp_file.write("a = 1+1\n")
            tmp_file.flush()
            sys.remote_exec(pid, tmp_file.name)
            # The remote-exec script runs as a pending call in this very
            # interpreter; give it a chance to run before checking.
            deadline = time.monotonic() + support.SHORT_TIMEOUT
            while time.monotonic() < deadline:
                if any(e == "cpython.remote_debugger_script"
                       for e, a in events):
                    break
                time.sleep(0.01)

            by_event = dict(events)
            self.assertIn("sys.remote_exec", by_event)
            self.assertIn("cpython.remote_debugger_script", by_event)
            self.assertEqual(by_event["sys.remote_exec"],
                             (pid, tmp_file.name))
            self.assertEqual(by_event["cpython.remote_debugger_script"],
                             (tmp_file.name,))

    @isolation.isolated()
    def test_import_module(self):
        import importlib

        with _Hook() as hook:
            importlib.import_module("importlib")  # imported, won't be logged
            importlib.import_module("email")  # standard library module
            importlib.import_module("test.pythoninfo")  # random module
            importlib.import_module(".audit_test_data.submodule", "test")  # relative
            importlib.import_module("test.audit_test_data.submodule2")  # absolute
            importlib.import_module("_testcapi")  # extension module

        actual = [a for e, a in hook.seen if e == "import"]
        self.assertEqual(
            actual,
            [
                ("email", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.pythoninfo", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.audit_test_data.submodule", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.audit_test_data", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.audit_test_data.submodule2", None, sys.path, sys.meta_path, sys.path_hooks),
                ("_testcapi", None, sys.path, sys.meta_path, sys.path_hooks),
                ("_testcapi", ANY, None, None, None),
            ],
        )

    @isolation.isolated()
    def test_builtin__import__(self):
        import importlib  # noqa: F401

        with _Hook() as hook:
            __import__("importlib")
            __import__("email")
            __import__("test.pythoninfo")
            __import__("audit_test_data.submodule", level=1,
                       globals={"__package__": "test"})
            __import__("test.audit_test_data.submodule2")
            __import__("_testcapi")

        actual = [a for e, a in hook.seen if e == "import"]
        self.assertEqual(
            actual,
            [
                ("email", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.pythoninfo", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.audit_test_data.submodule", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.audit_test_data", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.audit_test_data.submodule2", None, sys.path, sys.meta_path, sys.path_hooks),
                ("_testcapi", None, sys.path, sys.meta_path, sys.path_hooks),
                ("_testcapi", ANY, None, None, None),
            ],
        )

    @isolation.isolated()
    def test_import_statement(self):
        with _Hook() as hook:
            import importlib  # noqa: F401
            import email  # noqa: F401
            import test.pythoninfo  # noqa: F401
            from .audit_test_data import submodule  # noqa: F401
            import test.audit_test_data.submodule2  # noqa: F401
            import _testcapi  # noqa: F401

        actual = [a for e, a in hook.seen if e == "import"]
        # Import statement ordering is different because the package is
        # loaded first and then the submodule
        self.assertEqual(
            actual,
            [
                ("email", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.pythoninfo", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.audit_test_data", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.audit_test_data.submodule", None, sys.path, sys.meta_path, sys.path_hooks),
                ("test.audit_test_data.submodule2", None, sys.path, sys.meta_path, sys.path_hooks),
                ("_testcapi", None, sys.path, sys.meta_path, sys.path_hooks),
                ("_testcapi", ANY, None, None, None),
            ],
        )


if __name__ == "__main__":
    unittest.main()
