import contextlib
import os
import sys
import unittest
from concurrent.futures.interpreter import ExecutionFailed
from test import support
from test.support.interpreters import queues

from .executor import ExecutorTest, mul
from .util import BaseTestCase, InterpreterPoolMixin, setup_module


def write_msg(fd, msg):
    os.write(fd, msg + b'\0')


def read_msg(fd):
    msg = b''
    while ch := os.read(fd, 1):
        if ch == b'\0':
            return msg
        msg += ch


def get_current_name():
    return __name__


class InterpreterPoolExecutorTest(InterpreterPoolMixin, ExecutorTest, BaseTestCase):

    def pipe(self):
        r, w = os.pipe()
        self.addCleanup(lambda: os.close(r))
        self.addCleanup(lambda: os.close(w))
        return r, w

    def assertTaskRaises(self, exctype):
        return self.assertRaisesRegex(ExecutionFailed, exctype.__name__)

    def test_init_func(self):
        msg = b'step: init'
        r, w = self.pipe()
        os.write(w, b'\0')

        executor = self.executor_type(
                initializer=write_msg, initargs=(w, msg))
        before = os.read(r, 100)
        executor.submit(mul, 10, 10)
        after = read_msg(r)

        self.assertEqual(before, b'\0')
        self.assertEqual(after, msg)

    def test_init_script(self):
        msg1 = b'step: init'
        msg2 = b'step: run'
        r, w = self.pipe()
        initscript = f"""if True:
            import os
            msg = {msg2!r}
            os.write({w}, {msg1!r} + b'\\0')
            """
        script = f"""if True:
            os.write({w}, msg + b'\\0')
            """
        os.write(w, b'\0')

        executor = self.executor_type(initializer=initscript)
        before_init = os.read(r, 100)
        fut = executor.submit(script)
        after_init = read_msg(r)
        write_msg(w, b'')
        before_run = read_msg(r)
        fut.result()
        after_run = read_msg(r)

        self.assertEqual(before_init, b'\0')
        self.assertEqual(after_init, msg1)
        self.assertEqual(before_run, b'')
        self.assertEqual(after_run, msg2)

    def test_init_script_args(self):
        with self.assertRaises(ValueError):
            self.executor_type(initializer='pass', initargs=('spam',))

    def test_init_shared(self):
        msg = b'eggs'
        r, w = self.pipe()
        script = f"""if True:
            import os
            os.write({w}, spam + b'\\0')
            """

        executor = self.executor_type(shared={'spam': msg})
        fut = executor.submit(script)
        fut.result()
        after = read_msg(r)

        self.assertEqual(after, msg)

    def test_submit_script(self):
        msg = b'spam'
        r, w = self.pipe()
        script = f"""if True:
            import os
            os.write({w}, __name__.encode('utf-8') + b'\\0')
            """
        executor = self.executor_type()

        fut = executor.submit(script)
        res = fut.result()
        after = read_msg(r)

        self.assertEqual(after, b'__main__')
        self.assertIs(res, None)

    def test_submit_func_globals(self):
        executor = self.executor_type()
        fut = executor.submit(get_current_name)
        name = fut.result()

        self.assertEqual(name, __name__)
        self.assertNotEqual(name, '__main__')

    def test_saturation(self):
        blocker = queues.create()
        executor = self.executor_type(4, shared=dict(blocker=blocker))

        for i in range(15 * executor._max_workers):
            executor.submit('blocker.get()')
        self.assertEqual(len(executor._threads), executor._max_workers)
        for i in range(15 * executor._max_workers):
            blocker.put_nowait(None)
        executor.shutdown(wait=True)

    @support.requires_gil_enabled("gh-117344: test is flaky without the GIL")
    def test_idle_thread_reuse(self):
        executor = self.executor_type()
        executor.submit(mul, 21, 2).result()
        executor.submit(mul, 6, 7).result()
        executor.submit(mul, 3, 14).result()
        self.assertEqual(len(executor._threads), 1)
        executor.shutdown(wait=True)


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
