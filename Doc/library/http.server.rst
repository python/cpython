:mod:`http.server` --- HTTP servers
===================================

.. module:: http.server
   :synopsis: HTTP server and request handlers.


.. index::
   pair: WWW; server
   pair: HTTP; protocol
   single: URL
   single: httpd

**Source code:** :source:`Lib/http/server.py`

--------------

This module defines classes for implementing HTTP servers (Web servers).

One class, :class:`HTTPServer`, is a :class:`socketserver.TCPServer` subclass.
It creates and listens at the HTTP socket, dispatching the requests to a
handler.  Code to create and run the server looks like this::

   def run(server_class=HTTPServer, handler_class=BaseHTTPRequestHandler):
       server_address = ('', 8000)
       httpd = server_class(server_address, handler_class)
       httpd.serve_forever()


.. class:: HTTPServer(server_address, RequestHandlerClass)

   This class builds on the :class:`~socketserver.TCPServer` class by storing
   the server address as instance variables named :attr:`server_name` and
   :attr:`server_port`. The server is accessible by the handler, typically
   through the handler's :attr:`server` instance variable.


The :class:`HTTPServer` must be given a *RequestHandlerClass* on instantiation,
of which this module provides three different variants:

.. class:: BaseHTTPRequestHandler(request, client_address, server)

   This class is used to handle the HTTP requests that arrive at the server.  By
   itself, it cannot respond to any actual HTTP requests; it must be subclassed
   to handle each request method (e.g. GET or POST).
   :class:`BaseHTTPRequestHandler` provides a number of class and instance
   variables, and methods for use by subclasses.

   The handler will parse the request and the headers, then call a method
   specific to the request type. The method name is constructed from the
   request. For example, for the request method ``SPAM``, the :meth:`do_SPAM`
   method will be called with no arguments. All of the relevant information is
   stored in instance variables of the handler.  Subclasses should not need to
   override or extend the :meth:`__init__` method.

   :class:`BaseHTTPRequestHandler` has the following instance variables:

   .. attribute:: client_address

      Contains a tuple of the form ``(host, port)`` referring to the client's
      address.

   .. attribute:: server

      Contains the server instance.

   .. attribute:: close_connection

      Boolean that should be set before :meth:`handle_one_request` returns,
      indicating if another request may be expected, or if the connection should
      be shut down.

   .. attribute:: requestline

      Contains the string representation of the HTTP request line. The
      terminating CRLF is stripped. This attribute should be set by
      :meth:`handle_one_request`. If no valid request line was processed, it
      should be set to the empty string.

   .. attribute:: command

      Contains the command (request type). For example, ``'GET'``.

   .. attribute:: path

      Contains the request path.

   .. attribute:: request_version

      Contains the version string from the request. For example, ``'HTTP/1.0'``.

   .. attribute:: headers

      Holds an instance of the class specified by the :attr:`MessageClass` class
      variable. This instance parses and manages the headers in the HTTP
      request. The :func:`~http.client.parse_headers` function from
      :mod:`http.client` is used to parse the headers and it requires that the
      HTTP request provide a valid :rfc:`2822` style header.


   .. attribute:: rfile

      Contains an input stream, positioned at the start of the optional input
      data.

   .. attribute:: wfile

      Contains the output stream for writing a response back to the
      client. Proper adherence to the HTTP protocol must be used when writing to
      this stream.

   :class:`BaseHTTPRequestHandler` has the following class variables:

   .. attribute:: server_version

      Specifies the server software version.  You may want to override this. The
      format is multiple whitespace-separated strings, where each string is of
      the form name[/version]. For example, ``'BaseHTTP/0.2'``.

   .. attribute:: sys_version

      Contains the Python system version, in a form usable by the
      :attr:`version_string` method and the :attr:`server_version` class
      variable. For example, ``'Python/1.4'``.

   .. attribute:: error_message_format

      Specifies a format string for building an error response to the client. It
      uses parenthesized, keyed format specifiers, so the format operand must be
      a dictionary. The *code* key should be an integer, specifying the numeric
      HTTP error code value. *message* should be a string containing a
      (detailed) error message of what occurred, and *explain* should be an
      explanation of the error code number. Default *message* and *explain*
      values can found in the :attr:`responses` class variable.

   .. attribute:: error_content_type

      Specifies the Content-Type HTTP header of error responses sent to the
      client.  The default value is ``'text/html'``.

   .. attribute:: protocol_version

      This specifies the HTTP protocol version used in responses.  If set to
      ``'HTTP/1.1'``, the server will permit HTTP persistent connections;
      however, your server *must* then include an accurate ``Content-Length``
      header (using :meth:`send_header`) in all of its responses to clients.
      For backwards compatibility, the setting defaults to ``'HTTP/1.0'``.

   .. attribute:: MessageClass

      Specifies an :class:`email.message.Message`\ -like class to parse HTTP
      headers.  Typically, this is not overridden, and it defaults to
      :class:`http.client.HTTPMessage`.

   .. attribute:: responses

      This variable contains a mapping of error code integers to two-element tuples
      containing a short and long message. For example, ``{code: (shortmessage,
      longmessage)}``. The *shortmessage* is usually used as the *message* key in an
      error response, and *longmessage* as the *explain* key (see the
      :attr:`error_message_format` class variable).

   A :class:`BaseHTTPRequestHandler` instance has the following methods:

   .. method:: handle()

      Calls :meth:`handle_one_request` once (or, if persistent connections are
      enabled, multiple times) to handle incoming HTTP requests. You should
      never need to override it; instead, implement appropriate :meth:`do_\*`
      methods.

   .. method:: handle_one_request()

      This method will parse and dispatch the request to the appropriate
      :meth:`do_\*` method.  You should never need to override it.

   .. method:: handle_expect_100()

      When a HTTP/1.1 compliant server receives a ``Expect: 100-continue``
      request header it responds back with a ``100 Continue`` followed by ``200
      OK`` headers.
      This method can be overridden to raise an error if the server does not
      want the client to continue.  For e.g. server can chose to send ``417
      Expectation Failed`` as a response header and ``return False``.

      .. versionadded:: 3.2

   .. method:: send_error(code, message=None, explain=None)

      Sends and logs a complete error reply to the client. The numeric *code*
      specifies the HTTP error code, with *message* as an optional, short, human
      readable description of the error.  The *explain* argument can be used to
      provide more detailed information about the error; it will be formatted
      using the :attr:`error_message_format` class variable and emitted, after
      a complete set of headers, as the response body.  The :attr:`responses`
      class variable holds the default values for *message* and *explain* that
      will be used if no value is provided; for unknown codes the default value
      for both is the string ``???``.

      .. versionchanged:: 3.4
         The error response includes a Content-Length header.
         Added the *explain* argument.


   .. method:: send_response(code, message=None)

      Adds a response header to the headers buffer and logs the accepted
      request. The HTTP response line is written to the internal buffer,
      followed by *Server* and *Date* headers. The values for these two headers
      are picked up from the :meth:`version_string` and
      :meth:`date_time_string` methods, respectively. If the server does not
      intend to send any other headers using the :meth:`send_header` method,
      then :meth:`send_response` should be followed by a :meth:`end_headers`
      call.

      .. versionchanged:: 3.3
         Headers are stored to an internal buffer and :meth:`end_headers`
         needs to be called explicitly.


   .. method:: send_header(keyword, value)

      Adds the HTTP header to an internal buffer which will be written to the
      output stream when either :meth:`end_headers` or :meth:`flush_headers` is
      invoked. *keyword* should specify the header keyword, with *value*
      specifying its value. Note that, after the send_header calls are done,
      :meth:`end_headers` MUST BE called in order to complete the operation.

      .. versionchanged:: 3.2
         Headers are stored in an internal buffer.


   .. method:: send_response_only(code, message=None)

      Sends the response header only, used for the purposes when ``100
      Continue`` response is sent by the server to the client. The headers not
      buffered and sent directly the output stream.If the *message* is not
      specified, the HTTP message corresponding the response *code*  is sent.

      .. versionadded:: 3.2

   .. method:: end_headers()

      Adds a blank line
      (indicating the end of the HTTP headers in the response)
      to the headers buffer and calls :meth:`flush_headers()`.

      .. versionchanged:: 3.2
         The buffered headers are written to the output stream.

   .. method:: flush_headers()

      Finally send the headers to the output stream and flush the internal
      headers buffer.

      .. versionadded:: 3.3

   .. method:: log_request(code='-', size='-')

      Logs an accepted (successful) request. *code* should specify the numeric
      HTTP code associated with the response. If a size of the response is
      available, then it should be passed as the *size* parameter.

   .. method:: log_error(...)

      Logs an error when a request cannot be fulfilled. By default, it passes
      the message to :meth:`log_message`, so it takes the same arguments
      (*format* and additional values).


   .. method:: log_message(format, ...)

      Logs an arbitrary message to ``sys.stderr``. This is typically overridden
      to create custom error logging mechanisms. The *format* argument is a
      standard printf-style format string, where the additional arguments to
      :meth:`log_message` are applied as inputs to the formatting. The client
      ip address and current date and time are prefixed to every message logged.

   .. method:: version_string()

      Returns the server software's version string. This is a combination of the
      :attr:`server_version` and :attr:`sys_version` class variables.

   .. method:: date_time_string(timestamp=None)

      Returns the date and time given by *timestamp* (which must be None or in
      the format returned by :func:`time.time`), formatted for a message
      header. If *timestamp* is omitted, it uses the current date and time.

      The result looks like ``'Sun, 06 Nov 1994 08:49:37 GMT'``.

   .. method:: log_date_time_string()

      Returns the current date and time, formatted for logging.

   .. method:: address_string()

      Returns the client address.

      .. versionchanged:: 3.3
         Previously, a name lookup was performed. To avoid name resolution
         delays, it now always returns the IP address.


.. class:: SimpleHTTPRequestHandler(request, client_address, server)

   This class serves files from the current directory and below, directly
   mapping the directory structure to HTTP requests.

   A lot of the work, such as parsing the request, is done by the base class
   :class:`BaseHTTPRequestHandler`.  This class implements the :func:`do_GET`
   and :func:`do_HEAD` functions.

   The following are defined as class-level attributes of
   :class:`SimpleHTTPRequestHandler`:

   .. attribute:: server_version

      This will be ``"SimpleHTTP/" + __version__``, where ``__version__`` is
      defined at the module level.

   .. attribute:: extensions_map

      A dictionary mapping suffixes into MIME types. The default is
      signified by an empty string, and is considered to be
      ``application/octet-stream``. The mapping is used case-insensitively,
      and so should contain only lower-cased keys.

   The :class:`SimpleHTTPRequestHandler` class defines the following methods:

   .. method:: do_HEAD()

      This method serves the ``'HEAD'`` request type: it sends the headers it
      would send for the equivalent ``GET`` request. See the :meth:`do_GET`
      method for a more complete explanation of the possible headers.

   .. method:: do_GET()

      The request is mapped to a local file by interpreting the request as a
      path relative to the current working directory.

      If the request was mapped to a directory, the directory is checked for a
      file named ``index.html`` or ``index.htm`` (in that order). If found, the
      file's contents are returned; otherwise a directory listing is generated
      by calling the :meth:`list_directory` method. This method uses
      :func:`os.listdir` to scan the directory, and returns a ``404`` error
      response if the :func:`~os.listdir` fails.

      If the request was mapped to a file, it is opened and the contents are
      returned.  Any :exc:`OSError` exception in opening the requested file is
      mapped to a ``404``, ``'File not found'`` error. Otherwise, the content
      type is guessed by calling the :meth:`guess_type` method, which in turn
      uses the *extensions_map* variable.

      A ``'Content-type:'`` header with the guessed content type is output,
      followed by a ``'Content-Length:'`` header with the file's size and a
      ``'Last-Modified:'`` header with the file's modification time.

      Then follows a blank line signifying the end of the headers, and then the
      contents of the file are output. If the file's MIME type starts with
      ``text/`` the file is opened in text mode; otherwise binary mode is used.

      For example usage, see the implementation of the :func:`test` function
      invocation in the :mod:`http.server` module.


The :class:`SimpleHTTPRequestHandler` class can be used in the following
manner in order to create a very basic webserver serving files relative to
the current directory::

   import http.server
   import socketserver

   PORT = 8000

   Handler = http.server.SimpleHTTPRequestHandler

   httpd = socketserver.TCPServer(("", PORT), Handler)

   print("serving at port", PORT)
   httpd.serve_forever()

.. _http-server-cli:

:mod:`http.server` can also be invoked directly using the :option:`-m`
switch of the interpreter with a ``port number`` argument.  Similar to
the previous example, this serves files relative to the current directory::

        python -m http.server 8000

By default, server binds itself to all interfaces.  The option ``-b/--bind``
specifies a specific address to which it should bind.  For example, the
following command causes the server to bind to localhost only::

        python -m http.server 8000 --bind 127.0.0.1

.. versionadded:: 3.4
    ``--bind`` argument was introduced.


.. class:: CGIHTTPRequestHandler(request, client_address, server)

   This class is used to serve either files or output of CGI scripts from the
   current directory and below. Note that mapping HTTP hierarchic structure to
   local directory structure is exactly as in :class:`SimpleHTTPRequestHandler`.

   .. note::

      CGI scripts run by the :class:`CGIHTTPRequestHandler` class cannot execute
      redirects (HTTP code 302), because code 200 (script output follows) is
      sent prior to execution of the CGI script.  This pre-empts the status
      code.

   The class will however, run the CGI script, instead of serving it as a file,
   if it guesses it to be a CGI script.  Only directory-based CGI are used ---
   the other common server configuration is to treat special extensions as
   denoting CGI scripts.

   The :func:`do_GET` and :func:`do_HEAD` functions are modified to run CGI scripts
   and serve the output, instead of serving files, if the request leads to
   somewhere below the ``cgi_directories`` path.

   The :class:`CGIHTTPRequestHandler` defines the following data member:

   .. attribute:: cgi_directories

      This defaults to ``['/cgi-bin', '/htbin']`` and describes directories to
      treat as containing CGI scripts.

   The :class:`CGIHTTPRequestHandler` defines the following method:

   .. method:: do_POST()

      This method serves the ``'POST'`` request type, only allowed for CGI
      scripts.  Error 501, "Can only POST to CGI scripts", is output when trying
      to POST to a non-CGI url.

   Note that CGI scripts will be run with UID of user nobody, for security
   reasons.  Problems with the CGI script will be translated to error 403.

:class:`CGIHTTPRequestHandler` can be enabled in the command line by passing
the ``--cgi`` option::

        python -m http.server --cgi 8000

