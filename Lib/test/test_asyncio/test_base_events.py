"""Tests for base_events.py"""

import concurrent.futures
import errno
import math
import platform
import socket
import sys
import threading
import time
import unittest
from unittest import mock

import asyncio
from asyncio import base_events
from asyncio import constants
from test.test_asyncio import utils as test_utils
from test import support
from test.support.script_helper import assert_python_ok
from test.support import os_helper
from test.support import socket_helper
import warnings

MOCK_ANY = mock.ANY


def tearDownModule():
    asyncio._set_event_loop_policy(None)


def mock_socket_module():
    m_socket = mock.MagicMock(spec=socket)
    for name in (
        'AF_INET', 'AF_INET6', 'AF_UNSPEC', 'IPPROTO_TCP', 'IPPROTO_UDP',
        'SOCK_STREAM', 'SOCK_DGRAM', 'SOL_SOCKET', 'SO_REUSEADDR', 'inet_pton'
    ):
        if hasattr(socket, name):
            setattr(m_socket, name, getattr(socket, name))
        else:
            delattr(m_socket, name)

    m_socket.socket = mock.MagicMock()
    m_socket.socket.return_value = test_utils.mock_nonblocking_socket()

    return m_socket


def patch_socket(f):
    return mock.patch('asyncio.base_events.socket',
                      new_callable=mock_socket_module)(f)


class BaseEventTests(test_utils.TestCase):

    def test_ipaddr_info(self):
        UNSPEC = socket.AF_UNSPEC
        INET = socket.AF_INET
        INET6 = socket.AF_INET6
        STREAM = socket.SOCK_STREAM
        DGRAM = socket.SOCK_DGRAM
        TCP = socket.IPPROTO_TCP
        UDP = socket.IPPROTO_UDP

        self.assertEqual(
            (INET, STREAM, TCP, '', ('1.2.3.4', 1)),
            base_events._ipaddr_info('1.2.3.4', 1, INET, STREAM, TCP))

        self.assertEqual(
            (INET, STREAM, TCP, '', ('1.2.3.4', 1)),
            base_events._ipaddr_info(b'1.2.3.4', 1, INET, STREAM, TCP))

        self.assertEqual(
            (INET, STREAM, TCP, '', ('1.2.3.4', 1)),
            base_events._ipaddr_info('1.2.3.4', 1, UNSPEC, STREAM, TCP))

        self.assertEqual(
            (INET, DGRAM, UDP, '', ('1.2.3.4', 1)),
            base_events._ipaddr_info('1.2.3.4', 1, UNSPEC, DGRAM, UDP))

        # Socket type STREAM implies TCP protocol.
        self.assertEqual(
            (INET, STREAM, TCP, '', ('1.2.3.4', 1)),
            base_events._ipaddr_info('1.2.3.4', 1, UNSPEC, STREAM, 0))

        # Socket type DGRAM implies UDP protocol.
        self.assertEqual(
            (INET, DGRAM, UDP, '', ('1.2.3.4', 1)),
            base_events._ipaddr_info('1.2.3.4', 1, UNSPEC, DGRAM, 0))

        # No socket type.
        self.assertIsNone(
            base_events._ipaddr_info('1.2.3.4', 1, UNSPEC, 0, 0))

        if socket_helper.IPV6_ENABLED:
            # IPv4 address with family IPv6.
            self.assertIsNone(
                base_events._ipaddr_info('1.2.3.4', 1, INET6, STREAM, TCP))

            self.assertEqual(
                (INET6, STREAM, TCP, '', ('::3', 1, 0, 0)),
                base_events._ipaddr_info('::3', 1, INET6, STREAM, TCP))

            self.assertEqual(
                (INET6, STREAM, TCP, '', ('::3', 1, 0, 0)),
                base_events._ipaddr_info('::3', 1, UNSPEC, STREAM, TCP))

            # IPv6 address with family IPv4.
            self.assertIsNone(
                base_events._ipaddr_info('::3', 1, INET, STREAM, TCP))

            # IPv6 address with zone index.
            self.assertIsNone(
                base_events._ipaddr_info('::3%lo0', 1, INET6, STREAM, TCP))

    def test_port_parameter_types(self):
        # Test obscure kinds of arguments for "port".
        INET = socket.AF_INET
        STREAM = socket.SOCK_STREAM
        TCP = socket.IPPROTO_TCP

        self.assertEqual(
            (INET, STREAM, TCP, '', ('1.2.3.4', 0)),
            base_events._ipaddr_info('1.2.3.4', None, INET, STREAM, TCP))

        self.assertEqual(
            (INET, STREAM, TCP, '', ('1.2.3.4', 0)),
            base_events._ipaddr_info('1.2.3.4', b'', INET, STREAM, TCP))

        self.assertEqual(
            (INET, STREAM, TCP, '', ('1.2.3.4', 0)),
            base_events._ipaddr_info('1.2.3.4', '', INET, STREAM, TCP))

        self.assertEqual(
            (INET, STREAM, TCP, '', ('1.2.3.4', 1)),
            base_events._ipaddr_info('1.2.3.4', '1', INET, STREAM, TCP))

        self.assertEqual(
            (INET, STREAM, TCP, '', ('1.2.3.4', 1)),
            base_events._ipaddr_info('1.2.3.4', b'1', INET, STREAM, TCP))

    @patch_socket
    def test_ipaddr_info_no_inet_pton(self, m_socket):
        del m_socket.inet_pton
        self.assertIsNone(base_events._ipaddr_info('1.2.3.4', 1,
                                                   socket.AF_INET,
                                                   socket.SOCK_STREAM,
                                                   socket.IPPROTO_TCP))


class BaseEventLoopTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = base_events.BaseEventLoop()
        self.loop._selector = mock.Mock()
        self.loop._selector.select.return_value = ()
        self.set_event_loop(self.loop)

    def test_not_implemented(self):
        m = mock.Mock()
        self.assertRaises(
            NotImplementedError,
            self.loop._make_socket_transport, m, m)
        self.assertRaises(
            NotImplementedError,
            self.loop._make_ssl_transport, m, m, m, m)
        self.assertRaises(
            NotImplementedError,
            self.loop._make_datagram_transport, m, m)
        self.assertRaises(
            NotImplementedError, self.loop._process_events, [])
        self.assertRaises(
            NotImplementedError, self.loop._write_to_self)
        self.assertRaises(
            NotImplementedError,
            self.loop._make_read_pipe_transport, m, m)
        self.assertRaises(
            NotImplementedError,
            self.loop._make_write_pipe_transport, m, m)
        gen = self.loop._make_subprocess_transport(m, m, m, m, m, m, m)
        with self.assertRaises(NotImplementedError):
            gen.send(None)

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

    def test_set_default_executor(self):
        class DummyExecutor(concurrent.futures.ThreadPoolExecutor):
            def submit(self, fn, *args, **kwargs):
                raise NotImplementedError(
                    'cannot submit into a dummy executor')

        self.loop._process_events = mock.Mock()
        self.loop._write_to_self = mock.Mock()

        executor = DummyExecutor()
        self.loop.set_default_executor(executor)
        self.assertIs(executor, self.loop._default_executor)

    def test_set_default_executor_error(self):
        executor = mock.Mock()

        msg = 'executor must be ThreadPoolExecutor instance'
        with self.assertRaisesRegex(TypeError, msg):
            self.loop.set_default_executor(executor)

        self.assertIsNone(self.loop._default_executor)

    def test_shutdown_default_executor_timeout(self):
        event = threading.Event()

        class DummyExecutor(concurrent.futures.ThreadPoolExecutor):
            def shutdown(self, wait=True, *, cancel_futures=False):
                if wait:
                    event.wait()

        self.loop._process_events = mock.Mock()
        self.loop._write_to_self = mock.Mock()
        executor = DummyExecutor()
        self.loop.set_default_executor(executor)

        try:
            with self.assertWarnsRegex(RuntimeWarning,
                                       "The executor did not finishing joining"):
                self.loop.run_until_complete(
                    self.loop.shutdown_default_executor(timeout=0.01))
        finally:
            event.set()

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
        delay = 0.100

        when = self.loop.time() + delay
        self.loop.call_at(when, cb)
        t0 = self.loop.time()
        self.loop.run_forever()
        dt = self.loop.time() - t0

        # 50 ms: maximum granularity of the event loop
        self.assertGreaterEqual(dt, delay - test_utils.CLOCK_RES)
        with self.assertRaises(TypeError, msg="when cannot be None"):
            self.loop.call_at(None, cb)

    def check_thread(self, loop, debug):
        def cb():
            pass

        loop.set_debug(debug)
        if debug:
            msg = ("Non-thread-safe operation invoked on an event loop other "
                   "than the current one")
            with self.assertRaisesRegex(RuntimeError, msg):
                loop.call_soon(cb)
            with self.assertRaisesRegex(RuntimeError, msg):
                loop.call_later(60, cb)
            with self.assertRaisesRegex(RuntimeError, msg):
                loop.call_at(loop.time() + 60, cb)
        else:
            loop.call_soon(cb)
            loop.call_later(60, cb)
            loop.call_at(loop.time() + 60, cb)

    def test_check_thread(self):
        def check_in_thread(loop, event, debug, create_loop, fut):
            # wait until the event loop is running
            event.wait()

            try:
                if create_loop:
                    loop2 = base_events.BaseEventLoop()
                    try:
                        asyncio.set_event_loop(loop2)
                        self.check_thread(loop, debug)
                    finally:
                        asyncio.set_event_loop(None)
                        loop2.close()
                else:
                    self.check_thread(loop, debug)
            except Exception as exc:
                loop.call_soon_threadsafe(fut.set_exception, exc)
            else:
                loop.call_soon_threadsafe(fut.set_result, None)

        def test_thread(loop, debug, create_loop=False):
            event = threading.Event()
            fut = loop.create_future()
            loop.call_soon(event.set)
            args = (loop, event, debug, create_loop, fut)
            thread = threading.Thread(target=check_in_thread, args=args)
            thread.start()
            loop.run_until_complete(fut)
            thread.join()

        self.loop._process_events = mock.Mock()
        self.loop._write_to_self = mock.Mock()

        # raise RuntimeError if the thread has no event loop
        test_thread(self.loop, True)

        # check disabled if debug mode is disabled
        test_thread(self.loop, False)

        # raise RuntimeError if the event loop of the thread is not the called
        # event loop
        test_thread(self.loop, True, create_loop=True)

        # check disabled if debug mode is disabled
        test_thread(self.loop, False, create_loop=True)

    def test__run_once(self):
        h1 = asyncio.TimerHandle(time.monotonic() + 5.0, lambda: True, (),
                                 self.loop, None)
        h2 = asyncio.TimerHandle(time.monotonic() + 10.0, lambda: True, (),
                                 self.loop, None)

        h1.cancel()

        self.loop._process_events = mock.Mock()
        self.loop._scheduled.append(h1)
        self.loop._scheduled.append(h2)
        self.loop._run_once()

        t = self.loop._selector.select.call_args[0][0]
        self.assertTrue(9.5 < t < 10.5, t)
        self.assertEqual([h2], self.loop._scheduled)
        self.assertTrue(self.loop._process_events.called)

    def test_set_debug(self):
        self.loop.set_debug(True)
        self.assertTrue(self.loop.get_debug())
        self.loop.set_debug(False)
        self.assertFalse(self.loop.get_debug())

    def test__run_once_schedule_handle(self):
        handle = None
        processed = False

        def cb(loop):
            nonlocal processed, handle
            processed = True
            handle = loop.call_soon(lambda: True)

        h = asyncio.TimerHandle(time.monotonic() - 1, cb, (self.loop,),
                                self.loop, None)

        self.loop._process_events = mock.Mock()
        self.loop._scheduled.append(h)
        self.loop._run_once()

        self.assertTrue(processed)
        self.assertEqual([handle], list(self.loop._ready))

    def test__run_once_cancelled_event_cleanup(self):
        self.loop._process_events = mock.Mock()

        self.assertTrue(
            0 < base_events._MIN_CANCELLED_TIMER_HANDLES_FRACTION < 1.0)

        def cb():
            pass

        # Set up one "blocking" event that will not be cancelled to
        # ensure later cancelled events do not make it to the head
        # of the queue and get cleaned.
        not_cancelled_count = 1
        self.loop.call_later(3000, cb)

        # Add less than threshold (base_events._MIN_SCHEDULED_TIMER_HANDLES)
        # cancelled handles, ensure they aren't removed

        cancelled_count = 2
        for x in range(2):
            h = self.loop.call_later(3600, cb)
            h.cancel()

        # Add some cancelled events that will be at head and removed
        cancelled_count += 2
        for x in range(2):
            h = self.loop.call_later(100, cb)
            h.cancel()

        # This test is invalid if _MIN_SCHEDULED_TIMER_HANDLES is too low
        self.assertLessEqual(cancelled_count + not_cancelled_count,
            base_events._MIN_SCHEDULED_TIMER_HANDLES)

        self.assertEqual(self.loop._timer_cancelled_count, cancelled_count)

        self.loop._run_once()

        cancelled_count -= 2

        self.assertEqual(self.loop._timer_cancelled_count, cancelled_count)

        self.assertEqual(len(self.loop._scheduled),
            cancelled_count + not_cancelled_count)

        # Need enough events to pass _MIN_CANCELLED_TIMER_HANDLES_FRACTION
        # so that deletion of cancelled events will occur on next _run_once
        add_cancel_count = int(math.ceil(
            base_events._MIN_SCHEDULED_TIMER_HANDLES *
            base_events._MIN_CANCELLED_TIMER_HANDLES_FRACTION)) + 1

        add_not_cancel_count = max(base_events._MIN_SCHEDULED_TIMER_HANDLES -
            add_cancel_count, 0)

        # Add some events that will not be cancelled
        not_cancelled_count += add_not_cancel_count
        for x in range(add_not_cancel_count):
            self.loop.call_later(3600, cb)

        # Add enough cancelled events
        cancelled_count += add_cancel_count
        for x in range(add_cancel_count):
            h = self.loop.call_later(3600, cb)
            h.cancel()

        # Ensure all handles are still scheduled
        self.assertEqual(len(self.loop._scheduled),
            cancelled_count + not_cancelled_count)

        self.loop._run_once()

        # Ensure cancelled events were removed
        self.assertEqual(len(self.loop._scheduled), not_cancelled_count)

        # Ensure only uncancelled events remain scheduled
        self.assertTrue(all([not x._cancelled for x in self.loop._scheduled]))

    def test_run_until_complete_type_error(self):
        self.assertRaises(TypeError,
            self.loop.run_until_complete, 'blah')

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

    def test_subprocess_exec_invalid_args(self):
        args = [sys.executable, '-c', 'pass']

        # missing program parameter (empty args)
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_exec,
            asyncio.SubprocessProtocol)

        # expected multiple arguments, not a list
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_exec,
            asyncio.SubprocessProtocol, args)

        # program arguments must be strings, not int
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_exec,
            asyncio.SubprocessProtocol, sys.executable, 123)

        # universal_newlines, shell, bufsize must not be set
        self.assertRaises(TypeError,
        self.loop.run_until_complete, self.loop.subprocess_exec,
            asyncio.SubprocessProtocol, *args, universal_newlines=True)
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_exec,
            asyncio.SubprocessProtocol, *args, shell=True)
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_exec,
            asyncio.SubprocessProtocol, *args, bufsize=4096)

    def test_subprocess_shell_invalid_args(self):
        # expected a string, not an int or a list
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_shell,
            asyncio.SubprocessProtocol, 123)
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_shell,
            asyncio.SubprocessProtocol, [sys.executable, '-c', 'pass'])

        # universal_newlines, shell, bufsize must not be set
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_shell,
            asyncio.SubprocessProtocol, 'exit 0', universal_newlines=True)
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_shell,
            asyncio.SubprocessProtocol, 'exit 0', shell=True)
        self.assertRaises(TypeError,
            self.loop.run_until_complete, self.loop.subprocess_shell,
            asyncio.SubprocessProtocol, 'exit 0', bufsize=4096)

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

    def test_set_task_factory_invalid(self):
        with self.assertRaisesRegex(
            TypeError, 'task factory must be a callable or None'):

            self.loop.set_task_factory(1)

        self.assertIsNone(self.loop.get_task_factory())

    def test_set_task_factory(self):
        self.loop._process_events = mock.Mock()

        class MyTask(asyncio.Task):
            pass

        async def coro():
            pass

        factory = lambda loop, coro: MyTask(coro, loop=loop)

        self.assertIsNone(self.loop.get_task_factory())
        self.loop.set_task_factory(factory)
        self.assertIs(self.loop.get_task_factory(), factory)

        task = self.loop.create_task(coro())
        self.assertTrue(isinstance(task, MyTask))
        self.loop.run_until_complete(task)

        self.loop.set_task_factory(None)
        self.assertIsNone(self.loop.get_task_factory())

        task = self.loop.create_task(coro())
        self.assertTrue(isinstance(task, asyncio.Task))
        self.assertFalse(isinstance(task, MyTask))
        self.loop.run_until_complete(task)

    def test_env_var_debug(self):
        code = '\n'.join((
            'import asyncio',
            'loop = asyncio.new_event_loop()',
            'print(loop.get_debug())'))

        # Test with -E to not fail if the unit test was run with
        # PYTHONASYNCIODEBUG set to a non-empty string
        sts, stdout, stderr = assert_python_ok('-E', '-c', code)
        self.assertEqual(stdout.rstrip(), b'False')

        sts, stdout, stderr = assert_python_ok('-c', code,
                                               PYTHONASYNCIODEBUG='',
                                               PYTHONDEVMODE='')
        self.assertEqual(stdout.rstrip(), b'False')

        sts, stdout, stderr = assert_python_ok('-c', code,
                                               PYTHONASYNCIODEBUG='1',
                                               PYTHONDEVMODE='')
        self.assertEqual(stdout.rstrip(), b'True')

        sts, stdout, stderr = assert_python_ok('-E', '-c', code,
                                               PYTHONASYNCIODEBUG='1')
        self.assertEqual(stdout.rstrip(), b'False')

        # -X dev
        sts, stdout, stderr = assert_python_ok('-E', '-X', 'dev',
                                               '-c', code)
        self.assertEqual(stdout.rstrip(), b'True')

    def test_create_task(self):
        class MyTask(asyncio.Task):
            pass

        async def test():
            pass

        class EventLoop(base_events.BaseEventLoop):
            def create_task(self, coro):
                return MyTask(coro, loop=loop)

        loop = EventLoop()
        self.set_event_loop(loop)

        coro = test()
        task = asyncio.ensure_future(coro, loop=loop)
        self.assertIsInstance(task, MyTask)

        # make warnings quiet
        task._log_destroy_pending = False
        coro.close()

    def test_create_task_error_closes_coro(self):
        async def test():
            pass
        loop = asyncio.new_event_loop()
        loop.close()
        with warnings.catch_warnings(record=True) as w:
            with self.assertRaises(RuntimeError):
                asyncio.ensure_future(test(), loop=loop)
            self.assertEqual(len(w), 0)


    def test_create_named_task_with_default_factory(self):
        async def test():
            pass

        loop = asyncio.new_event_loop()
        task = loop.create_task(test(), name='test_task')
        try:
            self.assertEqual(task.get_name(), 'test_task')
        finally:
            loop.run_until_complete(task)
            loop.close()

    def test_create_named_task_with_custom_factory(self):
        def task_factory(loop, coro, **kwargs):
            return asyncio.Task(coro, loop=loop, **kwargs)

        async def test():
            pass

        loop = asyncio.new_event_loop()
        loop.set_task_factory(task_factory)
        task = loop.create_task(test(), name='test_task')
        try:
            self.assertEqual(task.get_name(), 'test_task')
        finally:
            loop.run_until_complete(task)
            loop.close()

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

    def test_single_selecter_event_callback_after_stopping(self):
        # Python issue #25593: A stopped event loop may cause event callbacks
        # to run more than once.
        event_sentinel = object()
        callcount = 0
        doer = None

        def proc_events(event_list):
            nonlocal doer
            if event_sentinel in event_list:
                doer = self.loop.call_soon(do_event)

        def do_event():
            nonlocal callcount
            callcount += 1
            self.loop.call_soon(clear_selector)

        def clear_selector():
            doer.cancel()
            self.loop._selector.select.return_value = ()

        self.loop._process_events = proc_events
        self.loop._selector.select.return_value = (event_sentinel,)

        for i in range(1, 3):
            with self.subTest('Loop %d/2' % i):
                self.loop.call_soon(self.loop.stop)
                self.loop.run_forever()
                self.assertEqual(callcount, 1)

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

    def test_run_forever_pre_stopped(self):
        # Test that the old idiom for pre-stopping the loop works.
        self.loop._process_events = mock.Mock()
        self.loop.stop()
        self.loop.run_forever()
        self.loop._selector.select.assert_called_once_with(0)

    def test_custom_run_forever_integration(self):
        # Test that the run_forever_setup() and run_forever_cleanup() primitives
        # can be used to implement a custom run_forever loop.
        self.loop._process_events = mock.Mock()

        count = 0

        def callback():
            nonlocal count
            count += 1

        self.loop.call_soon(callback)

        # Set up the custom event loop
        self.loop._run_forever_setup()

        # Confirm the loop has been started
        self.assertEqual(asyncio.get_running_loop(), self.loop)
        self.assertTrue(self.loop.is_running())

        # Our custom "event loop" just iterates 10 times before exiting.
        for i in range(10):
            self.loop._run_once()

        # Clean up the event loop
        self.loop._run_forever_cleanup()

        # Confirm the loop has been cleaned up
        with self.assertRaises(RuntimeError):
            asyncio.get_running_loop()
        self.assertFalse(self.loop.is_running())

        # Confirm the loop actually did run, processing events 10 times,
        # and invoking the callback once.
        self.assertEqual(self.loop._process_events.call_count, 10)
        self.assertEqual(count, 1)

    async def leave_unfinalized_asyncgen(self):
        # Create an async generator, iterate it partially, and leave it
        # to be garbage collected.
        # Used in async generator finalization tests.
        # Depends on implementation details of garbage collector. Changes
        # in gc may break this function.
        status = {'started': False,
                  'stopped': False,
                  'finalized': False}

        async def agen():
            status['started'] = True
            try:
                for item in ['ZERO', 'ONE', 'TWO', 'THREE', 'FOUR']:
                    yield item
            finally:
                status['finalized'] = True

        ag = agen()
        ai = ag.__aiter__()

        async def iter_one():
            try:
                item = await ai.__anext__()
            except StopAsyncIteration:
                return
            if item == 'THREE':
                status['stopped'] = True
                return
            asyncio.create_task(iter_one())

        asyncio.create_task(iter_one())
        return status

    def test_asyncgen_finalization_by_gc(self):
        # Async generators should be finalized when garbage collected.
        self.loop._process_events = mock.Mock()
        self.loop._write_to_self = mock.Mock()
        with support.disable_gc():
            status = self.loop.run_until_complete(self.leave_unfinalized_asyncgen())
            while not status['stopped']:
                test_utils.run_briefly(self.loop)
            self.assertTrue(status['started'])
            self.assertTrue(status['stopped'])
            self.assertFalse(status['finalized'])
            support.gc_collect()
            test_utils.run_briefly(self.loop)
            self.assertTrue(status['finalized'])

    def test_asyncgen_finalization_by_gc_in_other_thread(self):
        # Python issue 34769: If garbage collector runs in another
        # thread, async generators will not finalize in debug
        # mode.
        self.loop._process_events = mock.Mock()
        self.loop._write_to_self = mock.Mock()
        self.loop.set_debug(True)
        with support.disable_gc():
            status = self.loop.run_until_complete(self.leave_unfinalized_asyncgen())
            while not status['stopped']:
                test_utils.run_briefly(self.loop)
            self.assertTrue(status['started'])
            self.assertTrue(status['stopped'])
            self.assertFalse(status['finalized'])
            self.loop.run_until_complete(
                self.loop.run_in_executor(None, support.gc_collect))
            test_utils.run_briefly(self.loop)
            self.assertTrue(status['finalized'])


class MyProto(asyncio.Protocol):
    done = None

    def __init__(self, create_future=False):
        self.state = 'INITIAL'
        self.nbytes = 0
        if create_future:
            self.done = asyncio.get_running_loop().create_future()

    def _assert_state(self, *expected):
        if self.state not in expected:
            raise AssertionError(f'state: {self.state!r}, expected: {expected!r}')

    def connection_made(self, transport):
        self.transport = transport
        self._assert_state('INITIAL')
        self.state = 'CONNECTED'
        transport.write(b'GET / HTTP/1.0\r\nHost: example.com\r\n\r\n')

    def data_received(self, data):
        self._assert_state('CONNECTED')
        self.nbytes += len(data)

    def eof_received(self):
        self._assert_state('CONNECTED')
        self.state = 'EOF'

    def connection_lost(self, exc):
        self._assert_state('CONNECTED', 'EOF')
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class MyDatagramProto(asyncio.DatagramProtocol):
    done = None

    def __init__(self, create_future=False, loop=None):
        self.state = 'INITIAL'
        self.nbytes = 0
        if create_future:
            self.done = loop.create_future()

    def _assert_state(self, expected):
        if self.state != expected:
            raise AssertionError(f'state: {self.state!r}, expected: {expected!r}')

    def connection_made(self, transport):
        self.transport = transport
        self._assert_state('INITIAL')
        self.state = 'INITIALIZED'

    def datagram_received(self, data, addr):
        self._assert_state('INITIALIZED')
        self.nbytes += len(data)

    def error_received(self, exc):
        self._assert_state('INITIALIZED')

    def connection_lost(self, exc):
        self._assert_state('INITIALIZED')
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class BaseEventLoopWithSelectorTests(test_utils.TestCase):

    def setUp(self):
        super().setUp()
        self.loop = asyncio.SelectorEventLoop()
        self.set_event_loop(self.loop)

    @mock.patch('socket.getnameinfo')
    def test_getnameinfo(self, m_gai):
        m_gai.side_effect = lambda *args: 42
        r = self.loop.run_until_complete(self.loop.getnameinfo(('abc', 123)))
        self.assertEqual(r, 42)

    @patch_socket
    def test_create_connection_multiple_errors(self, m_socket):

        class MyProto(asyncio.Protocol):
            pass

        async def getaddrinfo(*args, **kw):
            return [(2, 1, 6, '', ('107.6.106.82', 80)),
                    (2, 1, 6, '', ('107.6.106.82', 80))]

        def getaddrinfo_task(*args, **kwds):
            return self.loop.create_task(getaddrinfo(*args, **kwds))

        idx = -1
        errors = ['err1', 'err2']

        def _socket(*args, **kw):
            nonlocal idx, errors
            idx += 1
            raise OSError(errors[idx])

        m_socket.socket = _socket

        self.loop.getaddrinfo = getaddrinfo_task

        coro = self.loop.create_connection(MyProto, 'example.com', 80)
        with self.assertRaises(OSError) as cm:
            self.loop.run_until_complete(coro)

        self.assertEqual(str(cm.exception), 'Multiple exceptions: err1, err2')

        idx = -1
        coro = self.loop.create_connection(MyProto, 'example.com', 80, all_errors=True)
        with self.assertRaises(ExceptionGroup) as cm:
            self.loop.run_until_complete(coro)

        self.assertIsInstance(cm.exception, ExceptionGroup)
        for e in cm.exception.exceptions:
            self.assertIsInstance(e, OSError)

    @patch_socket
    def test_create_connection_timeout(self, m_socket):
        # Ensure that the socket is closed on timeout
        sock = mock.Mock()
        m_socket.socket.return_value = sock

        def getaddrinfo(*args, **kw):
            fut = self.loop.create_future()
            addr = (socket.AF_INET, socket.SOCK_STREAM, 0, '',
                    ('127.0.0.1', 80))
            fut.set_result([addr])
            return fut
        self.loop.getaddrinfo = getaddrinfo

        with mock.patch.object(self.loop, 'sock_connect',
                               side_effect=asyncio.TimeoutError):
            coro = self.loop.create_connection(MyProto, '127.0.0.1', 80)
            with self.assertRaises(asyncio.TimeoutError):
                self.loop.run_until_complete(coro)
            self.assertTrue(sock.close.called)

    def test_create_connection_host_port_sock(self):
        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, sock=object())
        self.assertRaises(ValueError, self.loop.run_until_complete, coro)

    def test_create_connection_wrong_sock(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        with sock:
            coro = self.loop.create_connection(MyProto, sock=sock)
            with self.assertRaisesRegex(ValueError,
                                        'A Stream Socket was expected'):
                self.loop.run_until_complete(coro)

    def test_create_server_wrong_sock(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        with sock:
            coro = self.loop.create_server(MyProto, sock=sock)
            with self.assertRaisesRegex(ValueError,
                                        'A Stream Socket was expected'):
                self.loop.run_until_complete(coro)

    def test_create_server_ssl_timeout_for_plain_socket(self):
        coro = self.loop.create_server(
            MyProto, 'example.com', 80, ssl_handshake_timeout=1)
        with self.assertRaisesRegex(
                ValueError,
                'ssl_handshake_timeout is only meaningful with ssl'):
            self.loop.run_until_complete(coro)

    @unittest.skipUnless(hasattr(socket, 'SOCK_NONBLOCK'),
                         'no socket.SOCK_NONBLOCK (linux only)')
    def test_create_server_stream_bittype(self):
        sock = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM | socket.SOCK_NONBLOCK)
        with sock:
            coro = self.loop.create_server(lambda: None, sock=sock)
            srv = self.loop.run_until_complete(coro)
            srv.close()
            self.loop.run_until_complete(srv.wait_closed())

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'no IPv6 support')
    def test_create_server_ipv6(self):
        async def main():
            srv = await asyncio.start_server(lambda: None, '::1', 0)
            try:
                self.assertGreater(len(srv.sockets), 0)
            finally:
                srv.close()
                await srv.wait_closed()

        try:
            self.loop.run_until_complete(main())
        except OSError as ex:
            if (hasattr(errno, 'EADDRNOTAVAIL') and
                    ex.errno == errno.EADDRNOTAVAIL):
                self.skipTest('failed to bind to ::1')
            else:
                raise

    def test_create_datagram_endpoint_wrong_sock(self):
        sock = socket.socket(socket.AF_INET)
        with sock:
            coro = self.loop.create_datagram_endpoint(MyProto, sock=sock)
            with self.assertRaisesRegex(ValueError,
                                        'A datagram socket was expected'):
                self.loop.run_until_complete(coro)

    def test_create_connection_no_host_port_sock(self):
        coro = self.loop.create_connection(MyProto)
        self.assertRaises(ValueError, self.loop.run_until_complete, coro)

    def test_create_connection_no_getaddrinfo(self):
        async def getaddrinfo(*args, **kw):
            return []

        def getaddrinfo_task(*args, **kwds):
            return self.loop.create_task(getaddrinfo(*args, **kwds))

        self.loop.getaddrinfo = getaddrinfo_task
        coro = self.loop.create_connection(MyProto, 'example.com', 80)
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    def test_create_connection_connect_err(self):
        async def getaddrinfo(*args, **kw):
            return [(2, 1, 6, '', ('107.6.106.82', 80))]

        def getaddrinfo_task(*args, **kwds):
            return self.loop.create_task(getaddrinfo(*args, **kwds))

        self.loop.getaddrinfo = getaddrinfo_task
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.side_effect = OSError

        coro = self.loop.create_connection(MyProto, 'example.com', 80)
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

        coro = self.loop.create_connection(MyProto, 'example.com', 80, all_errors=True)
        with self.assertRaises(ExceptionGroup) as cm:
            self.loop.run_until_complete(coro)

        self.assertIsInstance(cm.exception, ExceptionGroup)
        self.assertEqual(len(cm.exception.exceptions), 1)
        self.assertIsInstance(cm.exception.exceptions[0], OSError)

    def test_create_connection_multiple(self):
        async def getaddrinfo(*args, **kw):
            return [(2, 1, 6, '', ('0.0.0.1', 80)),
                    (2, 1, 6, '', ('0.0.0.2', 80))]

        def getaddrinfo_task(*args, **kwds):
            return self.loop.create_task(getaddrinfo(*args, **kwds))

        self.loop.getaddrinfo = getaddrinfo_task
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.side_effect = OSError

        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, family=socket.AF_INET)
        with self.assertRaises(OSError):
            self.loop.run_until_complete(coro)

        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, family=socket.AF_INET, all_errors=True)
        with self.assertRaises(ExceptionGroup) as cm:
            self.loop.run_until_complete(coro)

        self.assertIsInstance(cm.exception, ExceptionGroup)
        for e in cm.exception.exceptions:
            self.assertIsInstance(e, OSError)

    @patch_socket
    def test_create_connection_multiple_errors_local_addr(self, m_socket):

        def bind(addr):
            if addr[0] == '0.0.0.1':
                err = OSError('Err')
                err.strerror = 'Err'
                raise err

        m_socket.socket.return_value.bind = bind

        async def getaddrinfo(*args, **kw):
            return [(2, 1, 6, '', ('0.0.0.1', 80)),
                    (2, 1, 6, '', ('0.0.0.2', 80))]

        def getaddrinfo_task(*args, **kwds):
            return self.loop.create_task(getaddrinfo(*args, **kwds))

        self.loop.getaddrinfo = getaddrinfo_task
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.side_effect = OSError('Err2')

        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, family=socket.AF_INET,
            local_addr=(None, 8080))
        with self.assertRaises(OSError) as cm:
            self.loop.run_until_complete(coro)

        self.assertStartsWith(str(cm.exception), 'Multiple exceptions: ')
        self.assertTrue(m_socket.socket.return_value.close.called)

        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, family=socket.AF_INET,
            local_addr=(None, 8080), all_errors=True)
        with self.assertRaises(ExceptionGroup) as cm:
            self.loop.run_until_complete(coro)

        self.assertIsInstance(cm.exception, ExceptionGroup)
        for e in cm.exception.exceptions:
            self.assertIsInstance(e, OSError)

    def _test_create_connection_ip_addr(self, m_socket, allow_inet_pton):
        # Test the fallback code, even if this system has inet_pton.
        if not allow_inet_pton:
            del m_socket.inet_pton

        m_socket.getaddrinfo = socket.getaddrinfo
        sock = m_socket.socket.return_value

        self.loop._add_reader = mock.Mock()
        self.loop._add_writer = mock.Mock()

        coro = self.loop.create_connection(asyncio.Protocol, '1.2.3.4', 80)
        t, p = self.loop.run_until_complete(coro)
        try:
            sock.connect.assert_called_with(('1.2.3.4', 80))
            _, kwargs = m_socket.socket.call_args
            self.assertEqual(kwargs['family'], m_socket.AF_INET)
            self.assertEqual(kwargs['type'], m_socket.SOCK_STREAM)
        finally:
            t.close()
            test_utils.run_briefly(self.loop)  # allow transport to close

        if socket_helper.IPV6_ENABLED:
            sock.family = socket.AF_INET6
            coro = self.loop.create_connection(asyncio.Protocol, '::1', 80)
            t, p = self.loop.run_until_complete(coro)
            try:
                # Without inet_pton we use getaddrinfo, which transforms
                # ('::1', 80) to ('::1', 80, 0, 0). The last 0s are flow info,
                # scope id.
                [address] = sock.connect.call_args[0]
                host, port = address[:2]
                self.assertRegex(host, r'::(0\.)*1')
                self.assertEqual(port, 80)
                _, kwargs = m_socket.socket.call_args
                self.assertEqual(kwargs['family'], m_socket.AF_INET6)
                self.assertEqual(kwargs['type'], m_socket.SOCK_STREAM)
            finally:
                t.close()
                test_utils.run_briefly(self.loop)  # allow transport to close

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'no IPv6 support')
    @unittest.skipIf(sys.platform.startswith('aix'),
                    "bpo-25545: IPv6 scope id and getaddrinfo() behave differently on AIX")
    @patch_socket
    def test_create_connection_ipv6_scope(self, m_socket):
        m_socket.getaddrinfo = socket.getaddrinfo
        sock = m_socket.socket.return_value
        sock.family = socket.AF_INET6

        self.loop._add_reader = mock.Mock()
        self.loop._add_writer = mock.Mock()

        coro = self.loop.create_connection(asyncio.Protocol, 'fe80::1%1', 80)
        t, p = self.loop.run_until_complete(coro)
        try:
            sock.connect.assert_called_with(('fe80::1', 80, 0, 1))
            _, kwargs = m_socket.socket.call_args
            self.assertEqual(kwargs['family'], m_socket.AF_INET6)
            self.assertEqual(kwargs['type'], m_socket.SOCK_STREAM)
        finally:
            t.close()
            test_utils.run_briefly(self.loop)  # allow transport to close

    @patch_socket
    def test_create_connection_ip_addr(self, m_socket):
        self._test_create_connection_ip_addr(m_socket, True)

    @patch_socket
    def test_create_connection_no_inet_pton(self, m_socket):
        self._test_create_connection_ip_addr(m_socket, False)

    @patch_socket
    @unittest.skipIf(
        support.is_android and platform.android_ver().api_level < 23,
        "Issue gh-71123: this fails on Android before API level 23"
    )
    def test_create_connection_service_name(self, m_socket):
        m_socket.getaddrinfo = socket.getaddrinfo
        sock = m_socket.socket.return_value

        self.loop._add_reader = mock.Mock()
        self.loop._add_writer = mock.Mock()

        for service, port in ('http', 80), (b'http', 80):
            coro = self.loop.create_connection(asyncio.Protocol,
                                               '127.0.0.1', service)

            t, p = self.loop.run_until_complete(coro)
            try:
                sock.connect.assert_called_with(('127.0.0.1', port))
                _, kwargs = m_socket.socket.call_args
                self.assertEqual(kwargs['family'], m_socket.AF_INET)
                self.assertEqual(kwargs['type'], m_socket.SOCK_STREAM)
            finally:
                t.close()
                test_utils.run_briefly(self.loop)  # allow transport to close

        for service in 'nonsense', b'nonsense':
            coro = self.loop.create_connection(asyncio.Protocol,
                                               '127.0.0.1', service)

            with self.assertRaises(OSError):
                self.loop.run_until_complete(coro)

    def test_create_connection_no_local_addr(self):
        async def getaddrinfo(host, *args, **kw):
            if host == 'example.com':
                return [(2, 1, 6, '', ('107.6.106.82', 80)),
                        (2, 1, 6, '', ('107.6.106.82', 80))]
            else:
                return []

        def getaddrinfo_task(*args, **kwds):
            return self.loop.create_task(getaddrinfo(*args, **kwds))
        self.loop.getaddrinfo = getaddrinfo_task

        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, family=socket.AF_INET,
            local_addr=(None, 8080))
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    @patch_socket
    def test_create_connection_bluetooth(self, m_socket):
        # See http://bugs.python.org/issue27136, fallback to getaddrinfo when
        # we can't recognize an address is resolved, e.g. a Bluetooth address.
        addr = ('00:01:02:03:04:05', 1)

        def getaddrinfo(host, port, *args, **kw):
            self.assertEqual((host, port), addr)
            return [(999, 1, 999, '', (addr, 1))]

        m_socket.getaddrinfo = getaddrinfo
        sock = m_socket.socket()
        coro = self.loop.sock_connect(sock, addr)
        self.loop.run_until_complete(coro)

    def test_create_connection_ssl_server_hostname_default(self):
        self.loop.getaddrinfo = mock.Mock()

        def mock_getaddrinfo(*args, **kwds):
            f = self.loop.create_future()
            f.set_result([(socket.AF_INET, socket.SOCK_STREAM,
                           socket.SOL_TCP, '', ('1.2.3.4', 80))])
            return f

        self.loop.getaddrinfo.side_effect = mock_getaddrinfo
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.return_value = self.loop.create_future()
        self.loop.sock_connect.return_value.set_result(None)
        self.loop._make_ssl_transport = mock.Mock()

        class _SelectorTransportMock:
            _sock = None

            def get_extra_info(self, key):
                return mock.Mock()

            def close(self):
                self._sock.close()

        def mock_make_ssl_transport(sock, protocol, sslcontext, waiter,
                                    **kwds):
            waiter.set_result(None)
            transport = _SelectorTransportMock()
            transport._sock = sock
            return transport

        self.loop._make_ssl_transport.side_effect = mock_make_ssl_transport
        ANY = mock.ANY
        handshake_timeout = object()
        shutdown_timeout = object()
        # First try the default server_hostname.
        self.loop._make_ssl_transport.reset_mock()
        coro = self.loop.create_connection(
                MyProto, 'python.org', 80, ssl=True,
                ssl_handshake_timeout=handshake_timeout,
                ssl_shutdown_timeout=shutdown_timeout)
        transport, _ = self.loop.run_until_complete(coro)
        transport.close()
        self.loop._make_ssl_transport.assert_called_with(
            ANY, ANY, ANY, ANY,
            server_side=False,
            server_hostname='python.org',
            ssl_handshake_timeout=handshake_timeout,
            ssl_shutdown_timeout=shutdown_timeout)
        # Next try an explicit server_hostname.
        self.loop._make_ssl_transport.reset_mock()
        coro = self.loop.create_connection(
                MyProto, 'python.org', 80, ssl=True,
                server_hostname='perl.com',
                ssl_handshake_timeout=handshake_timeout,
                ssl_shutdown_timeout=shutdown_timeout)
        transport, _ = self.loop.run_until_complete(coro)
        transport.close()
        self.loop._make_ssl_transport.assert_called_with(
            ANY, ANY, ANY, ANY,
            server_side=False,
            server_hostname='perl.com',
            ssl_handshake_timeout=handshake_timeout,
            ssl_shutdown_timeout=shutdown_timeout)
        # Finally try an explicit empty server_hostname.
        self.loop._make_ssl_transport.reset_mock()
        coro = self.loop.create_connection(
                MyProto, 'python.org', 80, ssl=True,
                server_hostname='',
                ssl_handshake_timeout=handshake_timeout,
                ssl_shutdown_timeout=shutdown_timeout)
        transport, _ = self.loop.run_until_complete(coro)
        transport.close()
        self.loop._make_ssl_transport.assert_called_with(
                ANY, ANY, ANY, ANY,
                server_side=False,
                server_hostname='',
                ssl_handshake_timeout=handshake_timeout,
                ssl_shutdown_timeout=shutdown_timeout)

    def test_create_connection_no_ssl_server_hostname_errors(self):
        # When not using ssl, server_hostname must be None.
        coro = self.loop.create_connection(MyProto, 'python.org', 80,
                                           server_hostname='')
        self.assertRaises(ValueError, self.loop.run_until_complete, coro)
        coro = self.loop.create_connection(MyProto, 'python.org', 80,
                                           server_hostname='python.org')
        self.assertRaises(ValueError, self.loop.run_until_complete, coro)

    def test_create_connection_ssl_server_hostname_errors(self):
        # When using ssl, server_hostname may be None if host is non-empty.
        coro = self.loop.create_connection(MyProto, '', 80, ssl=True)
        self.assertRaises(ValueError, self.loop.run_until_complete, coro)
        coro = self.loop.create_connection(MyProto, None, 80, ssl=True)
        self.assertRaises(ValueError, self.loop.run_until_complete, coro)
        sock = socket.socket()
        coro = self.loop.create_connection(MyProto, None, None,
                                           ssl=True, sock=sock)
        self.addCleanup(sock.close)
        self.assertRaises(ValueError, self.loop.run_until_complete, coro)

    def test_create_connection_ssl_timeout_for_plain_socket(self):
        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, ssl_handshake_timeout=1)
        with self.assertRaisesRegex(
                ValueError,
                'ssl_handshake_timeout is only meaningful with ssl'):
            self.loop.run_until_complete(coro)

    def test_create_server_empty_host(self):
        # if host is empty string use None instead
        host = object()

        async def getaddrinfo(*args, **kw):
            nonlocal host
            host = args[0]
            return []

        def getaddrinfo_task(*args, **kwds):
            return self.loop.create_task(getaddrinfo(*args, **kwds))

        self.loop.getaddrinfo = getaddrinfo_task
        fut = self.loop.create_server(MyProto, '', 0)
        self.assertRaises(OSError, self.loop.run_until_complete, fut)
        self.assertIsNone(host)

    def test_create_server_host_port_sock(self):
        fut = self.loop.create_server(
            MyProto, '0.0.0.0', 0, sock=object())
        self.assertRaises(ValueError, self.loop.run_until_complete, fut)

    def test_create_server_no_host_port_sock(self):
        fut = self.loop.create_server(MyProto)
        self.assertRaises(ValueError, self.loop.run_until_complete, fut)

    def test_create_server_no_getaddrinfo(self):
        getaddrinfo = self.loop.getaddrinfo = mock.Mock()
        getaddrinfo.return_value = self.loop.create_future()
        getaddrinfo.return_value.set_result(None)

        f = self.loop.create_server(MyProto, 'python.org', 0)
        self.assertRaises(OSError, self.loop.run_until_complete, f)

    @patch_socket
    def test_create_server_nosoreuseport(self, m_socket):
        m_socket.getaddrinfo = socket.getaddrinfo
        del m_socket.SO_REUSEPORT
        m_socket.socket.return_value = mock.Mock()

        f = self.loop.create_server(
            MyProto, '0.0.0.0', 0, reuse_port=True)

        self.assertRaises(ValueError, self.loop.run_until_complete, f)

    @patch_socket
    def test_create_server_soreuseport_only_defined(self, m_socket):
        m_socket.getaddrinfo = socket.getaddrinfo
        m_socket.socket.return_value = mock.Mock()
        m_socket.SO_REUSEPORT = -1

        f = self.loop.create_server(
            MyProto, '0.0.0.0', 0, reuse_port=True)

        self.assertRaises(ValueError, self.loop.run_until_complete, f)

    @patch_socket
    def test_create_server_cant_bind(self, m_socket):

        class Err(OSError):
            strerror = 'error'

        m_socket.getaddrinfo.return_value = [
            (2, 1, 6, '', ('127.0.0.1', 10100))]
        m_sock = m_socket.socket.return_value = mock.Mock()
        m_sock.bind.side_effect = Err

        fut = self.loop.create_server(MyProto, '0.0.0.0', 0)
        self.assertRaises(OSError, self.loop.run_until_complete, fut)
        self.assertTrue(m_sock.close.called)

    @patch_socket
    def test_create_datagram_endpoint_no_addrinfo(self, m_socket):
        m_socket.getaddrinfo.return_value = []

        coro = self.loop.create_datagram_endpoint(
            MyDatagramProto, local_addr=('localhost', 0))
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    def test_create_datagram_endpoint_addr_error(self):
        coro = self.loop.create_datagram_endpoint(
            MyDatagramProto, local_addr='localhost')
        self.assertRaises(
            TypeError, self.loop.run_until_complete, coro)
        coro = self.loop.create_datagram_endpoint(
            MyDatagramProto, local_addr=('localhost', 1, 2, 3))
        self.assertRaises(
            TypeError, self.loop.run_until_complete, coro)

    def test_create_datagram_endpoint_connect_err(self):
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.side_effect = OSError

        coro = self.loop.create_datagram_endpoint(
            asyncio.DatagramProtocol, remote_addr=('127.0.0.1', 0))
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    def test_create_datagram_endpoint_allow_broadcast(self):
        protocol = MyDatagramProto(create_future=True, loop=self.loop)
        self.loop.sock_connect = sock_connect = mock.Mock()
        sock_connect.return_value = []

        coro = self.loop.create_datagram_endpoint(
            lambda: protocol,
            remote_addr=('127.0.0.1', 0),
            allow_broadcast=True)

        transport, _ = self.loop.run_until_complete(coro)
        self.assertFalse(sock_connect.called)

        transport.close()
        self.loop.run_until_complete(protocol.done)
        self.assertEqual('CLOSED', protocol.state)

    @patch_socket
    def test_create_datagram_endpoint_socket_err(self, m_socket):
        m_socket.getaddrinfo = socket.getaddrinfo
        m_socket.socket.side_effect = OSError

        coro = self.loop.create_datagram_endpoint(
            asyncio.DatagramProtocol, family=socket.AF_INET)
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

        coro = self.loop.create_datagram_endpoint(
            asyncio.DatagramProtocol, local_addr=('127.0.0.1', 0))
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 not supported or enabled')
    def test_create_datagram_endpoint_no_matching_family(self):
        coro = self.loop.create_datagram_endpoint(
            asyncio.DatagramProtocol,
            remote_addr=('127.0.0.1', 0), local_addr=('::1', 0))
        self.assertRaises(
            ValueError, self.loop.run_until_complete, coro)

    @patch_socket
    def test_create_datagram_endpoint_setblk_err(self, m_socket):
        m_socket.socket.return_value.setblocking.side_effect = OSError

        coro = self.loop.create_datagram_endpoint(
            asyncio.DatagramProtocol, family=socket.AF_INET)
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)
        self.assertTrue(
            m_socket.socket.return_value.close.called)

    def test_create_datagram_endpoint_noaddr_nofamily(self):
        coro = self.loop.create_datagram_endpoint(
            asyncio.DatagramProtocol)
        self.assertRaises(ValueError, self.loop.run_until_complete, coro)

    @patch_socket
    def test_create_datagram_endpoint_cant_bind(self, m_socket):
        class Err(OSError):
            pass

        m_socket.getaddrinfo = socket.getaddrinfo
        m_sock = m_socket.socket.return_value = mock.Mock()
        m_sock.bind.side_effect = Err

        fut = self.loop.create_datagram_endpoint(
            MyDatagramProto,
            local_addr=('127.0.0.1', 0), family=socket.AF_INET)
        self.assertRaises(Err, self.loop.run_until_complete, fut)
        self.assertTrue(m_sock.close.called)

    def test_create_datagram_endpoint_sock(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(('127.0.0.1', 0))
        fut = self.loop.create_datagram_endpoint(
            lambda: MyDatagramProto(create_future=True, loop=self.loop),
            sock=sock)
        transport, protocol = self.loop.run_until_complete(fut)
        transport.close()
        self.loop.run_until_complete(protocol.done)
        self.assertEqual('CLOSED', protocol.state)

    @unittest.skipUnless(hasattr(socket, 'AF_UNIX'), 'No UNIX Sockets')
    def test_create_datagram_endpoint_sock_unix(self):
        fut = self.loop.create_datagram_endpoint(
            lambda: MyDatagramProto(create_future=True, loop=self.loop),
            family=socket.AF_UNIX)
        transport, protocol = self.loop.run_until_complete(fut)
        self.assertEqual(transport._sock.family, socket.AF_UNIX)
        transport.close()
        self.loop.run_until_complete(protocol.done)
        self.assertEqual('CLOSED', protocol.state)

    @socket_helper.skip_unless_bind_unix_socket
    def test_create_datagram_endpoint_existing_sock_unix(self):
        with test_utils.unix_socket_path() as path:
            sock = socket.socket(socket.AF_UNIX, type=socket.SOCK_DGRAM)
            sock.bind(path)
            sock.close()

            coro = self.loop.create_datagram_endpoint(
                lambda: MyDatagramProto(create_future=True, loop=self.loop),
                path, family=socket.AF_UNIX)
            transport, protocol = self.loop.run_until_complete(coro)
            transport.close()
            self.loop.run_until_complete(protocol.done)

    def test_create_datagram_endpoint_sock_sockopts(self):
        class FakeSock:
            type = socket.SOCK_DGRAM

        fut = self.loop.create_datagram_endpoint(
            MyDatagramProto, local_addr=('127.0.0.1', 0), sock=FakeSock())
        self.assertRaises(ValueError, self.loop.run_until_complete, fut)

        fut = self.loop.create_datagram_endpoint(
            MyDatagramProto, remote_addr=('127.0.0.1', 0), sock=FakeSock())
        self.assertRaises(ValueError, self.loop.run_until_complete, fut)

        fut = self.loop.create_datagram_endpoint(
            MyDatagramProto, family=1, sock=FakeSock())
        self.assertRaises(ValueError, self.loop.run_until_complete, fut)

        fut = self.loop.create_datagram_endpoint(
            MyDatagramProto, proto=1, sock=FakeSock())
        self.assertRaises(ValueError, self.loop.run_until_complete, fut)

        fut = self.loop.create_datagram_endpoint(
            MyDatagramProto, flags=1, sock=FakeSock())
        self.assertRaises(ValueError, self.loop.run_until_complete, fut)

        fut = self.loop.create_datagram_endpoint(
            MyDatagramProto, reuse_port=True, sock=FakeSock())
        self.assertRaises(ValueError, self.loop.run_until_complete, fut)

        fut = self.loop.create_datagram_endpoint(
            MyDatagramProto, allow_broadcast=True, sock=FakeSock())
        self.assertRaises(ValueError, self.loop.run_until_complete, fut)

    @unittest.skipIf(sys.platform == 'vxworks',
                    "SO_BROADCAST is enabled by default on VxWorks")
    def test_create_datagram_endpoint_sockopts(self):
        # Socket options should not be applied unless asked for.
        # SO_REUSEPORT is not available on all platforms.

        coro = self.loop.create_datagram_endpoint(
            lambda: MyDatagramProto(create_future=True, loop=self.loop),
            local_addr=('127.0.0.1', 0))
        transport, protocol = self.loop.run_until_complete(coro)
        sock = transport.get_extra_info('socket')

        reuseport_supported = hasattr(socket, 'SO_REUSEPORT')

        if reuseport_supported:
            self.assertFalse(
                sock.getsockopt(
                    socket.SOL_SOCKET, socket.SO_REUSEPORT))
        self.assertFalse(
            sock.getsockopt(
                socket.SOL_SOCKET, socket.SO_BROADCAST))

        transport.close()
        self.loop.run_until_complete(protocol.done)
        self.assertEqual('CLOSED', protocol.state)

        coro = self.loop.create_datagram_endpoint(
            lambda: MyDatagramProto(create_future=True, loop=self.loop),
            local_addr=('127.0.0.1', 0),
            reuse_port=reuseport_supported,
            allow_broadcast=True)
        transport, protocol = self.loop.run_until_complete(coro)
        sock = transport.get_extra_info('socket')

        self.assertFalse(
            sock.getsockopt(
                socket.SOL_SOCKET, socket.SO_REUSEADDR))
        if reuseport_supported:
            self.assertTrue(
                sock.getsockopt(
                    socket.SOL_SOCKET, socket.SO_REUSEPORT))
        self.assertTrue(
            sock.getsockopt(
                socket.SOL_SOCKET, socket.SO_BROADCAST))

        transport.close()
        self.loop.run_until_complete(protocol.done)
        self.assertEqual('CLOSED', protocol.state)

    @patch_socket
    def test_create_datagram_endpoint_nosoreuseport(self, m_socket):
        del m_socket.SO_REUSEPORT
        m_socket.socket.return_value = mock.Mock()

        coro = self.loop.create_datagram_endpoint(
            lambda: MyDatagramProto(loop=self.loop),
            local_addr=('127.0.0.1', 0),
            reuse_port=True)

        self.assertRaises(ValueError, self.loop.run_until_complete, coro)

    @patch_socket
    def test_create_datagram_endpoint_ip_addr(self, m_socket):
        def getaddrinfo(*args, **kw):
            self.fail('should not have called getaddrinfo')

        m_socket.getaddrinfo = getaddrinfo
        m_socket.socket.return_value.bind = bind = mock.Mock()
        self.loop._add_reader = mock.Mock()

        reuseport_supported = hasattr(socket, 'SO_REUSEPORT')
        coro = self.loop.create_datagram_endpoint(
            lambda: MyDatagramProto(loop=self.loop),
            local_addr=('1.2.3.4', 0),
            reuse_port=reuseport_supported)

        t, p = self.loop.run_until_complete(coro)
        try:
            bind.assert_called_with(('1.2.3.4', 0))
            m_socket.socket.assert_called_with(family=m_socket.AF_INET,
                                               proto=m_socket.IPPROTO_UDP,
                                               type=m_socket.SOCK_DGRAM)
        finally:
            t.close()
            test_utils.run_briefly(self.loop)  # allow transport to close

    def test_accept_connection_retry(self):
        sock = mock.Mock()
        sock.accept.side_effect = BlockingIOError()

        self.loop._accept_connection(MyProto, sock)
        self.assertFalse(sock.close.called)

    @mock.patch('asyncio.base_events.logger')
    def test_accept_connection_exception(self, m_log):
        sock = mock.Mock()
        sock.fileno.return_value = 10
        sock.accept.side_effect = OSError(errno.EMFILE, 'Too many open files')
        self.loop._remove_reader = mock.Mock()
        self.loop.call_later = mock.Mock()

        self.loop._accept_connection(MyProto, sock)
        self.assertTrue(m_log.error.called)
        self.assertFalse(sock.close.called)
        self.loop._remove_reader.assert_called_with(10)
        self.loop.call_later.assert_called_with(
            constants.ACCEPT_RETRY_DELAY,
            # self.loop._start_serving
            mock.ANY,
            MyProto, sock, None, None, mock.ANY, mock.ANY, mock.ANY)

    def test_call_coroutine(self):
        async def simple_coroutine():
            pass

        self.loop.set_debug(True)
        coro_func = simple_coroutine
        coro_obj = coro_func()
        self.addCleanup(coro_obj.close)
        for func in (coro_func, coro_obj):
            with self.assertRaises(TypeError):
                self.loop.call_soon(func)
            with self.assertRaises(TypeError):
                self.loop.call_soon_threadsafe(func)
            with self.assertRaises(TypeError):
                self.loop.call_later(60, func)
            with self.assertRaises(TypeError):
                self.loop.call_at(self.loop.time() + 60, func)
            with self.assertRaises(TypeError):
                self.loop.run_until_complete(
                    self.loop.run_in_executor(None, func))

    @mock.patch('asyncio.base_events.logger')
    def test_log_slow_callbacks(self, m_logger):
        def stop_loop_cb(loop):
            loop.stop()

        async def stop_loop_coro(loop):
            loop.stop()

        asyncio.set_event_loop(self.loop)
        self.loop.set_debug(True)
        self.loop.slow_callback_duration = 0.0

        # slow callback
        self.loop.call_soon(stop_loop_cb, self.loop)
        self.loop.run_forever()
        fmt, *args = m_logger.warning.call_args[0]
        self.assertRegex(fmt % tuple(args),
                         "^Executing <Handle.*stop_loop_cb.*> "
                         "took .* seconds$")

        # slow task
        asyncio.ensure_future(stop_loop_coro(self.loop), loop=self.loop)
        self.loop.run_forever()
        fmt, *args = m_logger.warning.call_args[0]
        self.assertRegex(fmt % tuple(args),
                         "^Executing <Task.*stop_loop_coro.*> "
                         "took .* seconds$")


class RunningLoopTests(unittest.TestCase):

    def test_running_loop_within_a_loop(self):
        async def runner(loop):
            loop.run_forever()

        loop = asyncio.new_event_loop()
        outer_loop = asyncio.new_event_loop()
        try:
            with self.assertRaisesRegex(RuntimeError,
                                        'while another loop is running'):
                outer_loop.run_until_complete(runner(loop))
        finally:
            loop.close()
            outer_loop.close()


class BaseLoopSockSendfileTests(test_utils.TestCase):

    DATA = b"12345abcde" * 16 * 1024  # 160 KiB

    class MyProto(asyncio.Protocol):

        def __init__(self, loop):
            self.started = False
            self.closed = False
            self.data = bytearray()
            self.fut = loop.create_future()
            self.transport = None

        def connection_made(self, transport):
            self.started = True
            self.transport = transport

        def data_received(self, data):
            self.data.extend(data)

        def connection_lost(self, exc):
            self.closed = True
            self.fut.set_result(None)
            self.transport = None

        async def wait_closed(self):
            await self.fut

    @classmethod
    def setUpClass(cls):
        cls.__old_bufsize = constants.SENDFILE_FALLBACK_READBUFFER_SIZE
        constants.SENDFILE_FALLBACK_READBUFFER_SIZE = 1024 * 16
        with open(os_helper.TESTFN, 'wb') as fp:
            fp.write(cls.DATA)
        super().setUpClass()

    @classmethod
    def tearDownClass(cls):
        constants.SENDFILE_FALLBACK_READBUFFER_SIZE = cls.__old_bufsize
        os_helper.unlink(os_helper.TESTFN)
        super().tearDownClass()

    def setUp(self):
        from asyncio.selector_events import BaseSelectorEventLoop
        # BaseSelectorEventLoop() has no native implementation
        self.loop = BaseSelectorEventLoop()
        self.set_event_loop(self.loop)
        self.file = open(os_helper.TESTFN, 'rb')
        self.addCleanup(self.file.close)
        super().setUp()

    def make_socket(self, blocking=False):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setblocking(blocking)
        self.addCleanup(sock.close)
        return sock

    def run_loop(self, coro):
        return self.loop.run_until_complete(coro)

    def prepare(self):
        sock = self.make_socket()
        proto = self.MyProto(self.loop)
        server = self.run_loop(self.loop.create_server(
            lambda: proto, socket_helper.HOST, 0, family=socket.AF_INET))
        addr = server.sockets[0].getsockname()

        for _ in range(10):
            try:
                self.run_loop(self.loop.sock_connect(sock, addr))
            except OSError:
                self.run_loop(asyncio.sleep(0.5))
                continue
            else:
                break
        else:
            # One last try, so we get the exception
            self.run_loop(self.loop.sock_connect(sock, addr))

        def cleanup():
            server.close()
            sock.close()
            if proto.transport is not None:
                proto.transport.close()
                self.run_loop(proto.wait_closed())
            self.run_loop(server.wait_closed())

        self.addCleanup(cleanup)

        return sock, proto

    def test__sock_sendfile_native_failure(self):
        sock, proto = self.prepare()

        with self.assertRaisesRegex(asyncio.SendfileNotAvailableError,
                                    "sendfile is not available"):
            self.run_loop(self.loop._sock_sendfile_native(sock, self.file,
                                                          0, None))

        self.assertEqual(proto.data, b'')
        self.assertEqual(self.file.tell(), 0)

    def test_sock_sendfile_no_fallback(self):
        sock, proto = self.prepare()

        with self.assertRaisesRegex(asyncio.SendfileNotAvailableError,
                                    "sendfile is not available"):
            self.run_loop(self.loop.sock_sendfile(sock, self.file,
                                                  fallback=False))

        self.assertEqual(self.file.tell(), 0)
        self.assertEqual(proto.data, b'')

    def test_sock_sendfile_fallback(self):
        sock, proto = self.prepare()

        ret = self.run_loop(self.loop.sock_sendfile(sock, self.file))
        sock.close()
        self.run_loop(proto.wait_closed())

        self.assertEqual(ret, len(self.DATA))
        self.assertEqual(self.file.tell(), len(self.DATA))
        self.assertEqual(proto.data, self.DATA)

    def test_sock_sendfile_fallback_offset_and_count(self):
        sock, proto = self.prepare()

        ret = self.run_loop(self.loop.sock_sendfile(sock, self.file,
                                                    1000, 2000))
        sock.close()
        self.run_loop(proto.wait_closed())

        self.assertEqual(ret, 2000)
        self.assertEqual(self.file.tell(), 3000)
        self.assertEqual(proto.data, self.DATA[1000:3000])

    def test_blocking_socket(self):
        self.loop.set_debug(True)
        sock = self.make_socket(blocking=True)
        with self.assertRaisesRegex(ValueError, "must be non-blocking"):
            self.run_loop(self.loop.sock_sendfile(sock, self.file))

    def test_nonbinary_file(self):
        sock = self.make_socket()
        with open(os_helper.TESTFN, encoding="utf-8") as f:
            with self.assertRaisesRegex(ValueError, "binary mode"):
                self.run_loop(self.loop.sock_sendfile(sock, f))

    def test_nonstream_socket(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setblocking(False)
        self.addCleanup(sock.close)
        with self.assertRaisesRegex(ValueError, "only SOCK_STREAM type"):
            self.run_loop(self.loop.sock_sendfile(sock, self.file))

    def test_notint_count(self):
        sock = self.make_socket()
        with self.assertRaisesRegex(TypeError,
                                    "count must be a positive integer"):
            self.run_loop(self.loop.sock_sendfile(sock, self.file, 0, 'count'))

    def test_negative_count(self):
        sock = self.make_socket()
        with self.assertRaisesRegex(ValueError,
                                    "count must be a positive integer"):
            self.run_loop(self.loop.sock_sendfile(sock, self.file, 0, -1))

    def test_notint_offset(self):
        sock = self.make_socket()
        with self.assertRaisesRegex(TypeError,
                                    "offset must be a non-negative integer"):
            self.run_loop(self.loop.sock_sendfile(sock, self.file, 'offset'))

    def test_negative_offset(self):
        sock = self.make_socket()
        with self.assertRaisesRegex(ValueError,
                                    "offset must be a non-negative integer"):
            self.run_loop(self.loop.sock_sendfile(sock, self.file, -1))


class TestSelectorUtils(test_utils.TestCase):
    def check_set_nodelay(self, sock):
        opt = sock.getsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY)
        self.assertFalse(opt)

        base_events._set_nodelay(sock)

        opt = sock.getsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY)
        self.assertTrue(opt)

    @unittest.skipUnless(hasattr(socket, 'TCP_NODELAY'),
                         'need socket.TCP_NODELAY')
    def test_set_nodelay(self):
        sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM,
                             proto=socket.IPPROTO_TCP)
        with sock:
            self.check_set_nodelay(sock)

        sock = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM,
                             proto=socket.IPPROTO_TCP)
        with sock:
            sock.setblocking(False)
            self.check_set_nodelay(sock)



if __name__ == '__main__':
    unittest.main()
