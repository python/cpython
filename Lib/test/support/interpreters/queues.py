"""Cross-interpreter Queues High Level Module."""

import pickle
import queue
import time
import weakref
import _interpqueues as _queues
from . import _crossinterp

# aliases:
from _interpqueues import (
    QueueError, QueueNotFoundError,
)
from ._crossinterp import (
    UNBOUND_ERROR, UNBOUND_REMOVE,
)

__all__ = [
    'UNBOUND', 'UNBOUND_ERROR', 'UNBOUND_REMOVE',
    'create', 'list_all',
    'Queue',
    'QueueError', 'QueueNotFoundError', 'QueueEmpty', 'QueueFull',
    'ItemInterpreterDestroyed',
]


class QueueEmpty(QueueError, queue.Empty):
    """Raised from get_nowait() when the queue is empty.

    It is also raised from get() if it times out.
    """


class QueueFull(QueueError, queue.Full):
    """Raised from put_nowait() when the queue is full.

    It is also raised from put() if it times out.
    """


class ItemInterpreterDestroyed(QueueError,
                               _crossinterp.ItemInterpreterDestroyed):
    """Raised from get() and get_nowait()."""


_SHARED_ONLY = 0
_PICKLED = 1


UNBOUND = _crossinterp.UnboundItem.singleton('queue', __name__)


def _serialize_unbound(unbound):
    if unbound is UNBOUND:
        unbound = _crossinterp.UNBOUND
    return _crossinterp.serialize_unbound(unbound)


def _resolve_unbound(flag):
    resolved = _crossinterp.resolve_unbound(flag, ItemInterpreterDestroyed)
    if resolved is _crossinterp.UNBOUND:
        resolved = UNBOUND
    return resolved


def create(maxsize=0, *, syncobj=False, unbounditems=UNBOUND):
    """Return a new cross-interpreter queue.

    The queue may be used to pass data safely between interpreters.

    "syncobj" sets the default for Queue.put()
    and Queue.put_nowait().

    "unbounditems" likewise sets the default.  See Queue.put() for
    supported values.  The default value is UNBOUND, which replaces
    the unbound item.
    """
    fmt = _SHARED_ONLY if syncobj else _PICKLED
    unbound = _serialize_unbound(unbounditems)
    unboundop, = unbound
    qid = _queues.create(maxsize, fmt, unboundop)
    return Queue(qid, _fmt=fmt, _unbound=unbound)


def list_all():
    """Return a list of all open queues."""
    return [Queue(qid, _fmt=fmt, _unbound=(unboundop,))
            for qid, fmt, unboundop in _queues.list_all()]


_known_queues = weakref.WeakValueDictionary()

class Queue:
    """A cross-interpreter queue."""

    def __new__(cls, id, /, *, _fmt=None, _unbound=None):
        # There is only one instance for any given ID.
        if isinstance(id, int):
            id = int(id)
        else:
            raise TypeError(f'id must be an int, got {id!r}')
        if _fmt is None:
            if _unbound is None:
                _fmt, op = _queues.get_queue_defaults(id)
                _unbound = (op,)
            else:
                _fmt, _ = _queues.get_queue_defaults(id)
        elif _unbound is None:
            _, op = _queues.get_queue_defaults(id)
            _unbound = (op,)
        try:
            self = _known_queues[id]
        except KeyError:
            self = super().__new__(cls)
            self._id = id
            self._fmt = _fmt
            self._unbound = _unbound
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

    # for pickling:
    def __getnewargs__(self):
        return (self._id,)

    # for pickling:
    def __getstate__(self):
        return None

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
            unbound=None,
            _delay=10 / 1000,  # 10 milliseconds
            ):
        """Add the object to the queue.

        This blocks while the queue is full.

        If "syncobj" is None (the default) then it uses the
        queue's default, set with create_queue().

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

        "unbound" controls the behavior of Queue.get() for the given
        object if the current interpreter (calling put()) is later
        destroyed.

        If "unbound" is None (the default) then it uses the
        queue's default, set with create_queue(),
        which is usually UNBOUND.

        If "unbound" is UNBOUND_ERROR then get() will raise an
        ItemInterpreterDestroyed exception if the original interpreter
        has been destroyed.  This does not otherwise affect the queue;
        the next call to put() will work like normal, returning the next
        item in the queue.

        If "unbound" is UNBOUND_REMOVE then the item will be removed
        from the queue as soon as the original interpreter is destroyed.
        Be aware that this will introduce an imbalance between put()
        and get() calls.

        If "unbound" is UNBOUND then it is returned by get() in place
        of the unbound item.
        """
        if syncobj is None:
            fmt = self._fmt
        else:
            fmt = _SHARED_ONLY if syncobj else _PICKLED
        if unbound is None:
            unboundop, = self._unbound
        else:
            unboundop, = _serialize_unbound(unbound)
        if timeout is not None:
            timeout = int(timeout)
            if timeout < 0:
                raise ValueError(f'timeout value must be non-negative')
            end = time.time() + timeout
        if fmt is _PICKLED:
            obj = pickle.dumps(obj)
        while True:
            try:
                _queues.put(self._id, obj, fmt, unboundop)
            except QueueFull as exc:
                if timeout is not None and time.time() >= end:
                    raise  # re-raise
                time.sleep(_delay)
            else:
                break

    def put_nowait(self, obj, *, syncobj=None, unbound=None):
        if syncobj is None:
            fmt = self._fmt
        else:
            fmt = _SHARED_ONLY if syncobj else _PICKLED
        if unbound is None:
            unboundop, = self._unbound
        else:
            unboundop, = _serialize_unbound(unbound)
        if fmt is _PICKLED:
            obj = pickle.dumps(obj)
        _queues.put(self._id, obj, fmt, unboundop)

    def get(self, timeout=None, *,
            _delay=10 / 1000,  # 10 milliseconds
            ):
        """Return the next object from the queue.

        This blocks while the queue is empty.

        If the next item's original interpreter has been destroyed
        then the "next object" is determined by the value of the
        "unbound" argument to put().
        """
        if timeout is not None:
            timeout = int(timeout)
            if timeout < 0:
                raise ValueError(f'timeout value must be non-negative')
            end = time.time() + timeout
        while True:
            try:
                obj, fmt, unboundop = _queues.get(self._id)
            except QueueEmpty as exc:
                if timeout is not None and time.time() >= end:
                    raise  # re-raise
                time.sleep(_delay)
            else:
                break
        if unboundop is not None:
            assert obj is None, repr(obj)
            return _resolve_unbound(unboundop)
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
            obj, fmt, unboundop = _queues.get(self._id)
        except QueueEmpty as exc:
            raise  # re-raise
        if unboundop is not None:
            assert obj is None, repr(obj)
            return _resolve_unbound(unboundop)
        if fmt == _PICKLED:
            obj = pickle.loads(obj)
        else:
            assert fmt == _SHARED_ONLY
        return obj


_queues._register_heap_types(Queue, QueueEmpty, QueueFull)
