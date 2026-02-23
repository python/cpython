import unittest

from test.support import import_helper
from test.support import threading_helper

# Raise SkipTest if subinterpreters not supported.
import_helper.import_module("_interpreters")
from concurrent.interpreters import NotShareableError, share, SharedObjectProxy
from test.test_interpreters.utils import TestBase
from threading import Barrier, Thread, Lock, local
from concurrent import interpreters
from contextlib import contextmanager


class SharedObjectProxyTests(TestBase):
    @contextmanager
    def create_interp(self, **to_prepare):
        interp = interpreters.create()
        try:
            if to_prepare != {}:
                interp.prepare_main(**to_prepare)
            yield interp
        finally:
            interp.close()

    def run_concurrently(self, func, num_threads=4, **to_prepare):
        barrier = Barrier(num_threads)

        def thread():
            with self.create_interp(**to_prepare) as interp:
                barrier.wait()
                func(interp)

        with threading_helper.catch_threading_exception() as cm:
            with threading_helper.start_threads(
                (Thread(target=thread) for _ in range(num_threads))
            ):
                pass

            if cm.exc_value is not None:
                raise cm.exc_value

    def test_create(self):
        proxy = share(object())
        self.assertIsInstance(proxy, SharedObjectProxy)

        # Shareable objects should pass through
        for shareable in (
            None,
            True,
            False,
            100,
            10000,
            "hello",
            b"world",
            memoryview(b"test"),
        ):
            self.assertTrue(interpreters.is_shareable(shareable))
            with self.subTest(shareable=shareable):
                not_a_proxy = share(shareable)
                self.assertNotIsInstance(not_a_proxy, SharedObjectProxy)
                self.assertIs(not_a_proxy, shareable)

    @threading_helper.requires_working_threading()
    def test_create_concurrently(self):
        def thread(interp):
            for iteration in range(100):
                with self.subTest(iteration=iteration):
                    interp.exec(
                        """if True:
                    from concurrent.interpreters import share

                    share(object())"""
                    )

        self.run_concurrently(thread)

    def test_access_proxy(self):
        class Test:
            def silly(self):
                return "silly"

        obj = Test()
        obj.test = "silly"
        proxy = share(obj)
        with self.create_interp(proxy=proxy) as interp:
            interp.exec("assert proxy.test == 'silly'")
            interp.exec("assert isinstance(proxy.test, str)")
            interp.exec(
                """if True:
            from concurrent.interpreters import SharedObjectProxy
            method = proxy.silly
            assert isinstance(method, SharedObjectProxy)
            assert method() == 'silly'
            assert isinstance(method(), str)
            """
            )
            with self.assertRaises(interpreters.ExecutionFailed):
                interp.exec("proxy.noexist")

    @threading_helper.requires_working_threading()
    def test_access_proxy_concurrently(self):
        class Test:
            def __init__(self):
                self.lock = Lock()
                self.value = 0

            def increment(self):
                with self.lock:
                    self.value += 1

        test = Test()
        proxy = share(test)

        def thread(interp):
            for _ in range(100):
                interp.exec("proxy.increment()")
                interp.exec("assert isinstance(proxy.value, int)")

        self.run_concurrently(thread, proxy=proxy)
        self.assertEqual(test.value, 400)

    def test_proxy_call(self):
        constant = 67  # Hilarious

        def my_function(arg=1, /, *, arg2=2):
            # We need the constant here to make this function unshareable.
            return constant + arg + arg2

        proxy = share(my_function)
        self.assertIsInstance(proxy, SharedObjectProxy)
        self.assertEqual(proxy(), 70)
        self.assertEqual(proxy(0, arg2=1), 68)
        self.assertEqual(proxy(2), 71)

        with self.create_interp(proxy=proxy) as interp:
            interp.exec(
                """if True:
            assert isinstance(proxy(), int)
            assert proxy() == 70
            assert proxy(0, arg2=1) == 68
            assert proxy(2) == 71"""
            )

    def test_proxy_call_args(self):
        def shared(arg):
            return type(arg).__name__

        proxy = share(shared)
        self.assertEqual(proxy(1), "int")
        self.assertEqual(proxy("test"), "str")
        self.assertEqual(proxy(object()), "SharedObjectProxy")

        with self.create_interp(proxy=proxy) as interp:
            interp.exec("assert proxy(1) == 'int'")
            interp.exec("assert proxy('test') == 'str'")
            interp.exec("assert proxy(object()) == 'SharedObjectProxy'")

    def test_proxy_call_return(self):
        class Test:
            def __init__(self, silly):
                self.silly = silly

        def shared():
            return Test("silly")

        proxy = share(shared)
        res = proxy()
        self.assertIsInstance(res, SharedObjectProxy)
        self.assertEqual(res.silly, "silly")

        with self.create_interp(proxy=proxy) as interp:
            interp.exec(
                """if True:
            obj = proxy()
            assert obj.silly == 'silly'
            assert type(obj).__name__ == 'SharedObjectProxy'"""
            )

    def test_proxy_call_concurrently(self):
        def shared(arg, *, kwarg):
            return arg + kwarg

        class Weird:
            def __add__(_self, other):
                self.assertIsInstance(_self, Weird)
                self.assertIsInstance(other, int)
                if other == 24:
                    ob = Weird()
                    ob.silly = "test"
                    return ob
                return 42

        def thread(interp):
            interp.exec("assert proxy(1, kwarg=2) == 3")
            interp.exec("assert proxy(2, kwarg=5) == 7")
            interp.exec("assert proxy(weird, kwarg=5) == 42")
            interp.exec("assert proxy(weird, kwarg=24).silly == 'test'")

        proxy = share(shared)
        weird = share(Weird())
        self.run_concurrently(thread, proxy=proxy, weird=weird)

    def test_proxy_reference_cycle(self):
        import gc

        called = 0

        class Cycle:
            def __init__(self, other):
                self.cycle = self
                self.other = other

            def __del__(self):
                nonlocal called
                called += 1

        cycle_type = share(Cycle)
        interp_a = cycle_type(0)
        with self.create_interp(cycle_type=cycle_type, interp_a=interp_a) as interp:
            interp.exec("interp_b = cycle_type(interp_a)")

        self.assertEqual(called, 0)
        del interp_a
        for _ in range(3):
            gc.collect()

        self.assertEqual(called, 2)

    def test_proxy_attribute_concurrently(self):
        class Test:
            def __init__(self):
                self.value = 0

        proxy = share(Test())

        def thread(interp):
            for _ in range(1000):
                interp.exec("proxy.value = 42")
                interp.exec("proxy.value = 0")
                interp.exec("assert proxy.value in (0, 42)")

        self.run_concurrently(thread, proxy=proxy)

    def test_retain_thread_local_variables(self):
        thread_local = local()
        thread_local.value = 42

        def test():
            old = thread_local.value
            thread_local.value = 24
            return old

        proxy = share(test)
        with self.create_interp(proxy=proxy) as interp:
            interp.exec("assert proxy() == 42")
            self.assertEqual(thread_local.value, 24)

    def test_destruct_object_in_subinterp(self):
        called = False

        class Test:
            def __del__(self):
                nonlocal called
                called = True

        foo = Test()
        proxy = share(foo)
        with self.create_interp(proxy=proxy):
            self.assertFalse(called)
            del foo, proxy
            self.assertFalse(called)

        self.assertTrue(called)

    def test_called_in_correct_interpreter(self):
        called = False

        def foo():
            nonlocal called
            self.assertEqual(interpreters.get_current(), interpreters.get_main())
            called = True

        proxy = share(foo)
        with self.create_interp(proxy=proxy) as interp:
            interp.exec("proxy()")

        self.assertTrue(called)

    def test_proxy_reshare_does_not_copy(self):
        class Test:
            pass

        proxy = share(Test())
        reproxy = share(proxy)
        self.assertIs(proxy, reproxy)

    def test_object_share_method(self):
        class Test:
            def __share__(self):
                return 42

        shared = share(Test())
        self.assertEqual(shared, 42)

    def test_object_share_method_failure(self):
        class Test:
            def __share__(self):
                return self

        exception = RuntimeError("ouch")
        class Evil:
            def __share__(self):
                raise exception

        with self.assertRaises(NotShareableError):
            share(Test())

        with self.assertRaises(RuntimeError) as exc:
            share(Evil())

        self.assertIs(exc.exception, exception)

    def test_proxy_manual_construction(self):
        called = False

        class Test:
            def __init__(self):
                self.attr = 24

            def __share__(self):
                nonlocal called
                called = True
                return 42

        proxy = SharedObjectProxy(Test())
        self.assertIsInstance(proxy, SharedObjectProxy)
        self.assertFalse(called)
        self.assertEqual(proxy.attr, 24)

if __name__ == "__main__":
    unittest.main()
