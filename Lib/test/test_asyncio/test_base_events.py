"""Tests for base_events.py"""

import errno
import logging
import math
import socket
import sys
import threading
import time
import unittest
from unittest import mock

import asyncio
from asyncio import base_events
from asyncio import constants
from asyncio import test_utils
try:
    from test import support
except ImportError:
    from asyncio import test_support as support
try:
    from test.support.script_helper import assert_python_ok
except ImportError:
    try:
        from test.script_helper import assert_python_ok
    except ImportError:
        from asyncio.test_support import assert_python_ok


MOCK_ANY = mock.ANY
PY34 = sys.version_info >= (3, 4)


class BaseEventLoopTests(test_utils.TestCase):

    def setUp(self):
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
        f = asyncio.Future(loop=self.loop)
        self.assertRaises(RuntimeError, self.loop.run_forever)
        self.assertRaises(RuntimeError, self.loop.run_until_complete, f)

    def test__add_callback_handle(self):
        h = asyncio.Handle(lambda: False, (), self.loop)

        self.loop._add_callback(h)
        self.assertFalse(self.loop._scheduled)
        self.assertIn(h, self.loop._ready)

    def test__add_callback_cancelled_handle(self):
        h = asyncio.Handle(lambda: False, (), self.loop)
        h.cancel()

        self.loop._add_callback(h)
        self.assertFalse(self.loop._scheduled)
        self.assertFalse(self.loop._ready)

    def test_set_default_executor(self):
        executor = mock.Mock()
        self.loop.set_default_executor(executor)
        self.assertIs(executor, self.loop._default_executor)

    def test_getnameinfo(self):
        sockaddr = mock.Mock()
        self.loop.run_in_executor = mock.Mock()
        self.loop.getnameinfo(sockaddr)
        self.assertEqual(
            (None, socket.getnameinfo, sockaddr, 0),
            self.loop.run_in_executor.call_args[0])

    def test_call_soon(self):
        def cb():
            pass

        h = self.loop.call_soon(cb)
        self.assertEqual(h._callback, cb)
        self.assertIsInstance(h, asyncio.Handle)
        self.assertIn(h, self.loop._ready)

    def test_call_later(self):
        def cb():
            pass

        h = self.loop.call_later(10.0, cb)
        self.assertIsInstance(h, asyncio.TimerHandle)
        self.assertIn(h, self.loop._scheduled)
        self.assertNotIn(h, self.loop._ready)

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
            fut = asyncio.Future(loop=loop)
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

    def test_run_once_in_executor_handle(self):
        def cb():
            pass

        self.assertRaises(
            AssertionError, self.loop.run_in_executor,
            None, asyncio.Handle(cb, (), self.loop), ('',))
        self.assertRaises(
            AssertionError, self.loop.run_in_executor,
            None, asyncio.TimerHandle(10, cb, (), self.loop))

    def test_run_once_in_executor_cancelled(self):
        def cb():
            pass
        h = asyncio.Handle(cb, (), self.loop)
        h.cancel()

        f = self.loop.run_in_executor(None, h)
        self.assertIsInstance(f, asyncio.Future)
        self.assertTrue(f.done())
        self.assertIsNone(f.result())

    def test_run_once_in_executor_plain(self):
        def cb():
            pass
        h = asyncio.Handle(cb, (), self.loop)
        f = asyncio.Future(loop=self.loop)
        executor = mock.Mock()
        executor.submit.return_value = f

        self.loop.set_default_executor(executor)

        res = self.loop.run_in_executor(None, h)
        self.assertIs(f, res)

        executor = mock.Mock()
        executor.submit.return_value = f
        res = self.loop.run_in_executor(executor, h)
        self.assertIs(f, res)
        self.assertTrue(executor.submit.called)

        f.cancel()  # Don't complain about abandoned Future.

    def test__run_once(self):
        h1 = asyncio.TimerHandle(time.monotonic() + 5.0, lambda: True, (),
                                 self.loop)
        h2 = asyncio.TimerHandle(time.monotonic() + 10.0, lambda: True, (),
                                 self.loop)

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

    @mock.patch('asyncio.base_events.logger')
    def test__run_once_logging(self, m_logger):
        def slow_select(timeout):
            # Sleep a bit longer than a second to avoid timer resolution
            # issues.
            time.sleep(1.1)
            return []

        # logging needs debug flag
        self.loop.set_debug(True)

        # Log to INFO level if timeout > 1.0 sec.
        self.loop._selector.select = slow_select
        self.loop._process_events = mock.Mock()
        self.loop._run_once()
        self.assertEqual(logging.INFO, m_logger.log.call_args[0][0])

        def fast_select(timeout):
            time.sleep(0.001)
            return []

        self.loop._selector.select = fast_select
        self.loop._run_once()
        self.assertEqual(logging.DEBUG, m_logger.log.call_args[0][0])

    def test__run_once_schedule_handle(self):
        handle = None
        processed = False

        def cb(loop):
            nonlocal processed, handle
            processed = True
            handle = loop.call_soon(lambda: True)

        h = asyncio.TimerHandle(time.monotonic() - 1, cb, (self.loop,),
                                self.loop)

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
        task = asyncio.Future(loop=self.loop)
        other_loop = self.new_test_loop()
        self.addCleanup(other_loop.close)
        self.assertRaises(ValueError,
            other_loop.run_until_complete, task)

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
            fut = asyncio.Future(loop=self.loop)
            self.loop.call_soon(zero_error, fut)
            fut.add_done_callback(lambda fut: self.loop.stop())
            self.loop.run_forever()
            log.error.assert_called_with(
                test_utils.MockPattern('Exception in callback.*zero'),
                exc_info=(ZeroDivisionError, MOCK_ANY, MOCK_ANY))

        # Test call_later (events.TimerHandle)
        with mock.patch('asyncio.base_events.logger') as log:
            fut = asyncio.Future(loop=self.loop)
            self.loop.call_later(0.01, zero_error, fut)
            fut.add_done_callback(lambda fut: self.loop.stop())
            self.loop.run_forever()
            log.error.assert_called_with(
                test_utils.MockPattern('Exception in callback.*zero'),
                exc_info=(ZeroDivisionError, MOCK_ANY, MOCK_ANY))

    def test_default_exc_handler_coro(self):
        self.loop._process_events = mock.Mock()

        @asyncio.coroutine
        def zero_error_coro():
            yield from asyncio.sleep(0.01, loop=self.loop)
            1/0

        # Test Future.__del__
        with mock.patch('asyncio.base_events.logger') as log:
            fut = asyncio.ensure_future(zero_error_coro(), loop=self.loop)
            fut.add_done_callback(lambda *args: self.loop.stop())
            self.loop.run_forever()
            fut = None # Trigger Future.__del__ or futures._TracebackLogger
            if PY34:
                # Future.__del__ in Python 3.4 logs error with
                # an actual exception context
                log.error.assert_called_with(
                    test_utils.MockPattern('.*exception was never retrieved'),
                    exc_info=(ZeroDivisionError, MOCK_ANY, MOCK_ANY))
            else:
                # futures._TracebackLogger logs only textual traceback
                log.error.assert_called_with(
                    test_utils.MockPattern(
                        '.*exception was never retrieved.*ZeroDiv'),
                    exc_info=False)

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

        mock_handler = mock.Mock()
        self.loop.set_exception_handler(mock_handler)
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

        assert not mock_handler.called

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

        @asyncio.coroutine
        def coro():
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
            'loop = asyncio.get_event_loop()',
            'print(loop.get_debug())'))

        # Test with -E to not fail if the unit test was run with
        # PYTHONASYNCIODEBUG set to a non-empty string
        sts, stdout, stderr = assert_python_ok('-E', '-c', code)
        self.assertEqual(stdout.rstrip(), b'False')

        sts, stdout, stderr = assert_python_ok('-c', code,
                                               PYTHONASYNCIODEBUG='')
        self.assertEqual(stdout.rstrip(), b'False')

        sts, stdout, stderr = assert_python_ok('-c', code,
                                               PYTHONASYNCIODEBUG='1')
        self.assertEqual(stdout.rstrip(), b'True')

        sts, stdout, stderr = assert_python_ok('-E', '-c', code,
                                               PYTHONASYNCIODEBUG='1')
        self.assertEqual(stdout.rstrip(), b'False')

    def test_create_task(self):
        class MyTask(asyncio.Task):
            pass

        @asyncio.coroutine
        def test():
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

    def test_run_forever_keyboard_interrupt(self):
        # Python issue #22601: ensure that the temporary task created by
        # run_forever() consumes the KeyboardInterrupt and so don't log
        # a warning
        @asyncio.coroutine
        def raise_keyboard_interrupt():
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
        @asyncio.coroutine
        def raise_keyboard_interrupt():
            raise KeyboardInterrupt

        self.loop._process_events = mock.Mock()

        try:
            self.loop.run_until_complete(raise_keyboard_interrupt())
        except KeyboardInterrupt:
            pass

        def func():
            self.loop.stop()
            func.called = True
        func.called = False
        try:
            self.loop.call_soon(func)
            self.loop.run_forever()
        except KeyboardInterrupt:
            pass
        self.assertTrue(func.called)


class MyProto(asyncio.Protocol):
    done = None

    def __init__(self, create_future=False):
        self.state = 'INITIAL'
        self.nbytes = 0
        if create_future:
            self.done = asyncio.Future()

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == 'INITIAL', self.state
        self.state = 'CONNECTED'
        transport.write(b'GET / HTTP/1.0\r\nHost: example.com\r\n\r\n')

    def data_received(self, data):
        assert self.state == 'CONNECTED', self.state
        self.nbytes += len(data)

    def eof_received(self):
        assert self.state == 'CONNECTED', self.state
        self.state = 'EOF'

    def connection_lost(self, exc):
        assert self.state in ('CONNECTED', 'EOF'), self.state
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class MyDatagramProto(asyncio.DatagramProtocol):
    done = None

    def __init__(self, create_future=False):
        self.state = 'INITIAL'
        self.nbytes = 0
        if create_future:
            self.done = asyncio.Future()

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == 'INITIAL', self.state
        self.state = 'INITIALIZED'

    def datagram_received(self, data, addr):
        assert self.state == 'INITIALIZED', self.state
        self.nbytes += len(data)

    def error_received(self, exc):
        assert self.state == 'INITIALIZED', self.state

    def connection_lost(self, exc):
        assert self.state == 'INITIALIZED', self.state
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class BaseEventLoopWithSelectorTests(test_utils.TestCase):

    def setUp(self):
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)

    @mock.patch('asyncio.base_events.socket')
    def test_create_connection_multiple_errors(self, m_socket):

        class MyProto(asyncio.Protocol):
            pass

        @asyncio.coroutine
        def getaddrinfo(*args, **kw):
            yield from []
            return [(2, 1, 6, '', ('107.6.106.82', 80)),
                    (2, 1, 6, '', ('107.6.106.82', 80))]

        def getaddrinfo_task(*args, **kwds):
            return asyncio.Task(getaddrinfo(*args, **kwds), loop=self.loop)

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

    @mock.patch('asyncio.base_events.socket')
    def test_create_connection_timeout(self, m_socket):
        # Ensure that the socket is closed on timeout
        sock = mock.Mock()
        m_socket.socket.return_value = sock

        def getaddrinfo(*args, **kw):
            fut = asyncio.Future(loop=self.loop)
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

    def test_create_connection_no_host_port_sock(self):
        coro = self.loop.create_connection(MyProto)
        self.assertRaises(ValueError, self.loop.run_until_complete, coro)

    def test_create_connection_no_getaddrinfo(self):
        @asyncio.coroutine
        def getaddrinfo(*args, **kw):
            yield from []

        def getaddrinfo_task(*args, **kwds):
            return asyncio.Task(getaddrinfo(*args, **kwds), loop=self.loop)

        self.loop.getaddrinfo = getaddrinfo_task
        coro = self.loop.create_connection(MyProto, 'example.com', 80)
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    def test_create_connection_connect_err(self):
        @asyncio.coroutine
        def getaddrinfo(*args, **kw):
            yield from []
            return [(2, 1, 6, '', ('107.6.106.82', 80))]

        def getaddrinfo_task(*args, **kwds):
            return asyncio.Task(getaddrinfo(*args, **kwds), loop=self.loop)

        self.loop.getaddrinfo = getaddrinfo_task
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.side_effect = OSError

        coro = self.loop.create_connection(MyProto, 'example.com', 80)
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    def test_create_connection_multiple(self):
        @asyncio.coroutine
        def getaddrinfo(*args, **kw):
            return [(2, 1, 6, '', ('0.0.0.1', 80)),
                    (2, 1, 6, '', ('0.0.0.2', 80))]

        def getaddrinfo_task(*args, **kwds):
            return asyncio.Task(getaddrinfo(*args, **kwds), loop=self.loop)

        self.loop.getaddrinfo = getaddrinfo_task
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.side_effect = OSError

        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, family=socket.AF_INET)
        with self.assertRaises(OSError):
            self.loop.run_until_complete(coro)

    @mock.patch('asyncio.base_events.socket')
    def test_create_connection_multiple_errors_local_addr(self, m_socket):

        def bind(addr):
            if addr[0] == '0.0.0.1':
                err = OSError('Err')
                err.strerror = 'Err'
                raise err

        m_socket.socket.return_value.bind = bind

        @asyncio.coroutine
        def getaddrinfo(*args, **kw):
            return [(2, 1, 6, '', ('0.0.0.1', 80)),
                    (2, 1, 6, '', ('0.0.0.2', 80))]

        def getaddrinfo_task(*args, **kwds):
            return asyncio.Task(getaddrinfo(*args, **kwds), loop=self.loop)

        self.loop.getaddrinfo = getaddrinfo_task
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.side_effect = OSError('Err2')

        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, family=socket.AF_INET,
            local_addr=(None, 8080))
        with self.assertRaises(OSError) as cm:
            self.loop.run_until_complete(coro)

        self.assertTrue(str(cm.exception).startswith('Multiple exceptions: '))
        self.assertTrue(m_socket.socket.return_value.close.called)

    def test_create_connection_no_local_addr(self):
        @asyncio.coroutine
        def getaddrinfo(host, *args, **kw):
            if host == 'example.com':
                return [(2, 1, 6, '', ('107.6.106.82', 80)),
                        (2, 1, 6, '', ('107.6.106.82', 80))]
            else:
                return []

        def getaddrinfo_task(*args, **kwds):
            return asyncio.Task(getaddrinfo(*args, **kwds), loop=self.loop)
        self.loop.getaddrinfo = getaddrinfo_task

        coro = self.loop.create_connection(
            MyProto, 'example.com', 80, family=socket.AF_INET,
            local_addr=(None, 8080))
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    def test_create_connection_ssl_server_hostname_default(self):
        self.loop.getaddrinfo = mock.Mock()

        def mock_getaddrinfo(*args, **kwds):
            f = asyncio.Future(loop=self.loop)
            f.set_result([(socket.AF_INET, socket.SOCK_STREAM,
                           socket.SOL_TCP, '', ('1.2.3.4', 80))])
            return f

        self.loop.getaddrinfo.side_effect = mock_getaddrinfo
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.return_value = ()
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
        # First try the default server_hostname.
        self.loop._make_ssl_transport.reset_mock()
        coro = self.loop.create_connection(MyProto, 'python.org', 80, ssl=True)
        transport, _ = self.loop.run_until_complete(coro)
        transport.close()
        self.loop._make_ssl_transport.assert_called_with(
            ANY, ANY, ANY, ANY,
            server_side=False,
            server_hostname='python.org')
        # Next try an explicit server_hostname.
        self.loop._make_ssl_transport.reset_mock()
        coro = self.loop.create_connection(MyProto, 'python.org', 80, ssl=True,
                                           server_hostname='perl.com')
        transport, _ = self.loop.run_until_complete(coro)
        transport.close()
        self.loop._make_ssl_transport.assert_called_with(
            ANY, ANY, ANY, ANY,
            server_side=False,
            server_hostname='perl.com')
        # Finally try an explicit empty server_hostname.
        self.loop._make_ssl_transport.reset_mock()
        coro = self.loop.create_connection(MyProto, 'python.org', 80, ssl=True,
                                           server_hostname='')
        transport, _ = self.loop.run_until_complete(coro)
        transport.close()
        self.loop._make_ssl_transport.assert_called_with(ANY, ANY, ANY, ANY,
                                                         server_side=False,
                                                         server_hostname='')

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

    def test_create_server_empty_host(self):
        # if host is empty string use None instead
        host = object()

        @asyncio.coroutine
        def getaddrinfo(*args, **kw):
            nonlocal host
            host = args[0]
            yield from []

        def getaddrinfo_task(*args, **kwds):
            return asyncio.Task(getaddrinfo(*args, **kwds), loop=self.loop)

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
        getaddrinfo.return_value = []

        f = self.loop.create_server(MyProto, '0.0.0.0', 0)
        self.assertRaises(OSError, self.loop.run_until_complete, f)

    @mock.patch('asyncio.base_events.socket')
    def test_create_server_cant_bind(self, m_socket):

        class Err(OSError):
            strerror = 'error'

        m_socket.getaddrinfo.return_value = [
            (2, 1, 6, '', ('127.0.0.1', 10100))]
        m_socket.getaddrinfo._is_coroutine = False
        m_sock = m_socket.socket.return_value = mock.Mock()
        m_sock.bind.side_effect = Err

        fut = self.loop.create_server(MyProto, '0.0.0.0', 0)
        self.assertRaises(OSError, self.loop.run_until_complete, fut)
        self.assertTrue(m_sock.close.called)

    @mock.patch('asyncio.base_events.socket')
    def test_create_datagram_endpoint_no_addrinfo(self, m_socket):
        m_socket.getaddrinfo.return_value = []
        m_socket.getaddrinfo._is_coroutine = False

        coro = self.loop.create_datagram_endpoint(
            MyDatagramProto, local_addr=('localhost', 0))
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    def test_create_datagram_endpoint_addr_error(self):
        coro = self.loop.create_datagram_endpoint(
            MyDatagramProto, local_addr='localhost')
        self.assertRaises(
            AssertionError, self.loop.run_until_complete, coro)
        coro = self.loop.create_datagram_endpoint(
            MyDatagramProto, local_addr=('localhost', 1, 2, 3))
        self.assertRaises(
            AssertionError, self.loop.run_until_complete, coro)

    def test_create_datagram_endpoint_connect_err(self):
        self.loop.sock_connect = mock.Mock()
        self.loop.sock_connect.side_effect = OSError

        coro = self.loop.create_datagram_endpoint(
            asyncio.DatagramProtocol, remote_addr=('127.0.0.1', 0))
        self.assertRaises(
            OSError, self.loop.run_until_complete, coro)

    @mock.patch('asyncio.base_events.socket')
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

    @unittest.skipUnless(support.IPV6_ENABLED, 'IPv6 not supported or enabled')
    def test_create_datagram_endpoint_no_matching_family(self):
        coro = self.loop.create_datagram_endpoint(
            asyncio.DatagramProtocol,
            remote_addr=('127.0.0.1', 0), local_addr=('::1', 0))
        self.assertRaises(
            ValueError, self.loop.run_until_complete, coro)

    @mock.patch('asyncio.base_events.socket')
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

    @mock.patch('asyncio.base_events.socket')
    def test_create_datagram_endpoint_cant_bind(self, m_socket):
        class Err(OSError):
            pass

        m_socket.AF_INET6 = socket.AF_INET6
        m_socket.getaddrinfo = socket.getaddrinfo
        m_sock = m_socket.socket.return_value = mock.Mock()
        m_sock.bind.side_effect = Err

        fut = self.loop.create_datagram_endpoint(
            MyDatagramProto,
            local_addr=('127.0.0.1', 0), family=socket.AF_INET)
        self.assertRaises(Err, self.loop.run_until_complete, fut)
        self.assertTrue(m_sock.close.called)

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
        self.loop.remove_reader = mock.Mock()
        self.loop.call_later = mock.Mock()

        self.loop._accept_connection(MyProto, sock)
        self.assertTrue(m_log.error.called)
        self.assertFalse(sock.close.called)
        self.loop.remove_reader.assert_called_with(10)
        self.loop.call_later.assert_called_with(constants.ACCEPT_RETRY_DELAY,
                                                # self.loop._start_serving
                                                mock.ANY,
                                                MyProto, sock, None, None)

    def test_call_coroutine(self):
        @asyncio.coroutine
        def simple_coroutine():
            pass

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
                self.loop.run_in_executor(None, func)

    @mock.patch('asyncio.base_events.logger')
    def test_log_slow_callbacks(self, m_logger):
        def stop_loop_cb(loop):
            loop.stop()

        @asyncio.coroutine
        def stop_loop_coro(loop):
            yield from ()
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


if __name__ == '__main__':
    unittest.main()
