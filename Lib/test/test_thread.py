import os
import unittest
import random
from test import test_support
import thread
import time


NUMTASKS = 10
NUMTRIPS = 3


_print_mutex = thread.allocate_lock()

def verbose_print(arg):
    """Helper function for printing out debugging output."""
    if test_support.verbose:
        with _print_mutex:
            print arg


class BasicThreadTest(unittest.TestCase):

    def setUp(self):
        self.done_mutex = thread.allocate_lock()
        self.done_mutex.acquire()
        self.running_mutex = thread.allocate_lock()
        self.random_mutex = thread.allocate_lock()
        self.running = 0
        self.next_ident = 0


class ThreadRunningTests(BasicThreadTest):

    def newtask(self):
        with self.running_mutex:
            self.next_ident += 1
            verbose_print("creating task %s" % self.next_ident)
            thread.start_new_thread(self.task, (self.next_ident,))
            self.running += 1

    def task(self, ident):
        with self.random_mutex:
            delay = random.random() / 10000.0
        verbose_print("task %s will run for %sus" % (ident, round(delay*1e6)))
        time.sleep(delay)
        verbose_print("task %s done" % ident)
        with self.running_mutex:
            self.running -= 1
            if self.running == 0:
                self.done_mutex.release()

    def test_starting_threads(self):
        # Basic test for thread creation.
        for i in range(NUMTASKS):
            self.newtask()
        verbose_print("waiting for tasks to complete...")
        self.done_mutex.acquire()
        verbose_print("all tasks done")

    def test_stack_size(self):
        # Various stack size tests.
        self.assertEquals(thread.stack_size(), 0, "intial stack size is not 0")

        thread.stack_size(0)
        self.assertEquals(thread.stack_size(), 0, "stack_size not reset to default")

        if os.name not in ("nt", "os2", "posix"):
            return

        tss_supported = True
        try:
            thread.stack_size(4096)
        except ValueError:
            verbose_print("caught expected ValueError setting "
                            "stack_size(4096)")
        except thread.error:
            tss_supported = False
            verbose_print("platform does not support changing thread stack "
                            "size")

        if tss_supported:
            fail_msg = "stack_size(%d) failed - should succeed"
            for tss in (262144, 0x100000, 0):
                thread.stack_size(tss)
                self.assertEquals(thread.stack_size(), tss, fail_msg % tss)
                verbose_print("successfully set stack_size(%d)" % tss)

            for tss in (262144, 0x100000):
                verbose_print("trying stack_size = (%d)" % tss)
                self.next_ident = 0
                for i in range(NUMTASKS):
                    self.newtask()

                verbose_print("waiting for all tasks to complete")
                self.done_mutex.acquire()
                verbose_print("all tasks done")

            thread.stack_size(0)


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


def test_main():
    test_support.run_unittest(ThreadRunningTests, BarrierTest)

if __name__ == "__main__":
    test_main()
