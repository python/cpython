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
    "open", "compress", "decompress", "is_check_supported",
]

import builtins
import io
from _lzma import *
from _lzma import _encode_filter_properties, _decode_filter_properties


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
                 format=None, check=-1, preset=None, filters=None):
        """Open an LZMA-compressed file in binary mode.

        filename can be either an actual file name (given as a str or
        bytes object), in which case the named file is opened, or it can
        be an existing file object to read from or write to.

        mode can be "r" for reading (default), "w" for (over)writing,
        "x" for creating exclusively, or "a" for appending. These can
        equivalently be given as "rb", "wb", "xb" and "ab" respectively.

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

        if mode in ("r", "rb"):
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
            self._buffer = b""
            self._buffer_offset = 0
        elif mode in ("w", "wb", "a", "ab", "x", "xb"):
            if format is None:
                format = FORMAT_XZ
            mode_code = _MODE_WRITE
            self._compressor = LZMACompressor(format=format, check=check,
                                              preset=preset, filters=filters)
        else:
            raise ValueError("Invalid mode: {!r}".format(mode))

        if isinstance(filename, (str, bytes)):
            if "b" not in mode:
                mode += "b"
            self._fp = builtins.open(filename, mode)
            self._closefp = True
            self._mode = mode_code
        elif hasattr(filename, "read") or hasattr(filename, "write"):
            self._fp = filename
            self._mode = mode_code
        else:
            raise TypeError("filename must be a str or bytes object, or a file")

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
                self._buffer = b""
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
        if self._mode not in (_MODE_READ, _MODE_READ_EOF):
            self._check_not_closed()
            raise io.UnsupportedOperation("File not open for reading")

    def _check_can_write(self):
        if self._mode != _MODE_WRITE:
            self._check_not_closed()
            raise io.UnsupportedOperation("File not open for writing")

    def _check_can_seek(self):
        if self._mode not in (_MODE_READ, _MODE_READ_EOF):
            self._check_not_closed()
            raise io.UnsupportedOperation("Seeking is only supported "
                                          "on files open for reading")
        if not self._fp.seekable():
            raise io.UnsupportedOperation("The underlying file object "
                                          "does not support seeking")

    # Fill the readahead buffer if it is empty. Returns False on EOF.
    def _fill_buffer(self):
        if self._mode == _MODE_READ_EOF:
            return False
        # Depending on the input data, our call to the decompressor may not
        # return any data. In this case, try again after reading another block.
        while self._buffer_offset == len(self._buffer):
            rawblock = (self._decompressor.unused_data or
                        self._fp.read(_BUFFER_SIZE))

            if not rawblock:
                if self._decompressor.eof:
                    self._mode = _MODE_READ_EOF
                    self._size = self._pos
                    return False
                else:
                    raise EOFError("Compressed file ended before the "
                                   "end-of-stream marker was reached")

            if self._decompressor.eof:
                # Continue to next stream.
                self._decompressor = LZMADecompressor(**self._init_args)
                try:
                    self._buffer = self._decompressor.decompress(rawblock)
                except LZMAError:
                    # Trailing data isn't a valid compressed stream; ignore it.
                    self._mode = _MODE_READ_EOF
                    self._size = self._pos
                    return False
            else:
                self._buffer = self._decompressor.decompress(rawblock)
            self._buffer_offset = 0
        return True

    # Read data until EOF.
    # If return_data is false, consume the data without returning it.
    def _read_all(self, return_data=True):
        # The loop assumes that _buffer_offset is 0. Ensure that this is true.
        self._buffer = self._buffer[self._buffer_offset:]
        self._buffer_offset = 0

        blocks = []
        while self._fill_buffer():
            if return_data:
                blocks.append(self._buffer)
            self._pos += len(self._buffer)
            self._buffer = b""
        if return_data:
            return b"".join(blocks)

    # Read a block of up to n bytes.
    # If return_data is false, consume the data without returning it.
    def _read_block(self, n, return_data=True):
        # If we have enough data buffered, return immediately.
        end = self._buffer_offset + n
        if end <= len(self._buffer):
            data = self._buffer[self._buffer_offset : end]
            self._buffer_offset = end
            self._pos += len(data)
            return data if return_data else None

        # The loop assumes that _buffer_offset is 0. Ensure that this is true.
        self._buffer = self._buffer[self._buffer_offset:]
        self._buffer_offset = 0

        blocks = []
        while n > 0 and self._fill_buffer():
            if n < len(self._buffer):
                data = self._buffer[:n]
                self._buffer_offset = n
            else:
                data = self._buffer
                self._buffer = b""
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
        if not self._fill_buffer():
            return b""
        return self._buffer[self._buffer_offset:]

    def read(self, size=-1):
        """Read up to size uncompressed bytes from the file.

        If size is negative or omitted, read until EOF is reached.
        Returns b"" if the file is already at EOF.
        """
        self._check_can_read()
        if size == 0:
            return b""
        elif size < 0:
            return self._read_all()
        else:
            return self._read_block(size)

    def read1(self, size=-1):
        """Read up to size uncompressed bytes, while trying to avoid
        making multiple reads from the underlying stream.

        Returns b"" if the file is at EOF.
        """
        # Usually, read1() calls _fp.read() at most once. However, sometimes
        # this does not give enough data for the decompressor to make progress.
        # In this case we make multiple reads, to avoid returning b"".
        self._check_can_read()
        if (size == 0 or
            # Only call _fill_buffer() if the buffer is actually empty.
            # This gives a significant speedup if *size* is small.
            (self._buffer_offset == len(self._buffer) and not self._fill_buffer())):
            return b""
        if size > 0:
            data = self._buffer[self._buffer_offset :
                                self._buffer_offset + size]
            self._buffer_offset += len(data)
        else:
            data = self._buffer[self._buffer_offset:]
            self._buffer = b""
            self._buffer_offset = 0
        self._pos += len(data)
        return data

    def readline(self, size=-1):
        """Read a line of uncompressed bytes from the file.

        The terminating newline (if present) is retained. If size is
        non-negative, no more than size bytes will be read (in which
        case the line may be incomplete). Returns b'' if already at EOF.
        """
        self._check_can_read()
        # Shortcut for the common case - the whole line is in the buffer.
        if size < 0:
            end = self._buffer.find(b"\n", self._buffer_offset) + 1
            if end > 0:
                line = self._buffer[self._buffer_offset : end]
                self._buffer_offset = end
                self._pos += len(line)
                return line
        return io.BufferedIOBase.readline(self, size)

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
        self._buffer = b""
        self._buffer_offset = 0

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
        self._read_block(offset, return_data=False)

        return self._pos

    def tell(self):
        """Return the current file position."""
        self._check_not_closed()
        return self._pos


def open(filename, mode="rb", *,
         format=None, check=-1, preset=None, filters=None,
         encoding=None, errors=None, newline=None):
    """Open an LZMA-compressed file in binary or text mode.

    filename can be either an actual file name (given as a str or bytes
    object), in which case the named file is opened, or it can be an
    existing file object to read from or write to.

    The mode argument can be "r", "rb" (default), "w", "wb", "x", "xb",
    "a", or "ab" for binary mode, or "rt", "wt", "xt", or "at" for text
    mode.

    The format, check, preset and filters arguments specify the
    compression settings, as for LZMACompressor, LZMADecompressor and
    LZMAFile.

    For binary mode, this function is equivalent to the LZMAFile
    constructor: LZMAFile(filename, mode, ...). In this case, the
    encoding, errors and newline arguments must not be provided.

    For text mode, a LZMAFile object is created, and wrapped in an
    io.TextIOWrapper instance with the specified encoding, error
    handling behavior, and line ending(s).

    """
    if "t" in mode:
        if "b" in mode:
            raise ValueError("Invalid mode: %r" % (mode,))
    else:
        if encoding is not None:
            raise ValueError("Argument 'encoding' not supported in binary mode")
        if errors is not None:
            raise ValueError("Argument 'errors' not supported in binary mode")
        if newline is not None:
            raise ValueError("Argument 'newline' not supported in binary mode")

    lz_mode = mode.replace("t", "")
    binary_file = LZMAFile(filename, lz_mode, format=format, check=check,
                           preset=preset, filters=filters)

    if "t" in mode:
        return io.TextIOWrapper(binary_file, encoding, errors, newline)
    else:
        return binary_file


def compress(data, format=FORMAT_XZ, check=-1, preset=None, filters=None):
    """Compress a block of data.

    Refer to LZMACompressor's docstring for a description of the
    optional arguments *format*, *check*, *preset* and *filters*.

    For incremental compression, use an LZMACompressor instead.
    """
    comp = LZMACompressor(format, check, preset, filters)
    return comp.compress(data) + comp.flush()


def decompress(data, format=FORMAT_AUTO, memlimit=None, filters=None):
    """Decompress a block of data.

    Refer to LZMADecompressor's docstring for a description of the
    optional arguments *format*, *check* and *filters*.

    For incremental decompression, use an LZMADecompressor instead.
    """
    results = []
    while True:
        decomp = LZMADecompressor(format, memlimit, filters)
        try:
            res = decomp.decompress(data)
        except LZMAError:
            if results:
                break  # Leftover data is not a valid LZMA/XZ stream; ignore it.
            else:
                raise  # Error on the first iteration; bail out.
        results.append(res)
        if not decomp.eof:
            raise LZMAError("Compressed data ended before the "
                            "end-of-stream marker was reached")
        data = decomp.unused_data
        if not data:
            break
    return b"".join(results)
