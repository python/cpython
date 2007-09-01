:mod:`email`: Miscellaneous utilities
-------------------------------------

.. module:: email.utils
   :synopsis: Miscellaneous email package utilities.


There are several useful utilities provided in the :mod:`email.utils` module:


.. function:: quote(str)

   Return a new string with backslashes in *str* replaced by two backslashes, and
   double quotes replaced by backslash-double quote.


.. function:: unquote(str)

   Return a new string which is an *unquoted* version of *str*. If *str* ends and
   begins with double quotes, they are stripped off.  Likewise if *str* ends and
   begins with angle brackets, they are stripped off.


.. function:: parseaddr(address)

   Parse address -- which should be the value of some address-containing field such
   as :mailheader:`To` or :mailheader:`Cc` -- into its constituent *realname* and
   *email address* parts.  Returns a tuple of that information, unless the parse
   fails, in which case a 2-tuple of ``('', '')`` is returned.


.. function:: formataddr(pair)

   The inverse of :meth:`parseaddr`, this takes a 2-tuple of the form ``(realname,
   email_address)`` and returns the string value suitable for a :mailheader:`To` or
   :mailheader:`Cc` header.  If the first element of *pair* is false, then the
   second element is returned unmodified.


.. function:: getaddresses(fieldvalues)

   This method returns a list of 2-tuples of the form returned by ``parseaddr()``.
   *fieldvalues* is a sequence of header field values as might be returned by
   :meth:`Message.get_all`.  Here's a simple example that gets all the recipients
   of a message::

      from email.utils import getaddresses

      tos = msg.get_all('to', [])
      ccs = msg.get_all('cc', [])
      resent_tos = msg.get_all('resent-to', [])
      resent_ccs = msg.get_all('resent-cc', [])
      all_recipients = getaddresses(tos + ccs + resent_tos + resent_ccs)


.. function:: parsedate(date)

   Attempts to parse a date according to the rules in :rfc:`2822`. however, some
   mailers don't follow that format as specified, so :func:`parsedate` tries to
   guess correctly in such cases.  *date* is a string containing an :rfc:`2822`
   date, such as  ``"Mon, 20 Nov 1995 19:12:08 -0500"``.  If it succeeds in parsing
   the date, :func:`parsedate` returns a 9-tuple that can be passed directly to
   :func:`time.mktime`; otherwise ``None`` will be returned.  Note that indexes 6,
   7, and 8 of the result tuple are not usable.


.. function:: parsedate_tz(date)

   Performs the same function as :func:`parsedate`, but returns either ``None`` or
   a 10-tuple; the first 9 elements make up a tuple that can be passed directly to
   :func:`time.mktime`, and the tenth is the offset of the date's timezone from UTC
   (which is the official term for Greenwich Mean Time) [#]_.  If the input string
   has no timezone, the last element of the tuple returned is ``None``.  Note that
   indexes 6, 7, and 8 of the result tuple are not usable.


.. function:: mktime_tz(tuple)

   Turn a 10-tuple as returned by :func:`parsedate_tz` into a UTC timestamp.  It
   the timezone item in the tuple is ``None``, assume local time.  Minor
   deficiency: :func:`mktime_tz` interprets the first 8 elements of *tuple* as a
   local time and then compensates for the timezone difference.  This may yield a
   slight error around changes in daylight savings time, though not worth worrying
   about for common use.


.. function:: formatdate([timeval[, localtime][, usegmt]])

   Returns a date string as per :rfc:`2822`, e.g.::

      Fri, 09 Nov 2001 01:08:47 -0000

   Optional *timeval* if given is a floating point time value as accepted by
   :func:`time.gmtime` and :func:`time.localtime`, otherwise the current time is
   used.

   Optional *localtime* is a flag that when ``True``, interprets *timeval*, and
   returns a date relative to the local timezone instead of UTC, properly taking
   daylight savings time into account. The default is ``False`` meaning UTC is
   used.

   Optional *usegmt* is a flag that when ``True``, outputs a  date string with the
   timezone as an ascii string ``GMT``, rather than a numeric ``-0000``. This is
   needed for some protocols (such as HTTP). This only applies when *localtime* is
   ``False``.


.. function:: make_msgid([idstring])

   Returns a string suitable for an :rfc:`2822`\ -compliant
   :mailheader:`Message-ID` header.  Optional *idstring* if given, is a string used
   to strengthen the uniqueness of the message id.


.. function:: decode_rfc2231(s)

   Decode the string *s* according to :rfc:`2231`.


.. function:: encode_rfc2231(s[, charset[, language]])

   Encode the string *s* according to :rfc:`2231`.  Optional *charset* and
   *language*, if given is the character set name and language name to use.  If
   neither is given, *s* is returned as-is.  If *charset* is given but *language*
   is not, the string is encoded using the empty string for *language*.


.. function:: collapse_rfc2231_value(value[, errors[, fallback_charset]])

   When a header parameter is encoded in :rfc:`2231` format,
   :meth:`Message.get_param` may return a 3-tuple containing the character set,
   language, and value.  :func:`collapse_rfc2231_value` turns this into a unicode
   string.  Optional *errors* is passed to the *errors* argument of the built-in
   :func:`unicode` function; it defaults to ``replace``.  Optional
   *fallback_charset* specifies the character set to use if the one in the
   :rfc:`2231` header is not known by Python; it defaults to ``us-ascii``.

   For convenience, if the *value* passed to :func:`collapse_rfc2231_value` is not
   a tuple, it should be a string and it is returned unquoted.


.. function:: decode_params(params)

   Decode parameters list according to :rfc:`2231`.  *params* is a sequence of
   2-tuples containing elements of the form ``(content-type, string-value)``.


.. rubric:: Footnotes

.. [#] Note that the sign of the timezone offset is the opposite of the sign of the
   ``time.timezone`` variable for the same timezone; the latter variable follows
   the POSIX standard while this module follows :rfc:`2822`.

