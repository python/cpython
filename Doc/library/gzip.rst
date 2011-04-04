:mod:`gzip` --- Support for :program:`gzip` files
=================================================

.. module:: gzip
   :synopsis: Interfaces for gzip compression and decompression using file objects.

**Source code:** :source:`Lib/gzip.py`

--------------

This module provides a simple interface to compress and decompress files just
like the GNU programs :program:`gzip` and :program:`gunzip` would.

The data compression is provided by the :mod:`zlib` module.

The :mod:`gzip` module provides the :class:`GzipFile` class. The :class:`GzipFile`
class reads and writes :program:`gzip`\ -format files, automatically compressing
or decompressing the data so that it looks like an ordinary :term:`file object`.

Note that additional file formats which can be decompressed by the
:program:`gzip` and :program:`gunzip` programs, such  as those produced by
:program:`compress` and :program:`pack`, are not supported by this module.

For other archive formats, see the :mod:`bz2`, :mod:`zipfile`, and
:mod:`tarfile` modules.

The module defines the following items:


.. class:: GzipFile(filename=None, mode=None, compresslevel=9, fileobj=None, mtime=None)

   Constructor for the :class:`GzipFile` class, which simulates most of the
   methods of a :term:`file object`, with the exception of the :meth:`truncate`
   method.  At least one of *fileobj* and *filename* must be given a non-trivial
   value.

   The new class instance is based on *fileobj*, which can be a regular file, a
   :class:`StringIO` object, or any other object which simulates a file.  It
   defaults to ``None``, in which case *filename* is opened to provide a file
   object.

   When *fileobj* is not ``None``, the *filename* argument is only used to be
   included in the :program:`gzip` file header, which may includes the original
   filename of the uncompressed file.  It defaults to the filename of *fileobj*, if
   discernible; otherwise, it defaults to the empty string, and in this case the
   original filename is not included in the header.

   The *mode* argument can be any of ``'r'``, ``'rb'``, ``'a'``, ``'ab'``, ``'w'``,
   or ``'wb'``, depending on whether the file will be read or written.  The default
   is the mode of *fileobj* if discernible; otherwise, the default is ``'rb'``. If
   not given, the 'b' flag will be added to the mode to ensure the file is opened
   in binary mode for cross-platform portability.

   The *compresslevel* argument is an integer from ``1`` to ``9`` controlling the
   level of compression; ``1`` is fastest and produces the least compression, and
   ``9`` is slowest and produces the most compression.  The default is ``9``.

   The *mtime* argument is an optional numeric timestamp to be written to
   the stream when compressing.  All :program:`gzip` compressed streams are
   required to contain a timestamp.  If omitted or ``None``, the current
   time is used.  This module ignores the timestamp when decompressing;
   however, some programs, such as :program:`gunzip`\ , make use of it.
   The format of the timestamp is the same as that of the return value of
   ``time.time()`` and of the ``st_mtime`` member of the object returned
   by ``os.stat()``.

   Calling a :class:`GzipFile` object's :meth:`close` method does not close
   *fileobj*, since you might wish to append more material after the compressed
   data.  This also allows you to pass a :class:`io.BytesIO` object opened for
   writing as *fileobj*, and retrieve the resulting memory buffer using the
   :class:`io.BytesIO` object's :meth:`~io.BytesIO.getvalue` method.

   :class:`GzipFile` supports the :class:`io.BufferedIOBase` interface,
   including iteration and the :keyword:`with` statement.  Only the
   :meth:`truncate` method isn't implemented.

   :class:`GzipFile` also provides the following method:

   .. method:: peek([n])

      Read *n* uncompressed bytes without advancing the file position.
      At most one single read on the compressed stream is done to satisfy
      the call.  The number of bytes returned may be more or less than
      requested.

      .. versionadded:: 3.2

   .. versionchanged:: 3.1
      Support for the :keyword:`with` statement was added.

   .. versionchanged:: 3.2
      Support for zero-padded files was added.

   .. versionchanged:: 3.2
      Support for unseekable files was added.

   .. versionchanged:: 3.3
      The :meth:`io.BufferedIOBase.read1` method is now implemented.


.. function:: open(filename, mode='rb', compresslevel=9)

   This is a shorthand for ``GzipFile(filename,`` ``mode,`` ``compresslevel)``.
   The *filename* argument is required; *mode* defaults to ``'rb'`` and
   *compresslevel* defaults to ``9``.

.. function:: compress(data, compresslevel=9)

   Compress the *data*, returning a :class:`bytes` object containing
   the compressed data.  *compresslevel* has the same meaning as in
   the :class:`GzipFile` constructor above.

   .. versionadded:: 3.2

.. function:: decompress(data)

   Decompress the *data*, returning a :class:`bytes` object containing the
   uncompressed data.

   .. versionadded:: 3.2


.. _gzip-usage-examples:

Examples of usage
-----------------

Example of how to read a compressed file::

   import gzip
   with gzip.open('/home/joe/file.txt.gz', 'rb') as f:
       file_content = f.read()

Example of how to create a compressed GZIP file::

   import gzip
   content = b"Lots of content here"
   with gzip.open('/home/joe/file.txt.gz', 'wb') as f:
       f.write(content)

Example of how to GZIP compress an existing file::

   import gzip
   with open('/home/joe/file.txt', 'rb') as f_in:
       with gzip.open('/home/joe/file.txt.gz', 'wb') as f_out:
           f_out.writelines(f_in)

Example of how to GZIP compress a binary string::

   import gzip
   s_in = b"Lots of content here"
   s_out = gzip.compress(s_in)

.. seealso::

   Module :mod:`zlib`
      The basic data compression module needed to support the :program:`gzip` file
      format.

