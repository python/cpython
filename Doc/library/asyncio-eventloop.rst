.. module:: asyncio

.. _event-loop:

Event loops
===========

The event loop is the central execution device provided by :mod:`asyncio`.
It provides multiple facilities, amongst which:

* Registering, executing and cancelling delayed calls (timeouts)

* Creating client and server :ref:`transports <transport>` for various
  kinds of communication

* Launching subprocesses and the associated :ref:`transports <transport>`
  for communication with an external program

* Delegating costly function calls to a pool of threads

Event loop functions
--------------------

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
-----------------

.. function:: get_event_loop_policy()

   XXX

.. function:: set_event_loop_policy(policy)

   XXX


Run an event loop
-----------------

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
-----

.. method:: BaseEventLoop.call_soon(callback, \*args)

   Arrange for a callback to be called as soon as possible.

   This operates as a FIFO queue, callbacks are called in the order in
   which they are registered.  Each callback will be called exactly once.

   Any positional arguments after the callback will be passed to the
   callback when it is called.

.. method:: BaseEventLoop.call_soon_threadsafe(callback, \*args)

   Like :meth:`call_soon`, but thread safe.


Delayed calls
-------------

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


Creating listening connections
------------------------------

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



Resolve name
------------

.. method:: BaseEventLoop.getaddrinfo(host, port, \*, family=0, type=0, proto=0, flags=0)

   XXX

.. method:: BaseEventLoop.getnameinfo(sockaddr, flags=0)

   XXX


Running subprocesses
--------------------

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


Executor
--------

Call a function in an :class:`~concurrent.futures.Executor` (pool of threads or
pool of processes). By default, an event loop uses a thread pool executor
(:class:`~concurrent.futures.ThreadPoolExecutor`).

.. method:: BaseEventLoop.run_in_executor(executor, callback, \*args)

   Arrange for a callback to be called in the specified executor.

   *executor* is a :class:`~concurrent.futures.Executor` instance,
   the default executor is used if *executor* is ``None``.

.. method:: BaseEventLoop.set_default_executor(executor)

   Set the default executor used by :meth:`run_in_executor`.


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


Hello World (coroutine)
^^^^^^^^^^^^^^^^^^^^^^^

Print ``Hello World`` every two seconds, using a coroutine::

    import asyncio

    @asyncio.coroutine
    def greet_every_two_seconds():
        while True:
            print('Hello World')
            yield from asyncio.sleep(2)

    loop = asyncio.get_event_loop()
    loop.run_until_complete(greet_every_two_seconds())

