"""A multi-producer, multi-consumer queue."""

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
        import thread
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
        """Return 1 if the queue is empty, 0 otherwise (not reliable!)."""
        self.mutex.acquire()
        n = self._empty()
        self.mutex.release()
        return n

    def full(self):
        """Return 1 if the queue is full, 0 otherwise (not reliable!)."""
        self.mutex.acquire()
        n = self._full()
        self.mutex.release()
        return n

    def put(self, item, block=1):
        """Put an item into the queue.

        If optional arg 'block' is 1 (the default), block if
        necessary until a free slot is available.  Otherwise (block
        is 0), put an item on the queue if a free slot is immediately
        available, else raise the Full exception.
        """
        if block:
            self.fsema.acquire()
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
        return self.put(item, 0)

    def get(self, block=1):
        """Remove and return an item from the queue.

        If optional arg 'block' is 1 (the default), block if
        necessary until an item is available.  Otherwise (block is 0),
        return an item if one is immediately available, else raise the
        Empty exception.
        """
        if block:
            self.esema.acquire()
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

        Only get an item if one is immediately available.  Otherwise
        raise the Empty exception.
        """
        return self.get(0)

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
        item = self.queue[0]
        del self.queue[0]
        return item
