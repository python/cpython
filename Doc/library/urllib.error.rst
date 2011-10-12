:mod:`urllib.error` --- Exception classes raised by urllib.request
==================================================================

.. module:: urllib.error
   :synopsis: Exception classes raised by urllib.request.
.. moduleauthor:: Jeremy Hylton <jeremy@alum.mit.edu>
.. sectionauthor:: Senthil Kumaran <orsenthil@gmail.com>


The :mod:`urllib.error` module defines the exception classes for exceptions
raised by :mod:`urllib.request`.  The base exception class is :exc:`URLError`,
which inherits from :exc:`IOError`.

The following exceptions are raised by :mod:`urllib.error` as appropriate:

.. exception:: URLError

   The handlers raise this exception (or derived exceptions) when they run into
   a problem.  It is a subclass of :exc:`IOError`.

   .. attribute:: reason

      The reason for this error.  It can be a message string or another
      exception instance such as :exc:`OSError`.


.. exception:: HTTPError

   Though being an exception (a subclass of :exc:`URLError`), an
   :exc:`HTTPError` can also function as a non-exceptional file-like return
   value (the same thing that :func:`urlopen` returns).  This is useful when
   handling exotic HTTP errors, such as requests for authentication.

   .. attribute:: code

      An HTTP status code as defined in `RFC 2616
      <http://www.faqs.org/rfcs/rfc2616.html>`_.  This numeric value corresponds
      to a value found in the dictionary of codes as found in
      :attr:`http.server.BaseHTTPRequestHandler.responses`.

.. exception:: ContentTooShortError(msg, content)

   This exception is raised when the :func:`urlretrieve` function detects that
   the amount of the downloaded data is less than the expected amount (given by
   the *Content-Length* header).  The :attr:`content` attribute stores the
   downloaded (and supposedly truncated) data.

