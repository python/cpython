:mod:`!interpreters.channels` -- A cross-interpreter "channel" implementation
==================================================================

XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

.. module:: interpreters.channels
   :synopsis: Multiple Interpreters in the Same Process

.. moduleauthor:: Eric Snow <ericsnowcurrently@gmail.com>
.. sectionauthor:: Eric Snow <ericsnowcurrently@gmail.com>

.. versionadded:: 3.14

**Source code:** :source:`Lib/interpreters/channels.py`

--------------


Introduction
------------

This module constructs higher-level interfaces on top of the lower
level ``_interpchannels`` module.

.. seealso::

   :ref:`execcomponents`
       ...

   :mod:`interpreters`
       provides details about communicating between interpreters.

   :ref:`multiple-interpreters-howto`
       demonstrates how to use channels

.. include:: ../includes/wasm-notavail.rst


API Summary
-----------

+-------------------------------------------------------------------+----------------------------------------------+
| signature                                                         | description                                  |
+===================================================================+==============================================+
| ``list_all() -> [(RecvChannel, SendChannel)]``                    | Get all existing cross-interpreter channels. |
+-------------------------------------------------------------------+----------------------------------------------+
| ``create(*, unbounditems=UNBOUND) -> (RecvChannel, SendChannel)`` | Initialize a new cross-interpreter channel.  |
+-------------------------------------------------------------------+----------------------------------------------+

|

+-------------------------------------------+---------------------------------------------------+
| signature                                 | description                                       |
+===========================================+===================================================+
| ``class RecvChannel``                     | The receiving end of a cross-interpreter channel. |
+-------------------------------------------+---------------------------------------------------+
| ``.id``                                   | The channel's ID (read-only).                     |
+-------------------------------------------+---------------------------------------------------+
| ``.is_closed``                            | If the channel has been closed (read-only).       |
+-------------------------------------------+---------------------------------------------------+
| ``.recv(timeout=None)``                   | Pop off the next item in channel.                 |
+-------------------------------------------+---------------------------------------------------+
| ``.recv_nowait(default=None)``            | Pop off the next item in channel, if any.         |
+-------------------------------------------+---------------------------------------------------+
| ``.close()``                              | Stop receiving more items.                        |
+-------------------------------------------+---------------------------------------------------+

|

+------------------------------------------------------------+--------------------------------------------------+
| signature                                                  | description                                      |
+============================================================+==================================================+
| ``class SendChannel``                                      | The sending end of a cross-interpreter channel.  |
+------------------------------------------------------------+--------------------------------------------------+
| ``.id``                                                    | The channel's ID (read-only).                    |
+------------------------------------------------------------+--------------------------------------------------+
| ``.is_closed``                                             | If the channel has been closed (read-only).      |
+------------------------------------------------------------+--------------------------------------------------+
| ``.send(obj, timeout=None, *, unbound=None)``              | Add an item to the back of the channel's queue.  |
+------------------------------------------------------------+--------------------------------------------------+
| ``.send_nowait(obj, *, unbound=None)``                     | Add an item to the back of the channel's queue.  |
+------------------------------------------------------------+--------------------------------------------------+
| ``.send_buffer(obj, timeout=None, *, unbound=None)``       | Add an item to the back of the channel's queue.  |
+------------------------------------------------------------+--------------------------------------------------+
| ``.send_buffer_nowait(obj, *, unbound=None)``              | Add an item to the back of the channel's queue.  |
+------------------------------------------------------------+--------------------------------------------------+
| ``.close()``                                               | Stop sending more items.                         |
+------------------------------------------------------------+--------------------------------------------------+

Exceptions:

+--------------------------+--------------+---------------------------------------------------+
| class                    | base class   | description                                       |
+==========================+==============+===================================================+
| ChannelError             | Exception    | A channel-relaed error happened.                  |
+--------------------------+--------------+---------------------------------------------------+
| ChannelNotFoundError     | ChannelError | The targeted channel no longer exists.            |
+--------------------------+--------------+---------------------------------------------------+
| ChannelClosedError       | ChannelError | The targeted channel has been closed.             |
+--------------------------+--------------+---------------------------------------------------+
| ChannelEmptyError        | ChannelError | The targeted channel is empty.                    |
+--------------------------+--------------+---------------------------------------------------+
| ChannelNotEmptyError     | ChannelError | The targeted channel should have been empty.      |
+--------------------------+--------------+---------------------------------------------------+
| ItemInterpreterDestroyed | ChannelError | The interpreter that added the next item is gone. |
+--------------------------+--------------+---------------------------------------------------+


Basic Usage
-----------

::

    import interpreters.channels

    rch, sch = interpreters.channels.create()
    interp = interpreters.create()
    interp.prepare_main(rch=rch)

    sch.send_nowait('spam!')
    interp.exec("""
        msg = rch.recv()
        print(msg)
        """)

::

    rch, sch = interpreters.channels.create()
    interp = interpreters.create()
    interp.prepare_main(rch=rch)

    data = bytearray(100)

    sch.send_buffer_nowait(data)
    interp.exec("""
        data = rch.recv()
        for i in range(len(data)):
            data[i] = i
        """)
    assert len(data) == 100
    for i in range(len(data)):
        assert data[i] == i


Functions
---------

This module defines the following functions:


