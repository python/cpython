:mod:`email.charset`: Representing character sets
-------------------------------------------------

.. module:: email.charset
   :synopsis: Character Sets


This module provides a class :class:`Charset` for representing character sets
and character set conversions in email messages, as well as a character set
registry and several convenience methods for manipulating this registry.
Instances of :class:`Charset` are used in several other modules within the
:mod:`email` package.

Import this class from the :mod:`email.charset` module.

.. versionadded:: 2.2.2


.. class:: Charset([input_charset])

   Map character sets to their email properties.

   This class provides information about the requirements imposed on email for a
   specific character set.  It also provides convenience routines for converting
   between character sets, given the availability of the applicable codecs.  Given
   a character set, it will do its best to provide information on how to use that
   character set in an email message in an RFC-compliant way.

   Certain character sets must be encoded with quoted-printable or base64 when used
   in email headers or bodies.  Certain character sets must be converted outright,
   and are not allowed in email.

   Optional *input_charset* is as described below; it is always coerced to lower
   case.  After being alias normalized it is also used as a lookup into the
   registry of character sets to find out the header encoding, body encoding, and
   output conversion codec to be used for the character set.  For example, if
   *input_charset* is ``iso-8859-1``, then headers and bodies will be encoded using
   quoted-printable and no output conversion codec is necessary.  If
   *input_charset* is ``euc-jp``, then headers will be encoded with base64, bodies
   will not be encoded, but output text will be converted from the ``euc-jp``
   character set to the ``iso-2022-jp`` character set.

   :class:`Charset` instances have the following data attributes:


   .. attribute:: input_charset

      The initial character set specified.  Common aliases are converted to
      their *official* email names (e.g. ``latin_1`` is converted to
      ``iso-8859-1``).  Defaults to 7-bit ``us-ascii``.


   .. attribute:: header_encoding

      If the character set must be encoded before it can be used in an email
      header, this attribute will be set to ``Charset.QP`` (for
      quoted-printable), ``Charset.BASE64`` (for base64 encoding), or
      ``Charset.SHORTEST`` for the shortest of QP or BASE64 encoding. Otherwise,
      it will be ``None``.


   .. attribute:: body_encoding

      Same as *header_encoding*, but describes the encoding for the mail
      message's body, which indeed may be different than the header encoding.
      ``Charset.SHORTEST`` is not allowed for *body_encoding*.


   .. attribute:: output_charset

      Some character sets must be converted before they can be used in email headers
      or bodies.  If the *input_charset* is one of them, this attribute will
      contain the name of the character set output will be converted to.  Otherwise, it will
      be ``None``.


   .. attribute:: input_codec

      The name of the Python codec used to convert the *input_charset* to
      Unicode.  If no conversion codec is necessary, this attribute will be
      ``None``.


   .. attribute:: output_codec

      The name of the Python codec used to convert Unicode to the
      *output_charset*.  If no conversion codec is necessary, this attribute
      will have the same value as the *input_codec*.

   :class:`Charset` instances also have the following methods:


   .. method:: get_body_encoding()

      Return the content transfer encoding used for body encoding.

      This is either the string ``quoted-printable`` or ``base64`` depending on
      the encoding used, or it is a function, in which case you should call the
      function with a single argument, the Message object being encoded.  The
      function should then set the :mailheader:`Content-Transfer-Encoding`
      header itself to whatever is appropriate.

      Returns the string ``quoted-printable`` if *body_encoding* is ``QP``,
      returns the string ``base64`` if *body_encoding* is ``BASE64``, and
      returns the string ``7bit`` otherwise.


   .. method:: convert(s)

      Convert the string *s* from the *input_codec* to the *output_codec*.


   .. method:: to_splittable(s)

      Convert a possibly multibyte string to a safely splittable format. *s* is
      the string to split.

      Uses the *input_codec* to try and convert the string to Unicode, so it can
      be safely split on character boundaries (even for multibyte characters).

      Returns the string as-is if it isn't known how to convert *s* to Unicode
      with the *input_charset*.

      Characters that could not be converted to Unicode will be replaced with
      the Unicode replacement character ``'U+FFFD'``.


   .. method:: from_splittable(ustr[, to_output])

      Convert a splittable string back into an encoded string.  *ustr* is a
      Unicode string to "unsplit".

      This method uses the proper codec to try and convert the string from
      Unicode back into an encoded format.  Return the string as-is if it is not
      Unicode, or if it could not be converted from Unicode.

      Characters that could not be converted from Unicode will be replaced with
      an appropriate character (usually ``'?'``).

      If *to_output* is ``True`` (the default), uses *output_codec* to convert
      to an encoded format.  If *to_output* is ``False``, it uses *input_codec*.


   .. method:: get_output_charset()

      Return the output character set.

      This is the *output_charset* attribute if that is not ``None``, otherwise
      it is *input_charset*.


   .. method:: encoded_header_len()

      Return the length of the encoded header string, properly calculating for
      quoted-printable or base64 encoding.


   .. method:: header_encode(s[, convert])

      Header-encode the string *s*.

      If *convert* is ``True``, the string will be converted from the input
      charset to the output charset automatically.  This is not useful for
      multibyte character sets, which have line length issues (multibyte
      characters must be split on a character, not a byte boundary); use the
      higher-level :class:`~email.header.Header` class to deal with these issues
      (see :mod:`email.header`).  *convert* defaults to ``False``.

      The type of encoding (base64 or quoted-printable) will be based on the
      *header_encoding* attribute.


   .. method:: body_encode(s[, convert])

      Body-encode the string *s*.

      If *convert* is ``True`` (the default), the string will be converted from
      the input charset to output charset automatically. Unlike
      :meth:`header_encode`, there are no issues with byte boundaries and
      multibyte charsets in email bodies, so this is usually pretty safe.

      The type of encoding (base64 or quoted-printable) will be based on the
      *body_encoding* attribute.

   The :class:`Charset` class also provides a number of methods to support
   standard operations and built-in functions.


   .. method:: __str__()

      Returns *input_charset* as a string coerced to lower
      case. :meth:`__repr__` is an alias for :meth:`__str__`.


   .. method:: __eq__(other)

      This method allows you to compare two :class:`Charset` instances for
      equality.


   .. method:: __ne__(other)

      This method allows you to compare two :class:`Charset` instances for
      inequality.

The :mod:`email.charset` module also provides the following functions for adding
new entries to the global character set, alias, and codec registries:


.. function:: add_charset(charset[, header_enc[, body_enc[, output_charset]]])

   Add character properties to the global registry.

   *charset* is the input character set, and must be the canonical name of a
   character set.

   Optional *header_enc* and *body_enc* is either ``Charset.QP`` for
   quoted-printable, ``Charset.BASE64`` for base64 encoding,
   ``Charset.SHORTEST`` for the shortest of quoted-printable or base64 encoding,
   or ``None`` for no encoding.  ``SHORTEST`` is only valid for
   *header_enc*. The default is ``None`` for no encoding.

   Optional *output_charset* is the character set that the output should be in.
   Conversions will proceed from input charset, to Unicode, to the output charset
   when the method :meth:`Charset.convert` is called.  The default is to output in
   the same character set as the input.

   Both *input_charset* and *output_charset* must have Unicode codec entries in the
   module's character set-to-codec mapping; use :func:`add_codec` to add codecs the
   module does not know about.  See the :mod:`codecs` module's documentation for
   more information.

   The global character set registry is kept in the module global dictionary
   ``CHARSETS``.


.. function:: add_alias(alias, canonical)

   Add a character set alias.  *alias* is the alias name, e.g. ``latin-1``.
   *canonical* is the character set's canonical name, e.g. ``iso-8859-1``.

   The global charset alias registry is kept in the module global dictionary
   ``ALIASES``.


.. function:: add_codec(charset, codecname)

   Add a codec that map characters in the given character set to and from Unicode.

   *charset* is the canonical name of a character set. *codecname* is the name of a
   Python codec, as appropriate for the second argument to the :func:`unicode`
   built-in, or to the :meth:`~unicode.encode` method of a Unicode string.

