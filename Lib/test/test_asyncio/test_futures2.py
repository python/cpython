# IsolatedAsyncioTestCase based tests
import asyncio
import unittest


class FutureTests(unittest.IsolatedAsyncioTestCase):
    async def test_recursive_repr_for_pending_tasks(self):
        # The call crashes if the guard for recursive call
        # in base_futures:_future_repr_info is absent
        # See Also: https://bugs.python.org/issue42183

        async def func():
            return asyncio.all_tasks()

        self.assertIn('...', repr(await asyncio.wait_for(func(), timeout=10)))
