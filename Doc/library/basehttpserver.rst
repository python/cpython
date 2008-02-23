
:mod:`BaseHTTPServer` --- Basic HTTP server
===========================================

.. module:: BaseHTTPServer
   :synopsis: Basic HTTP server (base class for SimpleHTTPServer and CGIHTTPServer).


.. index::
   pair: WWW; server
   pair: HTTP; protocol
   single: URL
   single: httpd

.. index::
   module: SimpleHTTPServer
   module: CGIHTTPServer

This module defines two classes for implementing HTTP servers (Web servers).
Usually, this module isn't used directly, but is used as a basis for building
functioning Web servers. See the :mod:`SimpleHTTPServer` and
:mod:`CGIHTTPServer` modules.

The first class, :class:`HTTPServer`, is a :class:`SocketServer.TCPServer`
subclass.  It creates and listens at the HTTP socket, dispatching the requests
to a handler.  Code to create and run the server looks like this::

   def run(server_class=BaseHTTPServer.HTTPServer,
           handler_class=BaseHTTPServer.BaseHTTPRequestHandler):
       server_address = ('', 8000)
       httpd = server_class(server_address, handler_class)
       httpd.serve_forever()


.. class:: HTTPServer(server_address, RequestHandlerClass)

   This class builds on the :class:`TCPServer` class by storing the server address
   as instance variables named :attr:`server_name` and :attr:`server_port`. The
   server is accessible by the handler, typically through the handler's
   :attr:`server` instance variable.


.. class:: BaseHTTPRequestHandler(request, client_address, server)

   This class is used to handle the HTTP requests that arrive at the server. By
   itself, it cannot respond to any actual HTTP requests; it must be subclassed to
   handle each request method (e.g. GET or POST). :class:`BaseHTTPRequestHandler`
   provides a number of class and instance variables, and methods for use by
   subclasses.

   The handler will parse the request and the headers, then call a method specific
   to the request type. The method name is constructed from the request. For
   example, for the request method ``SPAM``, the :meth:`do_SPAM` method will be
   called with no arguments. All of the relevant information is stored in instance
   variables of the handler.  Subclasses should not need to override or extend the
   :meth:`__init__` method.

:class:`BaseHTTPRequestHandler` has the following instance variables:


.. attribute:: BaseHTTPRequestHandler.client_address

   Contains a tuple of the form ``(host, port)`` referring to the client's address.


.. attribute:: BaseHTTPRequestHandler.command

   Contains the command (request type). For example, ``'GET'``.


.. attribute:: BaseHTTPRequestHandler.path

   Contains the request path.


.. attribute:: BaseHTTPRequestHandler.request_version

   Contains the version string from the request. For example, ``'HTTP/1.0'``.


.. attribute:: BaseHTTPRequestHandler.headers

   Holds an instance of the class specified by the :attr:`MessageClass` class
   variable. This instance parses and manages the headers in the HTTP request.


.. attribute:: BaseHTTPRequestHandler.rfile

   Contains an input stream, positioned at the start of the optional input data.


.. attribute:: BaseHTTPRequestHandler.wfile

   Contains the output stream for writing a response back to the client. Proper
   adherence to the HTTP protocol must be used when writing to this stream.

:class:`BaseHTTPRequestHandler` has the following class variables:


.. attribute:: BaseHTTPRequestHandler.server_version

   Specifies the server software version.  You may want to override this. The
   format is multiple whitespace-separated strings, where each string is of the
   form name[/version]. For example, ``'BaseHTTP/0.2'``.


.. attribute:: BaseHTTPRequestHandler.sys_version

   Contains the Python system version, in a form usable by the
   :attr:`version_string` method and the :attr:`server_version` class variable. For
   example, ``'Python/1.4'``.


.. attribute:: BaseHTTPRequestHandler.error_message_format

   Specifies a format string for building an error response to the client. It uses
   parenthesized, keyed format specifiers, so the format operand must be a
   dictionary. The *code* key should be an integer, specifying the numeric HTTP
   error code value. *message* should be a string containing a (detailed) error
   message of what occurred, and *explain* should be an explanation of the error
   code number. Default *message* and *explain* values can found in the *responses*
   class variable.


.. attribute:: BaseHTTPRequestHandler.error_content_type

   Specifies the Content-Type HTTP header of error responses sent to the client.
   The default value is ``'text/html'``.

   .. versionadded:: 2.6
      Previously, the content type was always ``'text/html'``.


.. attribute:: BaseHTTPRequestHandler.protocol_version

   This specifies the HTTP protocol version used in responses.  If set to
   ``'HTTP/1.1'``, the server will permit HTTP persistent connections; however,
   your server *must* then include an accurate ``Content-Length`` header (using
   :meth:`send_header`) in all of its responses to clients.  For backwards
   compatibility, the setting defaults to ``'HTTP/1.0'``.


.. attribute:: BaseHTTPRequestHandler.MessageClass

   .. index:: single: Message (in module mimetools)

   Specifies a :class:`rfc822.Message`\ -like class to parse HTTP headers.
   Typically, this is not overridden, and it defaults to
   :class:`mimetools.Message`.


.. attribute:: BaseHTTPRequestHandler.responses

   This variable contains a mapping of error code integers to two-element tuples
   containing a short and long message. For example, ``{code: (shortmessage,
   longmessage)}``. The *shortmessage* is usually used as the *message* key in an
   error response, and *longmessage* as the *explain* key (see the
   :attr:`error_message_format` class variable).

A :class:`BaseHTTPRequestHandler` instance has the following methods:


.. method:: BaseHTTPRequestHandler.handle()

   Calls :meth:`handle_one_request` once (or, if persistent connections are
   enabled, multiple times) to handle incoming HTTP requests. You should never need
   to override it; instead, implement appropriate :meth:`do_\*` methods.


.. method:: BaseHTTPRequestHandler.handle_one_request()

   This method will parse and dispatch the request to the appropriate :meth:`do_\*`
   method.  You should never need to override it.


.. method:: BaseHTTPRequestHandler.send_error(code[, message])

   Sends and logs a complete error reply to the client. The numeric *code*
   specifies the HTTP error code, with *message* as optional, more specific text. A
   complete set of headers is sent, followed by text composed using the
   :attr:`error_message_format` class variable.


.. method:: BaseHTTPRequestHandler.send_response(code[, message])

   Sends a response header and logs the accepted request. The HTTP response line is
   sent, followed by *Server* and *Date* headers. The values for these two headers
   are picked up from the :meth:`version_string` and :meth:`date_time_string`
   methods, respectively.


.. method:: BaseHTTPRequestHandler.send_header(keyword, value)

   Writes a specific HTTP header to the output stream. *keyword* should specify the
   header keyword, with *value* specifying its value.


.. method:: BaseHTTPRequestHandler.end_headers()

   Sends a blank line, indicating the end of the HTTP headers in the response.


.. method:: BaseHTTPRequestHandler.log_request([code[, size]])

   Logs an accepted (successful) request. *code* should specify the numeric HTTP
   code associated with the response. If a size of the response is available, then
   it should be passed as the *size* parameter.


.. method:: BaseHTTPRequestHandler.log_error(...)

   Logs an error when a request cannot be fulfilled. By default, it passes the
   message to :meth:`log_message`, so it takes the same arguments (*format* and
   additional values).


.. method:: BaseHTTPRequestHandler.log_message(format, ...)

   Logs an arbitrary message to ``sys.stderr``. This is typically overridden to
   create custom error logging mechanisms. The *format* argument is a standard
   printf-style format string, where the additional arguments to
   :meth:`log_message` are applied as inputs to the formatting. The client address
   and current date and time are prefixed to every message logged.


.. method:: BaseHTTPRequestHandler.version_string()

   Returns the server software's version string. This is a combination of the
   :attr:`server_version` and :attr:`sys_version` class variables.


.. method:: BaseHTTPRequestHandler.date_time_string([timestamp])

   Returns the date and time given by *timestamp* (which must be in the format
   returned by :func:`time.time`), formatted for a message header. If *timestamp*
   is omitted, it uses the current date and time.

   The result looks like ``'Sun, 06 Nov 1994 08:49:37 GMT'``.


.. method:: BaseHTTPRequestHandler.log_date_time_string()

   Returns the current date and time, formatted for logging.


.. method:: BaseHTTPRequestHandler.address_string()

   Returns the client address, formatted for logging. A name lookup is performed on
   the client's IP address.


.. seealso::

   Module :mod:`CGIHTTPServer`
      Extended request handler that supports CGI scripts.

   Module :mod:`SimpleHTTPServer`
      Basic request handler that limits response to files actually under the document
      root.

