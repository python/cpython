import io
import os
import sys
import unittest
import random
from test import support
from test.support import threading_helper
import _thread as thread
import time
import warnings
import weakref

from test import lock_tests

threading_helper.requires_working_threading(module=True)

NUMTASKS = 10
NUMTRIPS = 3

_print_mutex = thread.allocate_lock()

def verbose_print(arg):
    """Helper function for printing out debugging output."""
    if support.verbose:
        with _print_mutex:
            print(arg)


class BasicThreadTest(unittest.TestCase):

    def setUp(self):
        self.done_mutex = thread.allocate_lock()
        self.done_mutex.acquire()
        self.running_mutex = thread.allocate_lock()
        self.random_mutex = thread.allocate_lock()
        self.created = 0
        self.running = 0
        self.next_ident = 0

        key = threading_helper.threading_setup()
        self.addCleanup(threading_helper.threading_cleanup, *key)


class ThreadRunningTests(BasicThreadTest):

    def newtask(self):
        with self.running_mutex:
            self.next_ident += 1
            verbose_print("creating task %s" % self.next_ident)
            thread.start_new_thread(self.task, (self.next_ident,))
            self.created += 1
            self.running += 1

    def task(self, ident):
        with self.random_mutex:
            delay = random.random() / 10000.0
        verbose_print("task %s will run for %sus" % (ident, round(delay*1e6)))
        time.sleep(delay)
        verbose_print("task %s done" % ident)
        with self.running_mutex:
            self.running -= 1
            if self.created == NUMTASKS and self.running == 0:
                self.done_mutex.release()

    def test_starting_threads(self):
        with threading_helper.wait_threads_exit():
            # Basic test for thread creation.
            for i in range(NUMTASKS):
                self.newtask()
            verbose_print("waiting for tasks to complete...")
            self.done_mutex.acquire()
            verbose_print("all tasks done")

    def test_stack_size(self):
        # Various stack size tests.
        self.assertEqual(thread.stack_size(), 0, "initial stack size is not 0")

        thread.stack_size(0)
        self.assertEqual(thread.stack_size(), 0, "stack_size not reset to default")

    @unittest.skipIf(os.name not in ("nt", "posix"), 'test meant for nt and posix')
    def test_nt_and_posix_stack_size(self):
        try:
            thread.stack_size(4096)
        except ValueError:
            verbose_print("caught expected ValueError setting "
                            "stack_size(4096)")
        except thread.error:
            self.skipTest("platform does not support changing thread stack "
                          "size")

        fail_msg = "stack_size(%d) failed - should succeed"
        for tss in (262144, 0x100000, 0):
            thread.stack_size(tss)
            self.assertEqual(thread.stack_size(), tss, fail_msg % tss)
            verbose_print("successfully set stack_size(%d)" % tss)

        for tss in (262144, 0x100000):
            verbose_print("trying stack_size = (%d)" % tss)
            self.next_ident = 0
            self.created = 0
            with threading_helper.wait_threads_exit():
                for i in range(NUMTASKS):
                    self.newtask()

                verbose_print("waiting for all tasks to complete")
                self.done_mutex.acquire()
                verbose_print("all tasks done")

        thread.stack_size(0)

    def test__count(self):
        # Test the _count() function.
        orig = thread._count()
        mut = thread.allocate_lock()
        mut.acquire()
        started = []

        def task():
            started.append(None)
            mut.acquire()
            mut.release()

        with threading_helper.wait_threads_exit():
            thread.start_new_thread(task, ())
            for _ in support.sleeping_retry(support.LONG_TIMEOUT):
                if started:
                    break
            self.assertEqual(thread._count(), orig + 1)

            # Allow the task to finish.
            mut.release()

            # The only reliable way to be sure that the thread ended from the
            # interpreter's point of view is to wait for the function object to
            # be destroyed.
            done = []
            wr = weakref.ref(task, lambda _: done.append(None))
            del task

            for _ in support.sleeping_retry(support.LONG_TIMEOUT):
                if done:
                    break
                support.gc_collect()  # For PyPy or other GCs.
            self.assertEqual(thread._count(), orig)

    def test_unraisable_exception(self):
        def task():
            started.release()
            raise ValueError("task failed")

        started = thread.allocate_lock()
        with support.catch_unraisable_exception() as cm:
            with threading_helper.wait_threads_exit():
                started.acquire()
                thread.start_new_thread(task, ())
                started.acquire()

            self.assertEqual(str(cm.unraisable.exc_value), "task failed")
            self.assertIs(cm.unraisable.object, task)
            self.assertEqual(cm.unraisable.err_msg,
                             "Exception ignored in thread started by")
            self.assertIsNotNone(cm.unraisable.exc_traceback)


class Barrier:
    def __init__(self, num_threads):
        self.num_threads = num_threads
        self.waiting = 0
        self.checkin_mutex  = thread.allocate_lock()
        self.checkout_mutex = thread.allocate_lock()
        self.checkout_mutex.acquire()

    def enter(self):
        self.checkin_mutex.acquire()
        self.waiting = self.waiting + 1
        if self.waiting == self.num_threads:
            self.waiting = self.num_threads - 1
            self.checkout_mutex.release()
            return
        self.checkin_mutex.release()

        self.checkout_mutex.acquire()
        self.waiting = self.waiting - 1
        if self.waiting == 0:
            self.checkin_mutex.release()
            return
        self.checkout_mutex.release()


class BarrierTest(BasicThreadTest):

    def test_barrier(self):
        with threading_helper.wait_threads_exit():
            self.bar = Barrier(NUMTASKS)
            self.running = NUMTASKS
            for i in range(NUMTASKS):
                thread.start_new_thread(self.task2, (i,))
            verbose_print("waiting for tasks to end")
            self.done_mutex.acquire()
            verbose_print("tasks done")

    def task2(self, ident):
        for i in range(NUMTRIPS):
            if ident == 0:
                # give it a good chance to enter the next
                # barrier before the others are all out
                # of the current one
                delay = 0
            else:
                with self.random_mutex:
                    delay = random.random() / 10000.0
            verbose_print("task %s will run for %sus" %
                          (ident, round(delay * 1e6)))
            time.sleep(delay)
            verbose_print("task %s entering %s" % (ident, i))
            self.bar.enter()
            verbose_print("task %s leaving barrier" % ident)
        with self.running_mutex:
            self.running -= 1
            # Must release mutex before releasing done, else the main thread can
            # exit and set mutex to None as part of global teardown; then
            # mutex.release() raises AttributeError.
            finished = self.running == 0
        if finished:
            self.done_mutex.release()

class LockTests(lock_tests.LockTests):
    locktype = thread.allocate_lock


class TestForkInThread(unittest.TestCase):
    def setUp(self):
        self.read_fd, self.write_fd = os.pipe()

    @support.requires_fork()
    @threading_helper.reap_threads
    def test_forkinthread(self):
        pid = None

        def fork_thread(read_fd, write_fd):
            nonlocal pid

            # Ignore the warning about fork with threads.
            with warnings.catch_warnings(category=DeprecationWarning,
                                         action="ignore"):
                # fork in a thread (DANGER, undefined per POSIX)
                if (pid := os.fork()):
                    # parent process
                    return

            # child process
            try:
                os.close(read_fd)
                os.write(write_fd, b"OK")
            finally:
                os._exit(0)

        with threading_helper.wait_threads_exit():
            thread.start_new_thread(fork_thread, (self.read_fd, self.write_fd))
            self.assertEqual(os.read(self.read_fd, 2), b"OK")
            os.close(self.write_fd)

        self.assertIsNotNone(pid)
        support.wait_process(pid, exitcode=0)

    def tearDown(self):
        try:
            os.close(self.read_fd)
        except OSError:
            pass

        try:
            os.close(self.write_fd)
        except OSError:
            pass


class TestStdioAtForkReInit(unittest.TestCase):

    class MockStdio(io.TextIOWrapper):
        def __init__(self):
            import _io, tempfile
            self._file = tempfile.mktemp()
            raw = _io.FileIO(self._file, mode='wb')
            buffer = _io.BufferedWriter(raw)
            super().__init__(buffer)

        def __del__(self):
            try:
                self.close()
                os.remove(self._file)
            except:
                pass

        def isatty(self):
            # pretend to be a tty, so _PySys_ReInitStdio
            # will attempt to reinitialize our buffer lock.
            return True

    def setUp(self):
        # Replace stdout & stderr with mock objects with the
        # same underlying buffer type to avoid polluting the
        # test output stream.
        self._saved_stdout = sys.stdout
        self._saved_stderr = sys.stderr
        sys.stdout = self.MockStdio()
        sys.stderr = self.MockStdio()

    def tearDown(self):
        sys.stdout = self._saved_stdout
        sys.stderr = self._saved_stderr

    @unittest.skipUnless(hasattr(os, 'fork'), 'need os.fork')
    @threading_helper.reap_threads
    def test_stdio_at_fork_reinit(self):

        # bpo-46210: Added _PySys_ReInitStdio to prevent children
        # from deadlocking here if printer_thread held the stdout
        # (or stderr) buffer's lock when one of the children forked.

        num_prints = 100
        num_forks = 100
        msg = 'hello'*10

        def printer_thread(stdio):
            for n in range(num_prints):
                print(msg, file=stdio)
                stdio.flush()

        def fork_processes(stdio):
            pids = []
            for n in range(num_forks):
                if pid := os.fork():
                    pids.append(pid)
                else:
                    print(msg, file=stdio)
                    stdio.flush()
                    os._exit(0)

            return pids

        def main(stdio):
            thread.start_new_thread(printer_thread, (stdio,))
            pids = fork_processes(stdio)
            for pid in pids:
                support.wait_process(pid, exitcode=0)
            return

        # Forking below is not necessary to illustrate the bug
        # in bpo-46210 and its fix. The issue in bpo-46210 is
        # that calling main(sys.stdout) or main(sys.stderr)
        # is sufficient to cause a deadlock in print. We fork
        # here only to allow us to give a single timeout to the
        # main() call, since its failure mode (absent the fix)
        # is for some subset of the forked child processes to
        # deadlock at the moment when they try to print, rather
        # than to raise an exception. Therefore, simply looping
        # over the child pids and calling support.wait_process
        # with a separate nonzero timeout for each child leads
        # to a rather unpredictable total wait time, whereas
        # forking again here at the top (though not necessary
        # to illustrate the bug) allows us to give a predictable
        # timeout to the process of waiting for the children.
        #
        # bpo-46210 is present if and only if one or more of the
        # children forked by main() deadlock when they call print.
        #
        # pr-30310 proposes a fix following the example of the
        # import lock, by providing a function _PySys_ReInitStdio
        # that is called alongside the other preexisting lock
        # reinitialization functions in PyOS_AfterFork_Child.
        for stdio in (sys.stdout, sys.stderr):
            with threading_helper.wait_threads_exit():
                if main_pid := os.fork():
                    support.wait_process(main_pid, exitcode=0, timeout=5)
                else:
                    main(stdio)
                    os._exit(0)

if __name__ == "__main__":
    unittest.main()
