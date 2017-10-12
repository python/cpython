import _dummy_thread as _thread
import time
import queue
import random
import unittest
from test import support
from unittest import mock

DELAY = 0


class LockTests(unittest.TestCase):
    """Test lock objects."""

    def setUp(self):
        # Create a lock
        self.lock = _thread.allocate_lock()

    def test_initlock(self):
        #Make sure locks start locked
        self.assertFalse(self.lock.locked(),
                        "Lock object is not initialized unlocked.")

    def test_release(self):
        # Test self.lock.release()
        self.lock.acquire()
        self.lock.release()
        self.assertFalse(self.lock.locked(),
                        "Lock object did not release properly.")

    def test_LockType_context_manager(self):
        with _thread.LockType():
            pass
        self.assertFalse(self.lock.locked(),
                         "Acquired Lock was not released")

    def test_improper_release(self):
        #Make sure release of an unlocked thread raises RuntimeError
        self.assertRaises(RuntimeError, self.lock.release)

    def test_cond_acquire_success(self):
        #Make sure the conditional acquiring of the lock works.
        self.assertTrue(self.lock.acquire(0),
                        "Conditional acquiring of the lock failed.")

    def test_cond_acquire_fail(self):
        #Test acquiring locked lock returns False
        self.lock.acquire(0)
        self.assertFalse(self.lock.acquire(0),
                        "Conditional acquiring of a locked lock incorrectly "
                         "succeeded.")

    def test_uncond_acquire_success(self):
        #Make sure unconditional acquiring of a lock works.
        self.lock.acquire()
        self.assertTrue(self.lock.locked(),
                        "Uncondional locking failed.")

    def test_uncond_acquire_return_val(self):
        #Make sure that an unconditional locking returns True.
        self.assertIs(self.lock.acquire(1), True,
                        "Unconditional locking did not return True.")
        self.assertIs(self.lock.acquire(), True)

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
        self.assertGreaterEqual(end_time - start_time, DELAY,
                        "Blocking by unconditional acquiring failed.")

    @mock.patch('time.sleep')
    def test_acquire_timeout(self, mock_sleep):
        """Test invoking acquire() with a positive timeout when the lock is
        already acquired. Ensure that time.sleep() is invoked with the given
        timeout and that False is returned."""

        self.lock.acquire()
        retval = self.lock.acquire(waitflag=0, timeout=1)
        self.assertTrue(mock_sleep.called)
        mock_sleep.assert_called_once_with(1)
        self.assertEqual(retval, False)

    def test_lock_representation(self):
        self.lock.acquire()
        self.assertIn("locked", repr(self.lock))
        self.lock.release()
        self.assertIn("unlocked", repr(self.lock))


class MiscTests(unittest.TestCase):
    """Miscellaneous tests."""

    def test_exit(self):
        self.assertRaises(SystemExit, _thread.exit)

    def test_ident(self):
        self.assertIsInstance(_thread.get_ident(), int,
                              "_thread.get_ident() returned a non-integer")
        self.assertGreater(_thread.get_ident(), 0)

    def test_LockType(self):
        self.assertIsInstance(_thread.allocate_lock(), _thread.LockType,
                              "_thread.LockType is not an instance of what "
                              "is returned by _thread.allocate_lock()")

    def test_set_sentinel(self):
        self.assertIsInstance(_thread._set_sentinel(), _thread.LockType,
                              "_thread._set_sentinel() did not return a "
                              "LockType instance.")

    def test_interrupt_main(self):
        #Calling start_new_thread with a function that executes interrupt_main
        # should raise KeyboardInterrupt upon completion.
        def call_interrupt():
            _thread.interrupt_main()

        self.assertRaises(KeyboardInterrupt,
                          _thread.start_new_thread,
                          call_interrupt,
                          tuple())

    def test_interrupt_in_main(self):
        self.assertRaises(KeyboardInterrupt, _thread.interrupt_main)

    def test_stack_size_None(self):
        retval = _thread.stack_size(None)
        self.assertEqual(retval, 0)

    def test_stack_size_not_None(self):
        with self.assertRaises(_thread.error) as cm:
            _thread.stack_size("")
        self.assertEqual(cm.exception.args[0],
                         "setting thread stack size not supported")


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
                        "Argument passing for thread creation "
                        "using tuple failed")

        _thread.start_new_thread(
                arg_tester,
                tuple(),
                {'queue':testing_queue, 'arg1':True, 'arg2':True})

        result = testing_queue.get()
        self.assertTrue(result[0] and result[1],
                        "Argument passing for thread creation "
                        "using kwargs failed")

        _thread.start_new_thread(
                arg_tester,
                (testing_queue, True),
                {'arg2':True})

        result = testing_queue.get()
        self.assertTrue(result[0] and result[1],
                        "Argument passing for thread creation using both tuple"
                        " and kwargs failed")

    def test_multi_thread_creation(self):
        def queue_mark(queue, delay):
            time.sleep(delay)
            queue.put(_thread.get_ident())

        thread_count = 5
        testing_queue = queue.Queue(thread_count)

        if support.verbose:
            print()
            print("*** Testing multiple thread creation "
                  "(will take approx. %s to %s sec.) ***" % (
                    DELAY, thread_count))

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
        self.assertEqual(testing_queue.qsize(), thread_count,
                         "Not all %s threads executed properly "
                         "after %s sec." % (thread_count, DELAY))

    def test_args_not_tuple(self):
        """
        Test invoking start_new_thread() with a non-tuple value for "args".
        Expect TypeError with a meaningful error message to be raised.
        """
        with self.assertRaises(TypeError) as cm:
            _thread.start_new_thread(mock.Mock(), [])
        self.assertEqual(cm.exception.args[0], "2nd arg must be a tuple")

    def test_kwargs_not_dict(self):
        """
        Test invoking start_new_thread() with a non-dict value for "kwargs".
        Expect TypeError with a meaningful error message to be raised.
        """
        with self.assertRaises(TypeError) as cm:
            _thread.start_new_thread(mock.Mock(), tuple(), kwargs=[])
        self.assertEqual(cm.exception.args[0], "3rd arg must be a dict")

    def test_SystemExit(self):
        """
        Test invoking start_new_thread() with a function that raises
        SystemExit.
        The exception should be discarded.
        """
        func = mock.Mock(side_effect=SystemExit())
        try:
            _thread.start_new_thread(func, tuple())
        except SystemExit:
            self.fail("start_new_thread raised SystemExit.")

    @mock.patch('traceback.print_exc')
    def test_RaiseException(self, mock_print_exc):
        """
        Test invoking start_new_thread() with a function that raises exception.

        The exception should be discarded and the traceback should be printed
        via traceback.print_exc()
        """
        func = mock.Mock(side_effect=Exception)
        _thread.start_new_thread(func, tuple())
        self.assertTrue(mock_print_exc.called)
