"""Functions that read and write gzipped files.

The user of the file doesn't have to worry about the compression,
but random access is not allowed."""

# based on Andrew Kuchling's minigzip.py distributed with the zlib module

import struct, sys, time
import zlib
import __builtin__

__all__ = ["GzipFile","open"]

FTEXT, FHCRC, FEXTRA, FNAME, FCOMMENT = 1, 2, 4, 8, 16

READ, WRITE = 1, 2

def write32(output, value):
    output.write(struct.pack("<l", value))

def write32u(output, value):
    if value < 0:
        value = value + 0x100000000L
    output.write(struct.pack("<L", value))

def read32(input):
    return struct.unpack("<l", input.read(4))[0]

def open(filename, mode="rb", compresslevel=9):
    return GzipFile(filename, mode, compresslevel)

class GzipFile:

    myfileobj = None

    def __init__(self, filename=None, mode=None,
                 compresslevel=9, fileobj=None):
        # guarantee the file is opened in binary mode on platforms
        # that care about that sort of thing
        if mode and 'b' not in mode:
            mode += 'b'
        if fileobj is None:
            fileobj = self.myfileobj = __builtin__.open(filename, mode or 'rb')
        if filename is None:
            if hasattr(fileobj, 'name'): filename = fileobj.name
            else: filename = ''
        if mode is None:
            if hasattr(fileobj, 'mode'): mode = fileobj.mode
            else: mode = 'rb'

        if mode[0:1] == 'r':
            self.mode = READ
            # Set flag indicating start of a new member
            self._new_member = 1
            self.extrabuf = ""
            self.extrasize = 0
            self.filename = filename

        elif mode[0:1] == 'w' or mode[0:1] == 'a':
            self.mode = WRITE
            self._init_write(filename)
            self.compress = zlib.compressobj(compresslevel,
                                             zlib.DEFLATED,
                                             -zlib.MAX_WBITS,
                                             zlib.DEF_MEM_LEVEL,
                                             0)
        else:
            raise ValueError, "Mode " + mode + " not supported"

        self.fileobj = fileobj
        self.offset = 0

        if self.mode == WRITE:
            self._write_gzip_header()

    def __repr__(self):
        s = repr(self.fileobj)
        return '<gzip ' + s[1:-1] + ' ' + hex(id(self)) + '>'

    def _init_write(self, filename):
        if filename[-3:] != '.gz':
            filename = filename + '.gz'
        self.filename = filename
        self.crc = zlib.crc32("")
        self.size = 0
        self.writebuf = []
        self.bufsize = 0

    def _write_gzip_header(self):
        self.fileobj.write('\037\213')             # magic header
        self.fileobj.write('\010')                 # compression method
        fname = self.filename[:-3]
        flags = 0
        if fname:
            flags = FNAME
        self.fileobj.write(chr(flags))
        write32u(self.fileobj, long(time.time()))
        self.fileobj.write('\002')
        self.fileobj.write('\377')
        if fname:
            self.fileobj.write(fname + '\000')

    def _init_read(self):
        self.crc = zlib.crc32("")
        self.size = 0

    def _read_gzip_header(self):
        magic = self.fileobj.read(2)
        if magic != '\037\213':
            raise IOError, 'Not a gzipped file'
        method = ord( self.fileobj.read(1) )
        if method != 8:
            raise IOError, 'Unknown compression method'
        flag = ord( self.fileobj.read(1) )
        # modtime = self.fileobj.read(4)
        # extraflag = self.fileobj.read(1)
        # os = self.fileobj.read(1)
        self.fileobj.read(6)

        if flag & FEXTRA:
            # Read & discard the extra field, if present
            xlen=ord(self.fileobj.read(1))
            xlen=xlen+256*ord(self.fileobj.read(1))
            self.fileobj.read(xlen)
        if flag & FNAME:
            # Read and discard a null-terminated string containing the filename
            while (1):
                s=self.fileobj.read(1)
                if not s or s=='\000': break
        if flag & FCOMMENT:
            # Read and discard a null-terminated string containing a comment
            while (1):
                s=self.fileobj.read(1)
                if not s or s=='\000': break
        if flag & FHCRC:
            self.fileobj.read(2)     # Read & discard the 16-bit header CRC


    def write(self,data):
        if self.fileobj is None:
            raise ValueError, "write() on closed GzipFile object"
        if len(data) > 0:
            self.size = self.size + len(data)
            self.crc = zlib.crc32(data, self.crc)
            self.fileobj.write( self.compress.compress(data) )
            self.offset += len(data)

    def read(self, size=-1):
        if self.extrasize <= 0 and self.fileobj is None:
            return ''

        readsize = 1024
        if size < 0:        # get the whole thing
            try:
                while 1:
                    self._read(readsize)
                    readsize = readsize * 2
            except EOFError:
                size = self.extrasize
        else:               # just get some more of it
            try:
                while size > self.extrasize:
                    self._read(readsize)
                    readsize = readsize * 2
            except EOFError:
                if size > self.extrasize:
                    size = self.extrasize

        chunk = self.extrabuf[:size]
        self.extrabuf = self.extrabuf[size:]
        self.extrasize = self.extrasize - size

        self.offset += size
        return chunk

    def _unread(self, buf):
        self.extrabuf = buf + self.extrabuf
        self.extrasize = len(buf) + self.extrasize
        self.offset -= len(buf)

    def _read(self, size=1024):
        if self.fileobj is None: raise EOFError, "Reached EOF"

        if self._new_member:
            # If the _new_member flag is set, we have to
            # jump to the next member, if there is one.
            #
            # First, check if we're at the end of the file;
            # if so, it's time to stop; no more members to read.
            pos = self.fileobj.tell()   # Save current position
            self.fileobj.seek(0, 2)     # Seek to end of file
            if pos == self.fileobj.tell():
                raise EOFError, "Reached EOF"
            else:
                self.fileobj.seek( pos ) # Return to original position

            self._init_read()
            self._read_gzip_header()
            self.decompress = zlib.decompressobj(-zlib.MAX_WBITS)
            self._new_member = 0

        # Read a chunk of data from the file
        buf = self.fileobj.read(size)

        # If the EOF has been reached, flush the decompression object
        # and mark this object as finished.

        if buf == "":
            uncompress = self.decompress.flush()
            self._read_eof()
            self._add_read_data( uncompress )
            raise EOFError, 'Reached EOF'

        uncompress = self.decompress.decompress(buf)
        self._add_read_data( uncompress )

        if self.decompress.unused_data != "":
            # Ending case: we've come to the end of a member in the file,
            # so seek back to the start of the unused data, finish up
            # this member, and read a new gzip header.
            # (The number of bytes to seek back is the length of the unused
            # data, minus 8 because _read_eof() will rewind a further 8 bytes)
            self.fileobj.seek( -len(self.decompress.unused_data)+8, 1)

            # Check the CRC and file size, and set the flag so we read
            # a new member on the next call
            self._read_eof()
            self._new_member = 1

    def _add_read_data(self, data):
        self.crc = zlib.crc32(data, self.crc)
        self.extrabuf = self.extrabuf + data
        self.extrasize = self.extrasize + len(data)
        self.size = self.size + len(data)

    def _read_eof(self):
        # We've read to the end of the file, so we have to rewind in order
        # to reread the 8 bytes containing the CRC and the file size.
        # We check the that the computed CRC and size of the
        # uncompressed data matches the stored values.
        self.fileobj.seek(-8, 1)
        crc32 = read32(self.fileobj)
        isize = read32(self.fileobj)
        if crc32%0x100000000L != self.crc%0x100000000L:
            raise ValueError, "CRC check failed"
        elif isize != self.size:
            raise ValueError, "Incorrect length of data produced"

    def close(self):
        if self.mode == WRITE:
            self.fileobj.write(self.compress.flush())
            write32(self.fileobj, self.crc)
            write32(self.fileobj, self.size)
            self.fileobj = None
        elif self.mode == READ:
            self.fileobj = None
        if self.myfileobj:
            self.myfileobj.close()
            self.myfileobj = None

    def __del__(self):
        try:
            if (self.myfileobj is None and
                self.fileobj is None):
                return
        except AttributeError:
            return
        self.close()

    def flush(self):
        self.fileobj.flush()

    def isatty(self):
        return 0

    def tell(self):
        return self.offset

    def rewind(self):
        '''Return the uncompressed stream file position indicator to the
        beginning of the file'''
        if self.mode != READ:
            raise IOError("Can't rewind in write mode")
        self.fileobj.seek(0)
        self._new_member = 1
        self.extrabuf = ""
        self.extrasize = 0
        self.offset = 0

    def seek(self, offset):
        if self.mode == WRITE:
            if offset < self.offset:
                raise IOError('Negative seek in write mode')
            count = offset - self.offset
            for i in range(count/1024):
                self.write(1024*'\0')
            self.write((count%1024)*'\0')
        elif self.mode == READ:
            if offset < self.offset:
                # for negative seek, rewind and do positive seek
                self.rewind()
            count = offset - self.offset
            for i in range(count/1024): self.read(1024)
            self.read(count % 1024)

    def readline(self, size=-1):
        if size < 0: size = sys.maxint
        bufs = []
        readsize = min(100, size)    # Read from the file in small chunks
        while 1:
            if size == 0:
                return "".join(bufs) # Return resulting line

            c = self.read(readsize)
            i = c.find('\n')
            if size is not None:
                # We set i=size to break out of the loop under two
                # conditions: 1) there's no newline, and the chunk is
                # larger than size, or 2) there is a newline, but the
                # resulting line would be longer than 'size'.
                if i==-1 and len(c) > size: i=size-1
                elif size <= i: i = size -1

            if i >= 0 or c == '':
                bufs.append(c[:i+1])    # Add portion of last chunk
                self._unread(c[i+1:])   # Push back rest of chunk
                return ''.join(bufs)    # Return resulting line

            # Append chunk to list, decrease 'size',
            bufs.append(c)
            size = size - len(c)
            readsize = min(size, readsize * 2)

    def readlines(self, sizehint=0):
        # Negative numbers result in reading all the lines
        if sizehint <= 0: sizehint = sys.maxint
        L = []
        while sizehint > 0:
            line = self.readline()
            if line == "": break
            L.append( line )
            sizehint = sizehint - len(line)

        return L

    def writelines(self, L):
        for line in L:
            self.write(line)


def _test():
    # Act like gzip; with -d, act like gunzip.
    # The input file is not deleted, however, nor are any other gzip
    # options or features supported.
    args = sys.argv[1:]
    decompress = args and args[0] == "-d"
    if decompress:
        args = args[1:]
    if not args:
        args = ["-"]
    for arg in args:
        if decompress:
            if arg == "-":
                f = GzipFile(filename="", mode="rb", fileobj=sys.stdin)
                g = sys.stdout
            else:
                if arg[-3:] != ".gz":
                    print "filename doesn't end in .gz:", `arg`
                    continue
                f = open(arg, "rb")
                g = __builtin__.open(arg[:-3], "wb")
        else:
            if arg == "-":
                f = sys.stdin
                g = GzipFile(filename="", mode="wb", fileobj=sys.stdout)
            else:
                f = __builtin__.open(arg, "rb")
                g = open(arg + ".gz", "wb")
        while 1:
            chunk = f.read(1024)
            if not chunk:
                break
            g.write(chunk)
        if g is not sys.stdout:
            g.close()
        if f is not sys.stdin:
            f.close()

if __name__ == '__main__':
    _test()
