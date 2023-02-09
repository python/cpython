# IsolatedAsyncioTestCase based tests
import asyncio
import contextvars
import traceback
import unittest
from asyncio import tasks


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class FutureTests:

    async def test_future_traceback(self):

        async def raise_exc():
            raise TypeError(42)

        future = self.cls(raise_exc())

        for _ in range(5):
            try:
                await future
            except TypeError as e:
                tb = ''.join(traceback.format_tb(e.__traceback__))
                self.assertEqual(tb.count("await future"), 1)
            else:
                self.fail('TypeError was not raised')

    async def test_task_exc_handler_correct_context(self):
        # see https://github.com/python/cpython/issues/96704
        name = contextvars.ContextVar('name', default='foo')
        exc_handler_called = False

        def exc_handler(*args):
            self.assertEqual(name.get(), 'bar')
            nonlocal exc_handler_called
            exc_handler_called = True

        async def task():
            name.set('bar')
            1/0

        loop = asyncio.get_running_loop()
        loop.set_exception_handler(exc_handler)
        self.cls(task())
        await asyncio.sleep(0)
        self.assertTrue(exc_handler_called)

    async def test_handle_exc_handler_correct_context(self):
        # see https://github.com/python/cpython/issues/96704
        name = contextvars.ContextVar('name', default='foo')
        exc_handler_called = False

        def exc_handler(*args):
            self.assertEqual(name.get(), 'bar')
            nonlocal exc_handler_called
            exc_handler_called = True

        def callback():
            name.set('bar')
            1/0

        loop = asyncio.get_running_loop()
        loop.set_exception_handler(exc_handler)
        loop.call_soon(callback)
        await asyncio.sleep(0)
        self.assertTrue(exc_handler_called)

@unittest.skipUnless(hasattr(tasks, '_CTask'),
                       'requires the C _asyncio module')
class CFutureTests(FutureTests, unittest.IsolatedAsyncioTestCase):
    cls = tasks._CTask

class PyFutureTests(FutureTests, unittest.IsolatedAsyncioTestCase):
    cls = tasks._PyTask

class FutureReprTests(unittest.IsolatedAsyncioTestCase):

    async def test_recursive_repr_for_pending_tasks(self):
        # The call crashes if the guard for recursive call
        # in base_futures:_future_repr_info is absent
        # See Also: https://bugs.python.org/issue42183

        async def func():
            return asyncio.all_tasks()

        # The repr() call should not raise RecursiveError at first.
        # The check for returned string is not very reliable but
        # exact comparison for the whole string is even weaker.
        self.assertIn('...', repr(await asyncio.wait_for(func(), timeout=10)))


if __name__ == '__main__':
    unittest.main()
