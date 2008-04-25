
:mod:`CGIHTTPServer` --- CGI-capable HTTP request handler
=========================================================

.. module:: CGIHTTPServer
   :synopsis: This module provides a request handler for HTTP servers which can run CGI
              scripts.
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`CGIHTTPServer` module defines a request-handler class, interface
compatible with :class:`BaseHTTPServer.BaseHTTPRequestHandler` and inherits
behavior from :class:`SimpleHTTPServer.SimpleHTTPRequestHandler` but can also
run CGI scripts.

.. note::

   This module can run CGI scripts on Unix and Windows systems; on Mac OS it will
   only be able to run Python scripts within the same process as itself.

.. note::

   CGI scripts run by the :class:`CGIHTTPRequestHandler` class cannot execute
   redirects (HTTP code 302), because code 200 (script output follows) is sent
   prior to execution of the CGI script.  This pre-empts the status code.

The :mod:`CGIHTTPServer` module defines the following class:


.. class:: CGIHTTPRequestHandler(request, client_address, server)

   This class is used to serve either files or output of CGI scripts from  the
   current directory and below. Note that mapping HTTP hierarchic structure to
   local directory structure is exactly as in
   :class:`SimpleHTTPServer.SimpleHTTPRequestHandler`.

   The class will however, run the CGI script, instead of serving it as a file, if
   it guesses it to be a CGI script. Only directory-based CGI are used --- the
   other common server configuration is to treat special extensions as denoting CGI
   scripts.

   The :func:`do_GET` and :func:`do_HEAD` functions are modified to run CGI scripts
   and serve the output, instead of serving files, if the request leads to
   somewhere below the ``cgi_directories`` path.

   The :class:`CGIHTTPRequestHandler` defines the following data member:


   .. attribute:: cgi_directories

      This defaults to ``['/cgi-bin', '/htbin']`` and describes directories to
      treat as containing CGI scripts.

   The :class:`CGIHTTPRequestHandler` defines the following methods:


   .. method:: do_POST()

      This method serves the ``'POST'`` request type, only allowed for CGI
      scripts.  Error 501, "Can only POST to CGI scripts", is output when trying
      to POST to a non-CGI url.

Note that CGI scripts will be run with UID of user nobody, for security reasons.
Problems with the CGI script will be translated to error 403.

For example usage, see the implementation of the :func:`test` function.


.. seealso::

   Module :mod:`BaseHTTPServer`
      Base class implementation for Web server and request handler.

