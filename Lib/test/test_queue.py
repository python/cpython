# Some simple Queue module tests, plus some failure conditions
# to ensure the Queue locks remain stable.
import Queue
import sys
import threading
import time

from test.test_support import verify, TestFailed, verbose

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
# triggers the release.  Returns the result of the blocking function.
# Caution:  block_func must guarantee to block until trigger_func is
# called, and trigger_func must guarantee to change queue state so that
# block_func can make enough progress to return.  In particular, a
# block_func that just raises an exception regardless of whether trigger_func
# is called will lead to timing-dependent sporadic failures, and one of
# those went rarely seen but undiagnosed for years.  Now block_func
# must be unexceptional.  If block_func is supposed to raise an exception,
# call _doExceptionalBlockingTest() instead.
def _doBlockingTest(block_func, block_args, trigger_func, trigger_args):
    t = _TriggerThread(trigger_func, trigger_args)
    t.start()
    result = block_func(*block_args)
    # If block_func returned before our thread made the call, we failed!
    if not t.startedEvent.isSet():
        raise TestFailed("blocking function '%r' appeared not to block" %
                         block_func)
    t.join(10) # make sure the thread terminates
    if t.isAlive():
        raise TestFailed("trigger function '%r' appeared to not return" %
                         trigger_func)
    return result

# Call this instead if block_func is supposed to raise an exception.
def _doExceptionalBlockingTest(block_func, block_args, trigger_func,
                               trigger_args, expected_exception_class):
    t = _TriggerThread(trigger_func, trigger_args)
    t.start()
    try:
        try:
            block_func(*block_args)
        except expected_exception_class:
            raise
        else:
            raise TestFailed("expected exception of kind %r" %
                             expected_exception_class)
    finally:
        t.join(10) # make sure the thread terminates
        if t.isAlive():
            raise TestFailed("trigger function '%r' appeared to not return" %
                             trigger_func)
        if not t.startedEvent.isSet():
            raise TestFailed("trigger thread ended but event never set")

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
    for i in range(QUEUE_SIZE-1):
        q.put(i)
    # Test a failing non-blocking put.
    q.fail_next_put = True
    try:
        q.put("oops", block=0)
        raise TestFailed("The queue didn't fail when it should have")
    except FailingQueueException:
        pass
    q.fail_next_put = True
    try:
        q.put("oops", timeout=0.1)
        raise TestFailed("The queue didn't fail when it should have")
    except FailingQueueException:
        pass
    q.put("last")
    verify(q.full(), "Queue should be full")
    # Test a failing blocking put
    q.fail_next_put = True
    try:
        _doBlockingTest(q.put, ("full",), q.get, ())
        raise TestFailed("The queue didn't fail when it should have")
    except FailingQueueException:
        pass
    # Check the Queue isn't damaged.
    # put failed, but get succeeded - re-add
    q.put("last")
    # Test a failing timeout put
    q.fail_next_put = True
    try:
        _doExceptionalBlockingTest(q.put, ("full", True, 10), q.get, (),
                                   FailingQueueException)
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
    for i in range(QUEUE_SIZE):
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
    q.fail_next_get = True
    try:
        q.get(timeout=0.1)
        raise TestFailed("The queue didn't fail when it should have")
    except FailingQueueException:
        pass
    verify(not q.empty(), "Queue should not be empty")
    q.get()
    verify(q.empty(), "Queue should be empty")
    q.fail_next_get = True
    try:
        _doExceptionalBlockingTest(q.get, (), q.put, ('empty',),
                                   FailingQueueException)
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
    verify(q.get() == 111 and q.get() == 222,
           "Didn't seem to queue the correct data!")
    for i in range(QUEUE_SIZE-1):
        q.put(i)
        verify(not q.empty(), "Queue should not be empty")
    verify(not q.full(), "Queue should not be full")
    q.put("last")
    verify(q.full(), "Queue should be full")
    try:
        q.put("full", block=0)
        raise TestFailed("Didn't appear to block with a full queue")
    except Queue.Full:
        pass
    try:
        q.put("full", timeout=0.01)
        raise TestFailed("Didn't appear to time-out with a full queue")
    except Queue.Full:
        pass
    # Test a blocking put
    _doBlockingTest(q.put, ("full",), q.get, ())
    _doBlockingTest(q.put, ("full", True, 10), q.get, ())
    # Empty it
    for i in range(QUEUE_SIZE):
        q.get()
    verify(q.empty(), "Queue should be empty")
    try:
        q.get(block=0)
        raise TestFailed("Didn't appear to block with an empty queue")
    except Queue.Empty:
        pass
    try:
        q.get(timeout=0.01)
        raise TestFailed("Didn't appear to time-out with an empty queue")
    except Queue.Empty:
        pass
    # Test a blocking get
    _doBlockingTest(q.get, (), q.put, ('empty',))
    _doBlockingTest(q.get, (True, 10), q.put, ('empty',))

def test():
    q = Queue.Queue(QUEUE_SIZE)
    # Do it a couple of times on the same queue
    SimpleQueueTest(q)
    SimpleQueueTest(q)
    if verbose:
        print "Simple Queue tests seemed to work"
    q = FailingQueue(QUEUE_SIZE)
    FailingQueueTest(q)
    FailingQueueTest(q)
    if verbose:
        print "Failing Queue tests seemed to work"

test()
