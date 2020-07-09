"""This script contains the actual auditing tests.

It should not be imported directly, but should be run by the test_audit
module with arguments identifying each test.

"""

import contextlib
import sys


class TestHook:
    """Used in standard hook tests to collect any logged events.

    Should be used in a with block to ensure that it has no impact
    after the test completes.
    """

    def __init__(self, raise_on_events=None, exc_type=RuntimeError):
        self.raise_on_events = raise_on_events or ()
        self.exc_type = exc_type
        self.seen = []
        self.closed = False

    def __enter__(self, *a):
        sys.addaudithook(self)
        return self

    def __exit__(self, *a):
        self.close()

    def close(self):
        self.closed = True

    @property
    def seen_events(self):
        return [i[0] for i in self.seen]

    def __call__(self, event, args):
        if self.closed:
            return
        self.seen.append((event, args))
        if event in self.raise_on_events:
            raise self.exc_type("saw event " + event)


# Simple helpers, since we are not in unittest here
def assertEqual(x, y):
    if x != y:
        raise AssertionError(f"{x!r} should equal {y!r}")


def assertIn(el, series):
    if el not in series:
        raise AssertionError(f"{el!r} should be in {series!r}")


def assertNotIn(el, series):
    if el in series:
        raise AssertionError(f"{el!r} should not be in {series!r}")


def assertSequenceEqual(x, y):
    if len(x) != len(y):
        raise AssertionError(f"{x!r} should equal {y!r}")
    if any(ix != iy for ix, iy in zip(x, y)):
        raise AssertionError(f"{x!r} should equal {y!r}")


@contextlib.contextmanager
def assertRaises(ex_type):
    try:
        yield
        assert False, f"expected {ex_type}"
    except BaseException as ex:
        if isinstance(ex, AssertionError):
            raise
        assert type(ex) is ex_type, f"{ex} should be {ex_type}"


def test_basic():
    with TestHook() as hook:
        sys.audit("test_event", 1, 2, 3)
        assertEqual(hook.seen[0][0], "test_event")
        assertEqual(hook.seen[0][1], (1, 2, 3))


def test_block_add_hook():
    # Raising an exception should prevent a new hook from being added,
    # but will not propagate out.
    with TestHook(raise_on_events="sys.addaudithook") as hook1:
        with TestHook() as hook2:
            sys.audit("test_event")
            assertIn("test_event", hook1.seen_events)
            assertNotIn("test_event", hook2.seen_events)


def test_block_add_hook_baseexception():
    # Raising BaseException will propagate out when adding a hook
    with assertRaises(BaseException):
        with TestHook(
            raise_on_events="sys.addaudithook", exc_type=BaseException
        ) as hook1:
            # Adding this next hook should raise BaseException
            with TestHook() as hook2:
                pass


def test_pickle():
    import pickle

    class PicklePrint:
        def __reduce_ex__(self, p):
            return str, ("Pwned!",)

    payload_1 = pickle.dumps(PicklePrint())
    payload_2 = pickle.dumps(("a", "b", "c", 1, 2, 3))

    # Before we add the hook, ensure our malicious pickle loads
    assertEqual("Pwned!", pickle.loads(payload_1))

    with TestHook(raise_on_events="pickle.find_class") as hook:
        with assertRaises(RuntimeError):
            # With the hook enabled, loading globals is not allowed
            pickle.loads(payload_1)
        # pickles with no globals are okay
        pickle.loads(payload_2)


def test_monkeypatch():
    class A:
        pass

    class B:
        pass

    class C(A):
        pass

    a = A()

    with TestHook() as hook:
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

    actual = [(a[0], a[1]) for e, a in hook.seen if e == "object.__setattr__"]
    assertSequenceEqual(
        [(C, "__name__"), (C, "__bases__"), (C, "__bases__"), (a, "__class__")], actual
    )


def test_open():
    # SSLContext.load_dh_params uses _Py_fopen_obj rather than normal open()
    try:
        import ssl

        load_dh_params = ssl.create_default_context().load_dh_params
    except ImportError:
        load_dh_params = None

    # Try a range of "open" functions.
    # All of them should fail
    with TestHook(raise_on_events={"open"}) as hook:
        for fn, *args in [
            (open, sys.argv[2], "r"),
            (open, sys.executable, "rb"),
            (open, 3, "wb"),
            (open, sys.argv[2], "w", -1, None, None, None, False, lambda *a: 1),
            (load_dh_params, sys.argv[2]),
        ]:
            if not fn:
                continue
            with assertRaises(RuntimeError):
                fn(*args)

    actual_mode = [(a[0], a[1]) for e, a in hook.seen if e == "open" and a[1]]
    actual_flag = [(a[0], a[2]) for e, a in hook.seen if e == "open" and not a[1]]
    assertSequenceEqual(
        [
            i
            for i in [
                (sys.argv[2], "r"),
                (sys.executable, "r"),
                (3, "w"),
                (sys.argv[2], "w"),
                (sys.argv[2], "rb") if load_dh_params else None,
            ]
            if i is not None
        ],
        actual_mode,
    )
    assertSequenceEqual([], actual_flag)


def test_cantrace():
    traced = []

    def trace(frame, event, *args):
        if frame.f_code == TestHook.__call__.__code__:
            traced.append(event)

    old = sys.settrace(trace)
    try:
        with TestHook() as hook:
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

    assertSequenceEqual(["call"] * 4, traced)


def test_mmap():
    import mmap

    with TestHook() as hook:
        mmap.mmap(-1, 8)
        assertEqual(hook.seen[0][1][:2], (-1, 8))


def test_excepthook():
    def excepthook(exc_type, exc_value, exc_tb):
        if exc_type is not RuntimeError:
            sys.__excepthook__(exc_type, exc_value, exc_tb)

    def hook(event, args):
        if event == "sys.excepthook":
            if not isinstance(args[2], args[1]):
                raise TypeError(f"Expected isinstance({args[2]!r}, " f"{args[1]!r})")
            if args[0] != excepthook:
                raise ValueError(f"Expected {args[0]} == {excepthook}")
            print(event, repr(args[2]))

    sys.addaudithook(hook)
    sys.excepthook = excepthook
    raise RuntimeError("fatal-error")


def test_unraisablehook():
    from _testcapi import write_unraisable_exc

    def unraisablehook(hookargs):
        pass

    def hook(event, args):
        if event == "sys.unraisablehook":
            if args[0] != unraisablehook:
                raise ValueError(f"Expected {args[0]} == {unraisablehook}")
            print(event, repr(args[1].exc_value), args[1].err_msg)

    sys.addaudithook(hook)
    sys.unraisablehook = unraisablehook
    write_unraisable_exc(RuntimeError("nonfatal-error"), "for audit hook test", None)


def test_winreg():
    from winreg import OpenKey, EnumKey, CloseKey, HKEY_LOCAL_MACHINE

    def hook(event, args):
        if not event.startswith("winreg."):
            return
        print(event, *args)

    sys.addaudithook(hook)

    k = OpenKey(HKEY_LOCAL_MACHINE, "Software")
    EnumKey(k, 0)
    try:
        EnumKey(k, 10000)
    except OSError:
        pass
    else:
        raise RuntimeError("Expected EnumKey(HKLM, 10000) to fail")

    kv = k.Detach()
    CloseKey(kv)


def test_socket():
    import socket

    def hook(event, args):
        if event.startswith("socket."):
            print(event, *args)

    sys.addaudithook(hook)

    socket.gethostname()

    # Don't care if this fails, we just want the audit message
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        # Don't care if this fails, we just want the audit message
        sock.bind(('127.0.0.1', 8080))
    except Exception:
        pass
    finally:
        sock.close()


def test_gc():
    import gc

    def hook(event, args):
        if event.startswith("gc."):
            print(event, *args)

    sys.addaudithook(hook)

    gc.get_objects(generation=1)

    x = object()
    y = [x]

    gc.get_referrers(x)
    gc.get_referents(y)


def test_http_client():
    import http.client

    def hook(event, args):
        if event.startswith("http.client."):
            print(event, *args[1:])

    sys.addaudithook(hook)

    conn = http.client.HTTPConnection('www.python.org')
    try:
        conn.request('GET', '/')
    except OSError:
        print('http.client.send', '[cannot send]')
    finally:
        conn.close()


if __name__ == "__main__":
    from test.support import suppress_msvcrt_asserts

    suppress_msvcrt_asserts()

    test = sys.argv[1]
    globals()[test]()
