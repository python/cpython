:mod:`email.encoders`: Encoders
-------------------------------

.. module:: email.encoders
   :synopsis: Encoders for email message payloads.

**Source code:** :source:`Lib/email/encoders.py`

--------------

This module is part of the legacy (``Compat32``) email API.  In the
new API the functionality is provided by the *cte* parameter of
the :meth:`~email.message.EmailMessage.set_content` method.

The remaining text in this section is the original documentation of the module.

When creating :class:`~email.message.Message` objects from scratch, you often
need to encode the payloads for transport through compliant mail servers. This
is especially true for :mimetype:`image/\*` and :mimetype:`text/\*` type messages
containing binary data.

The :mod:`email` package provides some convenient encodings in its
:mod:`encoders` module.  These encoders are actually used by the
:class:`~email.mime.audio.MIMEAudio` and :class:`~email.mime.image.MIMEImage`
class constructors to provide default encodings.  All encoder functions take
exactly one argument, the message object to encode.  They usually extract the
payload, encode it, and reset the payload to this newly encoded value.  They
should also set the :mailheader:`Content-Transfer-Encoding` header as appropriate.

Note that these functions are not meaningful for a multipart message.  They
must be applied to individual subparts instead, and will raise a
:exc:`TypeError` if passed a message whose type is multipart.

Here are the encoding functions provided:


.. function:: encode_quopri(msg)

   Encodes the payload into quoted-printable form and sets the
   :mailheader:`Content-Transfer-Encoding` header to ``quoted-printable`` [#]_.
   This is a good encoding to use when most of your payload is normal printable
   data, but contains a few unprintable characters.


.. function:: encode_base64(msg)

   Encodes the payload into base64 form and sets the
   :mailheader:`Content-Transfer-Encoding` header to ``base64``.  This is a good
   encoding to use when most of your payload is unprintable data since it is a more
   compact form than quoted-printable.  The drawback of base64 encoding is that it
   renders the text non-human readable.


.. function:: encode_7or8bit(msg)

   This doesn't actually modify the message's payload, but it does set the
   :mailheader:`Content-Transfer-Encoding` header to either ``7bit`` or ``8bit`` as
   appropriate, based on the payload data.


.. function:: encode_noop(msg)

   This does nothing; it doesn't even set the
   :mailheader:`Content-Transfer-Encoding` header.

.. rubric:: Footnotes

.. [#] Note that encoding with :meth:`encode_quopri` also encodes all tabs and space
   characters in the data.

