import _thread
import asyncio
import contextvars
import re
import signal
import sys
import threading
import unittest
from test.test_asyncio import utils as test_utils
from unittest import mock
from unittest.mock import patch


def tearDownModule():
    asyncio.set_event_loop(None)


def interrupt_self():
    _thread.interrupt_main()


class BaseTest(unittest.TestCase):

    def new_loop(self):
        loop = asyncio.BaseEventLoop()
        loop._process_events = mock.Mock()
        # Mock waking event loop from select
        loop._write_to_self = mock.Mock()
        loop._write_to_self.return_value = None
        loop._selector = mock.Mock()
        loop._selector.select.return_value = ()
        loop.shutdown_ag_run = False

        async def shutdown_asyncgens():
            loop.shutdown_ag_run = True
        loop.shutdown_asyncgens = shutdown_asyncgens

        # Track created loops so tearDown can assert they were closed and
        # had their async generators shut down.
        self._loops.append(loop)
        return loop

    def setUp(self):
        super().setUp()

        # Isolate the current thread's event loop and use ``new_loop`` as
        # an explicit ``loop_factory`` in the tests.
        self._loops = []
        asyncio.set_event_loop(None)

    def tearDown(self):
        if self._loops:
            loop = self._loops[-1]
            self.assertTrue(loop.is_closed())
            self.assertTrue(loop.shutdown_ag_run)

        asyncio.set_event_loop(None)
        super().tearDown()


class RunTests(BaseTest):

    def test_asyncio_run_return(self):
        async def main():
            await asyncio.sleep(0)
            return 42

        self.assertEqual(asyncio.run(main(), loop_factory=self.new_loop), 42)

    def test_asyncio_run_raises(self):
        async def main():
            await asyncio.sleep(0)
            raise ValueError('spam')

        with self.assertRaisesRegex(ValueError, 'spam'):
            asyncio.run(main(), loop_factory=self.new_loop)

    def test_asyncio_run_only_coro(self):
        for o in {1, lambda: None}:
            with self.subTest(obj=o), \
                    self.assertRaisesRegex(TypeError,
                                           'an awaitable is required'):
                asyncio.run(o)

    def test_asyncio_run_debug(self):
        async def main(expected):
            loop = asyncio.get_event_loop()
            self.assertIs(loop.get_debug(), expected)

        asyncio.run(main(False), debug=False, loop_factory=self.new_loop)
        asyncio.run(main(True), debug=True, loop_factory=self.new_loop)
        with mock.patch('asyncio.coroutines._is_debug_mode', lambda: True):
            asyncio.run(main(True), loop_factory=self.new_loop)
            asyncio.run(main(False), debug=False, loop_factory=self.new_loop)
        with mock.patch('asyncio.coroutines._is_debug_mode', lambda: False):
            asyncio.run(main(True), debug=True, loop_factory=self.new_loop)
            asyncio.run(main(False), loop_factory=self.new_loop)

    def test_asyncio_run_from_running_loop(self):
        async def main():
            coro = main()
            try:
                asyncio.run(coro, loop_factory=self.new_loop)
            finally:
                coro.close()  # Suppress ResourceWarning

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot be called from a running'):
            asyncio.run(main(), loop_factory=self.new_loop)

    def test_asyncio_run_cancels_hanging_tasks(self):
        lo_task = None

        async def leftover():
            await asyncio.sleep(0.1)

        async def main():
            nonlocal lo_task
            lo_task = asyncio.create_task(leftover())
            return 123

        self.assertEqual(asyncio.run(main(), loop_factory=self.new_loop), 123)
        self.assertTrue(lo_task.done())

    def test_asyncio_run_reports_hanging_tasks_errors(self):
        lo_task = None
        call_exc_handler_mock = mock.Mock()

        async def leftover():
            try:
                await asyncio.sleep(0.1)
            except asyncio.CancelledError:
                1 / 0

        async def main():
            loop = asyncio.get_running_loop()
            loop.call_exception_handler = call_exc_handler_mock

            nonlocal lo_task
            lo_task = asyncio.create_task(leftover())
            return 123

        self.assertEqual(asyncio.run(main(), loop_factory=self.new_loop), 123)
        self.assertTrue(lo_task.done())

        call_exc_handler_mock.assert_called_with({
            'message': test_utils.MockPattern(r'asyncio.run.*shutdown'),
            'task': lo_task,
            'exception': test_utils.MockInstanceOf(ZeroDivisionError)
        })

    def test_asyncio_run_closes_gens_after_hanging_tasks_errors(self):
        spinner = None
        lazyboy = None

        class FancyExit(Exception):
            pass

        async def fidget():
            while True:
                yield 1
                await asyncio.sleep(1)

        async def spin():
            nonlocal spinner
            spinner = fidget()
            try:
                async for the_meaning_of_life in spinner:  # NoQA
                    pass
            except asyncio.CancelledError:
                1 / 0

        async def main():
            loop = asyncio.get_running_loop()
            loop.call_exception_handler = mock.Mock()

            nonlocal lazyboy
            lazyboy = asyncio.create_task(spin())
            raise FancyExit

        with self.assertRaises(FancyExit):
            asyncio.run(main(), loop_factory=self.new_loop)

        self.assertTrue(lazyboy.done())

        self.assertIsNone(spinner.ag_frame)
        self.assertFalse(spinner.ag_running)

    def test_asyncio_run_set_event_loop(self):
        #See https://github.com/python/cpython/issues/93896

        async def main():
            await asyncio.sleep(0)
            return 42

        # asyncio.run() must set the event loop for the running thread.
        # This only happens when no explicit loop_factory is passed, so
        # patch the default loop creation to use a mock loop and verify
        # set_event_loop() is called.
        with mock.patch('asyncio.events.new_event_loop', self.new_loop), \
                mock.patch('asyncio.runners.events.set_event_loop') as set_event_loop:
            asyncio.run(main())
        self.assertTrue(set_event_loop.called)

    def test_asyncio_run_without_uncancel(self):
        # See https://github.com/python/cpython/issues/95097
        class Task:
            def __init__(self, loop, coro, **kwargs):
                self._task = asyncio.Task(coro, loop=loop, **kwargs)

            def cancel(self, *args, **kwargs):
                return self._task.cancel(*args, **kwargs)

            def add_done_callback(self, *args, **kwargs):
                return self._task.add_done_callback(*args, **kwargs)

            def remove_done_callback(self, *args, **kwargs):
                return self._task.remove_done_callback(*args, **kwargs)

            @property
            def _asyncio_future_blocking(self):
                return self._task._asyncio_future_blocking

            def result(self, *args, **kwargs):
                return self._task.result(*args, **kwargs)

            def done(self, *args, **kwargs):
                return self._task.done(*args, **kwargs)

            def cancelled(self, *args, **kwargs):
                return self._task.cancelled(*args, **kwargs)

            def exception(self, *args, **kwargs):
                return self._task.exception(*args, **kwargs)

            def get_loop(self, *args, **kwargs):
                return self._task.get_loop(*args, **kwargs)

            def set_name(self, *args, **kwargs):
                return self._task.set_name(*args, **kwargs)

        async def main():
            interrupt_self()
            await asyncio.Event().wait()

        def new_event_loop():
            loop = self.new_loop()
            loop.set_task_factory(Task)
            return loop

        with self.assertRaises(asyncio.CancelledError):
            asyncio.run(main(), loop_factory=new_event_loop)

    def test_asyncio_run_loop_factory(self):
        factory = mock.Mock()
        loop = factory.return_value = self.new_loop()

        async def main():
            self.assertEqual(asyncio.get_running_loop(), loop)

        asyncio.run(main(), loop_factory=factory)
        factory.assert_called_once_with()

    def test_loop_factory_default_event_loop(self):
        async def main():
            if sys.platform == "win32":
                self.assertIsInstance(asyncio.get_running_loop(), asyncio.ProactorEventLoop)
            else:
                self.assertIsInstance(asyncio.get_running_loop(), asyncio.SelectorEventLoop)


        asyncio.run(main(), loop_factory=asyncio.EventLoop)


class RunnerTests(BaseTest):

    def test_non_debug(self):
        with asyncio.Runner(debug=False) as runner:
            self.assertFalse(runner.get_loop().get_debug())

    def test_debug(self):
        with asyncio.Runner(debug=True) as runner:
            self.assertTrue(runner.get_loop().get_debug())

    def test_custom_factory(self):
        loop = mock.Mock()
        with asyncio.Runner(loop_factory=lambda: loop) as runner:
            self.assertIs(runner.get_loop(), loop)

    def test_run(self):
        async def f():
            await asyncio.sleep(0)
            return 'done'

        with asyncio.Runner() as runner:
            self.assertEqual('done', runner.run(f()))
            loop = runner.get_loop()

        with self.assertRaisesRegex(
            RuntimeError,
            "Runner is closed"
        ):
            runner.get_loop()

        self.assertTrue(loop.is_closed())

    def test_run_non_coro(self):
        with asyncio.Runner() as runner:
            with self.assertRaisesRegex(
                TypeError,
                "an awaitable is required"
            ):
                runner.run(123)

    def test_run_future(self):
        with asyncio.Runner() as runner:
            fut = runner.get_loop().create_future()
            fut.set_result('done')
            self.assertEqual('done', runner.run(fut))

    def test_run_awaitable(self):
        class MyAwaitable:
            def __await__(self):
                return self.run().__await__()

            @staticmethod
            async def run():
                return 'done'

        with asyncio.Runner() as runner:
            self.assertEqual('done', runner.run(MyAwaitable()))

    def test_explicit_close(self):
        runner = asyncio.Runner()
        loop = runner.get_loop()
        runner.close()
        with self.assertRaisesRegex(
                RuntimeError,
                "Runner is closed"
        ):
            runner.get_loop()

        self.assertTrue(loop.is_closed())

    def test_double_close(self):
        runner = asyncio.Runner()
        loop = runner.get_loop()

        runner.close()
        self.assertTrue(loop.is_closed())

        # the second call is no-op
        runner.close()
        self.assertTrue(loop.is_closed())

    def test_second_with_block_raises(self):
        ret = []

        async def f(arg):
            ret.append(arg)

        runner = asyncio.Runner()
        with runner:
            runner.run(f(1))

        with self.assertRaisesRegex(
            RuntimeError,
            "Runner is closed"
        ):
            with runner:
                runner.run(f(2))

        self.assertEqual([1], ret)

    def test_run_keeps_context(self):
        cvar = contextvars.ContextVar("cvar", default=-1)

        async def f(val):
            old = cvar.get()
            await asyncio.sleep(0)
            cvar.set(val)
            return old

        async def get_context():
            return contextvars.copy_context()

        with asyncio.Runner() as runner:
            self.assertEqual(-1, runner.run(f(1)))
            self.assertEqual(1, runner.run(f(2)))

            self.assertEqual(2, runner.run(get_context()).get(cvar))

    def test_recursive_run(self):
        async def g():
            pass

        async def f():
            runner.run(g())

        with asyncio.Runner() as runner:
            with self.assertWarnsRegex(
                RuntimeWarning,
                "coroutine .+ was never awaited",
            ):
                with self.assertRaisesRegex(
                    RuntimeError,
                    re.escape(
                        "Runner.run() cannot be called from a running event loop"
                    ),
                ):
                    runner.run(f())

    def test_interrupt_call_soon(self):
        # The only case when task is not suspended by waiting a future
        # or another task
        assert threading.current_thread() is threading.main_thread()

        async def coro():
            with self.assertRaises(asyncio.CancelledError):
                while True:
                    await asyncio.sleep(0)
            raise asyncio.CancelledError()

        with asyncio.Runner() as runner:
            runner.get_loop().call_later(0.1, interrupt_self)
            with self.assertRaises(KeyboardInterrupt):
                runner.run(coro())

    def test_interrupt_wait(self):
        # interrupting when waiting a future cancels both future and main task
        assert threading.current_thread() is threading.main_thread()

        async def coro(fut):
            with self.assertRaises(asyncio.CancelledError):
                await fut
            raise asyncio.CancelledError()

        with asyncio.Runner() as runner:
            fut = runner.get_loop().create_future()
            runner.get_loop().call_later(0.1, interrupt_self)

            with self.assertRaises(KeyboardInterrupt):
                runner.run(coro(fut))

            self.assertTrue(fut.cancelled())

    def test_interrupt_cancelled_task(self):
        # interrupting cancelled main task doesn't raise KeyboardInterrupt
        assert threading.current_thread() is threading.main_thread()

        async def subtask(task):
            await asyncio.sleep(0)
            task.cancel()
            interrupt_self()

        async def coro():
            asyncio.create_task(subtask(asyncio.current_task()))
            await asyncio.sleep(10)

        with asyncio.Runner() as runner:
            with self.assertRaises(asyncio.CancelledError):
                runner.run(coro())

    def test_signal_install_not_supported_ok(self):
        # signal.signal() can throw if the "main thread" doesn't have signals enabled
        assert threading.current_thread() is threading.main_thread()

        async def coro():
            pass

        with asyncio.Runner() as runner:
            with patch.object(
                signal,
                "signal",
                side_effect=ValueError(
                    "signal only works in main thread of the main interpreter"
                )
            ):
                runner.run(coro())

    def test_set_event_loop_called_once(self):
        # See https://github.com/python/cpython/issues/95736
        async def coro():
            pass

        # The runner must call set_event_loop() exactly once even when
        # run() is called multiple times.  This only happens on the
        # implicit loop_factory path, so patch the default loop creation.
        with mock.patch('asyncio.events.new_event_loop', self.new_loop), \
                mock.patch('asyncio.runners.events.set_event_loop') as set_event_loop:
            runner = asyncio.Runner()
            runner.run(coro())
            runner.run(coro())

            self.assertEqual(1, set_event_loop.call_count)
            runner.close()

    def test_no_repr_is_call_on_the_task_result(self):
        # See https://github.com/python/cpython/issues/112559.
        class MyResult:
            def __init__(self):
                self.repr_count = 0
            def __repr__(self):
                self.repr_count += 1
                return super().__repr__()

        async def coro():
            return MyResult()


        with asyncio.Runner() as runner:
            result = runner.run(coro())

        self.assertEqual(0, result.repr_count)


if __name__ == '__main__':
    unittest.main()
