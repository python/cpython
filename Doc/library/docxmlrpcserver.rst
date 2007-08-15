
:mod:`DocXMLRPCServer` --- Self-documenting XML-RPC server
==========================================================

.. module:: DocXMLRPCServer
   :synopsis: Self-documenting XML-RPC server implementation.
.. moduleauthor:: Brian Quinlan <brianq@activestate.com>
.. sectionauthor:: Brian Quinlan <brianq@activestate.com>


.. versionadded:: 2.3

The :mod:`DocXMLRPCServer` module extends the classes found in
:mod:`SimpleXMLRPCServer` to serve HTML documentation in response to HTTP GET
requests. Servers can either be free standing, using :class:`DocXMLRPCServer`,
or embedded in a CGI environment, using :class:`DocCGIXMLRPCRequestHandler`.


.. class:: DocXMLRPCServer(addr[, requestHandler[, logRequests[, allow_none[,  encoding[, bind_and_activate]]]]])

   Create a new server instance. All parameters have the same meaning as for
   :class:`SimpleXMLRPCServer.SimpleXMLRPCServer`; *requestHandler* defaults to
   :class:`DocXMLRPCRequestHandler`.


.. class:: DocCGIXMLRPCRequestHandler()

   Create a new instance to handle XML-RPC requests in a CGI environment.


.. class:: DocXMLRPCRequestHandler()

   Create a new request handler instance. This request handler supports XML-RPC
   POST requests, documentation GET requests, and modifies logging so that the
   *logRequests* parameter to the :class:`DocXMLRPCServer` constructor parameter is
   honored.


.. _doc-xmlrpc-servers:

DocXMLRPCServer Objects
-----------------------

The :class:`DocXMLRPCServer` class is derived from
:class:`SimpleXMLRPCServer.SimpleXMLRPCServer` and provides a means of creating
self-documenting, stand alone XML-RPC servers. HTTP POST requests are handled as
XML-RPC method calls. HTTP GET requests are handled by generating pydoc-style
HTML documentation. This allows a server to provide its own web-based
documentation.


.. method:: DocXMLRPCServer.set_server_title(server_title)

   Set the title used in the generated HTML documentation. This title will be used
   inside the HTML "title" element.


.. method:: DocXMLRPCServer.set_server_name(server_name)

   Set the name used in the generated HTML documentation. This name will appear at
   the top of the generated documentation inside a "h1" element.


.. method:: DocXMLRPCServer.set_server_documentation(server_documentation)

   Set the description used in the generated HTML documentation. This description
   will appear as a paragraph, below the server name, in the documentation.


DocCGIXMLRPCRequestHandler
--------------------------

The :class:`DocCGIXMLRPCRequestHandler` class is derived from
:class:`SimpleXMLRPCServer.CGIXMLRPCRequestHandler` and provides a means of
creating self-documenting, XML-RPC CGI scripts. HTTP POST requests are handled
as XML-RPC method calls. HTTP GET requests are handled by generating pydoc-style
HTML documentation. This allows a server to provide its own web-based
documentation.


.. method:: DocCGIXMLRPCRequestHandler.set_server_title(server_title)

   Set the title used in the generated HTML documentation. This title will be used
   inside the HTML "title" element.


.. method:: DocCGIXMLRPCRequestHandler.set_server_name(server_name)

   Set the name used in the generated HTML documentation. This name will appear at
   the top of the generated documentation inside a "h1" element.


.. method:: DocCGIXMLRPCRequestHandler.set_server_documentation(server_documentation)

   Set the description used in the generated HTML documentation. This description
   will appear as a paragraph, below the server name, in the documentation.

