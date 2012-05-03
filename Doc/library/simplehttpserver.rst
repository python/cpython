
:mod:`SimpleHTTPServer` --- Simple HTTP request handler
=======================================================

.. module:: SimpleHTTPServer
   :synopsis: This module provides a basic request handler for HTTP servers.
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>

.. note::
   The :mod:`SimpleHTTPServer` module has been merged into :mod:`http.server` in
   Python 3.  The :term:`2to3` tool will automatically adapt imports when
   converting your sources to Python 3.


The :mod:`SimpleHTTPServer` module defines a single class,
:class:`SimpleHTTPRequestHandler`, which is interface-compatible with
:class:`BaseHTTPServer.BaseHTTPRequestHandler`.

The :mod:`SimpleHTTPServer` module defines the following class:


.. class:: SimpleHTTPRequestHandler(request, client_address, server)

   This class serves files from the current directory and below, directly
   mapping the directory structure to HTTP requests.

   A lot of the work, such as parsing the request, is done by the base class
   :class:`BaseHTTPServer.BaseHTTPRequestHandler`.  This class implements the
   :func:`do_GET` and :func:`do_HEAD` functions.

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
      response if the :func:`listdir` fails.

      If the request was mapped to a file, it is opened and the contents are
      returned.  Any :exc:`IOError` exception in opening the requested file is
      mapped to a ``404``, ``'File not found'`` error. Otherwise, the content
      type is guessed by calling the :meth:`guess_type` method, which in turn
      uses the *extensions_map* variable.

      A ``'Content-type:'`` header with the guessed content type is output,
      followed by a ``'Content-Length:'`` header with the file's size and a
      ``'Last-Modified:'`` header with the file's modification time.

      Then follows a blank line signifying the end of the headers, and then the
      contents of the file are output. If the file's MIME type starts with
      ``text/`` the file is opened in text mode; otherwise binary mode is used.

      The :func:`test` function in the :mod:`SimpleHTTPServer` module is an
      example which creates a server using the :class:`SimpleHTTPRequestHandler`
      as the Handler.

      .. versionadded:: 2.5
         The ``'Last-Modified'`` header.


The :mod:`SimpleHTTPServer` module can be used in the following manner in order
to set up a very basic web server serving files relative to the current
directory. ::

   import SimpleHTTPServer
   import SocketServer

   PORT = 8000

   Handler = SimpleHTTPServer.SimpleHTTPRequestHandler

   httpd = SocketServer.TCPServer(("", PORT), Handler)

   print "serving at port", PORT
   httpd.serve_forever()

The :mod:`SimpleHTTPServer` module can also be invoked directly using the
:option:`-m` switch of the interpreter with a ``port number`` argument.
Similar to the previous example, this serves the files relative to the
current directory. ::

   python -m SimpleHTTPServer 8000

.. seealso::

   Module :mod:`BaseHTTPServer`
      Base class implementation for Web server and request handler.

