
:mod:`mimetools` --- Tools for parsing MIME messages
====================================================

.. module:: mimetools
   :synopsis: Tools for parsing MIME-style message bodies.
   :deprecated:


.. deprecated:: 2.3
   The :mod:`email` package should be used in preference to the :mod:`mimetools`
   module.  This module is present only to maintain backward compatibility.

.. index:: module: rfc822

This module defines a subclass of the :mod:`rfc822` module's :class:`Message`
class and a number of utility functions that are useful for the manipulation for
MIME multipart or encoded message.

It defines the following items:


.. class:: Message(fp[, seekable])

   Return a new instance of the :class:`Message` class.  This is a subclass of the
   :class:`rfc822.Message` class, with some additional methods (see below).  The
   *seekable* argument has the same meaning as for :class:`rfc822.Message`.


.. function:: choose_boundary()

   Return a unique string that has a high likelihood of being usable as a part
   boundary.  The string has the form ``'hostipaddr.uid.pid.timestamp.random'``.


.. function:: decode(input, output, encoding)

   Read data encoded using the allowed MIME *encoding* from open file object
   *input* and write the decoded data to open file object *output*.  Valid values
   for *encoding* include ``'base64'``, ``'quoted-printable'``, ``'uuencode'``,
   ``'x-uuencode'``, ``'uue'``, ``'x-uue'``, ``'7bit'``, and  ``'8bit'``.  Decoding
   messages encoded in ``'7bit'`` or ``'8bit'`` has no effect.  The input is simply
   copied to the output.


.. function:: encode(input, output, encoding)

   Read data from open file object *input* and write it encoded using the allowed
   MIME *encoding* to open file object *output*. Valid values for *encoding* are
   the same as for :meth:`decode`.


.. function:: copyliteral(input, output)

   Read lines from open file *input* until EOF and write them to open file
   *output*.


.. function:: copybinary(input, output)

   Read blocks until EOF from open file *input* and write them to open file
   *output*.  The block size is currently fixed at 8192.


.. seealso::

   Module :mod:`email`
      Comprehensive email handling package; supersedes the :mod:`mimetools` module.

   Module :mod:`rfc822`
      Provides the base class for :class:`mimetools.Message`.

   Module :mod:`multifile`
      Support for reading files which contain distinct parts, such as MIME data.

   http://faqs.cs.uu.nl/na-dir/mail/mime-faq/.html
      The MIME Frequently Asked Questions document.  For an overview of MIME, see the
      answer to question 1.1 in Part 1 of this document.


.. _mimetools-message-objects:

Additional Methods of Message Objects
-------------------------------------

The :class:`Message` class defines the following methods in addition to the
:class:`rfc822.Message` methods:


.. method:: Message.getplist()

   Return the parameter list of the :mailheader:`Content-Type` header. This is a
   list of strings.  For parameters of the form ``key=value``, *key* is converted
   to lower case but *value* is not.  For example, if the message contains the
   header ``Content-type: text/html; spam=1; Spam=2; Spam`` then :meth:`getplist`
   will return the Python list ``['spam=1', 'spam=2', 'Spam']``.


.. method:: Message.getparam(name)

   Return the *value* of the first parameter (as returned by :meth:`getplist`) of
   the form ``name=value`` for the given *name*.  If *value* is surrounded by
   quotes of the form '``<``...\ ``>``' or '``"``...\ ``"``', these are removed.


.. method:: Message.getencoding()

   Return the encoding specified in the :mailheader:`Content-Transfer-Encoding`
   message header.  If no such header exists, return ``'7bit'``.  The encoding is
   converted to lower case.


.. method:: Message.gettype()

   Return the message type (of the form ``type/subtype``) as specified in the
   :mailheader:`Content-Type` header.  If no such header exists, return
   ``'text/plain'``.  The type is converted to lower case.


.. method:: Message.getmaintype()

   Return the main type as specified in the :mailheader:`Content-Type` header.  If
   no such header exists, return ``'text'``.  The main type is converted to lower
   case.


.. method:: Message.getsubtype()

   Return the subtype as specified in the :mailheader:`Content-Type` header.  If no
   such header exists, return ``'plain'``.  The subtype is converted to lower case.

