:mod:`!gzip` --- Support for :program:`gzip` files
==================================================

.. module:: gzip
   :synopsis: Interfaces for gzip compression and decompression using file objects.

**Source code:** :source:`Lib/gzip.py`

--------------

This module provides a simple interface to compress and decompress files just
like the GNU programs :program:`gzip` and :program:`gunzip` would.

The data compression is provided by the :mod:`zlib` module.

The :mod:`gzip` module provides the :class:`GzipFile` class, as well as the
:func:`.open`, :func:`compress` and :func:`decompress` convenience functions.
The :class:`GzipFile` class reads and writes :program:`gzip`\ -format files,
automatically compressing or decompressing the data so that it looks like an
ordinary :term:`file object`.

Note that additional file formats which can be decompressed by the
:program:`gzip` and :program:`gunzip` programs, such  as those produced by
:program:`compress` and :program:`pack`, are not supported by this module.

The module defines the following items:


.. function:: open(filename, mode='rb', compresslevel=9, encoding=None, errors=None, newline=None)

   Open a gzip-compressed file in binary or text mode, returning a :term:`file
   object`.

   The *filename* argument can be an actual filename (a :class:`str` or
   :class:`bytes` object), or an existing file object to read from or write to.

   The *mode* argument can be any of ``'r'``, ``'rb'``, ``'a'``, ``'ab'``,
   ``'w'``, ``'wb'``, ``'x'`` or ``'xb'`` for binary mode, or ``'rt'``,
   ``'at'``, ``'wt'``, or ``'xt'`` for text mode. The default is ``'rb'``.

   The *compresslevel* argument is an integer from 0 to 9, as for the
   :class:`GzipFile` constructor.

   For binary mode, this function is equivalent to the :class:`GzipFile`
   constructor: ``GzipFile(filename, mode, compresslevel)``. In this case, the
   *encoding*, *errors* and *newline* arguments must not be provided.

   For text mode, a :class:`GzipFile` object is created, and wrapped in an
   :class:`io.TextIOWrapper` instance with the specified encoding, error
   handling behavior, and line ending(s).

   .. versionchanged:: 3.3
      Added support for *filename* being a file object, support for text mode,
      and the *encoding*, *errors* and *newline* arguments.

   .. versionchanged:: 3.4
      Added support for the ``'x'``, ``'xb'`` and ``'xt'`` modes.

   .. versionchanged:: 3.6
      Accepts a :term:`path-like object`.

.. exception:: BadGzipFile

   An exception raised for invalid gzip files.  It inherits from :exc:`OSError`.
   :exc:`EOFError` and :exc:`zlib.error` can also be raised for invalid gzip
   files.

   .. versionadded:: 3.8

.. class:: GzipFile(filename=None, mode=None, compresslevel=9, fileobj=None, mtime=None)

   Constructor for the :class:`GzipFile` class, which simulates most of the
   methods of a :term:`file object`, with the exception of the :meth:`~io.IOBase.truncate`
   method.  At least one of *fileobj* and *filename* must be given a non-trivial
   value.

   The new class instance is based on *fileobj*, which can be a regular file, an
   :class:`io.BytesIO` object, or any other object which simulates a file.  It
   defaults to ``None``, in which case *filename* is opened to provide a file
   object.

   When *fileobj* is not ``None``, the *filename* argument is only used to be
   included in the :program:`gzip` file header, which may include the original
   filename of the uncompressed file.  It defaults to the filename of *fileobj*, if
   discernible; otherwise, it defaults to the empty string, and in this case the
   original filename is not included in the header.

   The *mode* argument can be any of ``'r'``, ``'rb'``, ``'a'``, ``'ab'``, ``'w'``,
   ``'wb'``, ``'x'``, or ``'xb'``, depending on whether the file will be read or
   written.  The default is the mode of *fileobj* if discernible; otherwise, the
   default is ``'rb'``.  In future Python releases the mode of *fileobj* will
   not be used.  It is better to always specify *mode* for writing.

   Note that the file is always opened in binary mode. To open a compressed file
   in text mode, use :func:`.open` (or wrap your :class:`GzipFile` with an
   :class:`io.TextIOWrapper`).

   The *compresslevel* argument is an integer from ``0`` to ``9`` controlling
   the level of compression; ``1`` is fastest and produces the least
   compression, and ``9`` is slowest and produces the most compression. ``0``
   is no compression. The default is ``9``.

   The *mtime* argument is an optional numeric timestamp to be written to
   the last modification time field in the stream when compressing.  It
   should only be provided in compression mode.  If omitted or ``None``, the
   current time is used.  See the :attr:`mtime` attribute for more details.

   Calling a :class:`GzipFile` object's :meth:`!close` method does not close
   *fileobj*, since you might wish to append more material after the compressed
   data.  This also allows you to pass an :class:`io.BytesIO` object opened for
   writing as *fileobj*, and retrieve the resulting memory buffer using the
   :class:`io.BytesIO` object's :meth:`~io.BytesIO.getvalue` method.

   :class:`GzipFile` supports the :class:`io.BufferedIOBase` interface,
   including iteration and the :keyword:`with` statement.  Only the
   :meth:`~io.IOBase.truncate` method isn't implemented.

   :class:`GzipFile` also provides the following method and attribute:

   .. method:: peek(n)

      Read *n* uncompressed bytes without advancing the file position.
      At most one single read on the compressed stream is done to satisfy
      the call.  The number of bytes returned may be more or less than
      requested.

      .. note:: While calling :meth:`peek` does not change the file position of
         the :class:`GzipFile`, it may change the position of the underlying
         file object (e.g. if the :class:`GzipFile` was constructed with the
         *fileobj* parameter).

      .. versionadded:: 3.2

   .. attribute:: mtime

      When decompressing, the value of the last modification time field in
      the most recently read header may be read from this attribute, as an
      integer.  The initial value before reading any headers is ``None``.

      All :program:`gzip` compressed streams are required to contain this
      timestamp field.  Some programs, such as :program:`gunzip`\ , make use
      of the timestamp.  The format is the same as the return value of
      :func:`time.time` and the :attr:`~os.stat_result.st_mtime` attribute of
      the object returned by :func:`os.stat`.

   .. attribute:: name

      The path to the gzip file on disk, as a :class:`str` or :class:`bytes`.
      Equivalent to the output of :func:`os.fspath` on the original input path,
      with no other normalization, resolution or expansion.

   .. versionchanged:: 3.1
      Support for the :keyword:`with` statement was added, along with the
      *mtime* constructor argument and :attr:`mtime` attribute.

   .. versionchanged:: 3.2
      Support for zero-padded and unseekable files was added.

   .. versionchanged:: 3.3
      The :meth:`io.BufferedIOBase.read1` method is now implemented.

   .. versionchanged:: 3.4
      Added support for the ``'x'`` and ``'xb'`` modes.

   .. versionchanged:: 3.5
      Added support for writing arbitrary
      :term:`bytes-like objects <bytes-like object>`.
      The :meth:`~io.BufferedIOBase.read` method now accepts an argument of
      ``None``.

   .. versionchanged:: 3.6
      Accepts a :term:`path-like object`.

   .. versionchanged:: 3.12
      Remove the ``filename`` attribute, use the :attr:`~GzipFile.name`
      attribute instead.

   .. deprecated:: 3.9
      Opening :class:`GzipFile` for writing without specifying the *mode*
      argument is deprecated.


.. function:: compress(data, compresslevel=9, *, mtime=None)

   Compress the *data*, returning a :class:`bytes` object containing
   the compressed data.  *compresslevel* and *mtime* have the same meaning as in
   the :class:`GzipFile` constructor above. When *mtime* is set to ``0``, this
   function is equivalent to :func:`zlib.compress` with *wbits* set to ``31``.
   The zlib function is faster.

   .. versionadded:: 3.2
   .. versionchanged:: 3.8
      Added the *mtime* parameter for reproducible output.
   .. versionchanged:: 3.11
      Speed is improved by compressing all data at once instead of in a
      streamed fashion. Calls with *mtime* set to ``0`` are delegated to
      :func:`zlib.compress` for better speed.

.. function:: decompress(data)

   Decompress the *data*, returning a :class:`bytes` object containing the
   uncompressed data. This function is capable of decompressing multi-member
   gzip data (multiple gzip blocks concatenated together). When the data is
   certain to contain only one member the :func:`zlib.decompress` function with
   *wbits* set to 31 is faster.

   .. versionadded:: 3.2
   .. versionchanged:: 3.11
      Speed is improved by decompressing members at once in memory instead of in
      a streamed fashion.

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
   import shutil
   with open('/home/joe/file.txt', 'rb') as f_in:
       with gzip.open('/home/joe/file.txt.gz', 'wb') as f_out:
           shutil.copyfileobj(f_in, f_out)

Example of how to GZIP compress a binary string::

   import gzip
   s_in = b"Lots of content here"
   s_out = gzip.compress(s_in)

.. seealso::

   Module :mod:`zlib`
      The basic data compression module needed to support the :program:`gzip` file
      format.


.. program:: gzip

.. _gzip-cli:

Command Line Interface
----------------------

The :mod:`gzip` module provides a simple command line interface to compress or
decompress files.

Once executed the :mod:`gzip` module keeps the input file(s).

.. versionchanged:: 3.8

   Add a new command line interface with a usage.
   By default, when you will execute the CLI, the default compression level is 6.

Command line options
^^^^^^^^^^^^^^^^^^^^

.. option:: file

   If *file* is not specified, read from :data:`sys.stdin`.

.. option:: --fast

   Indicates the fastest compression method (less compression).

.. option:: --best

   Indicates the slowest compression method (best compression).

.. option:: -d, --decompress

   Decompress the given file.

.. option:: -h, --help

   Show the help message.
