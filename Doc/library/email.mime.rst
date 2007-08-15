:mod:`email`: Creating email and MIME objects from scratch
----------------------------------------------------------

.. module:: email.mime
   :synopsis: Build MIME messages. 


Ordinarily, you get a message object structure by passing a file or some text to
a parser, which parses the text and returns the root message object.  However
you can also build a complete message structure from scratch, or even individual
:class:`Message` objects by hand.  In fact, you can also take an existing
structure and add new :class:`Message` objects, move them around, etc.  This
makes a very convenient interface for slicing-and-dicing MIME messages.

You can create a new object structure by creating :class:`Message` instances,
adding attachments and all the appropriate headers manually.  For MIME messages
though, the :mod:`email` package provides some convenient subclasses to make
things easier.

Here are the classes:


.. class:: MIMEBase(_maintype, _subtype, **_params)

   Module: :mod:`email.mime.base`

   This is the base class for all the MIME-specific subclasses of :class:`Message`.
   Ordinarily you won't create instances specifically of :class:`MIMEBase`,
   although you could.  :class:`MIMEBase` is provided primarily as a convenient
   base class for more specific MIME-aware subclasses.

   *_maintype* is the :mailheader:`Content-Type` major type (e.g. :mimetype:`text`
   or :mimetype:`image`), and *_subtype* is the :mailheader:`Content-Type` minor
   type  (e.g. :mimetype:`plain` or :mimetype:`gif`).  *_params* is a parameter
   key/value dictionary and is passed directly to :meth:`Message.add_header`.

   The :class:`MIMEBase` class always adds a :mailheader:`Content-Type` header
   (based on *_maintype*, *_subtype*, and *_params*), and a
   :mailheader:`MIME-Version` header (always set to ``1.0``).


.. class:: MIMENonMultipart()

   Module: :mod:`email.mime.nonmultipart`

   A subclass of :class:`MIMEBase`, this is an intermediate base class for MIME
   messages that are not :mimetype:`multipart`.  The primary purpose of this class
   is to prevent the use of the :meth:`attach` method, which only makes sense for
   :mimetype:`multipart` messages.  If :meth:`attach` is called, a
   :exc:`MultipartConversionError` exception is raised.

   .. versionadded:: 2.2.2


.. class:: MIMEMultipart([subtype[, boundary[, _subparts[, _params]]]])

   Module: :mod:`email.mime.multipart`

   A subclass of :class:`MIMEBase`, this is an intermediate base class for MIME
   messages that are :mimetype:`multipart`.  Optional *_subtype* defaults to
   :mimetype:`mixed`, but can be used to specify the subtype of the message.  A
   :mailheader:`Content-Type` header of :mimetype:`multipart/`*_subtype* will be
   added to the message object.  A :mailheader:`MIME-Version` header will also be
   added.

   Optional *boundary* is the multipart boundary string.  When ``None`` (the
   default), the boundary is calculated when needed.

   *_subparts* is a sequence of initial subparts for the payload.  It must be
   possible to convert this sequence to a list.  You can always attach new subparts
   to the message by using the :meth:`Message.attach` method.

   Additional parameters for the :mailheader:`Content-Type` header are taken from
   the keyword arguments, or passed into the *_params* argument, which is a keyword
   dictionary.

   .. versionadded:: 2.2.2


.. class:: MIMEApplication(_data[, _subtype[, _encoder[, **_params]]])

   Module: :mod:`email.mime.application`

   A subclass of :class:`MIMENonMultipart`, the :class:`MIMEApplication` class is
   used to represent MIME message objects of major type :mimetype:`application`.
   *_data* is a string containing the raw byte data.  Optional *_subtype* specifies
   the MIME subtype and defaults to :mimetype:`octet-stream`.

   Optional *_encoder* is a callable (i.e. function) which will perform the actual
   encoding of the data for transport.  This callable takes one argument, which is
   the :class:`MIMEApplication` instance. It should use :meth:`get_payload` and
   :meth:`set_payload` to change the payload to encoded form.  It should also add
   any :mailheader:`Content-Transfer-Encoding` or other headers to the message
   object as necessary.  The default encoding is base64.  See the
   :mod:`email.encoders` module for a list of the built-in encoders.

   *_params* are passed straight through to the base class constructor.

   .. versionadded:: 2.5


.. class:: MIMEAudio(_audiodata[, _subtype[, _encoder[, **_params]]])

   Module: :mod:`email.mime.audio`

   A subclass of :class:`MIMENonMultipart`, the :class:`MIMEAudio` class is used to
   create MIME message objects of major type :mimetype:`audio`. *_audiodata* is a
   string containing the raw audio data.  If this data can be decoded by the
   standard Python module :mod:`sndhdr`, then the subtype will be automatically
   included in the :mailheader:`Content-Type` header.  Otherwise you can explicitly
   specify the audio subtype via the *_subtype* parameter.  If the minor type could
   not be guessed and *_subtype* was not given, then :exc:`TypeError` is raised.

   Optional *_encoder* is a callable (i.e. function) which will perform the actual
   encoding of the audio data for transport.  This callable takes one argument,
   which is the :class:`MIMEAudio` instance. It should use :meth:`get_payload` and
   :meth:`set_payload` to change the payload to encoded form.  It should also add
   any :mailheader:`Content-Transfer-Encoding` or other headers to the message
   object as necessary.  The default encoding is base64.  See the
   :mod:`email.encoders` module for a list of the built-in encoders.

   *_params* are passed straight through to the base class constructor.


.. class:: MIMEImage(_imagedata[, _subtype[, _encoder[, **_params]]])

   Module: :mod:`email.mime.image`

   A subclass of :class:`MIMENonMultipart`, the :class:`MIMEImage` class is used to
   create MIME message objects of major type :mimetype:`image`. *_imagedata* is a
   string containing the raw image data.  If this data can be decoded by the
   standard Python module :mod:`imghdr`, then the subtype will be automatically
   included in the :mailheader:`Content-Type` header.  Otherwise you can explicitly
   specify the image subtype via the *_subtype* parameter.  If the minor type could
   not be guessed and *_subtype* was not given, then :exc:`TypeError` is raised.

   Optional *_encoder* is a callable (i.e. function) which will perform the actual
   encoding of the image data for transport.  This callable takes one argument,
   which is the :class:`MIMEImage` instance. It should use :meth:`get_payload` and
   :meth:`set_payload` to change the payload to encoded form.  It should also add
   any :mailheader:`Content-Transfer-Encoding` or other headers to the message
   object as necessary.  The default encoding is base64.  See the
   :mod:`email.encoders` module for a list of the built-in encoders.

   *_params* are passed straight through to the :class:`MIMEBase` constructor.


.. class:: MIMEMessage(_msg[, _subtype])

   Module: :mod:`email.mime.message`

   A subclass of :class:`MIMENonMultipart`, the :class:`MIMEMessage` class is used
   to create MIME objects of main type :mimetype:`message`. *_msg* is used as the
   payload, and must be an instance of class :class:`Message` (or a subclass
   thereof), otherwise a :exc:`TypeError` is raised.

   Optional *_subtype* sets the subtype of the message; it defaults to
   :mimetype:`rfc822`.


.. class:: MIMEText(_text[, _subtype[, _charset]])

   Module: :mod:`email.mime.text`

   A subclass of :class:`MIMENonMultipart`, the :class:`MIMEText` class is used to
   create MIME objects of major type :mimetype:`text`. *_text* is the string for
   the payload.  *_subtype* is the minor type and defaults to :mimetype:`plain`.
   *_charset* is the character set of the text and is passed as a parameter to the
   :class:`MIMENonMultipart` constructor; it defaults to ``us-ascii``.  No guessing
   or encoding is performed on the text data.

   .. versionchanged:: 2.4
      The previously deprecated *_encoding* argument has been removed.  Encoding
      happens implicitly based on the *_charset* argument.

