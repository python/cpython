import os
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
            self.assertIsNone(cm.unraisable.object)
            self.assertEqual(cm.unraisable.err_msg,
                             f"Exception ignored in thread started by {task!r}")
            self.assertIsNotNone(cm.unraisable.exc_traceback)

    def test_join_thread(self):
        finished = []

        def task():
            time.sleep(0.05)
            finished.append(thread.get_ident())

        with threading_helper.wait_threads_exit():
            handle = thread.start_joinable_thread(task)
            handle.join()
            self.assertEqual(len(finished), 1)
            self.assertEqual(handle.ident, finished[0])

    def test_join_thread_already_exited(self):
        def task():
            pass

        with threading_helper.wait_threads_exit():
            handle = thread.start_joinable_thread(task)
            time.sleep(0.05)
            handle.join()

    def test_join_several_times(self):
        def task():
            pass

        with threading_helper.wait_threads_exit():
            handle = thread.start_joinable_thread(task)
            handle.join()
            # Subsequent join() calls should succeed
            handle.join()

    def test_joinable_not_joined(self):
        handle_destroyed = thread.allocate_lock()
        handle_destroyed.acquire()

        def task():
            handle_destroyed.acquire()

        with threading_helper.wait_threads_exit():
            handle = thread.start_joinable_thread(task)
            del handle
            handle_destroyed.release()

    def test_join_from_self(self):
        errors = []
        handles = []
        start_joinable_thread_returned = thread.allocate_lock()
        start_joinable_thread_returned.acquire()
        task_tried_to_join = thread.allocate_lock()
        task_tried_to_join.acquire()

        def task():
            start_joinable_thread_returned.acquire()
            try:
                handles[0].join()
            except Exception as e:
                errors.append(e)
            finally:
                task_tried_to_join.release()

        with threading_helper.wait_threads_exit():
            handle = thread.start_joinable_thread(task)
            handles.append(handle)
            start_joinable_thread_returned.release()
            # Can still join after joining failed in other thread
            task_tried_to_join.acquire()
            handle.join()

        assert len(errors) == 1
        with self.assertRaisesRegex(RuntimeError, "Cannot join current thread"):
            raise errors[0]

    def test_join_then_self_join(self):
        # make sure we can't deadlock in the following scenario with
        # threads t0 and t1 (see comment in `ThreadHandle_join()` for more
        # details):
        #
        # - t0 joins t1
        # - t1 self joins
        def make_lock():
            lock = thread.allocate_lock()
            lock.acquire()
            return lock

        error = None
        self_joiner_handle = None
        self_joiner_started = make_lock()
        self_joiner_barrier = make_lock()
        def self_joiner():
            nonlocal error

            self_joiner_started.release()
            self_joiner_barrier.acquire()

            try:
                self_joiner_handle.join()
            except Exception as e:
                error = e

        joiner_started = make_lock()
        def joiner():
            joiner_started.release()
            self_joiner_handle.join()

        with threading_helper.wait_threads_exit():
            self_joiner_handle = thread.start_joinable_thread(self_joiner)
            # Wait for the self-joining thread to start
            self_joiner_started.acquire()

            # Start the thread that joins the self-joiner
            joiner_handle = thread.start_joinable_thread(joiner)

            # Wait for the joiner to start
            joiner_started.acquire()

            # Not great, but I don't think there's a deterministic way to make
            # sure that the self-joining thread has been joined.
            time.sleep(0.1)

            # Unblock the self-joiner
            self_joiner_barrier.release()

            self_joiner_handle.join()
            joiner_handle.join()

            with self.assertRaisesRegex(RuntimeError, "Cannot join current thread"):
                raise error

    def test_join_with_timeout(self):
        lock = thread.allocate_lock()
        lock.acquire()

        def thr():
            lock.acquire()

        with threading_helper.wait_threads_exit():
            handle = thread.start_joinable_thread(thr)
            handle.join(0.1)
            self.assertFalse(handle.is_done())
            lock.release()
            handle.join()
            self.assertTrue(handle.is_done())

    def test_join_unstarted(self):
        handle = thread._ThreadHandle()
        with self.assertRaisesRegex(RuntimeError, "thread not started"):
            handle.join()

    def test_set_done_unstarted(self):
        handle = thread._ThreadHandle()
        with self.assertRaisesRegex(RuntimeError, "thread not started"):
            handle._set_done()

    def test_start_duplicate_handle(self):
        lock = thread.allocate_lock()
        lock.acquire()

        def func():
            lock.acquire()

        handle = thread._ThreadHandle()
        with threading_helper.wait_threads_exit():
            thread.start_joinable_thread(func, handle=handle)
            with self.assertRaisesRegex(RuntimeError, "thread already started"):
                thread.start_joinable_thread(func, handle=handle)
            lock.release()
            handle.join()

    def test_start_with_none_handle(self):
        def func():
            pass

        with threading_helper.wait_threads_exit():
            handle = thread.start_joinable_thread(func, handle=None)
            handle.join()


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


if __name__ == "__main__":
    unittest.main()
