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

The event loop is the central execution device provided by :mod:`asyncio`.
It provides multiple facilities, amongst which:

* Registering, executing and cancelling delayed calls (timeouts)

* Creating client and server :ref:`transports <transport>` for various
  kinds of communication

* Launching subprocesses and the associated :ref:`transports <transport>`
  for communication with an external program

* Delegating costly function calls to a pool of threads

Event loop functions
^^^^^^^^^^^^^^^^^^^^

The easiest way to get an event loop is to call the :func:`get_event_loop`
function.

.. function:: get_event_loop()

   Get the event loop for current context. Returns an event loop object
   implementing :class:`BaseEventLoop` interface, or raises an exception in case no
   event loop has been set for the current context and the current policy does
   not specify to create one. It should never return ``None``.

.. function:: set_event_loop(loop)

   XXX

.. function:: new_event_loop()

   XXX


Event loop policy
^^^^^^^^^^^^^^^^^

.. function:: get_event_loop_policy()

   XXX

.. function:: set_event_loop_policy(policy)

   XXX


Run an event loop
^^^^^^^^^^^^^^^^^

.. method:: BaseEventLoop.run_forever()

   Run until :meth:`stop` is called.

.. method:: BaseEventLoop.run_until_complete(future)

   Run until the :class:`~concurrent.futures.Future` is done.

   If the argument is a coroutine, it is wrapped in a :class:`Task`.

   Return the Future's result, or raise its exception.

.. method:: BaseEventLoop.is_running()

   Returns running status of event loop.

.. method:: stop()

   Stop running the event loop.

   Every callback scheduled before :meth:`stop` is called will run.
   Callback scheduled after :meth:`stop` is called won't.  However, those
   callbacks will run if :meth:`run_forever` is called again later.

.. method:: BaseEventLoop.close()

   Close the event loop. The loop should not be running.

   This clears the queues and shuts down the executor, but does not wait for
   the executor to finish.

   This is idempotent and irreversible. No other methods should be called after
   this one.


Calls
^^^^^

.. method:: BaseEventLoop.call_soon(callback, \*args)

   Arrange for a callback to be called as soon as possible.

   This operates as a FIFO queue, callbacks are called in the order in
   which they are registered.  Each callback will be called exactly once.

   Any positional arguments after the callback will be passed to the
   callback when it is called.

.. method:: BaseEventLoop.call_soon_threadsafe(callback, \*args)

   Like :meth:`call_soon`, but thread safe.


Delayed calls
^^^^^^^^^^^^^

The event loop has its own internal clock for computing timeouts.
Which clock is used depends on the (platform-specific) event loop
implementation; ideally it is a monotonic clock.  This will generally be
a different clock than :func:`time.time`.

.. method:: BaseEventLoop.call_later(delay, callback, *args)

   Arrange for the *callback* to be called after the given *delay*
   seconds (either an int or float).

   A "handle" is returned: an opaque object with a :meth:`cancel` method
   that can be used to cancel the call.

   *callback* will be called exactly once per call to :meth:`call_later`.
   If two callbacks are scheduled for exactly the same time, it is
   undefined which will be called first.

   The optional positional *args* will be passed to the callback when it
   is called. If you want the callback to be called with some named
   arguments, use a closure or :func:`functools.partial`.

.. method:: BaseEventLoop.call_at(when, callback, *args)

   Arrange for the *callback* to be called at the given absolute timestamp
   *when* (an int or float), using the same time reference as :meth:`time`.

   This method's behavior is the same as :meth:`call_later`.

.. method:: BaseEventLoop.time()

   Return the current time, as a :class:`float` value, according to the
   event loop's internal clock.


Executor
^^^^^^^^

Call a function in an :class:`~concurrent.futures.Executor` (pool of threads or
pool of processes). By default, an event loop uses a thread pool executor
(:class:`~concurrent.futures.ThreadPoolExecutor`).

.. method:: BaseEventLoop.run_in_executor(executor, callback, \*args)

   Arrange for a callback to be called in the specified executor.

   *executor* is a :class:`~concurrent.futures.Executor` instance,
   the default executor is used if *executor* is ``None``.

.. method:: BaseEventLoop.set_default_executor(executor)

   Set the default executor used by :meth:`run_in_executor`.


Creating listening connections
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: BaseEventLoop.create_server(protocol_factory, host=None, port=None, \*, family=socket.AF_UNSPEC, flags=socket.AI_PASSIVE, sock=None, backlog=100, ssl=None, reuse_address=None)

   A :ref:`coroutine <coroutine>` which creates a TCP server bound to host and
   port.

   The return value is a :class:`AbstractServer` object which can be used to stop
   the service.

   If *host* is an empty string or None all interfaces are assumed
   and a list of multiple sockets will be returned (most likely
   one for IPv4 and another one for IPv6).

   *family* can be set to either :data:`~socket.AF_INET` or
   :data:`~socket.AF_INET6` to force the socket to use IPv4 or IPv6. If not set
   it will be determined from host (defaults to :data:`~socket.AF_UNSPEC`).

   *flags* is a bitmask for :meth:`getaddrinfo`.

   *sock* can optionally be specified in order to use a preexisting
   socket object.

   *backlog* is the maximum number of queued connections passed to
   :meth:`~socket.socket.listen` (defaults to 100).

   ssl can be set to an :class:`~ssl.SSLContext` to enable SSL over the
   accepted connections.

   *reuse_address* tells the kernel to reuse a local socket in
   TIME_WAIT state, without waiting for its natural timeout to
   expire. If not specified will automatically be set to True on
   UNIX.

   This method returns a :ref:`coroutine <coroutine>`.

.. method:: BaseEventLoop.create_datagram_endpoint(protocol_factory, local_addr=None, remote_addr=None, \*, family=0, proto=0, flags=0)

   Create datagram connection.

   This method returns a :ref:`coroutine <coroutine>`.



Creating connections
^^^^^^^^^^^^^^^^^^^^

.. method:: BaseEventLoop.create_connection(protocol_factory, host=None, port=None, \*, ssl=None, family=0, proto=0, flags=0, sock=None, local_addr=None, server_hostname=None)

   Create a streaming transport connection to a given Internet *host* and
   *port*.  *protocol_factory* must be a callable returning a
   :ref:`protocol <protocol>` instance.

   This method returns a :ref:`coroutine <coroutine>` which will try to
   establish the connection in the background.  When successful, the
   coroutine returns a ``(transport, protocol)`` pair.

   The chronological synopsis of the underlying operation is as follows:

   #. The connection is established, and a :ref:`transport <transport>`
      is created to represent it.

   #. *protocol_factory* is called without arguments and must return a
      :ref:`protocol <protocol>` instance.

   #. The protocol instance is tied to the transport, and its
      :meth:`connection_made` method is called.

   #. The coroutine returns successfully with the ``(transport, protocol)``
      pair.

   The created transport is an implementation-dependent bidirectional stream.

   .. note::
      *protocol_factory* can be any kind of callable, not necessarily
      a class.  For example, if you want to use a pre-created
      protocol instance, you can pass ``lambda: my_protocol``.

   Options allowing to change how the connection is created:

   * *ssl*: if given and not false, a SSL/TLS transport is created
     (by default a plain TCP transport is created).  If *ssl* is
     a :class:`ssl.SSLContext` object, this context is used to create
     the transport; if *ssl* is :const:`True`, a context with some
     unspecified default settings is used.

   * *server_hostname*, is only for use together with *ssl*,
     and sets or overrides the hostname that the target server's certificate
     will be matched against.  By default the value of the *host* argument
     is used.  If *host* is empty, there is no default and you must pass a
     value for *server_hostname*.  If *server_hostname* is an empty
     string, hostname matching is disabled (which is a serious security
     risk, allowing for man-in-the-middle-attacks).

   * *family*, *proto*, *flags* are the optional address family, protocol
     and flags to be passed through to getaddrinfo() for *host* resolution.
     If given, these should all be integers from the corresponding
     :mod:`socket` module constants.

   * *sock*, if given, should be an existing, already connected
     :class:`socket.socket` object to be used by the transport.
     If *sock* is given, none of *host*, *port*, *family*, *proto*, *flags*
     and *local_addr* should be specified.

   * *local_addr*, if given, is a ``(local_host, local_port)`` tuple used
     to bind the socket to locally.  The *local_host* and *local_port*
     are looked up using getaddrinfo(), similarly to *host* and *port*.


Resolve name
^^^^^^^^^^^^

.. method:: BaseEventLoop.getaddrinfo(host, port, \*, family=0, type=0, proto=0, flags=0)

   XXX

.. method:: BaseEventLoop.getnameinfo(sockaddr, flags=0)

   XXX


Running subprocesses
^^^^^^^^^^^^^^^^^^^^

Run subprocesses asynchronously using the :mod:`subprocess` module.

.. method:: BaseEventLoop.subprocess_exec(protocol_factory, \*args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=False, shell=False, bufsize=0, \*\*kwargs)

   XXX

   This method returns a :ref:`coroutine <coroutine>`.

   See the constructor of the :class:`subprocess.Popen` class for parameters.

.. method:: BaseEventLoop.subprocess_shell(protocol_factory, cmd, \*, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=False, shell=True, bufsize=0, \*\*kwargs)

   XXX

   This method returns a :ref:`coroutine <coroutine>`.

   See the constructor of the :class:`subprocess.Popen` class for parameters.

.. method:: BaseEventLoop.connect_read_pipe(protocol_factory, pipe)

   Register read pipe in eventloop.

   *protocol_factory* should instantiate object with :class:`Protocol`
   interface.  pipe is file-like object already switched to nonblocking.
   Return pair (transport, protocol), where transport support
   :class:`ReadTransport` interface.

   This method returns a :ref:`coroutine <coroutine>`.

.. method:: BaseEventLoop.connect_write_pipe(protocol_factory, pipe)

   Register write pipe in eventloop.

   *protocol_factory* should instantiate object with :class:`BaseProtocol`
   interface.  Pipe is file-like object already switched to nonblocking.
   Return pair (transport, protocol), where transport support
   :class:`WriteTransport` interface.

   This method returns a :ref:`coroutine <coroutine>`.


.. _coroutine:

Coroutines
----------

A coroutine is a generator that follows certain conventions.  For
documentation purposes, all coroutines should be decorated with
``@asyncio.coroutine``, but this cannot be strictly enforced.

Coroutines use the ``yield from`` syntax introduced in :pep:`380`,
instead of the original ``yield`` syntax.

The word "coroutine", like the word "generator", is used for two
different (though related) concepts:

- The function that defines a coroutine (a function definition
  decorated with ``asyncio.coroutine``).  If disambiguation is needed
  we will call this a *coroutine function*.

- The object obtained by calling a coroutine function.  This object
  represents a computation or an I/O operation (usually a combination)
  that will complete eventually.  If disambiguation is needed we will
  call it a *coroutine object*.

Things a coroutine can do:

- ``result = yield from future`` -- suspends the coroutine until the
  future is done, then returns the future's result, or raises an
  exception, which will be propagated.  (If the future is cancelled,
  it will raise a ``CancelledError`` exception.)  Note that tasks are
  futures, and everything said about futures also applies to tasks.

- ``result = yield from coroutine`` -- wait for another coroutine to
  produce a result (or raise an exception, which will be propagated).
  The ``coroutine`` expression must be a *call* to another coroutine.

- ``return expression`` -- produce a result to the coroutine that is
  waiting for this one using ``yield from``.

- ``raise exception`` -- raise an exception in the coroutine that is
  waiting for this one using ``yield from``.

Calling a coroutine does not start its code running -- it is just a
generator, and the coroutine object returned by the call is really a
generator object, which doesn't do anything until you iterate over it.
In the case of a coroutine object, there are two basic ways to start
it running: call ``yield from coroutine`` from another coroutine
(assuming the other coroutine is already running!), or convert it to a
:class:`Task`.

Coroutines (and tasks) can only run when the event loop is running.


Task
----

.. class:: Task(coro, \*, loop=None)

   A coroutine wrapped in a :class:`~concurrent.futures.Future`.

   .. classmethod:: all_tasks(loop=None)

      Return a set of all tasks for an event loop.

      By default all tasks for the current event loop are returned.

   .. method:: cancel()

      Cancel the task.

   .. method:: get_stack(self, \*, limit=None)

      Return the list of stack frames for this task's coroutine.

      If the coroutine is active, this returns the stack where it is suspended.
      If the coroutine has completed successfully or was cancelled, this
      returns an empty list.  If the coroutine was terminated by an exception,
      this returns the list of traceback frames.

      The frames are always ordered from oldest to newest.

      The optional limit gives the maximum nummber of frames to return; by
      default all available frames are returned.  Its meaning differs depending
      on whether a stack or a traceback is returned: the newest frames of a
      stack are returned, but the oldest frames of a traceback are returned.
      (This matches the behavior of the traceback module.)

      For reasons beyond our control, only one stack frame is returned for a
      suspended coroutine.

   .. method:: print_stack(\*, limit=None, file=None)

      Print the stack or traceback for this task's coroutine.

      This produces output similar to that of the traceback module, for the
      frames retrieved by get_stack().  The limit argument is passed to
      get_stack().  The file argument is an I/O stream to which the output
      goes; by default it goes to sys.stderr.


Task functions
--------------

.. function:: as_completed(fs, *, loop=None, timeout=None)

   Return an iterator whose values, when waited for, are
   :class:`~concurrent.futures.Future` instances.

   Raises :exc:`TimeoutError` if the timeout occurs before all Futures are done.

   Example::

       for f in as_completed(fs):
           result = yield from f  # The 'yield from' may raise
           # Use result

   .. note::

      The futures ``f`` are not necessarily members of fs.

.. function:: async(coro_or_future, *, loop=None)

   Wrap a :ref:`coroutine <coroutine>` in a future.

   If the argument is a :class:`~concurrent.futures.Future`, it is returned
   directly.

.. function:: gather(*coros_or_futures, loop=None, return_exceptions=False)

   Return a future aggregating results from the given coroutines or futures.

   All futures must share the same event loop.  If all the tasks are done
   successfully, the returned future's result is the list of results (in the
   order of the original sequence, not necessarily the order of results
   arrival).  If *result_exception* is True, exceptions in the tasks are
   treated the same as successful results, and gathered in the result list;
   otherwise, the first raised exception will be immediately propagated to the
   returned future.

   Cancellation: if the outer Future is cancelled, all children (that have not
   completed yet) are also cancelled.  If any child is cancelled, this is
   treated as if it raised :exc:`~concurrent.futures.CancelledError` -- the
   outer Future is *not* cancelled in this case.  (This is to prevent the
   cancellation of one child to cause other children to be cancelled.)

.. function:: tasks.iscoroutinefunction(func)

   Return ``True`` if *func* is a decorated coroutine function.

.. function:: tasks.iscoroutine(obj)

   Return ``True`` if *obj* is a coroutine object.

.. function:: sleep(delay, result=None, \*, loop=None)

   Create a :ref:`coroutine <coroutine>` that completes after a given time
   (in seconds).

.. function:: shield(arg, \*, loop=None)

   Wait for a future, shielding it from cancellation.

   The statement::

       res = yield from shield(something())

   is exactly equivalent to the statement::

       res = yield from something()

   *except* that if the coroutine containing it is cancelled, the task running
   in ``something()`` is not cancelled.  From the point of view of
   ``something()``, the cancellation did not happen.  But its caller is still
   cancelled, so the yield-from expression still raises
   :exc:`~concurrent.futures.CancelledError`.  Note: If ``something()`` is
   cancelled by other means this will still cancel ``shield()``.

   If you want to completely ignore cancellation (not recommended) you can
   combine ``shield()`` with a try/except clause, as follows::

       try:
           res = yield from shield(something())
       except CancelledError:
           res = None

.. function:: wait(fs, \*, loop=None, timeout=None, return_when=ALL_COMPLETED)

   Wait for the Futures and coroutines given by fs to complete. Coroutines will
   be wrapped in Tasks.  Returns two sets of
   :class:`~concurrent.futures.Future`: (done, pending).

   *timeout* can be used to control the maximum number of seconds to wait before
   returning.  *timeout* can be an int or float.  If *timeout* is not specified
   or ``None``, there is no limit to the wait time.

   *return_when* indicates when this function should return.  It must be one of
   the following constants of the :mod`concurrent.futures` module:

   .. tabularcolumns:: |l|L|

   +-----------------------------+----------------------------------------+
   | Constant                    | Description                            |
   +=============================+========================================+
   | :const:`FIRST_COMPLETED`    | The function will return when any      |
   |                             | future finishes or is cancelled.       |
   +-----------------------------+----------------------------------------+
   | :const:`FIRST_EXCEPTION`    | The function will return when any      |
   |                             | future finishes by raising an          |
   |                             | exception.  If no future raises an     |
   |                             | exception then it is equivalent to     |
   |                             | :const:`ALL_COMPLETED`.                |
   +-----------------------------+----------------------------------------+
   | :const:`ALL_COMPLETED`      | The function will return when all      |
   |                             | futures finish or are cancelled.       |
   +-----------------------------+----------------------------------------+

   This function returns a :ref:`coroutine <coroutine>`.

   Usage::

        done, pending = yield from asyncio.wait(fs)

   .. note::

      This does not raise :exc:`TimeoutError`! Futures that aren't done when
      the timeout occurs are returned in the second set.


.. _transport:

Transports
----------

Transports are classed provided by :mod:`asyncio` in order to abstract
various kinds of communication channels.  You generally won't instantiate
a transport yourself; instead, you will call a :class:`BaseEventLoop` method
which will create the transport and try to initiate the underlying
communication channel, calling you back when it succeeds.

Once the communication channel is established, a transport is always
paired with a :ref:`protocol <protocol>` instance.  The protocol can
then call the transport's methods for various purposes.

:mod:`asyncio` currently implements transports for TCP, UDP, SSL, and
subprocess pipes.  The methods available on a transport depend on
the transport's kind.


Methods common to all transports: BaseTransport
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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


Methods of readable streaming transports: ReadTransport
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: ReadTransport

   Interface for read-only transports.

   .. method:: pause_reading()

      Pause the receiving end of the transport.  No data will be passed to
      the protocol's :meth:`data_received` method until meth:`resume_reading`
      is called.

   .. method:: resume_reading()

      Resume the receiving end.  The protocol's :meth:`data_received` method
      will be called once again if some data is available for reading.


Methods of writable streaming transports: WriteTransport
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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


Methods of datagram transports
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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


Methods of subprocess transports
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: BaseSubprocessTransport

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


Stream reader and writer
------------------------

.. class:: StreamWriter(transport, protocol, reader, loop)

   Wraps a Transport.

   This exposes :meth:`write`, :meth:`writelines`, :meth:`can_write_eof()`, :meth:`write_eof`, :meth:`get_extra_info` and
   :meth:`close`.  It adds :meth:`drain` which returns an optional :class:`~concurrent.futures.Future` on which you can
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
      :class:`~concurrent.futures.Future` and the yield-from will block until
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

      This method returns a :ref:`coroutine <coroutine>`.

   .. method:: readline()

      XXX

      This method returns a :ref:`coroutine <coroutine>`.

   .. method:: readexactly(n)

      XXX

      This method returns a :ref:`coroutine <coroutine>`.



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


Data reception callbacks
^^^^^^^^^^^^^^^^^^^^^^^^

Streaming protocols
"""""""""""""""""""

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
""""""""""""""""""

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
^^^^^^^^^^^^^^^^^^^^^^

These callbacks may be called on :class:`Protocol` and
:class:`SubprocessProtocol` instances:

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


Server
------

.. class:: AbstractServer

   Abstract server returned by create_service().

   .. method:: close()

      Stop serving.  This leaves existing connections open.

   .. method:: wait_closed()

      Coroutine to wait until service is closed.


Network functions
-----------------

.. function:: open_connection(host=None, port=None, *, loop=None, limit=_DEFAULT_LIMIT, **kwds)

   A wrapper for create_connection() returning a (reader, writer) pair.

   The reader returned is a StreamReader instance; the writer is a
   :class:`Transport`.

   The arguments are all the usual arguments to
   :meth:`BaseEventLoop.create_connection` except *protocol_factory*; most
   common are positional host and port, with various optional keyword arguments
   following.

   Additional optional keyword arguments are *loop* (to set the event loop
   instance to use) and *limit* (to set the buffer limit passed to the
   StreamReader).

   (If you want to customize the :class:`StreamReader` and/or
   :class:`StreamReaderProtocol` classes, just copy the code -- there's really
   nothing special here except some convenience.)

   This function returns a :ref:`coroutine <coroutine>`.

.. function:: start_server(client_connected_cb, host=None, port=None, *, loop=None, limit=_DEFAULT_LIMIT, **kwds)

   Start a socket server, call back for each client connected.

   The first parameter, *client_connected_cb*, takes two parameters:
   *client_reader*, *client_writer*.  *client_reader* is a
   :class:`StreamReader` object, while *client_writer* is a
   :class:`StreamWriter` object.  This parameter can either be a plain callback
   function or a :ref:`coroutine <coroutine>`; if it is a coroutine, it will be
   automatically converted into a :class:`Task`.

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

   This function returns a :ref:`coroutine <coroutine>`.


.. _sync:

Synchronization primitives
--------------------------

.. class:: Lock(\*, loop=None)

   Primitive lock objects.

   A primitive lock is a synchronization primitive that is not owned by a
   particular coroutine when locked.  A primitive lock is in one of two states,
   'locked' or 'unlocked'.

   It is created in the unlocked state.  It has two basic methods, :meth:`acquire`
   and :meth:`release`.  When the state is unlocked, acquire() changes the state to
   locked and returns immediately.  When the state is locked, acquire() blocks
   until a call to release() in another coroutine changes it to unlocked, then
   the acquire() call resets it to locked and returns.  The release() method
   should only be called in the locked state; it changes the state to unlocked
   and returns immediately.  If an attempt is made to release an unlocked lock,
   a :exc:`RuntimeError` will be raised.

   When more than one coroutine is blocked in acquire() waiting for the state
   to turn to unlocked, only one coroutine proceeds when a release() call
   resets the state to unlocked; first coroutine which is blocked in acquire()
   is being processed.

   :meth:`acquire` is a coroutine and should be called with ``yield from``.

   Locks also support the context manager protocol.  ``(yield from lock)``
   should be used as context manager expression.

   Usage::

       lock = Lock()
       ...
       yield from lock
       try:
           ...
       finally:
           lock.release()

   Context manager usage::

       lock = Lock()
       ...
       with (yield from lock):
            ...

   Lock objects can be tested for locking state::

       if not lock.locked():
          yield from lock
       else:
          # lock is acquired
           ...

   .. method:: locked()

      Return ``True`` if lock is acquired.

   .. method:: acquire()

      Acquire a lock.

      This method blocks until the lock is unlocked, then sets it to locked and
      returns ``True``.

      This method returns a :ref:`coroutine <coroutine>`.

   .. method:: release()

      Release a lock.

      When the lock is locked, reset it to unlocked, and return.  If any other
      coroutines are blocked waiting for the lock to become unlocked, allow
      exactly one of them to proceed.

      When invoked on an unlocked lock, a :exc:`RuntimeError` is raised.

      There is no return value.


.. class:: Event(\*, loop=None)

   An Event implementation, asynchronous equivalent to :class:`threading.Event`.

   Class implementing event objects. An event manages a flag that can be set to
   true with the :meth:`set` method and reset to false with the :meth:`clear`
   method.  The :meth:`wait` method blocks until the flag is true. The flag is
   initially false.

   .. method:: clear()

      Reset the internal flag to false. Subsequently, coroutines calling
      :meth:`wait` will block until :meth:`set` is called to set the internal
      flag to true again.

   .. method:: is_set()

      Return ``True`` if and only if the internal flag is true.

   .. method:: set()

      Set the internal flag to true. All coroutines waiting for it to become
      true are awakened. Coroutine that call :meth:`wait` once the flag is true
      will not block at all.

   .. method:: wait()

      Block until the internal flag is true.

      If the internal flag is true on entry, return ``True`` immediately.
      Otherwise, block until another coroutine calls :meth:`set` to set the
      flag to true, then return ``True``.

      This method returns a :ref:`coroutine <coroutine>`.


.. class:: Condition(\*, loop=None)

   A Condition implementation, asynchronous equivalent to
   :class:`threading.Condition`.

   This class implements condition variable objects. A condition variable
   allows one or more coroutines to wait until they are notified by another
   coroutine.

   A new :class:`Lock` object is created and used as the underlying lock.

   .. method:: notify(n=1)

      By default, wake up one coroutine waiting on this condition, if any.
      If the calling coroutine has not acquired the lock when this method is
      called, a :exc:`RuntimeError` is raised.

      This method wakes up at most *n* of the coroutines waiting for the
      condition variable; it is a no-op if no coroutines are waiting.

      .. note::

         An awakened coroutine does not actually return from its :meth:`wait`
         call until it can reacquire the lock. Since :meth:`notify` does not
         release the lock, its caller should.

   .. method:: notify_all()

      Wake up all threads waiting on this condition. This method acts like
      :meth:`notify`, but wakes up all waiting threads instead of one. If the
      calling thread has not acquired the lock when this method is called, a
      :exc:`RuntimeError` is raised.

   .. method:: wait()

      Wait until notified.

      If the calling coroutine has not acquired the lock when this method is
      called, a :exc:`RuntimeError` is raised.

      This method releases the underlying lock, and then blocks until it is
      awakened by a :meth:`notify` or :meth:`notify_all` call for the same
      condition variable in another coroutine.  Once awakened, it re-acquires
      the lock and returns ``True``.

      This method returns a :ref:`coroutine <coroutine>`.

   .. method:: wait_for(predicate)

      Wait until a predicate becomes true.

      The predicate should be a callable which result will be interpreted as a
      boolean value. The final predicate value is the return value.

      This method returns a :ref:`coroutine <coroutine>`.


.. class:: Semaphore(value=1, \*, loop=None)

   A Semaphore implementation.

   A semaphore manages an internal counter which is decremented by each
   :meth:`acquire` call and incremented by each :meth:`release` call. The
   counter can never go below zero; when :meth:`acquire` finds that it is zero,
   it blocks, waiting until some other thread calls :meth:`release`.

   Semaphores also support the context manager protocol.

   The optional argument gives the initial value for the internal counter; it
   defaults to ``1``. If the value given is less than ``0``, :exc:`ValueError`
   is raised.

   .. method:: acquire()

      Acquire a semaphore.

      If the internal counter is larger than zero on entry, decrement it by one
      and return ``True`` immediately.  If it is zero on entry, block, waiting
      until some other coroutine has called :meth:`release` to make it larger
      than ``0``, and then return ``True``.

      This method returns a :ref:`coroutine <coroutine>`.

   .. method:: locked()

      Returns ``True`` if semaphore can not be acquired immediately.

   .. method:: release()

      Release a semaphore, incrementing the internal counter by one. When it
      was zero on entry and another coroutine is waiting for it to become
      larger than zero again, wake up that coroutine.


.. class:: BoundedSemaphore(value=1, \*, loop=None)

    A bounded semaphore implementation. Inherit from :class:`Semaphore`.

    This raises :exc:`ValueError` in :meth:`~Semaphore.release` if it would
    increase the value above the initial value.


.. class:: Queue(maxsize=0, \*, loop=None)

   A queue, useful for coordinating producer and consumer coroutines.

   If *maxsize* is less than or equal to zero, the queue size is infinite. If
   it is an integer greater than ``0``, then ``yield from put()`` will block
   when the queue reaches *maxsize*, until an item is removed by :meth:`get`.

   Unlike the standard library :mod:`queue`, you can reliably know this Queue's
   size with :meth:`qsize`, since your single-threaded Tulip application won't
   be interrupted between calling :meth:`qsize` and doing an operation on the
   Queue.

   .. method:: empty()

      Return ``True`` if the queue is empty, ``False`` otherwise.

   .. method:: full()

      Return ``True`` if there are maxsize items in the queue.

      .. note::

         If the Queue was initialized with ``maxsize=0`` (the default), then
         :meth:`full()` is never ``True``.

   .. method:: get()

      Remove and return an item from the queue.

      If you yield from :meth:`get()`, wait until a item is available.

      This method returns a :ref:`coroutine <coroutine>`.

   .. method:: get_nowait()

      Remove and return an item from the queue.

      Return an item if one is immediately available, else raise
      :exc:`~queue.Empty`.

   .. method:: put(item)

      Put an item into the queue.

      If you yield from ``put()``, wait until a free slot is available before
      adding item.

      This method returns a :ref:`coroutine <coroutine>`.

   .. method:: put_nowait(item)

      Put an item into the queue without blocking.

      If no free slot is immediately available, raise :exc:`~queue.Full`.

   .. method:: qsize()

      Number of items in the queue.

   .. attribute:: maxsize

      Number of items allowed in the queue.


.. class:: PriorityQueue

   A subclass of :class:`Queue`; retrieves entries in priority order (lowest
   first).

   Entries are typically tuples of the form: (priority number, data).


.. class:: LifoQueue

    A subclass of :class:`Queue` that retrieves most recently added entries
    first.


.. class:: JoinableQueue

   A subclass of :class:`Queue` with :meth:`task_done` and :meth:`join`
   methods.

   .. method:: join()

      Block until all items in the queue have been gotten and processed.

      The count of unfinished tasks goes up whenever an item is added to the
      queue. The count goes down whenever a consumer thread calls
      :meth:`task_done` to indicate that the item was retrieved and all work on
      it is complete.  When the count of unfinished tasks drops to zero,
      :meth:`join` unblocks.

      This method returns a :ref:`coroutine <coroutine>`.

   .. method:: task_done()

      Indicate that a formerly enqueued task is complete.

      Used by queue consumers. For each :meth:`~Queue.get` used to fetch a task, a
      subsequent call to :meth:`task_done` tells the queue that the processing
      on the task is complete.

      If a :meth:`join` is currently blocking, it will resume when all items
      have been processed (meaning that a :meth:`task_done` call was received
      for every item that had been :meth:`~Queue.put` into the queue).

      Raises :exc:`ValueError` if called more times than there were items
      placed in the queue.


Examples
--------

Hello World (callback)
^^^^^^^^^^^^^^^^^^^^^^

Print ``Hello World`` every two seconds, using a callback::

    import asyncio

    def print_and_repeat(loop):
        print('Hello World')
        loop.call_later(2, print_and_repeat, loop)

    loop = asyncio.get_event_loop()
    print_and_repeat(loop)
    loop.run_forever()


Hello World (callback)
^^^^^^^^^^^^^^^^^^^^^^

Print ``Hello World`` every two seconds, using a coroutine::

    import asyncio

    @asyncio.coroutine
    def greet_every_two_seconds():
        while True:
            print('Hello World')
            yield from asyncio.sleep(2)

    loop = asyncio.get_event_loop()
    loop.run_until_complete(greet_every_two_seconds())


Echo server
^^^^^^^^^^^

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

