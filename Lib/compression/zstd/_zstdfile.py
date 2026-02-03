import io
from os import PathLike
from _zstd import ZstdCompressor, ZstdDecompressor, ZSTD_DStreamOutSize
from compression._common import _streams

__all__ = ('ZstdFile', 'open')

_MODE_CLOSED = 0
_MODE_READ = 1
_MODE_WRITE = 2


def _nbytes(dat, /):
    if isinstance(dat, (bytes, bytearray)):
        return len(dat)
    with memoryview(dat) as mv:
        return mv.nbytes


class ZstdFile(_streams.BaseStream):
    """A file-like object providing transparent Zstandard (de)compression.

    A ZstdFile can act as a wrapper for an existing file object, or refer
    directly to a named file on disk.

    ZstdFile provides a *binary* file interface. Data is read and returned as
    bytes, and may only be written to objects that support the Buffer Protocol.
    """

    FLUSH_BLOCK = ZstdCompressor.FLUSH_BLOCK
    FLUSH_FRAME = ZstdCompressor.FLUSH_FRAME

    def __init__(self, file, /, mode='r', *,
                 level=None, options=None, zstd_dict=None):
        """Open a Zstandard compressed file in binary mode.

        *file* can be either an file-like object, or a file name to open.

        *mode* can be 'r' for reading (default), 'w' for (over)writing, 'x' for
        creating exclusively, or 'a' for appending. These can equivalently be
        given as 'rb', 'wb', 'xb' and 'ab' respectively.

        *level* is an optional int specifying the compression level to use,
        or COMPRESSION_LEVEL_DEFAULT if not given.

        *options* is an optional dict for advanced compression parameters.
        See CompressionParameter and DecompressionParameter for the possible
        options.

        *zstd_dict* is an optional ZstdDict object, a pre-trained Zstandard
        dictionary. See train_dict() to train ZstdDict on sample data.
        """
        self._fp = None
        self._close_fp = False
        self._mode = _MODE_CLOSED
        self._buffer = None

        if not isinstance(mode, str):
            raise ValueError('mode must be a str')
        if options is not None and not isinstance(options, dict):
            raise TypeError('options must be a dict or None')
        mode = mode.removesuffix('b')  # handle rb, wb, xb, ab
        if mode == 'r':
            if level is not None:
                raise TypeError('level is illegal in read mode')
            self._mode = _MODE_READ
        elif mode in {'w', 'a', 'x'}:
            if level is not None and not isinstance(level, int):
                raise TypeError('level must be int or None')
            self._mode = _MODE_WRITE
            self._compressor = ZstdCompressor(level=level, options=options,
                                              zstd_dict=zstd_dict)
            self._pos = 0
        else:
            raise ValueError(f'Invalid mode: {mode!r}')

        if isinstance(file, (str, bytes, PathLike)):
            self._fp = io.open(file, f'{mode}b')
            self._close_fp = True
        elif ((mode == 'r' and hasattr(file, 'read'))
                or (mode != 'r' and hasattr(file, 'write'))):
            self._fp = file
        else:
            raise TypeError('file must be a file-like object '
                            'or a str, bytes, or PathLike object')

        if self._mode == _MODE_READ:
            raw = _streams.DecompressReader(
                self._fp,
                ZstdDecompressor,
                zstd_dict=zstd_dict,
                options=options,
            )
            self._buffer = io.BufferedReader(raw)

    def close(self):
        """Flush and close the file.

        May be called multiple times. Once the file has been closed,
        any other operation on it will raise ValueError.
        """
        if self._fp is None:
            return
        try:
            if self._mode == _MODE_READ:
                if getattr(self, '_buffer', None):
                    self._buffer.close()
                    self._buffer = None
            elif self._mode == _MODE_WRITE:
                self.flush(self.FLUSH_FRAME)
                self._compressor = None
        finally:
            self._mode = _MODE_CLOSED
            try:
                if self._close_fp:
                    self._fp.close()
            finally:
                self._fp = None
                self._close_fp = False

    def write(self, data, /):
        """Write a bytes-like object *data* to the file.

        Returns the number of uncompressed bytes written, which is
        always the length of data in bytes. Note that due to buffering,
        the file on disk may not reflect the data written until .flush()
        or .close() is called.
        """
        self._check_can_write()

        length = _nbytes(data)

        compressed = self._compressor.compress(data)
        self._fp.write(compressed)
        self._pos += length
        return length

    def flush(self, mode=FLUSH_BLOCK):
        """Flush remaining data to the underlying stream.

        The mode argument can be FLUSH_BLOCK or FLUSH_FRAME. Abuse of this
        method will reduce compression ratio, use it only when necessary.

        If the program is interrupted afterwards, all data can be recovered.
        To ensure saving to disk, also need to use os.fsync(fd).

        This method does nothing in reading mode.
        """
        if self._mode == _MODE_READ:
            return
        self._check_not_closed()
        if mode not in {self.FLUSH_BLOCK, self.FLUSH_FRAME}:
            raise ValueError('Invalid mode argument, expected either '
                             'ZstdFile.FLUSH_FRAME or '
                             'ZstdFile.FLUSH_BLOCK')
        if self._compressor.last_mode == mode:
            return
        # Flush zstd block/frame, and write.
        data = self._compressor.flush(mode)
        self._fp.write(data)
        if hasattr(self._fp, 'flush'):
            self._fp.flush()

    def read(self, size=-1):
        """Read up to size uncompressed bytes from the file.

        If size is negative or omitted, read until EOF is reached.
        Returns b'' if the file is already at EOF.
        """
        if size is None:
            size = -1
        self._check_can_read()
        return self._buffer.read(size)

    def read1(self, size=-1):
        """Read up to size uncompressed bytes, while trying to avoid
        making multiple reads from the underlying stream. Reads up to a
        buffer's worth of data if size is negative.

        Returns b'' if the file is at EOF.
        """
        self._check_can_read()
        if size < 0:
            # Note this should *not* be io.DEFAULT_BUFFER_SIZE.
            # ZSTD_DStreamOutSize is the minimum amount to read guaranteeing
            # a full block is read.
            size = ZSTD_DStreamOutSize
        return self._buffer.read1(size)

    def readinto(self, b):
        """Read bytes into b.

        Returns the number of bytes read (0 for EOF).
        """
        self._check_can_read()
        return self._buffer.readinto(b)

    def readinto1(self, b):
        """Read bytes into b, while trying to avoid making multiple reads
        from the underlying stream.

        Returns the number of bytes read (0 for EOF).
        """
        self._check_can_read()
        return self._buffer.readinto1(b)

    def readline(self, size=-1):
        """Read a line of uncompressed bytes from the file.

        The terminating newline (if present) is retained. If size is
        non-negative, no more than size bytes will be read (in which
        case the line may be incomplete). Returns b'' if already at EOF.
        """
        self._check_can_read()
        return self._buffer.readline(size)

    def seek(self, offset, whence=io.SEEK_SET):
        """Change the file position.

        The new position is specified by offset, relative to the
        position indicated by whence. Possible values for whence are:

            0: start of stream (default): offset must not be negative
            1: current stream position
            2: end of stream; offset must not be positive

        Returns the new file position.

        Note that seeking is emulated, so depending on the arguments,
        this operation may be extremely slow.
        """
        self._check_can_read()

        # BufferedReader.seek() checks seekable
        return self._buffer.seek(offset, whence)

    def peek(self, size=-1):
        """Return buffered data without advancing the file position.

        Always returns at least one byte of data, unless at EOF.
        The exact number of bytes returned is unspecified.
        """
        # Relies on the undocumented fact that BufferedReader.peek() always
        # returns at least one byte (except at EOF)
        self._check_can_read()
        return self._buffer.peek(size)

    def __next__(self):
        if ret := self._buffer.readline():
            return ret
        raise StopIteration

    def tell(self):
        """Return the current file position."""
        self._check_not_closed()
        if self._mode == _MODE_READ:
            return self._buffer.tell()
        elif self._mode == _MODE_WRITE:
            return self._pos

    def fileno(self):
        """Return the file descriptor for the underlying file."""
        self._check_not_closed()
        return self._fp.fileno()

    @property
    def name(self):
        self._check_not_closed()
        return self._fp.name

    @property
    def mode(self):
        return 'wb' if self._mode == _MODE_WRITE else 'rb'

    @property
    def closed(self):
        """True if this file is closed."""
        return self._mode == _MODE_CLOSED

    def seekable(self):
        """Return whether the file supports seeking."""
        return self.readable() and self._buffer.seekable()

    def readable(self):
        """Return whether the file was opened for reading."""
        self._check_not_closed()
        return self._mode == _MODE_READ

    def writable(self):
        """Return whether the file was opened for writing."""
        self._check_not_closed()
        return self._mode == _MODE_WRITE


def open(file, /, mode='rb', *, level=None, options=None, zstd_dict=None,
         encoding=None, errors=None, newline=None):
    """Open a Zstandard compressed file in binary or text mode.

    file can be either a file name (given as a str, bytes, or PathLike object),
    in which case the named file is opened, or it can be an existing file object
    to read from or write to.

    The mode parameter can be 'r', 'rb' (default), 'w', 'wb', 'x', 'xb', 'a',
    'ab' for binary mode, or 'rt', 'wt', 'xt', 'at' for text mode.

    The level, options, and zstd_dict parameters specify the settings the same
    as ZstdFile.

    When using read mode (decompression), the options parameter is a dict
    representing advanced decompression options. The level parameter is not
    supported in this case. When using write mode (compression), only one of
    level, an int representing the compression level, or options, a dict
    representing advanced compression options, may be passed. In both modes,
    zstd_dict is a ZstdDict instance containing a trained Zstandard dictionary.

    For binary mode, this function is equivalent to the ZstdFile constructor:
    ZstdFile(filename, mode, ...). In this case, the encoding, errors and
    newline parameters must not be provided.

    For text mode, an ZstdFile object is created, and wrapped in an
    io.TextIOWrapper instance with the specified encoding, error handling
    behavior, and line ending(s).
    """

    text_mode = 't' in mode
    mode = mode.replace('t', '')

    if text_mode:
        if 'b' in mode:
            raise ValueError(f'Invalid mode: {mode!r}')
    else:
        if encoding is not None:
            raise ValueError('Argument "encoding" not supported in binary mode')
        if errors is not None:
            raise ValueError('Argument "errors" not supported in binary mode')
        if newline is not None:
            raise ValueError('Argument "newline" not supported in binary mode')

    binary_file = ZstdFile(file, mode, level=level, options=options,
                           zstd_dict=zstd_dict)

    if text_mode:
        return io.TextIOWrapper(binary_file, encoding, errors, newline)
    else:
        return binary_file
