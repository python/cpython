:mod:`SimpleXMLRPCServer` --- Basic XML-RPC server
==================================================

.. module:: SimpleXMLRPCServer
   :synopsis: Basic XML-RPC server implementation.
.. moduleauthor:: Brian Quinlan <brianq@activestate.com>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

.. note::
   The :mod:`SimpleXMLRPCServer` module has been merged into
   :mod:`xmlrpc.server` in Python 3.  The :term:`2to3` tool will automatically
   adapt imports when converting your sources to Python 3.


.. versionadded:: 2.2

**Source code:** :source:`Lib/SimpleXMLRPCServer.py`

--------------

The :mod:`SimpleXMLRPCServer` module provides a basic server framework for
XML-RPC servers written in Python.  Servers can either be free standing, using
:class:`~SimpleXMLRPCServer.SimpleXMLRPCServer`, or embedded in a CGI environment, using
:class:`CGIXMLRPCRequestHandler`.


.. class:: SimpleXMLRPCServer(addr[, requestHandler[, logRequests[, allow_none[, encoding[, bind_and_activate]]]])

   Create a new server instance.  This class provides methods for registration of
   functions that can be called by the XML-RPC protocol.  The *requestHandler*
   parameter should be a factory for request handler instances; it defaults to
   :class:`SimpleXMLRPCRequestHandler`.  The *addr* and *requestHandler* parameters
   are passed to the :class:`SocketServer.TCPServer` constructor.  If *logRequests*
   is true (the default), requests will be logged; setting this parameter to false
   will turn off logging.   The *allow_none* and *encoding* parameters are passed
   on to  :mod:`xmlrpclib` and control the XML-RPC responses that will be returned
   from the server. The *bind_and_activate* parameter controls whether
   :meth:`server_bind` and :meth:`server_activate` are called immediately by the
   constructor; it defaults to true. Setting it to false allows code to manipulate
   the *allow_reuse_address* class variable before the address is bound.

   .. versionchanged:: 2.5
      The *allow_none* and *encoding* parameters were added.

   .. versionchanged:: 2.6
      The *bind_and_activate* parameter was added.


.. class:: CGIXMLRPCRequestHandler([allow_none[, encoding]])

   Create a new instance to handle XML-RPC requests in a CGI environment.  The
   *allow_none* and *encoding* parameters are passed on to  :mod:`xmlrpclib` and
   control the XML-RPC responses that will be returned  from the server.

   .. versionadded:: 2.3

   .. versionchanged:: 2.5
      The *allow_none* and *encoding* parameters were added.


.. class:: SimpleXMLRPCRequestHandler()

   Create a new request handler instance.  This request handler supports ``POST``
   requests and modifies logging so that the *logRequests* parameter to the
   :class:`~SimpleXMLRPCServer.SimpleXMLRPCServer` constructor parameter is honored.


.. _simple-xmlrpc-servers:

SimpleXMLRPCServer Objects
--------------------------

The :class:`~SimpleXMLRPCServer.SimpleXMLRPCServer` class is based on
:class:`SocketServer.TCPServer` and provides a means of creating simple, stand
alone XML-RPC servers.


.. method:: SimpleXMLRPCServer.register_function(function[, name])

   Register a function that can respond to XML-RPC requests.  If *name* is given,
   it will be the method name associated with *function*, otherwise
   ``function.__name__`` will be used.  *name* can be either a normal or Unicode
   string, and may contain characters not legal in Python identifiers, including
   the period character.


.. method:: SimpleXMLRPCServer.register_instance(instance[, allow_dotted_names])

   Register an object which is used to expose method names which have not been
   registered using :meth:`register_function`.  If *instance* contains a
   :meth:`_dispatch` method, it is called with the requested method name and the
   parameters from the request.  Its API is ``def _dispatch(self, method, params)``
   (note that *params* does not represent a variable argument list).  If it calls
   an underlying function to perform its task, that function is called as
   ``func(*params)``, expanding the parameter list. The return value from
   :meth:`_dispatch` is returned to the client as the result.  If *instance* does
   not have a :meth:`_dispatch` method, it is searched for an attribute matching
   the name of the requested method.

   If the optional *allow_dotted_names* argument is true and the instance does not
   have a :meth:`_dispatch` method, then if the requested method name contains
   periods, each component of the method name is searched for individually, with
   the effect that a simple hierarchical search is performed.  The value found from
   this search is then called with the parameters from the request, and the return
   value is passed back to the client.

   .. warning::

      Enabling the *allow_dotted_names* option allows intruders to access your
      module's global variables and may allow intruders to execute arbitrary code on
      your machine.  Only use this option on a secure, closed network.

   .. versionchanged:: 2.3.5, 2.4.1
      *allow_dotted_names* was added to plug a security hole; prior versions are
      insecure.


.. method:: SimpleXMLRPCServer.register_introspection_functions()

   Registers the XML-RPC introspection functions ``system.listMethods``,
   ``system.methodHelp`` and ``system.methodSignature``.

   .. versionadded:: 2.3


.. method:: SimpleXMLRPCServer.register_multicall_functions()

   Registers the XML-RPC multicall function system.multicall.


.. attribute:: SimpleXMLRPCRequestHandler.rpc_paths

   An attribute value that must be a tuple listing valid path portions of the URL
   for receiving XML-RPC requests.  Requests posted to other paths will result in a
   404 "no such page" HTTP error.  If this tuple is empty, all paths will be
   considered valid. The default value is ``('/', '/RPC2')``.

   .. versionadded:: 2.5

.. attribute:: SimpleXMLRPCRequestHandler.encode_threshold

   If this attribute is not ``None``, responses larger than this value
   will be encoded using the *gzip* transfer encoding, if permitted by
   the client.  The default is ``1400`` which corresponds roughly
   to a single TCP packet.

   .. versionadded:: 2.7

.. _simplexmlrpcserver-example:

SimpleXMLRPCServer Example
^^^^^^^^^^^^^^^^^^^^^^^^^^
Server code::

   from SimpleXMLRPCServer import SimpleXMLRPCServer
   from SimpleXMLRPCServer import SimpleXMLRPCRequestHandler

   # Restrict to a particular path.
   class RequestHandler(SimpleXMLRPCRequestHandler):
       rpc_paths = ('/RPC2',)

   # Create server
   server = SimpleXMLRPCServer(("localhost", 8000),
                               requestHandler=RequestHandler)
   server.register_introspection_functions()

   # Register pow() function; this will use the value of
   # pow.__name__ as the name, which is just 'pow'.
   server.register_function(pow)

   # Register a function under a different name
   def adder_function(x,y):
       return x + y
   server.register_function(adder_function, 'add')

   # Register an instance; all the methods of the instance are
   # published as XML-RPC methods (in this case, just 'div').
   class MyFuncs:
       def div(self, x, y):
           return x // y

   server.register_instance(MyFuncs())

   # Run the server's main loop
   server.serve_forever()

The following client code will call the methods made available by the preceding
server::

   import xmlrpclib

   s = xmlrpclib.ServerProxy('http://localhost:8000')
   print s.pow(2,3)  # Returns 2**3 = 8
   print s.add(2,3)  # Returns 5
   print s.div(5,2)  # Returns 5//2 = 2

   # Print list of available methods
   print s.system.listMethods()

The following :class:`~SimpleXMLRPCServer.SimpleXMLRPCServer` example is included in the module
`Lib/SimpleXMLRPCServer.py`::

    server = SimpleXMLRPCServer(("localhost", 8000))
    server.register_function(pow)
    server.register_function(lambda x,y: x+y, 'add')
    server.register_multicall_functions()
    server.serve_forever()

This demo server can be run from the command line as::

    python -m SimpleXMLRPCServer

Example client code which talks to the above server is included with
`Lib/xmlrpclib.py`::

    server = ServerProxy("http://localhost:8000")
    print server
    multi = MultiCall(server)
    multi.pow(2, 9)
    multi.add(5, 1)
    multi.add(24, 11)
    try:
        for response in multi():
            print response
    except Error, v:
        print "ERROR", v

And the client can be invoked directly using the following command::

    python -m xmlrpclib


CGIXMLRPCRequestHandler
-----------------------

The :class:`CGIXMLRPCRequestHandler` class can be used to  handle XML-RPC
requests sent to Python CGI scripts.


.. method:: CGIXMLRPCRequestHandler.register_function(function[, name])

   Register a function that can respond to XML-RPC requests. If  *name* is given,
   it will be the method name associated with  function, otherwise
   *function.__name__* will be used. *name* can be either a normal or Unicode
   string, and may contain  characters not legal in Python identifiers, including
   the period character.


.. method:: CGIXMLRPCRequestHandler.register_instance(instance)

   Register an object which is used to expose method names  which have not been
   registered using :meth:`register_function`. If  instance contains a
   :meth:`_dispatch` method, it is called with the  requested method name and the
   parameters from the  request; the return value is returned to the client as the
   result. If instance does not have a :meth:`_dispatch` method, it is searched
   for an attribute matching the name of the requested method; if  the requested
   method name contains periods, each  component of the method name is searched for
   individually,  with the effect that a simple hierarchical search is performed.
   The value found from this search is then called with the  parameters from the
   request, and the return value is passed  back to the client.


.. method:: CGIXMLRPCRequestHandler.register_introspection_functions()

   Register the XML-RPC introspection functions  ``system.listMethods``,
   ``system.methodHelp`` and  ``system.methodSignature``.


.. method:: CGIXMLRPCRequestHandler.register_multicall_functions()

   Register the XML-RPC multicall function ``system.multicall``.


.. method:: CGIXMLRPCRequestHandler.handle_request([request_text = None])

   Handle an XML-RPC request. If *request_text* is given, it should be the POST
   data provided by the HTTP server,  otherwise the contents of stdin will be used.

Example::

   class MyFuncs:
       def div(self, x, y): return x // y


   handler = CGIXMLRPCRequestHandler()
   handler.register_function(pow)
   handler.register_function(lambda x,y: x+y, 'add')
   handler.register_introspection_functions()
   handler.register_instance(MyFuncs())
   handler.handle_request()

