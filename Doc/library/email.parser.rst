:mod:`email.parser`: Parsing email messages
-------------------------------------------

.. module:: email.parser
   :synopsis: Parse flat text email messages to produce a message object structure.

**Source code:** :source:`Lib/email/parser.py`

--------------

Message object structures can be created in one of two ways: they can be
created from whole cloth by creating an :class:`~email.message.EmailMessage`
object, adding headers using the dictionary interface, and adding payload(s)
using :meth:`~email.message.EmailMessage.set_content` and related methods, or
they can be created by parsing a serialized representation of the email
message.

The :mod:`email` package provides a standard parser that understands most email
document structures, including MIME documents.  You can pass the parser a
bytes, string or file object, and the parser will return to you the root
:class:`~email.message.EmailMessage` instance of the object structure.  For
simple, non-MIME messages the payload of this root object will likely be a
string containing the text of the message.  For MIME messages, the root object
will return ``True`` from its :meth:`~email.message.EmailMessage.is_multipart`
method, and the subparts can be accessed via the payload manipulation methods,
such as :meth:`~email.message.EmailMessage.get_body`,
:meth:`~email.message.EmailMessage.iter_parts`, and
:meth:`~email.message.EmailMessage.walk`.

There are actually two parser interfaces available for use, the :class:`Parser`
API and the incremental :class:`FeedParser` API.  The :class:`Parser` API is
most useful if you have the entire text of the message in memory, or if the
entire message lives in a file on the file system.  :class:`FeedParser` is more
appropriate when you are reading the message from a stream which might block
waiting for more input (such as reading an email message from a socket).  The
:class:`FeedParser` can consume and parse the message incrementally, and only
returns the root object when you close the parser.

Note that the parser can be extended in limited ways, and of course you can
implement your own parser completely from scratch.  All of the logic that
connects the :mod:`email` package's bundled parser and the
:class:`~email.message.EmailMessage` class is embodied in the :mod:`policy`
class, so a custom parser can create message object trees any way it finds
necessary by implementing custom versions of the appropriate :mod:`policy`
methods.


FeedParser API
^^^^^^^^^^^^^^

The :class:`BytesFeedParser`, imported from the :mod:`email.feedparser` module,
provides an API that is conducive to incremental parsing of email messages,
such as would be necessary when reading the text of an email message from a
source that can block (such as a socket).  The :class:`BytesFeedParser` can of
course be used to parse an email message fully contained in a :term:`bytes-like
object`, string, or file, but the :class:`BytesParser` API may be more
convenient for such use cases.  The semantics and results of the two parser
APIs are identical.

The :class:`BytesFeedParser`'s API is simple; you create an instance, feed it a
bunch of bytes until there's no more to feed it, then close the parser to
retrieve the root message object.  The :class:`BytesFeedParser` is extremely
accurate when parsing standards-compliant messages, and it does a very good job
of parsing non-compliant messages, providing information about how a message
was deemed broken.  It will populate a message object's
:attr:`~email.message.EmailMessage.defects` attribute with a list of any
problems it found in a message.  See the :mod:`email.errors` module for the
list of defects that it can find.

Here is the API for the :class:`BytesFeedParser`:


.. class:: BytesFeedParser(_factory=None, *, policy=policy.compat32)

   Create a :class:`BytesFeedParser` instance.  Optional *_factory* is a
   no-argument callable; if not specified use the
   :attr:`~email.policy.Policy.message_factory` from the *policy*.  Call
   *_factory* whenever a new message object is needed.

   If *policy* is specified use the rules it specifies to update the
   representation of the message.  If *policy* is not set, use the
   :class:`compat32 <email.policy.Compat32>` policy, which maintains backward
   compatibility with the Python 3.2 version of the email package and provides
   :class:`~email.message.Message` as the default factory.  All other policies
   provide :class:`~email.message.EmailMessage` as the default *_factory*. For
   more information on what else *policy* controls, see the
   :mod:`~email.policy` documentation.

   Note: **The policy keyword should always be specified**; The default will
   change to :data:`email.policy.default` in a future version of Python.

   .. versionadded:: 3.2

   .. versionchanged:: 3.3 Added the *policy* keyword.
   .. versionchanged:: 3.6 *_factory* defaults to the policy ``message_factory``.


   .. method:: feed(data)

      Feed the parser some more data.  *data* should be a :term:`bytes-like
      object` containing one or more lines.  The lines can be partial and the
      parser will stitch such partial lines together properly.  The lines can
      have any of the three common line endings: carriage return, newline, or
      carriage return and newline (they can even be mixed).


   .. method:: close()

      Complete the parsing of all previously fed data and return the root
      message object.  It is undefined what happens if :meth:`~feed` is called
      after this method has been called.


.. class:: FeedParser(_factory=None, *, policy=policy.compat32)

   Works like :class:`BytesFeedParser` except that the input to the
   :meth:`~BytesFeedParser.feed` method must be a string.  This is of limited
   utility, since the only way for such a message to be valid is for it to
   contain only ASCII text or, if :attr:`~email.policy.Policy.utf8` is
   ``True``, no binary attachments.

   .. versionchanged:: 3.3 Added the *policy* keyword.


Parser API
^^^^^^^^^^

The :class:`BytesParser` class, imported from the :mod:`email.parser` module,
provides an API that can be used to parse a message when the complete contents
of the message are available in a :term:`bytes-like object` or file.  The
:mod:`email.parser` module also provides :class:`Parser` for parsing strings,
and header-only parsers, :class:`BytesHeaderParser` and
:class:`HeaderParser`, which can be used if you're only interested in the
headers of the message.  :class:`BytesHeaderParser` and :class:`HeaderParser`
can be much faster in these situations, since they do not attempt to parse the
message body, instead setting the payload to the raw body.


.. class:: BytesParser(_class=None, *, policy=policy.compat32)

   Create a :class:`BytesParser` instance.  The *_class* and *policy*
   arguments have the same meaning and sematnics as the *_factory*
   and *policy* arguments of :class:`BytesFeedParser`.

   Note: **The policy keyword should always be specified**; The default will
   change to :data:`email.policy.default` in a future version of Python.

   .. versionchanged:: 3.3
      Removed the *strict* argument that was deprecated in 2.4.  Added the
      *policy* keyword.
   .. versionchanged:: 3.6 *_class* defaults to the policy ``message_factory``.


   .. method:: parse(fp, headersonly=False)

      Read all the data from the binary file-like object *fp*, parse the
      resulting bytes, and return the message object.  *fp* must support
      both the :meth:`~io.IOBase.readline` and the :meth:`~io.IOBase.read`
      methods.

      The bytes contained in *fp* must be formatted as a block of :rfc:`5322`
      (or, if :attr:`~email.policy.Policy.utf8` is ``True``, :rfc:`6532`)
      style headers and header continuation lines, optionally preceded by an
      envelope header.  The header block is terminated either by the end of the
      data or by a blank line.  Following the header block is the body of the
      message (which may contain MIME-encoded subparts, including subparts
      with a :mailheader:`Content-Transfer-Encoding` of ``8bit``.

      Optional *headersonly* is a flag specifying whether to stop parsing after
      reading the headers or not.  The default is ``False``, meaning it parses
      the entire contents of the file.


   .. method:: parsebytes(bytes, headersonly=False)

      Similar to the :meth:`parse` method, except it takes a :term:`bytes-like
      object` instead of a file-like object.  Calling this method on a
      :term:`bytes-like object` is equivalent to wrapping *bytes* in a
      :class:`~io.BytesIO` instance first and calling :meth:`parse`.

      Optional *headersonly* is as with the :meth:`parse` method.

   .. versionadded:: 3.2


.. class:: BytesHeaderParser(_class=None, *, policy=policy.compat32)

   Exactly like :class:`BytesParser`, except that *headersonly*
   defaults to ``True``.

   .. versionadded:: 3.3


.. class:: Parser(_class=None, *, policy=policy.compat32)

   This class is parallel to :class:`BytesParser`, but handles string input.

   .. versionchanged:: 3.3
      Removed the *strict* argument.  Added the *policy* keyword.
   .. versionchanged:: 3.6 *_class* defaults to the policy ``message_factory``.


   .. method:: parse(fp, headersonly=False)

      Read all the data from the text-mode file-like object *fp*, parse the
      resulting text, and return the root message object.  *fp* must support
      both the :meth:`~io.TextIOBase.readline` and the
      :meth:`~io.TextIOBase.read` methods on file-like objects.

      Other than the text mode requirement, this method operates like
      :meth:`BytesParser.parse`.


   .. method:: parsestr(text, headersonly=False)

      Similar to the :meth:`parse` method, except it takes a string object
      instead of a file-like object.  Calling this method on a string is
      equivalent to wrapping *text* in a :class:`~io.StringIO` instance first
      and calling :meth:`parse`.

      Optional *headersonly* is as with the :meth:`parse` method.


.. class:: HeaderParser(_class=None, *, policy=policy.compat32)

   Exactly like :class:`Parser`, except that *headersonly*
   defaults to ``True``.


Since creating a message object structure from a string or a file object is such
a common task, four functions are provided as a convenience.  They are available
in the top-level :mod:`email` package namespace.

.. currentmodule:: email


.. function:: message_from_bytes(s, _class=None, *, policy=policy.compat32)

   Return a message object structure from a :term:`bytes-like object`.  This is
   equivalent to ``BytesParser().parsebytes(s)``.  Optional *_class* and
   *strict* are interpreted as with the :class:`~email.parser.BytesParser` class
   constructor.

   .. versionadded:: 3.2
   .. versionchanged:: 3.3
      Removed the *strict* argument.  Added the *policy* keyword.


.. function:: message_from_binary_file(fp, _class=None, *,
                                       policy=policy.compat32)

   Return a message object structure tree from an open binary :term:`file
   object`.  This is equivalent to ``BytesParser().parse(fp)``.  *_class* and
   *policy* are interpreted as with the :class:`~email.parser.BytesParser` class
   constructor.

   .. versionadded:: 3.2
   .. versionchanged:: 3.3
      Removed the *strict* argument.  Added the *policy* keyword.


.. function:: message_from_string(s, _class=None, *, policy=policy.compat32)

   Return a message object structure from a string.  This is equivalent to
   ``Parser().parsestr(s)``.  *_class* and *policy* are interpreted as
   with the :class:`~email.parser.Parser` class constructor.

   .. versionchanged:: 3.3
      Removed the *strict* argument.  Added the *policy* keyword.


.. function:: message_from_file(fp, _class=None, *, policy=policy.compat32)

   Return a message object structure tree from an open :term:`file object`.
   This is equivalent to ``Parser().parse(fp)``.  *_class* and *policy* are
   interpreted as with the :class:`~email.parser.Parser` class constructor.

   .. versionchanged:: 3.3
      Removed the *strict* argument.  Added the *policy* keyword.
   .. versionchanged:: 3.6 *_class* defaults to the policy ``message_factory``.


Here's an example of how you might use :func:`message_from_bytes` at an
interactive Python prompt::

   >>> import email
   >>> msg = email.message_from_bytes(myBytes)  # doctest: +SKIP


Additional notes
^^^^^^^^^^^^^^^^

Here are some notes on the parsing semantics:

* Most non-\ :mimetype:`multipart` type messages are parsed as a single message
  object with a string payload.  These objects will return ``False`` for
  :meth:`~email.message.EmailMessage.is_multipart`, and
  :meth:`~email.message.EmailMessage.iter_parts` will yield an empty list.

* All :mimetype:`multipart` type messages will be parsed as a container message
  object with a list of sub-message objects for their payload.  The outer
  container message will return ``True`` for
  :meth:`~email.message.EmailMessage.is_multipart`, and
  :meth:`~email.message.EmailMessage.iter_parts` will yield a list of subparts.

* Most messages with a content type of :mimetype:`message/\*` (such as
  :mimetype:`message/delivery-status` and :mimetype:`message/rfc822`) will also
  be parsed as container object containing a list payload of length 1.  Their
  :meth:`~email.message.EmailMessage.is_multipart` method will return ``True``.
  The single element yielded by :meth:`~email.message.EmailMessage.iter_parts`
  will be a sub-message object.

* Some non-standards-compliant messages may not be internally consistent about
  their :mimetype:`multipart`\ -edness.  Such messages may have a
  :mailheader:`Content-Type` header of type :mimetype:`multipart`, but their
  :meth:`~email.message.EmailMessage.is_multipart` method may return ``False``.
  If such messages were parsed with the :class:`~email.parser.FeedParser`,
  they will have an instance of the
  :class:`~email.errors.MultipartInvariantViolationDefect` class in their
  *defects* attribute list.  See :mod:`email.errors` for details.
