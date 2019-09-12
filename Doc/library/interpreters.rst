:mod:`interpreters` --- High-level Sub-interpreters Module
==========================================================

.. module:: interpreters
   :synopsis: High-level Sub-Interpreters Module.

**Source code:** :source:`Lib/interpreters.py`

--------------

This module constructs higher-level interpreters interfaces on top of the lower
level :mod:`_interpreters` module.

.. versionchanged:: 3.9

Interpreter Objects
-------------------

The Interpreter object represents a single interpreter.

.. class:: Interpreter(id)

    The class implementing a subinterpreter object.

    .. method:: is_running()

       Return whether or not the identified interpreter is running.

    .. method:: destroy()

       Destroy the identified interpreter. Attempting to destroy the current
       interpreter results in a RuntimeError. So does an unrecognized ID.

    .. method:: run(self, src_str, /, *, channels=None):

       Run the given source code in the interpreter. This blocks the current
       thread until done.

RecvChannel Objects
-------------------

The RecvChannel object represents a recieving channel.

.. class:: RecvChannel(id)

    This class represents the receiving end of a channel.

    .. method:: recv()

        Get the next object from the channel, and wait if none have been
        sent. Associate the interpreter with the channel.

    .. method:: recv_nowait(default=None)

        Like ``recv()``, but return the default instead of waiting.

     .. method:: release()

        No longer associate the current interpreter with the channel
        (on the sending end).

    .. method:: close(force=False)

        Close the channel in all interpreters.


SendChannel Objects
--------------------

The SendChannel object represents a sending channel.

.. class:: SendChannel(id)

    This class represents the receiving end of a channel.

    .. method:: send(obj)

       Send the object (i.e. its data) to the receiving end of the channel
       and wait.Associate the interpreter with the channel.

    .. method:: send_nowait(obj)

        Like ``send()``, but return False if not received.

    .. method:: send_buffer(obj)

       Send the object's buffer to the receiving end of the channel and wait.
       Associate the interpreter with the channel.

    .. method:: send_buffer_nowait(obj)

       Like ``send_buffer()``, but return False if not received.

    .. method:: release()

       No longer associate the current interpreter with the channel
       (on the sending end).

    .. method:: close(force=False)

        Close the channel in all interpreters.


This module defines the following global functions:


.. function:: is_shareable(obj)

   Return `True` if the object's data can be shared between interpreters.

.. function:: create_channel()

   Create a new channel for passing data between interpreters.

.. function:: list_all_channels()

   Return all open channels.

.. function:: create()

   Initialize a new (idle) Python interpreter.

.. function:: get_current()

   Get the currently running interpreter.

.. function:: list_all()

   Get all existing interpreters.

This module also defines the following exceptions.

.. exception:: RunFailedError

   This exception, a subclass of :exc:`RuntimeError`, is raised when the
   ``Interpreter.run()`` results in an uncaught exception.

.. exception:: ChannelError

   This exception, a subclass of :exc:`Exception`, and is the base class for
   channel-related exceptions.

.. exception:: ChannelNotFoundError

   This exception, a subclass of :exc:`ChannelError`, is raised when the
   the identified channel was not found.

.. exception:: ChannelEmptyError

   This exception, a subclass of :exc:`ChannelError`, is raised when
   the channel is unexpectedly empty.

.. exception:: ChannelNotEmptyError

   This exception, a subclass of :exc:`ChannelError`, is raised when
   the channel is unexpectedly not empty.

.. exception:: NotReceivedError

   This exception, a subclass of :exc:`ChannelError`, is raised when
   nothing was waiting to receive a sent object.

.. exception:: ChannelClosedError

   This exception, a subclass of :exc:`ChannelError`, is raised when
   the channel is closed.

.. exception:: ChannelReleasedError

   This exception, a subclass of :exc:`ChannelClosedError`, is raised when
   the channel is released (but not yet closed).
