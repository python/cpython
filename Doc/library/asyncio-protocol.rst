.. currentmodule:: asyncio

+++++++++++++++++++++++++++++++++++++++++
Transports  and protocols (low-level API)
+++++++++++++++++++++++++++++++++++++++++

.. _asyncio-transport:

Transports
==========

Transports are classed provided by :mod:`asyncio` in order to abstract
various kinds of communication channels.  You generally won't instantiate
a transport yourself; instead, you will call a :class:`BaseEventLoop` method
which will create the transport and try to initiate the underlying
communication channel, calling you back when it succeeds.

Once the communication channel is established, a transport is always
paired with a :ref:`protocol <asyncio-protocol>` instance.  The protocol can
then call the transport's methods for various purposes.

:mod:`asyncio` currently implements transports for TCP, UDP, SSL, and
subprocess pipes.  The methods available on a transport depend on
the transport's kind.


BaseTransport
-------------

.. class:: BaseTransport

   Base class for transports.

   .. method:: close(self)

      Close the transport.  If the transport has a buffer for outgoing
      data, buffered data will be flushed asynchronously.  No more data
      will be received.  After all buffered data is flushed, the
      protocol's :meth:`connection_lost` method will be called with
      :const:`None` as its argument.


   .. method:: get_extra_info(name, default=None)

      Return optional transport information.  *name* is a string representing
      the piece of transport-specific information to get, *default* is the
      value to return if the information doesn't exist.

      This method allows transport implementations to easily expose
      channel-specific information.

      * socket:

        - ``'peername'``: the remote address to which the socket is connected,
          result of :meth:`socket.socket.getpeername` (``None`` on error)
        - ``'socket'``: :class:`socket.socket` instance
        - ``'sockname'``: the socket's own address,
          result of :meth:`socket.socket.getsockname`

      * SSL socket:

        - ``'compression'``: the compression algorithm being used as a string,
          or ``None`` if the connection isn't compressed; result of
          :meth:`ssl.SSLSocket.compression`
        - ``'cipher'``: a three-value tuple containing the name of the cipher
          being used, the version of the SSL protocol that defines its use, and
          the number of secret bits being used; result of
          :meth:`ssl.SSLSocket.cipher`
        - ``'peercert'``: peer certificate; result of
          :meth:`ssl.SSLSocket.getpeercert`
        - ``'sslcontext'``: :class:`ssl.SSLContext` instance

      * pipe:

        - ``'pipe'``: pipe object

      * subprocess:

        - ``'subprocess'``: :class:`subprocess.Popen` instance


ReadTransport
-------------

.. class:: ReadTransport

   Interface for read-only transports.

   .. method:: pause_reading()

      Pause the receiving end of the transport.  No data will be passed to
      the protocol's :meth:`data_received` method until meth:`resume_reading`
      is called.

   .. method:: resume_reading()

      Resume the receiving end.  The protocol's :meth:`data_received` method
      will be called once again if some data is available for reading.


WriteTransport
--------------

.. class:: WriteTransport

   Interface for write-only transports.

   .. method:: abort()

      Close the transport immediately, without waiting for pending operations
      to complete.  Buffered data will be lost.  No more data will be received.
      The protocol's :meth:`connection_lost` method will eventually be
      called with :const:`None` as its argument.

   .. method:: can_write_eof()

      Return :const:`True` if the transport supports :meth:`write_eof`,
      :const:`False` if not.

   .. method:: get_write_buffer_size()

      Return the current size of the output buffer used by the transport.

   .. method:: set_write_buffer_limits(high=None, low=None)

      Set the *high*- and *low*-water limits for write flow control.

      These two values control when call the protocol's
      :meth:`pause_writing` and :meth:`resume_writing` methods are called.
      If specified, the low-water limit must be less than or equal to the
      high-water limit.  Neither *high* nor *low* can be negative.

      The defaults are implementation-specific.  If only the
      high-water limit is given, the low-water limit defaults to a
      implementation-specific value less than or equal to the
      high-water limit.  Setting *high* to zero forces *low* to zero as
      well, and causes :meth:`pause_writing` to be called whenever the
      buffer becomes non-empty.  Setting *low* to zero causes
      :meth:`resume_writing` to be called only once the buffer is empty.
      Use of zero for either limit is generally sub-optimal as it
      reduces opportunities for doing I/O and computation
      concurrently.

   .. method:: write(data)

      Write some *data* bytes to the transport.

      This method does not block; it buffers the data and arranges for it
      to be sent out asynchronously.

   .. method:: writelines(list_of_data)

      Write a list (or any iterable) of data bytes to the transport.
      This is functionally equivalent to calling :meth:`write` on each
      element yielded by the iterable, but may be implemented more efficiently.

   .. method:: write_eof()

      Close the write end of the transport after flushing buffered data.
      Data may still be received.

      This method can raise :exc:`NotImplementedError` if the transport
      (e.g. SSL) doesn't support half-closes.


DatagramTransport
-----------------

.. method:: DatagramTransport.sendto(data, addr=None)

   Send the *data* bytes to the remote peer given by *addr* (a
   transport-dependent target address).  If *addr* is :const:`None`, the
   data is sent to the target address given on transport creation.

   This method does not block; it buffers the data and arranges for it
   to be sent out asynchronously.

.. method:: DatagramTransport.abort()

   Close the transport immediately, without waiting for pending operations
   to complete.  Buffered data will be lost.  No more data will be received.
   The protocol's :meth:`connection_lost` method will eventually be
   called with :const:`None` as its argument.


BaseSubprocessTransport
-----------------------

.. class:: BaseSubprocessTransport

   .. method:: get_pid()

      Return the subprocess process id as an integer.

   .. method:: get_pipe_transport(fd)

      Return the transport for the communication pipe corresponding to the
      integer file descriptor *fd*.  The return value can be a readable or
      writable streaming transport, depending on the *fd*.  If *fd* doesn't
      correspond to a pipe belonging to this transport, :const:`None` is
      returned.

   .. method:: get_returncode()

      Return the subprocess returncode as an integer or :const:`None`
      if it hasn't returned, similarly to the
      :attr:`subprocess.Popen.returncode` attribute.

   .. method:: kill(self)

      Kill the subprocess, as in :meth:`subprocess.Popen.kill`

      On POSIX systems, the function sends SIGKILL to the subprocess.
      On Windows, this method is an alias for :meth:`terminate`.

   .. method:: send_signal(signal)

      Send the *signal* number to the subprocess, as in
      :meth:`subprocess.Popen.send_signal`.

   .. method:: terminate()

      Ask the subprocess to stop, as in :meth:`subprocess.Popen.terminate`.
      This method is an alias for the :meth:`close` method.

      On POSIX systems, this method sends SIGTERM to the subprocess.
      On Windows, the Windows API function TerminateProcess() is called to
      stop the subprocess.


.. _asyncio-protocol:

Protocols
=========

:mod:`asyncio` provides base classes that you can subclass to implement
your network protocols.  Those classes are used in conjunction with
:ref:`transports <asyncio-transport>` (see below): the protocol parses incoming
data and asks for the writing of outgoing data, while the transport is
responsible for the actual I/O and buffering.

When subclassing a protocol class, it is recommended you override certain
methods.  Those methods are callbacks: they will be called by the transport
on certain events (for example when some data is received); you shouldn't
call them yourself, unless you are implementing a transport.

.. note::
   All callbacks have default implementations, which are empty.  Therefore,
   you only need to implement the callbacks for the events in which you
   are interested.


Protocol classes
----------------

.. class:: Protocol

   The base class for implementing streaming protocols (for use with
   e.g. TCP and SSL transports).

.. class:: DatagramProtocol

   The base class for implementing datagram protocols (for use with
   e.g. UDP transports).

.. class:: SubprocessProtocol

   The base class for implementing protocols communicating with child
   processes (through a set of unidirectional pipes).


Connection callbacks
--------------------

These callbacks may be called on :class:`Protocol` and
:class:`SubprocessProtocol` instances:

.. method:: BaseProtocol.connection_made(transport)

   Called when a connection is made.

   The *transport* argument is the transport representing the
   connection.  You are responsible for storing it somewhere
   (e.g. as an attribute) if you need to.

.. method:: BaseProtocol.connection_lost(exc)

   Called when the connection is lost or closed.

   The argument is either an exception object or :const:`None`.
   The latter means a regular EOF is received, or the connection was
   aborted or closed by this side of the connection.

:meth:`connection_made` and :meth:`connection_lost` are called exactly once
per successful connection.  All other callbacks will be called between those
two methods, which allows for easier resource management in your protocol
implementation.

The following callbacks may be called only on :class:`SubprocessProtocol`
instances:

.. method:: SubprocessProtocol.pipe_data_received(fd, data)

   Called when the child process writes data into its stdout or stderr pipe.
   *fd* is the integer file descriptor of the pipe.  *data* is a non-empty
   bytes object containing the data.

.. method:: SubprocessProtocol.pipe_connection_lost(fd, exc)

   Called when one of the pipes communicating with the child process
   is closed.  *fd* is the integer file descriptor that was closed.

.. method:: SubprocessProtocol.process_exited()

   Called when the child process has exited.


Streaming protocols
-------------------

The following callbacks are called on :class:`Protocol` instances:

.. method:: Protocol.data_received(data)

   Called when some data is received.  *data* is a non-empty bytes object
   containing the incoming data.

   .. note::
      Whether the data is buffered, chunked or reassembled depends on
      the transport.  In general, you shouldn't rely on specific semantics
      and instead make your parsing generic and flexible enough.  However,
      data is always received in the correct order.

.. method:: Protocol.eof_received()

   Calls when the other end signals it won't send any more data
   (for example by calling :meth:`write_eof`, if the other end also uses
   asyncio).

   This method may return a false value (including None), in which case
   the transport will close itself.  Conversely, if this method returns a
   true value, closing the transport is up to the protocol.  Since the
   default implementation returns None, it implicitly closes the connection.

   .. note::
      Some transports such as SSL don't support half-closed connections,
      in which case returning true from this method will not prevent closing
      the connection.

:meth:`data_received` can be called an arbitrary number of times during
a connection.  However, :meth:`eof_received` is called at most once
and, if called, :meth:`data_received` won't be called after it.

Datagram protocols
------------------

The following callbacks are called on :class:`DatagramProtocol` instances.

.. method:: DatagramProtocol.datagram_received(data, addr)

   Called when a datagram is received.  *data* is a bytes object containing
   the incoming data.  *addr* is the address of the peer sending the data;
   the exact format depends on the transport.

.. method:: DatagramProtocol.error_received(exc)

   Called when a previous send or receive operation raises an
   :class:`OSError`.  *exc* is the :class:`OSError` instance.

   This method is called in rare conditions, when the transport (e.g. UDP)
   detects that a datagram couldn't be delivered to its recipient.
   In many conditions though, undeliverable datagrams will be silently
   dropped.


Flow control callbacks
----------------------

These callbacks may be called on :class:`Protocol`,
:class:`DatagramProtocol` and :class:`SubprocessProtocol` instances:

.. method:: BaseProtocol.pause_writing()

   Called when the transport's buffer goes over the high-water mark.

.. method:: BaseProtocol.resume_writing()

   Called when the transport's buffer drains below the low-water mark.


:meth:`pause_writing` and :meth:`resume_writing` calls are paired --
:meth:`pause_writing` is called once when the buffer goes strictly over
the high-water mark (even if subsequent writes increases the buffer size
even more), and eventually :meth:`resume_writing` is called once when the
buffer size reaches the low-water mark.

.. note::
   If the buffer size equals the high-water mark,
   :meth:`pause_writing` is not called -- it must go strictly over.
   Conversely, :meth:`resume_writing` is called when the buffer size is
   equal or lower than the low-water mark.  These end conditions
   are important to ensure that things go as expected when either
   mark is zero.

.. note::
   On BSD systems (OS X, FreeBSD, etc.) flow control is not supported
   for :class:`DatagramProtocol`, because send failures caused by
   writing too many packets cannot be detected easily.  The socket
   always appears 'ready' and excess packets are dropped; an
   :class:`OSError` with errno set to :const:`errno.ENOBUFS` may or
   may not be raised; if it is raised, it will be reported to
   :meth:`DatagramProtocol.error_received` but otherwise ignored.


Coroutines and protocols
------------------------

Coroutines can be scheduled in a protocol method using :func:`async`, but there
is not guarantee on the execution order. Protocols are not aware of coroutines
created in protocol methods and so will not wait for them.

To have a reliable execution order, use :ref:`stream objects <asyncio-streams>` in a
coroutine with ``yield from``. For example, the :meth:`StreamWriter.drain`
coroutine can be used to wait until the write buffer is flushed.


Protocol example: TCP echo server and client
============================================

Echo client
-----------

TCP echo client example, send data and wait until the connection is closed::

    import asyncio

    class EchoClient(asyncio.Protocol):
        message = 'This is the message. It will be echoed.'

        def connection_made(self, transport):
            transport.write(self.message.encode())
            print('data sent: {}'.format(self.message))

        def data_received(self, data):
            print('data received: {}'.format(data.decode()))

        def connection_lost(self, exc):
            print('server closed the connection')
            asyncio.get_event_loop().stop()

    loop = asyncio.get_event_loop()
    coro = loop.create_connection(EchoClient, '127.0.0.1', 8888)
    loop.run_until_complete(coro)
    loop.run_forever()
    loop.close()

The event loop is running twice. The
:meth:`~BaseEventLoop.run_until_complete` method is preferred in this short
example to raise an exception if the server is not listening, instead of
having to write a short coroutine to handle the exception and stop the
running loop. At :meth:`~BaseEventLoop.run_until_complete` exit, the loop is
no more running, so there is no need to stop the loop in case of an error.

Echo server
-----------

TCP echo server example, send back received data and close the connection::

    import asyncio

    class EchoServer(asyncio.Protocol):
        def connection_made(self, transport):
            peername = transport.get_extra_info('peername')
            print('connection from {}'.format(peername))
            self.transport = transport

        def data_received(self, data):
            print('data received: {}'.format(data.decode()))
            self.transport.write(data)

            # close the socket
            self.transport.close()

    loop = asyncio.get_event_loop()
    coro = loop.create_server(EchoServer, '127.0.0.1', 8888)
    server = loop.run_until_complete(coro)
    print('serving on {}'.format(server.sockets[0].getsockname()))

    try:
        loop.run_forever()
    except KeyboardInterrupt:
        print("exit")
    finally:
        server.close()
        loop.close()

:meth:`Transport.close` can be called immediately after
:meth:`WriteTransport.write` even if data are not sent yet on the socket: both
methods are asynchronous. ``yield from`` is not needed because these transport
methods are not coroutines.


