:mod:`zlib` --- Compression compatible with :program:`gzip`
===========================================================

.. module:: zlib
   :synopsis: Low-level interface to compression and decompression routines
              compatible with gzip.


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

   Computes a Adler-32 checksum of *data*.  (An Adler-32 checksum is almost as
   reliable as a CRC32 but can be computed much more quickly.)  If *value* is
   present, it is used as the starting value of the checksum; otherwise, a fixed
   default value is used.  This allows computing a running checksum over the
   concatenation of several inputs.  The algorithm is not cryptographically
   strong, and should not be used for authentication or digital signatures.  Since
   the algorithm is designed for use as a checksum algorithm, it is not suitable
   for use as a general hash algorithm.

   Always returns an unsigned 32-bit integer.

.. note::
   To generate the same numeric value across all Python versions and
   platforms use adler32(data) & 0xffffffff.  If you are only using
   the checksum in packed binary format this is not necessary as the
   return value is the correct 32bit binary representation
   regardless of sign.


.. function:: compress(data[, level])

   Compresses the bytes in *data*, returning a bytes object containing compressed data.
   *level* is an integer from ``1`` to ``9`` controlling the level of compression;
   ``1`` is fastest and produces the least compression, ``9`` is slowest and
   produces the most.  The default value is ``6``.  Raises the :exc:`error`
   exception if any error occurs.


.. function:: compressobj(level=-1, method=DEFLATED, wbits=15, memlevel=8, strategy=Z_DEFAULT_STRATEGY[, zdict])

   Returns a compression object, to be used for compressing data streams that won't
   fit into memory at once.

   *level* is the compression level -- an integer from ``1`` to ``9``. A value
   of ``1`` is fastest and produces the least compression, while a value of
   ``9`` is slowest and produces the most. The default value is ``6``.

   *method* is the compression algorithm. Currently, the only supported value is
   ``DEFLATED``.

   *wbits* is the base two logarithm of the size of the window buffer. This
   should be an integer from ``8`` to ``15``. Higher values give better
   compression, but use more memory.

   *memlevel* controls the amount of memory used for internal compression state.
   Valid values range from ``1`` to ``9``. Higher values using more memory,
   but are faster and produce smaller output.

   *strategy* is used to tune the compression algorithm. Possible values are
   ``Z_DEFAULT_STRATEGY``, ``Z_FILTERED``, and ``Z_HUFFMAN_ONLY``.

   *zdict* is a predefined compression dictionary. This is a sequence of bytes
   (such as a :class:`bytes` object) containing subsequences that are expected
   to occur frequently in the data that is to be compressed. Those subsequences
   that are expected to be most common should come at the end of the dictionary.

   .. versionchanged:: 3.3
      Added the *method*, *wbits*, *memlevel*, *strategy* and *zdict*
      parameters.


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

   Always returns an unsigned 32-bit integer.

   .. note::

      To generate the same numeric value across all Python versions and
      platforms, use ``crc32(data) & 0xffffffff``.  If you are only using
      the checksum in packed binary format this is not necessary as the
      return value is the correct 32-bit binary representation
      regardless of sign.


.. function:: decompress(data[, wbits[, bufsize]])

   Decompresses the bytes in *data*, returning a bytes object containing the
   uncompressed data.  The *wbits* parameter controls the size of the window
   buffer, and is discussed further below.
   If *bufsize* is given, it is used as the initial size of the output
   buffer.  Raises the :exc:`error` exception if any error occurs.

   The absolute value of *wbits* is the base two logarithm of the size of the
   history buffer (the "window size") used when compressing data.  Its absolute
   value should be between 8 and 15 for the most recent versions of the zlib
   library, larger values resulting in better compression at the expense of greater
   memory usage.  When decompressing a stream, *wbits* must not be smaller
   than the size originally used to compress the stream; using a too-small
   value will result in an exception. The default value is therefore the
   highest value, 15.  When *wbits* is negative, the standard
   :program:`gzip` header is suppressed.

   *bufsize* is the initial size of the buffer used to hold decompressed data.  If
   more space is required, the buffer size will be increased as needed, so you
   don't have to get this value exactly right; tuning it will only save a few calls
   to :c:func:`malloc`.  The default size is 16384.


.. function:: decompressobj(wbits=15[, zdict])

   Returns a decompression object, to be used for decompressing data streams that
   won't fit into memory at once.

   The *wbits* parameter controls the size of the window buffer.

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
   :const:`Z_SYNC_FLUSH`,  :const:`Z_FULL_FLUSH`,  or  :const:`Z_FINISH`,
   defaulting to :const:`Z_FINISH`.  :const:`Z_SYNC_FLUSH` and
   :const:`Z_FULL_FLUSH` allow compressing further bytestrings of data, while
   :const:`Z_FINISH` finishes the compressed stream and  prevents compressing any
   more data.  After calling :meth:`flush` with *mode* set to :const:`Z_FINISH`,
   the :meth:`compress` method cannot be called again; the only realistic action is
   to delete the object.


.. method:: Compress.copy()

   Returns a copy of the compression object.  This can be used to efficiently
   compress a set of data that share a common initial prefix.


Decompression objects support the following methods and attributes:


.. attribute:: Decompress.unused_data

   A bytes object which contains any bytes past the end of the compressed data. That is,
   this remains ``""`` until the last byte that contains compression data is
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

   This makes it possible to distinguish between a properly-formed compressed
   stream, and an incomplete or truncated one.

   .. versionadded:: 3.3


.. method:: Decompress.decompress(data[, max_length])

   Decompress *data*, returning a bytes object containing the uncompressed data
   corresponding to at least part of the data in *string*.  This data should be
   concatenated to the output produced by any preceding calls to the
   :meth:`decompress` method.  Some of the input data may be preserved in internal
   buffers for later processing.

   If the optional parameter *max_length* is supplied then the return value will be
   no longer than *max_length*. This may mean that not all of the compressed input
   can be processed; and unconsumed data will be stored in the attribute
   :attr:`unconsumed_tail`. This bytestring must be passed to a subsequent call to
   :meth:`decompress` if decompression is to continue.  If *max_length* is not
   supplied then the whole input is decompressed, and :attr:`unconsumed_tail` is
   empty.


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


Information about the version of the zlib library in use is available through
the following constants:


.. data:: ZLIB_VERSION

   The version string of the zlib library that was used for building the module.
   This may be different from the zlib library actually used at runtime, which
   is available as :const:`ZLIB_RUNTIME_VERSION`.


.. data:: ZLIB_RUNTIME_VERSION

   The version string of the zlib library actually loaded by the interpreter.

   .. versionadded:: 3.3


.. seealso::

   Module :mod:`gzip`
      Reading and writing :program:`gzip`\ -format files.

   http://www.zlib.net
      The zlib library home page.

   http://www.zlib.net/manual.html
      The zlib manual explains  the semantics and usage of the library's many
      functions.

