import contextlib
import io
import os
import pickle
import sys
import unittest
from concurrent.futures.interpreter import (
    ExecutionFailed, BrokenInterpreterPool,
)
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


def fail(exctype, msg=None):
    raise exctype(msg)


class InterpreterPoolExecutorTest(InterpreterPoolMixin, ExecutorTest, BaseTestCase):

    def pipe(self):
        r, w = os.pipe()
        self.addCleanup(lambda: os.close(r))
        self.addCleanup(lambda: os.close(w))
        return r, w

    def test_init_script(self):
        msg1 = b'step: init'
        msg2 = b'step: run'
        r, w = self.pipe()
        initscript = f"""
            import os
            msg = {msg2!r}
            os.write({w}, {msg1!r} + b'\\0')
            """
        script = f"""
            os.write({w}, msg + b'\\0')
            """
        os.write(w, b'\0')

        executor = self.executor_type(initializer=initscript)
        before_init = os.read(r, 100)
        fut = executor.submit(script)
        after_init = read_msg(r)
        fut.result()
        after_run = read_msg(r)

        self.assertEqual(before_init, b'\0')
        self.assertEqual(after_init, msg1)
        self.assertEqual(after_run, msg2)

    def test_init_script_args(self):
        with self.assertRaises(ValueError):
            self.executor_type(initializer='pass', initargs=('spam',))

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

    def test_init_closure(self):
        count = 0
        def init1():
            assert count == 0, count
        def init2():
            nonlocal count
            count += 1

        with self.assertRaises(pickle.PicklingError):
            self.executor_type(initializer=init1)
        with self.assertRaises(pickle.PicklingError):
            self.executor_type(initializer=init2)

    def test_init_instance_method(self):
        class Spam:
            def initializer(self):
                raise NotImplementedError
        spam = Spam()

        with self.assertRaises(pickle.PicklingError):
            self.executor_type(initializer=spam.initializer)

    def test_init_shared(self):
        msg = b'eggs'
        r, w = self.pipe()
        script = f"""
            import os
            os.write({w}, spam + b'\\0')
            """

        executor = self.executor_type(shared={'spam': msg})
        fut = executor.submit(script)
        fut.result()
        after = read_msg(r)

        self.assertEqual(after, msg)

    def test_init_exception(self):
        with self.subTest('script'):
            executor = self.executor_type(initializer='raise Exception("spam")')
            with executor:
                with contextlib.redirect_stderr(io.StringIO()) as stderr:
                    fut = executor.submit('pass')
                    with self.assertRaises(BrokenInterpreterPool):
                        fut.result()
            stderr = stderr.getvalue()
            self.assertIn('ExecutionFailed: Exception: spam', stderr)
            self.assertIn('Uncaught in the interpreter:', stderr)
            self.assertIn('The above exception was the direct cause of the following exception:',
                          stderr)

        with self.subTest('func'):
            executor = self.executor_type(initializer=fail,
                                          initargs=(Exception, 'spam'))
            with executor:
                with contextlib.redirect_stderr(io.StringIO()) as stderr:
                    fut = executor.submit('pass')
                    with self.assertRaises(BrokenInterpreterPool):
                        fut.result()
            stderr = stderr.getvalue()
            self.assertIn('ExecutionFailed: Exception: spam', stderr)
            self.assertIn('Uncaught in the interpreter:', stderr)
            self.assertIn('The above exception was the direct cause of the following exception:',
                          stderr)

    def test_submit_script(self):
        msg = b'spam'
        r, w = self.pipe()
        script = f"""
            import os
            os.write({w}, __name__.encode('utf-8') + b'\\0')
            """
        executor = self.executor_type()

        fut = executor.submit(script)
        res = fut.result()
        after = read_msg(r)

        self.assertEqual(after, b'__main__')
        self.assertIs(res, None)

    def test_submit_closure(self):
        spam = True
        def task1():
            return spam
        def task2():
            nonlocal spam
            spam += 1
            return spam

        executor = self.executor_type()
        with self.assertRaises(pickle.PicklingError):
            executor.submit(task1)
        with self.assertRaises(pickle.PicklingError):
            executor.submit(task2)

    def test_submit_local_instance(self):
        class Spam:
            def __init__(self):
                self.value = True

        executor = self.executor_type()
        with self.assertRaises(pickle.PicklingError):
            executor.submit(Spam)

    def test_submit_instance_method(self):
        class Spam:
            def run(self):
                return True
        spam = Spam()

        executor = self.executor_type()
        with self.assertRaises(pickle.PicklingError):
            executor.submit(spam.run)

    def test_submit_func_globals(self):
        executor = self.executor_type()
        fut = executor.submit(get_current_name)
        name = fut.result()

        self.assertEqual(name, __name__)
        self.assertNotEqual(name, '__main__')

    def test_submit_exception(self):
        with self.subTest('script'):
            fut = self.executor.submit('raise Exception("spam")')
            with self.assertRaises(Exception) as captured:
                fut.result()
            self.assertIs(type(captured.exception), Exception)
            self.assertEqual(str(captured.exception), 'spam')
            cause = captured.exception.__cause__
            self.assertIs(type(cause), ExecutionFailed)
            for attr in ('__name__', '__qualname__', '__module__'):
                self.assertEqual(getattr(cause.excinfo.type, attr),
                                 getattr(Exception, attr))
            self.assertEqual(cause.excinfo.msg, 'spam')

        with self.subTest('func'):
            fut = self.executor.submit(fail, Exception, 'spam')
            with self.assertRaises(Exception) as captured:
                fut.result()
            self.assertIs(type(captured.exception), Exception)
            self.assertEqual(str(captured.exception), 'spam')
            cause = captured.exception.__cause__
            self.assertIs(type(cause), ExecutionFailed)
            for attr in ('__name__', '__qualname__', '__module__'):
                self.assertEqual(getattr(cause.excinfo.type, attr),
                                 getattr(Exception, attr))
            self.assertEqual(cause.excinfo.msg, 'spam')

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
