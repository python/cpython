:mod:`asyncio` -- Asynchronous I/O, event loop, coroutines and tasks
====================================================================

.. module:: asyncio
   :synopsis: Asynchronous I/O, event loop, coroutines and tasks.

.. versionadded:: 3.4

**Source code:** :source:`Lib/asyncio/`

--------------

This module provides infrastructure for writing single-threaded concurrent
code using coroutines, multiplexing I/O access over sockets and other
resources, running network clients and servers, and other related primitives.

Here is a more detailed list of the package contents:

* a pluggable :ref:`event loop <event-loop>` with various system-specific
  implementations;

* :ref:`transport <transport>` and :ref:`protocol <protocol>` abstractions
  (similar to those in `Twisted <http://twistedmatrix.com/>`_);

* concrete support for TCP, UDP, SSL, subprocess pipes, delayed calls, and
  others (some may be system-dependent);

* a Future class that mimicks the one in the :mod:`concurrent.futures` module,
  but adapted for use with the event loop;

* coroutines and tasks based on ``yield from`` (:PEP:`380`), to help write
  concurrent code in a sequential fashion;

* cancellation support for Futures and coroutines;

* :ref:`synchronization primitives <sync>` for use between coroutines in
  a single thread, mimicking those in the :mod:`threading` module;

* an interface for passing work off to a threadpool, for times when
  you absolutely, positively have to use a library that makes blocking
  I/O calls.


Disclaimer
----------

Full documentation is not yet ready; we hope to have it written
before Python 3.4 leaves beta.  Until then, the best reference is
:PEP:`3156`.  For a motivational primer on transports and protocols,
see :PEP:`3153`.


.. XXX should the asyncio documentation come in several pages, as for logging?


.. _event-loop:

Event loops
-----------


.. _protocol:

Protocols
---------

:mod:`asyncio` provides base classes that you can subclass to implement
your network protocols.  Those classes are used in conjunction with
:ref:`transports <transport>` (see below): the protocol parses incoming
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
^^^^^^^^^^^^^^^^

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
^^^^^^^^^^^^^^^^^^^^

These callbacks may be called on :class:`Protocol` and
:class:`SubprocessProtocol` instances:

.. method:: connection_made(transport)

   Called when a connection is made.

   The *transport* argument is the transport representing the
   connection.  You are responsible for storing it somewhere
   (e.g. as an attribute) if you need to.

.. method:: connection_lost(exc)

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

.. method:: pipe_data_received(fd, data)

   Called when the child process writes data into its stdout or stderr pipe.
   *fd* is the integer file descriptor of the pipe.  *data* is a non-empty
   bytes object containing the data.

.. method:: pipe_connection_lost(fd, exc)

   Called when one of the pipes communicating with the child process
   is closed.  *fd* is the integer file descriptor that was closed.

.. method:: process_exited()

   Called when the child process has exited.


Data reception callbacks
^^^^^^^^^^^^^^^^^^^^^^^^

Streaming protocols
"""""""""""""""""""

The following callbacks are called on :class:`Protocol` instances:

.. method:: data_received(data)

   Called when some data is received.  *data* is a non-empty bytes object
   containing the incoming data.

   .. note::
      Whether the data is buffered, chunked or reassembled depends on
      the transport.  In general, you shouldn't rely on specific semantics
      and instead make your parsing generic and flexible enough.  However,
      data is always received in the correct order.

.. method:: eof_received()

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
""""""""""""""""""

The following callbacks are called on :class:`DatagramProtocol` instances.

.. method:: datagram_received(data, addr)

   Called when a datagram is received.  *data* is a bytes object containing
   the incoming data.  *addr* is the address of the peer sending the data;
   the exact format depends on the transport.

.. method:: error_received(exc)

   Called when a previous send or receive operation raises an
   :class:`OSError`.  *exc* is the :class:`OSError` instance.

   This method is called in rare conditions, when the transport (e.g. UDP)
   detects that a datagram couldn't be delivered to its recipient.
   In many conditions though, undeliverable datagrams will be silently
   dropped.


Flow control callbacks
^^^^^^^^^^^^^^^^^^^^^^

These callbacks may be called on :class:`Protocol` and
:class:`SubprocessProtocol` instances:

.. method:: pause_writing()

   Called when the transport's buffer goes over the high-water mark.

.. method:: resume_writing()

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


.. _transport:

Transports
----------

Transports are classed provided by :mod:`asyncio` in order to abstract
various kinds of communication channels.  You generally won't instantiate
a transport yourself; instead, you will call a :class:`EventLoop` method
which will create the transport and try to initiate the underlying
communication channel, calling you back when it succeeds.

Once the communication channel is established, a transport is always
paired with a :ref:`protocol <protocol>` instance.  The protocol can
then call the transport's methods for various purposes.

:mod:`asyncio` currently implements transports for TCP, UDP, SSL, and
subprocess pipes.  The methods available on a transport depend on
the transport's kind.

Methods common to all transports
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

Methods of readable streaming transports
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: pause_reading()

   Pause the receiving end of the transport.  No data will be passed to
   the protocol's :meth:`data_received` method until meth:`resume_reading`
   is called.

.. method:: resume_reading()

   Resume the receiving end.  The protocol's :meth:`data_received` method
   will be called once again if some data is available for reading.

Methods of writable streaming transports
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

.. method:: can_write_eof()

   Return :const:`True` if the transport supports :meth:`write_eof`,
   :const:`False` if not.

.. method:: abort()

   Close the transport immediately, without waiting for pending operations
   to complete.  Buffered data will be lost.  No more data will be received.
   The protocol's :meth:`connection_lost` method will eventually be
   called with :const:`None` as its argument.

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

.. method:: get_write_buffer_size()

   Return the current size of the output buffer used by the transport.

Methods of datagram transports
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: sendto(data, addr=None)

   Send the *data* bytes to the remote peer given by *addr* (a
   transport-dependent target address).  If *addr* is :const:`None`, the
   data is sent to the target address given on transport creation.

   This method does not block; it buffers the data and arranges for it
   to be sent out asynchronously.

.. method:: abort()

   Close the transport immediately, without waiting for pending operations
   to complete.  Buffered data will be lost.  No more data will be received.
   The protocol's :meth:`connection_lost` method will eventually be
   called with :const:`None` as its argument.

Methods of subprocess transports
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: get_pid()

   Return the subprocess process id as an integer.

.. method:: get_returncode()

   Return the subprocess returncode as an integer or :const:`None`
   if it hasn't returned, similarly to the
   :attr:`subprocess.Popen.returncode` attribute.

.. method:: get_pipe_transport(fd)

   Return the transport for the communication pipe correspondong to the
   integer file descriptor *fd*.  The return value can be a readable or
   writable streaming transport, depending on the *fd*.  If *fd* doesn't
   correspond to a pipe belonging to this transport, :const:`None` is
   returned.

.. method:: send_signal(signal)

   Send the *signal* number to the subprocess, as in
   :meth:`subprocess.Popen.send_signal`.

.. method:: terminate()

   Ask the subprocess to stop, as in :meth:`subprocess.Popen.terminate`.
   This method is an alias for the :meth:`close` method.

   On POSIX systems, this method sends SIGTERM to the subprocess.
   On Windows, the Windows API function TerminateProcess() is called to
   stop the subprocess.

.. method:: kill(self)

   Kill the subprocess, as in :meth:`subprocess.Popen.kill`

   On POSIX systems, the function sends SIGKILL to the subprocess.
   On Windows, this method is an alias for :meth:`terminate`.


.. _sync:

Synchronization primitives
--------------------------


Examples
--------

A :class:`Protocol` implementing an echo server::

   class EchoServer(asyncio.Protocol):

       TIMEOUT = 5.0

       def timeout(self):
           print('connection timeout, closing.')
           self.transport.close()

       def connection_made(self, transport):
           print('connection made')
           self.transport = transport

           # start 5 seconds timeout timer
           self.h_timeout = asyncio.get_event_loop().call_later(
               self.TIMEOUT, self.timeout)

       def data_received(self, data):
           print('data received: ', data.decode())
           self.transport.write(b'Re: ' + data)

           # restart timeout timer
           self.h_timeout.cancel()
           self.h_timeout = asyncio.get_event_loop().call_later(
               self.TIMEOUT, self.timeout)

       def eof_received(self):
           pass

       def connection_lost(self, exc):
           print('connection lost:', exc)
           self.h_timeout.cancel()

