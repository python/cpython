"""Cross-interpreter Queues High Level Module."""

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


def create(maxsize=0, *, unbounditems=UNBOUND):
    """Return a new cross-interpreter queue.

    The queue may be used to pass data safely between interpreters.

    "unbounditems" sets the default for Queue.put(); see that method for
    supported values.  The default value is UNBOUND, which replaces
    the unbound item.
    """
    unbound = _serialize_unbound(unbounditems)
    unboundop, = unbound
    qid = _queues.create(maxsize, unboundop, -1)
    self = Queue(qid)
    self._set_unbound(unboundop, unbounditems)
    return self


def list_all():
    """Return a list of all open queues."""
    queues = []
    for qid, unboundop, _ in _queues.list_all():
        self = Queue(qid)
        if not hasattr(self, '_unbound'):
            self._set_unbound(unboundop)
        else:
            assert self._unbound[0] == unboundop
        queues.append(self)
    return queues


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

    # for pickling:
    def __reduce__(self):
        return (type(self), (self._id,))

    def _set_unbound(self, op, items=None):
        assert not hasattr(self, '_unbound')
        if items is None:
            items = _resolve_unbound(op)
        unbound = (op, items)
        self._unbound = unbound
        return unbound

    @property
    def id(self):
        return self._id

    @property
    def unbounditems(self):
        try:
            _, items = self._unbound
        except AttributeError:
            op, _ = _queues.get_queue_defaults(self._id)
            _, items = self._set_unbound(op)
        return items

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

    def put(self, obj, block=True, timeout=None, *,
            unbounditems=None,
            _delay=10 / 1000,  # 10 milliseconds
            ):
        """Add the object to the queue.

        If "block" is true, this blocks while the queue is full.

        For most objects, the object received through Queue.get() will
        be a new one, equivalent to the original and not sharing any
        actual underlying data.  The notable exceptions include
        cross-interpreter types (like Queue) and memoryview, where the
        underlying data is actually shared.  Furthermore, some types
        can be sent through a queue more efficiently than others.  This
        group includes various immutable types like int, str, bytes, and
        tuple (if the items are likewise efficiently shareable).  See interpreters.is_shareable().

        "unbounditems" controls the behavior of Queue.get() for the given
        object if the current interpreter (calling put()) is later
        destroyed.

        If "unbounditems" is None (the default) then it uses the
        queue's default, set with create_queue(),
        which is usually UNBOUND.

        If "unbounditems" is UNBOUND_ERROR then get() will raise an
        ItemInterpreterDestroyed exception if the original interpreter
        has been destroyed.  This does not otherwise affect the queue;
        the next call to put() will work like normal, returning the next
        item in the queue.

        If "unbounditems" is UNBOUND_REMOVE then the item will be removed
        from the queue as soon as the original interpreter is destroyed.
        Be aware that this will introduce an imbalance between put()
        and get() calls.

        If "unbounditems" is UNBOUND then it is returned by get() in place
        of the unbound item.
        """
        if not block:
            return self.put_nowait(obj, unbounditems=unbounditems)
        if unbounditems is None:
            unboundop = -1
        else:
            unboundop, = _serialize_unbound(unbounditems)
        if timeout is not None:
            timeout = int(timeout)
            if timeout < 0:
                raise ValueError(f'timeout value must be non-negative')
            end = time.time() + timeout
        while True:
            try:
                _queues.put(self._id, obj, unboundop)
            except QueueFull as exc:
                if timeout is not None and time.time() >= end:
                    raise  # re-raise
                time.sleep(_delay)
            else:
                break

    def put_nowait(self, obj, *, unbounditems=None):
        if unbounditems is None:
            unboundop = -1
        else:
            unboundop, = _serialize_unbound(unbounditems)
        _queues.put(self._id, obj, unboundop)

    def get(self, block=True, timeout=None, *,
            _delay=10 / 1000,  # 10 milliseconds
            ):
        """Return the next object from the queue.

        If "block" is true, this blocks while the queue is empty.

        If the next item's original interpreter has been destroyed
        then the "next object" is determined by the value of the
        "unbounditems" argument to put().
        """
        if not block:
            return self.get_nowait()
        if timeout is not None:
            timeout = int(timeout)
            if timeout < 0:
                raise ValueError(f'timeout value must be non-negative')
            end = time.time() + timeout
        while True:
            try:
                obj, unboundop = _queues.get(self._id)
            except QueueEmpty as exc:
                if timeout is not None and time.time() >= end:
                    raise  # re-raise
                time.sleep(_delay)
            else:
                break
        if unboundop is not None:
            assert obj is None, repr(obj)
            return _resolve_unbound(unboundop)
        return obj

    def get_nowait(self):
        """Return the next object from the channel.

        If the queue is empty then raise QueueEmpty.  Otherwise this
        is the same as get().
        """
        try:
            obj, unboundop = _queues.get(self._id)
        except QueueEmpty as exc:
            raise  # re-raise
        if unboundop is not None:
            assert obj is None, repr(obj)
            return _resolve_unbound(unboundop)
        return obj


_queues._register_heap_types(Queue, QueueEmpty, QueueFull)
