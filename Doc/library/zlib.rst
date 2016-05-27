
:mod:`zlib` --- Compression compatible with :program:`gzip`
===========================================================

.. module:: zlib
   :synopsis: Low-level interface to compression and decompression routines compatible with
              gzip.


For applications that require data compression, the functions in this module
allow compression and decompression, using the zlib library. The zlib library
has its own home page at http://www.zlib.net.   There are known
incompatibilities between the Python module and versions of the zlib library
earlier than 1.1.3; 1.1.3 has a security vulnerability, so we recommend using
1.1.4 or later.

zlib's functions have many options and often need to be used in a particular
order.  This documentation doesn't attempt to cover all of the permutations;
consult the zlib manual at http://www.zlib.net/manual.html for authoritative
information.

For reading and writing ``.gz`` files see the :mod:`gzip` module.

The available exception and functions in this module are:


.. exception:: error

   Exception raised on compression and decompression errors.


.. function:: adler32(data[, value])

   Computes an Adler-32 checksum of *data*.  (An Adler-32 checksum is almost as
   reliable as a CRC32 but can be computed much more quickly.)  If *value* is
   present, it is used as the starting value of the checksum; otherwise, a fixed
   default value is used.  This allows computing a running checksum over the
   concatenation of several inputs.  The algorithm is not cryptographically
   strong, and should not be used for authentication or digital signatures.  Since
   the algorithm is designed for use as a checksum algorithm, it is not suitable
   for use as a general hash algorithm.

   This function always returns an integer object.

.. note::
   To generate the same numeric value across all Python versions and
   platforms use adler32(data) & 0xffffffff.  If you are only using
   the checksum in packed binary format this is not necessary as the
   return value is the correct 32bit binary representation
   regardless of sign.

.. versionchanged:: 2.6
   The return value is in the range [-2**31, 2**31-1]
   regardless of platform.  In older versions the value is
   signed on some platforms and unsigned on others.

.. versionchanged:: 3.0
   The return value is unsigned and in the range [0, 2**32-1]
   regardless of platform.


.. function:: compress(string[, level])

   Compresses the data in *string*, returning a string contained compressed data.
   *level* is an integer from ``0`` to ``9`` controlling the level of compression;
   ``1`` is fastest and produces the least compression, ``9`` is slowest and
   produces the most.  ``0`` is no compression.  The default value is ``6``.
   Raises the :exc:`error` exception if any error occurs.


.. function:: compressobj([level[, method[, wbits[, memlevel[, strategy]]]]])

   Returns a compression object, to be used for compressing data streams that won't
   fit into memory at once.  *level* is an integer from
   ``0`` to ``9`` or ``-1``, controlling
   the level of compression; ``1`` is fastest and produces the least compression,
   ``9`` is slowest and produces the most.  ``0`` is no compression.  The default
   value is ``-1`` (Z_DEFAULT_COMPRESSION). Z_DEFAULT_COMPRESSION represents a default
   compromise between speed and compression (currently equivalent to level 6).

   *method* is the compression algorithm. Currently, the only supported value is
   ``DEFLATED``.

   The *wbits* argument controls the size of the history buffer (or the
   "window size") used when compressing data, and whether a header and
   trailer is included in the output.  It can take several ranges of values.
   The default is 15.

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

   *memlevel* controls the amount of memory used for internal compression state.
   Valid values range from ``1`` to ``9``. Higher values using more memory,
   but are faster and produce smaller output. The default is 8.

   *strategy* is used to tune the compression algorithm. Possible values are
   ``Z_DEFAULT_STRATEGY``, ``Z_FILTERED``, and ``Z_HUFFMAN_ONLY``. The default
   is ``Z_DEFAULT_STRATEGY``.


.. function:: crc32(data[, value])

   .. index::
      single: Cyclic Redundancy Check
      single: checksum; Cyclic Redundancy Check

   Computes a CRC (Cyclic Redundancy Check)  checksum of *data*. If *value* is
   present, it is used as the starting value of the checksum; otherwise, a fixed
   default value is used.  This allows computing a running checksum over the
   concatenation of several inputs.  The algorithm is not cryptographically
   strong, and should not be used for authentication or digital signatures.  Since
   the algorithm is designed for use as a checksum algorithm, it is not suitable
   for use as a general hash algorithm.

   This function always returns an integer object.

.. note::
   To generate the same numeric value across all Python versions and
   platforms use crc32(data) & 0xffffffff.  If you are only using
   the checksum in packed binary format this is not necessary as the
   return value is the correct 32bit binary representation
   regardless of sign.

.. versionchanged:: 2.6
   The return value is in the range [-2**31, 2**31-1]
   regardless of platform.  In older versions the value would be
   signed on some platforms and unsigned on others.

.. versionchanged:: 3.0
   The return value is unsigned and in the range [0, 2**32-1]
   regardless of platform.


.. function:: decompress(string[, wbits[, bufsize]])

   Decompresses the data in *string*, returning a string containing the
   uncompressed data.  The *wbits* parameter depends on
   the format of *string*, and is discussed further below.
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
   is 15, which corresponds to the largest window size and requires a zlib
   header and trailer to be included.

   *bufsize* is the initial size of the buffer used to hold decompressed data.  If
   more space is required, the buffer size will be increased as needed, so you
   don't have to get this value exactly right; tuning it will only save a few calls
   to :c:func:`malloc`.  The default size is 16384.


.. function:: decompressobj([wbits])

   Returns a decompression object, to be used for decompressing data streams that
   won't fit into memory at once.

   The *wbits* parameter controls the size of the history buffer (or the
   "window size"), and what header and trailer format is expected.  It has
   the same meaning as `described for decompress() <#decompress-wbits>`__.

Compression objects support the following methods:


.. method:: Compress.compress(string)

   Compress *string*, returning a string containing compressed data for at least
   part of the data in *string*.  This data should be concatenated to the output
   produced by any preceding calls to the :meth:`compress` method.  Some input may
   be kept in internal buffers for later processing.


.. method:: Compress.flush([mode])

   All pending input is processed, and a string containing the remaining compressed
   output is returned.  *mode* can be selected from the constants
   :const:`Z_SYNC_FLUSH`,  :const:`Z_FULL_FLUSH`,  or  :const:`Z_FINISH`,
   defaulting to :const:`Z_FINISH`.  :const:`Z_SYNC_FLUSH` and
   :const:`Z_FULL_FLUSH` allow compressing further strings of data, while
   :const:`Z_FINISH` finishes the compressed stream and  prevents compressing any
   more data.  After calling :meth:`flush` with *mode* set to :const:`Z_FINISH`,
   the :meth:`compress` method cannot be called again; the only realistic action is
   to delete the object.


.. method:: Compress.copy()

   Returns a copy of the compression object.  This can be used to efficiently
   compress a set of data that share a common initial prefix.

   .. versionadded:: 2.5

Decompression objects support the following methods, and two attributes:


.. attribute:: Decompress.unused_data

   A string which contains any bytes past the end of the compressed data. That is,
   this remains ``""`` until the last byte that contains compression data is
   available.  If the whole string turned out to contain compressed data, this is
   ``""``, the empty string.

   The only way to determine where a string of compressed data ends is by actually
   decompressing it.  This means that when compressed data is contained part of a
   larger file, you can only find the end of it by reading data and feeding it
   followed by some non-empty string into a decompression object's
   :meth:`decompress` method until the :attr:`unused_data` attribute is no longer
   the empty string.


.. attribute:: Decompress.unconsumed_tail

   A string that contains any data that was not consumed by the last
   :meth:`decompress` call because it exceeded the limit for the uncompressed data
   buffer.  This data has not yet been seen by the zlib machinery, so you must feed
   it (possibly with further data concatenated to it) back to a subsequent
   :meth:`decompress` method call in order to get correct output.


.. method:: Decompress.decompress(string[, max_length])

   Decompress *string*, returning a string containing the uncompressed data
   corresponding to at least part of the data in *string*.  This data should be
   concatenated to the output produced by any preceding calls to the
   :meth:`decompress` method.  Some of the input data may be preserved in internal
   buffers for later processing.

   If the optional parameter *max_length* is non-zero then the return value will be
   no longer than *max_length*. This may mean that not all of the compressed input
   can be processed; and unconsumed data will be stored in the attribute
   :attr:`unconsumed_tail`. This string must be passed to a subsequent call to
   :meth:`decompress` if decompression is to continue.  If *max_length* is not
   supplied then the whole input is decompressed, and :attr:`unconsumed_tail` is an
   empty string.


.. method:: Decompress.flush([length])

   All pending input is processed, and a string containing the remaining
   uncompressed output is returned.  After calling :meth:`flush`, the
   :meth:`decompress` method cannot be called again; the only realistic action is
   to delete the object.

   The optional parameter *length* sets the initial size of the output buffer.


.. method:: Decompress.copy()

   Returns a copy of the decompression object.  This can be used to save the state
   of the decompressor midway through the data stream in order to speed up random
   seeks into the stream at a future point.

   .. versionadded:: 2.5


.. seealso::

   Module :mod:`gzip`
      Reading and writing :program:`gzip`\ -format files.

   http://www.zlib.net
      The zlib library home page.

   http://www.zlib.net/manual.html
      The zlib manual explains  the semantics and usage of the library's many
      functions.

