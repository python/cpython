:mod:`email.parser`: Parsing email messages
-------------------------------------------

.. module:: email.parser
   :synopsis: Parse flat text email messages to produce a message object structure.


Message object structures can be created in one of two ways: they can be created
from whole cloth by instantiating :class:`~email.message.Message` objects and
stringing them together via :meth:`attach` and :meth:`set_payload` calls, or they
can be created by parsing a flat text representation of the email message.

The :mod:`email` package provides a standard parser that understands most email
document structures, including MIME documents.  You can pass the parser a string
or a file object, and the parser will return to you the root
:class:`~email.message.Message` instance of the object structure.  For simple,
non-MIME messages the payload of this root object will likely be a string
containing the text of the message.  For MIME messages, the root object will
return ``True`` from its :meth:`is_multipart` method, and the subparts can be
accessed via the :meth:`get_payload` and :meth:`walk` methods.

There are actually two parser interfaces available for use, the classic
:class:`Parser` API and the incremental :class:`FeedParser` API.  The classic
:class:`Parser` API is fine if you have the entire text of the message in memory
as a string, or if the entire message lives in a file on the file system.
:class:`FeedParser` is more appropriate for when you're reading the message from
a stream which might block waiting for more input (e.g. reading an email message
from a socket).  The :class:`FeedParser` can consume and parse the message
incrementally, and only returns the root object when you close the parser [#]_.

Note that the parser can be extended in limited ways, and of course you can
implement your own parser completely from scratch.  There is no magical
connection between the :mod:`email` package's bundled parser and the
:class:`~email.message.Message` class, so your custom parser can create message
object trees any way it finds necessary.


FeedParser API
^^^^^^^^^^^^^^

.. versionadded:: 2.4

The :class:`FeedParser`, imported from the :mod:`email.feedparser` module,
provides an API that is conducive to incremental parsing of email messages, such
as would be necessary when reading the text of an email message from a source
that can block (e.g. a socket).  The :class:`FeedParser` can of course be used
to parse an email message fully contained in a string or a file, but the classic
:class:`Parser` API may be more convenient for such use cases.  The semantics
and results of the two parser APIs are identical.

The :class:`FeedParser`'s API is simple; you create an instance, feed it a bunch
of text until there's no more to feed it, then close the parser to retrieve the
root message object.  The :class:`FeedParser` is extremely accurate when parsing
standards-compliant messages, and it does a very good job of parsing
non-compliant messages, providing information about how a message was deemed
broken.  It will populate a message object's *defects* attribute with a list of
any problems it found in a message.  See the :mod:`email.errors` module for the
list of defects that it can find.

Here is the API for the :class:`FeedParser`:


.. class:: FeedParser([_factory])

   Create a :class:`FeedParser` instance.  Optional *_factory* is a no-argument
   callable that will be called whenever a new message object is needed.  It
   defaults to the :class:`email.message.Message` class.


   .. method:: feed(data)

      Feed the :class:`FeedParser` some more data.  *data* should be a string
      containing one or more lines.  The lines can be partial and the
      :class:`FeedParser` will stitch such partial lines together properly.  The
      lines in the string can have any of the common three line endings,
      carriage return, newline, or carriage return and newline (they can even be
      mixed).


   .. method:: close()

      Closing a :class:`FeedParser` completes the parsing of all previously fed
      data, and returns the root message object.  It is undefined what happens
      if you feed more data to a closed :class:`FeedParser`.


Parser class API
^^^^^^^^^^^^^^^^

The :class:`Parser` class, imported from the :mod:`email.parser` module,
provides an API that can be used to parse a message when the complete contents
of the message are available in a string or file.  The :mod:`email.parser`
module also provides a second class, called :class:`HeaderParser` which can be
used if you're only interested in the headers of the message.
:class:`HeaderParser` can be much faster in these situations, since it does not
attempt to parse the message body, instead setting the payload to the raw body
as a string. :class:`HeaderParser` has the same API as the :class:`Parser`
class.


.. class:: Parser([_class])

   The constructor for the :class:`Parser` class takes an optional argument
   *_class*.  This must be a callable factory (such as a function or a class), and
   it is used whenever a sub-message object needs to be created.  It defaults to
   :class:`~email.message.Message` (see :mod:`email.message`).  The factory will
   be called without arguments.

   The optional *strict* flag is ignored.

   .. deprecated:: 2.4
      Because the :class:`Parser` class is a backward compatible API wrapper
      around the new-in-Python 2.4 :class:`FeedParser`, *all* parsing is
      effectively non-strict.  You should simply stop passing a *strict* flag to
      the :class:`Parser` constructor.

   .. versionchanged:: 2.2.2
      The *strict* flag was added.

   .. versionchanged:: 2.4
      The *strict* flag was deprecated.

   The other public :class:`Parser` methods are:


   .. method:: parse(fp[, headersonly])

      Read all the data from the file-like object *fp*, parse the resulting
      text, and return the root message object.  *fp* must support both the
      :meth:`readline` and the :meth:`read` methods on file-like objects.

      The text contained in *fp* must be formatted as a block of :rfc:`2822`
      style headers and header continuation lines, optionally preceded by a
      envelope header.  The header block is terminated either by the end of the
      data or by a blank line.  Following the header block is the body of the
      message (which may contain MIME-encoded subparts).

      Optional *headersonly* is a flag specifying whether to stop parsing after
      reading the headers or not.  The default is ``False``, meaning it parses
      the entire contents of the file.

      .. versionchanged:: 2.2.2
         The *headersonly* flag was added.


   .. method:: parsestr(text[, headersonly])

      Similar to the :meth:`parse` method, except it takes a string object
      instead of a file-like object.  Calling this method on a string is exactly
      equivalent to wrapping *text* in a :class:`StringIO` instance first and
      calling :meth:`parse`.

      Optional *headersonly* is as with the :meth:`parse` method.

      .. versionchanged:: 2.2.2
         The *headersonly* flag was added.

Since creating a message object structure from a string or a file object is such
a common task, two functions are provided as a convenience.  They are available
in the top-level :mod:`email` package namespace.

.. currentmodule:: email

.. function:: message_from_string(s[, _class[, strict]])

   Return a message object structure from a string.  This is exactly equivalent to
   ``Parser().parsestr(s)``.  Optional *_class* and *strict* are interpreted as
   with the :class:`Parser` class constructor.

   .. versionchanged:: 2.2.2
      The *strict* flag was added.


.. function:: message_from_file(fp[, _class[, strict]])

   Return a message object structure tree from an open file object.  This is
   exactly equivalent to ``Parser().parse(fp)``.  Optional *_class* and *strict*
   are interpreted as with the :class:`Parser` class constructor.

   .. versionchanged:: 2.2.2
      The *strict* flag was added.

Here's an example of how you might use this at an interactive Python prompt::

   >>> import email
   >>> msg = email.message_from_string(myString)


Additional notes
^^^^^^^^^^^^^^^^

Here are some notes on the parsing semantics:

* Most non-\ :mimetype:`multipart` type messages are parsed as a single message
  object with a string payload.  These objects will return ``False`` for
  :meth:`is_multipart`.  Their :meth:`get_payload` method will return a string
  object.

* All :mimetype:`multipart` type messages will be parsed as a container message
  object with a list of sub-message objects for their payload.  The outer
  container message will return ``True`` for :meth:`is_multipart` and their
  :meth:`get_payload` method will return the list of :class:`~email.message.Message`
  subparts.

* Most messages with a content type of :mimetype:`message/\*` (e.g.
  :mimetype:`message/delivery-status` and :mimetype:`message/rfc822`) will also be
  parsed as container object containing a list payload of length 1.  Their
  :meth:`is_multipart` method will return ``True``.  The single element in the
  list payload will be a sub-message object.

* Some non-standards compliant messages may not be internally consistent about
  their :mimetype:`multipart`\ -edness.  Such messages may have a
  :mailheader:`Content-Type` header of type :mimetype:`multipart`, but their
  :meth:`is_multipart` method may return ``False``.  If such messages were parsed
  with the :class:`FeedParser`, they will have an instance of the
  :class:`MultipartInvariantViolationDefect` class in their *defects* attribute
  list.  See :mod:`email.errors` for details.

.. rubric:: Footnotes

.. [#] As of email package version 3.0, introduced in Python 2.4, the classic
   :class:`Parser` was re-implemented in terms of the :class:`FeedParser`, so the
   semantics and results are identical between the two parsers.

