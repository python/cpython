"""Cross-interpreter Channels High Level Module."""

import time
import _interpchannels as _channels
from concurrent.interpreters import _crossinterp

# aliases:
from _interpchannels import (
    ChannelError, ChannelNotFoundError, ChannelClosedError,  # noqa: F401
    ChannelEmptyError, ChannelNotEmptyError,  # noqa: F401
)
from concurrent.interpreters._crossinterp import (
    UNBOUND_ERROR, UNBOUND_REMOVE,
)


__all__ = [
    'UNBOUND', 'UNBOUND_ERROR', 'UNBOUND_REMOVE',
    'create', 'list_all',
    'SendChannel', 'RecvChannel',
    'ChannelError', 'ChannelNotFoundError', 'ChannelEmptyError',
    'ItemInterpreterDestroyed',
]


class ItemInterpreterDestroyed(ChannelError,
                               _crossinterp.ItemInterpreterDestroyed):
    """Raised from get() and get_nowait()."""


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


def create(*, unbounditems=UNBOUND):
    """Return (recv, send) for a new cross-interpreter channel.

    The channel may be used to pass data safely between interpreters.

    "unbounditems" sets the default for the send end of the channel.
    See SendChannel.send() for supported values.  The default value
    is UNBOUND, which replaces the unbound item when received.
    """
    unbound = _serialize_unbound(unbounditems)
    unboundop, = unbound
    cid = _channels.create(unboundop, -1)
    recv, send = RecvChannel(cid), SendChannel(cid)
    send._set_unbound(unboundop, unbounditems)
    return recv, send


def list_all():
    """Return a list of (recv, send) for all open channels."""
    channels = []
    for cid, unboundop, _ in _channels.list_all():
        chan = _, send = RecvChannel(cid), SendChannel(cid)
        if not hasattr(send, '_unboundop'):
            send._set_unbound(unboundop)
        else:
            assert send._unbound[0] == unboundop
        channels.append(chan)
    return channels


class _ChannelEnd:
    """The base class for RecvChannel and SendChannel."""

    _end = None

    def __new__(cls, cid):
        self = super().__new__(cls)
        if self._end == 'send':
            cid = _channels._channel_id(cid, send=True, force=True)
        elif self._end == 'recv':
            cid = _channels._channel_id(cid, recv=True, force=True)
        else:
            raise NotImplementedError(self._end)
        self._id = cid
        return self

    def __repr__(self):
        return f'{type(self).__name__}(id={int(self._id)})'

    def __hash__(self):
        return hash(self._id)

    def __eq__(self, other):
        if isinstance(self, RecvChannel):
            if not isinstance(other, RecvChannel):
                return NotImplemented
        elif not isinstance(other, SendChannel):
            return NotImplemented
        return other._id == self._id

    # for pickling:
    def __getnewargs__(self):
        return (int(self._id),)

    # for pickling:
    def __getstate__(self):
        return None

    @property
    def id(self):
        return self._id

    @property
    def _info(self):
        return _channels.get_info(self._id)

    @property
    def is_closed(self):
        return self._info.closed


_NOT_SET = object()


class RecvChannel(_ChannelEnd):
    """The receiving end of a cross-interpreter channel."""

    _end = 'recv'

    def recv(self, timeout=None, *,
             _sentinel=object(),
             _delay=10 / 1000,  # 10 milliseconds
             ):
        """Return the next object from the channel.

        This blocks until an object has been sent, if none have been
        sent already.
        """
        if timeout is not None:
            timeout = int(timeout)
            if timeout < 0:
                raise ValueError(f'timeout value must be non-negative')
            end = time.time() + timeout
        obj, unboundop = _channels.recv(self._id, _sentinel)
        while obj is _sentinel:
            time.sleep(_delay)
            if timeout is not None and time.time() >= end:
                raise TimeoutError
            obj, unboundop = _channels.recv(self._id, _sentinel)
        if unboundop is not None:
            assert obj is None, repr(obj)
            return _resolve_unbound(unboundop)
        return obj

    def recv_nowait(self, default=_NOT_SET):
        """Return the next object from the channel.

        If none have been sent then return the default if one
        is provided or fail with ChannelEmptyError.  Otherwise this
        is the same as recv().
        """
        if default is _NOT_SET:
            obj, unboundop = _channels.recv(self._id)
        else:
            obj, unboundop = _channels.recv(self._id, default)
        if unboundop is not None:
            assert obj is None, repr(obj)
            return _resolve_unbound(unboundop)
        return obj

    def close(self):
        _channels.close(self._id, recv=True)


class SendChannel(_ChannelEnd):
    """The sending end of a cross-interpreter channel."""

    _end = 'send'

#    def __new__(cls, cid, *, _unbound=None):
#        if _unbound is None:
#            try:
#                op = _channels.get_channel_defaults(cid)
#                _unbound = (op,)
#            except ChannelNotFoundError:
#                _unbound = _serialize_unbound(UNBOUND)
#        self = super().__new__(cls, cid)
#        self._unbound = _unbound
#        return self

    def _set_unbound(self, op, items=None):
        assert not hasattr(self, '_unbound')
        if items is None:
            items = _resolve_unbound(op)
        unbound = (op, items)
        self._unbound = unbound
        return unbound

    @property
    def unbounditems(self):
        try:
            _, items = self._unbound
        except AttributeError:
            op, _ = _channels.get_queue_defaults(self._id)
            _, items = self._set_unbound(op)
        return items

    @property
    def is_closed(self):
        info = self._info
        return info.closed or info.closing

    def send(self, obj, timeout=None, *,
             unbounditems=None,
             ):
        """Send the object (i.e. its data) to the channel's receiving end.

        This blocks until the object is received.
        """
        if unbounditems is None:
            unboundop = -1
        else:
            unboundop, = _serialize_unbound(unbounditems)
        _channels.send(self._id, obj, unboundop, timeout=timeout, blocking=True)

    def send_nowait(self, obj, *,
                    unbounditems=None,
                    ):
        """Send the object to the channel's receiving end.

        If the object is immediately received then return True
        (else False).  Otherwise this is the same as send().
        """
        if unbounditems is None:
            unboundop = -1
        else:
            unboundop, = _serialize_unbound(unbounditems)
        # XXX Note that at the moment channel_send() only ever returns
        # None.  This should be fixed when channel_send_wait() is added.
        # See bpo-32604 and gh-19829.
        return _channels.send(self._id, obj, unboundop, blocking=False)

    def send_buffer(self, obj, timeout=None, *,
                    unbounditems=None,
                    ):
        """Send the object's buffer to the channel's receiving end.

        This blocks until the object is received.
        """
        if unbounditems is None:
            unboundop = -1
        else:
            unboundop, = _serialize_unbound(unbounditems)
        _channels.send_buffer(self._id, obj, unboundop,
                              timeout=timeout, blocking=True)

    def send_buffer_nowait(self, obj, *,
                           unbounditems=None,
                           ):
        """Send the object's buffer to the channel's receiving end.

        If the object is immediately received then return True
        (else False).  Otherwise this is the same as send().
        """
        if unbounditems is None:
            unboundop = -1
        else:
            unboundop, = _serialize_unbound(unbounditems)
        return _channels.send_buffer(self._id, obj, unboundop, blocking=False)

    def close(self):
        _channels.close(self._id, send=True)


# XXX This is causing leaks (gh-110318):
_channels._register_end_types(SendChannel, RecvChannel)
