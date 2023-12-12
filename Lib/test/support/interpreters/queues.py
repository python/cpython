"""Cross-interpreter Queues High Level Module."""

import queue
import time
import weakref
import _xxinterpchannels as _channels
import _xxinterpchannels as _queues

# aliases:
from _xxinterpchannels import (
    ChannelError as QueueError,
    ChannelNotFoundError as QueueNotFoundError,
)

__all__ = [
    'create', 'list_all',
    'Queue',
    'QueueError', 'QueueNotFoundError', 'QueueEmpty', 'QueueFull',
]


def create(maxsize=0):
    """Return a new cross-interpreter queue.

    The queue may be used to pass data safely between interpreters.
    """
    # XXX honor maxsize
    qid = _queues.create()
    return Queue._with_maxsize(qid, maxsize)


def list_all():
    """Return a list of all open queues."""
    return [Queue(qid)
            for qid in _queues.list_all()]


class QueueEmpty(queue.Empty):
    """Raised from get_nowait() when the queue is empty.

    It is also raised from get() if it times out.
    """


class QueueFull(queue.Full):
    """Raised from put_nowait() when the queue is full.

    It is also raised from put() if it times out.
    """


_known_queues = weakref.WeakValueDictionary()

class Queue:
    """A cross-interpreter queue."""

    @classmethod
    def _with_maxsize(cls, id, maxsize):
        if not isinstance(maxsize, int):
            raise TypeError(f'maxsize must be an int, got {maxsize!r}')
        elif maxsize < 0:
            maxsize = 0
        else:
            maxsize = int(maxsize)
        self = cls(id)
        self._maxsize = maxsize
        return self

    def __new__(cls, id, /):
        # There is only one instance for any given ID.
        if isinstance(id, int):
            id = _channels._channel_id(id, force=False)
        elif not isinstance(id, _channels.ChannelID):
            raise TypeError(f'id must be an int, got {id!r}')
        key = int(id)
        try:
            self = _known_queues[key]
        except KeyError:
            self = super().__new__(cls)
            self._id = id
            self._maxsize = 0
            _known_queues[key] = self
        return self

    def __repr__(self):
        return f'{type(self).__name__}({self.id})'

    def __hash__(self):
        return hash(self._id)

    @property
    def id(self):
        return int(self._id)

    @property
    def maxsize(self):
        return self._maxsize

    @property
    def _info(self):
        return _channels.get_info(self._id)

    def empty(self):
        return self._info.count == 0

    def full(self):
        if self._maxsize <= 0:
            return False
        return self._info.count >= self._maxsize

    def qsize(self):
        return self._info.count

    def put(self, obj, timeout=None):
        # XXX block if full
        _channels.send(self._id, obj, blocking=False)

    def put_nowait(self, obj):
        # XXX raise QueueFull if full
        return _channels.send(self._id, obj, blocking=False)

    def get(self, timeout=None, *,
             _sentinel=object(),
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
        obj = _channels.recv(self._id, _sentinel)
        while obj is _sentinel:
            time.sleep(_delay)
            if timeout is not None and time.time() >= end:
                raise QueueEmpty
            obj = _channels.recv(self._id, _sentinel)
        return obj

    def get_nowait(self, *, _sentinel=object()):
        """Return the next object from the channel.

        If the queue is empty then raise QueueEmpty.  Otherwise this
        is the same as get().
        """
        obj = _channels.recv(self._id, _sentinel)
        if obj is _sentinel:
            raise QueueEmpty
        return obj


# XXX add this:
#_channels._register_queue_type(Queue)
