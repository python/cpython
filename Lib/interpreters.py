"""Subinterpreters High Level Module."""

import _interpreters
import logging

__all__ = ['Interpreter', 'SendChannel', 'RecvChannel', 'is_shareable',
           'create_channel', 'list_all_channels', 'list_all', 'get_current',
           'create']


def create():
    """ create() -> Interpreter

   Initialize a new (idle) Python interpreter.
    """
    id = _interpreters.create()
    return Interpreter(id)

def list_all():
    """ list_all() -> [Interpreter]

    Get all existing interpreters.
    """
    return [Interpreter(id) for id in _interpreters.list_all()]

def get_current():
    """ get_current() -> Interpreter

    Get the currently running interpreter.
    """
    id = _interpreters.get_current()
    return Interpreter(id)


class Interpreter:

    def __init__(self, id):
        self._id = id

    @property
    def id(self):
        return self._id

    def is_running(self):
        """is_running() -> bool

        Return whether or not the identified interpreter is running.
        """
        return _interpreters.is_running(self._id)

    def destroy(self):
        """destroy()

        Destroy the identified interpreter.

        Attempting to destroy the current
        interpreter results in a RuntimeError. So does an unrecognized ID
        """
        return _interpreters.destroy(self._id)

    def run(self, src_str, /, *, channels=None):
        """run(src_str, /, *, channels=None)

        Run the given source code in the interpreter.
        This blocks the current thread until done.
        """
        try:
            _interpreters.run_string(self._id, src_str, channels)
        except RunFailedError as err:
            logger.error(err)
            raise


def is_shareable(obj):
    """ is_shareable(obj) -> Bool

    Return `True` if the object's data can be shared between
    interpreters and `False` otherwise.
    """
    return _interpreters.is_shareable(obj)

def create_channel():
    """ create_channel() -> (RecvChannel, SendChannel)

    Create a new channel for passing data between interpreters.
    """

    cid = _interpreters.channel_create()
    return (RecvChannel(cid), SendChannel(cid))

def list_all_channels():
    """ list_all_channels() -> [(RecvChannel, SendChannel)]

    Return all open channels.
    """
    return [(RecvChannel(cid), SendChannel(cid)) for cid in _interpreters.channel_list_all()]

def wait(timeout):
    #The implementation for wait
    # will be non trivial to be useful
    import time
    time.sleep(timeout)

class RecvChannel:

    def __init__(self, id):
        self.id = id
        self.interpreters = _interpreters.channel_list_interpreters(self.id, send=False)

    def recv(self):
        """ channel_recv() -> obj

        Get the next object from the channel,
        and wait if none have been sent.
        Associate the interpreter with the channel.
        """
        obj = _interpreters.channel_recv(self.id)
        if obj == None:
            wait(timeout)
            obj = _interpreters.channel_recv(self.id)
        return obj

    def recv_nowait(self, default=None):
        """recv_nowait(default=None) -> object

        Like recv(), but return the default instead of waiting.
        """
        obj = _interpreters.channel_recv(self.id)
        if obj == None:
            obj = default
        return obj

    def release(self):
        """ release()

        No longer associate the current interpreterwith the channel
        (on the receiving end).
        """
        return _interpreters.channel_release(self.id, recv=True)

    def close(self, force=False):
        """close(force=False)

        Close the channel in all interpreters..
        """
        return _interpreters.channel_close(self.id, recv=force)


class SendChannel:

    def __init__(self, id):
        self.id = id
        self.interpreters = _interpreters.channel_list_interpreters(self.id, send=True)

    def send(self, obj):
        """ send(obj)

        Send the object (i.e. its data) to the receiving end of the channel
        and wait. Associate the interpreter with the channel.
        """
        _interpreters.channel_send(self.id, obj)
        wait(2)

    def send_nowait(self, obj):
        """ send_nowait(obj)

        Like send(), but return False if not received.
        """
        _interpreters.channel_send(self.id, obj)
        recv_obj = _interpreters.channel_recv(self.id)
        if recv_obj:
            return obj   
        else:
            return False

    def send_buffer(self, obj):
        """ ssend_buffer(obj)

        Send the object's buffer to the receiving
        end of the channel and wait. Associate the interpreter
        with the channel.
        """
        _interpreters.channel_send_buffer(self.id, obj)
        wait(2)

    def send_buffer_nowait(self, obj):
        """ send_buffer_nowait(obj)

        Like send(), but return False if not received.
        """
        _interpreters.channel_send_buffer(self.id, obj)
        recv_obj = _interpreters.channel_recv(self.id)
        if recv_obj:
            return obj   
        else:
            return False

    def release(self):
        """ release()

        No longer associate the current interpreterwith the channel
        (on the sending end).
        """
        return _interpreters.channel_release(self.id, send=True)

    def close(self, force=False):
        """close(force=False)

        Close the channel in all interpreters..
        """
        return _interpreters.channel_close(self.id, send=force)


class ChannelError(Exception):
    pass


class ChannelNotFoundError(ChannelError):
    pass


class ChannelEmptyError(ChannelError):
    pass


class ChannelNotEmptyError(ChannelError):
    pass


class NotReceivedError(ChannelError):
    pass


class ChannelClosedError(ChannelError):
    pass


class ChannelReleasedError(ChannelClosedError):
    pass


class RunFailedError(RuntimeError):
    pass
