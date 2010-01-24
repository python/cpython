"""Generic thread tests.

Meant to be used by dummy_thread and thread.  To allow for different modules
to be used, test_main() can be called with the module to use as the thread
implementation as its sole argument.

"""
import _dummy_thread as _thread
import time
import queue
import random
import unittest
from test import support

DELAY = 0 # Set > 0 when testing a module other than _dummy_thread, such as
          # the '_thread' module.

class LockTests(unittest.TestCase):
    """Test lock objects."""

    def setUp(self):
        # Create a lock
        self.lock = _thread.allocate_lock()

    def test_initlock(self):
        #Make sure locks start locked
        self.assertTrue(not self.lock.locked(),
                        "Lock object is not initialized unlocked.")

    def test_release(self):
        # Test self.lock.release()
        self.lock.acquire()
        self.lock.release()
        self.assertTrue(not self.lock.locked(),
                        "Lock object did not release properly.")

    def test_improper_release(self):
        #Make sure release of an unlocked thread raises _thread.error
        self.assertRaises(_thread.error, self.lock.release)

    def test_cond_acquire_success(self):
        #Make sure the conditional acquiring of the lock works.
        self.assertTrue(self.lock.acquire(0),
                        "Conditional acquiring of the lock failed.")

    def test_cond_acquire_fail(self):
        #Test acquiring locked lock returns False
        self.lock.acquire(0)
        self.assertTrue(not self.lock.acquire(0),
                        "Conditional acquiring of a locked lock incorrectly "
                         "succeeded.")

    def test_uncond_acquire_success(self):
        #Make sure unconditional acquiring of a lock works.
        self.lock.acquire()
        self.assertTrue(self.lock.locked(),
                        "Uncondional locking failed.")

    def test_uncond_acquire_return_val(self):
        #Make sure that an unconditional locking returns True.
        self.assertTrue(self.lock.acquire(1) is True,
                        "Unconditional locking did not return True.")
        self.assertTrue(self.lock.acquire() is True)

    def test_uncond_acquire_blocking(self):
        #Make sure that unconditional acquiring of a locked lock blocks.
        def delay_unlock(to_unlock, delay):
            """Hold on to lock for a set amount of time before unlocking."""
            time.sleep(delay)
            to_unlock.release()

        self.lock.acquire()
        start_time = int(time.time())
        _thread.start_new_thread(delay_unlock,(self.lock, DELAY))
        if support.verbose:
            print()
            print("*** Waiting for thread to release the lock "\
            "(approx. %s sec.) ***" % DELAY)
        self.lock.acquire()
        end_time = int(time.time())
        if support.verbose:
            print("done")
        self.assertTrue((end_time - start_time) >= DELAY,
                        "Blocking by unconditional acquiring failed.")

class MiscTests(unittest.TestCase):
    """Miscellaneous tests."""

    def test_exit(self):
        #Make sure _thread.exit() raises SystemExit
        self.assertRaises(SystemExit, _thread.exit)

    def test_ident(self):
        #Test sanity of _thread.get_ident()
        self.assertIsInstance(_thread.get_ident(), int,
                              "_thread.get_ident() returned a non-integer")
        self.assertTrue(_thread.get_ident() != 0,
                        "_thread.get_ident() returned 0")

    def test_LockType(self):
        #Make sure _thread.LockType is the same type as _thread.allocate_locke()
        self.assertIsInstance(_thread.allocate_lock(), _thread.LockType,
                              "_thread.LockType is not an instance of what "
                              "is returned by _thread.allocate_lock()")

    def test_interrupt_main(self):
        #Calling start_new_thread with a function that executes interrupt_main
        # should raise KeyboardInterrupt upon completion.
        def call_interrupt():
            _thread.interrupt_main()
        self.assertRaises(KeyboardInterrupt, _thread.start_new_thread,
                              call_interrupt, tuple())

    def test_interrupt_in_main(self):
        # Make sure that if interrupt_main is called in main threat that
        # KeyboardInterrupt is raised instantly.
        self.assertRaises(KeyboardInterrupt, _thread.interrupt_main)

class ThreadTests(unittest.TestCase):
    """Test thread creation."""

    def test_arg_passing(self):
        #Make sure that parameter passing works.
        def arg_tester(queue, arg1=False, arg2=False):
            """Use to test _thread.start_new_thread() passes args properly."""
            queue.put((arg1, arg2))

        testing_queue = queue.Queue(1)
        _thread.start_new_thread(arg_tester, (testing_queue, True, True))
        result = testing_queue.get()
        self.assertTrue(result[0] and result[1],
                        "Argument passing for thread creation using tuple failed")
        _thread.start_new_thread(arg_tester, tuple(), {'queue':testing_queue,
                                                       'arg1':True, 'arg2':True})
        result = testing_queue.get()
        self.assertTrue(result[0] and result[1],
                        "Argument passing for thread creation using kwargs failed")
        _thread.start_new_thread(arg_tester, (testing_queue, True), {'arg2':True})
        result = testing_queue.get()
        self.assertTrue(result[0] and result[1],
                        "Argument passing for thread creation using both tuple"
                        " and kwargs failed")

    def test_multi_creation(self):
        #Make sure multiple threads can be created.
        def queue_mark(queue, delay):
            """Wait for ``delay`` seconds and then put something into ``queue``"""
            time.sleep(delay)
            queue.put(_thread.get_ident())

        thread_count = 5
        testing_queue = queue.Queue(thread_count)
        if support.verbose:
            print()
            print("*** Testing multiple thread creation "\
            "(will take approx. %s to %s sec.) ***" % (DELAY, thread_count))
        for count in range(thread_count):
            if DELAY:
                local_delay = round(random.random(), 1)
            else:
                local_delay = 0
            _thread.start_new_thread(queue_mark,
                                     (testing_queue, local_delay))
        time.sleep(DELAY)
        if support.verbose:
            print('done')
        self.assertTrue(testing_queue.qsize() == thread_count,
                        "Not all %s threads executed properly after %s sec." %
                        (thread_count, DELAY))

def test_main(imported_module=None):
    global _thread, DELAY
    if imported_module:
        _thread = imported_module
        DELAY = 2
    if support.verbose:
        print()
        print("*** Using %s as _thread module ***" % _thread)
    support.run_unittest(LockTests, MiscTests, ThreadTests)

if __name__ == '__main__':
    test_main()
