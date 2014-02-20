.. currentmodule:: asyncio

.. _asyncio-streams:

++++++++++++++++++++++++
Streams (high-level API)
++++++++++++++++++++++++

Stream functions
================

.. function:: open_connection(host=None, port=None, \*, loop=None, limit=None, **kwds)

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

   This function is a :ref:`coroutine <coroutine>`.

.. function:: start_server(client_connected_cb, host=None, port=None, \*, loop=None, limit=None, **kwds)

   Start a socket server, with a callback for each client connected.

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

   This function is a :ref:`coroutine <coroutine>`.

.. function:: open_unix_connection(path=None, \*, loop=None, limit=None, **kwds)

   A wrapper for :meth:`~BaseEventLoop.create_unix_connection()` returning
   a (reader, writer) pair.

   See :func:`open_connection` for information about return value and other
   details.

   This function is a :ref:`coroutine <coroutine>`.

   Availability: UNIX.

.. function:: start_unix_server(client_connected_cb, path=None, \*, loop=None, limit=None, **kwds)

   Start a UNIX Domain Socket server, with a callback for each client connected.

   See :func:`start_server` for information about return value and other
   details.

   This function is a :ref:`coroutine <coroutine>`.

   Availability: UNIX.


StreamReader
============

.. class:: StreamReader(limit=None, loop=None)

   .. method:: exception()

      Get the exception.

   .. method:: feed_eof()

      Acknowledge the EOF.

   .. method:: feed_data(data)

      Feed *data* bytes in the internal buffer.  Any operations waiting
      for the data will be resumed.

   .. method:: set_exception(exc)

      Set the exception.

   .. method:: set_transport(transport)

      Set the transport.

   .. method:: read(n=-1)

      Read up to *n* bytes.  If *n* is not provided, or set to ``-1``,
      read until EOF and return all read bytes.

      If the EOF was received and the internal buffer is empty,
      return an empty ``bytes`` object.

      This method is a :ref:`coroutine <coroutine>`.

   .. method:: readline()

      Read one line, where "line" is a sequence of bytes ending with ``\n``.

      If EOF is received, and ``\n`` was not found, the method will
      return the partial read bytes.

      If the EOF was received and the internal buffer is empty,
      return an empty ``bytes`` object.

      This method is a :ref:`coroutine <coroutine>`.

   .. method:: readexactly(n)

      Read exactly *n* bytes. Raise an :exc:`IncompleteReadError` if the end of
      the stream is reached before *n* can be read, the
      :attr:`IncompleteReadError.partial` attribute of the exception contains
      the partial read bytes.

      This method is a :ref:`coroutine <coroutine>`.

   .. method:: at_eof()

      Return ``True`` if the buffer is empty and :meth:`feed_eof` was called.


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

   .. method:: can_write_eof()

      Return :const:`True` if the transport supports :meth:`write_eof`,
      :const:`False` if not. See :meth:`WriteTransport.can_write_eof`.

   .. method:: close()

      Close the transport: see :meth:`BaseTransport.close`.

   .. method:: drain()

      Wait until the write buffer of the underlying transport is flushed.

      This method has an unusual return value. The intended use is to write::

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


IncompleteReadError
===================

.. exception:: IncompleteReadError

    Incomplete read error, subclass of :exc:`EOFError`.

   .. attribute:: expected

      Total number of expected bytes (:class:`int`).

   .. attribute:: partial

      Read bytes string before the end of stream was reached (:class:`bytes`).


Example
=======

Simple example querying HTTP headers of the URL passed on the command line::

    import asyncio
    import urllib.parse
    import sys

    @asyncio.coroutine
    def print_http_headers(url):
        url = urllib.parse.urlsplit(url)
        reader, writer = yield from asyncio.open_connection(url.hostname, 80)
        query = ('HEAD {url.path} HTTP/1.0\r\n'
                 'Host: {url.hostname}\r\n'
                 '\r\n').format(url=url)
        writer.write(query.encode('latin-1'))
        while True:
            line = yield from reader.readline()
            if not line:
                break
            line = line.decode('latin1').rstrip()
            if line:
                print('HTTP header> %s' % line)

    url = sys.argv[1]
    loop = asyncio.get_event_loop()
    task = asyncio.async(print_http_headers(url))
    loop.run_until_complete(task)
    loop.close()

Usage::

    python example.py http://example.com/path/page.html

