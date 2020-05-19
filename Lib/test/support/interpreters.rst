High-level implementation of Subinterpreters
============================================

**Source code:** :source:`Lib/test/support/_interpreters.py`

--------------

This module provides high-level tools for working with sub-interpreters,
such as creating them, running code in them, or sending data between them.
It is a wrapper around the low-level ``__xxsubinterpreters`` module.

.. versionchanged:: added in 3.9

Interpreter Objects
-------------------

The ``Interpreter`` object represents a single interpreter.

.. class:: Interpreter(id)

    The class implementing a subinterpreter object.

    .. method:: is_running()

       Return ``True`` if the identified interpreter is running.

    .. method:: close()

       Destroy the interpreter. Attempting to destroy the current
       interpreter results in a `RuntimeError`.

    .. method:: run(self, src_str, /, *, channels=None):

       Run the given source code in the interpreter. This blocks
       the current thread until done. ``channels`` should be in
       the form : `(RecvChannel, SendChannel)`.

RecvChannel Objects
-------------------

The ``RecvChannel`` object represents a recieving channel.

.. class:: RecvChannel(id)

    This class represents the receiving end of a channel.

    .. method:: recv()

        Get the next object from the channel, and wait if
        none have been sent. Associate the interpreter
        with the channel.

    .. method:: recv_nowait(default=None)

        Like ``recv()``, but return the default result
        instead of waiting.


SendChannel Objects
--------------------

The ``SendChannel`` object represents a sending channel.

.. class:: SendChannel(id)

    This class represents the sending end of a channel.

    .. method:: send(obj)

       Send the object ``obj`` to the receiving end of the channel
       and wait. Associate the interpreter with the channel.

    .. method:: send_nowait(obj)

        Similar to ``send()``, but returns ``False`` if
        *obj* is not immediately received instead of blocking.


This module defines the following global functions:


.. function:: is_shareable(obj)

   Return ``True`` if the object's data can be shared between
   interpreters.

.. function:: create_channel()

   Create a new channel for passing data between interpreters.

.. function:: list_all_channels()

   Return all open channels.

.. function:: create(*, isolated=True)

   Initialize a new (idle) Python interpreter. Get the currently
   running interpreter. This method returns an ``Interpreter`` object.

.. function:: get_current()

   Get the currently running interpreter. This method returns
   an ``Interpreter`` object.

.. function:: get_main()

   Get the main interpreter. This method returns
   an ``Interpreter`` object.

.. function:: list_all()

   Get all existing interpreters. Returns a list
   of ``Interpreter`` objects.

This module also defines the following exceptions.

.. exception:: RunFailedError

   This exception, a subclass of :exc:`RuntimeError`, is raised when the
   ``Interpreter.run()`` results in an uncaught exception.

.. exception:: ChannelError

   This exception is a subclass of :exc:`Exception`, and is the base
   class for all channel-related exceptions.

.. exception:: ChannelNotFoundError

   This exception is a subclass of :exc:`ChannelError`, and is raised
   when the the identified channel is not found.

.. exception:: ChannelEmptyError

   This exception is a subclass of :exc:`ChannelError`, and is raised when
   the channel is unexpectedly empty.

.. exception:: ChannelNotEmptyError

   This exception is a subclass of :exc:`ChannelError`, and is raised when
   the channel is unexpectedly not empty.

.. exception:: NotReceivedError

   This exception is a subclass of :exc:`ChannelError`, and is raised when
   nothing was waiting to receive a sent object.
