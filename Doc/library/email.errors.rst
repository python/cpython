:mod:`!email.errors`: Exception and Defect classes
--------------------------------------------------

.. module:: email.errors
   :synopsis: The exception classes used by the email package.

**Source code:** :source:`Lib/email/errors.py`

--------------

The following exception classes are defined in the :mod:`!email.errors` module:


.. exception:: MessageError()

   This is the base class for all exceptions that the :mod:`email` package can
   raise.  It is derived from the standard :exc:`Exception` class and defines no
   additional methods.


.. exception:: MessageParseError()

   This is the base class for exceptions raised by the
   :class:`~email.parser.Parser` class.  It is derived from
   :exc:`MessageError`.  This class is also used internally by the parser used
   by :mod:`~email.headerregistry`.


.. exception:: HeaderParseError()

   Raised under some error conditions when parsing the :rfc:`5322` headers of a
   message, this class is derived from :exc:`MessageParseError`.  The
   :meth:`~email.message.EmailMessage.set_boundary` method will raise this
   error if the content type is unknown when the method is called.
   :class:`~email.header.Header` may raise this error for certain base64
   decoding errors, and when an attempt is made to create a header that appears
   to contain an embedded header (that is, there is what is supposed to be a
   continuation line that has no leading whitespace and looks like a header).


.. exception:: BoundaryError()

   Deprecated and no longer used.


.. exception:: MultipartConversionError()

   Raised if the :meth:`~email.message.Message.attach` method is called
   on an instance of a class derived from
   :class:`~email.mime.nonmultipart.MIMENonMultipart` (e.g.
   :class:`~email.mime.image.MIMEImage`).
   :exc:`MultipartConversionError` multiply
   inherits from :exc:`MessageError` and the built-in :exc:`TypeError`.


.. exception:: HeaderWriteError()

   Raised when an error occurs when the :mod:`~email.generator` outputs
   headers.


.. exception:: MessageDefect()

   This is the base class for all defects found when parsing email messages.
   It is derived from :exc:`ValueError`.

.. exception:: HeaderDefect()

   This is the base class for all defects found when parsing email headers.
   It is derived from :exc:`MessageDefect`.

Here is the list of the defects that the :class:`~email.parser.FeedParser`
can find while parsing messages.  Note that the defects are added to the message
where the problem was found, so for example, if a message nested inside a
:mimetype:`multipart/alternative` had a malformed header, that nested message
object would have a defect, but the containing messages would not.

All defect classes are subclassed from :class:`email.errors.MessageDefect`.

.. exception:: NoBoundaryInMultipartDefect

   A message claimed to be a multipart, but had no :mimetype:`boundary`
   parameter.

.. exception:: StartBoundaryNotFoundDefect

   The start boundary claimed in the :mailheader:`Content-Type` header was
   never found.

.. exception:: CloseBoundaryNotFoundDefect

   A start boundary was found, but no corresponding close boundary was ever
   found.

   .. versionadded:: 3.3

.. exception:: FirstHeaderLineIsContinuationDefect

   The message had a continuation line as its first header line.

.. exception:: MisplacedEnvelopeHeaderDefect

   A "Unix From" header was found in the middle of a header block.

.. exception:: MissingHeaderBodySeparatorDefect

   A line was found while parsing headers that had no leading white space but
   contained no ':'.  Parsing continues assuming that the line represents the
   first line of the body.

   .. versionadded:: 3.3

.. exception:: MalformedHeaderDefect

   A header was found that was missing a colon, or was otherwise malformed.

   .. deprecated:: 3.3
      This defect has not been used for several Python versions.

.. exception:: MultipartInvariantViolationDefect

   A message claimed to be a :mimetype:`multipart`, but no subparts were found.
   Note that when a message has this defect, its
   :meth:`~email.message.Message.is_multipart` method may return ``False``
   even though its content type claims to be :mimetype:`multipart`.

.. exception:: InvalidBase64PaddingDefect

   When decoding a block of base64 encoded bytes, the padding was not correct.
   Enough padding is added to perform the decode, but the resulting decoded
   bytes may be invalid.

.. exception:: InvalidBase64CharactersDefect

   When decoding a block of base64 encoded bytes, characters outside the base64
   alphabet were encountered.  The characters are ignored, but the resulting
   decoded bytes may be invalid.

.. exception:: InvalidBase64LengthDefect

   When decoding a block of base64 encoded bytes, the number of non-padding
   base64 characters was invalid (1 more than a multiple of 4).  The encoded
   block was kept as-is.

.. exception:: InvalidDateDefect

   When decoding an invalid or unparsable date field.  The original value is
   kept as-is.
