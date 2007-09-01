
:mod:`bz2` --- Compression compatible with :program:`bzip2`
===========================================================

.. module:: bz2
   :synopsis: Interface to compression and decompression routines compatible with bzip2.
.. moduleauthor:: Gustavo Niemeyer <niemeyer@conectiva.com>
.. sectionauthor:: Gustavo Niemeyer <niemeyer@conectiva.com>


This module provides a comprehensive interface for the bz2 compression library.
It implements a complete file interface, one-shot (de)compression functions, and
types for sequential (de)compression.

Here is a resume of the features offered by the bz2 module:

* :class:`BZ2File` class implements a complete file interface, including
  :meth:`readline`, :meth:`readlines`, :meth:`writelines`, :meth:`seek`, etc;

* :class:`BZ2File` class implements emulated :meth:`seek` support;

* :class:`BZ2File` class implements universal newline support;

* :class:`BZ2File` class offers an optimized line iteration using the readahead
  algorithm borrowed from file objects;

* Sequential (de)compression supported by :class:`BZ2Compressor` and
  :class:`BZ2Decompressor` classes;

* One-shot (de)compression supported by :func:`compress` and :func:`decompress`
  functions;

* Thread safety uses individual locking mechanism;

* Complete inline documentation;


(De)compression of files
------------------------

Handling of compressed files is offered by the :class:`BZ2File` class.


.. class:: BZ2File(filename[, mode[, buffering[, compresslevel]]])

   Open a bz2 file. Mode can be either ``'r'`` or ``'w'``, for reading  (default)
   or writing. When opened for writing, the file will be created if it doesn't
   exist, and truncated otherwise. If *buffering* is given, ``0`` means unbuffered,
   and larger numbers specify the buffer size; the default is ``0``. If
   *compresslevel* is given, it must be a number between ``1`` and ``9``; the
   default is ``9``. Add a ``'U'`` to mode to open the file for input with
   universal newline support. Any line ending in the input file will be seen as a
   ``'\n'`` in Python.  Also, a file so opened gains the attribute
   :attr:`newlines`; the value for this attribute is one of ``None`` (no newline
   read yet), ``'\r'``, ``'\n'``, ``'\r\n'`` or a tuple containing all the newline
   types seen. Universal newlines are available only when reading. Instances
   support iteration in the same way as normal :class:`file` instances.


.. method:: BZ2File.close()

   Close the file. Sets data attribute :attr:`closed` to true. A closed file cannot
   be used for further I/O operations. :meth:`close` may be called more than once
   without error.


.. method:: BZ2File.read([size])

   Read at most *size* uncompressed bytes, returned as a string. If the *size*
   argument is negative or omitted, read until EOF is reached.


.. method:: BZ2File.readline([size])

   Return the next line from the file, as a string, retaining newline. A
   non-negative *size* argument limits the maximum number of bytes to return (an
   incomplete line may be returned then). Return an empty string at EOF.


.. method:: BZ2File.readlines([size])

   Return a list of lines read. The optional *size* argument, if given, is an
   approximate bound on the total number of bytes in the lines returned.


.. method:: BZ2File.seek(offset[, whence])

   Move to new file position. Argument *offset* is a byte count. Optional argument
   *whence* defaults to ``os.SEEK_SET`` or ``0`` (offset from start of file; offset
   should be ``>= 0``); other values are ``os.SEEK_CUR`` or ``1`` (move relative to
   current position; offset can be positive or negative), and ``os.SEEK_END`` or
   ``2`` (move relative to end of file; offset is usually negative, although many
   platforms allow seeking beyond the end of a file).

   Note that seeking of bz2 files is emulated, and depending on the parameters the
   operation may be extremely slow.


.. method:: BZ2File.tell()

   Return the current file position, an integer (may be a long integer).


.. method:: BZ2File.write(data)

   Write string *data* to file. Note that due to buffering, :meth:`close` may be
   needed before the file on disk reflects the data written.


.. method:: BZ2File.writelines(sequence_of_strings)

   Write the sequence of strings to the file. Note that newlines are not added. The
   sequence can be any iterable object producing strings. This is equivalent to
   calling write() for each string.


Sequential (de)compression
--------------------------

Sequential compression and decompression is done using the classes
:class:`BZ2Compressor` and :class:`BZ2Decompressor`.


.. class:: BZ2Compressor([compresslevel])

   Create a new compressor object. This object may be used to compress data
   sequentially. If you want to compress data in one shot, use the :func:`compress`
   function instead. The *compresslevel* parameter, if given, must be a number
   between ``1`` and ``9``; the default is ``9``.


.. method:: BZ2Compressor.compress(data)

   Provide more data to the compressor object. It will return chunks of compressed
   data whenever possible. When you've finished providing data to compress, call
   the :meth:`flush` method to finish the compression process, and return what is
   left in internal buffers.


.. method:: BZ2Compressor.flush()

   Finish the compression process and return what is left in internal buffers. You
   must not use the compressor object after calling this method.


.. class:: BZ2Decompressor()

   Create a new decompressor object. This object may be used to decompress data
   sequentially. If you want to decompress data in one shot, use the
   :func:`decompress` function instead.


.. method:: BZ2Decompressor.decompress(data)

   Provide more data to the decompressor object. It will return chunks of
   decompressed data whenever possible. If you try to decompress data after the end
   of stream is found, :exc:`EOFError` will be raised. If any data was found after
   the end of stream, it'll be ignored and saved in :attr:`unused_data` attribute.


One-shot (de)compression
------------------------

One-shot compression and decompression is provided through the :func:`compress`
and :func:`decompress` functions.


.. function:: compress(data[, compresslevel])

   Compress *data* in one shot. If you want to compress data sequentially, use an
   instance of :class:`BZ2Compressor` instead. The *compresslevel* parameter, if
   given, must be a number between ``1`` and ``9``; the default is ``9``.


.. function:: decompress(data)

   Decompress *data* in one shot. If you want to decompress data sequentially, use
   an instance of :class:`BZ2Decompressor` instead.

