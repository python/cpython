"""Internal classes used by compression modules"""

import io
import sys

BUFFER_SIZE = io.DEFAULT_BUFFER_SIZE  # Compressed data read chunk size


class BaseStream(io.BufferedIOBase):
    """Mode-checking helper functions."""

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
        if not self.seekable():
            raise io.UnsupportedOperation("The underlying file object "
                                          "does not support seeking")


class DecompressReader(io.RawIOBase):
    """Adapts the decompressor API to a RawIOBase reader API"""

    def readable(self):
        return True

    def __init__(self, fp, decomp_factory, trailing_error=(), *,
                 stream_magic=(), xz_stream_padding=False, **decomp_args):
        self._fp = fp
        self._eof = False
        self._pos = 0  # Current offset in decompressed stream

        # Set to size of decompressed stream once it is known, for SEEK_END
        self._size = -1

        # Save the decompressor factory and arguments.
        # If the file contains multiple compressed streams, each
        # stream will need a separate decompressor object. A new decompressor
        # object is also needed when implementing a backwards seek().
        self._decomp_factory = decomp_factory
        self._decomp_args = decomp_args
        self._decompressor = self._decomp_factory(**self._decomp_args)

        # Exception class to catch from decompressor signifying invalid
        # trailing data to ignore
        self._trailing_error = trailing_error

        # Prefix (or tuple of prefixes) that every compressed stream starts
        # with, if the format has one.  Data starting with it is a damaged
        # stream rather than trailing data, so its error is not ignored.
        self._stream_magic = stream_magic

        # Whether to skip the Stream Padding of the .xz format: null bytes,
        # in multiples of four, after a stream that starts with stream_magic.
        # No other format here has such padding, so only lzma sets this, and
        # stream_magic must then be a single bytes object, not a tuple.
        # _head holds the first bytes of the current stream.
        self._xz_stream_padding = xz_stream_padding and bool(stream_magic)
        self._head = b""

        # Null bytes that _skip_xz_padding() read past but did not skip
        # (only their number is kept), and the data that was read after
        # them.  _read_block() returns these before reading the file again.
        self._unread_nulls = 0
        self._unread_data = b""

    def close(self):
        self._decompressor = None
        return super().close()

    def seekable(self):
        return self._fp.seekable()

    def readinto(self, b):
        with memoryview(b) as view, view.cast("B") as byte_view:
            data = self.read(len(byte_view))
            byte_view[:len(data)] = data
        return len(data)

    def read(self, size=-1):
        if size < 0:
            return self.readall()

        if not size or self._eof:
            return b""
        data = None  # Default if EOF is encountered
        # Depending on the input data, our call to the decompressor may not
        # return any data. In this case, try again after reading another block.
        while True:
            if self._decompressor.eof:
                rawblock = (self._decompressor.unused_data or
                            self._read_block())
                if (self._xz_stream_padding
                        and self._head == self._stream_magic):
                    rawblock = self._skip_xz_padding(rawblock)
                if not rawblock:
                    break
                # Continue to next stream.
                self._decompressor = self._decomp_factory(
                    **self._decomp_args)
                if self._xz_stream_padding:
                    self._head = rawblock[:len(self._stream_magic)]
                try:
                    data = self._decompressor.decompress(rawblock, size)
                except self._trailing_error:
                    if (self._stream_magic
                            and rawblock.startswith(self._stream_magic)):
                        raise  # Damaged stream, not trailing data.
                    # Trailing data isn't a valid compressed stream; ignore it.
                    break
            else:
                if self._decompressor.needs_input:
                    rawblock = self._read_block()
                    if not rawblock:
                        raise EOFError("Compressed file ended before the "
                                       "end-of-stream marker was reached")
                else:
                    rawblock = b""
                if (self._xz_stream_padding
                        and len(self._head) < len(self._stream_magic)):
                    self._head += rawblock[:len(self._stream_magic)
                                           - len(self._head)]
                data = self._decompressor.decompress(rawblock, size)
            if data:
                break
        if not data:
            self._eof = True
            self._size = self._pos
            return b""
        self._pos += len(data)
        return data

    def readall(self):
        chunks = []
        # sys.maxsize means the max length of output buffer is unlimited,
        # so that the whole input buffer can be decompressed within one
        # .decompress() call.
        while data := self.read(sys.maxsize):
            chunks.append(data)

        return b"".join(chunks)

    def _read_block(self):
        # Return the next block of compressed data: what _skip_xz_padding()
        # left unread, if anything, and otherwise a block from the file.
        if self._unread_nulls:
            size = min(self._unread_nulls, BUFFER_SIZE)
            self._unread_nulls -= size
            return bytes(size)
        if self._unread_data:
            rawblock, self._unread_data = self._unread_data, b""
            return rawblock
        return self._fp.read(BUFFER_SIZE)

    def _skip_xz_padding(self, rawblock):
        # Skip the null bytes at the start of the data following an .xz
        # stream if they are Stream Padding, whose size is a multiple of
        # four.  Return the block of data to go on with.
        first = rawblock
        size = 0
        while True:
            rest = rawblock.lstrip(b"\0")
            size += len(rawblock) - len(rest)
            if rest:
                break
            rawblock = self._read_block()
            if not rawblock:
                break
        if size % 4 == 0:
            return rest
        # Not Stream Padding: the null bytes are left to the decompressor,
        # in the blocks they were read in, as if nothing had been read past
        # the first one.  There can be any number of null bytes, so the
        # blocks of them between the first and the last are not kept.
        if rawblock is not first:
            self._unread_nulls = (size - len(first)
                                  - (len(rawblock) - len(rest)))
            self._unread_data = rawblock
        return first

    # Rewind the file to the beginning of the data stream.
    def _rewind(self):
        self._fp.seek(0)
        self._eof = False
        self._pos = 0
        self._decompressor = self._decomp_factory(**self._decomp_args)
        self._head = b""
        self._unread_nulls = 0
        self._unread_data = b""

    def seek(self, offset, whence=io.SEEK_SET):
        # Recalculate offset as an absolute file position.
        if whence == io.SEEK_SET:
            pass
        elif whence == io.SEEK_CUR:
            offset = self._pos + offset
        elif whence == io.SEEK_END:
            # Seeking relative to EOF - we need to know the file's size.
            if self._size < 0:
                while self.read(io.DEFAULT_BUFFER_SIZE):
                    pass
            offset = self._size + offset
        else:
            raise ValueError("Invalid value for whence: {}".format(whence))

        # Make it so that offset is the number of bytes to skip forward.
        if offset < self._pos:
            self._rewind()
        else:
            offset -= self._pos

        # Read and discard data until we reach the desired position.
        while offset > 0:
            data = self.read(min(io.DEFAULT_BUFFER_SIZE, offset))
            if not data:
                break
            offset -= len(data)

        return self._pos

    def tell(self):
        """Return the current file position."""
        return self._pos


def _xz_padding_size(data, pos):
    """Return the size of the .xz Stream Padding at *data[pos:]*, or 0.

    Stream Padding is a run of null bytes whose size is a multiple of four.
    """
    end = pos
    # There is usually no padding, or little of it: the chunks to look at
    # start small.  Each one is copied, so their size is limited too.
    chunk_size = 4
    while end < len(data):
        with data[end:end + chunk_size] as chunk:
            block = chunk.tobytes()
        rest = len(block.lstrip(b"\0"))
        end += len(block) - rest
        if rest:
            break
        chunk_size = min(chunk_size * 2, BUFFER_SIZE)
    size = end - pos
    return 0 if size % 4 else size


def _decompress_all(data, decomp_factory, trailing_error=(), *,
                    stream_magic=(), xz_stream_padding=False,
                    truncated_error, **decomp_args):
    """Decompress all of the concatenated streams in *data*.

    Each stream is decompressed by a new decompressor, made by calling
    *decomp_factory* with *decomp_args*.  As in DecompressReader, an error
    of class *trailing_error* in a stream after the first one ends the
    data, unless that stream starts with *stream_magic*.  A stream without
    an end-of-stream marker raises *truncated_error*.  If *xz_stream_padding*
    is true, the Stream Padding of the .xz format is skipped after a stream
    that starts with *stream_magic*, which must then be a bytes object.
    """
    results = []
    # The views are released on exit, so that the caller can resize a
    # bytearray argument again, also while handling an exception.
    with memoryview(data) as view, view.cast("B") as data:
        pos = 0
        # There is usually a single stream: the first chunk is all of the
        # data, to decompress it in one go.
        chunk_size = len(data)
        while True:
            decomp = decomp_factory(**decomp_args)
            start = pos
            res = []
            try:
                # Feed the stream in chunks of increasing size, so that what
                # is read past its end (and copied into unused_data) is
                # bounded by the size of the stream, not by the size of all
                # the streams that follow it.
                while not decomp.eof and pos < len(data):
                    with data[pos:pos + chunk_size] as chunk:
                        res.append(decomp.decompress(chunk))
                        pos += len(chunk)
                    chunk_size *= 2
            except trailing_error:
                if not results:
                    raise  # Error in the first stream; bail out.
                if stream_magic:
                    if isinstance(stream_magic, bytes):
                        stream_magic = (stream_magic,)
                    size = max(map(len, stream_magic))
                    with data[start:start + size] as head:
                        if head.tobytes().startswith(stream_magic):
                            raise  # Damaged stream, not trailing data.
                # Trailing data isn't a valid compressed stream; ignore it.
                break
            results.extend(res)
            if not decomp.eof:
                raise truncated_error("Compressed data ended before the "
                                      "end-of-stream marker was reached")
            pos -= len(decomp.unused_data)
            if xz_stream_padding and stream_magic:
                with data[start:start + len(stream_magic)] as head:
                    if head == stream_magic:
                        pos += _xz_padding_size(data, pos)
            if pos == len(data):
                break
            # Later streams start small: a larger first chunk reads further
            # past the end of a short stream, a smaller one needs more calls.
            chunk_size = 1024
    return b"".join(results)
