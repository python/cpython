"""Tests for sys.audit and sys.addaudithook
"""

import os
import subprocess
import sys
import unittest
from test import support

if not hasattr(sys, "addaudithook") or not hasattr(sys, "audit"):
    raise unittest.SkipTest("test only relevant when sys.audit is available")


class TestHook:
    """Used in standard hook tests to collect any logged events.

    Should be used in a with block to ensure that it has no impact
    after the test completes. Audit hooks cannot be removed, so the
    best we can do for the test run is disable it by calling close().
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


class TestFinalizeHook:
    """Used in the test_finalize_hooks function to ensure that hooks
    are correctly cleaned up, that they are notified about the cleanup,
    and are unable to prevent it.
    """

    def __init__(self):
        print("Created", id(self), file=sys.stderr, flush=True)

    def __call__(self, event, args):
        # Avoid recursion when we call id() below
        if event == "builtins.id":
            return

        print(event, id(self), file=sys.stderr, flush=True)

        if event == "cpython._PySys_ClearAuditHooks":
            raise RuntimeError("Should be ignored")
        elif event == "cpython.PyInterpreterState_Clear":
            raise RuntimeError("Should be ignored")


def run_finalize_test():
    """Called by test_finalize_hooks in a subprocess."""
    sys.addaudithook(TestFinalizeHook())


class AuditTest(unittest.TestCase):
    def test_basic(self):
        with TestHook() as hook:
            sys.audit("test_event", 1, 2, 3)
            self.assertEqual(hook.seen[0][0], "test_event")
            self.assertEqual(hook.seen[0][1], (1, 2, 3))

    def test_block_add_hook(self):
        # Raising an exception should prevent a new hook from being added,
        # but will not propagate out.
        with TestHook(raise_on_events="sys.addaudithook") as hook1:
            with TestHook() as hook2:
                sys.audit("test_event")
                self.assertIn("test_event", hook1.seen_events)
                self.assertNotIn("test_event", hook2.seen_events)

    def test_block_add_hook_baseexception(self):
        # Raising BaseException will propagate out when adding a hook
        with self.assertRaises(BaseException):
            with TestHook(
                raise_on_events="sys.addaudithook", exc_type=BaseException
            ) as hook1:
                # Adding this next hook should raise BaseException
                with TestHook() as hook2:
                    pass

    def test_finalize_hooks(self):
        events = []
        with subprocess.Popen(
            [
                sys.executable,
                "-c",
                "import test.test_audit; test.test_audit.run_finalize_test()",
            ],
            encoding="utf-8",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ) as p:
            p.wait()
            for line in p.stderr:
                events.append(line.strip().partition(" "))
        firstId = events[0][2]
        self.assertSequenceEqual(
            [
                ("Created", " ", firstId),
                ("cpython._PySys_ClearAuditHooks", " ", firstId),
            ],
            events,
        )

    def test_pickle(self):
        pickle = support.import_module("pickle")

        class PicklePrint:
            def __reduce_ex__(self, p):
                return str, ("Pwned!",)

        payload_1 = pickle.dumps(PicklePrint())
        payload_2 = pickle.dumps(("a", "b", "c", 1, 2, 3))

        # Before we add the hook, ensure our malicious pickle loads
        self.assertEqual("Pwned!", pickle.loads(payload_1))

        with TestHook(raise_on_events="pickle.find_class") as hook:
            with self.assertRaises(RuntimeError):
                # With the hook enabled, loading globals is not allowed
                pickle.loads(payload_1)
            # pickles with no globals are okay
            pickle.loads(payload_2)

    def test_monkeypatch(self):
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
        self.assertSequenceEqual(
            [(C, "__name__"), (C, "__bases__"), (C, "__bases__"), (a, "__class__")],
            actual,
        )

    def test_open(self):
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
                (open, support.TESTFN, "r"),
                (open, sys.executable, "rb"),
                (open, 3, "wb"),
                (open, support.TESTFN, "w", -1, None, None, None, False, lambda *a: 1),
                (load_dh_params, support.TESTFN),
            ]:
                if not fn:
                    continue
                self.assertRaises(RuntimeError, fn, *args)

        actual_mode = [(a[0], a[1]) for e, a in hook.seen if e == "open" and a[1]]
        actual_flag = [(a[0], a[2]) for e, a in hook.seen if e == "open" and not a[1]]
        self.assertSequenceEqual(
            [
                i
                for i in [
                    (support.TESTFN, "r"),
                    (sys.executable, "r"),
                    (3, "w"),
                    (support.TESTFN, "w"),
                    (support.TESTFN, "rb") if load_dh_params else None,
                ]
                if i is not None
            ],
            actual_mode,
        )
        self.assertSequenceEqual([], actual_flag)

    def test_cantrace(self):
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

        self.assertSequenceEqual(["call"] * 4, traced)


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "spython_test":
        # Doesn't matter what we add - it will be blocked
        sys.addaudithook(None)

        sys.exit(0)

    unittest.main()
