"""Simple class to read IFF chunks.

An IFF chunk (used in formats such as AIFF, TIFF, RMFF (RealMedia File
Format)) has the following structure:

+----------------+
| ID (4 bytes)   |
+----------------+
| size (4 bytes) |
+----------------+
| data           |
| ...            |
+----------------+

The ID is a 4-byte string which identifies the type of chunk.

The size field (a 32-bit value, encoded using big-endian byte order)
gives the size of the whole chunk, including the 8-byte header.

Usually a IFF-type file consists of one or more chunks.  The proposed
usage of the Chunk class defined here is to instantiate an instance at 
the start of each chunk and read from the instance until it reaches
the end, after which a new instance can be instantiated.  At the end
of the file, creating a new instance will fail with a EOFError
exception.

Usage:
while 1:
    try:
        chunk = Chunk(file)
    except EOFError:
        break
    chunktype = chunk.getname()
    while 1:
        data = chunk.read(nbytes)
        if not data:
            pass
        # do something with data

The interface is file-like.  The implemented methods are:
read, close, seek, tell, isatty.
Extra methods are: skip() (called by close, skips to the end of the chunk),
getname() (returns the name (ID) of the chunk)

The __init__ method has one required argument, a file-like object
(including a chunk instance), and one optional argument, a flag which
specifies whether or not chunks are aligned on 2-byte boundaries.  The 
default is 1, i.e. aligned.
"""

class Chunk:
    def __init__(self, file, align = 1):
        import struct
        self.closed = 0
        self.align = align	# whether to align to word (2-byte) boundaries
        self.file = file
        self.chunkname = file.read(4)
        if len(self.chunkname) < 4:
            raise EOFError
        try:
            self.chunksize = struct.unpack('>l', file.read(4))[0]
        except struct.error:
            raise EOFError
        self.chunksize = self.chunksize - 8 # subtract header
        self.size_read = 0
        try:
            self.offset = self.file.tell()
        except:
            self.seekable = 0
        else:
            self.seekable = 1

    def getname(self):
        """Return the name (ID) of the current chunk."""
        return self.chunkname

    def close(self):
        if not self.closed:
            self.skip()
            self.closed = 1

    def isatty(self):
        if self.closed:
            raise ValueError, "I/O operation on closed file"
        return 0

    def seek(self, pos, mode = 0):
        """Seek to specified position into the chunk.
        Default position is 0 (start of chunk).
        If the file is not seekable, this will result in an error.
	"""

        if self.closed:
            raise ValueError, "I/O operation on closed file"
        if not self.seekable:
            raise IOError, "cannot seek"
        if mode == 1:
            pos = pos + self.size_read
        elif mode == 2:
            pos = pos + self.chunk_size
        if pos < 0 or pos > self.chunksize:
            raise RuntimeError
        self.file.seek(self.offset + pos, 0)
        self.size_read = pos

    def tell(self):
        if self.closed:
            raise ValueError, "I/O operation on closed file"
        return self.size_read

    def read(self, n = -1):
        """Read at most n bytes from the chunk.
        If n is omitted or negative, read until the end
        of the chunk.
	"""

        if self.closed:
            raise ValueError, "I/O operation on closed file"
        if self.size_read >= self.chunksize:
            return ''
        if n < 0:
            n = self.chunksize - self.size_read
        if n > self.chunksize - self.size_read:
             n = self.chunksize - self.size_read
        data = self.file.read(n)
        self.size_read = self.size_read + len(data)
        if self.size_read == self.chunksize and \
           self.align and \
           (self.chunksize & 1):
            dummy = self.file.read(1)
            self.size_read = self.size_read + len(dummy)
        return data

    def skip(self):
        """Skip the rest of the chunk.
        If you are not interested in the contents of the chunk,
        this method should be called so that the file points to
        the start of the next chunk.
	"""

        if self.closed:
            raise ValueError, "I/O operation on closed file"
        if self.seekable:
            try:
                n = self.chunksize - self.size_read
                # maybe fix alignment
                if self.align and (self.chunksize & 1):
                    n = n + 1
                self.file.seek(n, 1)
                self.size_read = self.size_read + n
                return
            except:
                pass
        while self.size_read < self.chunksize:
            n = min(8192, self.chunksize - self.size_read)
            dummy = self.read(n)
            if not dummy:
                raise EOFError

