:mod:`http.server` --- HTTP servers
===================================

.. module:: http.server
   :synopsis: HTTP server and request handlers.

**Source code:** :source:`Lib/http/server.py`

.. index::
   pair: WWW; server
   pair: HTTP; protocol
   single: URL
   single: httpd

--------------

This module defines classes for implementing HTTP servers.


.. warning::

    :mod:`http.server` is not recommended for production. It only implements
    basic security checks.

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

.. class:: ThreadingHTTPServer(server_address, RequestHandlerClass)

   This class is identical to HTTPServer but uses threads to handle
   requests by using the :class:`~socketserver.ThreadingMixIn`. This
   is useful to handle web browsers pre-opening sockets, on which
   :class:`HTTPServer` would wait indefinitely.

   .. versionadded:: 3.7


The :class:`HTTPServer` and :class:`ThreadingHTTPServer` must be given
a *RequestHandlerClass* on instantiation, of which this module
provides three different variants:

.. class:: BaseHTTPRequestHandler(request, client_address, server)

   This class is used to handle the HTTP requests that arrive at the server.  By
   itself, it cannot respond to any actual HTTP requests; it must be subclassed
   to handle each request method (e.g. GET or POST).
   :class:`BaseHTTPRequestHandler` provides a number of class and instance
   variables, and methods for use by subclasses.

   The handler will parse the request and the headers, then call a method
   specific to the request type. The method name is constructed from the
   request. For example, for the request method SPAM, the :meth:`do_SPAM`
   method will be called with no arguments. All of the relevant information is
   stored in instance variables of the handler.  Subclasses should not need to
   override or extend the :meth:`__init__` method.

   :class:`BaseHTTPRequestHandler` defines the following instance variables:

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

      Contains the request path. If query component of the URL is present,
      then ``path`` includes the query. Using the terminology of :rfc:`3986`,
      ``path`` here includes ``hier-part`` and the ``query``.

   .. attribute:: request_version

      Contains the version string from the request. For example, ``'HTTP/1.0'``.

   .. attribute:: headers

      Holds an instance of the class specified by the :attr:`MessageClass` class
      variable. This instance parses and manages the headers in the HTTP
      request. The :func:`~http.client.parse_headers` function from
      :mod:`http.client` is used to parse the headers and it requires that the
      HTTP request provide a valid :rfc:`2822` style header.

   .. attribute:: rfile

      An :class:`io.BufferedIOBase` input stream, ready to read from
      the start of the optional input data.

   .. attribute:: wfile

      Contains the output stream for writing a response back to the
      client. Proper adherence to the HTTP protocol must be used when writing to
      this stream in order to achieve successful interoperation with HTTP
      clients.

      .. versionchanged:: 3.6
         This is an :class:`io.BufferedIOBase` stream.

   :class:`BaseHTTPRequestHandler` defines the following class variables:

   .. attribute:: server_version

      Specifies the server software version based on ``__version__`` defined
      at the module level. You may want to override this. The
      format is multiple whitespace-separated strings, where each string is of
      the form name[/version]. For example, ``'BaseHTTP/0.2'``.

   .. attribute:: sys_version

      Specifies the Python system version, in a form usable by the
      :attr:`version_string` method and the :attr:`server_version` class
      variable. For example, ``'Python/1.4'``.

   .. attribute:: error_message_format

      Specifies a format string that should be used by :meth:`send_error` method
      for building an error response to the client. The string is filled by
      default with variables from :attr:`responses` based on the status code
      that passed to :meth:`send_error`.

   .. attribute:: error_content_type

      Specifies the Content-Type header of error responses sent to the
      client.  The default value is ``'text/html'``.

   .. attribute:: protocol_version

      Specifies the HTTP protocol version used in responses.  If set to
      ``'HTTP/1.1'``, the server will permit HTTP persistent connections;
      however, your server *must* then include an accurate Content-Length
      header (using :meth:`send_header`) in all of its responses to clients.
      For backwards compatibility, the default value is ``'HTTP/1.0'``.

   .. attribute:: MessageClass

      Specifies an :class:`email.message.Message`\ -like class to parse HTTP
      headers.  Typically, this is not overridden, and it defaults to
      :class:`http.client.HTTPMessage`.

   .. attribute:: responses

      This attribute contains a mapping of status codes to two-element tuples
      containing a reason phrase and long description.  For example, ``{code: (reason,
      description)}``.  The reason phrase is usually used as the *message* key in an
      error response, and the long description as the *explain* key.  It is used by
      :meth:`send_response_only` and :meth:`send_error` methods.

   :class:`BaseHTTPRequestHandler` defines the following methods:

   .. method:: handle()

      Calls :meth:`handle_one_request` once (or, if persistent connections are
      enabled, multiple times) to handle incoming HTTP requests. You should
      never need to override it; instead, implement appropriate :meth:`do_\*`
      methods.

   .. method:: handle_one_request()

      Parses and dispatches the request to the appropriate
      :meth:`do_\*` method.  You should never need to override it.

   .. method:: handle_expect_100()

      Sends a 100 (Continue) informational status line to the output stream.
      When a HTTP/1.1 compliant server receives a request with an ``Expect: 100-continue``
      header which indicates that a request message body will follow, it must either
      send a 100 (Continue) informational response to tell the client to continue, or send a response
      with a final status code to tell the client to stop.
      This method can be overridden to tell the client to stop. E.g. the server can
      choose to send a 417 (Expectation Failed) client error status line and ``return False``.

      .. versionadded:: 3.2

   .. method:: send_error(code, message=None, explain=None)

      Sends a complete error response to the client and logs the status code and reason phrase.
      *code* specifies the status code, *message* the reason phrase, and *explain*
      a long description of the error. If *message* or *explain* is not specified,
      the value corresponding to the status code in the :class:`http.HTTPStatus` enum is used.
      For unknown status codes, the default value for both is the string ``???``.
      *explain* will be formatted
      using the :attr:`error_message_format` attribute and emitted, after
      a complete set of headers, as the response body.
      The body will be empty if the method is
      HEAD or the status code is one of the following:
      1xx, 204 (No Content), 205 (Reset Content), 304 (Not Modified).

      .. versionchanged:: 3.4
         The error response includes a Content-Length header.
         Added the *explain* argument.

   .. method:: send_response(code, message=None)

      Adds a status line followed by a Server header and Date header to
      the internal buffer, and logs the status code. If *message* is not specified,
      the value corresponding to the status code in the :class:`http.HTTPStatus` enum is used.
      The values for the headers are picked up from the :meth:`version_string` and
      :meth:`date_time_string` methods, respectively. If the server does not
      intend to send any other headers using the :meth:`send_header` method,
      then :meth:`send_response` should be followed by an :meth:`end_headers`
      call.

      .. versionchanged:: 3.3
         Headers are stored to an internal buffer and :meth:`end_headers`
         needs to be called explicitly.

   .. method:: send_header(keyword, value)

      Adds the header to the internal buffer which will be written to the
      output stream when either :meth:`end_headers` or :meth:`flush_headers` is
      invoked. *keyword* specifies the header name and *value* its value.
      Note that, after the send_header calls are done,
      :meth:`end_headers` must be called in order to complete the operation.

      .. versionchanged:: 3.2
         Headers are stored in an internal buffer.

   .. method:: send_response_only(code, message=None)

      Adds a status line to the internal buffer. *code* specifies the status code
      and *message* the reason phrase. If *message* is not specified, the value
      corresponding to the status code in the :class:`http.HTTPStatus` enum is used.

      .. versionadded:: 3.2

   .. method:: end_headers()

      Adds a blank line (indicating the end of the headers in the response)
      to the internal buffer and calls :meth:`flush_headers()`.

      .. versionchanged:: 3.2
         The buffered headers are written to the output stream.

   .. method:: flush_headers()

      Sends the headers to the output stream and flush the internal buffer.

      .. versionadded:: 3.3

   .. method:: log_request(code='-', size='-')

      Logs an accepted (successful) request. *code* specifies the status code
      and *size* the response size, if available.

   .. method:: log_error(...)

      Logs an error when a request cannot be fulfilled. By default, it passes
      the message to :meth:`log_message`, so it takes the same arguments
      (*format* and additional values).

   .. method:: log_message(format, ...)

      Logs an arbitrary message to ``sys.stderr``. This is typically overridden
      to create custom error logging mechanisms. The *format* argument is a
      standard printf-style format string, where the additional arguments to
      :meth:`log_message` are applied as inputs to the formatting. The client
      IP address and current date and time are prefixed to every message logged.

   .. method:: version_string()

      Returns the server software's version string. This is a combination of the
      :attr:`server_version` and :attr:`sys_version` attributes.

   .. method:: date_time_string(timestamp=None)

      Returns the date and time given by *timestamp* (which must be ``None`` or in
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


.. class:: SimpleHTTPRequestHandler(request, client_address, server, directory=None)

   This class serves files from the directory specified by *directory* and below,
   or the current directory and below if *directory* is not specified, directly
   mapping URI structure to directory structure.

   .. versionadded:: 3.7
      The *directory* parameter.

   .. versionchanged:: 3.9
      The *directory* parameter accepts a :term:`path-like object`.

   A lot of the work, such as parsing the request, is done by the base class
   :class:`BaseHTTPRequestHandler`.  This class implements the :func:`do_GET`
   and :func:`do_HEAD` functions.

   :class:`SimpleHTTPRequestHandler` defines the following class variables:

   .. attribute:: server_version

      Specifies the server software version based on ``__version__`` defined
      at the module level. For example, ``'SimpleHTTP/0.2'``.

   .. attribute:: extensions_map

      A dictionary mapping suffixes into MIME types, contains custom overrides
      for the default system mappings. The mapping is used case-insensitively,
      and so should contain only lower-cased keys.

      .. versionchanged:: 3.9
         This dictionary is no longer filled with the default system mappings,
         but only contains overrides.

   :class:`SimpleHTTPRequestHandler` defines the following methods:

   .. method:: do_HEAD()

      Serves HEAD requests: it sends the headers it
      would send for the equivalent GET request. See the :meth:`do_GET`
      method for a more complete explanation of the possible headers.

   .. method:: do_GET()

      Serves GET requests: it sends a local file by interpreting the request as a
      path relative to the current working directory.

      If the request was mapped to a directory, the directory is checked for a
      file named ``index.html`` or ``index.htm`` (in that order). If found, the
      file's contents are returned; otherwise a directory listing is generated
      by calling the :meth:`list_directory` method. This method uses
      :func:`os.listdir` to scan the directory, and returns a 404 (Not Found)
      client error response if the :func:`~os.listdir` fails.

      If the request was mapped to a file, it is opened. For any :exc:`OSError`
      exception in opening the requested file, a 404 (Not Found) client error
      response is sent. If there was a If-Modified-Since
      header in the request, and the file was not modified after this time,
      a 304 (Not Modified) redirection response is sent. Otherwise, the content
      type is guessed by calling the :meth:`guess_type` method, which in turn
      uses the *extensions_map* variable, and the file contents are returned.

      A Content-Type header with the guessed content type is output,
      followed by a Content-Length header with the file's size and a
      Last-Modified header with the file's modification time.

      Then follows a blank line signifying the end of the headers, and then the
      contents of the file are output. If the file's MIME type starts with
      ``text/`` the file is opened in text mode; otherwise binary mode is used.

      For example usage, see the implementation of the :func:`test` function
      invocation in the :mod:`http.server` module.

      .. versionchanged:: 3.7
         Support of the If-Modified-Since header.

The :class:`SimpleHTTPRequestHandler` class can be used in the following
manner in order to create a very basic webserver serving files relative to
the current directory::

   import http.server
   import socketserver

   PORT = 8000

   Handler = http.server.SimpleHTTPRequestHandler

   with socketserver.TCPServer(("", PORT), Handler) as httpd:
       print("serving at port", PORT)
       httpd.serve_forever()

.. _http-server-cli:

:mod:`http.server` can also be invoked directly using the :option:`-m`
switch of the interpreter with a ``port number`` argument.  Similar to
the previous example, this serves files relative to the current directory::

        python -m http.server 8000

By default, server binds itself to all interfaces.  The option ``-b/--bind``
specifies a specific address to which it should bind. Both IPv4 and IPv6
addresses are supported. For example, the following command causes the server
to bind to localhost only::

        python -m http.server 8000 --bind 127.0.0.1

.. versionadded:: 3.4
    ``--bind`` argument was introduced.

.. versionadded:: 3.8
    ``--bind`` argument enhanced to support IPv6.

By default, server uses the current directory. The option ``-d/--directory``
specifies a directory to which it should serve the files. For example,
the following command uses a specific directory::

        python -m http.server --directory /tmp/

.. versionadded:: 3.7
    ``--directory`` specifies alternate directory.

.. class:: CGIHTTPRequestHandler(request, client_address, server, directory=None)

   This class serves files or runs and serves the output of CGI scripts from the directory
   specified by *directory* and below, or the current directory and below if
   *directory* is not specified, directly mapping URI structure to directory structure.

   .. note::

      CGI scripts run by the :class:`CGIHTTPRequestHandler` class cannot
      send a 302 (Found) redirection status code, because a 200 (OK) successful status code is
      sent prior to execution of the CGI script.  This pre-empts the status
      code.

   The class will however, run the CGI script, instead of serving it as a file,
   if it guesses it to be a CGI script.  Only directory-based CGI are used ---
   the other common server configuration is to treat special extensions as
   denoting CGI scripts.

   The :func:`do_GET` and :func:`do_HEAD` functions inherited from
   :class:`SimpleHTTPRequestHandler` are extended to run and serve the output
   of CGI scripts, instead of serving files, if the request path points to
   a file below the ``cgi_directories`` path.

   :class:`CGIHTTPRequestHandler` defines the following class variable:

   .. attribute:: cgi_directories

      This defaults to ``['/cgi-bin', '/htbin']`` and describes directories to
      treat as containing CGI scripts.

   :class:`CGIHTTPRequestHandler` defines the following method:

   .. method:: do_POST()

      Serves POST requests and is only allowed for CGI scripts.
      A 501 (Not Implemented) server error response is sent when trying
      to POST to a non-CGI URI.

   Note that CGI scripts will be run with UID of user nobody, for security
   reasons.  Problems with the CGI script will be translated to a 403 (Forbidden) client error response.

:class:`CGIHTTPRequestHandler` can be enabled in the command line by passing
the ``--cgi`` option::

        python -m http.server --cgi 8000
