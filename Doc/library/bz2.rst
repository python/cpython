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

The :mod:`bz2` module contains:

* The :class:`BZ2File` class for reading and writing compressed files.
* The :class:`BZ2Compressor` and :class:`BZ2Decompressor` classes for
  incremental (de)compression.
* The :func:`compress` and :func:`decompress` functions for one-shot
  (de)compression.

All of the classes in this module may safely be accessed from multiple threads.


(De)compression of files
------------------------

.. class:: BZ2File(filename, mode='r', buffering=None, compresslevel=9)

   Open a bzip2-compressed file.

   If *filename* is a :class:`str` or :class:`bytes` object, open the named file
   directly. Otherwise, *filename* should be a :term:`file object`, which will
   be used to read or write the compressed data.

   The *mode* argument can be either ``'r'`` for reading (default), ``'w'`` for
   overwriting, or ``'a'`` for appending. If *filename* is a file object (rather
   than an actual file name), a mode of ``'w'`` does not truncate the file, and
   is instead equivalent to ``'a'``.

   The *buffering* argument is ignored. Its use is deprecated.

   If *mode* is ``'w'`` or ``'a'``, *compresslevel* can be a number between
   ``1`` and ``9`` specifying the level of compression: ``1`` produces the
   least compression, and ``9`` (default) produces the most compression.

   If *mode* is ``'r'``, the input file may be the concatenation of multiple
   compressed streams.

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
      Support was added for *filename* being a :term:`file object` instead of an
      actual filename.

   .. versionchanged:: 3.3
      The ``'a'`` (append) mode was added, along with support for reading
      multi-stream files.


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

   .. note::
      This class does not transparently handle inputs containing multiple
      compressed streams, unlike :func:`decompress` and :class:`BZ2File`. If
      you need to decompress a multi-stream input with :class:`BZ2Decompressor`,
      you must use a new decompressor for each stream.

   .. method:: decompress(data)

      Provide data to the decompressor object. Returns a chunk of decompressed
      data if possible, or an empty byte string otherwise.

      Attempting to decompress data after the end of the current stream is
      reached raises an :exc:`EOFError`. If any data is found after the end of
      the stream, it is ignored and saved in the :attr:`unused_data` attribute.


   .. attribute:: eof

      True if the end-of-stream marker has been reached.

      .. versionadded:: 3.3


   .. attribute:: unused_data

      Data found after the end of the compressed stream.

      If this attribute is accessed before the end of the stream has been
      reached, its value will be ``b''``.


One-shot (de)compression
------------------------

.. function:: compress(data, compresslevel=9)

   Compress *data*.

   *compresslevel*, if given, must be a number between ``1`` and ``9``. The
   default is ``9``.

   For incremental compression, use a :class:`BZ2Compressor` instead.


.. function:: decompress(data)

   Decompress *data*.

   If *data* is the concatenation of multiple compressed streams, decompress
   all of the streams.

   For incremental decompression, use a :class:`BZ2Decompressor` instead.

   .. versionchanged:: 3.3
      Support for multi-stream inputs was added.

