# A multi-producer, multi-consumer queue.

# define this exception to be compatible with Python 1.5's class
# exceptions, but also when -X option is used.
try:
    class Empty(Exception):
        pass
except TypeError:
    # string based exceptions
    Empty = 'Queue.Empty'               # Exception raised by get_nowait()

class Queue:
    def __init__(self, maxsize):
        """Initialize a queue object with a given maximum size.

        If maxsize is <= 0, the queue size is infinite.
        """
        import thread
        self._init(maxsize)
        self.mutex = thread.allocate_lock()
        self.esema = thread.allocate_lock()
        self.esema.acquire_lock()
        self.fsema = thread.allocate_lock()

    def qsize(self):
        """Returns the approximate size of the queue (not reliable!)."""
        self.mutex.acquire_lock()
        n = self._qsize()
        self.mutex.release_lock()
        return n

    def empty(self):
        """Returns 1 if the queue is empty, 0 otherwise (not reliable!)."""
        self.mutex.acquire_lock()
        n = self._empty()
        self.mutex.release_lock()
        return n

    def full(self):
        """Returns 1 if the queue is full, 0 otherwise (not reliable!)."""
        self.mutex.acquire_lock()
        n = self._full()
        self.mutex.release_lock()
        return n

    def put(self, item):
        """Put an item into the queue."""
        self.fsema.acquire_lock()
        self.mutex.acquire_lock()
        was_empty = self._empty()
        self._put(item)
        if was_empty:
            self.esema.release_lock()
        if not self._full():
            self.fsema.release_lock()
        self.mutex.release_lock()

    def get(self):
        """Gets and returns an item from the queue.
        This method blocks if necessary until an item is available.
        """
        self.esema.acquire_lock()
        self.mutex.acquire_lock()
        was_full = self._full()
        item = self._get()
        if was_full:
            self.fsema.release_lock()
        if not self._empty():
            self.esema.release_lock()
        self.mutex.release_lock()
        return item

    # Get an item from the queue if one is immediately available,
    # raise Empty if the queue is empty or temporarily unavailable
    def get_nowait(self):
        """Gets and returns an item from the queue.
        Only gets an item if one is immediately available, Otherwise
        this raises the Empty exception if the queue is empty or
        temporarily unavailable.
        """
        locked = self.esema.acquire_lock(0)
        self.mutex.acquire_lock()
        if self._empty():
            # The queue is empty -- we can't have esema
            self.mutex.release_lock()
            raise Empty
        if not locked:
            locked = self.esema.acquire_lock(0)
            if not locked:
                # Somebody else has esema
                # but we have mutex --
                # go out of their way
                self.mutex.release_lock()
                raise Empty
        was_full = self._full()
        item = self._get()
        if was_full:
            self.fsema.release_lock()
        if not self._empty():
            self.esema.release_lock()
        self.mutex.release_lock()
        return item

    # XXX Need to define put_nowait() as well.


    # Override these methods to implement other queue organizations
    # (e.g. stack or priority queue).
    # These will only be called with appropriate locks held

    # Initialize the queue representation
    def _init(self, maxsize):
        self.maxsize = maxsize
        self.queue = []

    def _qsize(self):
        return len(self.queue)

    # Check wheter the queue is empty
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
