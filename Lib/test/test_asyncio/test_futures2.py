# IsolatedAsyncioTestCase based tests
import asyncio
import inspect
import unittest
import traceback


def tearDownModule():
    asyncio.set_event_loop_policy(None)


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class FutureTests(unittest.IsolatedAsyncioTestCase):
    maxDiff = None

    async def test_future_traceback(self):
        # Note: the test is sensitive to line numbers.
        #
        # To make the test more flexible
        # and ready for future file modifications,
        # RAISE_MARKER and AWAIT_MARKER_<N> are
        # used for finding analyzed line numbers.

        markers = {}

        def init_markers():
            source_lines, offset = inspect.getsourcelines(
                FutureTests.test_future_traceback,
            )
            mark = "# pos: "
            for pos, line in enumerate(source_lines):
                found = line.find(mark)
                if found != -1:
                    marker = line[found + len(mark):].strip()
                    if marker == '"':
                        # skip 'mark' variable definition from above
                        continue
                    markers[marker] = offset + pos

        init_markers()

        def analyze_traceback(tb, await_marker):
            summary = traceback.extract_tb(tb)
            self.assertEqual(2, len(summary))
            self.assertEqual(markers[await_marker], summary[0].lineno)
            self.assertEqual(markers["RAISE_MARKER"], summary[1].lineno)

        async def raise_exc():
            raise TypeError(42)  # pos: RAISE_MARKER

        future = asyncio.create_task(raise_exc())
        # first await
        try:
            await future  # pos: AWAIT_MARKER_1
        except TypeError as exc:
            analyze_traceback(exc.__traceback__, "AWAIT_MARKER_1")
        else:
            self.fail('TypeError not raised')
        # the second await replaces traceback
        try:
            await future  # pos: AWAIT_MARKER_2
        except TypeError as exc:
            analyze_traceback(exc.__traceback__, "AWAIT_MARKER_2")
        else:
            self.fail('TypeError not raised')

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
