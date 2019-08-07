.. currentmodule:: asyncio

.. _asyncio-streams:

=======
Streams
=======

Streams are high-level async/await-ready primitives to work with
network connections.  Streams allow sending and receiving data without
using callbacks or low-level protocols and transports.

.. _asyncio_example_stream:

Here is an example of a TCP echo client written using asyncio
streams::

    import asyncio

    async def tcp_echo_client(message):
        async with asyncio.connect('127.0.0.1', 8888) as stream:
            print(f'Send: {message!r}')
            await stream.write(message.encode())

            data = await stream.read(100)
            print(f'Received: {data.decode()!r}')

    asyncio.run(tcp_echo_client('Hello World!'))


See also the `Examples`_ section below.


.. rubric:: Stream Functions

The following top-level asyncio functions can be used to create
and work with streams:


.. coroutinefunction:: connect(host=None, port=None, \*, \
                               limit=2**16, ssl=None, family=0, \
                               proto=0, flags=0, sock=None, local_addr=None, \
                               server_hostname=None, ssl_handshake_timeout=None, \
                               happy_eyeballs_delay=None, interleave=None)

   Connect to TCP socket on *host* : *port* address and return a :class:`Stream`
   object of mode :attr:`StreamMode.READWRITE`.


   *limit* determines the buffer size limit used by the returned :class:`Stream`
   instance. By default the *limit* is set to 64 KiB.

   The rest of the arguments are passed directly to :meth:`loop.create_connection`.

   The function can be used with ``await`` to get a connected stream::

       stream = await asyncio.connect('127.0.0.1', 8888)

   The function can also be used as an async context manager::

       async with asyncio.connect('127.0.0.1', 8888) as stream:
           ...

   .. versionadded:: 3.8

.. coroutinefunction:: open_connection(host=None, port=None, \*, \
                          loop=None, limit=None, ssl=None, family=0, \
                          proto=0, flags=0, sock=None, local_addr=None, \
                          server_hostname=None, ssl_handshake_timeout=None)

   Establish a network connection and return a pair of
   ``(reader, writer)`` objects.

   The returned *reader* and *writer* objects are instances of
   :class:`StreamReader` and :class:`StreamWriter` classes.

   The *loop* argument is optional and can always be determined
   automatically when this function is awaited from a coroutine.

   *limit* determines the buffer size limit used by the
   returned :class:`StreamReader` instance.  By default the *limit*
   is set to 64 KiB.

   The rest of the arguments are passed directly to
   :meth:`loop.create_connection`.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* parameter.

   .. deprecated-removed:: 3.8 3.10

      `open_connection()` is deprecated in favor of :func:`connect`.

.. coroutinefunction:: start_server(client_connected_cb, host=None, \
                          port=None, \*, loop=None, limit=2**16, \
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
   it will be automatically scheduled as a :class:`Task`.

   The *loop* argument is optional and can always be determined
   automatically when this method is awaited from a coroutine.

   *limit* determines the buffer size limit used by the
   returned :class:`StreamReader` instance.  By default the *limit*
   is set to 64 KiB.

   The rest of the arguments are passed directly to
   :meth:`loop.create_server`.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* and *start_serving* parameters.

   .. deprecated-removed:: 3.8 3.10

      `start_server()` is deprecated if favor of :class:`StreamServer`

.. coroutinefunction:: connect_read_pipe(pipe, *, limit=2**16)

   Takes a :term:`file-like object <file object>` *pipe* to return a
   :class:`Stream` object of the mode :attr:`StreamMode.READ` that has
   similar API of :class:`StreamReader`. It can also be used as an async context manager.

   *limit* determines the buffer size limit used by the returned :class:`Stream`
   instance. By default the limit is set to 64 KiB.

   .. versionadded:: 3.8

.. coroutinefunction:: connect_write_pipe(pipe, *, limit=2**16)

   Takes a :term:`file-like object <file object>` *pipe* to return a
   :class:`Stream` object of the mode :attr:`StreamMode.WRITE` that has
   similar API of :class:`StreamWriter`. It can also be used as an async context manager.

   *limit* determines the buffer size limit used by the returned :class:`Stream`
   instance. By default the limit is set to 64 KiB.

   .. versionadded:: 3.8

.. rubric:: Unix Sockets

.. function:: connect_unix(path=None, *, limit=2**16, ssl=None, \
                           sock=None, server_hostname=None, \
                           ssl_handshake_timeout=None)

   Establish a Unix socket connection to socket with *path* address and
   return an awaitable :class:`Stream` object of the mode :attr:`StreamMode.READWRITE`
   that can be used as a reader and a writer.

   *limit* determines the buffer size limit used by the returned :class:`Stream`
   instance. By default the *limit* is set to 64 KiB.

   The rest of the arguments are passed directly to :meth:`loop.create_unix_connection`.

   The function can be used with ``await`` to get a connected stream::

       stream = await asyncio.connect_unix('/tmp/example.sock')

   The function can also be used as an async context manager::

       async with asyncio.connect_unix('/tmp/example.sock') as stream:
           ...

   .. availability:: Unix.

   .. versionadded:: 3.8

.. coroutinefunction:: open_unix_connection(path=None, \*, loop=None, \
                        limit=None, ssl=None, sock=None, \
                        server_hostname=None, ssl_handshake_timeout=None)

   Establish a Unix socket connection and return a pair of
   ``(reader, writer)``.

   Similar to :func:`open_connection` but operates on Unix sockets.

   See also the documentation of :meth:`loop.create_unix_connection`.

   .. availability:: Unix.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* parameter.

   .. versionchanged:: 3.7

      The *path* parameter can now be a :term:`path-like object`

   .. deprecated-removed:: 3.8 3.10

      ``open_unix_connection()`` is deprecated if favor of :func:`connect_unix`.


.. coroutinefunction:: start_unix_server(client_connected_cb, path=None, \
                          \*, loop=None, limit=None, sock=None, \
                          backlog=100, ssl=None, ssl_handshake_timeout=None, \
                          start_serving=True)

   Start a Unix socket server.

   Similar to :func:`start_server` but works with Unix sockets.

   See also the documentation of :meth:`loop.create_unix_server`.

   .. availability:: Unix.

   .. versionadded:: 3.7

      The *ssl_handshake_timeout* and *start_serving* parameters.

   .. versionchanged:: 3.7

      The *path* parameter can now be a :term:`path-like object`.

   .. deprecated-removed:: 3.8 3.10

      ``start_unix_server()`` is deprecated in favor of :class:`UnixStreamServer`.


---------

StreamServer
============

.. class:: StreamServer(client_connected_cb, /, host=None, port=None, *, \
                        limit=2**16, family=socket.AF_UNSPEC, \
                        flags=socket.AI_PASSIVE, sock=None, backlog=100, \
                        ssl=None, reuse_address=None, reuse_port=None, \
                        ssl_handshake_timeout=None, shutdown_timeout=60)

   The *client_connected_cb* callback is called whenever a new client
   connection is established.  It receives a :class:`Stream` object of the
   mode :attr:`StreamMode.READWRITE`.

   *client_connected_cb* can be a plain callable or a
   :ref:`coroutine function <coroutine>`; if it is a coroutine function,
   it will be automatically scheduled as a :class:`Task`.

   *limit* determines the buffer size limit used by the
   returned :class:`Stream` instance.  By default the *limit*
   is set to 64 KiB.

   The rest of the arguments are passed directly to
   :meth:`loop.create_server`.

   .. coroutinemethod:: start_serving()

      Binds to the given host and port to start the server.

   .. coroutinemethod:: serve_forever()

      Start accepting connections until the coroutine is cancelled.
      Cancellation of ``serve_forever`` task causes the server
      to be closed.

      This method can be called if the server is already accepting
      connections.  Only one ``serve_forever`` task can exist per
      one *Server* object.

   .. method:: is_serving()

      Returns ``True`` if the server is bound and currently serving.

   .. method:: bind()

      Bind the server to the given *host* and *port*. This method is
      automatically called during ``__aenter__`` when :class:`StreamServer` is
      used as an async context manager.

   .. method:: is_bound()

      Return ``True`` if the server is bound.

   .. coroutinemethod:: abort()

      Closes the connection and cancels all pending tasks.

   .. coroutinemethod:: close()

      Closes the connection. This method is automatically called during
      ``__aexit__`` when :class:`StreamServer` is used as an async context
      manager.

   .. attribute:: sockets

      Returns a tuple of socket objects the server is bound to.

   .. versionadded:: 3.8


UnixStreamServer
================

.. class:: UnixStreamServer(client_connected_cb, /, path=None, *, \
                            limit=2**16, sock=None, backlog=100, \
                            ssl=None, ssl_handshake_timeout=None, shutdown_timeout=60)

   The *client_connected_cb* callback is called whenever a new client
   connection is established.  It receives a :class:`Stream` object of the
   mode :attr:`StreamMode.READWRITE`.

   *client_connected_cb* can be a plain callable or a
   :ref:`coroutine function <coroutine>`; if it is a coroutine function,
   it will be automatically scheduled as a :class:`Task`.

   *limit* determines the buffer size limit used by the
   returned :class:`Stream` instance.  By default the *limit*
   is set to 64 KiB.

   The rest of the arguments are passed directly to
   :meth:`loop.create_unix_server`.

   .. coroutinemethod:: start_serving()

      Binds to the given host and port to start the server.

   .. method:: is_serving()

      Returns ``True`` if the server is bound and currently serving.

   .. method:: bind()

      Bind the server to the given *host* and *port*. This method is
      automatically called during ``__aenter__`` when :class:`UnixStreamServer` is
      used as an async context manager.

   .. method:: is_bound()

      Return ``True`` if the server is bound.

   .. coroutinemethod:: abort()

      Closes the connection and cancels all pending tasks.

   .. coroutinemethod:: close()

      Closes the connection. This method is automatically called during
      ``__aexit__`` when :class:`UnixStreamServer` is used as an async context
      manager.

   .. attribute:: sockets

      Returns a tuple of socket objects the server is bound to.

   .. availability:: Unix.

   .. versionadded:: 3.8

Stream
======

.. class:: Stream

   Represents a Stream object that provides APIs to read and write data
   to the IO stream . It includes the API provided by :class:`StreamReader`
   and :class:`StreamWriter`.

   Do not instantiate *Stream* objects directly; use API like :func:`connect`
   and :class:`StreamServer` instead.

   .. versionadded:: 3.8


StreamMode
==========

.. class:: StreamMode

   A subclass of :class:`enum.Flag` that defines a set of values that can be
   used to determine the ``mode`` of :class:`Stream` objects.

   .. data:: READ

   The stream object is readable and provides the API of :class:`StreamReader`.

   .. data:: WRITE

   The stream object is writeable and provides the API of :class:`StreamWriter`.

   .. data:: READWRITE

   The stream object is readable and writeable and provides the API of both
   :class:`StreamReader` and :class:`StreamWriter`.

  .. versionadded:: 3.8


StreamReader
============

.. class:: StreamReader

   Represents a reader object that provides APIs to read data
   from the IO stream.

   It is not recommended to instantiate *StreamReader* objects
   directly; use :func:`open_connection` and :func:`start_server`
   instead.

   .. coroutinemethod:: read(n=-1)

      Read up to *n* bytes.  If *n* is not provided, or set to ``-1``,
      read until EOF and return all read bytes.

      If EOF was received and the internal buffer is empty,
      return an empty ``bytes`` object.

   .. coroutinemethod:: readline()

      Read one line, where "line" is a sequence of bytes
      ending with ``\n``.

      If EOF is received and ``\n`` was not found, the method
      returns partially read data.

      If EOF is received and the internal buffer is empty,
      return an empty ``bytes`` object.

   .. coroutinemethod:: readexactly(n)

      Read exactly *n* bytes.

      Raise an :exc:`IncompleteReadError` if EOF is reached before *n*
      can be read.  Use the :attr:`IncompleteReadError.partial`
      attribute to get the partially read data.

   .. coroutinemethod:: readuntil(separator=b'\\n')

      Read data from the stream until *separator* is found.

      On success, the data and separator will be removed from the
      internal buffer (consumed). Returned data will include the
      separator at the end.

      If the amount of data read exceeds the configured stream limit, a
      :exc:`LimitOverrunError` exception is raised, and the data
      is left in the internal buffer and can be read again.

      If EOF is reached before the complete separator is found,
      an :exc:`IncompleteReadError` exception is raised, and the internal
      buffer is reset.  The :attr:`IncompleteReadError.partial` attribute
      may contain a portion of the separator.

      .. versionadded:: 3.5.2

   .. method:: at_eof()

      Return ``True`` if the buffer is empty and :meth:`feed_eof`
      was called.


StreamWriter
============

.. class:: StreamWriter

   Represents a writer object that provides APIs to write data
   to the IO stream.

   It is not recommended to instantiate *StreamWriter* objects
   directly; use :func:`open_connection` and :func:`start_server`
   instead.

   .. method:: write(data)

      The method attempts to write the *data* to the underlying socket immediately.
      If that fails, the data is queued in an internal write buffer until it can be
      sent.

      Starting with Python 3.8, it is possible to directly await on the `write()`
      method::

         await stream.write(data)

      The ``await`` pauses the current coroutine until the data is written to the
      socket.

      Below is an equivalent code that works with Python <= 3.7::

         stream.write(data)
         await stream.drain()

      .. versionchanged:: 3.8
         Support ``await stream.write(...)`` syntax.

   .. method:: writelines(data)

      The method writes a list (or any iterable) of bytes to the underlying socket
      immediately.
      If that fails, the data is queued in an internal write buffer until it can be
      sent.

      Starting with Python 3.8, it is possible to directly await on the `write()`
      method::

         await stream.writelines(lines)

      The ``await`` pauses the current coroutine until the data is written to the
      socket.

      Below is an equivalent code that works with Python <= 3.7::

         stream.writelines(lines)
         await stream.drain()

      .. versionchanged:: 3.8
         Support ``await stream.writelines()`` syntax.

   .. method:: close()

      The method closes the stream and the underlying socket.

      Starting with Python 3.8, it is possible to directly await on the `close()`
      method::

         await stream.close()

      The ``await`` pauses the current coroutine until the stream and the underlying
      socket are closed (and SSL shutdown is performed for a secure connection).

      Below is an equivalent code that works with Python <= 3.7::

         stream.close()
         await stream.wait_closed()

      .. versionchanged:: 3.8
         Support ``await stream.close()`` syntax.

   .. method:: can_write_eof()

      Return *True* if the underlying transport supports
      the :meth:`write_eof` method, *False* otherwise.

   .. method:: write_eof()

      Close the write end of the stream after the buffered write
      data is flushed.

   .. attribute:: transport

      Return the underlying asyncio transport.

   .. method:: get_extra_info(name, default=None)

      Access optional transport information; see
      :meth:`BaseTransport.get_extra_info` for details.

   .. coroutinemethod:: drain()

      Wait until it is appropriate to resume writing to the stream.
      Example::

          writer.write(data)
          await writer.drain()

      This is a flow control method that interacts with the underlying
      IO write buffer.  When the size of the buffer reaches
      the high watermark, *drain()* blocks until the size of the
      buffer is drained down to the low watermark and writing can
      be resumed.  When there is nothing to wait for, the :meth:`drain`
      returns immediately.

   .. method:: is_closing()

      Return ``True`` if the stream is closed or in the process of
      being closed.

      .. versionadded:: 3.7

   .. coroutinemethod:: wait_closed()

      Wait until the stream is closed.

      Should be called after :meth:`close` to wait until the underlying
      connection is closed.

      .. versionadded:: 3.7


Examples
========

.. _asyncio-tcp-echo-client-streams:

TCP echo client using streams
-----------------------------

TCP echo client using the :func:`asyncio.connect` function::

    import asyncio

    async def tcp_echo_client(message):
        async with asyncio.connect('127.0.0.1', 8888) as stream:
            print(f'Send: {message!r}')
            await stream.write(message.encode())

            data = await stream.read(100)
            print(f'Received: {data.decode()!r}')

    asyncio.run(tcp_echo_client('Hello World!'))


.. seealso::

   The :ref:`TCP echo client protocol <asyncio_example_tcp_echo_client_protocol>`
   example uses the low-level :meth:`loop.create_connection` method.


.. _asyncio-tcp-echo-server-streams:

TCP echo server using streams
-----------------------------

TCP echo server using the :class:`asyncio.StreamServer` class::

    import asyncio

    async def handle_echo(stream):
        data = await stream.read(100)
        message = data.decode()
        addr = stream.get_extra_info('peername')

        print(f"Received {message!r} from {addr!r}")

        print(f"Send: {message!r}")
        await stream.write(data)

        print("Close the connection")
        await stream.close()

    async def main():
        async with asyncio.StreamServer(
                handle_echo, '127.0.0.1', 8888) as server:
            addr = server.sockets[0].getsockname()
            print(f'Serving on {addr}')
            await server.serve_forever()

    asyncio.run(main())


.. seealso::

   The :ref:`TCP echo server protocol <asyncio_example_tcp_echo_server_protocol>`
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
            stream = await asyncio.connect(url.hostname, 443, ssl=True)
        else:
            stream = await asyncio.connect(url.hostname, 80)

        query = (
            f"HEAD {url.path or '/'} HTTP/1.0\r\n"
            f"Host: {url.hostname}\r\n"
            f"\r\n"
        )

        stream.write(query.encode('latin-1'))
        while (line := await stream.readline()):
            line = line.decode('latin1').rstrip()
            if line:
                print(f'HTTP header> {line}')

        # Ignore the body, close the socket
        await stream.close()

    url = sys.argv[1]
    asyncio.run(print_http_headers(url))


Usage::

    python example.py http://example.com/path/page.html

or with HTTPS::

    python example.py https://example.com/path/page.html


.. _asyncio_example_create_connection-streams:

Register an open socket to wait for data using streams
------------------------------------------------------

Coroutine waiting until a socket receives data using the
:func:`asyncio.connect` function::

    import asyncio
    import socket

    async def wait_for_data():
        # Get a reference to the current event loop because
        # we want to access low-level APIs.
        loop = asyncio.get_running_loop()

        # Create a pair of connected sockets.
        rsock, wsock = socket.socketpair()

        # Register the open socket to wait for data.
        async with asyncio.connect(sock=rsock) as stream:
            # Simulate the reception of data from the network
            loop.call_soon(wsock.send, 'abc'.encode())

            # Wait for data
            data = await stream.read(100)

            # Got data, we are done: close the socket
            print("Received:", data.decode())

        # Close the second socket
        wsock.close()

    asyncio.run(wait_for_data())

.. seealso::

   The :ref:`register an open socket to wait for data using a protocol
   <asyncio_example_create_connection>` example uses a low-level protocol and
   the :meth:`loop.create_connection` method.

   The :ref:`watch a file descriptor for read events
   <asyncio_example_watch_fd>` example uses the low-level
   :meth:`loop.add_reader` method to watch a file descriptor.
