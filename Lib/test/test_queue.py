# Some simple Queue module tests, plus some failure conditions
# to ensure the Queue locks remain stable
import Queue
import sys
import threading
import time

from test_support import verify, TestFailed, verbose

queue_size = 5

# Execute a function that blocks, and in a seperate thread, a function that
# triggers the release.  Returns the result of the blocking function.
class _TriggerThread(threading.Thread):
    def __init__(self, fn, args):
        self.fn = fn
        self.args = args
        self.startedEvent = threading.Event()
        threading.Thread.__init__(self)
    def run(self):
        time.sleep(.1)
        self.startedEvent.set()
        self.fn(*self.args)

def _doBlockingTest( block_func, block_args, trigger_func, trigger_args):
    t = _TriggerThread(trigger_func, trigger_args)
    t.start()
    try:
        return block_func(*block_args)
    finally:
        # If we unblocked before our thread made the call, we failed!
        if not t.startedEvent.isSet():
            raise TestFailed("blocking function '%r' appeared not to block" % (block_func,))
        t.join(1) # make sure the thread terminates
        if t.isAlive():
            raise TestFailed("trigger function '%r' appeared to not return" % (trigger_func,))

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

def FailingQueueTest(q):
    if not q.empty():
        raise RuntimeError, "Call this function with an empty queue"
    for i in range(queue_size-1):
        q.put(i)
    q.fail_next_put = True
    # Test a failing non-blocking put.
    try:
        q.put("oops", block=0)
        raise TestFailed("The queue didn't fail when it should have")
    except FailingQueueException:
        pass
    q.put("last")
    verify(q.full(), "Queue should be full")
    q.fail_next_put = True
    # Test a failing blocking put
    try:
        _doBlockingTest( q.put, ("full",), q.get, ())
        raise TestFailed("The queue didn't fail when it should have")
    except FailingQueueException:
        pass
    # Check the Queue isn't damaged.
    # put failed, but get succeeded - re-add
    q.put("last")
    verify(q.full(), "Queue should be full")
    q.get()
    verify(not q.full(), "Queue should not be full")
    q.put("last")
    verify(q.full(), "Queue should be full")
    # Test a blocking put
    _doBlockingTest( q.put, ("full",), q.get, ())
    # Empty it
    for i in range(queue_size):
        q.get()
    verify(q.empty(), "Queue should be empty")
    q.put("first")
    q.fail_next_get = True
    try:
        q.get()
        raise TestFailed("The queue didn't fail when it should have")
    except FailingQueueException:
        pass
    verify(not q.empty(), "Queue should not be empty")
    q.get()
    verify(q.empty(), "Queue should be empty")
    q.fail_next_get = True
    try:
        _doBlockingTest( q.get, (), q.put, ('empty',))
        raise TestFailed("The queue didn't fail when it should have")
    except FailingQueueException:
        pass
    # put succeeded, but get failed.
    verify(not q.empty(), "Queue should not be empty")
    q.get()
    verify(q.empty(), "Queue should be empty")

def SimpleQueueTest(q):
    if not q.empty():
        raise RuntimeError, "Call this function with an empty queue"
    # I guess we better check things actually queue correctly a little :)
    q.put(111)
    q.put(222)
    verify(q.get()==111 and q.get()==222, "Didn't seem to queue the correct data!")
    for i in range(queue_size-1):
        q.put(i)
    verify(not q.full(), "Queue should not be full")
    q.put("last")
    verify(q.full(), "Queue should be full")
    try:
        q.put("full", block=0)
        raise TestFailed("Didn't appear to block with a full queue")
    except Queue.Full:
        pass
    # Test a blocking put
    _doBlockingTest( q.put, ("full",), q.get, ())
    # Empty it
    for i in range(queue_size):
        q.get()
    verify(q.empty(), "Queue should be empty")
    try:
        q.get(block=0)
        raise TestFailed("Didn't appear to block with an empty queue")
    except Queue.Empty:
        pass
    # Test a blocking get
    _doBlockingTest( q.get, (), q.put, ('empty',))

def test():
    q=Queue.Queue(queue_size)
    # Do it a couple of times on the same queue
    SimpleQueueTest(q)
    SimpleQueueTest(q)
    if verbose:
        print "Simple Queue tests seemed to work"
    q = FailingQueue(queue_size)
    FailingQueueTest(q)
    FailingQueueTest(q)
    if verbose:
        print "Failing Queue tests seemed to work"

test()
