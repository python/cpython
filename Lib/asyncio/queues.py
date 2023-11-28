__all__ = ('Queue', 'PriorityQueue', 'LifoQueue', 'QueueFull', 'QueueEmpty',
           'QueueFullWithPendingPutTasks', 'QueueEmptyWithPendingGetTasks')

import collections
import heapq
from types import GenericAlias

from . import locks
from . import mixins


class QueueEmpty(Exception):
    """Raised when Queue.get_nowait() is called on an empty Queue."""
    pass

class QueueEmptyWithPendingGetTasks(QueueEmpty):
    """Raised when the Queue.get_nowait() method is called on not empty Queue._getters"""
    pass

class QueueFull(Exception):
    """Raised when the Queue.put_nowait() method is called on a full Queue."""
    pass

class QueueFullWithPendingPutTasks(QueueFull):
    """Raised when the Queue.put_nowait() method is called on not empty Queue._putters"""
    pass

class Queue(mixins._LoopBoundMixin):
    """A queue, useful for coordinating producer and consumer coroutines.

    If maxsize is less than or equal to zero, the queue size is infinite. If it
    is an integer greater than 0, then "await put()" will block when the
    queue reaches maxsize, until an item is removed by get().

    Unlike the standard library Queue, you can reliably know this Queue's size
    with qsize(), since your single-threaded asyncio application won't be
    interrupted between calling qsize() and doing an operation on the Queue.
    """

    def __init__(self, maxsize=0):
        self._maxsize = maxsize

        # Futures.
        self._getters = collections.deque()
        # Futures.
        self._putters = collections.deque()
        self._unfinished_tasks = 0
        self._finished = locks.Event()
        self._finished.set()
        self._init(maxsize)

    # These three are overridable in subclasses.

    def _init(self, maxsize):
        self._queue = collections.deque()

    def _get(self):
        return self._queue.popleft()

    def _put(self, item):
        self._queue.append(item)

    # End of the overridable methods.

    def _wakeup_next(self, waiters):
        # Wake up the next waiter (if any) that isn't cancelled.
        # from gh-83055: don't remove future from waiters,
        # do it in get() or put() when `waiter` wake up.
        for waiter in waiters:
            if not waiter.done():
                waiter.set_result(None)
                break

    def __repr__(self):
        return f'<{type(self).__name__} at {id(self):#x} {self._format()}>'

    def __str__(self):
        return f'<{type(self).__name__} {self._format()}>'

    __class_getitem__ = classmethod(GenericAlias)

    def _format(self):
        result = f'maxsize={self._maxsize!r}'
        if getattr(self, '_queue', None):
            result += f' _queue={list(self._queue)!r}'
        if self._getters:
            result += f' _getters[{len(self._getters)}]'
        if self._putters:
            result += f' _putters[{len(self._putters)}]'
        if self._unfinished_tasks:
            result += f' tasks={self._unfinished_tasks}'
        return result

    def qsize(self):
        """Number of items in the queue."""
        return len(self._queue)

    @property
    def maxsize(self):
        """Number of items allowed in the queue."""
        return self._maxsize

    def empty(self):
        """Return True if the queue is empty, False otherwise."""
        return not self._queue

    def full(self):
        """Return True if there are maxsize items in the queue.

        Note: if the Queue was initialized with maxsize=0 (the default),
        then full() is never True.
        """
        if self._maxsize <= 0:
            return False
        else:
            return self.qsize() >= self._maxsize

    async def put(self, item):
        """Put an item into the queue.

        Put an item into the queue. If the queue is full, wait until a free
        slot is available before adding item. If there are still item moving
        to a free or waiting for free slots, wait for its time.
        """
        if self.full() or self._putters:
            putter = self._get_loop().create_future()
            self._putters.append(putter)
            try:
                await putter
                # see gh-83055, clean self._putters.
                self._putters.remove(putter)
            except:
                putter.cancel()  # Just in case putter is not done yet.
                try:
                    # Clean self._putters from canceled putters.
                    self._putters.remove(putter)
                except ValueError:
                    # The putter could be removed from self._putters by a
                    # previous get_nowait call.
                    pass
                if not self.full() and not putter.cancelled():
                    # We were woken up by get_nowait(), but can't take
                    # the call.  Wake up the next in line.
                    self._wakeup_next(self._putters)
                raise
        # Don't run tests from self._put_nowait().
        return self._put_and_wakeup_next(item)

    def put_nowait(self, item):
        """Put an item into the queue without blocking.

        If no free slot is immediately available, raise QueueFull.
        If there are pending put tasks, raise QueueFullWithPendingPutTasks.
        """
        if self.full():
            raise QueueFull
        if self._putters:
            raise QueueFullWithPendingPutTasks
        self._put_and_wakeup_next(item)

    def _put_and_wakeup_next(self, item):
        """Put an item into the queue.

        Wakes up next tasks waiting for slots.
        """
        self._put(item)
        self._unfinished_tasks += 1
        self._finished.clear()
        self._wakeup_next(self._getters)

    async def get(self):
        """Remove and return an item from the queue.

        If queue is empty, wait until an item is available.
        If queue is not empty and there are still tasks waiting for items,
        task has to wait for its time.
        """
        if self.empty() or self._getters:
            getter = self._get_loop().create_future()
            self._getters.append(getter)
            try:
                await getter
                # see gh-83055, clean self._getters.
                self._getters.remove(getter)
            except:
                getter.cancel()  # Just in case getter is not done yet.
                try:
                    # Clean self._getters from canceled getters.
                    self._getters.remove(getter)
                except ValueError:
                    # The getter could be removed from self._getters by a
                    # previous put_nowait call.
                    pass
                if not self.empty() and not getter.cancelled():
                    # We were woken up by put_nowait(), but can't take
                    # the call.  Wake up the next in line.
                    self._wakeup_next(self._getters)
                raise
        # Don't run tests from self._get_nowait().
        return self._get_and_wakeup_next()

    def get_nowait(self):
        """Remove and return an item from the queue.

        Return an item if one is immediately available, else raise QueueEmpty
        Return an item if there is no pending get task, else raise QueueEmptyWithPendingGetTasks
        """
        if self.empty():
            raise QueueEmpty
        if self._getters:
            raise QueueEmptyWithPendingGetTasks
        return self._get_and_wakeup_next()

    def _get_and_wakeup_next(self):
        """Remove and return an item from the queue.

        Wakes up next tasks waiting for slots.
        """
        item = self._get()
        self._wakeup_next(self._putters)
        return item

    def task_done(self):
        """Indicate that a formerly enqueued task is complete.

        Used by queue consumers. For each get() used to fetch a task,
        a subsequent call to task_done() tells the queue that the processing
        on the task is complete.

        If a join() is currently blocking, it will resume when all items have
        been processed (meaning that a task_done() call was received for every
        item that had been put() into the queue).

        Raises ValueError if called more times than there were items placed in
        the queue.
        """
        if self._unfinished_tasks <= 0:
            raise ValueError('task_done() called too many times')
        self._unfinished_tasks -= 1
        if self._unfinished_tasks == 0:
            self._finished.set()

    async def join(self):
        """Block until all items in the queue have been gotten and processed.

        The count of unfinished tasks goes up whenever an item is added to the
        queue. The count goes down whenever a consumer calls task_done() to
        indicate that the item was retrieved and all work on it is complete.
        When the count of unfinished tasks drops to zero, join() unblocks.
        """
        if self._unfinished_tasks > 0:
            await self._finished.wait()


class PriorityQueue(Queue):
    """A subclass of Queue; retrieves entries in priority order (lowest first).

    Entries are typically tuples of the form: (priority number, data).
    """

    def _init(self, maxsize):
        self._queue = []

    def _put(self, item, heappush=heapq.heappush):
        heappush(self._queue, item)

    def _get(self, heappop=heapq.heappop):
        return heappop(self._queue)


class LifoQueue(Queue):
    """A subclass of Queue that retrieves most recently added entries first."""

    def _init(self, maxsize):
        self._queue = []

    def _put(self, item):
        self._queue.append(item)

    def _get(self):
        return self._queue.pop()
