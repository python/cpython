:mod:`binhex` --- Encode and decode binhex4 files
=================================================

.. module:: binhex
   :synopsis: Encode and decode files in binhex4 format.

**Source code:** :source:`Lib/binhex.py`

--------------

This module encodes and decodes files in binhex4 format, a format allowing
representation of Macintosh files in ASCII. Only the data fork is handled.

The :mod:`binhex` module defines the following functions:


.. function:: binhex(input, output)

   Convert a binary file with filename *input* to binhex file *output*. The
   *output* parameter can either be a filename or a file-like object (any object
   supporting a :meth:`write` and :meth:`close` method).


.. function:: hexbin(input, output)

   Decode a binhex file *input*. *input* may be a filename or a file-like object
   supporting :meth:`read` and :meth:`close` methods. The resulting file is written
   to a file named *output*, unless the argument is ``None`` in which case the
   output filename is read from the binhex file.

The following exception is also defined:


.. exception:: Error

   Exception raised when something can't be encoded using the binhex format (for
   example, a filename is too long to fit in the filename field), or when input is
   not properly encoded binhex data.


.. seealso::

   Module :mod:`binascii`
      Support module containing ASCII-to-binary and binary-to-ASCII conversions.


.. _binhex-notes:

Notes
-----

There is an alternative, more powerful interface to the coder and decoder, see
the source for details.

If you code or decode textfiles on non-Macintosh platforms they will still use
the old Macintosh newline convention (carriage-return as end of line).

