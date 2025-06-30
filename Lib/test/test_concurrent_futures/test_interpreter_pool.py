import asyncio
import contextlib
import io
import os
import sys
import time
import unittest
from concurrent.futures.interpreter import BrokenInterpreterPool
from concurrent import interpreters
from concurrent.interpreters import _queues as queues
import _interpreters
from test import support
from test.support import os_helper
from test.support import script_helper
import test.test_asyncio.utils as testasyncio_utils

from .executor import ExecutorTest, mul
from .util import BaseTestCase, InterpreterPoolMixin, setup_module


WINDOWS = sys.platform.startswith('win')


@contextlib.contextmanager
def nonblocking(fd):
    blocking = os.get_blocking(fd)
    if blocking:
        os.set_blocking(fd, False)
    try:
        yield
    finally:
        if blocking:
            os.set_blocking(fd, blocking)


def read_file_with_timeout(fd, nbytes, timeout):
    with nonblocking(fd):
        end = time.time() + timeout
        try:
            return os.read(fd, nbytes)
        except BlockingIOError:
            pass
        while time.time() < end:
            try:
                return os.read(fd, nbytes)
            except BlockingIOError:
                continue
        else:
            raise TimeoutError('nothing to read')


if not WINDOWS:
    import select
    def read_file_with_timeout(fd, nbytes, timeout):
        r, _, _ = select.select([fd], [], [], timeout)
        if fd not in r:
            raise TimeoutError('nothing to read')
        return os.read(fd, nbytes)


def noop():
    pass


def write_msg(fd, msg):
    import os
    os.write(fd, msg + b'\0')


def read_msg(fd, timeout=10.0):
    msg = b''
    ch = read_file_with_timeout(fd, 1, timeout)
    while ch != b'\0':
        msg += ch
        ch = os.read(fd, 1)
    return msg


def get_current_name():
    return __name__


def fail(exctype, msg=None):
    raise exctype(msg)


def get_current_interpid(*extra):
    interpid, _ = _interpreters.get_current()
    return (interpid, *extra)


class InterpretersMixin(InterpreterPoolMixin):

    def pipe(self):
        r, w = os.pipe()
        self.addCleanup(lambda: os.close(r))
        self.addCleanup(lambda: os.close(w))
        return r, w


class PickleShenanigans:
    """Succeeds with pickle.dumps(), but fails with pickle.loads()"""
    def __init__(self, value):
        if value == 1:
            raise RuntimeError("gotcha")

    def __reduce__(self):
        return (self.__class__, (1,))


class InterpreterPoolExecutorTest(
            InterpretersMixin, ExecutorTest, BaseTestCase):

    @unittest.expectedFailure
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

    @unittest.expectedFailure
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

    def test_init_with___main___global(self):
        # See https://github.com/python/cpython/pull/133957#issuecomment-2927415311.
        text = """if True:
            from concurrent.futures import InterpreterPoolExecutor

            INITIALIZER_STATUS = 'uninitialized'

            def init(x):
                global INITIALIZER_STATUS
                INITIALIZER_STATUS = x
                INITIALIZER_STATUS

            def get_init_status():
                return INITIALIZER_STATUS

            if __name__ == "__main__":
                exe = InterpreterPoolExecutor(initializer=init,
                                              initargs=('initialized',))
                fut = exe.submit(get_init_status)
                print(fut.result())  # 'initialized'
                exe.shutdown(wait=True)
                print(INITIALIZER_STATUS)  # 'uninitialized'
           """
        with os_helper.temp_dir() as tempdir:
            filename = script_helper.make_script(tempdir, 'my-script', text)
            res = script_helper.assert_python_ok(filename)
        stdout = res.out.decode('utf-8').strip()
        self.assertEqual(stdout.splitlines(), [
            'initialized',
            'uninitialized',
        ])

    def test_init_closure(self):
        count = 0
        def init1():
            assert count == 0, count
        def init2():
            nonlocal count
            count += 1

        with contextlib.redirect_stderr(io.StringIO()) as stderr:
            with self.executor_type(initializer=init1) as executor:
                fut = executor.submit(lambda: None)
        self.assertIn('NotShareableError', stderr.getvalue())
        with self.assertRaises(BrokenInterpreterPool):
            fut.result()

        with contextlib.redirect_stderr(io.StringIO()) as stderr:
            with self.executor_type(initializer=init2) as executor:
                fut = executor.submit(lambda: None)
        self.assertIn('NotShareableError', stderr.getvalue())
        with self.assertRaises(BrokenInterpreterPool):
            fut.result()

    def test_init_instance_method(self):
        class Spam:
            def initializer(self):
                raise NotImplementedError
        spam = Spam()

        with contextlib.redirect_stderr(io.StringIO()) as stderr:
            with self.executor_type(initializer=spam.initializer) as executor:
                fut = executor.submit(lambda: None)
        self.assertIn('NotShareableError', stderr.getvalue())
        with self.assertRaises(BrokenInterpreterPool):
            fut.result()

    @unittest.expectedFailure
    def test_init_exception_in_script(self):
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

    def test_init_exception_in_func(self):
        executor = self.executor_type(initializer=fail,
                                      initargs=(Exception, 'spam'))
        with executor:
            with contextlib.redirect_stderr(io.StringIO()) as stderr:
                fut = executor.submit(noop)
                with self.assertRaises(BrokenInterpreterPool):
                    fut.result()
        stderr = stderr.getvalue()
        self.assertIn('ExecutionFailed: Exception: spam', stderr)
        self.assertIn('Uncaught in the interpreter:', stderr)

    @unittest.expectedFailure
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

        fut = executor.submit(task1)
        with self.assertRaises(_interpreters.NotShareableError):
            fut.result()

        fut = executor.submit(task2)
        with self.assertRaises(_interpreters.NotShareableError):
            fut.result()

    def test_submit_local_instance(self):
        class Spam:
            def __init__(self):
                self.value = True

        executor = self.executor_type()
        fut = executor.submit(Spam)
        with self.assertRaises(_interpreters.NotShareableError):
            fut.result()

    def test_submit_instance_method(self):
        class Spam:
            def run(self):
                return True
        spam = Spam()

        executor = self.executor_type()
        fut = executor.submit(spam.run)
        with self.assertRaises(_interpreters.NotShareableError):
            fut.result()

    def test_submit_func_globals(self):
        executor = self.executor_type()
        fut = executor.submit(get_current_name)
        name = fut.result()

        self.assertEqual(name, __name__)
        self.assertNotEqual(name, '__main__')

    @unittest.expectedFailure
    def test_submit_exception_in_script(self):
        # Scripts are not supported currently.
        fut = self.executor.submit('raise Exception("spam")')
        with self.assertRaises(Exception) as captured:
            fut.result()
        self.assertIs(type(captured.exception), Exception)
        self.assertEqual(str(captured.exception), 'spam')
        cause = captured.exception.__cause__
        self.assertIs(type(cause), interpreters.ExecutionFailed)
        for attr in ('__name__', '__qualname__', '__module__'):
            self.assertEqual(getattr(cause.excinfo.type, attr),
                             getattr(Exception, attr))
        self.assertEqual(cause.excinfo.msg, 'spam')

    def test_submit_exception_in_func(self):
        fut = self.executor.submit(fail, Exception, 'spam')
        with self.assertRaises(Exception) as captured:
            fut.result()
        self.assertIs(type(captured.exception), Exception)
        self.assertEqual(str(captured.exception), 'spam')
        cause = captured.exception.__cause__
        self.assertIs(type(cause), interpreters.ExecutionFailed)
        for attr in ('__name__', '__qualname__', '__module__'):
            self.assertEqual(getattr(cause.excinfo.type, attr),
                             getattr(Exception, attr))
        self.assertEqual(cause.excinfo.msg, 'spam')

    def test_saturation(self):
        blocker = queues.create()
        executor = self.executor_type(4)

        for i in range(15 * executor._max_workers):
            executor.submit(blocker.get)
        self.assertEqual(len(executor._threads), executor._max_workers)
        for i in range(15 * executor._max_workers):
            blocker.put_nowait(None)
        executor.shutdown(wait=True)

    def test_blocking(self):
        # There is no guarantee that a worker will be created for every
        # submitted task.  That's because there's a race between:
        #
        # * a new worker thread, created when task A was just submitted,
        #   becoming non-idle when it picks up task A
        # * after task B is added to the queue, a new worker thread
        #   is started only if there are no idle workers
        #   (the check in ThreadPoolExecutor._adjust_thread_count())
        #
        # That means we must not block waiting for *all* tasks to report
        # "ready" before we unblock the known-ready workers.
        ready = queues.create()
        blocker = queues.create()

        def run(taskid, ready, blocker):
            # There can't be any globals here.
            ready.put_nowait(taskid)
            blocker.get()  # blocking

        numtasks = 10
        futures = []
        with self.executor_type() as executor:
            # Request the jobs.
            for i in range(numtasks):
                fut = executor.submit(run, i, ready, blocker)
                futures.append(fut)
            pending = numtasks
            while pending > 0:
                # Wait for any to be ready.
                done = 0
                for _ in range(pending):
                    try:
                        ready.get(timeout=1)  # blocking
                    except interpreters.QueueEmpty:
                        pass
                    else:
                        done += 1
                pending -= done
                # Unblock the workers.
                for _ in range(done):
                    blocker.put_nowait(None)

    def test_blocking_with_limited_workers(self):
        # This is essentially the same as test_blocking,
        # but we explicitly force a limited number of workers,
        # instead of it happening implicitly sometimes due to a race.
        ready = queues.create()
        blocker = queues.create()

        def run(taskid, ready, blocker):
            # There can't be any globals here.
            ready.put_nowait(taskid)
            blocker.get()  # blocking

        numtasks = 10
        futures = []
        with self.executor_type(4) as executor:
            # Request the jobs.
            for i in range(numtasks):
                fut = executor.submit(run, i, ready, blocker)
                futures.append(fut)
            pending = numtasks
            while pending > 0:
                # Wait for any to be ready.
                done = 0
                for _ in range(pending):
                    try:
                        ready.get(timeout=1)  # blocking
                    except interpreters.QueueEmpty:
                        pass
                    else:
                        done += 1
                pending -= done
                # Unblock the workers.
                for _ in range(done):
                    blocker.put_nowait(None)

    @support.requires_gil_enabled("gh-117344: test is flaky without the GIL")
    def test_idle_thread_reuse(self):
        executor = self.executor_type()
        executor.submit(mul, 21, 2).result()
        executor.submit(mul, 6, 7).result()
        executor.submit(mul, 3, 14).result()
        self.assertEqual(len(executor._threads), 1)
        executor.shutdown(wait=True)

    def test_pickle_errors_propagate(self):
        # GH-125864: Pickle errors happen before the script tries to execute,
        # so the queue used to wait infinitely.
        fut = self.executor.submit(PickleShenanigans(0))
        expected = interpreters.NotShareableError
        with self.assertRaisesRegex(expected, 'args not shareable') as cm:
            fut.result()
        self.assertRegex(str(cm.exception.__cause__), 'unpickled')

    def test_no_stale_references(self):
        # Weak references don't cross between interpreters.
        raise unittest.SkipTest('not applicable')

    def test_free_reference(self):
        # Weak references don't cross between interpreters.
        raise unittest.SkipTest('not applicable')


class AsyncioTest(InterpretersMixin, testasyncio_utils.TestCase):

    @classmethod
    def setUpClass(cls):
        # Most uses of asyncio will implicitly call set_event_loop_policy()
        # with the default policy if a policy hasn't been set already.
        # If that happens in a test, like here, we'll end up with a failure
        # when --fail-env-changed is used.  That's why the other tests that
        # use asyncio are careful to set the policy back to None and why
        # we're careful to do so here.  We also validate that no other
        # tests left a policy in place, just in case.
        policy = support.maybe_get_event_loop_policy()
        assert policy is None, policy
        cls.addClassCleanup(lambda: asyncio._set_event_loop_policy(None))

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)

        self.executor = self.executor_type()
        self.addCleanup(lambda: self.executor.shutdown())

    def tearDown(self):
        if not self.loop.is_closed():
            testasyncio_utils.run_briefly(self.loop)

        self.doCleanups()
        support.gc_collect()
        super().tearDown()

    def test_run_in_executor(self):
        unexpected, _ = _interpreters.get_current()

        func = get_current_interpid
        fut = self.loop.run_in_executor(self.executor, func, 'yo')
        interpid, res = self.loop.run_until_complete(fut)

        self.assertEqual(res, 'yo')
        self.assertNotEqual(interpid, unexpected)

    def test_run_in_executor_cancel(self):
        executor = self.executor_type()

        called = False

        def patched_call_soon(*args):
            nonlocal called
            called = True

        func = time.sleep
        fut = self.loop.run_in_executor(self.executor, func, 0.05)
        fut.cancel()
        self.loop.run_until_complete(
                self.loop.shutdown_default_executor())
        self.loop.close()
        self.loop.call_soon = patched_call_soon
        self.loop.call_soon_threadsafe = patched_call_soon
        time.sleep(0.4)
        self.assertFalse(called)

    def test_default_executor(self):
        unexpected, _ = _interpreters.get_current()

        self.loop.set_default_executor(self.executor)
        fut = self.loop.run_in_executor(None, get_current_interpid)
        interpid, = self.loop.run_until_complete(fut)

        self.assertNotEqual(interpid, unexpected)


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
