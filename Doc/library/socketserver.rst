:mod:`SocketServer` --- A framework for network servers
=======================================================

.. module:: SocketServer
   :synopsis: A framework for network servers.

.. note::

   The :mod:`SocketServer` module has been renamed to :mod:`socketserver` in
   Python 3.0.  The :term:`2to3` tool will automatically adapt imports when
   converting your sources to 3.0.

**Source code:** :source:`Lib/SocketServer.py`

--------------

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
:class:`UDPServer`.  Setting the various attributes also change the
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

.. class:: BaseServer

   This is the superclass of all Server objects in the module.  It defines the
   interface, given below, but does not implement most of the methods, which is
   done in subclasses.


.. method:: BaseServer.fileno()

   Return an integer file descriptor for the socket on which the server is
   listening.  This function is most commonly passed to :func:`select.select`, to
   allow monitoring multiple servers in the same process.


.. method:: BaseServer.handle_request()

   Process a single request.  This function calls the following methods in
   order: :meth:`get_request`, :meth:`verify_request`, and
   :meth:`process_request`.  If the user-provided :meth:`handle` method of the
   handler class raises an exception, the server's :meth:`handle_error` method
   will be called.  If no request is received within :attr:`self.timeout`
   seconds, :meth:`handle_timeout` will be called and :meth:`handle_request`
   will return.


.. method:: BaseServer.serve_forever(poll_interval=0.5)

   Handle requests until an explicit :meth:`shutdown` request.
   Poll for shutdown every *poll_interval* seconds. Ignores :attr:`self.timeout`.
   If you need to do periodic tasks, do them in another thread.


.. method:: BaseServer.shutdown()

   Tell the :meth:`serve_forever` loop to stop and wait until it does.

   .. versionadded:: 2.6


.. attribute:: BaseServer.address_family

   The family of protocols to which the server's socket belongs.
   Common examples are :const:`socket.AF_INET` and :const:`socket.AF_UNIX`.


.. attribute:: BaseServer.RequestHandlerClass

   The user-provided request handler class; an instance of this class is created
   for each request.


.. attribute:: BaseServer.server_address

   The address on which the server is listening.  The format of addresses varies
   depending on the protocol family; see the documentation for the socket module
   for details.  For Internet protocols, this is a tuple containing a string giving
   the address, and an integer port number: ``('127.0.0.1', 80)``, for example.


.. attribute:: BaseServer.socket

   The socket object on which the server will listen for incoming requests.


The server classes support the following class variables:

.. XXX should class variables be covered before instance variables, or vice versa?

.. attribute:: BaseServer.allow_reuse_address

   Whether the server will allow the reuse of an address. This defaults to
   :const:`False`, and can be set in subclasses to change the policy.


.. attribute:: BaseServer.request_queue_size

   The size of the request queue.  If it takes a long time to process a single
   request, any requests that arrive while the server is busy are placed into a
   queue, up to :attr:`request_queue_size` requests.  Once the queue is full,
   further requests from clients will get a "Connection denied" error.  The default
   value is usually 5, but this can be overridden by subclasses.


.. attribute:: BaseServer.socket_type

   The type of socket used by the server; :const:`socket.SOCK_STREAM` and
   :const:`socket.SOCK_DGRAM` are two common values.


.. attribute:: BaseServer.timeout

   Timeout duration, measured in seconds, or :const:`None` if no timeout is
   desired.  If :meth:`handle_request` receives no incoming requests within the
   timeout period, the :meth:`handle_timeout` method is called.


There are various server methods that can be overridden by subclasses of base
server classes like :class:`TCPServer`; these methods aren't useful to external
users of the server object.

.. XXX should the default implementations of these be documented, or should
   it be assumed that the user will look at SocketServer.py?

.. method:: BaseServer.finish_request()

   Actually processes the request by instantiating :attr:`RequestHandlerClass` and
   calling its :meth:`handle` method.


.. method:: BaseServer.get_request()

   Must accept a request from the socket, and return a 2-tuple containing the *new*
   socket object to be used to communicate with the client, and the client's
   address.


.. method:: BaseServer.handle_error(request, client_address)

   This function is called if the :attr:`RequestHandlerClass`'s :meth:`handle`
   method raises an exception.  The default action is to print the traceback to
   standard output and continue handling further requests.


.. method:: BaseServer.handle_timeout()

   This function is called when the :attr:`timeout` attribute has been set to a
   value other than :const:`None` and the timeout period has passed with no
   requests being received.  The default action for forking servers is
   to collect the status of any child processes that have exited, while
   in threading servers this method does nothing.


.. method:: BaseServer.process_request(request, client_address)

   Calls :meth:`finish_request` to create an instance of the
   :attr:`RequestHandlerClass`.  If desired, this function can create a new process
   or thread to handle the request; the :class:`ForkingMixIn` and
   :class:`ThreadingMixIn` classes do this.


.. Is there any point in documenting the following two functions?
   What would the purpose of overriding them be: initializing server
   instance variables, adding new network families?

.. method:: BaseServer.server_activate()

   Called by the server's constructor to activate the server.  The default behavior
   just :meth:`listen`\ s to the server's socket. May be overridden.


.. method:: BaseServer.server_bind()

   Called by the server's constructor to bind the socket to the desired address.
   May be overridden.


.. method:: BaseServer.verify_request(request, client_address)

   Must return a Boolean value; if the value is :const:`True`, the request will be
   processed, and if it's :const:`False`, the request will be denied. This function
   can be overridden to implement access controls for a server. The default
   implementation always returns :const:`True`.


RequestHandler Objects
----------------------

The request handler class must define a new :meth:`handle` method, and can
override any of the following methods.  A new instance is created for each
request.


.. method:: RequestHandler.finish()

   Called after the :meth:`handle` method to perform any clean-up actions
   required.  The default implementation does nothing.  If :meth:`setup` or
   :meth:`handle` raise an exception, this function will not be called.


.. method:: RequestHandler.handle()

   This function must do all the work required to service a request.  The
   default implementation does nothing.  Several instance attributes are
   available to it; the request is available as :attr:`self.request`; the client
   address as :attr:`self.client_address`; and the server instance as
   :attr:`self.server`, in case it needs access to per-server information.

   The type of :attr:`self.request` is different for datagram or stream
   services.  For stream services, :attr:`self.request` is a socket object; for
   datagram services, :attr:`self.request` is a pair of string and socket.
   However, this can be hidden by using the request handler subclasses
   :class:`StreamRequestHandler` or :class:`DatagramRequestHandler`, which
   override the :meth:`setup` and :meth:`finish` methods, and provide
   :attr:`self.rfile` and :attr:`self.wfile` attributes.  :attr:`self.rfile` and
   :attr:`self.wfile` can be read or written, respectively, to get the request
   data or return data to the client.


.. method:: RequestHandler.setup()

   Called before the :meth:`handle` method to perform any initialization actions
   required.  The default implementation does nothing.


Examples
--------

:class:`SocketServer.TCPServer` Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the server side::

   import SocketServer

   class MyTCPHandler(SocketServer.BaseRequestHandler):
       """
       The RequestHandler class for our server.

       It is instantiated once per connection to the server, and must
       override the handle() method to implement communication to the
       client.
       """

       def handle(self):
           # self.request is the TCP socket connected to the client
           self.data = self.request.recv(1024).strip()
           print "{} wrote:".format(self.client_address[0])
           print self.data
           # just send back the same data, but upper-cased
           self.request.sendall(self.data.upper())

   if __name__ == "__main__":
       HOST, PORT = "localhost", 9999

       # Create the server, binding to localhost on port 9999
       server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)

       # Activate the server; this will keep running until you
       # interrupt the program with Ctrl-C
       server.serve_forever()

An alternative request handler class that makes use of streams (file-like
objects that simplify communication by providing the standard file interface)::

   class MyTCPHandler(SocketServer.StreamRequestHandler):

       def handle(self):
           # self.rfile is a file-like object created by the handler;
           # we can now use e.g. readline() instead of raw recv() calls
           self.data = self.rfile.readline().strip()
           print "{} wrote:".format(self.client_address[0])
           print self.data
           # Likewise, self.wfile is a file-like object used to write back
           # to the client
           self.wfile.write(self.data.upper())

The difference is that the ``readline()`` call in the second handler will call
``recv()`` multiple times until it encounters a newline character, while the
single ``recv()`` call in the first handler will just return what has been sent
from the client in one ``sendall()`` call.


This is the client side::

   import socket
   import sys

   HOST, PORT = "localhost", 9999
   data = " ".join(sys.argv[1:])

   # Create a socket (SOCK_STREAM means a TCP socket)
   sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

   try:
       # Connect to server and send data
       sock.connect((HOST, PORT))
       sock.sendall(data + "\n")

       # Receive data from the server and shut down
       received = sock.recv(1024)
   finally:
       sock.close()

   print "Sent:     {}".format(data)
   print "Received: {}".format(received)


The output of the example should look something like this:

Server::

   $ python TCPServer.py
   127.0.0.1 wrote:
   hello world with TCP
   127.0.0.1 wrote:
   python is nice

Client::

   $ python TCPClient.py hello world with TCP
   Sent:     hello world with TCP
   Received: HELLO WORLD WITH TCP
   $ python TCPClient.py python is nice
   Sent:     python is nice
   Received: PYTHON IS NICE


:class:`SocketServer.UDPServer` Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the server side::

   import SocketServer

   class MyUDPHandler(SocketServer.BaseRequestHandler):
       """
       This class works similar to the TCP handler class, except that
       self.request consists of a pair of data and client socket, and since
       there is no connection the client address must be given explicitly
       when sending data back via sendto().
       """

       def handle(self):
           data = self.request[0].strip()
           socket = self.request[1]
           print "{} wrote:".format(self.client_address[0])
           print data
           socket.sendto(data.upper(), self.client_address)

   if __name__ == "__main__":
       HOST, PORT = "localhost", 9999
       server = SocketServer.UDPServer((HOST, PORT), MyUDPHandler)
       server.serve_forever()

This is the client side::

   import socket
   import sys

   HOST, PORT = "localhost", 9999
   data = " ".join(sys.argv[1:])

   # SOCK_DGRAM is the socket type to use for UDP sockets
   sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

   # As you can see, there is no connect() call; UDP has no connections.
   # Instead, data is directly sent to the recipient via sendto().
   sock.sendto(data + "\n", (HOST, PORT))
   received = sock.recv(1024)

   print "Sent:     {}".format(data)
   print "Received: {}".format(received)

The output of the example should look exactly like for the TCP server example.


Asynchronous Mixins
~~~~~~~~~~~~~~~~~~~

To build asynchronous handlers, use the :class:`ThreadingMixIn` and
:class:`ForkingMixIn` classes.

An example for the :class:`ThreadingMixIn` class::

   import socket
   import threading
   import SocketServer

   class ThreadedTCPRequestHandler(SocketServer.BaseRequestHandler):

       def handle(self):
           data = self.request.recv(1024)
           cur_thread = threading.current_thread()
           response = "{}: {}".format(cur_thread.name, data)
           self.request.sendall(response)

   class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
       pass

   def client(ip, port, message):
       sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
       sock.connect((ip, port))
       try:
           sock.sendall(message)
           response = sock.recv(1024)
           print "Received: {}".format(response)
       finally:
           sock.close()

   if __name__ == "__main__":
       # Port 0 means to select an arbitrary unused port
       HOST, PORT = "localhost", 0

       server = ThreadedTCPServer((HOST, PORT), ThreadedTCPRequestHandler)
       ip, port = server.server_address

       # Start a thread with the server -- that thread will then start one
       # more thread for each request
       server_thread = threading.Thread(target=server.serve_forever)
       # Exit the server thread when the main thread terminates
       server_thread.daemon = True
       server_thread.start()
       print "Server loop running in thread:", server_thread.name

       client(ip, port, "Hello World 1")
       client(ip, port, "Hello World 2")
       client(ip, port, "Hello World 3")

       server.shutdown()


The output of the example should look something like this::

   $ python ThreadedTCPServer.py
   Server loop running in thread: Thread-1
   Received: Thread-2: Hello World 1
   Received: Thread-3: Hello World 2
   Received: Thread-4: Hello World 3


The :class:`ForkingMixIn` class is used in the same way, except that the server
will spawn a new process for each request.
