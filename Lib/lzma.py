"""Interface to the liblzma compression library.

This module provides a class for reading and writing compressed files,
classes for incremental (de)compression, and convenience functions for
one-shot (de)compression.

These classes and functions support both the XZ and legacy LZMA
container formats, as well as raw compressed data streams.
"""

__all__ = [
    "CHECK_NONE", "CHECK_CRC32", "CHECK_CRC64", "CHECK_SHA256",
    "CHECK_ID_MAX", "CHECK_UNKNOWN",
    "FILTER_LZMA1", "FILTER_LZMA2", "FILTER_DELTA", "FILTER_X86", "FILTER_IA64",
    "FILTER_ARM", "FILTER_ARMTHUMB", "FILTER_POWERPC", "FILTER_SPARC",
    "FORMAT_AUTO", "FORMAT_XZ", "FORMAT_ALONE", "FORMAT_RAW",
    "MF_HC3", "MF_HC4", "MF_BT2", "MF_BT3", "MF_BT4",
    "MODE_FAST", "MODE_NORMAL", "PRESET_DEFAULT", "PRESET_EXTREME",

    "LZMACompressor", "LZMADecompressor", "LZMAFile", "LZMAError",
    "compress", "decompress", "check_is_supported",
    "encode_filter_properties", "decode_filter_properties",
]

import io
from _lzma import *


_MODE_CLOSED   = 0
_MODE_READ     = 1
_MODE_READ_EOF = 2
_MODE_WRITE    = 3

_BUFFER_SIZE = 8192


class LZMAFile(io.BufferedIOBase):

    """A file object providing transparent LZMA (de)compression.

    An LZMAFile can act as a wrapper for an existing file object, or
    refer directly to a named file on disk.

    Note that LZMAFile provides a *binary* file interface - data read
    is returned as bytes, and data to be written must be given as bytes.
    """

    def __init__(self, filename=None, mode="r", *,
                 fileobj=None, format=None, check=-1,
                 preset=None, filters=None):
        """Open an LZMA-compressed file.

        If filename is given, open the named file. Otherwise, operate on
        the file object given by fileobj. Exactly one of these two
        parameters should be provided.

        mode can be "r" for reading (default), "w" for (over)writing, or
        "a" for appending.

        format specifies the container format to use for the file.
        If mode is "r", this defaults to FORMAT_AUTO. Otherwise, the
        default is FORMAT_XZ.

        check specifies the integrity check to use. This argument can
        only be used when opening a file for writing. For FORMAT_XZ,
        the default is CHECK_CRC64. FORMAT_ALONE and FORMAT_RAW do not
        support integrity checks - for these formats, check must be
        omitted, or be CHECK_NONE.

        When opening a file for reading, the *preset* argument is not
        meaningful, and should be omitted. The *filters* argument should
        also be omitted, except when format is FORMAT_RAW (in which case
        it is required).

        When opening a file for writing, the settings used by the
        compressor can be specified either as a preset compression
        level (with the *preset* argument), or in detail as a custom
        filter chain (with the *filters* argument). For FORMAT_XZ and
        FORMAT_ALONE, the default is to use the PRESET_DEFAULT preset
        level. For FORMAT_RAW, the caller must always specify a filter
        chain; the raw compressor does not support preset compression
        levels.

        preset (if provided) should be an integer in the range 0-9,
        optionally OR-ed with the constant PRESET_EXTREME.

        filters (if provided) should be a sequence of dicts. Each dict
        should have an entry for "id" indicating ID of the filter, plus
        additional entries for options to the filter.
        """
        self._fp = None
        self._closefp = False
        self._mode = _MODE_CLOSED
        self._pos = 0
        self._size = -1

        if mode == "r":
            if check != -1:
                raise ValueError("Cannot specify an integrity check "
                                 "when opening a file for reading")
            if preset is not None:
                raise ValueError("Cannot specify a preset compression "
                                 "level when opening a file for reading")
            if format is None:
                format = FORMAT_AUTO
            mode_code = _MODE_READ
            # Save the args to pass to the LZMADecompressor initializer.
            # If the file contains multiple compressed streams, each
            # stream will need a separate decompressor object.
            self._init_args = {"format":format, "filters":filters}
            self._decompressor = LZMADecompressor(**self._init_args)
            self._buffer = None
        elif mode in ("w", "a"):
            if format is None:
                format = FORMAT_XZ
            mode_code = _MODE_WRITE
            self._compressor = LZMACompressor(format=format, check=check,
                                              preset=preset, filters=filters)
        else:
            raise ValueError("Invalid mode: {!r}".format(mode))

        if filename is not None and fileobj is None:
            mode += "b"
            self._fp = open(filename, mode)
            self._closefp = True
            self._mode = mode_code
        elif fileobj is not None and filename is None:
            self._fp = fileobj
            self._mode = mode_code
        else:
            raise ValueError("Must give exactly one of filename and fileobj")

    def close(self):
        """Flush and close the file.

        May be called more than once without error. Once the file is
        closed, any other operation on it will raise a ValueError.
        """
        if self._mode == _MODE_CLOSED:
            return
        try:
            if self._mode in (_MODE_READ, _MODE_READ_EOF):
                self._decompressor = None
                self._buffer = None
            elif self._mode == _MODE_WRITE:
                self._fp.write(self._compressor.flush())
                self._compressor = None
        finally:
            try:
                if self._closefp:
                    self._fp.close()
            finally:
                self._fp = None
                self._closefp = False
                self._mode = _MODE_CLOSED

    @property
    def closed(self):
        """True if this file is closed."""
        return self._mode == _MODE_CLOSED

    def fileno(self):
        """Return the file descriptor for the underlying file."""
        self._check_not_closed()
        return self._fp.fileno()

    def seekable(self):
        """Return whether the file supports seeking."""
        return self.readable() and self._fp.seekable()

    def readable(self):
        """Return whether the file was opened for reading."""
        self._check_not_closed()
        return self._mode in (_MODE_READ, _MODE_READ_EOF)

    def writable(self):
        """Return whether the file was opened for writing."""
        self._check_not_closed()
        return self._mode == _MODE_WRITE

    # Mode-checking helper functions.

    def _check_not_closed(self):
        if self.closed:
            raise ValueError("I/O operation on closed file")

    def _check_can_read(self):
        if not self.readable():
            raise io.UnsupportedOperation("File not open for reading")

    def _check_can_write(self):
        if not self.writable():
            raise io.UnsupportedOperation("File not open for writing")

    def _check_can_seek(self):
        if not self.readable():
            raise io.UnsupportedOperation("Seeking is only supported "
                                          "on files open for reading")
        if not self._fp.seekable():
            raise io.UnsupportedOperation("The underlying file object "
                                          "does not support seeking")

    # Fill the readahead buffer if it is empty. Returns False on EOF.
    def _fill_buffer(self):
        if self._buffer:
            return True

        if self._decompressor.unused_data:
            rawblock = self._decompressor.unused_data
        else:
            rawblock = self._fp.read(_BUFFER_SIZE)

        if not rawblock:
            if self._decompressor.eof:
                self._mode = _MODE_READ_EOF
                self._size = self._pos
                return False
            else:
                raise EOFError("Compressed file ended before the "
                               "end-of-stream marker was reached")

        # Continue to next stream.
        if self._decompressor.eof:
            self._decompressor = LZMADecompressor(**self._init_args)

        self._buffer = self._decompressor.decompress(rawblock)
        return True

    # Read data until EOF.
    # If return_data is false, consume the data without returning it.
    def _read_all(self, return_data=True):
        blocks = []
        while self._fill_buffer():
            if return_data:
                blocks.append(self._buffer)
            self._pos += len(self._buffer)
            self._buffer = None
        if return_data:
            return b"".join(blocks)

    # Read a block of up to n bytes.
    # If return_data is false, consume the data without returning it.
    def _read_block(self, n, return_data=True):
        blocks = []
        while n > 0 and self._fill_buffer():
            if n < len(self._buffer):
                data = self._buffer[:n]
                self._buffer = self._buffer[n:]
            else:
                data = self._buffer
                self._buffer = None
            if return_data:
                blocks.append(data)
            self._pos += len(data)
            n -= len(data)
        if return_data:
            return b"".join(blocks)

    def peek(self, size=-1):
        """Return buffered data without advancing the file position.

        Always returns at least one byte of data, unless at EOF.
        The exact number of bytes returned is unspecified.
        """
        self._check_can_read()
        if self._mode == _MODE_READ_EOF or not self._fill_buffer():
            return b""
        return self._buffer

    def read(self, size=-1):
        """Read up to size uncompressed bytes from the file.

        If size is negative or omitted, read until EOF is reached.
        Returns b"" if the file is already at EOF.
        """
        self._check_can_read()
        if self._mode == _MODE_READ_EOF or size == 0:
            return b""
        elif size < 0:
            return self._read_all()
        else:
            return self._read_block(size)

    def read1(self, size=-1):
        """Read up to size uncompressed bytes with at most one read
        from the underlying stream.

        Returns b"" if the file is at EOF.
        """
        self._check_can_read()
        if (size == 0 or self._mode == _MODE_READ_EOF or
            not self._fill_buffer()):
            return b""
        if 0 < size < len(self._buffer):
            data = self._buffer[:size]
            self._buffer = self._buffer[size:]
        else:
            data = self._buffer
            self._buffer = None
        self._pos += len(data)
        return data

    def write(self, data):
        """Write a bytes object to the file.

        Returns the number of uncompressed bytes written, which is
        always len(data). Note that due to buffering, the file on disk
        may not reflect the data written until close() is called.
        """
        self._check_can_write()
        compressed = self._compressor.compress(data)
        self._fp.write(compressed)
        self._pos += len(data)
        return len(data)

    # Rewind the file to the beginning of the data stream.
    def _rewind(self):
        self._fp.seek(0, 0)
        self._mode = _MODE_READ
        self._pos = 0
        self._decompressor = LZMADecompressor(**self._init_args)
        self._buffer = None

    def seek(self, offset, whence=0):
        """Change the file position.

        The new position is specified by offset, relative to the
        position indicated by whence. Possible values for whence are:

            0: start of stream (default): offset must not be negative
            1: current stream position
            2: end of stream; offset must not be positive

        Returns the new file position.

        Note that seeking is emulated, sp depending on the parameters,
        this operation may be extremely slow.
        """
        self._check_can_seek()

        # Recalculate offset as an absolute file position.
        if whence == 0:
            pass
        elif whence == 1:
            offset = self._pos + offset
        elif whence == 2:
            # Seeking relative to EOF - we need to know the file's size.
            if self._size < 0:
                self._read_all(return_data=False)
            offset = self._size + offset
        else:
            raise ValueError("Invalid value for whence: {}".format(whence))

        # Make it so that offset is the number of bytes to skip forward.
        if offset < self._pos:
            self._rewind()
        else:
            offset -= self._pos

        # Read and discard data until we reach the desired position.
        if self._mode != _MODE_READ_EOF:
            self._read_block(offset, return_data=False)

        return self._pos

    def tell(self):
        """Return the current file position."""
        self._check_not_closed()
        return self._pos


def compress(data, format=FORMAT_XZ, check=-1, preset=None, filters=None):
    """Compress a block of data.

    Refer to LZMACompressor's docstring for a description of the
    optional arguments *format*, *check*, *preset* and *filters*.

    For incremental compression, use an LZMACompressor object instead.
    """
    comp = LZMACompressor(format, check, preset, filters)
    return comp.compress(data) + comp.flush()


def decompress(data, format=FORMAT_AUTO, memlimit=None, filters=None):
    """Decompress a block of data.

    Refer to LZMADecompressor's docstring for a description of the
    optional arguments *format*, *check* and *filters*.

    For incremental decompression, use a LZMADecompressor object instead.
    """
    results = []
    while True:
        decomp = LZMADecompressor(format, memlimit, filters)
        results.append(decomp.decompress(data))
        if not decomp.eof:
            raise LZMAError("Compressed data ended before the "
                            "end-of-stream marker was reached")
        if not decomp.unused_data:
            return b"".join(results)
        # There is unused data left over. Proceed to next stream.
        data = decomp.unused_data
