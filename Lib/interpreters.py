"""Subinterpreters High Level Module."""

import _interpreters
import logging

__all__ = ['Interpreter', 'SendChannel', 'RecvChannel',
            'is_shareable', 'create_channel',
            'list_all_channels', 'get_current',
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
    return [Interpreter(id) for id in
            _interpreters.list_all()]

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

        Return whether or not the identified
        interpreter is running.
        """
        return _interpreters.is_running(self._id)

    def destroy(self):
        """destroy()

        Destroy the interpreter.

        Attempting to destroy the current
        interpreter results in a RuntimeError.
        """
        return _interpreters.destroy(self._id)

    def _handle_channels(self, channels):
        # Looks like the only way for an interpreter to be associated
        # to a channel is through sending or recving data.
        if channels:
            if channels[0] and channels[1] != None:
                _interpreters.channel_recv(channels[0].id)
                _interpreters.channel_send(channels[1].id, src_str)
            elif channels[0] != None and channels[1] == None:
                _interpreters.channel_recv(channels[0].id)
            elif channels[0] == None and channels[1] != None:
                _interpreters.channel_send(channels[1].id, src_str)
            else:
                pass

    def run(self, src_str, /, *, channels=None):
        """run(src_str, /, *, channels=None)

        Run the given source code in the interpreter.
        This blocks the current thread until done.
        """
        self._handle_channels(channels)
        try:
            _interpreters.run_string(self._id, src_str)
        except RunFailedError as err:
            logger.error(err)
            raise


def is_shareable(obj):
    """ is_shareable(obj) -> Bool

    Return `True` if the object's data can be
    shared between interpreters and `False` otherwise.
    """
    return _interpreters.is_shareable(obj)

def create_channel():
    """ create_channel() -> (RecvChannel, SendChannel)

    Create a new channel for passing data between
    interpreters.
    """

    cid = _interpreters.channel_create()
    return (RecvChannel(cid), SendChannel(cid))

def list_all_channels():
    """ list_all_channels() -> [(RecvChannel, SendChannel)]

    Return all open channels.
    """
    return [(RecvChannel(cid), SendChannel(cid))
            for cid in _interpreters.channel_list_all()]

def wait(timeout):
    #The implementation for wait
    # will be non trivial to be useful
    import time
    time.sleep(timeout)

class RecvChannel:

    def __init__(self, id):
        self.id = id
        self.interpreters = _interpreters.\
                            channel_list_interpreters(self.id,\
                                                      send=False)

    def recv(self):
        """ channel_recv() -> obj

        Get the next object from the channel,
        and wait if none have been sent.
        Associate the interpreter with the channel.
        """
        try:
            obj = _interpreters.channel_recv(self.id)
            if obj == None:
                wait(2)
                obj = _interpreters.channel_recv(self.id)
        except _interpreters.ChannelEmptyError:
            raise ChannelEmptyError
        except _interpreters.ChannelNotFoundError:
            raise ChannelNotFoundError
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.RunFailedError:
            raise RunFailedError
        return obj

    def recv_nowait(self, default=None):
        """recv_nowait(default=None) -> object

        Like recv(), but return the default
        instead of waiting.
        """
        try:
            obj = _interpreters.channel_recv(self.id)
            if obj == None:
                obj = default
        except _interpreters.ChannelEmptyError:
            raise ChannelEmptyError
        except _interpreters.ChannelNotFoundError:
            raise ChannelNotFoundError
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.RunFailedError:
            raise RunFailedError
        return obj

    def release(self):
        """ release()

        No longer associate the current interpreter
        with the channel (on the receiving end).
        """
        try:
            _interpreters.channel_release(self.id, recv=True)
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.ChannelNotEmptyError:
            raise ChannelNotEmptyError

    def close(self, force=False):
        """close(force=False)

        Close the channel in all interpreters.
        """
        try:
            _interpreters.channel_close(self.id,
                                        force=force, recv=True)
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.ChannelNotEmptyError:
            raise ChannelNotEmptyError


class SendChannel:

    def __init__(self, id):
        self.id = id
        self.interpreters = _interpreters.\
                            channel_list_interpreters(self.id,\
                                                      send=True)

    def send(self, obj):
        """ send(obj)

        Send the object (i.e. its data) to the receiving
        end of the channel and wait. Associate the interpreter
        with the channel.
        """
        try:
            _interpreters.channel_send(self.id, obj)
            wait(2)
        except _interpreters.ChannelNotFoundError:
            raise ChannelNotFoundError
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.RunFailedError:
            raise RunFailedError

    def send_nowait(self, obj):
        """ send_nowait(obj)

        Like send(), but return False if not received.
        """
        try:
            _interpreters.channel_send(self.id, obj)
        except _interpreters.ChannelNotFoundError:
            raise ChannelNotFoundError
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.RunFailedError:
            raise RunFailedError

        recv_obj = _interpreters.channel_recv(self.id)
        if recv_obj:
            return obj
        else:
            return False

    def send_buffer(self, obj):
        """ send_buffer(obj)

        Send the object's buffer to the receiving
        end of the channel and wait. Associate the interpreter
        with the channel.
        """
        try:
            _interpreters.channel_send_buffer(self.id, obj)
            wait(2)
        except _interpreters.ChannelNotFoundError:
            raise ChannelNotFoundError
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.RunFailedError:
            raise RunFailedError

    def send_buffer_nowait(self, obj):
        """ send_buffer_nowait(obj)

        Like send(), but return False if not received.
        """
        try:
            _interpreters.channel_send_buffer(self.id, obj)
        except _interpreters.ChannelNotFoundError:
            raise ChannelNotFoundError
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.RunFailedError:
            raise RunFailedError
        recv_obj = _interpreters.channel_recv(self.id)
        if recv_obj:
            return obj
        else:
            return False

    def release(self):
        """ release()

        No longer associate the current interpreter
        with the channel (on the sending end).
        """
        try:
            _interpreters.channel_release(self.id, send=True)
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.ChannelNotEmptyError:
            raise ChannelNotEmptyError

    def close(self, force=False):
        """close(force=False)

        Close the channel in all interpreters..
        """
        try:
            _interpreters.channel_close(self.id,
                                        force=force, send=False)
        except _interpreters.ChannelClosedError:
            raise ChannelClosedError
        except _interpreters.ChannelNotEmptyError:
            raise ChannelNotEmptyError


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
