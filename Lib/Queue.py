"""A multi-producer, multi-consumer queue."""

from time import time as _time, sleep as _sleep

__all__ = ['Empty', 'Full', 'Queue']

class Empty(Exception):
    "Exception raised by Queue.get(block=0)/get_nowait()."
    pass

class Full(Exception):
    "Exception raised by Queue.put(block=0)/put_nowait()."
    pass

class Queue:
    def __init__(self, maxsize=0):
        """Initialize a queue object with a given maximum size.

        If maxsize is <= 0, the queue size is infinite.
        """
        try:
            import thread
        except ImportError:
            import dummy_thread as thread
        self._init(maxsize)
        self.mutex = thread.allocate_lock()
        self.esema = thread.allocate_lock()
        self.esema.acquire()
        self.fsema = thread.allocate_lock()

    def qsize(self):
        """Return the approximate size of the queue (not reliable!)."""
        self.mutex.acquire()
        n = self._qsize()
        self.mutex.release()
        return n

    def empty(self):
        """Return True if the queue is empty, False otherwise (not reliable!)."""
        self.mutex.acquire()
        n = self._empty()
        self.mutex.release()
        return n

    def full(self):
        """Return True if the queue is full, False otherwise (not reliable!)."""
        self.mutex.acquire()
        n = self._full()
        self.mutex.release()
        return n

    def put(self, item, block=True, timeout=None):
        """Put an item into the queue.

        If optional args 'block' is true and 'timeout' is None (the default),
        block if necessary until a free slot is available. If 'timeout' is
        a positive number, it blocks at most 'timeout' seconds and raises
        the Full exception if no free slot was available within that time.
        Otherwise ('block' is false), put an item on the queue if a free slot
        is immediately available, else raise the Full exception ('timeout'
        is ignored in that case).
        """
        if block:
            if timeout is None:
                # blocking, w/o timeout, i.e. forever
                self.fsema.acquire()
            elif timeout >= 0:
                # waiting max. 'timeout' seconds.
                # this code snipped is from threading.py: _Event.wait():
                # Balancing act:  We can't afford a pure busy loop, so we
                # have to sleep; but if we sleep the whole timeout time,
                # we'll be unresponsive.  The scheme here sleeps very
                # little at first, longer as time goes on, but never longer
                # than 20 times per second (or the timeout time remaining).
                delay = 0.0005 # 500 us -> initial delay of 1 ms
                endtime = _time() + timeout
                while True:
                    if self.fsema.acquire(0):
                        break
                    remaining = endtime - _time()
                    if remaining <= 0:  #time is over and no slot was free
                        raise Full
                    delay = min(delay * 2, remaining, .05)
                    _sleep(delay)       #reduce CPU usage by using a sleep
            else:
                raise ValueError("'timeout' must be a positive number")
        elif not self.fsema.acquire(0):
            raise Full
        self.mutex.acquire()
        release_fsema = True
        try:
            was_empty = self._empty()
            self._put(item)
            # If we fail before here, the empty state has
            # not changed, so we can skip the release of esema
            if was_empty:
                self.esema.release()
            # If we fail before here, the queue can not be full, so
            # release_full_sema remains True
            release_fsema = not self._full()
        finally:
            # Catching system level exceptions here (RecursionDepth,
            # OutOfMemory, etc) - so do as little as possible in terms
            # of Python calls.
            if release_fsema:
                self.fsema.release()
            self.mutex.release()

    def put_nowait(self, item):
        """Put an item into the queue without blocking.

        Only enqueue the item if a free slot is immediately available.
        Otherwise raise the Full exception.
        """
        return self.put(item, False)

    def get(self, block=True, timeout=None):
        """Remove and return an item from the queue.

        If optional args 'block' is true and 'timeout' is None (the default),
        block if necessary until an item is available. If 'timeout' is
        a positive number, it blocks at most 'timeout' seconds and raises
        the Empty exception if no item was available within that time.
        Otherwise ('block' is false), return an item if one is immediately
        available, else raise the Empty exception ('timeout' is ignored
        in that case).
        """
        if block:
            if timeout is None:
                # blocking, w/o timeout, i.e. forever
                self.esema.acquire()
            elif timeout >= 0:
                # waiting max. 'timeout' seconds.
                # this code snipped is from threading.py: _Event.wait():
                # Balancing act:  We can't afford a pure busy loop, so we
                # have to sleep; but if we sleep the whole timeout time,
                # we'll be unresponsive.  The scheme here sleeps very
                # little at first, longer as time goes on, but never longer
                # than 20 times per second (or the timeout time remaining).
                delay = 0.0005 # 500 us -> initial delay of 1 ms
                endtime = _time() + timeout
                while 1:
                    if self.esema.acquire(0):
                        break
                    remaining = endtime - _time()
                    if remaining <= 0:  #time is over and no element arrived
                        raise Empty
                    delay = min(delay * 2, remaining, .05)
                    _sleep(delay)       #reduce CPU usage by using a sleep
            else:
                raise ValueError("'timeout' must be a positive number")
        elif not self.esema.acquire(0):
            raise Empty
        self.mutex.acquire()
        release_esema = True
        try:
            was_full = self._full()
            item = self._get()
            # If we fail before here, the full state has
            # not changed, so we can skip the release of fsema
            if was_full:
                self.fsema.release()
            # Failure means empty state also unchanged - release_esema
            # remains True.
            release_esema = not self._empty()
        finally:
            if release_esema:
                self.esema.release()
            self.mutex.release()
        return item

    def get_nowait(self):
        """Remove and return an item from the queue without blocking.

        Only get an item if one is immediately available. Otherwise
        raise the Empty exception.
        """
        return self.get(False)

    # Override these methods to implement other queue organizations
    # (e.g. stack or priority queue).
    # These will only be called with appropriate locks held

    # Initialize the queue representation
    def _init(self, maxsize):
        self.maxsize = maxsize
        self.queue = []

    def _qsize(self):
        return len(self.queue)

    # Check whether the queue is empty
    def _empty(self):
        return not self.queue

    # Check whether the queue is full
    def _full(self):
        return self.maxsize > 0 and len(self.queue) == self.maxsize

    # Put a new item in the queue
    def _put(self, item):
        self.queue.append(item)

    # Get an item from the queue
    def _get(self):
        return self.queue.pop(0)
