"""Tests for futures.py."""

import concurrent.futures
import threading
import unittest
from unittest import mock

import asyncio
from asyncio import test_utils


def _fakefunc(f):
    return f


class FutureTests(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        asyncio.set_event_loop(None)

    def tearDown(self):
        self.loop.close()

    def test_initial_state(self):
        f = asyncio.Future(loop=self.loop)
        self.assertFalse(f.cancelled())
        self.assertFalse(f.done())
        f.cancel()
        self.assertTrue(f.cancelled())

    def test_init_constructor_default_loop(self):
        try:
            asyncio.set_event_loop(self.loop)
            f = asyncio.Future()
            self.assertIs(f._loop, self.loop)
        finally:
            asyncio.set_event_loop(None)

    def test_constructor_positional(self):
        # Make sure Future doesn't accept a positional argument
        self.assertRaises(TypeError, asyncio.Future, 42)

    def test_cancel(self):
        f = asyncio.Future(loop=self.loop)
        self.assertTrue(f.cancel())
        self.assertTrue(f.cancelled())
        self.assertTrue(f.done())
        self.assertRaises(asyncio.CancelledError, f.result)
        self.assertRaises(asyncio.CancelledError, f.exception)
        self.assertRaises(asyncio.InvalidStateError, f.set_result, None)
        self.assertRaises(asyncio.InvalidStateError, f.set_exception, None)
        self.assertFalse(f.cancel())

    def test_result(self):
        f = asyncio.Future(loop=self.loop)
        self.assertRaises(asyncio.InvalidStateError, f.result)

        f.set_result(42)
        self.assertFalse(f.cancelled())
        self.assertTrue(f.done())
        self.assertEqual(f.result(), 42)
        self.assertEqual(f.exception(), None)
        self.assertRaises(asyncio.InvalidStateError, f.set_result, None)
        self.assertRaises(asyncio.InvalidStateError, f.set_exception, None)
        self.assertFalse(f.cancel())

    def test_exception(self):
        exc = RuntimeError()
        f = asyncio.Future(loop=self.loop)
        self.assertRaises(asyncio.InvalidStateError, f.exception)

        f.set_exception(exc)
        self.assertFalse(f.cancelled())
        self.assertTrue(f.done())
        self.assertRaises(RuntimeError, f.result)
        self.assertEqual(f.exception(), exc)
        self.assertRaises(asyncio.InvalidStateError, f.set_result, None)
        self.assertRaises(asyncio.InvalidStateError, f.set_exception, None)
        self.assertFalse(f.cancel())

    def test_exception_class(self):
        f = asyncio.Future(loop=self.loop)
        f.set_exception(RuntimeError)
        self.assertIsInstance(f.exception(), RuntimeError)

    def test_yield_from_twice(self):
        f = asyncio.Future(loop=self.loop)

        def fixture():
            yield 'A'
            x = yield from f
            yield 'B', x
            y = yield from f
            yield 'C', y

        g = fixture()
        self.assertEqual(next(g), 'A')  # yield 'A'.
        self.assertEqual(next(g), f)  # First yield from f.
        f.set_result(42)
        self.assertEqual(next(g), ('B', 42))  # yield 'B', x.
        # The second "yield from f" does not yield f.
        self.assertEqual(next(g), ('C', 42))  # yield 'C', y.

    def test_repr(self):
        f_pending = asyncio.Future(loop=self.loop)
        self.assertEqual(repr(f_pending), 'Future<PENDING>')
        f_pending.cancel()

        f_cancelled = asyncio.Future(loop=self.loop)
        f_cancelled.cancel()
        self.assertEqual(repr(f_cancelled), 'Future<CANCELLED>')

        f_result = asyncio.Future(loop=self.loop)
        f_result.set_result(4)
        self.assertEqual(repr(f_result), 'Future<result=4>')
        self.assertEqual(f_result.result(), 4)

        exc = RuntimeError()
        f_exception = asyncio.Future(loop=self.loop)
        f_exception.set_exception(exc)
        self.assertEqual(repr(f_exception), 'Future<exception=RuntimeError()>')
        self.assertIs(f_exception.exception(), exc)

        f_few_callbacks = asyncio.Future(loop=self.loop)
        f_few_callbacks.add_done_callback(_fakefunc)
        self.assertIn('Future<PENDING, [<function _fakefunc',
                      repr(f_few_callbacks))
        f_few_callbacks.cancel()

        f_many_callbacks = asyncio.Future(loop=self.loop)
        for i in range(20):
            f_many_callbacks.add_done_callback(_fakefunc)
        r = repr(f_many_callbacks)
        self.assertIn('Future<PENDING, [<function _fakefunc', r)
        self.assertIn('<18 more>', r)
        f_many_callbacks.cancel()

    def test_copy_state(self):
        # Test the internal _copy_state method since it's being directly
        # invoked in other modules.
        f = asyncio.Future(loop=self.loop)
        f.set_result(10)

        newf = asyncio.Future(loop=self.loop)
        newf._copy_state(f)
        self.assertTrue(newf.done())
        self.assertEqual(newf.result(), 10)

        f_exception = asyncio.Future(loop=self.loop)
        f_exception.set_exception(RuntimeError())

        newf_exception = asyncio.Future(loop=self.loop)
        newf_exception._copy_state(f_exception)
        self.assertTrue(newf_exception.done())
        self.assertRaises(RuntimeError, newf_exception.result)

        f_cancelled = asyncio.Future(loop=self.loop)
        f_cancelled.cancel()

        newf_cancelled = asyncio.Future(loop=self.loop)
        newf_cancelled._copy_state(f_cancelled)
        self.assertTrue(newf_cancelled.cancelled())

    def test_iter(self):
        fut = asyncio.Future(loop=self.loop)

        def coro():
            yield from fut

        def test():
            arg1, arg2 = coro()

        self.assertRaises(AssertionError, test)
        fut.cancel()

    @mock.patch('asyncio.base_events.logger')
    def test_tb_logger_abandoned(self, m_log):
        fut = asyncio.Future(loop=self.loop)
        del fut
        self.assertFalse(m_log.error.called)

    @mock.patch('asyncio.base_events.logger')
    def test_tb_logger_result_unretrieved(self, m_log):
        fut = asyncio.Future(loop=self.loop)
        fut.set_result(42)
        del fut
        self.assertFalse(m_log.error.called)

    @mock.patch('asyncio.base_events.logger')
    def test_tb_logger_result_retrieved(self, m_log):
        fut = asyncio.Future(loop=self.loop)
        fut.set_result(42)
        fut.result()
        del fut
        self.assertFalse(m_log.error.called)

    @mock.patch('asyncio.base_events.logger')
    def test_tb_logger_exception_unretrieved(self, m_log):
        fut = asyncio.Future(loop=self.loop)
        fut.set_exception(RuntimeError('boom'))
        del fut
        test_utils.run_briefly(self.loop)
        self.assertTrue(m_log.error.called)

    @mock.patch('asyncio.base_events.logger')
    def test_tb_logger_exception_retrieved(self, m_log):
        fut = asyncio.Future(loop=self.loop)
        fut.set_exception(RuntimeError('boom'))
        fut.exception()
        del fut
        self.assertFalse(m_log.error.called)

    @mock.patch('asyncio.base_events.logger')
    def test_tb_logger_exception_result_retrieved(self, m_log):
        fut = asyncio.Future(loop=self.loop)
        fut.set_exception(RuntimeError('boom'))
        self.assertRaises(RuntimeError, fut.result)
        del fut
        self.assertFalse(m_log.error.called)

    def test_wrap_future(self):

        def run(arg):
            return (arg, threading.get_ident())
        ex = concurrent.futures.ThreadPoolExecutor(1)
        f1 = ex.submit(run, 'oi')
        f2 = asyncio.wrap_future(f1, loop=self.loop)
        res, ident = self.loop.run_until_complete(f2)
        self.assertIsInstance(f2, asyncio.Future)
        self.assertEqual(res, 'oi')
        self.assertNotEqual(ident, threading.get_ident())

    def test_wrap_future_future(self):
        f1 = asyncio.Future(loop=self.loop)
        f2 = asyncio.wrap_future(f1)
        self.assertIs(f1, f2)

    @mock.patch('asyncio.futures.events')
    def test_wrap_future_use_global_loop(self, m_events):
        def run(arg):
            return (arg, threading.get_ident())
        ex = concurrent.futures.ThreadPoolExecutor(1)
        f1 = ex.submit(run, 'oi')
        f2 = asyncio.wrap_future(f1)
        self.assertIs(m_events.get_event_loop.return_value, f2._loop)

    def test_wrap_future_cancel(self):
        f1 = concurrent.futures.Future()
        f2 = asyncio.wrap_future(f1, loop=self.loop)
        f2.cancel()
        test_utils.run_briefly(self.loop)
        self.assertTrue(f1.cancelled())
        self.assertTrue(f2.cancelled())

    def test_wrap_future_cancel2(self):
        f1 = concurrent.futures.Future()
        f2 = asyncio.wrap_future(f1, loop=self.loop)
        f1.set_result(42)
        f2.cancel()
        test_utils.run_briefly(self.loop)
        self.assertFalse(f1.cancelled())
        self.assertEqual(f1.result(), 42)
        self.assertTrue(f2.cancelled())


class FutureDoneCallbackTests(unittest.TestCase):

    def setUp(self):
        self.loop = test_utils.TestLoop()
        asyncio.set_event_loop(None)

    def tearDown(self):
        self.loop.close()

    def run_briefly(self):
        test_utils.run_briefly(self.loop)

    def _make_callback(self, bag, thing):
        # Create a callback function that appends thing to bag.
        def bag_appender(future):
            bag.append(thing)
        return bag_appender

    def _new_future(self):
        return asyncio.Future(loop=self.loop)

    def test_callbacks_invoked_on_set_result(self):
        bag = []
        f = self._new_future()
        f.add_done_callback(self._make_callback(bag, 42))
        f.add_done_callback(self._make_callback(bag, 17))

        self.assertEqual(bag, [])
        f.set_result('foo')

        self.run_briefly()

        self.assertEqual(bag, [42, 17])
        self.assertEqual(f.result(), 'foo')

    def test_callbacks_invoked_on_set_exception(self):
        bag = []
        f = self._new_future()
        f.add_done_callback(self._make_callback(bag, 100))

        self.assertEqual(bag, [])
        exc = RuntimeError()
        f.set_exception(exc)

        self.run_briefly()

        self.assertEqual(bag, [100])
        self.assertEqual(f.exception(), exc)

    def test_remove_done_callback(self):
        bag = []
        f = self._new_future()
        cb1 = self._make_callback(bag, 1)
        cb2 = self._make_callback(bag, 2)
        cb3 = self._make_callback(bag, 3)

        # Add one cb1 and one cb2.
        f.add_done_callback(cb1)
        f.add_done_callback(cb2)

        # One instance of cb2 removed. Now there's only one cb1.
        self.assertEqual(f.remove_done_callback(cb2), 1)

        # Never had any cb3 in there.
        self.assertEqual(f.remove_done_callback(cb3), 0)

        # After this there will be 6 instances of cb1 and one of cb2.
        f.add_done_callback(cb2)
        for i in range(5):
            f.add_done_callback(cb1)

        # Remove all instances of cb1. One cb2 remains.
        self.assertEqual(f.remove_done_callback(cb1), 6)

        self.assertEqual(bag, [])
        f.set_result('foo')

        self.run_briefly()

        self.assertEqual(bag, [2])
        self.assertEqual(f.result(), 'foo')


if __name__ == '__main__':
    unittest.main()
