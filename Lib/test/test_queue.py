# Some simple Queue module tests, plus some failure conditions
# to ensure the Queue locks remain stable.
import Queue
import sys
import threading
import time
import unittest
from test import test_support

QUEUE_SIZE = 5

# A thread to run a function that unclogs a blocked Queue.
class _TriggerThread(threading.Thread):
    def __init__(self, fn, args):
        self.fn = fn
        self.args = args
        self.startedEvent = threading.Event()
        threading.Thread.__init__(self)

    def run(self):
        # The sleep isn't necessary, but is intended to give the blocking
        # function in the main thread a chance at actually blocking before
        # we unclog it.  But if the sleep is longer than the timeout-based
        # tests wait in their blocking functions, those tests will fail.
        # So we give them much longer timeout values compared to the
        # sleep here (I aimed at 10 seconds for blocking functions --
        # they should never actually wait that long - they should make
        # progress as soon as we call self.fn()).
        time.sleep(0.1)
        self.startedEvent.set()
        self.fn(*self.args)


# Execute a function that blocks, and in a separate thread, a function that
# triggers the release.  Returns the result of the blocking function.  Caution:
# block_func must guarantee to block until trigger_func is called, and
# trigger_func must guarantee to change queue state so that block_func can make
# enough progress to return.  In particular, a block_func that just raises an
# exception regardless of whether trigger_func is called will lead to
# timing-dependent sporadic failures, and one of those went rarely seen but
# undiagnosed for years.  Now block_func must be unexceptional.  If block_func
# is supposed to raise an exception, call do_exceptional_blocking_test()
# instead.

class BlockingTestMixin:

    def do_blocking_test(self, block_func, block_args, trigger_func, trigger_args):
        self.t = _TriggerThread(trigger_func, trigger_args)
        self.t.start()
        self.result = block_func(*block_args)
        # If block_func returned before our thread made the call, we failed!
        if not self.t.startedEvent.isSet():
            self.fail("blocking function '%r' appeared not to block" %
                      block_func)
        self.t.join(10) # make sure the thread terminates
        if self.t.isAlive():
            self.fail("trigger function '%r' appeared to not return" %
                      trigger_func)
        return self.result

    # Call this instead if block_func is supposed to raise an exception.
    def do_exceptional_blocking_test(self,block_func, block_args, trigger_func,
                                   trigger_args, expected_exception_class):
        self.t = _TriggerThread(trigger_func, trigger_args)
        self.t.start()
        try:
            try:
                block_func(*block_args)
            except expected_exception_class:
                raise
            else:
                self.fail("expected exception of kind %r" %
                                 expected_exception_class)
        finally:
            self.t.join(10) # make sure the thread terminates
            if self.t.isAlive():
                self.fail("trigger function '%r' appeared to not return" %
                                 trigger_func)
            if not self.t.startedEvent.isSet():
                self.fail("trigger thread ended but event never set")


class BaseQueueTest(unittest.TestCase, BlockingTestMixin):
    def setUp(self):
        self.cum = 0
        self.cumlock = threading.Lock()

    def simple_queue_test(self, q):
        if not q.empty():
            raise RuntimeError, "Call this function with an empty queue"
        # I guess we better check things actually queue correctly a little :)
        q.put(111)
        q.put(333)
        q.put(222)
        target_order = dict(Queue = [111, 333, 222],
                            LifoQueue = [222, 333, 111],
                            PriorityQueue = [111, 222, 333])
        actual_order = [q.get(), q.get(), q.get()]
        self.assertEquals(actual_order, target_order[q.__class__.__name__],
                          "Didn't seem to queue the correct data!")
        for i in range(QUEUE_SIZE-1):
            q.put(i)
            self.assert_(not q.empty(), "Queue should not be empty")
        self.assert_(not q.full(), "Queue should not be full")
        q.put("last")
        self.assert_(q.full(), "Queue should be full")
        try:
            q.put("full", block=0)
            self.fail("Didn't appear to block with a full queue")
        except Queue.Full:
            pass
        try:
            q.put("full", timeout=0.01)
            self.fail("Didn't appear to time-out with a full queue")
        except Queue.Full:
            pass
        # Test a blocking put
        self.do_blocking_test(q.put, ("full",), q.get, ())
        self.do_blocking_test(q.put, ("full", True, 10), q.get, ())
        # Empty it
        for i in range(QUEUE_SIZE):
            q.get()
        self.assert_(q.empty(), "Queue should be empty")
        try:
            q.get(block=0)
            self.fail("Didn't appear to block with an empty queue")
        except Queue.Empty:
            pass
        try:
            q.get(timeout=0.01)
            self.fail("Didn't appear to time-out with an empty queue")
        except Queue.Empty:
            pass
        # Test a blocking get
        self.do_blocking_test(q.get, (), q.put, ('empty',))
        self.do_blocking_test(q.get, (True, 10), q.put, ('empty',))


    def worker(self, q):
        while True:
            x = q.get()
            if x is None:
                q.task_done()
                return
            self.cumlock.acquire()
            try:
                self.cum += x
            finally:
                self.cumlock.release()
            q.task_done()

    def queue_join_test(self, q):
        self.cum = 0
        for i in (0,1):
            threading.Thread(target=self.worker, args=(q,)).start()
        for i in xrange(100):
            q.put(i)
        q.join()
        self.assertEquals(self.cum, sum(range(100)),
                          "q.join() did not block until all tasks were done")
        for i in (0,1):
            q.put(None)         # instruct the threads to close
        q.join()                # verify that you can join twice

    def test_queue_task_done(self):
        # Test to make sure a queue task completed successfully.
        q = self.type2test()
        try:
            q.task_done()
        except ValueError:
            pass
        else:
            self.fail("Did not detect task count going negative")

    def test_queue_join(self):
        # Test that a queue join()s successfully, and before anything else
        # (done twice for insurance).
        q = self.type2test()
        self.queue_join_test(q)
        self.queue_join_test(q)
        try:
            q.task_done()
        except ValueError:
            pass
        else:
            self.fail("Did not detect task count going negative")

    def test_simple_queue(self):
        # Do it a couple of times on the same queue.
        # Done twice to make sure works with same instance reused.
        q = self.type2test(QUEUE_SIZE)
        self.simple_queue_test(q)
        self.simple_queue_test(q)


class QueueTest(BaseQueueTest):
    type2test = Queue.Queue

class LifoQueueTest(BaseQueueTest):
    type2test = Queue.LifoQueue

class PriorityQueueTest(BaseQueueTest):
    type2test = Queue.PriorityQueue



# A Queue subclass that can provoke failure at a moment's notice :)
class FailingQueueException(Exception):
    pass

class FailingQueue(Queue.Queue):
    def __init__(self, *args):
        self.fail_next_put = False
        self.fail_next_get = False
        Queue.Queue.__init__(self, *args)
    def _put(self, item):
        if self.fail_next_put:
            self.fail_next_put = False
            raise FailingQueueException, "You Lose"
        return Queue.Queue._put(self, item)
    def _get(self):
        if self.fail_next_get:
            self.fail_next_get = False
            raise FailingQueueException, "You Lose"
        return Queue.Queue._get(self)

class FailingQueueTest(unittest.TestCase, BlockingTestMixin):

    def failing_queue_test(self, q):
        if not q.empty():
            raise RuntimeError, "Call this function with an empty queue"
        for i in range(QUEUE_SIZE-1):
            q.put(i)
        # Test a failing non-blocking put.
        q.fail_next_put = True
        try:
            q.put("oops", block=0)
            self.fail("The queue didn't fail when it should have")
        except FailingQueueException:
            pass
        q.fail_next_put = True
        try:
            q.put("oops", timeout=0.1)
            self.fail("The queue didn't fail when it should have")
        except FailingQueueException:
            pass
        q.put("last")
        self.assert_(q.full(), "Queue should be full")
        # Test a failing blocking put
        q.fail_next_put = True
        try:
            self.do_blocking_test(q.put, ("full",), q.get, ())
            self.fail("The queue didn't fail when it should have")
        except FailingQueueException:
            pass
        # Check the Queue isn't damaged.
        # put failed, but get succeeded - re-add
        q.put("last")
        # Test a failing timeout put
        q.fail_next_put = True
        try:
            self.do_exceptional_blocking_test(q.put, ("full", True, 10), q.get, (),
                                              FailingQueueException)
            self.fail("The queue didn't fail when it should have")
        except FailingQueueException:
            pass
        # Check the Queue isn't damaged.
        # put failed, but get succeeded - re-add
        q.put("last")
        self.assert_(q.full(), "Queue should be full")
        q.get()
        self.assert_(not q.full(), "Queue should not be full")
        q.put("last")
        self.assert_(q.full(), "Queue should be full")
        # Test a blocking put
        self.do_blocking_test(q.put, ("full",), q.get, ())
        # Empty it
        for i in range(QUEUE_SIZE):
            q.get()
        self.assert_(q.empty(), "Queue should be empty")
        q.put("first")
        q.fail_next_get = True
        try:
            q.get()
            self.fail("The queue didn't fail when it should have")
        except FailingQueueException:
            pass
        self.assert_(not q.empty(), "Queue should not be empty")
        q.fail_next_get = True
        try:
            q.get(timeout=0.1)
            self.fail("The queue didn't fail when it should have")
        except FailingQueueException:
            pass
        self.assert_(not q.empty(), "Queue should not be empty")
        q.get()
        self.assert_(q.empty(), "Queue should be empty")
        q.fail_next_get = True
        try:
            self.do_exceptional_blocking_test(q.get, (), q.put, ('empty',),
                                              FailingQueueException)
            self.fail("The queue didn't fail when it should have")
        except FailingQueueException:
            pass
        # put succeeded, but get failed.
        self.assert_(not q.empty(), "Queue should not be empty")
        q.get()
        self.assert_(q.empty(), "Queue should be empty")

    def test_failing_queue(self):
        # Test to make sure a queue is functioning correctly.
        # Done twice to the same instance.
        q = FailingQueue(QUEUE_SIZE)
        self.failing_queue_test(q)
        self.failing_queue_test(q)


def test_main():
    test_support.run_unittest(QueueTest, LifoQueueTest, PriorityQueueTest,
                              FailingQueueTest)


if __name__ == "__main__":
    test_main()
