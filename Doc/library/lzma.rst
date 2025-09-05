:mod:`!lzma` --- Compression using the LZMA algorithm
=====================================================

.. module:: lzma
   :synopsis: A Python wrapper for the liblzma compression library.

.. moduleauthor:: Nadeem Vawda <nadeem.vawda@gmail.com>
.. sectionauthor:: Nadeem Vawda <nadeem.vawda@gmail.com>

.. versionadded:: 3.3

**Source code:** :source:`Lib/lzma.py`

--------------

This module provides classes and convenience functions for compressing and
decompressing data using the LZMA compression algorithm. Also included is a file
interface supporting the ``.xz`` and legacy ``.lzma`` file formats used by the
:program:`xz` utility, as well as raw compressed streams.

The interface provided by this module is very similar to that of the :mod:`bz2`
module. Note that :class:`LZMAFile` and :class:`bz2.BZ2File` are *not*
thread-safe, so if you need to use a single :class:`LZMAFile` instance
from multiple threads, it is necessary to protect it with a lock.


.. exception:: LZMAError

   This exception is raised when an error occurs during compression or
   decompression, or while initializing the compressor/decompressor state.


Reading and writing compressed files
------------------------------------

.. function:: open(filename, mode="rb", *, format=None, check=-1, preset=None, filters=None, encoding=None, errors=None, newline=None)

   Open an LZMA-compressed file in binary or text mode, returning a :term:`file
   object`.

   The *filename* argument can be either an actual file name (given as a
   :class:`str`, :class:`bytes` or :term:`path-like <path-like object>` object), in
   which case the named file is opened, or it can be an existing file object
   to read from or write to.

   The *mode* argument can be any of ``"r"``, ``"rb"``, ``"w"``, ``"wb"``,
   ``"x"``, ``"xb"``, ``"a"`` or ``"ab"`` for binary mode, or ``"rt"``,
   ``"wt"``, ``"xt"``, or ``"at"`` for text mode. The default is ``"rb"``.

   When opening a file for reading, the *format* and *filters* arguments have
   the same meanings as for :class:`LZMADecompressor`. In this case, the *check*
   and *preset* arguments should not be used.

   When opening a file for writing, the *format*, *check*, *preset* and
   *filters* arguments have the same meanings as for :class:`LZMACompressor`.

   For binary mode, this function is equivalent to the :class:`LZMAFile`
   constructor: ``LZMAFile(filename, mode, ...)``. In this case, the *encoding*,
   *errors* and *newline* arguments must not be provided.

   For text mode, a :class:`LZMAFile` object is created, and wrapped in an
   :class:`io.TextIOWrapper` instance with the specified encoding, error
   handling behavior, and line ending(s).

   .. versionchanged:: 3.4
      Added support for the ``"x"``, ``"xb"`` and ``"xt"`` modes.

   .. versionchanged:: 3.6
      Accepts a :term:`path-like object`.


.. class:: LZMAFile(filename=None, mode="r", *, format=None, check=-1, preset=None, filters=None)

   Open an LZMA-compressed file in binary mode.

   An :class:`LZMAFile` can wrap an already-open :term:`file object`, or operate
   directly on a named file. The *filename* argument specifies either the file
   object to wrap, or the name of the file to open (as a :class:`str`,
   :class:`bytes` or :term:`path-like <path-like object>` object). When wrapping an
   existing file object, the wrapped file will not be closed when the
   :class:`LZMAFile` is closed.

   The *mode* argument can be either ``"r"`` for reading (default), ``"w"`` for
   overwriting, ``"x"`` for exclusive creation, or ``"a"`` for appending. These
   can equivalently be given as ``"rb"``, ``"wb"``, ``"xb"`` and ``"ab"``
   respectively.

   If *filename* is a file object (rather than an actual file name), a mode of
   ``"w"`` does not truncate the file, and is instead equivalent to ``"a"``.

   When opening a file for reading, the input file may be the concatenation of
   multiple separate compressed streams. These are transparently decoded as a
   single logical stream.

   When opening a file for reading, the *format* and *filters* arguments have
   the same meanings as for :class:`LZMADecompressor`. In this case, the *check*
   and *preset* arguments should not be used.

   When opening a file for writing, the *format*, *check*, *preset* and
   *filters* arguments have the same meanings as for :class:`LZMACompressor`.

   :class:`LZMAFile` supports all the members specified by
   :class:`io.BufferedIOBase`, except for :meth:`~io.BufferedIOBase.detach`
   and :meth:`~io.IOBase.truncate`.
   Iteration and the :keyword:`with` statement are supported.

   The following method and attributes are also provided:

   .. method:: peek(size=-1)

      Return buffered data without advancing the file position. At least one
      byte of data will be returned, unless EOF has been reached. The exact
      number of bytes returned is unspecified (the *size* argument is ignored).

      .. note:: While calling :meth:`peek` does not change the file position of
         the :class:`LZMAFile`, it may change the position of the underlying
         file object (e.g. if the :class:`LZMAFile` was constructed by passing a
         file object for *filename*).

   .. attribute:: mode

      ``'rb'`` for reading and ``'wb'`` for writing.

      .. versionadded:: 3.13

   .. attribute:: name

      The lzma file name.  Equivalent to the :attr:`~io.FileIO.name`
      attribute of the underlying :term:`file object`.

      .. versionadded:: 3.13


   .. versionchanged:: 3.4
      Added support for the ``"x"`` and ``"xb"`` modes.

   .. versionchanged:: 3.5
      The :meth:`~io.BufferedIOBase.read` method now accepts an argument of
      ``None``.

   .. versionchanged:: 3.6
      Accepts a :term:`path-like object`.


Compressing and decompressing data in memory
--------------------------------------------

.. class:: LZMACompressor(format=FORMAT_XZ, check=-1, preset=None, filters=None)

   Create a compressor object, which can be used to compress data incrementally.

   For a more convenient way of compressing a single chunk of data, see
   :func:`compress`.

   The *format* argument specifies what container format should be used.
   Possible values are:

   * :const:`FORMAT_XZ`: The ``.xz`` container format.
      This is the default format.

   * :const:`FORMAT_ALONE`: The legacy ``.lzma`` container format.
      This format is more limited than ``.xz`` -- it does not support integrity
      checks or multiple filters.

   * :const:`FORMAT_RAW`: A raw data stream, not using any container format.
      This format specifier does not support integrity checks, and requires that
      you always specify a custom filter chain (for both compression and
      decompression). Additionally, data compressed in this manner cannot be
      decompressed using :const:`FORMAT_AUTO` (see :class:`LZMADecompressor`).

   The *check* argument specifies the type of integrity check to include in the
   compressed data. This check is used when decompressing, to ensure that the
   data has not been corrupted. Possible values are:

   * :const:`CHECK_NONE`: No integrity check.
     This is the default (and the only acceptable value) for
     :const:`FORMAT_ALONE` and :const:`FORMAT_RAW`.

   * :const:`CHECK_CRC32`: 32-bit Cyclic Redundancy Check.

   * :const:`CHECK_CRC64`: 64-bit Cyclic Redundancy Check.
     This is the default for :const:`FORMAT_XZ`.

   * :const:`CHECK_SHA256`: 256-bit Secure Hash Algorithm.

   If the specified check is not supported, an :class:`LZMAError` is raised.

   The compression settings can be specified either as a preset compression
   level (with the *preset* argument), or in detail as a custom filter chain
   (with the *filters* argument).

   The *preset* argument (if provided) should be an integer between ``0`` and
   ``9`` (inclusive), optionally OR-ed with the constant
   :const:`PRESET_EXTREME`. If neither *preset* nor *filters* are given, the
   default behavior is to use :const:`PRESET_DEFAULT` (preset level ``6``).
   Higher presets produce smaller output, but make the compression process
   slower.

   .. note::

      In addition to being more CPU-intensive, compression with higher presets
      also requires much more memory (and produces output that needs more memory
      to decompress). With preset ``9`` for example, the overhead for an
      :class:`LZMACompressor` object can be as high as 800 MiB. For this reason,
      it is generally best to stick with the default preset.

   The *filters* argument (if provided) should be a filter chain specifier.
   See :ref:`filter-chain-specs` for details.

   .. method:: compress(data)

      Compress *data* (a :class:`bytes` object), returning a :class:`bytes`
      object containing compressed data for at least part of the input. Some of
      *data* may be buffered internally, for use in later calls to
      :meth:`compress` and :meth:`flush`. The returned data should be
      concatenated with the output of any previous calls to :meth:`compress`.

   .. method:: flush()

      Finish the compression process, returning a :class:`bytes` object
      containing any data stored in the compressor's internal buffers.

      The compressor cannot be used after this method has been called.


.. class:: LZMADecompressor(format=FORMAT_AUTO, memlimit=None, filters=None)

   Create a decompressor object, which can be used to decompress data
   incrementally.

   For a more convenient way of decompressing an entire compressed stream at
   once, see :func:`decompress`.

   The *format* argument specifies the container format that should be used. The
   default is :const:`FORMAT_AUTO`, which can decompress both ``.xz`` and
   ``.lzma`` files. Other possible values are :const:`FORMAT_XZ`,
   :const:`FORMAT_ALONE`, and :const:`FORMAT_RAW`.

   The *memlimit* argument specifies a limit (in bytes) on the amount of memory
   that the decompressor can use. When this argument is used, decompression will
   fail with an :class:`LZMAError` if it is not possible to decompress the input
   within the given memory limit.

   The *filters* argument specifies the filter chain that was used to create
   the stream being decompressed. This argument is required if *format* is
   :const:`FORMAT_RAW`, but should not be used for other formats.
   See :ref:`filter-chain-specs` for more information about filter chains.

   .. note::
      This class does not transparently handle inputs containing multiple
      compressed streams, unlike :func:`decompress` and :class:`LZMAFile`. To
      decompress a multi-stream input with :class:`LZMADecompressor`, you must
      create a new decompressor for each stream.

   .. method:: decompress(data, max_length=-1)

      Decompress *data* (a :term:`bytes-like object`), returning
      uncompressed data as bytes. Some of *data* may be buffered
      internally, for use in later calls to :meth:`decompress`. The
      returned data should be concatenated with the output of any
      previous calls to :meth:`decompress`.

      If *max_length* is nonnegative, returns at most *max_length*
      bytes of decompressed data. If this limit is reached and further
      output can be produced, the :attr:`~.needs_input` attribute will
      be set to ``False``. In this case, the next call to
      :meth:`~.decompress` may provide *data* as ``b''`` to obtain
      more of the output.

      If all of the input data was decompressed and returned (either
      because this was less than *max_length* bytes, or because
      *max_length* was negative), the :attr:`~.needs_input` attribute
      will be set to ``True``.

      Attempting to decompress data after the end of stream is reached
      raises an :exc:`EOFError`.  Any data found after the end of the
      stream is ignored and saved in the :attr:`~.unused_data` attribute.

      .. versionchanged:: 3.5
         Added the *max_length* parameter.

   .. attribute:: check

      The ID of the integrity check used by the input stream. This may be
      :const:`CHECK_UNKNOWN` until enough of the input has been decoded to
      determine what integrity check it uses.

   .. attribute:: eof

      ``True`` if the end-of-stream marker has been reached.

   .. attribute:: unused_data

      Data found after the end of the compressed stream.

      Before the end of the stream is reached, this will be ``b""``.

   .. attribute:: needs_input

      ``False`` if the :meth:`.decompress` method can provide more
      decompressed data before requiring new uncompressed input.

      .. versionadded:: 3.5

.. function:: compress(data, format=FORMAT_XZ, check=-1, preset=None, filters=None)

   Compress *data* (a :class:`bytes` object), returning the compressed data as a
   :class:`bytes` object.

   See :class:`LZMACompressor` above for a description of the *format*, *check*,
   *preset* and *filters* arguments.


.. function:: decompress(data, format=FORMAT_AUTO, memlimit=None, filters=None)

   Decompress *data* (a :class:`bytes` object), returning the uncompressed data
   as a :class:`bytes` object.

   If *data* is the concatenation of multiple distinct compressed streams,
   decompress all of these streams, and return the concatenation of the results.

   See :class:`LZMADecompressor` above for a description of the *format*,
   *memlimit* and *filters* arguments.


Miscellaneous
-------------

.. function:: is_check_supported(check)

   Return ``True`` if the given integrity check is supported on this system.

   :const:`CHECK_NONE` and :const:`CHECK_CRC32` are always supported.
   :const:`CHECK_CRC64` and :const:`CHECK_SHA256` may be unavailable if you are
   using a version of :program:`liblzma` that was compiled with a limited
   feature set.


.. _filter-chain-specs:

Specifying custom filter chains
-------------------------------

A filter chain specifier is a sequence of dictionaries, where each dictionary
contains the ID and options for a single filter. Each dictionary must contain
the key ``"id"``, and may contain additional keys to specify filter-dependent
options. Valid filter IDs are as follows:

* Compression filters:

  * :const:`FILTER_LZMA1` (for use with :const:`FORMAT_ALONE`)
  * :const:`FILTER_LZMA2` (for use with :const:`FORMAT_XZ` and :const:`FORMAT_RAW`)

* Delta filter:

  * :const:`FILTER_DELTA`

* Branch-Call-Jump (BCJ) filters:

  * :const:`FILTER_X86`
  * :const:`FILTER_IA64`
  * :const:`FILTER_ARM`
  * :const:`FILTER_ARMTHUMB`
  * :const:`FILTER_POWERPC`
  * :const:`FILTER_SPARC`

A filter chain can consist of up to 4 filters, and cannot be empty. The last
filter in the chain must be a compression filter, and any other filters must be
delta or BCJ filters.

Compression filters support the following options (specified as additional
entries in the dictionary representing the filter):

* ``preset``: A compression preset to use as a source of default values for
  options that are not specified explicitly.
* ``dict_size``: Dictionary size in bytes. This should be between 4 KiB and
  1.5 GiB (inclusive).
* ``lc``: Number of literal context bits.
* ``lp``: Number of literal position bits. The sum ``lc + lp`` must be at
  most 4.
* ``pb``: Number of position bits; must be at most 4.
* ``mode``: :const:`MODE_FAST` or :const:`MODE_NORMAL`.
* ``nice_len``: What should be considered a "nice length" for a match.
  This should be 273 or less.
* ``mf``: What match finder to use -- :const:`MF_HC3`, :const:`MF_HC4`,
  :const:`MF_BT2`, :const:`MF_BT3`, or :const:`MF_BT4`.
* ``depth``: Maximum search depth used by match finder. 0 (default) means to
  select automatically based on other filter options.

The delta filter stores the differences between bytes, producing more repetitive
input for the compressor in certain circumstances. It supports one option,
``dist``. This indicates the distance between bytes to be subtracted. The
default is 1, i.e. take the differences between adjacent bytes.

The BCJ filters are intended to be applied to machine code. They convert
relative branches, calls and jumps in the code to use absolute addressing, with
the aim of increasing the redundancy that can be exploited by the compressor.
These filters support one option, ``start_offset``. This specifies the address
that should be mapped to the beginning of the input data. The default is 0.


Examples
--------

Reading in a compressed file::

   import lzma
   with lzma.open("file.xz") as f:
       file_content = f.read()

Creating a compressed file::

   import lzma
   data = b"Insert Data Here"
   with lzma.open("file.xz", "w") as f:
       f.write(data)

Compressing data in memory::

   import lzma
   data_in = b"Insert Data Here"
   data_out = lzma.compress(data_in)

Incremental compression::

   import lzma
   lzc = lzma.LZMACompressor()
   out1 = lzc.compress(b"Some data\n")
   out2 = lzc.compress(b"Another piece of data\n")
   out3 = lzc.compress(b"Even more data\n")
   out4 = lzc.flush()
   # Concatenate all the partial results:
   result = b"".join([out1, out2, out3, out4])

Writing compressed data to an already-open file::

   import lzma
   with open("file.xz", "wb") as f:
       f.write(b"This data will not be compressed\n")
       with lzma.open(f, "w") as lzf:
           lzf.write(b"This *will* be compressed\n")
       f.write(b"Not compressed\n")

Creating a compressed file using a custom filter chain::

   import lzma
   my_filters = [
       {"id": lzma.FILTER_DELTA, "dist": 5},
       {"id": lzma.FILTER_LZMA2, "preset": 7 | lzma.PRESET_EXTREME},
   ]
   with lzma.open("file.xz", "w", filters=my_filters) as f:
       f.write(b"blah blah blah")
