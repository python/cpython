.. currentmodule:: asyncio

+++++++
Streams
+++++++

Stream functions
================

.. function:: open_connection(host=None, port=None, *, loop=None, limit=_DEFAULT_LIMIT, **kwds)

   A wrapper for :meth:`~BaseEventLoop.create_connection()` returning a (reader,
   writer) pair.

   The reader returned is a :class:`StreamReader` instance; the writer is
   a :class:`StreamWriter` instance.

   The arguments are all the usual arguments to
   :meth:`BaseEventLoop.create_connection` except *protocol_factory*; most
   common are positional host and port, with various optional keyword arguments
   following.

   Additional optional keyword arguments are *loop* (to set the event loop
   instance to use) and *limit* (to set the buffer limit passed to the
   :class:`StreamReader`).

   (If you want to customize the :class:`StreamReader` and/or
   :class:`StreamReaderProtocol` classes, just copy the code -- there's really
   nothing special here except some convenience.)

   This function returns a :ref:`coroutine object <coroutine>`.

.. function:: start_server(client_connected_cb, host=None, port=None, *, loop=None, limit=_DEFAULT_LIMIT, **kwds)

   Start a socket server, call back for each client connected.

   The first parameter, *client_connected_cb*, takes two parameters:
   *client_reader*, *client_writer*.  *client_reader* is a
   :class:`StreamReader` object, while *client_writer* is a
   :class:`StreamWriter` object.  This parameter can either be a plain callback
   function or a :ref:`coroutine function <coroutine>`; if it is a coroutine
   function, it will be automatically converted into a :class:`Task`.

   The rest of the arguments are all the usual arguments to
   :meth:`~BaseEventLoop.create_server()` except *protocol_factory*; most
   common are positional host and port, with various optional keyword arguments
   following.  The return value is the same as
   :meth:`~BaseEventLoop.create_server()`.

   Additional optional keyword arguments are *loop* (to set the event loop
   instance to use) and *limit* (to set the buffer limit passed to the
   :class:`StreamReader`).

   The return value is the same as :meth:`~BaseEventLoop.create_server()`, i.e.
   a :class:`AbstractServer` object which can be used to stop the service.

   This function returns a :ref:`coroutine object <coroutine>`.


StreamReader
============

.. class:: StreamReader(limit=_DEFAULT_LIMIT, loop=None)

   .. method:: exception()

      Get the exception.

   .. method:: feed_eof()

      XXX

   .. method:: feed_data(data)

      XXX

   .. method:: set_exception(exc)

      Set the exception.

   .. method:: set_transport(transport)

      Set the transport.

   .. method:: read(n=-1)

      XXX

      This method returns a :ref:`coroutine object <coroutine>`.

   .. method:: readline()

      XXX

      This method returns a :ref:`coroutine object <coroutine>`.

   .. method:: readexactly(n)

      XXX

      This method returns a :ref:`coroutine object <coroutine>`.


StreamWriter
============

.. class:: StreamWriter(transport, protocol, reader, loop)

   Wraps a Transport.

   This exposes :meth:`write`, :meth:`writelines`, :meth:`can_write_eof()`,
   :meth:`write_eof`, :meth:`get_extra_info` and :meth:`close`.  It adds
   :meth:`drain` which returns an optional :class:`Future` on which you can
   wait for flow control.  It also adds a transport attribute which references
   the :class:`Transport` directly.

   .. attribute:: transport

      Transport.

   .. method:: close()

      Close the transport: see :meth:`BaseTransport.close`.

   .. method:: drain()

      This method has an unusual return value.

      The intended use is to write::

          w.write(data)
          yield from w.drain()

      When there's nothing to wait for, :meth:`drain()` returns ``()``, and the
      yield-from continues immediately.  When the transport buffer is full (the
      protocol is paused), :meth:`drain` creates and returns a
      :class:`Future` and the yield-from will block until
      that Future is completed, which will happen when the buffer is
      (partially) drained and the protocol is resumed.

   .. method:: get_extra_info(name, default=None)

      Return optional transport information: see
      :meth:`BaseTransport.get_extra_info`.

   .. method:: write(data)

      Write some *data* bytes to the transport: see
      :meth:`WriteTransport.write`.

   .. method:: writelines(data)

      Write a list (or any iterable) of data bytes to the transport:
      see :meth:`WriteTransport.writelines`.

   .. method:: can_write_eof()

      Return :const:`True` if the transport supports :meth:`write_eof`,
      :const:`False` if not. See :meth:`WriteTransport.can_write_eof`.

   .. method:: write_eof()

      Close the write end of the transport after flushing buffered data:
      see :meth:`WriteTransport.write_eof`.


StreamReaderProtocol
====================

.. class:: StreamReaderProtocol(stream_reader, client_connected_cb=None, loop=None)

    Trivial helper class to adapt between :class:`Protocol` and
    :class:`StreamReader`. Sublclass of :class:`Protocol`.

    *stream_reader* is a :class:`StreamReader` instance, *client_connected_cb*
    is an optional function called with (stream_reader, stream_writer) when a
    connection is made, *loop* is the event loop instance to use.

    (This is a helper class instead of making :class:`StreamReader` itself a
    :class:`Protocol` subclass, because the :class:`StreamReader` has other
    potential uses, and to prevent the user of the :class:`StreamReader` to
    accidentally call inappropriate methods of the protocol.)

    .. method:: connection_made(transport)

       XXX

    .. method:: connection_lost(exc)

       XXX

    .. method:: data_received(data)

       XXX

    .. method:: eof_received()

       XXX

    .. method:: pause_writing()

       XXX

    .. method:: resume_writing()

       XXX

