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

        # The repr() call should not raise RecursiveError at first.
        # The check for returned string is not very reliable but
        # exact comparison for the whole string is even weaker.
        self.assertIn('...', repr(await asyncio.wait_for(func(), timeout=10)))


if __name__ == '__main__':
    unittest.main()
