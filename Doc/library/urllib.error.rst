:mod:`urllib.error` --- Exception classes raised by urllib.request
==================================================================

.. module:: urllib.error
   :synopsis: Exception classes raised by urllib.request.

.. moduleauthor:: Jeremy Hylton <jeremy@alum.mit.edu>
.. sectionauthor:: Senthil Kumaran <orsenthil@gmail.com>

**Source code:** :source:`Lib/urllib/error.py`

--------------

The :mod:`urllib.error` module defines the exception classes for exceptions
raised by :mod:`urllib.request`.  The base exception class is :exc:`URLError`.

The following exceptions are raised by :mod:`urllib.error` as appropriate:

.. exception:: URLError

   The handlers raise this exception (or derived exceptions) when they run into
   a problem.  It is a subclass of :exc:`OSError`.

   .. attribute:: reason

      The reason for this error.  It can be a message string or another
      exception instance.

   .. versionchanged:: 3.3
      :exc:`URLError` used to be a subtype of :exc:`IOError`, which is now an
      alias of :exc:`OSError`.


.. exception:: HTTPError

   Though being an exception (a subclass of :exc:`URLError`), an
   :exc:`HTTPError` can also function as a non-exceptional file-like return
   value (the same thing that :func:`~urllib.request.urlopen` returns).  This
   is useful when handling exotic HTTP errors, such as requests for
   authentication.

   .. attribute:: code

      An HTTP status code as defined in :rfc:`2616`.  This numeric value corresponds
      to a value found in the dictionary of codes as found in
      :attr:`http.server.BaseHTTPRequestHandler.responses`.

   .. attribute:: reason

      This is usually a string explaining the reason for this error.

   .. attribute:: headers

      The HTTP response headers for the HTTP request that caused the
      :exc:`HTTPError`.

      .. versionadded:: 3.4

.. exception:: ContentTooShortError(msg, content)

   This exception is raised when the :func:`~urllib.request.urlretrieve`
   function detects that
   the amount of the downloaded data is less than the expected amount (given by
   the *Content-Length* header).

   .. attribute:: content

      The downloaded (and supposedly truncated) data.
