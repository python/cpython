"""Cross-interpreter Queues High Level Module."""

import pickle
import queue
import time
import weakref
import _xxinterpqueues as _queues

# aliases:
from _xxinterpqueues import (
    QueueError, QueueNotFoundError,
)

__all__ = [
    'create', 'list_all',
    'Queue',
    'QueueError', 'QueueNotFoundError', 'QueueEmpty', 'QueueFull',
]


class QueueEmpty(_queues.QueueEmpty, queue.Empty):
    """Raised from get_nowait() when the queue is empty.

    It is also raised from get() if it times out.
    """


class QueueFull(_queues.QueueFull, queue.Full):
    """Raised from put_nowait() when the queue is full.

    It is also raised from put() if it times out.
    """


_SHARED_ONLY = 0
_PICKLED = 1

def create(maxsize=0, *, syncobj=False):
    """Return a new cross-interpreter queue.

    The queue may be used to pass data safely between interpreters.

    "syncobj" sets the default for Queue.put()
    and Queue.put_nowait().
    """
    fmt = _SHARED_ONLY if syncobj else _PICKLED
    qid = _queues.create(maxsize, fmt)
    return Queue(qid, _fmt=fmt)


def list_all():
    """Return a list of all open queues."""
    return [Queue(qid, _fmt=fmt)
            for qid, fmt in _queues.list_all()]


_known_queues = weakref.WeakValueDictionary()

class Queue:
    """A cross-interpreter queue."""

    def __new__(cls, id, /, *, _fmt=None):
        # There is only one instance for any given ID.
        if isinstance(id, int):
            id = int(id)
        else:
            raise TypeError(f'id must be an int, got {id!r}')
        if _fmt is None:
            _fmt = _queues.get_default_fmt(id)
        try:
            self = _known_queues[id]
        except KeyError:
            self = super().__new__(cls)
            self._id = id
            self._fmt = _fmt
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
            syncobj=None,
            _delay=10 / 1000,  # 10 milliseconds
            ):
        """Add the object to the queue.

        This blocks while the queue is full.

        If "syncobj" is None (the default) then it uses the
        queue's default, set with create_queue()..

        If "syncobj" is false then all objects are supported,
        at the expense of worse performance.

        If "syncobj" is true then the object must be "shareable".
        Examples of "shareable" objects include the builtin singletons,
        str, and memoryview.  One benefit is that such objects are
        passed through the queue efficiently.

        The key difference, though, is conceptual: the corresponding
        object returned from Queue.get() will be strictly equivalent
        to the given obj.  In other words, the two objects will be
        effectively indistinguishable from each other, even if the
        object is mutable.  The received object may actually be the
        same object, or a copy (immutable values only), or a proxy.
        Regardless, the received object should be treated as though
        the original has been shared directly, whether or not it
        actually is.  That's a slightly different and stronger promise
        than just (initial) equality, which is all "syncobj=False"
        can promise.
        """
        if syncobj is None:
            fmt = self._fmt
        else:
            fmt = _SHARED_ONLY if syncobj else _PICKLED
        if timeout is not None:
            timeout = int(timeout)
            if timeout < 0:
                raise ValueError(f'timeout value must be non-negative')
            end = time.time() + timeout
        if fmt is _PICKLED:
            obj = pickle.dumps(obj)
        while True:
            try:
                _queues.put(self._id, obj, fmt)
            except _queues.QueueFull as exc:
                if timeout is not None and time.time() >= end:
                    exc.__class__ = QueueFull
                    raise  # re-raise
                time.sleep(_delay)
            else:
                break

    def put_nowait(self, obj, *, syncobj=None):
        if syncobj is None:
            fmt = self._fmt
        else:
            fmt = _SHARED_ONLY if syncobj else _PICKLED
        if fmt is _PICKLED:
            obj = pickle.dumps(obj)
        try:
            _queues.put(self._id, obj, fmt)
        except _queues.QueueFull as exc:
            exc.__class__ = QueueFull
            raise  # re-raise

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
                obj, fmt = _queues.get(self._id)
            except _queues.QueueEmpty as exc:
                if timeout is not None and time.time() >= end:
                    exc.__class__ = QueueEmpty
                    raise  # re-raise
                time.sleep(_delay)
            else:
                break
        if fmt == _PICKLED:
            obj = pickle.loads(obj)
        else:
            assert fmt == _SHARED_ONLY
        return obj

    def get_nowait(self):
        """Return the next object from the channel.

        If the queue is empty then raise QueueEmpty.  Otherwise this
        is the same as get().
        """
        try:
            obj, fmt = _queues.get(self._id)
        except _queues.QueueEmpty as exc:
            exc.__class__ = QueueEmpty
            raise  # re-raise
        if fmt == _PICKLED:
            obj = pickle.loads(obj)
        else:
            assert fmt == _SHARED_ONLY
        return obj


_queues._register_queue_type(Queue)
