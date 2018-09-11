.. currentmodule:: asyncio

.. _asyncio-streams:

=======
Streams
=======

Streams are high-level async/await-ready primitives to work with
network connections.  Streams allow send and receive data without
using callbacks or low-level protocols and transports.

Here's an example of a TCP echo client written using asyncio
streams::

    import asyncio

    async def tcp_echo_client(message):
        reader, writer = await asyncio.open_connection(
            '127.0.0.1', 8888)

        print(f'Send: {message!r}')
        writer.write(message.encode())

        data = await reader.read(100)
        print(f'Received: {data.decode()!r}')

        print('Close the connection')
        writer.close()

    asyncio.run(tcp_echo_client('Hello World!'))


.. rubric:: Stream Functions

The following top-level asyncio functions can be used to create
and work with streams:


.. coroutinefunction:: open_connection(host=None, port=None, \*, \
                          loop=None, limit=None, ssl=None, family=0, \
                          proto=0, flags=0, sock=None, local_addr=None, \
                          server_hostname=None, ssl_handshake_timeout=None)

   Establish a network connection and return a pair of
   ``(reader, writer)``.

   The returned *reader* and *writer* objects are instances of
   :class:`StreamReader` and :class:`StreamWriter` classes.

   The *loop* argument is optional and can always be determined
   automatically when this method is awaited from a coroutine.

   *limit* determines the buffer size limit used by the
   returned :class:`StreamReader` instance.

   The rest of the arguments are passed directly to
   :meth:`loop.create_connection`.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* parameter.

.. coroutinefunction:: start_server(client_connected_cb, host=None, \
                          port=None, \*, loop=None, limit=None, \
                          family=socket.AF_UNSPEC, \
                          flags=socket.AI_PASSIVE, sock=None, \
                          backlog=100, ssl=None, reuse_address=None, \
                          reuse_port=None, ssl_handshake_timeout=None, \
                          start_serving=True)

   Start a socket server.

   The *client_connected_cb* callback is called whenever a new client
   connection is established.  It receives a ``(reader, writer)`` pair
   as two arguments, instances of the :class:`StreamReader` and
   :class:`StreamWriter` classes.

   *client_connected_cb* can be a plain callable or a
   :ref:`coroutine function <coroutine>`; if it is a coroutine function,
   it will be automatically wrapped into a :class:`Task`.

   The *loop* argument is optional and can always be determined
   automatically when this method is awaited from a coroutine.

   *limit* determines the buffer size limit used by the
   returned :class:`StreamReader` instance.

   The rest of the arguments are passed directly to
   :meth:`loop.create_server`.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* and *start_serving* parameters.

.. coroutinefunction:: open_unix_connection(path=None, \*, loop=None, \
                        limit=None, ssl=None, sock=None, \
                        server_hostname=None, ssl_handshake_timeout=None)

   Establish a UNIX socket connection and return a pair of
   ``(reader, writer)``.

   Similar to :func:`open_connection` but operates on UNIX sockets.

   See also the documentation of :meth:`loop.create_unix_connection`.

   Availability: UNIX.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* parameter.

   .. versionchanged:: 3.7

      The *path* parameter can now be a :term:`path-like object`

.. coroutinefunction:: start_unix_server(client_connected_cb, path=None, \
                          \*, loop=None, limit=None, sock=None, \
                          backlog=100, ssl=None, ssl_handshake_timeout=None, \
                          start_serving=True)

   Start a UNIX socket server.

   Similar to :func:`start_server` but operates on UNIX sockets.

   See also the documentation of :meth:`loop.create_unix_server`.

   Availability: UNIX.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* and *start_serving* parameters.

   .. versionchanged:: 3.7

      The *path* parameter can now be a :term:`path-like object`.


.. rubric:: Contents

* `StreamReader`_ and `StreamWriter`_
* `StreamReaderProtocol`_
* `Examples`_


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

   .. coroutinemethod:: readline()

      Read one line, where "line" is a sequence of bytes ending with ``\n``.

      If EOF is received, and ``\n`` was not found, the method will
      return the partial read bytes.

      If the EOF was received and the internal buffer is empty,
      return an empty ``bytes`` object.

   .. coroutinemethod:: readexactly(n)

      Read exactly *n* bytes. Raise an :exc:`IncompleteReadError` if the end of
      the stream is reached before *n* can be read, the
      :attr:`IncompleteReadError.partial` attribute of the exception contains
      the partial read bytes.

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

      Return ``True`` if the buffer is empty and :meth:`feed_eof`
      was called.


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

.. class:: StreamReaderProtocol(stream_reader, client_connected_cb=None, \
                loop=None)

    Trivial helper class to adapt between :class:`Protocol` and
    :class:`StreamReader`. Subclass of :class:`Protocol`.

    *stream_reader* is a :class:`StreamReader` instance, *client_connected_cb*
    is an optional function called with (stream_reader, stream_writer) when a
    connection is made, *loop* is the event loop instance to use.

    (This is a helper class instead of making :class:`StreamReader` itself a
    :class:`Protocol` subclass, because the :class:`StreamReader` has other
    potential uses, and to prevent the user of the :class:`StreamReader` from
    accidentally calling inappropriate methods of the protocol.)


Examples
========

.. _asyncio-tcp-echo-client-streams:

TCP echo client using streams
-----------------------------

TCP echo client using the :func:`asyncio.open_connection` function::

    import asyncio

    async def tcp_echo_client(message):
        reader, writer = await asyncio.open_connection(
            '127.0.0.1', 8888)

        print(f'Send: {message!r}')
        writer.write(message.encode())

        data = await reader.read(100)
        print(f'Received: {data.decode()!r}')

        print('Close the connection')
        writer.close()

    asyncio.run(tcp_echo_client('Hello World!'))


.. seealso::

   The :ref:`TCP echo client protocol <asyncio-tcp-echo-client-protocol>`
   example uses the low-level :meth:`loop.create_connection` method.


.. _asyncio-tcp-echo-server-streams:

TCP echo server using streams
-----------------------------

TCP echo server using the :func:`asyncio.start_server` function::

    import asyncio

    async def handle_echo(reader, writer):
        data = await reader.read(100)
        message = data.decode()
        addr = writer.get_extra_info('peername')

        print(f"Received {message!r} from {addr!r}")

        print(f"Send: {message!r}")
        writer.write(data)
        await writer.drain()

        print("Close the connection")
        writer.close()

    async def main():
        server = await asyncio.start_server(
            handle_echo, '127.0.0.1', 8888)

        addr = server.sockets[0].getsockname()
        print(f'Serving on {addr}')

        async with server:
            await server.serve_forever()

    asyncio.run(main())


.. seealso::

   The :ref:`TCP echo server protocol <asyncio-tcp-echo-server-protocol>`
   example uses the :meth:`loop.create_server` method.


Get HTTP headers
----------------

Simple example querying HTTP headers of the URL passed on the command line::

    import asyncio
    import urllib.parse
    import sys

    async def print_http_headers(url):
        url = urllib.parse.urlsplit(url)
        if url.scheme == 'https':
            reader, writer = await asyncio.open_connection(
                url.hostname, 443, ssl=True)
        else:
            reader, writer = await asyncio.open_connection(
                url.hostname, 80)

        query = (
            f"HEAD {url.path or '/'} HTTP/1.0\r\n"
            f"Host: {url.hostname}\r\n"
            f"\r\n"
        )

        writer.write(query.encode('latin-1'))
        while True:
            line = await reader.readline()
            if not line:
                break

            line = line.decode('latin1').rstrip()
            if line:
                print(f'HTTP header> {line}')

        # Ignore the body, close the socket
        writer.close()

    url = sys.argv[1]
    asyncio.run(print_http_headers(url))


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
    import socket

    async def wait_for_data():
        # Get a reference to the current event loop because
        # we want to access low-level APIs.
        loop = asyncio.get_running_loop()

        # Create a pair of connected sockets.
        rsock, wsock = socket.socketpair()

        # Register the open socket to wait for data.
        reader, writer = await asyncio.open_connection(sock=rsock)

        # Simulate the reception of data from the network
        loop.call_soon(wsock.send, 'abc'.encode())

        # Wait for data
        data = await reader.read(100)

        # Got data, we are done: close the socket
        print("Received:", data.decode())
        writer.close()

        # Close the second socket
        wsock.close()

    asyncio.run(wait_for_data())

.. seealso::

   The :ref:`register an open socket to wait for data using a protocol
   <asyncio-register-socket>` example uses a low-level protocol and
   the :meth:`loop.create_connection` method.

   The :ref:`watch a file descriptor for read events
   <asyncio-watch-read-event>` example uses the low-level
   :meth:`loop.add_reader` method to watch a file descriptor.
