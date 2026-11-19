import asyncio
import sys
import unittest


class SubprocessWaitClosedTests(unittest.IsolatedAsyncioTestCase):

    async def test_cancel_one_stdin_close_waiter(self):
        loop = asyncio.get_running_loop()
        errors = []
        loop.set_exception_handler(lambda loop, context: errors.append(context))
        proc = await asyncio.create_subprocess_exec(
            sys.executable, "-c", "import sys; sys.stdin.buffer.read()",
            stdin=asyncio.subprocess.PIPE,
        )
        stdin = proc.stdin
        try:
            cancelled = asyncio.create_task(stdin.wait_closed())
            remaining = asyncio.create_task(stdin.wait_closed())
            await asyncio.sleep(0)
            cancelled.cancel()
            with self.assertRaises(asyncio.CancelledError):
                await cancelled

            stdin.close()
            await remaining
            await stdin.wait_closed()
        finally:
            stdin.close()
            await proc.wait()
        self.assertEqual(proc.returncode, 0)
        self.assertEqual(errors, [])
