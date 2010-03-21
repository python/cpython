:mod:`gzip` --- Support for :program:`gzip` files
=================================================

.. module:: gzip
   :synopsis: Interfaces for gzip compression and decompression using file objects.

This module provides a simple interface to compress and decompress files just
like the GNU programs :program:`gzip` and :program:`gunzip` would.

The data compression is provided by the :mod:`zlib` module.

The :mod:`gzip` module provides the :class:`GzipFile` class which is modeled
after Python's File Object. The :class:`GzipFile` class reads and writes
:program:`gzip`\ -format files, automatically compressing or decompressing the
data so that it looks like an ordinary file object.

Note that additional file formats which can be decompressed by the
:program:`gzip` and :program:`gunzip` programs, such  as those produced by
:program:`compress` and :program:`pack`, are not supported by this module.

For other archive formats, see the :mod:`bz2`, :mod:`zipfile`, and
:mod:`tarfile` modules.

The module defines the following items:


.. class:: GzipFile([filename[, mode[, compresslevel[, fileobj[, mtime]]]]])

   Constructor for the :class:`GzipFile` class, which simulates most of the methods
   of a file object, with the exception of the :meth:`readinto` and
   :meth:`truncate` methods.  At least one of *fileobj* and *filename* must be
   given a non-trivial value.

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
   data.  This also allows you to pass a :class:`StringIO` object opened for
   writing as *fileobj*, and retrieve the resulting memory buffer using the
   :class:`StringIO` object's :meth:`getvalue` method.

   :class:`GzipFile` supports iteration and the :keyword:`with` statement.

   .. versionchanged:: 2.7
      Support for the :keyword:`with` statement was added.

   .. versionchanged:: 2.7
      Support for zero-padded files was added.


.. function:: open(filename[, mode[, compresslevel]])

   This is a shorthand for ``GzipFile(filename,`` ``mode,`` ``compresslevel)``.
   The *filename* argument is required; *mode* defaults to ``'rb'`` and
   *compresslevel* defaults to ``9``.


.. _gzip-usage-examples:

Examples of usage
-----------------

Example of how to read a compressed file::

   import gzip
   f = gzip.open('/home/joe/file.txt.gz', 'rb')
   file_content = f.read()
   f.close()

Example of how to create a compressed GZIP file::

   import gzip
   content = "Lots of content here"
   f = gzip.open('/home/joe/file.txt.gz', 'wb')
   f.write(content)
   f.close()

Example of how to GZIP compress an existing file::

   import gzip
   f_in = open('/home/joe/file.txt', 'rb')
   f_out = gzip.open('/home/joe/file.txt.gz', 'wb')
   f_out.writelines(f_in)
   f_out.close()
   f_in.close()


.. seealso::

   Module :mod:`zlib`
      The basic data compression module needed to support the :program:`gzip` file
      format.

