
:mod:`uu` --- Encode and decode uuencode files
==============================================

.. module:: uu
   :synopsis: Encode and decode files in uuencode format.
.. moduleauthor:: Lance Ellinghouse


This module encodes and decodes files in uuencode format, allowing arbitrary
binary data to be transferred over ASCII-only connections. Wherever a file
argument is expected, the methods accept a file-like object.  For backwards
compatibility, a string containing a pathname is also accepted, and the
corresponding file will be opened for reading and writing; the pathname ``'-'``
is understood to mean the standard input or output.  However, this interface is
deprecated; it's better for the caller to open the file itself, and be sure
that, when required, the mode is ``'rb'`` or ``'wb'`` on Windows.

.. index::
   single: Jansen, Jack
   single: Ellinghouse, Lance

This code was contributed by Lance Ellinghouse, and modified by Jack Jansen.

The :mod:`uu` module defines the following functions:


.. function:: encode(in_file, out_file[, name[, mode]])

   Uuencode file *in_file* into file *out_file*.  The uuencoded file will have the
   header specifying *name* and *mode* as the defaults for the results of decoding
   the file. The default defaults are taken from *in_file*, or ``'-'`` and ``0666``
   respectively.


.. function:: decode(in_file[, out_file[, mode[, quiet]]])

   This call decodes uuencoded file *in_file* placing the result on file
   *out_file*. If *out_file* is a pathname, *mode* is used to set the permission
   bits if the file must be created. Defaults for *out_file* and *mode* are taken
   from the uuencode header.  However, if the file specified in the header already
   exists, a :exc:`uu.Error` is raised.

   :func:`decode` may print a warning to standard error if the input was produced
   by an incorrect uuencoder and Python could recover from that error.  Setting
   *quiet* to a true value silences this warning.


.. exception:: Error()

   Subclass of :exc:`Exception`, this can be raised by :func:`uu.decode` under
   various situations, such as described above, but also including a badly
   formatted header, or truncated input file.


.. seealso::

   Module :mod:`binascii`
      Support module containing ASCII-to-binary and binary-to-ASCII conversions.

