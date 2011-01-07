:mod:`email`: Generating MIME documents
---------------------------------------

.. module:: email.generator
   :synopsis: Generate flat text email messages from a message structure.


One of the most common tasks is to generate the flat text of the email message
represented by a message object structure.  You will need to do this if you want
to send your message via the :mod:`smtplib` module or the :mod:`nntplib` module,
or print the message on the console.  Taking a message object structure and
producing a flat text document is the job of the :class:`Generator` class.

Again, as with the :mod:`email.parser` module, you aren't limited to the
functionality of the bundled generator; you could write one from scratch
yourself.  However the bundled generator knows how to generate most email in a
standards-compliant way, should handle MIME and non-MIME email messages just
fine, and is designed so that the transformation from flat text, to a message
structure via the :class:`~email.parser.Parser` class, and back to flat text,
is idempotent (the input is identical to the output).  On the other hand, using
the Generator on a :class:`~email.message.Message` constructed by program may
result in changes to the :class:`~email.message.Message` object as defaults are
filled in.

:class:`bytes` output can be generated using the :class:`BytesGenerator` class.
If the message object structure contains non-ASCII bytes, this generator's
:meth:`~BytesGenerator.flatten` method will emit the original bytes.  Parsing a
binary message and then flattening it with :class:`BytesGenerator` should be
idempotent for standards compliant messages.

Here are the public methods of the :class:`Generator` class, imported from the
:mod:`email.generator` module:


.. class:: Generator(outfp, mangle_from_=True, maxheaderlen=78)

   The constructor for the :class:`Generator` class takes a :term:`file-like object`
   called *outfp* for an argument.  *outfp* must support the :meth:`write` method
   and be usable as the output file for the :func:`print` function.

   Optional *mangle_from_* is a flag that, when ``True``, puts a ``>`` character in
   front of any line in the body that starts exactly as ``From``, i.e. ``From``
   followed by a space at the beginning of the line.  This is the only guaranteed
   portable way to avoid having such lines be mistaken for a Unix mailbox format
   envelope header separator (see `WHY THE CONTENT-LENGTH FORMAT IS BAD
   <http://www.jwz.org/doc/content-length.html>`_ for details).  *mangle_from_*
   defaults to ``True``, but you might want to set this to ``False`` if you are not
   writing Unix mailbox format files.

   Optional *maxheaderlen* specifies the longest length for a non-continued header.
   When a header line is longer than *maxheaderlen* (in characters, with tabs
   expanded to 8 spaces), the header will be split as defined in the
   :class:`~email.header.Header` class.  Set to zero to disable header wrapping.
   The default is 78, as recommended (but not required) by :rfc:`2822`.

   The other public :class:`Generator` methods are:


   .. method:: flatten(msg, unixfrom=False, linesep='\\n')

      Print the textual representation of the message object structure rooted at
      *msg* to the output file specified when the :class:`Generator` instance
      was created.  Subparts are visited depth-first and the resulting text will
      be properly MIME encoded.

      Optional *unixfrom* is a flag that forces the printing of the envelope
      header delimiter before the first :rfc:`2822` header of the root message
      object.  If the root object has no envelope header, a standard one is
      crafted.  By default, this is set to ``False`` to inhibit the printing of
      the envelope delimiter.

      Note that for subparts, no envelope header is ever printed.

      Optional *linesep* specifies the line separator character used to
      terminate lines in the output.  It defaults to ``\n`` because that is
      the most useful value for Python application code (other library packages
      expect ``\n`` separated lines).  ``linesep=\r\n`` can be used to
      generate output with RFC-compliant line separators.

      Messages parsed with a Bytes parser that have a
      :mailheader:`Content-Transfer-Encoding` of 8bit will be converted to a
      use a 7bit Content-Transfer-Encoding.  Non-ASCII bytes in the headers
      will be :rfc:`2047` encoded with a charset of `unknown-8bit`.

      .. versionchanged:: 3.2
         Added support for re-encoding 8bit message bodies, and the *linesep*
         argument.

   .. method:: clone(fp)

      Return an independent clone of this :class:`Generator` instance with the
      exact same options.

   .. method:: write(s)

      Write the string *s* to the underlying file object, i.e. *outfp* passed to
      :class:`Generator`'s constructor.  This provides just enough file-like API
      for :class:`Generator` instances to be used in the :func:`print` function.

As a convenience, see the :class:`~email.message.Message` methods
:meth:`~email.message.Message.as_string` and ``str(aMessage)``, a.k.a.
:meth:`~email.message.Message.__str__`, which simplify the generation of a
formatted string representation of a message object.  For more detail, see
:mod:`email.message`.

.. class:: BytesGenerator(outfp, mangle_from_=True, maxheaderlen=78)

   The constructor for the :class:`BytesGenerator` class takes a binary
   :term:`file-like object` called *outfp* for an argument.  *outfp* must
   support a :meth:`write` method that accepts binary data.

   Optional *mangle_from_* is a flag that, when ``True``, puts a ``>``
   character in front of any line in the body that starts exactly as ``From``,
   i.e. ``From`` followed by a space at the beginning of the line.  This is the
   only guaranteed portable way to avoid having such lines be mistaken for a
   Unix mailbox format envelope header separator (see `WHY THE CONTENT-LENGTH
   FORMAT IS BAD <http://www.jwz.org/doc/content-length.html>`_ for details).
   *mangle_from_* defaults to ``True``, but you might want to set this to
   ``False`` if you are not writing Unix mailbox format files.

   Optional *maxheaderlen* specifies the longest length for a non-continued
   header.  When a header line is longer than *maxheaderlen* (in characters,
   with tabs expanded to 8 spaces), the header will be split as defined in the
   :class:`~email.header.Header` class.  Set to zero to disable header
   wrapping.  The default is 78, as recommended (but not required) by
   :rfc:`2822`.

   The other public :class:`BytesGenerator` methods are:


   .. method:: flatten(msg, unixfrom=False, linesep='\n')

      Print the textual representation of the message object structure rooted
      at *msg* to the output file specified when the :class:`BytesGenerator`
      instance was created.  Subparts are visited depth-first and the resulting
      text will be properly MIME encoded.  If the input that created the *msg*
      contained bytes with the high bit set and those bytes have not been
      modified, they will be copied faithfully to the output, even if doing so
      is not strictly RFC compliant.  (To produce strictly RFC compliant
      output, use the :class:`Generator` class.)

      Messages parsed with a Bytes parser that have a
      :mailheader:`Content-Transfer-Encoding` of 8bit will be reconstructed
      as 8bit if they have not been modified.

      Optional *unixfrom* is a flag that forces the printing of the envelope
      header delimiter before the first :rfc:`2822` header of the root message
      object.  If the root object has no envelope header, a standard one is
      crafted.  By default, this is set to ``False`` to inhibit the printing of
      the envelope delimiter.

      Note that for subparts, no envelope header is ever printed.

      Optional *linesep* specifies the line separator character used to
      terminate lines in the output.  It defaults to ``\n`` because that is
      the most useful value for Python application code (other library packages
      expect ``\n`` separated lines).  ``linesep=\r\n`` can be used to
      generate output with RFC-compliant line separators.

   .. method:: clone(fp)

      Return an independent clone of this :class:`BytesGenerator` instance with
      the exact same options.

   .. method:: write(s)

      Write the string *s* to the underlying file object.  *s* is encoded using
      the ``ASCII`` codec and written to the *write* method of the  *outfp*
      *outfp* passed to the :class:`BytesGenerator`'s constructor.  This
      provides just enough file-like API for :class:`BytesGenerator` instances
      to be used in the :func:`print` function.

   .. versionadded:: 3.2

The :mod:`email.generator` module also provides a derived class, called
:class:`DecodedGenerator` which is like the :class:`Generator` base class,
except that non-\ :mimetype:`text` parts are substituted with a format string
representing the part.


.. class:: DecodedGenerator(outfp[, mangle_from_=True, maxheaderlen=78, fmt=None)

   This class, derived from :class:`Generator` walks through all the subparts of a
   message.  If the subpart is of main type :mimetype:`text`, then it prints the
   decoded payload of the subpart. Optional *_mangle_from_* and *maxheaderlen* are
   as with the :class:`Generator` base class.

   If the subpart is not of main type :mimetype:`text`, optional *fmt* is a format
   string that is used instead of the message payload. *fmt* is expanded with the
   following keywords, ``%(keyword)s`` format:

   * ``type`` -- Full MIME type of the non-\ :mimetype:`text` part

   * ``maintype`` -- Main MIME type of the non-\ :mimetype:`text` part

   * ``subtype`` -- Sub-MIME type of the non-\ :mimetype:`text` part

   * ``filename`` -- Filename of the non-\ :mimetype:`text` part

   * ``description`` -- Description associated with the non-\ :mimetype:`text` part

   * ``encoding`` -- Content transfer encoding of the non-\ :mimetype:`text` part

   The default value for *fmt* is ``None``, meaning ::

      [Non-text (%(type)s) part of message omitted, filename %(filename)s]
