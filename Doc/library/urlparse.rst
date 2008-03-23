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

This module defines a standard interface to break Uniform Resource Locator (URL)
strings up in components (addressing scheme, network location, path etc.), to
combine the components back into a URL string, and to convert a "relative URL"
to an absolute URL given a "base URL."

The module has been designed to match the Internet RFC on Relative Uniform
Resource Locators (and discovered a bug in an earlier draft!). It supports the
following URL schemes: ``file``, ``ftp``, ``gopher``, ``hdl``, ``http``,
``https``, ``imap``, ``mailto``, ``mms``, ``news``,  ``nntp``, ``prospero``,
``rsync``, ``rtsp``, ``rtspu``,  ``sftp``, ``shttp``, ``sip``, ``sips``,
``snews``, ``svn``,  ``svn+ssh``, ``telnet``, ``wais``.

The :mod:`urlparse` module defines the following functions:


.. function:: urlparse(urlstring[, default_scheme[, allow_fragments]])

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

   If the *default_scheme* argument is specified, it gives the default addressing
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


.. function:: urlunparse(parts)

   Construct a URL from a tuple as returned by ``urlparse()``. The *parts* argument
   can be any six-item iterable. This may result in a slightly different, but
   equivalent URL, if the URL that was parsed originally had unnecessary delimiters
   (for example, a ? with an empty query; the RFC states that these are
   equivalent).


.. function:: urlsplit(urlstring[, default_scheme[, allow_fragments]])

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

   Combine the elements of a tuple as returned by :func:`urlsplit` into a complete
   URL as a string. The *parts* argument can be any five-item iterable. This may
   result in a slightly different, but equivalent URL, if the URL that was parsed
   originally had unnecessary delimiters (for example, a ? with an empty query; the
   RFC states that these are equivalent).


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

      >>> urljoin('http://www.cwi.nl/%7Eguido/Python.html',
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

   :rfc:`1738` - Uniform Resource Locators (URL)
      This specifies the formal syntax and semantics of absolute URLs.

   :rfc:`1808` - Relative Uniform Resource Locators
      This Request For Comments includes the rules for joining an absolute and a
      relative URL, including a fair number of "Abnormal Examples" which govern the
      treatment of border cases.

   :rfc:`2396` - Uniform Resource Identifiers (URI): Generic Syntax
      Document describing the generic syntactic requirements for both Uniform Resource
      Names (URNs) and Uniform Resource Locators (URLs).


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


The following classes provide the implementations of the parse results::


.. class:: BaseResult

   Base class for the concrete result classes.  This provides most of the attribute
   definitions.  It does not provide a :meth:`geturl` method.  It is derived from
   :class:`tuple`, but does not override the :meth:`__init__` or :meth:`__new__`
   methods.


.. class:: ParseResult(scheme, netloc, path, params, query, fragment)

   Concrete class for :func:`urlparse` results.  The :meth:`__new__` method is
   overridden to support checking that the right number of arguments are passed.


.. class:: SplitResult(scheme, netloc, path, query, fragment)

   Concrete class for :func:`urlsplit` results.  The :meth:`__new__` method is
   overridden to support checking that the right number of arguments are passed.

