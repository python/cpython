.. currentmodule:: asyncio

.. _asyncio-streams:

+++++++++++++++++++++++++++++
Streams (coroutine based API)
+++++++++++++++++++++++++++++

**Source code:** :source:`Lib/asyncio/streams.py`

Stream functions
================

.. note::

   The top-level functions in this module are meant as convenience wrappers
   only; there's really nothing special there, and if they don't do
   exactly what you want, feel free to copy their code.


.. coroutinefunction:: open_connection(host=None, port=None, \*, loop=None, limit=None, ssl=None, family=0, proto=0, flags=0, sock=None, local_addr=None, server_hostname=None, ssl_handshake_timeout=None)

   A wrapper for :meth:`~AbstractEventLoop.create_connection()` returning a (reader,
   writer) pair.

   The reader returned is a :class:`StreamReader` instance; the writer is
   a :class:`StreamWriter` instance.

   When specified, the *loop* argument determines which event loop to use,
   and the *limit* argument determines the buffer size limit used by the
   returned :class:`StreamReader` instance.

   The rest of the arguments are passed directly to
   :meth:`AbstractEventLoop.create_connection`.

   This function is a :ref:`coroutine <coroutine>`.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* parameter.

.. coroutinefunction:: start_server(client_connected_cb, host=None, port=None, \*, loop=None, limit=None, family=socket.AF_UNSPEC, flags=socket.AI_PASSIVE, sock=None, backlog=100, ssl=None, reuse_address=None, reuse_port=None, ssl_handshake_timeout=None, start_serving=True)

   Start a socket server, with a callback for each client connected. The return
   value is the same as :meth:`~AbstractEventLoop.create_server()`.

   The *client_connected_cb* callback is called whenever a new client
   connection is established.  It receives a reader/writer pair as two
   arguments, the first is a :class:`StreamReader` instance,
   and the second is a :class:`StreamWriter` instance.

   *client_connected_cb* accepts a plain callable or a
   :ref:`coroutine function <coroutine>`; if it is a coroutine function,
   it will be automatically converted into a :class:`Task`.

   When specified, the *loop* argument determines which event loop to use,
   and the *limit* argument determines the buffer size limit used by the
   :class:`StreamReader` instance passed to *client_connected_cb*.

   The rest of the arguments are passed directly to
   :meth:`~AbstractEventLoop.create_server()`.

   This function is a :ref:`coroutine <coroutine>`.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* and *start_serving* parameters.

.. coroutinefunction:: open_unix_connection(path=None, \*, loop=None, limit=None, ssl=None, sock=None, server_hostname=None, ssl_handshake_timeout=None)

   A wrapper for :meth:`~AbstractEventLoop.create_unix_connection()` returning
   a (reader, writer) pair.

   When specified, the *loop* argument determines which event loop to use,
   and the *limit* argument determines the buffer size limit used by the
   returned :class:`StreamReader` instance.

   The rest of the arguments are passed directly to
   :meth:`~AbstractEventLoop.create_unix_connection()`.

   This function is a :ref:`coroutine <coroutine>`.

   Availability: UNIX.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* parameter.

   .. versionchanged:: 3.7

      The *path* parameter can now be a :term:`path-like object`

.. coroutinefunction:: start_unix_server(client_connected_cb, path=None, \*, loop=None, limit=None, sock=None, backlog=100, ssl=None, ssl_handshake_timeout=None, start_serving=True)

   Start a UNIX Domain Socket server, with a callback for each client connected.

   The *client_connected_cb* callback is called whenever a new client
   connection is established.  It receives a reader/writer pair as two
   arguments, the first is a :class:`StreamReader` instance,
   and the second is a :class:`StreamWriter` instance.

   *client_connected_cb* accepts a plain callable or a
   :ref:`coroutine function <coroutine>`; if it is a coroutine function,
   it will be automatically converted into a :class:`Task`.

   When specified, the *loop* argument determines which event loop to use,
   and the *limit* argument determines the buffer size limit used by the
   :class:`StreamReader` instance passed to *client_connected_cb*.

   The rest of the arguments are passed directly to
   :meth:`~AbstractEventLoop.create_unix_server()`.

   This function is a :ref:`coroutine <coroutine>`.

   Availability: UNIX.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* and *start_serving* parameters.

   .. versionchanged:: 3.7

      The *path* parameter can now be a :term:`path-like object`.


StreamReader
============

.. class:: StreamReader(limit=None, loop=None)

   This class is :ref:`not thread safe <asyncio-multithreading>`.

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

   .. coroutinemethod:: read(n=-1)

      Read up to *n* bytes.  If *n* is not provided, or set to ``-1``,
      read until EOF and return all read bytes.

      If the EOF was received and the internal buffer is empty,
      return an empty ``bytes`` object.

      This method is a :ref:`coroutine <coroutine>`.

   .. coroutinemethod:: readline()

      Read one line, where "line" is a sequence of bytes ending with ``\n``.

      If EOF is received, and ``\n`` was not found, the method will
      return the partial read bytes.

      If the EOF was received and the internal buffer is empty,
      return an empty ``bytes`` object.

      This method is a :ref:`coroutine <coroutine>`.

   .. coroutinemethod:: readexactly(n)

      Read exactly *n* bytes. Raise an :exc:`IncompleteReadError` if the end of
      the stream is reached before *n* can be read, the
      :attr:`IncompleteReadError.partial` attribute of the exception contains
      the partial read bytes.

      This method is a :ref:`coroutine <coroutine>`.

   .. coroutinemethod:: readuntil(separator=b'\\n')

      Read data from the stream until ``separator`` is found.

      On success, the data and separator will be removed from the
      internal buffer (consumed). Returned data will include the
      separator at the end.

      Configured stream limit is used to check result. Limit sets the
      maximal length of data that can be returned, not counting the
      separator.

      If an EOF occurs and the complete separator is still not found,
      an :exc:`IncompleteReadError` exception will be
      raised, and the internal buffer will be reset.  The
      :attr:`IncompleteReadError.partial` attribute may contain the
      separator partially.

      If the data cannot be read because of over limit, a
      :exc:`LimitOverrunError` exception  will be raised, and the data
      will be left in the internal buffer, so it can be read again.

      .. versionadded:: 3.5.2

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

   This class is :ref:`not thread safe <asyncio-multithreading>`.

   .. attribute:: transport

      Transport.

   .. method:: can_write_eof()

      Return :const:`True` if the transport supports :meth:`write_eof`,
      :const:`False` if not. See :meth:`WriteTransport.can_write_eof`.

   .. method:: close()

      Close the transport: see :meth:`BaseTransport.close`.

   .. method:: is_closing()

      Return ``True`` if the writer is closing or is closed.

      .. versionadded:: 3.7

   .. coroutinemethod:: wait_closed()

      Wait until the writer is closed.

      Should be called after :meth:`close`  to wait until the underlying
      connection (and the associated transport/protocol pair) is closed.

      .. versionadded:: 3.7

   .. coroutinemethod:: drain()

      Let the write buffer of the underlying transport a chance to be flushed.

      The intended use is to write::

          w.write(data)
          await w.drain()

      When the size of the transport buffer reaches the high-water limit (the
      protocol is paused), block until the size of the buffer is drained down
      to the low-water limit and the protocol is resumed. When there is nothing
      to wait for, the yield-from continues immediately.

      Yielding from :meth:`drain` gives the opportunity for the loop to
      schedule the write operation and flush the buffer. It should especially
      be used when a possibly large amount of data is written to the transport,
      and the coroutine does not yield-from between calls to :meth:`write`.

      This method is a :ref:`coroutine <coroutine>`.

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
    :class:`StreamReader`. Subclass of :class:`Protocol`.

    *stream_reader* is a :class:`StreamReader` instance, *client_connected_cb*
    is an optional function called with (stream_reader, stream_writer) when a
    connection is made, *loop* is the event loop instance to use.

    (This is a helper class instead of making :class:`StreamReader` itself a
    :class:`Protocol` subclass, because the :class:`StreamReader` has other
    potential uses, and to prevent the user of the :class:`StreamReader` from
    accidentally calling inappropriate methods of the protocol.)


IncompleteReadError
===================

.. exception:: IncompleteReadError

    Incomplete read error, subclass of :exc:`EOFError`.

   .. attribute:: expected

      Total number of expected bytes (:class:`int`).

   .. attribute:: partial

      Read bytes string before the end of stream was reached (:class:`bytes`).


LimitOverrunError
=================

.. exception:: LimitOverrunError

   Reached the buffer limit while looking for a separator.

   .. attribute:: consumed

      Total number of to be consumed bytes.


Stream examples
===============

.. _asyncio-tcp-echo-client-streams:

TCP echo client using streams
-----------------------------

TCP echo client using the :func:`asyncio.open_connection` function::

    import asyncio

    async def tcp_echo_client(message, loop):
        reader, writer = await asyncio.open_connection('127.0.0.1', 8888,
                                                       loop=loop)

        print('Send: %r' % message)
        writer.write(message.encode())

        data = await reader.read(100)
        print('Received: %r' % data.decode())

        print('Close the socket')
        writer.close()

    message = 'Hello World!'
    loop = asyncio.get_event_loop()
    loop.run_until_complete(tcp_echo_client(message, loop))
    loop.close()

.. seealso::

   The :ref:`TCP echo client protocol <asyncio-tcp-echo-client-protocol>`
   example uses the :meth:`AbstractEventLoop.create_connection` method.


.. _asyncio-tcp-echo-server-streams:

TCP echo server using streams
-----------------------------

TCP echo server using the :func:`asyncio.start_server` function::

    import asyncio

    async def handle_echo(reader, writer):
        data = await reader.read(100)
        message = data.decode()
        addr = writer.get_extra_info('peername')
        print("Received %r from %r" % (message, addr))

        print("Send: %r" % message)
        writer.write(data)
        await writer.drain()

        print("Close the client socket")
        writer.close()

    loop = asyncio.get_event_loop()
    coro = asyncio.start_server(handle_echo, '127.0.0.1', 8888, loop=loop)
    server = loop.run_until_complete(coro)

    # Serve requests until Ctrl+C is pressed
    print('Serving on {}'.format(server.sockets[0].getsockname()))
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass

    # Close the server
    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()

.. seealso::

   The :ref:`TCP echo server protocol <asyncio-tcp-echo-server-protocol>`
   example uses the :meth:`AbstractEventLoop.create_server` method.


Get HTTP headers
----------------

Simple example querying HTTP headers of the URL passed on the command line::

    import asyncio
    import urllib.parse
    import sys

    @asyncio.coroutine
    def print_http_headers(url):
        url = urllib.parse.urlsplit(url)
        if url.scheme == 'https':
            connect = asyncio.open_connection(url.hostname, 443, ssl=True)
        else:
            connect = asyncio.open_connection(url.hostname, 80)
        reader, writer = await connect
        query = ('HEAD {path} HTTP/1.0\r\n'
                 'Host: {hostname}\r\n'
                 '\r\n').format(path=url.path or '/', hostname=url.hostname)
        writer.write(query.encode('latin-1'))
        while True:
            line = await reader.readline()
            if not line:
                break
            line = line.decode('latin1').rstrip()
            if line:
                print('HTTP header> %s' % line)

        # Ignore the body, close the socket
        writer.close()

    url = sys.argv[1]
    loop = asyncio.get_event_loop()
    task = asyncio.ensure_future(print_http_headers(url))
    loop.run_until_complete(task)
    loop.close()

Usage::

    python example.py http://example.com/path/page.html

or with HTTPS::

    python example.py https://example.com/path/page.html

.. _asyncio-register-socket-streams:

Register an open socket to wait for data using streams
------------------------------------------------------

Coroutine waiting until a socket receives data using the
:func:`open_connection` function::

    import asyncio
    from socket import socketpair

    async def wait_for_data(loop):
        # Create a pair of connected sockets
        rsock, wsock = socketpair()

        # Register the open socket to wait for data
        reader, writer = await asyncio.open_connection(sock=rsock, loop=loop)

        # Simulate the reception of data from the network
        loop.call_soon(wsock.send, 'abc'.encode())

        # Wait for data
        data = await reader.read(100)

        # Got data, we are done: close the socket
        print("Received:", data.decode())
        writer.close()

        # Close the second socket
        wsock.close()

    loop = asyncio.get_event_loop()
    loop.run_until_complete(wait_for_data(loop))
    loop.close()

.. seealso::

   The :ref:`register an open socket to wait for data using a protocol
   <asyncio-register-socket>` example uses a low-level protocol created by the
   :meth:`AbstractEventLoop.create_connection` method.

   The :ref:`watch a file descriptor for read events
   <asyncio-watch-read-event>` example uses the low-level
   :meth:`AbstractEventLoop.add_reader` method to register the file descriptor of a
   socket.
