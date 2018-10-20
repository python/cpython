'''A multi-producer, multi-consumer queue.'''

import threading
from collections import deque
from heapq import heappush, heappop
from time import monotonic as time
try:
    from _queue import SimpleQueue
except ImportError:
    SimpleQueue = None

__all__ = ['Empty', 'Full', 'Closed', 'Exhausted',
           'Queue', 'PriorityQueue', 'LifoQueue', 'SimpleQueue']


try:
    from _queue import Empty
except AttributeError:
    class Empty(Exception):
        'Exception raised by Queue.get(block=0)/get_nowait().'
        pass

class Exhausted(Empty):
    'Exception raised by Queue.get/get_nowait().'
    pass

class Full(Exception):
    'Exception raised by Queue.put(block=0)/put_nowait().'
    pass

class Closed(Full):
    'Exception raised by Queue.put/put_nowait().'
    pass

class Queue:
    '''Create a queue object with a given maximum size.

    If maxsize is <= 0, the queue size is infinite.
    '''

    def __init__(self, maxsize=0):
        self.maxsize = maxsize
        self._init(maxsize)

        # mutex must be held whenever the queue is mutating.  All methods
        # that acquire mutex must release it before returning.  mutex
        # is shared between the three conditions, so acquiring and
        # releasing the conditions also acquires and releases mutex.
        self.mutex = threading.RLock()

        # Event waited upon by threads trying to get. Notify one thread
        # when an item is added to the queue; notify all when the queue is
        # exhausted (and thus no get operations can ever succeed anymore).
        self.exhausted_or_non_empty = threading.Condition(self.mutex)

        # Event waited upon by threads trying to put. Notify one thread
        # when an item is removed from the queue; notify all when the
        # queue is closed (and thus no put operations can ever succeed
        # anymore).
        self.closed_or_non_full = threading.Condition(self.mutex)

        self._closed = False

        # Notify all_tasks_done whenever the number of unfinished tasks
        # drops to zero; thread waiting to join() is notified to resume
        self.all_tasks_done = threading.Condition(self.mutex)
        self.unfinished_tasks = 0

    def task_done(self):
        '''Indicate that a formerly enqueued task is complete.

        Used by Queue consumer threads.  For each get() used to fetch a task,
        a subsequent call to task_done() tells the queue that the processing
        on the task is complete.

        If a join() is currently blocking, it will resume when all items
        have been processed (meaning that a task_done() call was received
        for every item that had been put() into the queue).

        Raises a ValueError if called more times than there were items
        placed in the queue.
        '''
        with self.all_tasks_done:
            unfinished = self.unfinished_tasks - 1
            if unfinished <= 0:
                if unfinished < 0:
                    raise ValueError('task_done() called too many times')
                self.all_tasks_done.notify_all()
            self.unfinished_tasks = unfinished

    def join(self):
        '''Blocks until all items in the Queue have been gotten and processed.

        The count of unfinished tasks goes up whenever an item is added to the
        queue. The count goes down whenever a consumer thread calls task_done()
        to indicate the item was retrieved and all work on it is complete.

        When the count of unfinished tasks drops to zero, join() unblocks.
        '''
        with self.all_tasks_done:
            while self.unfinished_tasks:
                self.all_tasks_done.wait()

    def qsize(self):
        '''Return the approximate size of the queue (not reliable!).'''
        with self.mutex:
            return self._qsize()

    def empty(self):
        '''Return True if the queue is empty, False otherwise (not reliable!).

        This method is likely to be removed at some point.  Use qsize() == 0
        as a direct substitute, but be aware that either approach risks a race
        condition where a queue can change from empty to non-empty or vice
        versa before the result of empty() or qsize() can be used.

        To create code that needs to wait for all queued tasks to be
        completed, the preferred technique is to use the join() method.
        '''
        with self.mutex:
            return not self._qsize()

    def closed(self):
        with self.mutex:
            return self._closed

    def exhausted(self):
        with self.mutex:
            return self._closed and not self._qsize()

    def full(self):
        '''Return True if the queue is full, False otherwise (not reliable!).

        A queue is considered full if, at the moment of the call, it has no
        free slots that could accept a new item. Therefore, closed queues are
        always full, and it is possible for a closed queue to be
        simultaneously empty and full.

        As with empty() and qsize(), a race condition is possible where a
        queue's state can change from full to non-full or vice versa before
        the result of full() can be used.
        '''
        with self.mutex:
            return self._closed or 0 < self.maxsize <= self._qsize()

    def put(self, item, block=True, timeout=None):
        '''Put an item into the queue.

        If optional args 'block' is true and 'timeout' is None (the default),
        block if necessary until a free slot is available or the queue is
        closed; in the latter case, raise a :exc:`Closed` exception.

        If 'block' is true and 'timeout' is a positive number, block at most
        'timeout' seconds and raise the Full exception if no free slot
        becomes available within that time, or the Closed exception if the
        queue gets closed before any slots are available.

        If 'block' is false, ignore 'timeout' and put an item on the
        queue if a free slot is immediately available, else raise the Full or
        Closed exception, depending on whether the queue is closed.
        (Note that Closed is a subclass of Full.)
        '''
        with self.closed_or_non_full:
            if self._closed:
                raise Closed
            if self.maxsize > 0:
                if not block:
                    if self._qsize() >= self.maxsize:
                        raise Full
                elif timeout is None:
                    while self._qsize() >= self.maxsize:
                        self.closed_or_non_full.wait()
                        if self._closed:
                            raise Closed
                elif timeout < 0:
                    raise ValueError("'timeout' must be a non-negative number")
                else:
                    endtime = time() + timeout
                    while self._qsize() >= self.maxsize:
                        remaining = endtime - time()
                        if remaining <= 0.0:
                            raise Full
                        self.closed_or_non_full.wait(remaining)
                        if self._closed:
                            raise Closed
            self._put(item)
            self.unfinished_tasks += 1
            self.exhausted_or_non_empty.notify()

    def get(self, block=True, timeout=None):
        '''Remove and return an item from the queue.

        If optional args 'block' is true and 'timeout' is None (the default),
        block if necessary until an item is available or the queue is closed.
        If the queue is closed and no items are available, raise the
        Exhausted exception.

        If 'block' is true and 'timeout' is a positive number, block at most
        'timeout' seconds and raise the Empty exception if no item becomes
        available within that time, or the Exhausted exception if the queue
        gets closed before any items are available.

        If 'block' is false, ignore 'timeout' and return an item if one is
        immediately available, else raise the Empty or Exhausted
        exception, depending on whether the queue is closed.
        (Note that Exhausted is a subclass of Empty.)
        '''
        with self.exhausted_or_non_empty:
            if self.exhausted():
                raise Exhausted
            if not block:
                if not self._qsize():
                    raise Empty
            elif timeout is None:
                while not self._qsize():
                    self.exhausted_or_non_empty.wait()
                    if self.exhausted():
                        raise Exhausted
            elif timeout < 0:
                raise ValueError("'timeout' must be a non-negative number")
            else:
                endtime = time() + timeout
                while not self._qsize():
                    remaining = endtime - time()
                    if remaining <= 0.0:
                        raise Empty
                    self.exhausted_or_non_empty.wait(remaining)
                    if self.exhausted():
                        raise Exhausted
            item = self._get()
            self.closed_or_non_full.notify()
            if self.exhausted():  # Was this the last item ever?
                self.exhausted_or_non_empty.notify_all()
            return item

    def put_nowait(self, item):
        '''Put an item into the queue without blocking.

        Only enqueue the item if a free slot is immediately available.
        Otherwise raise the Full exception, or Closed if the queue is
        closed.
        '''
        return self.put(item, block=False)

    def get_nowait(self):
        '''Remove and return an item from the queue without blocking.

        Only get an item if one is immediately available. Otherwise
        raise the Empty exception, or Exhausted if the queue is
        exhausted.
        '''
        return self.get(block=False)

    def close(self):
        '''Close the queue.

        Once closed, the queue does not accept new items and can not be
        reopened. Closing immediately aborts any currently-blocking put()
        calls; if the queue is empty at the moment of closing, it also
        aborts any currently-blocking get() calls.
        Repeated calls to close() after the first one do nothing.
        '''
        with self.mutex:
            if self._closed:
                return
            self._closed = True
            self.closed_or_non_full.notify_all()
            if self.empty():
                self.exhausted_or_non_empty.notify_all()

    def __iter__(self):
        '''Iterate through (blocking) retrieval of items from the queue.

        The iteration stops when the queue is exhausted.
        '''
        while True:
            try:
                yield self.get()
            except Exhausted:
                break

    # Override these methods to implement other queue organizations
    # (e.g. stack or priority queue).
    # These will only be called with appropriate locks held

    # Initialize the queue representation
    def _init(self, maxsize):
        self.queue = deque()

    def _qsize(self):
        return len(self.queue)

    # Put a new item in the queue
    def _put(self, item):
        self.queue.append(item)

    # Get an item from the queue
    def _get(self):
        return self.queue.popleft()


class PriorityQueue(Queue):
    '''Variant of Queue that retrieves open entries in priority order (lowest first).

    Entries are typically tuples of the form:  (priority number, data).
    '''

    def _init(self, maxsize):
        self.queue = []

    def _qsize(self):
        return len(self.queue)

    def _put(self, item):
        heappush(self.queue, item)

    def _get(self):
        return heappop(self.queue)


class LifoQueue(Queue):
    '''Variant of Queue that retrieves most recently added entries first.'''

    def _init(self, maxsize):
        self.queue = []

    def _qsize(self):
        return len(self.queue)

    def _put(self, item):
        self.queue.append(item)

    def _get(self):
        return self.queue.pop()


class _PySimpleQueue:
    '''Simple, unbounded FIFO queue.

    This pure Python implementation is not reentrant.
    '''
    # Note: while this pure Python version provides fairness
    # (by using a threading.Semaphore which is itself fair, being based
    #  on threading.Condition), fairness is not part of the API contract.
    # This allows the C version to use a different implementation.

    def __init__(self):
        self._queue = deque()
        self._count = threading.Semaphore(0)

    def put(self, item, block=True, timeout=None):
        '''Put the item on the queue.

        The optional 'block' and 'timeout' arguments are ignored, as this method
        never blocks.  They are provided for compatibility with the Queue class.
        '''
        self._queue.append(item)
        self._count.release()

    def get(self, block=True, timeout=None):
        '''Remove and return an item from the queue.

        If optional args 'block' is true and 'timeout' is None (the default),
        block if necessary until an item is available. If 'timeout' is
        a non-negative number, it blocks at most 'timeout' seconds and raises
        the Empty exception if no item was available within that time.
        Otherwise ('block' is false), return an item if one is immediately
        available, else raise the Empty exception ('timeout' is ignored
        in that case).
        '''
        if timeout is not None and timeout < 0:
            raise ValueError("'timeout' must be a non-negative number")
        if not self._count.acquire(block, timeout):
            raise Empty
        return self._queue.popleft()

    def put_nowait(self, item):
        '''Put an item into the queue without blocking.

        This is exactly equivalent to `put(item)` and is only provided
        for compatibility with the Queue class.
        '''
        return self.put(item, block=False)

    def get_nowait(self):
        '''Remove and return an item from the queue without blocking.

        Only get an item if one is immediately available. Otherwise
        raise the Empty exception.
        '''
        return self.get(block=False)

    def empty(self):
        '''Return True if the queue is empty, False otherwise (not reliable!).'''
        return len(self._queue) == 0

    def qsize(self):
        '''Return the approximate size of the queue (not reliable!).'''
        return len(self._queue)


if SimpleQueue is None:
    SimpleQueue = _PySimpleQueue
