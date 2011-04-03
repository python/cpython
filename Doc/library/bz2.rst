:mod:`bz2` --- Support for :program:`bzip2` compression
=======================================================

.. module:: bz2
   :synopsis: Interfaces for bzip2 compression and decompression.
.. moduleauthor:: Gustavo Niemeyer <niemeyer@conectiva.com>
.. moduleauthor:: Nadeem Vawda <nadeem.vawda@gmail.com>
.. sectionauthor:: Gustavo Niemeyer <niemeyer@conectiva.com>
.. sectionauthor:: Nadeem Vawda <nadeem.vawda@gmail.com>


This module provides a comprehensive interface for compressing and
decompressing data using the bzip2 compression algorithm.

For related file formats, see the :mod:`gzip`, :mod:`zipfile`, and
:mod:`tarfile` modules.

The :mod:`bz2` module contains:

* The :class:`BZ2File` class for reading and writing compressed files.
* The :class:`BZ2Compressor` and :class:`BZ2Decompressor` classes for
  incremental (de)compression.
* The :func:`compress` and :func:`decompress` functions for one-shot
  (de)compression.

All of the classes in this module may safely be accessed from multiple threads.


(De)compression of files
------------------------

.. class:: BZ2File(filename=None, mode='r', buffering=None, compresslevel=9, fileobj=None)

   Open a bzip2-compressed file.

   The :class:`BZ2File` can wrap an existing :term:`file object` (given by
   *fileobj*), or operate directly on a named file (named by *filename*).
   Exactly one of these two parameters should be provided.

   The *mode* argument can be either ``'r'`` for reading (default), or ``'w'``
   for writing.

   The *buffering* argument is ignored. Its use is deprecated.

   If *mode* is ``'w'``, *compresslevel* can be a number between ``1`` and
   ``9`` specifying the level of compression: ``1`` produces the least
   compression, and ``9`` (default) produces the most compression.

   :class:`BZ2File` provides all of the members specified by the
   :class:`io.BufferedIOBase`, except for :meth:`detach` and :meth:`truncate`.
   Iteration and the :keyword:`with` statement are supported.

   :class:`BZ2File` also provides the following method:

   .. method:: peek([n])

      Return buffered data without advancing the file position. At least one
      byte of data will be returned (unless at EOF). The exact number of bytes
      returned is unspecified.

      .. versionadded:: 3.3

   .. versionchanged:: 3.1
      Support for the :keyword:`with` statement was added.

   .. versionchanged:: 3.3
      The :meth:`fileno`, :meth:`readable`, :meth:`seekable`, :meth:`writable`,
      :meth:`read1` and :meth:`readinto` methods were added.

   .. versionchanged:: 3.3
      The *fileobj* argument to the constructor was added.


Incremental (de)compression
---------------------------

.. class:: BZ2Compressor(compresslevel=9)

   Create a new compressor object. This object may be used to compress data
   incrementally. For one-shot compression, use the :func:`compress` function
   instead.

   *compresslevel*, if given, must be a number between ``1`` and ``9``. The
   default is ``9``.

   .. method:: compress(data)

      Provide data to the compressor object. Returns a chunk of compressed data
      if possible, or an empty byte string otherwise.

      When you have finished providing data to the compressor, call the
      :meth:`flush` method to finish the compression process.


   .. method:: flush()

      Finish the compression process. Returns the compressed data left in
      internal buffers.

      The compressor object may not be used after this method has been called.


.. class:: BZ2Decompressor()

   Create a new decompressor object. This object may be used to decompress data
   incrementally. For one-shot compression, use the :func:`decompress` function
   instead.

   .. method:: decompress(data)

      Provide data to the decompressor object. Returns a chunk of decompressed
      data if possible, or an empty byte string otherwise.

      Attempting to decompress data after the end of stream is reached raises
      an :exc:`EOFError`. If any data is found after the end of the stream, it
      is ignored and saved in the :attr:`unused_data` attribute.


   .. attribute:: eof

      True if the end-of-stream marker has been reached.

      .. versionadded:: 3.3


   .. attribute:: unused_data

      Data found after the end of the compressed stream.


One-shot (de)compression
------------------------

.. function:: compress(data, compresslevel=9)

   Compress *data*.

   *compresslevel*, if given, must be a number between ``1`` and ``9``. The
   default is ``9``.

   For incremental compression, use a :class:`BZ2Compressor` instead.


.. function:: decompress(data)

   Decompress *data*.

   For incremental decompression, use a :class:`BZ2Decompressor` instead.

