.. currentmodule:: asyncio

.. _asyncio-event-loop:

Event loops
===========

The event loop is the central execution device provided by :mod:`asyncio`.
It provides multiple facilities, amongst which:

* Registering, executing and cancelling delayed calls (timeouts).

* Creating client and server :ref:`transports <asyncio-transport>` for various
  kinds of communication.

* Launching subprocesses and the associated :ref:`transports
  <asyncio-transport>` for communication with an external program.

* Delegating costly function calls to a pool of threads.

Event loop policies and the default policy
------------------------------------------

Event loop management is abstracted with a *policy* pattern, to provide maximal
flexibility for custom platforms and frameworks. Throughout the execution of a
process, a single global policy object manages the event loops available to the
process based on the calling context. A policy is an object implementing the
:class:`AbstractEventLoopPolicy` interface.

For most users of :mod:`asyncio`, policies never have to be dealt with
explicitly, since the default global policy is sufficient.

The default policy defines context as the current thread, and manages an event
loop per thread that interacts with :mod:`asyncio`. The module-level functions
:func:`get_event_loop` and :func:`set_event_loop` provide convenient access to
event loops managed by the default policy.

Event loop functions
--------------------

The following functions are convenient shortcuts to accessing the methods of the
global policy. Note that this provides access to the default policy, unless an
alternative policy was set by calling :func:`set_event_loop_policy` earlier in
the execution of the process.

.. function:: get_event_loop()

   Equivalent to calling ``get_event_loop_policy().get_event_loop()``.

.. function:: set_event_loop(loop)

   Equivalent to calling ``get_event_loop_policy().set_event_loop(loop)``.

.. function:: new_event_loop()

   Equivalent to calling ``get_event_loop_policy().new_event_loop()``.

Event loop policy interface
---------------------------

An event loop policy must implement the following interface:

.. class:: AbstractEventLoopPolicy

   .. method:: get_event_loop()

   Get the event loop for current context. Returns an event loop object
   implementing :class:`BaseEventLoop` interface, or raises an exception in case
   no event loop has been set for the current context and the current policy
   does not specify to create one. It should never return ``None``.

   .. method:: set_event_loop(loop)

   Set the event loop of the current context to *loop*.

   .. method:: new_event_loop()

   Create and return a new event loop object according to this policy's rules.
   If there's need to set this loop as the event loop of the current context,
   :meth:`set_event_loop` must be called explicitly.

Access to the global loop policy
--------------------------------

.. function:: get_event_loop_policy()

   Get the current event loop policy.

.. function:: set_event_loop_policy(policy)

   Set the current event loop policy. If *policy* is ``None``, the default
   policy is restored.

Run an event loop
-----------------

.. method:: BaseEventLoop.run_forever()

   Run until :meth:`stop` is called.

.. method:: BaseEventLoop.run_until_complete(future)

   Run until the :class:`Future` is done.

   If the argument is a :ref:`coroutine <coroutine>`, it is wrapped
   in a :class:`Task`.

   Return the Future's result, or raise its exception.

.. method:: BaseEventLoop.is_running()

   Returns running status of event loop.

.. method:: BaseEventLoop.stop()

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
-----

.. method:: BaseEventLoop.call_soon(callback, \*args)

   Arrange for a callback to be called as soon as possible.

   This operates as a FIFO queue, callbacks are called in the order in
   which they are registered.  Each callback will be called exactly once.

   Any positional arguments after the callback will be passed to the
   callback when it is called.

   An instance of :class:`asyncio.Handle` is returned.

.. method:: BaseEventLoop.call_soon_threadsafe(callback, \*args)

   Like :meth:`call_soon`, but thread safe.


.. _asyncio-delayed-calls:

Delayed calls
-------------

The event loop has its own internal clock for computing timeouts.
Which clock is used depends on the (platform-specific) event loop
implementation; ideally it is a monotonic clock.  This will generally be
a different clock than :func:`time.time`.

.. note::

   Timeouts (relative *delay* or absolute *when*) should not exceed one day.


.. method:: BaseEventLoop.call_later(delay, callback, *args)

   Arrange for the *callback* to be called after the given *delay*
   seconds (either an int or float).

   An instance of :class:`asyncio.Handle` is returned.

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

.. seealso::

   The :func:`asyncio.sleep` function.


Creating connections
--------------------

.. method:: BaseEventLoop.create_connection(protocol_factory, host=None, port=None, \*, ssl=None, family=0, proto=0, flags=0, sock=None, local_addr=None, server_hostname=None)

   Create a streaming transport connection to a given Internet *host* and
   *port*: socket family :py:data:`~socket.AF_INET` or
   :py:data:`~socket.AF_INET6` depending on *host* (or *family* if specified),
   socket type :py:data:`~socket.SOCK_STREAM`.  *protocol_factory* must be a
   callable returning a :ref:`protocol <asyncio-protocol>` instance.

   This method is a :ref:`coroutine <coroutine>` which will try to
   establish the connection in the background.  When successful, the
   coroutine returns a ``(transport, protocol)`` pair.

   The chronological synopsis of the underlying operation is as follows:

   #. The connection is established, and a :ref:`transport <asyncio-transport>`
      is created to represent it.

   #. *protocol_factory* is called without arguments and must return a
      :ref:`protocol <asyncio-protocol>` instance.

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

   .. seealso::

      The :func:`open_connection` function can be used to get a pair of
      (:class:`StreamReader`, :class:`StreamWriter`) instead of a protocol.


.. method:: BaseEventLoop.create_datagram_endpoint(protocol_factory, local_addr=None, remote_addr=None, \*, family=0, proto=0, flags=0)

   Create datagram connection: socket family :py:data:`~socket.AF_INET` or
   :py:data:`~socket.AF_INET6` depending on *host* (or *family* if specified),
   socket type :py:data:`~socket.SOCK_DGRAM`.

   This method is a :ref:`coroutine <coroutine>` which will try to
   establish the connection in the background.  When successful, the
   coroutine returns a ``(transport, protocol)`` pair.

   See the :meth:`BaseEventLoop.create_connection` method for parameters.


.. method:: BaseEventLoop.create_unix_connection(protocol_factory, path, \*, ssl=None, sock=None, server_hostname=None)

   Create UNIX connection: socket family :py:data:`~socket.AF_UNIX`, socket
   type :py:data:`~socket.SOCK_STREAM`. The :py:data:`~socket.AF_UNIX` socket
   family is used to communicate between processes on the same machine
   efficiently.

   This method is a :ref:`coroutine <coroutine>` which will try to
   establish the connection in the background.  When successful, the
   coroutine returns a ``(transport, protocol)`` pair.

   See the :meth:`BaseEventLoop.create_connection` method for parameters.

   Availability: UNIX.


Creating listening connections
------------------------------

.. method:: BaseEventLoop.create_server(protocol_factory, host=None, port=None, \*, family=socket.AF_UNSPEC, flags=socket.AI_PASSIVE, sock=None, backlog=100, ssl=None, reuse_address=None)

   A :ref:`coroutine <coroutine>` method which creates a TCP server bound to
   host and port.

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

   .. seealso::

      The function :func:`start_server` creates a (:class:`StreamReader`,
      :class:`StreamWriter`) pair and calls back a function with this pair.


.. method:: BaseEventLoop.create_unix_server(protocol_factory, path=None, \*, sock=None, backlog=100, ssl=None)

   Similar to :meth:`BaseEventLoop.create_server`, but specific to the
   socket family :py:data:`~socket.AF_UNIX`.

   Availability: UNIX.



Watch file descriptors
----------------------

.. method:: BaseEventLoop.add_reader(fd, callback, \*args)

   Start watching the file descriptor for read availability and then call the
   *callback* with specified arguments.

.. method:: BaseEventLoop.remove_reader(fd)

   Stop watching the file descriptor for read availability.

.. method:: BaseEventLoop.add_writer(fd, callback, \*args)

   Start watching the file descriptor for write availability and then call the
   *callback* with specified arguments.

.. method:: BaseEventLoop.remove_writer(fd)

   Stop watching the file descriptor for write availability.


Low-level socket operations
---------------------------

.. method:: BaseEventLoop.sock_recv(sock, nbytes)

   Receive data from the socket.  The return value is a bytes object
   representing the data received.  The maximum amount of data to be received
   at once is specified by *nbytes*.

   This method is a :ref:`coroutine <coroutine>`.

   .. seealso::

      The :meth:`socket.socket.recv` method.

.. method:: BaseEventLoop.sock_sendall(sock, data)

   Send data to the socket.  The socket must be connected to a remote socket.
   This method continues to send data from *data* until either all data has
   been sent or an error occurs.  ``None`` is returned on success.  On error,
   an exception is raised, and there is no way to determine how much data, if
   any, was successfully processed by the receiving end of the connection.

   This method is a :ref:`coroutine <coroutine>`.

   .. seealso::

      The :meth:`socket.socket.sendall` method.

.. method:: BaseEventLoop.sock_connect(sock, address)

   Connect to a remote socket at *address*.

   The *address* must be already resolved to avoid the trap of hanging the
   entire event loop when the address requires doing a DNS lookup.  For
   example, it must be an IP address, not an hostname, for
   :py:data:`~socket.AF_INET` and :py:data:`~socket.AF_INET6` address families.
   Use :meth:`getaddrinfo` to resolve the hostname asynchronously.

   This method is a :ref:`coroutine <coroutine>`.

   .. seealso::

      The :meth:`BaseEventLoop.create_connection` method, the
      :func:`open_connection` function and the :meth:`socket.socket.connect`
      method.


.. method:: BaseEventLoop.sock_accept(sock)

   Accept a connection. The socket must be bound to an address and listening
   for connections. The return value is a pair ``(conn, address)`` where *conn*
   is a *new* socket object usable to send and receive data on the connection,
   and *address* is the address bound to the socket on the other end of the
   connection.

   This method is a :ref:`coroutine <coroutine>`.

   .. seealso::

      The :meth:`BaseEventLoop.create_server` method, the :func:`start_server`
      function and the :meth:`socket.socket.accept` method.


Resolve host name
-----------------

.. method:: BaseEventLoop.getaddrinfo(host, port, \*, family=0, type=0, proto=0, flags=0)

   This method is a :ref:`coroutine <coroutine>`, similar to
   :meth:`socket.getaddrinfo` function but non-blocking.

.. method:: BaseEventLoop.getnameinfo(sockaddr, flags=0)

   This method is a :ref:`coroutine <coroutine>`, similar to
   :meth:`socket.getnameinfo` function but non-blocking.


Running subprocesses
--------------------

Run subprocesses asynchronously using the :mod:`subprocess` module.

.. note::

   On Windows, the default event loop uses
   :class:`selectors.SelectSelector` which only supports sockets. The
   :class:`ProactorEventLoop` should be used to support subprocesses.

.. note::

   On Mac OS X older than 10.9 (Mavericks), :class:`selectors.KqueueSelector`
   does not support character devices like PTY, whereas it is used by the
   default event loop. The :class:`SelectorEventLoop` can be used with
   :class:`SelectSelector` or :class:`PollSelector` to handle character devices
   on Mac OS X 10.6 (Snow Leopard) and later.

.. method:: BaseEventLoop.subprocess_exec(protocol_factory, \*args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, \*\*kwargs)

   Create a subprocess from one or more string arguments, where the first string
   specifies the program to execute, and the remaining strings specify the
   program's arguments. (Thus, together the string arguments form the
   ``sys.argv`` value of the program, assuming it is a Python script.) This is
   similar to the standard library :class:`subprocess.Popen` class called with
   shell=False and the list of strings passed as the first argument;
   however, where :class:`~subprocess.Popen` takes a single argument which is
   list of strings, :func:`subprocess_exec` takes multiple string arguments.

   Other parameters:

   * *stdin*: Either a file-like object representing the pipe to be connected
     to the subprocess's standard input stream using
     :meth:`~BaseEventLoop.connect_write_pipe`, or the constant
     :const:`subprocess.PIPE` (the default). By default a new pipe will be
     created and connected.

   * *stdout*: Either a file-like object representing the pipe to be connected
     to the subprocess's standard output stream using
     :meth:`~BaseEventLoop.connect_read_pipe`, or the constant
     :const:`subprocess.PIPE` (the default). By default a new pipe will be
     created and connected.

   * *stderr*: Either a file-like object representing the pipe to be connected
     to the subprocess's standard error stream using
     :meth:`~BaseEventLoop.connect_read_pipe`, or one of the constants
     :const:`subprocess.PIPE` (the default) or :const:`subprocess.STDOUT`.
     By default a new pipe will be created and connected. When
     :const:`subprocess.STDOUT` is specified, the subprocess's standard error
     stream will be connected to the same pipe as the standard output stream.

   * All other keyword arguments are passed to :class:`subprocess.Popen`
     without interpretation, except for *bufsize*, *universal_newlines* and
     *shell*, which should not be specified at all.

   Returns a pair of ``(transport, protocol)``, where *transport* is an
   instance of :class:`BaseSubprocessTransport`.

   This method is a :ref:`coroutine <coroutine>`.

   See the constructor of the :class:`subprocess.Popen` class for parameters.

.. method:: BaseEventLoop.subprocess_shell(protocol_factory, cmd, \*, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, \*\*kwargs)

   Create a subprocess from *cmd*, which is a string using the platform's
   "shell" syntax. This is similar to the standard library
   :class:`subprocess.Popen` class called with ``shell=True``.

   See :meth:`~BaseEventLoop.subprocess_exec` for more details about
   the remaining arguments.

   Returns a pair of ``(transport, protocol)``, where *transport* is an
   instance of :class:`BaseSubprocessTransport`.

   This method is a :ref:`coroutine <coroutine>`.

   See the constructor of the :class:`subprocess.Popen` class for parameters.

.. method:: BaseEventLoop.connect_read_pipe(protocol_factory, pipe)

   Register read pipe in eventloop.

   *protocol_factory* should instantiate object with :class:`Protocol`
   interface.  pipe is file-like object already switched to nonblocking.
   Return pair (transport, protocol), where transport support
   :class:`ReadTransport` interface.

   This method is a :ref:`coroutine <coroutine>`.

.. method:: BaseEventLoop.connect_write_pipe(protocol_factory, pipe)

   Register write pipe in eventloop.

   *protocol_factory* should instantiate object with :class:`BaseProtocol`
   interface.  Pipe is file-like object already switched to nonblocking.
   Return pair (transport, protocol), where transport support
   :class:`WriteTransport` interface.

   This method is a :ref:`coroutine <coroutine>`.

.. seealso::

   The :func:`create_subprocess_exec` and :func:`create_subprocess_shell`
   functions.


UNIX signals
------------

Availability: UNIX only.

.. method:: BaseEventLoop.add_signal_handler(signum, callback, \*args)

   Add a handler for a signal.

   Raise :exc:`ValueError` if the signal number is invalid or uncatchable.
   Raise :exc:`RuntimeError` if there is a problem setting up the handler.

.. method:: BaseEventLoop.remove_signal_handler(sig)

   Remove a handler for a signal.

   Return ``True`` if a signal handler was removed, ``False`` if not.

.. seealso::

   The :mod:`signal` module.


Executor
--------

Call a function in an :class:`~concurrent.futures.Executor` (pool of threads or
pool of processes). By default, an event loop uses a thread pool executor
(:class:`~concurrent.futures.ThreadPoolExecutor`).

.. method:: BaseEventLoop.run_in_executor(executor, callback, \*args)

   Arrange for a callback to be called in the specified executor.

   The *executor* argument should be an :class:`~concurrent.futures.Executor`
   instance. The default executor is used if *executor* is ``None``.

   This method is a :ref:`coroutine <coroutine>`.

.. method:: BaseEventLoop.set_default_executor(executor)

   Set the default executor used by :meth:`run_in_executor`.


Error Handling API
------------------

Allows to customize how exceptions are handled in the event loop.

.. method:: BaseEventLoop.set_exception_handler(handler)

   Set *handler* as the new event loop exception handler.

   If *handler* is ``None``, the default exception handler will
   be set.

   If *handler* is a callable object, it should have a
   matching signature to ``(loop, context)``, where ``loop``
   will be a reference to the active event loop, ``context``
   will be a ``dict`` object (see :meth:`call_exception_handler`
   documentation for details about context).

.. method:: BaseEventLoop.default_exception_handler(context)

   Default exception handler.

   This is called when an exception occurs and no exception
   handler is set, and can be called by a custom exception
   handler that wants to defer to the default behavior.

   *context* parameter has the same meaning as in
   :meth:`call_exception_handler`.

.. method:: BaseEventLoop.call_exception_handler(context)

   Call the current event loop exception handler.

   *context* is a ``dict`` object containing the following keys
   (new keys may be introduced later):

   * 'message': Error message;
   * 'exception' (optional): Exception object;
   * 'future' (optional): :class:`asyncio.Future` instance;
   * 'handle' (optional): :class:`asyncio.Handle` instance;
   * 'protocol' (optional): :ref:`Protocol <asyncio-protocol>` instance;
   * 'transport' (optional): :ref:`Transport <asyncio-transport>` instance;
   * 'socket' (optional): :class:`socket.socket` instance.

   .. note::

       Note: this method should not be overloaded in subclassed
       event loops.  For any custom exception handling, use
       :meth:`set_exception_handler()` method.

Debug mode
----------

.. method:: BaseEventLoop.get_debug()

   Get the debug mode (:class:`bool`) of the event loop, ``False`` by default.

.. method:: BaseEventLoop.set_debug(enabled: bool)

   Set the debug mode of the event loop.

.. seealso::

   The :ref:`Develop with asyncio <asyncio-dev>` section.


Server
------

.. class:: AbstractServer

   Abstract server returned by :func:`BaseEventLoop.create_server`.

   .. method:: close()

      Stop serving.  This leaves existing connections open.

   .. method:: wait_closed()

      A :ref:`coroutine <coroutine>` to wait until service is closed.


Handle
------

.. class:: Handle

   A callback wrapper object returned by :func:`BaseEventLoop.call_soon`,
   :func:`BaseEventLoop.call_soon_threadsafe`, :func:`BaseEventLoop.call_later`,
   and :func:`BaseEventLoop.call_at`.

   .. method:: cancel()

   Cancel the call.


.. _asyncio-hello-world-callback:

Example: Hello World (callback)
-------------------------------

Print ``Hello World`` every two seconds, using a callback::

    import asyncio

    def print_and_repeat(loop):
        print('Hello World')
        loop.call_later(2, print_and_repeat, loop)

    loop = asyncio.get_event_loop()
    loop.call_soon(print_and_repeat, loop)
    loop.run_forever()

.. seealso::

   :ref:`Hello World example using a coroutine <asyncio-hello-world-coroutine>`.


Example: Set signal handlers for SIGINT and SIGTERM
---------------------------------------------------

Register handlers for signals :py:data:`SIGINT` and :py:data:`SIGTERM`::

    import asyncio
    import functools
    import os
    import signal

    def ask_exit(signame):
        print("got signal %s: exit" % signame)
        loop.stop()

    loop = asyncio.get_event_loop()
    for signame in ('SIGINT', 'SIGTERM'):
        loop.add_signal_handler(getattr(signal, signame),
                                functools.partial(ask_exit, signame))

    print("Event loop running forever, press CTRL+c to interrupt.")
    print("pid %s: send SIGINT or SIGTERM to exit." % os.getpid())
    loop.run_forever()

