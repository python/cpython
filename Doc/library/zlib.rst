:mod:`!zlib` --- Compression compatible with :program:`gzip`
============================================================

.. module:: zlib
   :synopsis: Low-level interface to compression and decompression routines
              compatible with gzip.

--------------

For applications that require data compression, the functions in this module
allow compression and decompression, using the `zlib library <https://www.zlib.net>`_.

.. include:: ../includes/optional-module.rst

zlib's functions have many options and often need to be used in a particular
order.  This documentation doesn't attempt to cover all of the permutations;
consult the `zlib manual <https://www.zlib.net/manual.html>`_ for authoritative
information.

For reading and writing ``.gz`` files see the :mod:`gzip` module.

The available exception and functions in this module are:


.. exception:: error

   Exception raised on compression and decompression errors.


.. function:: adler32(data[, value])

   Computes an Adler-32 checksum of *data*.  (An Adler-32 checksum is almost as
   reliable as a CRC32 but can be computed much more quickly.)  The result
   is an unsigned 32-bit integer.  If *value* is present, it is used as
   the starting value of the checksum; otherwise, a default value of 1
   is used.  Passing in *value* allows computing a running checksum over the
   concatenation of several inputs.  The algorithm is not cryptographically
   strong, and should not be used for authentication or digital signatures.  Since
   the algorithm is designed for use as a checksum algorithm, it is not suitable
   for use as a general hash algorithm.

   .. versionchanged:: 3.0
      The result is always unsigned.

.. function:: adler32_combine(adler1, adler2, len2, /)

   Combine two Adler-32 checksums into one.

   Given the Adler-32 checksum *adler1* of a sequence ``A`` and the
   Adler-32 checksum *adler2* of a sequence ``B`` of length *len2*,
   return the Adler-32 checksum of ``A`` and ``B`` concatenated.

   This function is typically useful to combine Adler-32 checksums
   that were concurrently computed. To compute checksums sequentially, use
   :func:`adler32` with the running checksum as the ``value`` argument.

   .. versionadded:: 3.15

.. function:: compress(data, /, level=Z_DEFAULT_COMPRESSION, wbits=MAX_WBITS)

   Compresses the bytes in *data*, returning a bytes object containing compressed data.
   *level* is an integer from ``0`` to ``9`` or ``-1`` controlling the level of compression;
   See :const:`Z_BEST_SPEED` (``1``), :const:`Z_BEST_COMPRESSION` (``9``),
   :const:`Z_NO_COMPRESSION` (``0``), and the default,
   :const:`Z_DEFAULT_COMPRESSION` (``-1``) for more information about these values.

   .. _compress-wbits:

   The *wbits* argument controls the size of the history buffer (or the
   "window size") used when compressing data, and whether a header and
   trailer is included in the output.  It can take several ranges of values,
   defaulting to ``15`` (:const:`MAX_WBITS`):

   * +9 to +15: The base-two logarithm of the window size, which
     therefore ranges between 512 and 32768.  Larger values produce
     better compression at the expense of greater memory usage.  The
     resulting output will include a zlib-specific header and trailer.

   * −9 to −15: Uses the absolute value of *wbits* as the
     window size logarithm, while producing a raw output stream with no
     header or trailing checksum.

   * +25 to +31 = 16 + (9 to 15): Uses the low 4 bits of the value as the
     window size logarithm, while including a basic :program:`gzip` header
     and trailing checksum in the output.

   Raises the :exc:`error` exception if any error occurs.

   .. versionchanged:: 3.6
      *level* can now be used as a keyword parameter.

   .. versionchanged:: 3.11
      The *wbits* parameter is now available to set window bits and
      compression type.

.. function:: compressobj(level=Z_DEFAULT_COMPRESSION, method=DEFLATED, wbits=MAX_WBITS, memLevel=DEF_MEM_LEVEL, strategy=Z_DEFAULT_STRATEGY[, zdict])

   Returns a compression object, to be used for compressing data streams that won't
   fit into memory at once.

   *level* is the compression level -- an integer from ``0`` to ``9`` or ``-1``.
   See :const:`Z_BEST_SPEED` (``1``), :const:`Z_BEST_COMPRESSION` (``9``),
   :const:`Z_NO_COMPRESSION` (``0``), and the default,
   :const:`Z_DEFAULT_COMPRESSION` (``-1``) for more information about these values.

   *method* is the compression algorithm. Currently, the only supported value is
   :const:`DEFLATED`.

   The *wbits* parameter controls the size of the history buffer (or the
   "window size"), and what header and trailer format will be used. It has
   the same meaning as `described for compress() <#compress-wbits>`__.

   The *memLevel* argument controls the amount of memory used for the
   internal compression state. Valid values range from ``1`` to ``9``.
   Higher values use more memory, but are faster and produce smaller output.

   *strategy* is used to tune the compression algorithm. Possible values are
   :const:`Z_DEFAULT_STRATEGY`, :const:`Z_FILTERED`, :const:`Z_HUFFMAN_ONLY`,
   :const:`Z_RLE` and :const:`Z_FIXED`.

   *zdict* is a predefined compression dictionary. This is a sequence of bytes
   (such as a :class:`bytes` object) containing subsequences that are expected
   to occur frequently in the data that is to be compressed. Those subsequences
   that are expected to be most common should come at the end of the dictionary.

   .. versionchanged:: 3.3
      Added the *zdict* parameter and keyword argument support.


.. function:: crc32(data[, value])

   .. index::
      single: Cyclic Redundancy Check
      single: checksum; Cyclic Redundancy Check

   Computes a CRC (Cyclic Redundancy Check) checksum of *data*. The
   result is an unsigned 32-bit integer. If *value* is present, it is used
   as the starting value of the checksum; otherwise, a default value of 0
   is used.  Passing in *value* allows computing a running checksum over the
   concatenation of several inputs.  The algorithm is not cryptographically
   strong, and should not be used for authentication or digital signatures.  Since
   the algorithm is designed for use as a checksum algorithm, it is not suitable
   for use as a general hash algorithm.

   .. versionchanged:: 3.0
      The result is always unsigned.

.. function:: crc32_combine(crc1, crc2, len2, /)

   Combine two CRC-32 checksums into one.

   Given the CRC-32 checksum *crc1* of a sequence ``A`` and the
   CRC-32 checksum *crc2* of a sequence ``B`` of length *len2*,
   return the CRC-32 checksum of ``A`` and ``B`` concatenated.

   This function is typically useful to combine CRC-32 checksums
   that were concurrently computed. To compute checksums sequentially, use
   :func:`crc32` with the running checksum as the ``value`` argument.

   .. versionadded:: 3.15

.. function:: decompress(data, /, wbits=MAX_WBITS, bufsize=DEF_BUF_SIZE)

   Decompresses the bytes in *data*, returning a bytes object containing the
   uncompressed data.  The *wbits* parameter depends on
   the format of *data*, and is discussed further below.
   If *bufsize* is given, it is used as the initial size of the output
   buffer.  Raises the :exc:`error` exception if any error occurs.

   .. _decompress-wbits:

   The *wbits* parameter controls the size of the history buffer
   (or "window size"), and what header and trailer format is expected.
   It is similar to the parameter for :func:`compressobj`, but accepts
   more ranges of values:

   * +8 to +15: The base-two logarithm of the window size.  The input
     must include a zlib header and trailer.

   * 0: Automatically determine the window size from the zlib header.
     Only supported since zlib 1.2.3.5.

   * −8 to −15: Uses the absolute value of *wbits* as the window size
     logarithm.  The input must be a raw stream with no header or trailer.

   * +24 to +31 = 16 + (8 to 15): Uses the low 4 bits of the value as
     the window size logarithm.  The input must include a gzip header and
     trailer.

   * +40 to +47 = 32 + (8 to 15): Uses the low 4 bits of the value as
     the window size logarithm, and automatically accepts either
     the zlib or gzip format.

   When decompressing a stream, the window size must not be smaller
   than the size originally used to compress the stream; using a too-small
   value may result in an :exc:`error` exception. The default *wbits* value
   corresponds to the largest window size and requires a zlib header and
   trailer to be included.

   *bufsize* is the initial size of the buffer used to hold decompressed data.  If
   more space is required, the buffer size will be increased as needed, so you
   don't have to get this value exactly right; tuning it will only save a few calls
   to :c:func:`malloc`.

   .. versionchanged:: 3.6
      *wbits* and *bufsize* can be used as keyword arguments.

.. function:: decompressobj(wbits=MAX_WBITS[, zdict])

   Returns a decompression object, to be used for decompressing data streams that
   won't fit into memory at once.

   The *wbits* parameter controls the size of the history buffer (or the
   "window size"), and what header and trailer format is expected.  It has
   the same meaning as `described for decompress() <#decompress-wbits>`__.

   The *zdict* parameter specifies a predefined compression dictionary. If
   provided, this must be the same dictionary as was used by the compressor that
   produced the data that is to be decompressed.

   .. note::

      If *zdict* is a mutable object (such as a :class:`bytearray`), you must not
      modify its contents between the call to :func:`decompressobj` and the first
      call to the decompressor's ``decompress()`` method.

   .. versionchanged:: 3.3
      Added the *zdict* parameter.


Compression objects support the following methods:


.. method:: Compress.compress(data)

   Compress *data*, returning a bytes object containing compressed data for at least
   part of the data in *data*.  This data should be concatenated to the output
   produced by any preceding calls to the :meth:`compress` method.  Some input may
   be kept in internal buffers for later processing.


.. method:: Compress.flush([mode])

   All pending input is processed, and a bytes object containing the remaining compressed
   output is returned.  *mode* can be selected from the constants
   :const:`Z_NO_FLUSH`, :const:`Z_PARTIAL_FLUSH`, :const:`Z_SYNC_FLUSH`,
   :const:`Z_FULL_FLUSH`, :const:`Z_BLOCK`, or :const:`Z_FINISH`,
   defaulting to :const:`Z_FINISH`.  Except :const:`Z_FINISH`, all constants
   allow compressing further bytestrings of data, while :const:`Z_FINISH` finishes the
   compressed stream and prevents compressing any more data.  After calling :meth:`flush`
   with *mode* set to :const:`Z_FINISH`, the :meth:`compress` method cannot be called again;
   the only realistic action is to delete the object.


.. method:: Compress.copy()

   Returns a copy of the compression object.  This can be used to efficiently
   compress a set of data that share a common initial prefix.


.. versionchanged:: 3.8
   Added :func:`copy.copy` and :func:`copy.deepcopy` support to compression
   objects.


Decompression objects support the following methods and attributes:


.. attribute:: Decompress.unused_data

   A bytes object which contains any bytes past the end of the compressed data. That is,
   this remains ``b""`` until the last byte that contains compression data is
   available.  If the whole bytestring turned out to contain compressed data, this is
   ``b""``, an empty bytes object.


.. attribute:: Decompress.unconsumed_tail

   A bytes object that contains any data that was not consumed by the last
   :meth:`decompress` call because it exceeded the limit for the uncompressed data
   buffer.  This data has not yet been seen by the zlib machinery, so you must feed
   it (possibly with further data concatenated to it) back to a subsequent
   :meth:`decompress` method call in order to get correct output.


.. attribute:: Decompress.eof

   A boolean indicating whether the end of the compressed data stream has been
   reached.

   This makes it possible to distinguish between a properly formed compressed
   stream, and an incomplete or truncated one.

   .. versionadded:: 3.3


.. method:: Decompress.decompress(data, max_length=0)

   Decompress *data*, returning a bytes object containing the uncompressed data
   corresponding to at least part of the data in *string*.  This data should be
   concatenated to the output produced by any preceding calls to the
   :meth:`decompress` method.  Some of the input data may be preserved in internal
   buffers for later processing.

   If the optional parameter *max_length* is non-zero then the return value will be
   no longer than *max_length*. This may mean that not all of the compressed input
   can be processed; and unconsumed data will be stored in the attribute
   :attr:`unconsumed_tail`. This bytestring must be passed to a subsequent call to
   :meth:`decompress` if decompression is to continue.  If *max_length* is zero
   then the whole input is decompressed, and :attr:`unconsumed_tail` is empty.

   .. versionchanged:: 3.6
      *max_length* can be used as a keyword argument.


.. method:: Decompress.flush([length])

   All pending input is processed, and a bytes object containing the remaining
   uncompressed output is returned.  After calling :meth:`flush`, the
   :meth:`decompress` method cannot be called again; the only realistic action is
   to delete the object.

   The optional parameter *length* sets the initial size of the output buffer.


.. method:: Decompress.copy()

   Returns a copy of the decompression object.  This can be used to save the state
   of the decompressor midway through the data stream in order to speed up random
   seeks into the stream at a future point.


.. versionchanged:: 3.8
   Added :func:`copy.copy` and :func:`copy.deepcopy` support to decompression
   objects.


The following constants are available to configure compression and decompression
behavior:

.. data:: DEFLATED

   The deflate compression method.


.. data:: MAX_WBITS

   The maximum window size, expressed as a power of 2.
   For example, if :const:`!MAX_WBITS` is ``15`` it results in a window size
   of ``32 KiB``.


.. data:: DEF_MEM_LEVEL

   The default memory level for compression objects.


.. data:: DEF_BUF_SIZE

   The default buffer size for decompression operations.


.. data:: Z_NO_COMPRESSION

   Compression level ``0``; no compression.

   .. versionadded:: 3.6


.. data:: Z_BEST_SPEED

   Compression level ``1``; fastest and produces the least compression.


.. data:: Z_BEST_COMPRESSION

   Compression level ``9``; slowest and produces the most compression.


.. data:: Z_DEFAULT_COMPRESSION

   Default compression level (``-1``); a compromise between speed and
   compression. Currently equivalent to compression level ``6``.


.. data:: Z_DEFAULT_STRATEGY

   Default compression strategy, for normal data.


.. data:: Z_FILTERED

   Compression strategy for data produced by a filter (or predictor).


.. data:: Z_HUFFMAN_ONLY

   Compression strategy that forces Huffman coding only.


.. data:: Z_RLE

   Compression strategy that limits match distances to one (run-length encoding).

   This constant is only available if Python was compiled with zlib
   1.2.0.1 or greater.

   .. versionadded:: 3.6


.. data:: Z_FIXED

   Compression strategy that prevents the use of dynamic Huffman codes.

   This constant is only available if Python was compiled with zlib
   1.2.2.2 or greater.

   .. versionadded:: 3.6


.. data:: Z_NO_FLUSH

   Flush mode ``0``. No special flushing behavior.

   .. versionadded:: 3.6


.. data:: Z_PARTIAL_FLUSH

   Flush mode ``1``. Flush as much output as possible.


.. data:: Z_SYNC_FLUSH

   Flush mode ``2``. All output is flushed and the output is aligned to a byte boundary.


.. data:: Z_FULL_FLUSH

   Flush mode ``3``. All output is flushed and the compression state is reset.


.. data:: Z_FINISH

   Flush mode ``4``. All pending input is processed, no more input is expected.


.. data:: Z_BLOCK

   Flush mode ``5``. A deflate block is completed and emitted.

   This constant is only available if Python was compiled with zlib
   1.2.2.2 or greater.

   .. versionadded:: 3.6


.. data:: Z_TREES

   Flush mode ``6``, for inflate operations. Instructs inflate to return when
   it gets to the next deflate block boundary.

   This constant is only available if Python was compiled with zlib
   1.2.3.4 or greater.

   .. versionadded:: 3.6


Information about the version of the zlib library in use is available through
the following constants:


.. data:: ZLIB_VERSION

   The version string of the zlib library that was used for building the module.
   This may be different from the zlib library actually used at runtime, which
   is available as :const:`ZLIB_RUNTIME_VERSION`.


.. data:: ZLIB_RUNTIME_VERSION

   The version string of the zlib library actually loaded by the interpreter.

   .. versionadded:: 3.3


.. data:: ZLIBNG_VERSION

   The version string of the zlib-ng library that was used for building the
   module if zlib-ng was used. When present, the :data:`ZLIB_VERSION` and
   :data:`ZLIB_RUNTIME_VERSION` constants reflect the version of the zlib API
   provided by zlib-ng.

   If zlib-ng was not used to build the module, this constant will be absent.

   .. versionadded:: 3.14


.. seealso::

   Module :mod:`gzip`
      Reading and writing :program:`gzip`\ -format files.

   https://www.zlib.net
      The zlib library home page.

   https://www.zlib.net/manual.html
      The zlib manual explains  the semantics and usage of the library's many
      functions.

   In case gzip (de)compression is a bottleneck, the `python-isal`_
   package speeds up (de)compression with a mostly compatible API.

   .. _python-isal: https://github.com/pycompression/python-isal
