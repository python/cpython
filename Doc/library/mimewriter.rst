:mod:`MimeWriter` --- Generic MIME file writer
==============================================

.. module:: MimeWriter
   :synopsis: Write MIME format files.

.. sectionauthor:: Christopher G. Petrilli <petrilli@amber.org>


.. deprecated:: 2.3
   The :mod:`email` package should be used in preference to the :mod:`MimeWriter`
   module.  This module is present only to maintain backward compatibility.

This module defines the class :class:`MimeWriter`.  The :class:`MimeWriter`
class implements a basic formatter for creating MIME multi-part files.  It
doesn't seek around the output file nor does it use large amounts of buffer
space. You must write the parts out in the order that they should occur in the
final file. :class:`MimeWriter` does buffer the headers you add, allowing you
to rearrange their order.


.. class:: MimeWriter(fp)

   Return a new instance of the :class:`MimeWriter` class.  The only argument
   passed, *fp*, is a file object to be used for writing. Note that a
   :class:`StringIO` object could also be used.


.. _mimewriter-objects:

MimeWriter Objects
------------------

:class:`MimeWriter` instances have the following methods:


.. method:: MimeWriter.addheader(key, value[, prefix])

   Add a header line to the MIME message. The *key* is the name of the header,
   where the *value* obviously provides the value of the header. The optional
   argument *prefix* determines where the header  is inserted; ``0`` means append
   at the end, ``1`` is insert at the start. The default is to append.


.. method:: MimeWriter.flushheaders()

   Causes all headers accumulated so far to be written out (and forgotten). This is
   useful if you don't need a body part at all, e.g. for a subpart of type
   :mimetype:`message/rfc822` that's (mis)used to store some header-like
   information.


.. method:: MimeWriter.startbody(ctype[, plist[, prefix]])

   Returns a file-like object which can be used to write to the body of the
   message.  The content-type is set to the provided *ctype*, and the optional
   parameter *plist* provides additional parameters for the content-type
   declaration. *prefix* functions as in :meth:`addheader` except that the default
   is to insert at the start.


.. method:: MimeWriter.startmultipartbody(subtype[, boundary[, plist[, prefix]]])

   Returns a file-like object which can be used to write to the body of the
   message.  Additionally, this method initializes the multi-part code, where
   *subtype* provides the multipart subtype, *boundary* may provide a user-defined
   boundary specification, and *plist* provides optional parameters for the
   subtype. *prefix* functions as in :meth:`startbody`.  Subparts should be created
   using :meth:`nextpart`.


.. method:: MimeWriter.nextpart()

   Returns a new instance of :class:`MimeWriter` which represents an individual
   part in a multipart message.  This may be used to write the  part as well as
   used for creating recursively complex multipart messages. The message must first
   be initialized with :meth:`startmultipartbody` before using :meth:`nextpart`.


.. method:: MimeWriter.lastpart()

   This is used to designate the last part of a multipart message, and should
   *always* be used when writing multipart messages.

