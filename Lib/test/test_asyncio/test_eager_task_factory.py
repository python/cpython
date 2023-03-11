"""Tests for base_events.py"""

import time
import unittest
from unittest import mock

import asyncio
from asyncio import base_events
from asyncio import tasks
from test.test_asyncio import utils as test_utils
from test import support

MOCK_ANY = mock.ANY


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class AsyncTaskCounter:
    def __init__(self, loop, *, task_class, eager):
        self.loop = loop
        self.suspense_count = 0
        self.task_count = 0

        def CountingTask(*args, **kwargs):
            self.task_count += 1
            return task_class(*args, **kwargs)

        if eager:
            factory = asyncio.create_eager_task_factory(CountingTask)
        else:
            def factory(loop, coro, **kwargs):
                return CountingTask(coro, loop=loop, **kwargs)
        self.loop.set_task_factory(factory)

    def get(self):
        return self.task_count


async def recursive_taskgroups(width, depth):
    if depth == 0:
        return 0

    async with asyncio.TaskGroup() as tg:
        futures = [
            tg.create_task(recursive_taskgroups(width, depth - 1))
            for _ in range(width)
        ]
    return sum(
        (1 if isinstance(fut, (asyncio.Task, tasks._CTask, tasks._PyTask)) else 0)
        + fut.result()
        for fut in futures
    )


async def recursive_gather(width, depth):
    if depth == 0:
        return

    await asyncio.gather(
        *[recursive_gather(width, depth - 1) for _ in range(width)]
    )


class EagerTaskFactoryLoopTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.loop.set_task_factory(asyncio.eager_task_factory)
        self.set_event_loop(self.loop)

    def test_eager_task_factory_set(self):
        self.assertIs(self.loop.get_task_factory(), asyncio.eager_task_factory)

    def test_tg_non_eager_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=asyncio.Task, eager=False)
        num_tasks = counter.loop.run_until_complete(recursive_taskgroups(5, 4))
        self.assertEqual(num_tasks, 780)  # 5 + 5^2 + 5^3 + 5^4
        self.assertEqual(counter.get(), 781)  # 1 + ^^

    def test_tg_eager_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=asyncio.Task, eager=True)
        num_tasks = counter.loop.run_until_complete(recursive_taskgroups(5, 4))
        self.assertEqual(num_tasks, 155)  # 5 + 5^2 + 5^3
        self.assertEqual(counter.get(), 156)  # 1 + ^^

    def test_gather_non_eager_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=asyncio.Task, eager=False)
        counter.loop.run_until_complete(recursive_gather(5, 4))
        self.assertEqual(counter.get(), 781)  # 1 + 5 + 5^2 + 5^3 + 5^4

    def test_gater_eager_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=asyncio.Task, eager=True)
        counter.loop.run_until_complete(recursive_gather(5, 4))
        self.assertEqual(counter.get(), 156)  # 1 + 5 + 5^2 + 5^3

    @unittest.skipUnless(hasattr(tasks, '_CTask'),
                         'requires the C _asyncio module')
    def test_tg_non_eager_ctask_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=tasks._CTask, eager=False)
        num_tasks = counter.loop.run_until_complete(recursive_taskgroups(5, 4))
        self.assertEqual(num_tasks, 780)  # 5 + 5^2 + 5^3 + 5^4
        self.assertEqual(counter.get(), 781)  # 1 + ^^

    def test_tg_non_eager_pytask_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=tasks._PyTask, eager=False)
        num_tasks = counter.loop.run_until_complete(recursive_taskgroups(5, 4))
        self.assertEqual(num_tasks, 780)  # 5 + 5^2 + 5^3 + 5^4
        self.assertEqual(counter.get(), 781)  # 1 + ^^

    @unittest.skipUnless(hasattr(tasks, '_CTask'),
                         'requires the C _asyncio module')
    def test_tg_eager_ctask_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=tasks._CTask, eager=True)
        num_tasks = counter.loop.run_until_complete(recursive_taskgroups(5, 4))
        self.assertEqual(num_tasks, 155)  # 5 + 5^2 + 5^3
        self.assertEqual(counter.get(), 156)  # 1 + ^^

    def test_tg_eager_pytask_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=tasks._PyTask, eager=True)
        num_tasks = counter.loop.run_until_complete(recursive_taskgroups(5, 4))
        self.assertEqual(num_tasks, 155)  # 5 + 5^2 + 5^3
        self.assertEqual(counter.get(), 156)  # 1 + ^^

    @unittest.skipUnless(hasattr(tasks, '_CTask'),
                         'requires the C _asyncio module')
    def test_gather_non_eager_ctask_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=tasks._CTask, eager=False)
        counter.loop.run_until_complete(recursive_gather(5, 4))
        self.assertEqual(counter.get(), 781)  # 1 + 5 + 5^2 + 5^3 + 5^4

    def test_gather_non_eager_pytask_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=tasks._PyTask, eager=False)
        counter.loop.run_until_complete(recursive_gather(5, 4))
        self.assertEqual(counter.get(), 781)  # 1 + 5 + 5^2 + 5^3 + 5^4

    @unittest.skipUnless(hasattr(tasks, '_CTask'),
                         'requires the C _asyncio module')
    def test_gater_eager_ctask_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=tasks._CTask, eager=True)
        counter.loop.run_until_complete(recursive_gather(5, 4))
        self.assertEqual(counter.get(), 156)  # 1 + 5 + 5^2 + 5^3

    def test_gater_eager_pytask_execution(self):
        counter = AsyncTaskCounter(self.loop, task_class=tasks._PyTask, eager=True)
        counter.loop.run_until_complete(recursive_gather(5, 4))
        self.assertEqual(counter.get(), 156)  # 1 + 5 + 5^2 + 5^3

    def test_close(self):
        self.assertFalse(self.loop.is_closed())
        self.loop.close()
        self.assertTrue(self.loop.is_closed())

        # it should be possible to call close() more than once
        self.loop.close()
        self.loop.close()

        # operation blocked when the loop is closed
        f = self.loop.create_future()
        self.assertRaises(RuntimeError, self.loop.run_forever)
        self.assertRaises(RuntimeError, self.loop.run_until_complete, f)

    def test__add_callback_handle(self):
        h = asyncio.Handle(lambda: False, (), self.loop, None)

        self.loop._add_callback(h)
        self.assertFalse(self.loop._scheduled)
        self.assertIn(h, self.loop._ready)

    def test__add_callback_cancelled_handle(self):
        h = asyncio.Handle(lambda: False, (), self.loop, None)
        h.cancel()

        self.loop._add_callback(h)
        self.assertFalse(self.loop._scheduled)
        self.assertFalse(self.loop._ready)

    def test_call_soon(self):
        def cb():
            pass

        h = self.loop.call_soon(cb)
        self.assertEqual(h._callback, cb)
        self.assertIsInstance(h, asyncio.Handle)
        self.assertIn(h, self.loop._ready)

    def test_call_soon_non_callable(self):
        self.loop.set_debug(True)
        with self.assertRaisesRegex(TypeError, 'a callable object'):
            self.loop.call_soon(1)

    def test_call_later(self):
        def cb():
            pass

        h = self.loop.call_later(10.0, cb)
        self.assertIsInstance(h, asyncio.TimerHandle)
        self.assertIn(h, self.loop._scheduled)
        self.assertNotIn(h, self.loop._ready)
        with self.assertRaises(TypeError, msg="delay must not be None"):
            self.loop.call_later(None, cb)

    def test_call_later_negative_delays(self):
        calls = []

        def cb(arg):
            calls.append(arg)

        self.loop._process_events = mock.Mock()
        self.loop.call_later(-1, cb, 'a')
        self.loop.call_later(-2, cb, 'b')
        test_utils.run_briefly(self.loop)
        self.assertEqual(calls, ['b', 'a'])

    def test_time_and_call_at(self):
        def cb():
            self.loop.stop()

        self.loop._process_events = mock.Mock()
        delay = 0.1

        when = self.loop.time() + delay
        self.loop.call_at(when, cb)
        t0 = self.loop.time()
        self.loop.run_forever()
        dt = self.loop.time() - t0

        # 50 ms: maximum granularity of the event loop
        self.assertGreaterEqual(dt, delay - 0.050, dt)
        # tolerate a difference of +800 ms because some Python buildbots
        # are really slow
        self.assertLessEqual(dt, 0.9, dt)
        with self.assertRaises(TypeError, msg="when cannot be None"):
            self.loop.call_at(None, cb)

    def test_run_until_complete_loop(self):
        task = self.loop.create_future()
        other_loop = self.new_test_loop()
        self.addCleanup(other_loop.close)
        self.assertRaises(ValueError,
            other_loop.run_until_complete, task)

    def test_run_until_complete_loop_orphan_future_close_loop(self):
        class ShowStopper(SystemExit):
            pass

        async def foo(delay):
            await asyncio.sleep(delay)

        def throw():
            raise ShowStopper

        self.loop._process_events = mock.Mock()
        self.loop.call_soon(throw)
        with self.assertRaises(ShowStopper):
            self.loop.run_until_complete(foo(0.1))

        # This call fails if run_until_complete does not clean up
        # done-callback for the previous future.
        self.loop.run_until_complete(foo(0.2))

    def test_default_exc_handler_callback(self):
        self.loop._process_events = mock.Mock()

        def zero_error(fut):
            fut.set_result(True)
            1/0

        # Test call_soon (events.Handle)
        with mock.patch('asyncio.base_events.logger') as log:
            fut = self.loop.create_future()
            self.loop.call_soon(zero_error, fut)
            fut.add_done_callback(lambda fut: self.loop.stop())
            self.loop.run_forever()
            log.error.assert_called_with(
                test_utils.MockPattern('Exception in callback.*zero'),
                exc_info=(ZeroDivisionError, MOCK_ANY, MOCK_ANY))

        # Test call_later (events.TimerHandle)
        with mock.patch('asyncio.base_events.logger') as log:
            fut = self.loop.create_future()
            self.loop.call_later(0.01, zero_error, fut)
            fut.add_done_callback(lambda fut: self.loop.stop())
            self.loop.run_forever()
            log.error.assert_called_with(
                test_utils.MockPattern('Exception in callback.*zero'),
                exc_info=(ZeroDivisionError, MOCK_ANY, MOCK_ANY))

    def test_default_exc_handler_coro(self):
        self.loop._process_events = mock.Mock()

        async def zero_error_coro():
            await asyncio.sleep(0.01)
            1/0

        # Test Future.__del__
        with mock.patch('asyncio.base_events.logger') as log:
            fut = asyncio.ensure_future(zero_error_coro(), loop=self.loop)
            fut.add_done_callback(lambda *args: self.loop.stop())
            self.loop.run_forever()
            fut = None # Trigger Future.__del__ or futures._TracebackLogger
            support.gc_collect()
            # Future.__del__ in logs error with an actual exception context
            log.error.assert_called_with(
                test_utils.MockPattern('.*exception was never retrieved'),
                exc_info=(ZeroDivisionError, MOCK_ANY, MOCK_ANY))

    def test_set_exc_handler_invalid(self):
        with self.assertRaisesRegex(TypeError, 'A callable object or None'):
            self.loop.set_exception_handler('spam')

    def test_set_exc_handler_custom(self):
        def zero_error():
            1/0

        def run_loop():
            handle = self.loop.call_soon(zero_error)
            self.loop._run_once()
            return handle

        self.loop.set_debug(True)
        self.loop._process_events = mock.Mock()

        self.assertIsNone(self.loop.get_exception_handler())
        mock_handler = mock.Mock()
        self.loop.set_exception_handler(mock_handler)
        self.assertIs(self.loop.get_exception_handler(), mock_handler)
        handle = run_loop()
        mock_handler.assert_called_with(self.loop, {
            'exception': MOCK_ANY,
            'message': test_utils.MockPattern(
                                'Exception in callback.*zero_error'),
            'handle': handle,
            'source_traceback': handle._source_traceback,
        })
        mock_handler.reset_mock()

        self.loop.set_exception_handler(None)
        with mock.patch('asyncio.base_events.logger') as log:
            run_loop()
            log.error.assert_called_with(
                        test_utils.MockPattern(
                                'Exception in callback.*zero'),
                        exc_info=(ZeroDivisionError, MOCK_ANY, MOCK_ANY))

        self.assertFalse(mock_handler.called)

    def test_set_exc_handler_broken(self):
        def run_loop():
            def zero_error():
                1/0
            self.loop.call_soon(zero_error)
            self.loop._run_once()

        def handler(loop, context):
            raise AttributeError('spam')

        self.loop._process_events = mock.Mock()

        self.loop.set_exception_handler(handler)

        with mock.patch('asyncio.base_events.logger') as log:
            run_loop()
            log.error.assert_called_with(
                test_utils.MockPattern(
                    'Unhandled error in exception handler'),
                exc_info=(AttributeError, MOCK_ANY, MOCK_ANY))

    def test_default_exc_handler_broken(self):
        _context = None

        class Loop(base_events.BaseEventLoop):

            _selector = mock.Mock()
            _process_events = mock.Mock()

            def default_exception_handler(self, context):
                nonlocal _context
                _context = context
                # Simulates custom buggy "default_exception_handler"
                raise ValueError('spam')

        loop = Loop()
        self.addCleanup(loop.close)
        asyncio.set_event_loop(loop)

        def run_loop():
            def zero_error():
                1/0
            loop.call_soon(zero_error)
            loop._run_once()

        with mock.patch('asyncio.base_events.logger') as log:
            run_loop()
            log.error.assert_called_with(
                'Exception in default exception handler',
                exc_info=True)

        def custom_handler(loop, context):
            raise ValueError('ham')

        _context = None
        loop.set_exception_handler(custom_handler)
        with mock.patch('asyncio.base_events.logger') as log:
            run_loop()
            log.error.assert_called_with(
                test_utils.MockPattern('Exception in default exception.*'
                                       'while handling.*in custom'),
                exc_info=True)

            # Check that original context was passed to default
            # exception handler.
            self.assertIn('context', _context)
            self.assertIs(type(_context['context']['exception']),
                          ZeroDivisionError)

    def test_eager_task_factory_with_custom_task_ctor(self):

        class MyTask(asyncio.Task):
            pass

        async def coro():
            pass

        factory = asyncio.create_eager_task_factory(MyTask)

        self.loop.set_task_factory(factory)
        self.assertIs(self.loop.get_task_factory(), factory)

        task = self.loop.create_task(coro())
        self.assertTrue(isinstance(task, MyTask))
        self.loop.run_until_complete(task)

    def test_create_named_task(self):
        async def test():
            pass

        task = self.loop.create_task(test(), name='test_task')
        try:
            self.assertEqual(task.get_name(), 'test_task')
        finally:
            self.loop.run_until_complete(task)

    def test_run_forever_keyboard_interrupt(self):
        # Python issue #22601: ensure that the temporary task created by
        # run_forever() consumes the KeyboardInterrupt and so don't log
        # a warning
        async def raise_keyboard_interrupt():
            raise KeyboardInterrupt

        self.loop._process_events = mock.Mock()
        self.loop.call_exception_handler = mock.Mock()

        try:
            self.loop.run_until_complete(raise_keyboard_interrupt())
        except KeyboardInterrupt:
            pass
        self.loop.close()
        support.gc_collect()

        self.assertFalse(self.loop.call_exception_handler.called)

    def test_run_until_complete_baseexception(self):
        # Python issue #22429: run_until_complete() must not schedule a pending
        # call to stop() if the future raised a BaseException
        async def raise_keyboard_interrupt():
            raise KeyboardInterrupt

        self.loop._process_events = mock.Mock()

        with self.assertRaises(KeyboardInterrupt):
            self.loop.run_until_complete(raise_keyboard_interrupt())

        def func():
            self.loop.stop()
            func.called = True
        func.called = False
        self.loop.call_soon(self.loop.call_soon, func)
        self.loop.run_forever()
        self.assertTrue(func.called)

    def test_run_once(self):
        # Simple test for test_utils.run_once().  It may seem strange
        # to have a test for this (the function isn't even used!) but
        # it's a de-factor standard API for library tests.  This tests
        # the idiom: loop.call_soon(loop.stop); loop.run_forever().
        count = 0

        def callback():
            nonlocal count
            count += 1

        self.loop._process_events = mock.Mock()
        self.loop.call_soon(callback)
        test_utils.run_once(self.loop)
        self.assertEqual(count, 1)


# class EagerTaskFactoryLoopTests(BaseEagerTaskFactoryLoopTests, test_utils.TestCase):
#     pass

if __name__ == '__main__':
    unittest.main()
