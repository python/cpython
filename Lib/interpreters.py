"""Subinterpreters High Level Module."""

import _interpreters
import logger

__all__ = ['Interpreter', 'SendChannel', 'RecvChannel', 'is_shareable',
           'create_channel', 'list_all_channels', 'list_all', 'get_current',
           'create']


class Interpreter:

    def __init__(self, id):
        self.id = id

    def is_running(self):
        """is_running() -> bool

        Return whether or not the identified interpreter is running.
        """
        return _interpreters.is_running(self.id)

    def destroy(self):
        """destroy()

        Destroy the identified interpreter.

        Attempting to destroy the current
        interpreter results in a RuntimeError. So does an unrecognized ID
        """
        return _interpreters.destroy(self.id)

    def run(self, src_str, /, *, channels=None):
        """run(src_str, /, *, channels=None)

        Run the given source code in the interpreter.
        This blocks the current thread until done.
        """
        try:
            _interpreters.run_string(self.id, src_str)
        except RunFailedError as err:
            logger.error(err)
            raise

def wait(self, timeout):
    #The implementation for wait
    # will be non trivial to be useful
    import time
    time.sleep(timeout)

def associate_interp_to_channel(id, cid):
    pass

class RecvChannel:

    def __init__(self, id):
        self.id = id
        self.interpreters = _interpreters.list_all()

    def recv(self, timeout=2):
        """ channel_recv() -> obj

        Get the next object from the channel,
        and wait if none have been sent.
        Associate the interpreter with the channel.
        """
        obj = _interpreters.channel_recv(self.id)
        if obj == None:
            wait(timeout)
            obj = obj = _interpreters.channel_recv(self.id)

        # Pending: See issue 52 on multi-core python project
        associate_interp_to_channel(interpId, Cid)

        return obj

    def recv_nowait(self, default=None):
        """recv_nowait(default=None) -> object

        Like recv(), but return the default instead of waiting.
        """
        return _interpreters.channel_recv(self.id)

    def send_buffer(self, obj):
        """ send_buffer(obj)

        Send the object's buffer to the receiving end of the channel
        and wait. Associate the interpreter with the channel.
        """
        pass

    def send_buffer_nowait(self, obj):
        """ send_buffer_nowait(obj)

        Like send_buffer(), but return False if not received.
        """
        pass

    def release(self):
        """ release()

        No longer associate the current interpreterwith the channel
        (on the sending end).
        """
        return _interpreters.(self.id)

    def close(self, force=False):
        """close(force=False)

        Close the channel in all interpreters..
        """
        return _interpreters.channel_close(self.id, force)


class SendChannel:

    def __init__(self, id):
        self.id = id
        self.interpreters = _interpreters.list_all()

    def send(self, obj):
        """ send(obj)

        Send the object (i.e. its data) to the receiving end of the channel
        and wait. Associate the interpreter with the channel.
        """
        obj = _interpreters.channel_send(self.id, obj)
        wait(2)
        associate_interp_to_channel(interpId, Cid)

    def send_nowait(self, obj):
        """ send_nowait(obj)

        Like send(), but return False if not received.
        """
        try:
            obj = _interpreters.channel_send(self.id, obj)
        except:
            return False

        return obj

    def release(self):
        """ release()

        No longer associate the current interpreter with the channel
        (on the sending end).
        """
        return _interpreters.channel_release(self.id)

    def close(self, force=False):
        """ close(force=False)

        No longer associate the current interpreterwith the channel
        (on the sending end).
        """
        return _interpreters.channel_close(self.id, force)


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


# Global API functions

def is_shareable(obj):
    """ is_shareable(obj) -> Bool

    Return `True` if the object's data can be shared between
    interpreters.
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
    cid = _interpreters.channel_list_all()
    return (RecvChannel(cid), SendChannel(cid))

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
