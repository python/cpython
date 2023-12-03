"""Subinterpreters High Level Module."""

import time
import _xxsubinterpreters as _interpreters
import _xxinterpchannels as _channels

# aliases:
from _xxsubinterpreters import is_shareable
from _xxinterpchannels import (
    ChannelError, ChannelNotFoundError, ChannelClosedError,
    ChannelEmptyError, ChannelNotEmptyError,
)


__all__ = [
    'Interpreter', 'get_current', 'get_main', 'create', 'list_all',
    'RunFailedError',
    'SendChannel', 'RecvChannel',
    'create_channel', 'list_all_channels', 'is_shareable',
    'ChannelError', 'ChannelNotFoundError',
    'ChannelEmptyError',
    ]


class RunFailedError(RuntimeError):

    def __init__(self, excinfo):
        msg = excinfo.formatted
        if not msg:
            if excinfo.type and snapshot.msg:
                msg = f'{snapshot.type.__name__}: {snapshot.msg}'
            else:
                msg = snapshot.type.__name__ or snapshot.msg
        super().__init__(msg)
        self.snapshot = excinfo


def create(*, isolated=True):
    """Return a new (idle) Python interpreter."""
    id = _interpreters.create(isolated=isolated)
    return Interpreter(id, isolated=isolated)


def list_all():
    """Return all existing interpreters."""
    return [Interpreter(id) for id in _interpreters.list_all()]


def get_current():
    """Return the currently running interpreter."""
    id = _interpreters.get_current()
    return Interpreter(id)


def get_main():
    """Return the main interpreter."""
    id = _interpreters.get_main()
    return Interpreter(id)


class Interpreter:
    """A single Python interpreter."""

    def __init__(self, id, *, isolated=None):
        if not isinstance(id, (int, _interpreters.InterpreterID)):
            raise TypeError(f'id must be an int, got {id!r}')
        self._id = id
        self._isolated = isolated

    def __repr__(self):
        data = dict(id=int(self._id), isolated=self._isolated)
        kwargs = (f'{k}={v!r}' for k, v in data.items())
        return f'{type(self).__name__}({", ".join(kwargs)})'

    def __hash__(self):
        return hash(self._id)

    def __eq__(self, other):
        if not isinstance(other, Interpreter):
            return NotImplemented
        else:
            return other._id == self._id

    @property
    def id(self):
        return self._id

    @property
    def isolated(self):
        if self._isolated is None:
            # XXX The low-level function has not been added yet.
            # See bpo-....
            self._isolated = _interpreters.is_isolated(self._id)
        return self._isolated

    def is_running(self):
        """Return whether or not the identified interpreter is running."""
        return _interpreters.is_running(self._id)

    def close(self):
        """Finalize and destroy the interpreter.

        Attempting to destroy the current interpreter results
        in a RuntimeError.
        """
        return _interpreters.destroy(self._id)

    # XXX Rename "run" to "exec"?
    def run(self, src_str, /, channels=None):
        """Run the given source code in the interpreter.

        This is essentially the same as calling the builtin "exec"
        with this interpreter, using the __dict__ of its __main__
        module as both globals and locals.

        There is no return value.

        If the code raises an unhandled exception then a RunFailedError
        is raised, which summarizes the unhandled exception.  The actual
        exception is discarded because objects cannot be shared between
        interpreters.

        This blocks the current Python thread until done.  During
        that time, the previous interpreter is allowed to run
        in other threads.
        """
        excinfo = _interpreters.exec(self._id, src_str, channels)
        if excinfo is not None:
            raise RunFailedError(excinfo)


def create_channel():
    """Return (recv, send) for a new cross-interpreter channel.

    The channel may be used to pass data safely between interpreters.
    """
    cid = _channels.create()
    recv, send = RecvChannel(cid), SendChannel(cid)
    return recv, send


def list_all_channels():
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
