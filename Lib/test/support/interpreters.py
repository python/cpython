"""Subinterpreters High Level Module."""

import _xxsubinterpreters as _interpreters

# aliases:
from _xxsubinterpreters import (
    ChannelError, ChannelNotFoundError, ChannelEmptyError,
    is_shareable,
)


__all__ = [
    'Interpreter', 'get_current', 'get_main', 'create', 'list_all',
    'SendChannel', 'RecvChannel',
    'create_channel', 'list_all_channels', 'is_shareable',
    'ChannelError', 'ChannelNotFoundError',
    'ChannelEmptyError',
    ]


def create(*, isolated=True):
    """
    Initialize a new (idle) Python interpreter.
    """
    id = _interpreters.create(isolated=isolated)
    return Interpreter(id, isolated=isolated)


def list_all():
    """
    Get all existing interpreters.
    """
    return [Interpreter(id) for id in
            _interpreters.list_all()]


def get_current():
    """
    Get the currently running interpreter.
    """
    id = _interpreters.get_current()
    return Interpreter(id)


def get_main():
    """
    Get the main interpreter.
    """
    id = _interpreters.get_main()
    return Interpreter(id)


class Interpreter:
    """
    The Interpreter object represents
    a single interpreter.
    """

    def __init__(self, id, *, isolated=None):
        self._id = id
        self._isolated = isolated

    @property
    def id(self):
        return self._id

    @property
    def isolated(self):
        if self._isolated is None:
            self._isolated = _interpreters.is_isolated(self._id)
        return self._isolated

    def is_running(self):
        """
        Return whether or not the identified
        interpreter is running.
        """
        return _interpreters.is_running(self._id)

    def close(self):
        """
        Finalize and destroy the interpreter.

        Attempting to destroy the current
        interpreter results in a RuntimeError.
        """
        return _interpreters.destroy(self._id)

    def run(self, src_str, /, *, channels=None):
        """
        Run the given source code in the interpreter.
        This blocks the current Python thread until done.
        """
        _interpreters.run_string(self._id, src_str)


def create_channel():
    """
    Create a new channel for passing data between
    interpreters.
    """

    cid = _interpreters.channel_create()
    return (RecvChannel(cid), SendChannel(cid))


def list_all_channels():
    """
    Get all open channels.
    """
    return [(RecvChannel(cid), SendChannel(cid))
            for cid in _interpreters.channel_list_all()]


_NOT_SET = object()


class RecvChannel:
    """
    The RecvChannel object represents
    a recieving channel.
    """

    def __init__(self, id):
        self._id = id

    def recv(self, *, _delay=10 / 1000):  # 10 milliseconds
        """
        Get the next object from the channel,
        and wait if none have been sent.
        Associate the interpreter with the channel.
        """
        import time
        sentinel = object()
        obj = _interpreters.channel_recv(self._id, sentinel)
        while obj is sentinel:
            time.sleep(_delay)
            obj = _interpreters.channel_recv(self._id, sentinel)
        return obj

    def recv_nowait(self, default=_NOT_SET):
        """
        Like recv(), but return the default
        instead of waiting.

        This function is blocked by a missing low-level
        implementation of channel_recv_wait().
        """
        if default is _NOT_SET:
            return _interpreters.channel_recv(self._id)
        else:
            return _interpreters.channel_recv(self._id, default)


class SendChannel:
    """
    The SendChannel object represents
    a sending channel.
    """

    def __init__(self, id):
        self._id = id

    def send(self, obj):
        """
        Send the object (i.e. its data) to the receiving
        end of the channel and wait. Associate the interpreter
        with the channel.
        """
        import time
        _interpreters.channel_send(self._id, obj)
        time.sleep(2)

    def send_nowait(self, obj):
        """
        Like send(), but return False if not received.

        This function is blocked by a missing low-level
        implementation of channel_send_wait().
        """

        _interpreters.channel_send(self._id, obj)
        return False
