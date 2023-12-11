"""Cross-interpreter Queues High Level Module."""

import queue
import time
import weakref
import _xxinterpqueues as _queues

# aliases:
from _xxinterpqueues import (
    QueueError, QueueNotFoundError, QueueFull, QueueEmpty,
)

__all__ = [
    'create', 'list_all',
    'Queue',
    'QueueError', 'QueueNotFoundError', 'QueueEmpty', 'QueueFull',
]


def create(maxsize=-1):
    """Return a new cross-interpreter queue.

    The queue may be used to pass data safely between interpreters.
    """
    qid = _queues.create(maxsize)
    return Queue(qid)


def list_all():
    """Return a list of all open queues."""
    return [Queue(qid)
            for qid in _queues.list_all()]



_known_queues = weakref.WeakValueDictionary()

class Queue:
    """A cross-interpreter queue."""

    def __new__(cls, id, /):
        # There is only one instance for any given ID.
        if isinstance(id, int):
            id = int(id)
        else:
            raise TypeError(f'id must be an int, got {id!r}')
        try:
            self = _known_queues[id]
        except KeyError:
            self = super().__new__(cls)
            self._id = id
            _known_queues[id] = self
            _queues.bind(id)
        return self

    def __del__(self):
        try:
            _queues.release(self._id)
        except QueueNotFoundError:
            pass
        try:
            del _known_queues[self._id]
        except KeyError:
            pass

    def __repr__(self):
        return f'{type(self).__name__}({self.id})'

    def __hash__(self):
        return hash(self._id)

    @property
    def id(self):
        return self._id

    @property
    def maxsize(self):
        try:
            return self._maxsize
        except AttributeError:
            self._maxsize = _queues.get_maxsize(self._id)
            return self._maxsize

    def empty(self):
        return self.qsize() == 0

    def full(self):
        return _queues.is_full(self._id)

    def qsize(self):
        return _queues.get_count(self._id)

    def put(self, obj, timeout=None, *,
            _delay=10 / 1000,  # 10 milliseconds
            ):
        """Add the object to the queue.

        This blocks while the queue is full.
        """
        if timeout is not None:
            timeout = int(timeout)
            if timeout < 0:
                raise ValueError(f'timeout value must be non-negative')
            end = time.time() + timeout
        while True:
            try:
                _queues.put(self._id, obj)
            except QueueFull:
                if timeout is not None and time.time() >= end:
                    raise  # re-raise
                time.sleep(_delay)
            else:
                break

    def put_nowait(self, obj):
        return _queues.put(self._id, obj)

    def get(self, timeout=None, *,
            _delay=10 / 1000,  # 10 milliseconds
            ):
        """Return the next object from the queue.

        This blocks while the queue is empty.
        """
        if timeout is not None:
            timeout = int(timeout)
            if timeout < 0:
                raise ValueError(f'timeout value must be non-negative')
            end = time.time() + timeout
        while True:
            try:
                return _queues.get(self._id)
            except QueueEmpty:
                if timeout is not None and time.time() >= end:
                    raise  # re-raise
                time.sleep(_delay)
        return obj

    def get_nowait(self):
        """Return the next object from the channel.

        If the queue is empty then raise QueueEmpty.  Otherwise this
        is the same as get().
        """
        return _queues.get(self._id)


_queues._register_queue_type(Queue)
