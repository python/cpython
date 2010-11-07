:mod:`urllib.parse` --- Parse URLs into components
==================================================

.. module:: urllib.parse
   :synopsis: Parse URLs into or assemble them from components.


.. index::
   single: WWW
   single: World Wide Web
   single: URL
   pair: URL; parsing
   pair: relative; URL

This module defines a standard interface to break Uniform Resource Locator (URL)
strings up in components (addressing scheme, network location, path etc.), to
combine the components back into a URL string, and to convert a "relative URL"
to an absolute URL given a "base URL."

The module has been designed to match the Internet RFC on Relative Uniform
Resource Locators (and discovered a bug in an earlier draft!). It supports the
following URL schemes: ``file``, ``ftp``, ``gopher``, ``hdl``, ``http``,
``https``, ``imap``, ``mailto``, ``mms``, ``news``, ``nntp``, ``prospero``,
``rsync``, ``rtsp``, ``rtspu``, ``sftp``, ``shttp``, ``sip``, ``sips``,
``snews``, ``svn``, ``svn+ssh``, ``telnet``, ``wais``.

The :mod:`urllib.parse` module defines the following functions:

.. function:: urlparse(urlstring, scheme='', allow_fragments=True)

   Parse a URL into six components, returning a 6-tuple.  This corresponds to the
   general structure of a URL: ``scheme://netloc/path;parameters?query#fragment``.
   Each tuple item is a string, possibly empty. The components are not broken up in
   smaller parts (for example, the network location is a single string), and %
   escapes are not expanded. The delimiters as shown above are not part of the
   result, except for a leading slash in the *path* component, which is retained if
   present.  For example:

      >>> from urllib.parse import urlparse
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
       >>> urlparse('www.cwi.nl:80/%7Eguido/Python.html')
       ParseResult(scheme='', netloc='', path='www.cwi.nl:80/%7Eguido/Python.html',
                  params='', query='', fragment='')
       >>> urlparse('help/Python.html')
       ParseResult(scheme='', netloc='', path='help/Python.html', params='',
                  query='', fragment='')

   If the *scheme* argument is specified, it gives the default addressing
   scheme, to be used only if the URL does not specify one.  The default value for
   this argument is the empty string.

   If the *allow_fragments* argument is false, fragment identifiers are not
   allowed, even if the URL's addressing scheme normally does support them.  The
   default value for this argument is :const:`True`.

   The return value is actually an instance of a subclass of :class:`tuple`.  This
   class has the following additional read-only convenience attributes:

   +------------------+-------+--------------------------+----------------------+
   | Attribute        | Index | Value                    | Value if not present |
   +==================+=======+==========================+======================+
   | :attr:`scheme`   | 0     | URL scheme specifier     | empty string         |
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

   .. versionchanged:: 3.2
      Added IPv6 URL parsing capabilities.


.. function:: parse_qs(qs, keep_blank_values=False, strict_parsing=False)

   Parse a query string given as a string argument (data of type
   :mimetype:`application/x-www-form-urlencoded`).  Data are returned as a
   dictionary.  The dictionary keys are the unique query variable names and the
   values are lists of values for each name.

   The optional argument *keep_blank_values* is a flag indicating whether blank
   values in percent-encoded queries should be treated as blank strings. A true value
   indicates that blanks should be retained as  blank strings.  The default false
   value indicates that blank values are to be ignored and treated as if they were
   not included.

   The optional argument *strict_parsing* is a flag indicating what to do with
   parsing errors.  If false (the default), errors are silently ignored.  If true,
   errors raise a :exc:`ValueError` exception.

   Use the :func:`urllib.parse.urlencode` function to convert such
   dictionaries into query strings.


.. function:: parse_qsl(qs, keep_blank_values=False, strict_parsing=False)

   Parse a query string given as a string argument (data of type
   :mimetype:`application/x-www-form-urlencoded`).  Data are returned as a list of
   name, value pairs.

   The optional argument *keep_blank_values* is a flag indicating whether blank
   values in percent-encoded queries should be treated as blank strings. A true value
   indicates that blanks should be retained as  blank strings.  The default false
   value indicates that blank values are to be ignored and treated as if they were
   not included.

   The optional argument *strict_parsing* is a flag indicating what to do with
   parsing errors.  If false (the default), errors are silently ignored.  If true,
   errors raise a :exc:`ValueError` exception.

   Use the :func:`urllib.parse.urlencode` function to convert such lists of pairs into
   query strings.


.. function:: urlunparse(parts)

   Construct a URL from a tuple as returned by ``urlparse()``. The *parts*
   argument can be any six-item iterable. This may result in a slightly
   different, but equivalent URL, if the URL that was parsed originally had
   unnecessary delimiters (for example, a ``?`` with an empty query; the RFC
   states that these are equivalent).


.. function:: urlsplit(urlstring, scheme='', allow_fragments=True)

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
   | :attr:`scheme`   | 0     | URL scheme specifier    | empty string         |
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


.. function:: urlunsplit(parts)

   Combine the elements of a tuple as returned by :func:`urlsplit` into a
   complete URL as a string. The *parts* argument can be any five-item
   iterable. This may result in a slightly different, but equivalent URL, if the
   URL that was parsed originally had unnecessary delimiters (for example, a ?
   with an empty query; the RFC states that these are equivalent).


.. function:: urljoin(base, url, allow_fragments=True)

   Construct a full ("absolute") URL by combining a "base URL" (*base*) with
   another URL (*url*).  Informally, this uses components of the base URL, in
   particular the addressing scheme, the network location and (part of) the
   path, to provide missing components in the relative URL.  For example:

      >>> from urllib.parse import urljoin
      >>> urljoin('http://www.cwi.nl/%7Eguido/Python.html', 'FAQ.html')
      'http://www.cwi.nl/%7Eguido/FAQ.html'

   The *allow_fragments* argument has the same meaning and default as for
   :func:`urlparse`.

   .. note::

      If *url* is an absolute URL (that is, starting with ``//`` or ``scheme://``),
      the *url*'s host name and/or scheme will be present in the result.  For example:

   .. doctest::

      >>> urljoin('http://www.cwi.nl/%7Eguido/Python.html',
      ...         '//www.python.org/%7Eguido')
      'http://www.python.org/%7Eguido'

   If you do not want that behavior, preprocess the *url* with :func:`urlsplit` and
   :func:`urlunsplit`, removing possible *scheme* and *netloc* parts.


.. function:: urldefrag(url)

   If *url* contains a fragment identifier, return a modified version of *url*
   with no fragment identifier, and the fragment identifier as a separate
   string.  If there is no fragment identifier in *url*, return *url* unmodified
   and an empty string.


.. function:: quote(string, safe='/', encoding=None, errors=None)

   Replace special characters in *string* using the ``%xx`` escape. Letters,
   digits, and the characters ``'_.-'`` are never quoted. By default, this
   function is intended for quoting the path section of URL. The optional *safe*
   parameter specifies additional ASCII characters that should not be quoted
   --- its default value is ``'/'``.

   *string* may be either a :class:`str` or a :class:`bytes`.

   The optional *encoding* and *errors* parameters specify how to deal with
   non-ASCII characters, as accepted by the :meth:`str.encode` method.
   *encoding* defaults to ``'utf-8'``.
   *errors* defaults to ``'strict'``, meaning unsupported characters raise a
   :class:`UnicodeEncodeError`.
   *encoding* and *errors* must not be supplied if *string* is a
   :class:`bytes`, or a :class:`TypeError` is raised.

   Note that ``quote(string, safe, encoding, errors)`` is equivalent to
   ``quote_from_bytes(string.encode(encoding, errors), safe)``.

   Example: ``quote('/El Niño/')`` yields ``'/El%20Ni%C3%B1o/'``.


.. function:: quote_plus(string, safe='', encoding=None, errors=None)

   Like :func:`quote`, but also replace spaces by plus signs, as required for
   quoting HTML form values when building up a query string to go into a URL.
   Plus signs in the original string are escaped unless they are included in
   *safe*.  It also does not have *safe* default to ``'/'``.

   Example: ``quote_plus('/El Niño/')`` yields ``'%2FEl+Ni%C3%B1o%2F'``.


.. function:: quote_from_bytes(bytes, safe='/')

   Like :func:`quote`, but accepts a :class:`bytes` object rather than a
   :class:`str`, and does not perform string-to-bytes encoding.

   Example: ``quote_from_bytes(b'a&\xef')`` yields
   ``'a%26%EF'``.


.. function:: unquote(string, encoding='utf-8', errors='replace')

   Replace ``%xx`` escapes by their single-character equivalent.
   The optional *encoding* and *errors* parameters specify how to decode
   percent-encoded sequences into Unicode characters, as accepted by the
   :meth:`bytes.decode` method.

   *string* must be a :class:`str`.

   *encoding* defaults to ``'utf-8'``.
   *errors* defaults to ``'replace'``, meaning invalid sequences are replaced
   by a placeholder character.

   Example: ``unquote('/El%20Ni%C3%B1o/')`` yields ``'/El Niño/'``.


.. function:: unquote_plus(string, encoding='utf-8', errors='replace')

   Like :func:`unquote`, but also replace plus signs by spaces, as required for
   unquoting HTML form values.

   *string* must be a :class:`str`.

   Example: ``unquote_plus('/El+Ni%C3%B1o/')`` yields ``'/El Niño/'``.


.. function:: unquote_to_bytes(string)

   Replace ``%xx`` escapes by their single-octet equivalent, and return a
   :class:`bytes` object.

   *string* may be either a :class:`str` or a :class:`bytes`.

   If it is a :class:`str`, unescaped non-ASCII characters in *string*
   are encoded into UTF-8 bytes.

   Example: ``unquote_to_bytes('a%26%EF')`` yields
   ``b'a&\xef'``.


.. function:: urlencode(query, doseq=False, safe='', encoding=None, errors=None)

   Convert a mapping object or a sequence of two-element tuples, which may
   either be a :class:`str` or a :class:`bytes`,  to a "percent-encoded" string,
   suitable to pass to :func:`urlopen` above as the optional *data* argument.
   This is useful to pass a dictionary of form fields to a ``POST`` request.
   The resulting string is a series of ``key=value`` pairs separated by ``'&'``
   characters, where both *key* and *value* are quoted using :func:`quote_plus`
   above. When a sequence of two-element tuples is used as the *query*
   argument, the first element of each tuple is a key and the second is a
   value. The value element in itself can be a sequence and in that case, if
   the optional parameter *doseq* is evaluates to *True*, individual
   ``key=value`` pairs separated by ``'&'`` are generated for each element of
   the value sequence for the key.  The order of parameters in the encoded
   string will match the order of parameter tuples in the sequence. This module
   provides the functions :func:`parse_qs` and :func:`parse_qsl` which are used
   to parse query strings into Python data structures.

   When *query* parameter is a :class:`str`, the *safe*, *encoding* and *error*
   parameters are sent the :func:`quote_plus` for encoding.

   .. versionchanged:: 3.2
      Query parameter supports bytes and string objects.


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

      >>> import urllib.parse
      >>> url = 'HTTP://www.Python.org/doc/#'

      >>> r1 = urllib.parse.urlsplit(url)
      >>> r1.geturl()
      'http://www.Python.org/doc/'

      >>> r2 = urllib.parse.urlsplit(r1.geturl())
      >>> r2.geturl()
      'http://www.Python.org/doc/'


The following classes provide the implementations of the parse results:

.. class:: BaseResult

   Base class for the concrete result classes.  This provides most of the
   attribute definitions.  It does not provide a :meth:`geturl` method.  It is
   derived from :class:`tuple`, but does not override the :meth:`__init__` or
   :meth:`__new__` methods.


.. class:: ParseResult(scheme, netloc, path, params, query, fragment)

   Concrete class for :func:`urlparse` results.  The :meth:`__new__` method is
   overridden to support checking that the right number of arguments are passed.


.. class:: SplitResult(scheme, netloc, path, query, fragment)

   Concrete class for :func:`urlsplit` results.  The :meth:`__new__` method is
   overridden to support checking that the right number of arguments are passed.

