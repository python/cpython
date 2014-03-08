:mod:`base64` --- Base16, Base32, Base64, Base85 Data Encodings
===============================================================

.. module:: base64
   :synopsis: RFC 3548: Base16, Base32, Base64 Data Encodings;
              Base85 and Ascii85


.. index::
   pair: base64; encoding
   single: MIME; base64 encoding

This module provides functions for encoding binary data to printable
ASCII characters and decoding such encodings back to binary data.
It provides encoding and decoding functions for the encodings specified in
in :rfc:`3548`, which defines the Base16, Base32, and Base64 algorithms,
and for the de-facto standard Ascii85 and Base85 encodings.

The :rfc:`3548` encodings are suitable for encoding binary data so that it can
safely sent by email, used as parts of URLs, or included as part of an HTTP
POST request.  The encoding algorithm is not the same as the
:program:`uuencode` program.

There are two :rfc:`3548` interfaces provided by this module.  The modern
interface supports encoding and decoding ASCII byte string objects using all
three :rfc:`3548` defined alphabets (normal, URL-safe, and filesystem-safe).
Additionally, the decoding functions of the modern interface also accept
Unicode strings containing only ASCII characters. The legacy interface provides
for encoding and decoding to and from file-like objects as well as byte
strings, but only using the Base64 standard alphabet.

.. versionchanged:: 3.3
   ASCII-only Unicode strings are now accepted by the decoding functions of
   the modern interface.

.. versionchanged:: 3.4
   Any :term:`bytes-like object`\ s are now accepted by all
   encoding and decoding functions in this module.  Ascii85/Base85 support added.

The modern interface provides:

.. function:: b64encode(s, altchars=None)

   Encode a byte string using Base64.

   *s* is the string to encode.  Optional *altchars* must be a string of at least
   length 2 (additional characters are ignored) which specifies an alternative
   alphabet for the ``+`` and ``/`` characters.  This allows an application to e.g.
   generate URL or filesystem safe Base64 strings.  The default is ``None``, for
   which the standard Base64 alphabet is used.

   The encoded byte string is returned.


.. function:: b64decode(s, altchars=None, validate=False)

   Decode a Base64 encoded byte string.

   *s* is the byte string to decode.  Optional *altchars* must be a string of
   at least length 2 (additional characters are ignored) which specifies the
   alternative alphabet used instead of the ``+`` and ``/`` characters.

   The decoded string is returned.  A :exc:`binascii.Error` exception is raised
   if *s* is incorrectly padded.

   If *validate* is ``False`` (the default), non-base64-alphabet characters are
   discarded prior to the padding check.  If *validate* is ``True``,
   non-base64-alphabet characters in the input result in a
   :exc:`binascii.Error`.


.. function:: standard_b64encode(s)

   Encode byte string *s* using the standard Base64 alphabet.


.. function:: standard_b64decode(s)

   Decode byte string *s* using the standard Base64 alphabet.


.. function:: urlsafe_b64encode(s)

   Encode byte string *s* using a URL-safe alphabet, which substitutes ``-`` instead of
   ``+`` and ``_`` instead of ``/`` in the standard Base64 alphabet.  The result
   can still contain ``=``.


.. function:: urlsafe_b64decode(s)

   Decode byte string *s* using a URL-safe alphabet, which substitutes ``-`` instead of
   ``+`` and ``_`` instead of ``/`` in the standard Base64 alphabet.


.. function:: b32encode(s)

   Encode a byte string using Base32.  *s* is the string to encode.  The encoded string
   is returned.


.. function:: b32decode(s, casefold=False, map01=None)

   Decode a Base32 encoded byte string.

   *s* is the byte string to decode.  Optional *casefold* is a flag specifying
   whether a lowercase alphabet is acceptable as input.  For security purposes,
   the default is ``False``.

   :rfc:`3548` allows for optional mapping of the digit 0 (zero) to the letter O
   (oh), and for optional mapping of the digit 1 (one) to either the letter I (eye)
   or letter L (el).  The optional argument *map01* when not ``None``, specifies
   which letter the digit 1 should be mapped to (when *map01* is not ``None``, the
   digit 0 is always mapped to the letter O).  For security purposes the default is
   ``None``, so that 0 and 1 are not allowed in the input.

   The decoded byte string is returned.  A :exc:`binascii.Error` is raised if *s* is
   incorrectly padded or if there are non-alphabet characters present in the
   string.


.. function:: b16encode(s)

   Encode a byte string using Base16.

   *s* is the string to encode.  The encoded byte string is returned.


.. function:: b16decode(s, casefold=False)

   Decode a Base16 encoded byte string.

   *s* is the string to decode.  Optional *casefold* is a flag specifying whether a
   lowercase alphabet is acceptable as input.  For security purposes, the default
   is ``False``.

   The decoded byte string is returned.  A :exc:`TypeError` is raised if *s* were
   incorrectly padded or if there are non-alphabet characters present in the
   string.


.. function:: a85encode(s, *, foldspaces=False, wrapcol=0, pad=False, adobe=False)

   Encode a byte string using Ascii85.

   *s* is the string to encode. The encoded byte string is returned.

   *foldspaces* is an optional flag that uses the special short sequence 'y'
   instead of 4 consecutive spaces (ASCII 0x20) as supported by 'btoa'. This
   feature is not supported by the "standard" Ascii85 encoding.

   *wrapcol* controls whether the output should have newline ('\n')
   characters added to it. If this is non-zero, each output line will be
   at most this many characters long.

   *pad* controls whether the input string is padded to a multiple of 4
   before encoding. Note that the ``btoa`` implementation always pads.

   *adobe* controls whether the encoded byte sequence is framed with ``<~``
   and ``~>``, which is used by the Adobe implementation.

   .. versionadded:: 3.4


.. function:: a85decode(s, *, foldspaces=False, adobe=False, ignorechars=b' \t\n\r\v')

   Decode an Ascii85 encoded byte string.

   *s* is the byte string to decode.

   *foldspaces* is a flag that specifies whether the 'y' short sequence
   should be accepted as shorthand for 4 consecutive spaces (ASCII 0x20).
   This feature is not supported by the "standard" Ascii85 encoding.

   *adobe* controls whether the input sequence is in Adobe Ascii85 format
   (i.e. is framed with <~ and ~>).

   *ignorechars* should be a byte string containing characters to ignore
   from the input. This should only contain whitespace characters, and by
   default contains all whitespace characters in ASCII.

   .. versionadded:: 3.4


.. function:: b85encode(s, pad=False)

   Encode a byte string using base85, as used in e.g. git-style binary
   diffs.

   If *pad* is true, the input is padded with "\\0" so its length is a
   multiple of 4 characters before encoding.

   .. versionadded:: 3.4


.. function:: b85decode(b)

   Decode base85-encoded byte string.  Padding is implicitly removed, if
   necessary.

   .. versionadded:: 3.4


.. note::
   Both Base85 and Ascii85 have an expansion factor of 5 to 4 (5 Base85 or
   Ascii85 characters can encode 4 binary bytes), while the better-known
   Base64 has an expansion factor of 6 to 4.  They are therefore more
   efficient when space expensive.  They differ by details such as the
   character map used for encoding.


The legacy interface:

.. function:: decode(input, output)

   Decode the contents of the binary *input* file and write the resulting binary
   data to the *output* file. *input* and *output* must be :term:`file objects
   <file object>`. *input* will be read until ``input.read()`` returns an empty
   bytes object.


.. function:: decodebytes(s)
              decodestring(s)

   Decode the byte string *s*, which must contain one or more lines of base64
   encoded data, and return a byte string containing the resulting binary data.
   ``decodestring`` is a deprecated alias.

   .. versionadded:: 3.1


.. function:: encode(input, output)

   Encode the contents of the binary *input* file and write the resulting base64
   encoded data to the *output* file. *input* and *output* must be :term:`file
   objects <file object>`. *input* will be read until ``input.read()`` returns
   an empty bytes object. :func:`encode` returns the encoded data plus a trailing
   newline character (``b'\n'``).


.. function:: encodebytes(s)
              encodestring(s)

   Encode the byte string *s*, which can contain arbitrary binary data, and
   return a byte string containing one or more lines of base64-encoded data.
   :func:`encodebytes` returns a string containing one or more lines of
   base64-encoded data always including an extra trailing newline (``b'\n'``).
   ``encodestring`` is a deprecated alias.


An example usage of the module:

   >>> import base64
   >>> encoded = base64.b64encode(b'data to be encoded')
   >>> encoded
   b'ZGF0YSB0byBiZSBlbmNvZGVk'
   >>> data = base64.b64decode(encoded)
   >>> data
   b'data to be encoded'


.. seealso::

   Module :mod:`binascii`
      Support module containing ASCII-to-binary and binary-to-ASCII conversions.

   :rfc:`1521` - MIME (Multipurpose Internet Mail Extensions) Part One: Mechanisms for Specifying and Describing the Format of Internet Message Bodies
      Section 5.2, "Base64 Content-Transfer-Encoding," provides the definition of the
      base64 encoding.

