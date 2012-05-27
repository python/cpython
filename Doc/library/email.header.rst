:mod:`email.header`: Internationalized headers
----------------------------------------------

.. module:: email.header
   :synopsis: Representing non-ASCII headers


:rfc:`2822` is the base standard that describes the format of email messages.
It derives from the older :rfc:`822` standard which came into widespread use at
a time when most email was composed of ASCII characters only.  :rfc:`2822` is a
specification written assuming email contains only 7-bit ASCII characters.

Of course, as email has been deployed worldwide, it has become
internationalized, such that language specific character sets can now be used in
email messages.  The base standard still requires email messages to be
transferred using only 7-bit ASCII characters, so a slew of RFCs have been
written describing how to encode email containing non-ASCII characters into
:rfc:`2822`\ -compliant format. These RFCs include :rfc:`2045`, :rfc:`2046`,
:rfc:`2047`, and :rfc:`2231`. The :mod:`email` package supports these standards
in its :mod:`email.header` and :mod:`email.charset` modules.

If you want to include non-ASCII characters in your email headers, say in the
:mailheader:`Subject` or :mailheader:`To` fields, you should use the
:class:`Header` class and assign the field in the :class:`~email.message.Message`
object to an instance of :class:`Header` instead of using a string for the header
value.  Import the :class:`Header` class from the :mod:`email.header` module.
For example::

   >>> from email.message import Message
   >>> from email.header import Header
   >>> msg = Message()
   >>> h = Header('p\xf6stal', 'iso-8859-1')
   >>> msg['Subject'] = h
   >>> print msg.as_string()
   Subject: =?iso-8859-1?q?p=F6stal?=



Notice here how we wanted the :mailheader:`Subject` field to contain a non-ASCII
character?  We did this by creating a :class:`Header` instance and passing in
the character set that the byte string was encoded in.  When the subsequent
:class:`~email.message.Message` instance was flattened, the :mailheader:`Subject`
field was properly :rfc:`2047` encoded.  MIME-aware mail readers would show this
header using the embedded ISO-8859-1 character.

.. versionadded:: 2.2.2

Here is the :class:`Header` class description:


.. class:: Header([s[, charset[, maxlinelen[, header_name[, continuation_ws[, errors]]]]]])

   Create a MIME-compliant header that can contain strings in different character
   sets.

   Optional *s* is the initial header value.  If ``None`` (the default), the
   initial header value is not set.  You can later append to the header with
   :meth:`append` method calls.  *s* may be a byte string or a Unicode string, but
   see the :meth:`append` documentation for semantics.

   Optional *charset* serves two purposes: it has the same meaning as the *charset*
   argument to the :meth:`append` method.  It also sets the default character set
   for all subsequent :meth:`append` calls that omit the *charset* argument.  If
   *charset* is not provided in the constructor (the default), the ``us-ascii``
   character set is used both as *s*'s initial charset and as the default for
   subsequent :meth:`append` calls.

   The maximum line length can be specified explicitly via *maxlinelen*.  For
   splitting the first line to a shorter value (to account for the field header
   which isn't included in *s*, e.g. :mailheader:`Subject`) pass in the name of the
   field in *header_name*.  The default *maxlinelen* is 76, and the default value
   for *header_name* is ``None``, meaning it is not taken into account for the
   first line of a long, split header.

   Optional *continuation_ws* must be :rfc:`2822`\ -compliant folding whitespace,
   and is usually either a space or a hard tab character. This character will be
   prepended to continuation lines.  *continuation_ws* defaults to a single
   space character (" ").

   Optional *errors* is passed straight through to the :meth:`append` method.


   .. method:: append(s[, charset[, errors]])

      Append the string *s* to the MIME header.

      Optional *charset*, if given, should be a :class:`~email.charset.Charset`
      instance (see :mod:`email.charset`) or the name of a character set, which
      will be converted to a :class:`~email.charset.Charset` instance.  A value
      of ``None`` (the default) means that the *charset* given in the constructor
      is used.

      *s* may be a byte string or a Unicode string.  If it is a byte string
      (i.e.  ``isinstance(s, str)`` is true), then *charset* is the encoding of
      that byte string, and a :exc:`UnicodeError` will be raised if the string
      cannot be decoded with that character set.

      If *s* is a Unicode string, then *charset* is a hint specifying the
      character set of the characters in the string.  In this case, when
      producing an :rfc:`2822`\ -compliant header using :rfc:`2047` rules, the
      Unicode string will be encoded using the following charsets in order:
      ``us-ascii``, the *charset* hint, ``utf-8``.  The first character set to
      not provoke a :exc:`UnicodeError` is used.

      Optional *errors* is passed through to any :func:`unicode` or
      :func:`ustr.encode` call, and defaults to "strict".


   .. method:: encode([splitchars])

      Encode a message header into an RFC-compliant format, possibly wrapping
      long lines and encapsulating non-ASCII parts in base64 or quoted-printable
      encodings.  Optional *splitchars* is a string containing characters to
      split long ASCII lines on, in rough support of :rfc:`2822`'s *highest
      level syntactic breaks*.  This doesn't affect :rfc:`2047` encoded lines.

   The :class:`Header` class also provides a number of methods to support
   standard operators and built-in functions.


   .. method:: __str__()

      A synonym for :meth:`Header.encode`.  Useful for ``str(aHeader)``.


   .. method:: __unicode__()

      A helper for the built-in :func:`unicode` function.  Returns the header as
      a Unicode string.


   .. method:: __eq__(other)

      This method allows you to compare two :class:`Header` instances for
      equality.


   .. method:: __ne__(other)

      This method allows you to compare two :class:`Header` instances for
      inequality.

The :mod:`email.header` module also provides the following convenient functions.


.. function:: decode_header(header)

   Decode a message header value without converting the character set. The header
   value is in *header*.

   This function returns a list of ``(decoded_string, charset)`` pairs containing
   each of the decoded parts of the header.  *charset* is ``None`` for non-encoded
   parts of the header, otherwise a lower case string containing the name of the
   character set specified in the encoded string.

   Here's an example::

      >>> from email.header import decode_header
      >>> decode_header('=?iso-8859-1?q?p=F6stal?=')
      [('p\xf6stal', 'iso-8859-1')]


.. function:: make_header(decoded_seq[, maxlinelen[, header_name[, continuation_ws]]])

   Create a :class:`Header` instance from a sequence of pairs as returned by
   :func:`decode_header`.

   :func:`decode_header` takes a header value string and returns a sequence of
   pairs of the format ``(decoded_string, charset)`` where *charset* is the name of
   the character set.

   This function takes one of those sequence of pairs and returns a :class:`Header`
   instance.  Optional *maxlinelen*, *header_name*, and *continuation_ws* are as in
   the :class:`Header` constructor.

