"""Cross-interpreter Channels High Level Module."""

import time
import _xxinterpchannels as _channels

# aliases:
from _xxinterpchannels import (
    ChannelError, ChannelNotFoundError, ChannelClosedError,
    ChannelEmptyError, ChannelNotEmptyError,
)


__all__ = [
    'create', 'list_all',
    'SendChannel', 'RecvChannel',
    'ChannelError', 'ChannelNotFoundError', 'ChannelEmptyError',
]


def create():
    """Return (recv, send) for a new cross-interpreter channel.

    The channel may be used to pass data safely between interpreters.
    """
    cid = _channels.create()
    recv, send = RecvChannel(cid), SendChannel(cid)
    return recv, send


def list_all():
    """Return a list of (recv, send) for all open channels."""
    return [(RecvChannel(cid), SendChannel(cid))
            for cid in _channels.list_all()]


class _ChannelEnd:
    """The base class for RecvChannel and SendChannel."""

    _end = None

    def __init__(self, cid):
        if self._end == 'send':
            cid = _channels._channel_id(cid, send=True, force=True)
        elif self._end == 'recv':
            cid = _channels._channel_id(cid, recv=True, force=True)
        else:
            raise NotImplementedError(self._end)
        self._id = cid

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
        obj = _channels.recv(self._id, _sentinel)
        while obj is _sentinel:
            time.sleep(_delay)
            if timeout is not None and time.time() >= end:
                raise TimeoutError
            obj = _channels.recv(self._id, _sentinel)
        return obj

    def recv_nowait(self, default=_NOT_SET):
        """Return the next object from the channel.

        If none have been sent then return the default if one
        is provided or fail with ChannelEmptyError.  Otherwise this
        is the same as recv().
        """
        if default is _NOT_SET:
            return _channels.recv(self._id)
        else:
            return _channels.recv(self._id, default)

    def close(self):
        _channels.close(self._id, recv=True)


class SendChannel(_ChannelEnd):
    """The sending end of a cross-interpreter channel."""

    _end = 'send'

    @property
    def is_closed(self):
        info = self._info
        return info.closed or info.closing

    def send(self, obj, timeout=None):
        """Send the object (i.e. its data) to the channel's receiving end.

        This blocks until the object is received.
        """
        _channels.send(self._id, obj, timeout=timeout, blocking=True)

    def send_nowait(self, obj):
        """Send the object to the channel's receiving end.

        If the object is immediately received then return True
        (else False).  Otherwise this is the same as send().
        """
        # XXX Note that at the moment channel_send() only ever returns
        # None.  This should be fixed when channel_send_wait() is added.
        # See bpo-32604 and gh-19829.
        return _channels.send(self._id, obj, blocking=False)

    def send_buffer(self, obj, timeout=None):
        """Send the object's buffer to the channel's receiving end.

        This blocks until the object is received.
        """
        _channels.send_buffer(self._id, obj, timeout=timeout, blocking=True)

    def send_buffer_nowait(self, obj):
        """Send the object's buffer to the channel's receiving end.

        If the object is immediately received then return True
        (else False).  Otherwise this is the same as send().
        """
        return _channels.send_buffer(self._id, obj, blocking=False)

    def close(self):
        _channels.close(self._id, send=True)


# XXX This is causing leaks (gh-110318):
_channels._register_end_types(SendChannel, RecvChannel)
