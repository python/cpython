:mod:`urlparse` --- Parse URLs into components
==============================================

.. module:: urlparse
   :synopsis: Parse URLs into or assemble them from components.


.. index::
   single: WWW
   single: World Wide Web
   single: URL
   pair: URL; parsing
   pair: relative; URL

.. note::
   The :mod:`urlparse` module is renamed to :mod:`urllib.parse` in Python 3.
   The :term:`2to3` tool will automatically adapt imports when converting
   your sources to Python 3.

**Source code:** :source:`Lib/urlparse.py`

--------------

This module defines a standard interface to break Uniform Resource Locator (URL)
strings up in components (addressing scheme, network location, path etc.), to
combine the components back into a URL string, and to convert a "relative URL"
to an absolute URL given a "base URL."

The module has been designed to match the Internet RFC on Relative Uniform
Resource Locators. It supports the following URL schemes: ``file``, ``ftp``,
``gopher``, ``hdl``, ``http``, ``https``, ``imap``, ``mailto``, ``mms``,
``news``,  ``nntp``, ``prospero``, ``rsync``, ``rtsp``, ``rtspu``,  ``sftp``,
``shttp``, ``sip``, ``sips``, ``snews``, ``svn``,  ``svn+ssh``, ``telnet``,
``wais``.

.. versionadded:: 2.5
   Support for the ``sftp`` and ``sips`` schemes.

The :mod:`urlparse` module defines the following functions:


.. function:: urlparse(urlstring[, scheme[, allow_fragments]])

   Parse a URL into six components, returning a 6-tuple.  This corresponds to the
   general structure of a URL: ``scheme://netloc/path;parameters?query#fragment``.
   Each tuple item is a string, possibly empty. The components are not broken up in
   smaller parts (for example, the network location is a single string), and %
   escapes are not expanded. The delimiters as shown above are not part of the
   result, except for a leading slash in the *path* component, which is retained if
   present.  For example:

      >>> from urlparse import urlparse
      >>> o = urlparse('http://www.cwi.nl:80/%7Eguido/Python.html')
      >>> o   # doctest: +NORMALIZE_WHITESPACE
      ParseResult(scheme='http', netloc='www.cwi.nl:80', path='/%7Eguido/Python.html',
                  params='', query='', fragment='')
      >>> o.scheme
      'http'
      >>> o.port
      80
      >>> o.geturl()
      'http://www.cwi.nl:80/%7Eguido/Python.html'


   Following the syntax specifications in :rfc:`1808`, urlparse recognizes
   a netloc only if it is properly introduced by '//'.  Otherwise the
   input is presumed to be a relative URL and thus to start with
   a path component.

       >>> from urlparse import urlparse
       >>> urlparse('//www.cwi.nl:80/%7Eguido/Python.html')
       ParseResult(scheme='', netloc='www.cwi.nl:80', path='/%7Eguido/Python.html',
                  params='', query='', fragment='')
       >>> urlparse('www.cwi.nl/%7Eguido/Python.html')
       ParseResult(scheme='', netloc='', path='www.cwi.nl/%7Eguido/Python.html',
                  params='', query='', fragment='')
       >>> urlparse('help/Python.html')
       ParseResult(scheme='', netloc='', path='help/Python.html', params='',
                  query='', fragment='')

   If the *scheme* argument is specified, it gives the default addressing
   scheme, to be used only if the URL does not specify one.  The default value for
   this argument is the empty string.

   If the *allow_fragments* argument is false, fragment identifiers are not
   recognized and parsed as part of the preceding component, even if the URL's
   addressing scheme normally does support them.  The default value for this
   argument is :const:`True`.

   The return value is actually an instance of a subclass of :class:`tuple`.  This
   class has the following additional read-only convenience attributes:

   +------------------+-------+--------------------------+----------------------+
   | Attribute        | Index | Value                    | Value if not present |
   +==================+=======+==========================+======================+
   | :attr:`scheme`   | 0     | URL scheme specifier     | *scheme* parameter   |
   +------------------+-------+--------------------------+----------------------+
   | :attr:`netloc`   | 1     | Network location part    | empty string         |
   +------------------+-------+--------------------------+----------------------+
   | :attr:`path`     | 2     | Hierarchical path        | empty string         |
   +------------------+-------+--------------------------+----------------------+
   | :attr:`params`   | 3     | Parameters for last path | empty string         |
   |                  |       | element                  |                      |
   +------------------+-------+--------------------------+----------------------+
   | :attr:`query`    | 4     | Query component          | empty string         |
   +------------------+-------+--------------------------+----------------------+
   | :attr:`fragment` | 5     | Fragment identifier      | empty string         |
   +------------------+-------+--------------------------+----------------------+
   | :attr:`username` |       | User name                | :const:`None`        |
   +------------------+-------+--------------------------+----------------------+
   | :attr:`password` |       | Password                 | :const:`None`        |
   +------------------+-------+--------------------------+----------------------+
   | :attr:`hostname` |       | Host name (lower case)   | :const:`None`        |
   +------------------+-------+--------------------------+----------------------+
   | :attr:`port`     |       | Port number as integer,  | :const:`None`        |
   |                  |       | if present               |                      |
   +------------------+-------+--------------------------+----------------------+

   See section :ref:`urlparse-result-object` for more information on the result
   object.

   .. versionchanged:: 2.5
      Added attributes to return value.

   .. versionchanged:: 2.7
      Added IPv6 URL parsing capabilities.


.. function:: parse_qs(qs[, keep_blank_values[, strict_parsing]])

   Parse a query string given as a string argument (data of type
   :mimetype:`application/x-www-form-urlencoded`).  Data are returned as a
   dictionary.  The dictionary keys are the unique query variable names and the
   values are lists of values for each name.

   The optional argument *keep_blank_values* is a flag indicating whether blank
   values in percent-encoded queries should be treated as blank strings.   A true value
   indicates that blanks should be retained as  blank strings.  The default false
   value indicates that blank values are to be ignored and treated as if they were
   not included.

   The optional argument *strict_parsing* is a flag indicating what to do with
   parsing errors.  If false (the default), errors are silently ignored.  If true,
   errors raise a :exc:`ValueError` exception.

   Use the :func:`urllib.urlencode` function to convert such dictionaries into
   query strings.

   .. versionadded:: 2.6
      Copied from the :mod:`cgi` module.


.. function:: parse_qsl(qs[, keep_blank_values[, strict_parsing]])

   Parse a query string given as a string argument (data of type
   :mimetype:`application/x-www-form-urlencoded`).  Data are returned as a list of
   name, value pairs.

   The optional argument *keep_blank_values* is a flag indicating whether blank
   values in percent-encoded queries should be treated as blank strings.   A true value
   indicates that blanks should be retained as  blank strings.  The default false
   value indicates that blank values are to be ignored and treated as if they were
   not included.

   The optional argument *strict_parsing* is a flag indicating what to do with
   parsing errors.  If false (the default), errors are silently ignored.  If true,
   errors raise a :exc:`ValueError` exception.

   Use the :func:`urllib.urlencode` function to convert such lists of pairs into
   query strings.

   .. versionadded:: 2.6
      Copied from the :mod:`cgi` module.


.. function:: urlunparse(parts)

   Construct a URL from a tuple as returned by ``urlparse()``. The *parts* argument
   can be any six-item iterable. This may result in a slightly different, but
   equivalent URL, if the URL that was parsed originally had unnecessary delimiters
   (for example, a ? with an empty query; the RFC states that these are
   equivalent).


.. function:: urlsplit(urlstring[, scheme[, allow_fragments]])

   This is similar to :func:`urlparse`, but does not split the params from the URL.
   This should generally be used instead of :func:`urlparse` if the more recent URL
   syntax allowing parameters to be applied to each segment of the *path* portion
   of the URL (see :rfc:`2396`) is wanted.  A separate function is needed to
   separate the path segments and parameters.  This function returns a 5-tuple:
   (addressing scheme, network location, path, query, fragment identifier).

   The return value is actually an instance of a subclass of :class:`tuple`.  This
   class has the following additional read-only convenience attributes:

   +------------------+-------+-------------------------+----------------------+
   | Attribute        | Index | Value                   | Value if not present |
   +==================+=======+=========================+======================+
   | :attr:`scheme`   | 0     | URL scheme specifier    | *scheme* parameter   |
   +------------------+-------+-------------------------+----------------------+
   | :attr:`netloc`   | 1     | Network location part   | empty string         |
   +------------------+-------+-------------------------+----------------------+
   | :attr:`path`     | 2     | Hierarchical path       | empty string         |
   +------------------+-------+-------------------------+----------------------+
   | :attr:`query`    | 3     | Query component         | empty string         |
   +------------------+-------+-------------------------+----------------------+
   | :attr:`fragment` | 4     | Fragment identifier     | empty string         |
   +------------------+-------+-------------------------+----------------------+
   | :attr:`username` |       | User name               | :const:`None`        |
   +------------------+-------+-------------------------+----------------------+
   | :attr:`password` |       | Password                | :const:`None`        |
   +------------------+-------+-------------------------+----------------------+
   | :attr:`hostname` |       | Host name (lower case)  | :const:`None`        |
   +------------------+-------+-------------------------+----------------------+
   | :attr:`port`     |       | Port number as integer, | :const:`None`        |
   |                  |       | if present              |                      |
   +------------------+-------+-------------------------+----------------------+

   See section :ref:`urlparse-result-object` for more information on the result
   object.

   .. versionadded:: 2.2

   .. versionchanged:: 2.5
      Added attributes to return value.


.. function:: urlunsplit(parts)

   Combine the elements of a tuple as returned by :func:`urlsplit` into a complete
   URL as a string. The *parts* argument can be any five-item iterable. This may
   result in a slightly different, but equivalent URL, if the URL that was parsed
   originally had unnecessary delimiters (for example, a ? with an empty query; the
   RFC states that these are equivalent).

   .. versionadded:: 2.2


.. function:: urljoin(base, url[, allow_fragments])

   Construct a full ("absolute") URL by combining a "base URL" (*base*) with
   another URL (*url*).  Informally, this uses components of the base URL, in
   particular the addressing scheme, the network location and (part of) the path,
   to provide missing components in the relative URL.  For example:

      >>> from urlparse import urljoin
      >>> urljoin('http://www.cwi.nl/%7Eguido/Python.html', 'FAQ.html')
      'http://www.cwi.nl/%7Eguido/FAQ.html'

   The *allow_fragments* argument has the same meaning and default as for
   :func:`urlparse`.

   .. note::

      If *url* is an absolute URL (that is, starting with ``//`` or ``scheme://``),
      the *url*'s host name and/or scheme will be present in the result.  For example:

   .. doctest::

      >>> urljoin('https://www.cwi.nl/%7Eguido/Python.html',
      ...         '//www.python.org/%7Eguido')
      'http://www.python.org/%7Eguido'

   If you do not want that behavior, preprocess the *url* with :func:`urlsplit` and
   :func:`urlunsplit`, removing possible *scheme* and *netloc* parts.


.. function:: urldefrag(url)

   If *url* contains a fragment identifier, returns a modified version of *url*
   with no fragment identifier, and the fragment identifier as a separate string.
   If there is no fragment identifier in *url*, returns *url* unmodified and an
   empty string.


.. seealso::

   :rfc:`3986` - Uniform Resource Identifiers
      This is the current standard (STD66). Any changes to urlparse module
      should conform to this. Certain deviations could be observed, which are
      mostly for backward compatibility purposes and for certain de-facto
      parsing requirements as commonly observed in major browsers.

   :rfc:`2732` - Format for Literal IPv6 Addresses in URL's.
      This specifies the parsing requirements of IPv6 URLs.

   :rfc:`2396` - Uniform Resource Identifiers (URI): Generic Syntax
      Document describing the generic syntactic requirements for both Uniform Resource
      Names (URNs) and Uniform Resource Locators (URLs).

   :rfc:`2368` - The mailto URL scheme.
      Parsing requirements for mailto url schemes.

   :rfc:`1808` - Relative Uniform Resource Locators
      This Request For Comments includes the rules for joining an absolute and a
      relative URL, including a fair number of "Abnormal Examples" which govern the
      treatment of border cases.

   :rfc:`1738` - Uniform Resource Locators (URL)
      This specifies the formal syntax and semantics of absolute URLs.


.. _urlparse-result-object:

Results of :func:`urlparse` and :func:`urlsplit`
------------------------------------------------

The result objects from the :func:`urlparse` and :func:`urlsplit` functions are
subclasses of the :class:`tuple` type.  These subclasses add the attributes
described in those functions, as well as provide an additional method:


.. method:: ParseResult.geturl()

   Return the re-combined version of the original URL as a string. This may differ
   from the original URL in that the scheme will always be normalized to lower case
   and empty components may be dropped. Specifically, empty parameters, queries,
   and fragment identifiers will be removed.

   The result of this method is a fixpoint if passed back through the original
   parsing function:

      >>> import urlparse
      >>> url = 'HTTP://www.Python.org/doc/#'

      >>> r1 = urlparse.urlsplit(url)
      >>> r1.geturl()
      'http://www.Python.org/doc/'

      >>> r2 = urlparse.urlsplit(r1.geturl())
      >>> r2.geturl()
      'http://www.Python.org/doc/'

   .. versionadded:: 2.5

The following classes provide the implementations of the parse results:


.. class:: ParseResult(scheme, netloc, path, params, query, fragment)

   Concrete class for :func:`urlparse` results.


.. class:: SplitResult(scheme, netloc, path, query, fragment)

   Concrete class for :func:`urlsplit` results.

