
:mod:`SocketServer` --- A framework for network servers
=======================================================

.. module:: SocketServer
   :synopsis: A framework for network servers.


The :mod:`SocketServer` module simplifies the task of writing network servers.

There are four basic server classes: :class:`TCPServer` uses the Internet TCP
protocol, which provides for continuous streams of data between the client and
server.  :class:`UDPServer` uses datagrams, which are discrete packets of
information that may arrive out of order or be lost while in transit.  The more
infrequently used :class:`UnixStreamServer` and :class:`UnixDatagramServer`
classes are similar, but use Unix domain sockets; they're not available on
non-Unix platforms.  For more details on network programming, consult a book
such as
W. Richard Steven's UNIX Network Programming or Ralph Davis's Win32 Network
Programming.

These four classes process requests :dfn:`synchronously`; each request must be
completed before the next request can be started.  This isn't suitable if each
request takes a long time to complete, because it requires a lot of computation,
or because it returns a lot of data which the client is slow to process.  The
solution is to create a separate process or thread to handle each request; the
:class:`ForkingMixIn` and :class:`ThreadingMixIn` mix-in classes can be used to
support asynchronous behaviour.

Creating a server requires several steps.  First, you must create a request
handler class by subclassing the :class:`BaseRequestHandler` class and
overriding its :meth:`handle` method; this method will process incoming
requests.  Second, you must instantiate one of the server classes, passing it
the server's address and the request handler class.  Finally, call the
:meth:`handle_request` or :meth:`serve_forever` method of the server object to
process one or many requests.

When inheriting from :class:`ThreadingMixIn` for threaded connection behavior,
you should explicitly declare how you want your threads to behave on an abrupt
shutdown. The :class:`ThreadingMixIn` class defines an attribute
*daemon_threads*, which indicates whether or not the server should wait for
thread termination. You should set the flag explicitly if you would like threads
to behave autonomously; the default is :const:`False`, meaning that Python will
not exit until all threads created by :class:`ThreadingMixIn` have exited.

Server classes have the same external methods and attributes, no matter what
network protocol they use.


Server Creation Notes
---------------------

There are five classes in an inheritance diagram, four of which represent
synchronous servers of four types::

   +------------+
   | BaseServer |
   +------------+
         |
         v
   +-----------+        +------------------+
   | TCPServer |------->| UnixStreamServer |
   +-----------+        +------------------+
         |
         v
   +-----------+        +--------------------+
   | UDPServer |------->| UnixDatagramServer |
   +-----------+        +--------------------+

Note that :class:`UnixDatagramServer` derives from :class:`UDPServer`, not from
:class:`UnixStreamServer` --- the only difference between an IP and a Unix
stream server is the address family, which is simply repeated in both Unix
server classes.

Forking and threading versions of each type of server can be created using the
:class:`ForkingMixIn` and :class:`ThreadingMixIn` mix-in classes.  For instance,
a threading UDP server class is created as follows::

   class ThreadingUDPServer(ThreadingMixIn, UDPServer): pass

The mix-in class must come first, since it overrides a method defined in
:class:`UDPServer`.  Setting the various member variables also changes the
behavior of the underlying server mechanism.

To implement a service, you must derive a class from :class:`BaseRequestHandler`
and redefine its :meth:`handle` method.  You can then run various versions of
the service by combining one of the server classes with your request handler
class.  The request handler class must be different for datagram or stream
services.  This can be hidden by using the handler subclasses
:class:`StreamRequestHandler` or :class:`DatagramRequestHandler`.

Of course, you still have to use your head!  For instance, it makes no sense to
use a forking server if the service contains state in memory that can be
modified by different requests, since the modifications in the child process
would never reach the initial state kept in the parent process and passed to
each child.  In this case, you can use a threading server, but you will probably
have to use locks to protect the integrity of the shared data.

On the other hand, if you are building an HTTP server where all data is stored
externally (for instance, in the file system), a synchronous class will
essentially render the service "deaf" while one request is being handled --
which may be for a very long time if a client is slow to receive all the data it
has requested.  Here a threading or forking server is appropriate.

In some cases, it may be appropriate to process part of a request synchronously,
but to finish processing in a forked child depending on the request data.  This
can be implemented by using a synchronous server and doing an explicit fork in
the request handler class :meth:`handle` method.

Another approach to handling multiple simultaneous requests in an environment
that supports neither threads nor :func:`fork` (or where these are too expensive
or inappropriate for the service) is to maintain an explicit table of partially
finished requests and to use :func:`select` to decide which request to work on
next (or whether to handle a new incoming request).  This is particularly
important for stream services where each client can potentially be connected for
a long time (if threads or subprocesses cannot be used). See :mod:`asyncore` for
another way to manage this.

.. XXX should data and methods be intermingled, or separate?
   how should the distinction between class and instance variables be drawn?


Server Objects
--------------


.. function:: fileno()

   Return an integer file descriptor for the socket on which the server is
   listening.  This function is most commonly passed to :func:`select.select`, to
   allow monitoring multiple servers in the same process.


.. function:: handle_request()

   Process a single request.  This function calls the following methods in
   order: :meth:`get_request`, :meth:`verify_request`, and
   :meth:`process_request`.  If the user-provided :meth:`handle` method of the
   handler class raises an exception, the server's :meth:`handle_error` method
   will be called.  If no request is received within :attr:`self.timeout`
   seconds, :meth:`handle_timeout` will be called and :meth:`handle_request`
   will return.


.. function:: serve_forever(poll_interval=0.5)

   Handle requests until an explicit :meth:`shutdown` request.  Polls for
   shutdown every *poll_interval* seconds.


.. function:: shutdown()

   Tells the :meth:`serve_forever` loop to stop and waits until it does.


.. data:: address_family

   The family of protocols to which the server's socket belongs.
   :const:`socket.AF_INET` and :const:`socket.AF_UNIX` are two possible values.


.. data:: RequestHandlerClass

   The user-provided request handler class; an instance of this class is created
   for each request.


.. data:: server_address

   The address on which the server is listening.  The format of addresses varies
   depending on the protocol family; see the documentation for the socket module
   for details.  For Internet protocols, this is a tuple containing a string giving
   the address, and an integer port number: ``('127.0.0.1', 80)``, for example.


.. data:: socket

   The socket object on which the server will listen for incoming requests.

The server classes support the following class variables:

.. XXX should class variables be covered before instance variables, or vice versa?


.. data:: allow_reuse_address

   Whether the server will allow the reuse of an address. This defaults to
   :const:`False`, and can be set in subclasses to change the policy.


.. data:: request_queue_size

   The size of the request queue.  If it takes a long time to process a single
   request, any requests that arrive while the server is busy are placed into a
   queue, up to :attr:`request_queue_size` requests.  Once the queue is full,
   further requests from clients will get a "Connection denied" error.  The default
   value is usually 5, but this can be overridden by subclasses.


.. data:: socket_type

   The type of socket used by the server; :const:`socket.SOCK_STREAM` and
   :const:`socket.SOCK_DGRAM` are two possible values.

.. data:: timeout

   Timeout duration, measured in seconds, or :const:`None` if no timeout is
   desired.  If :meth:`handle_request` receives no incoming requests within the
   timeout period, the :meth:`handle_timeout` method is called.

There are various server methods that can be overridden by subclasses of base
server classes like :class:`TCPServer`; these methods aren't useful to external
users of the server object.

.. XXX should the default implementations of these be documented, or should
   it be assumed that the user will look at SocketServer.py?


.. function:: finish_request()

   Actually processes the request by instantiating :attr:`RequestHandlerClass` and
   calling its :meth:`handle` method.


.. function:: get_request()

   Must accept a request from the socket, and return a 2-tuple containing the *new*
   socket object to be used to communicate with the client, and the client's
   address.


.. function:: handle_error(request, client_address)

   This function is called if the :attr:`RequestHandlerClass`'s :meth:`handle`
   method raises an exception.  The default action is to print the traceback to
   standard output and continue handling further requests.

.. function:: handle_timeout()

   This function is called when the :attr:`timeout` attribute has been set to a 
   value other than :const:`None` and the timeout period has passed with no 
   requests being received.  The default action for forking servers is
   to collect the status of any child processes that have exited, while
   in threading servers this method does nothing.

.. function:: process_request(request, client_address)

   Calls :meth:`finish_request` to create an instance of the
   :attr:`RequestHandlerClass`.  If desired, this function can create a new process
   or thread to handle the request; the :class:`ForkingMixIn` and
   :class:`ThreadingMixIn` classes do this.

.. Is there any point in documenting the following two functions?
   What would the purpose of overriding them be: initializing server
   instance variables, adding new network families?


.. function:: server_activate()

   Called by the server's constructor to activate the server.  The default behavior
   just :meth:`listen`\ s to the server's socket. May be overridden.


.. function:: server_bind()

   Called by the server's constructor to bind the socket to the desired address.
   May be overridden.


.. function:: verify_request(request, client_address)

   Must return a Boolean value; if the value is :const:`True`, the request will be
   processed, and if it's :const:`False`, the request will be denied. This function
   can be overridden to implement access controls for a server. The default
   implementation always returns :const:`True`.


RequestHandler Objects
----------------------

The request handler class must define a new :meth:`handle` method, and can
override any of the following methods.  A new instance is created for each
request.


.. function:: finish()

   Called after the :meth:`handle` method to perform any clean-up actions required.
   The default implementation does nothing.  If :meth:`setup` or :meth:`handle`
   raise an exception, this function will not be called.


.. function:: handle()

   This function must do all the work required to service a request. The default
   implementation does nothing. Several instance attributes are available to it;
   the request is available as :attr:`self.request`; the client address as
   :attr:`self.client_address`; and the server instance as :attr:`self.server`, in
   case it needs access to per-server information.

   The type of :attr:`self.request` is different for datagram or stream services.
   For stream services, :attr:`self.request` is a socket object; for datagram
   services, :attr:`self.request` is a string. However, this can be hidden by using
   the  request handler subclasses :class:`StreamRequestHandler` or
   :class:`DatagramRequestHandler`, which override the :meth:`setup` and
   :meth:`finish` methods, and provide :attr:`self.rfile` and :attr:`self.wfile`
   attributes. :attr:`self.rfile` and :attr:`self.wfile` can be read or written,
   respectively, to get the request data or return data to the client.


.. function:: setup()

   Called before the :meth:`handle` method to perform any initialization actions
   required.  The default implementation does nothing.

