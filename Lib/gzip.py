import time
import string
import zlib
import StringIO
import __builtin__

# implements a python function that reads and writes a gzipped file
# the user of the file doesn't have to worry about the compression,
# but random access is not allowed

# based on Andrew Kuchling's minigzip.py distributed with the zlib module

FTEXT, FHCRC, FEXTRA, FNAME, FCOMMENT = 1, 2, 4, 8, 16

READ, WRITE = 1, 2

def write32(output, value):
    t = divmod(value, 256)
    b1 = chr(t[1])

    t = divmod(t[0], 256)
    b2 = chr(t[1])

    t = divmod(t[0], 256)
    b3 = chr(t[1])

    t = divmod(t[0], 256)
    b4 = chr(t[1])

    buf = b1 + b2 + b3 + b4
    output.write(buf)
   
    
def read32(input):
    buf = input.read(4)
    v = ord(buf[0])
    v = v + (ord(buf[1]) << 8)
    v = v + (ord(buf[2]) << 16)
    v = v + (ord(buf[3]) << 24)
    return v

def open(filename, mode="r", compresslevel=9):
    return GzipFile(filename, mode, compresslevel)

class GzipFile:

    myfileobj = None

    def __init__(self, filename=None, mode=None, 
                 compresslevel=9, fileobj=None):
        if fileobj is None:
            fileobj = self.myfileobj = __builtin__.open(filename, mode or 'r')
        if filename is None:
            if hasattr(fileobj, 'name'): filename = fileobj.name
            else: filename = ''
        if mode is None:
            if hasattr(fileobj, 'mode'): mode = fileobj.mode
            else: mode = 'r'

        if mode[0:1] == 'r':
            self.mode = READ
            self._init_read()
            self.filename = filename
            self.decompress = zlib.decompressobj(-zlib.MAX_WBITS)

        elif mode[0:1] == 'w':
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

        if self.mode == WRITE:
            self._write_gzip_header()
        elif self.mode == READ:
            self._read_gzip_header()

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
        write32(self.fileobj, int(time.time()))
        self.fileobj.write('\002')
        self.fileobj.write('\377')
        if fname:
            self.fileobj.write(fname + '\000')

    def _init_read(self):
        self.crc = zlib.crc32("")
        self.size = 0
        self.extrabuf = ""
        self.extrasize = 0

    def _read_gzip_header(self):
        magic = self.fileobj.read(2)
        if magic != '\037\213':
            raise RuntimeError, 'Not a gzipped file'
        method = ord( self.fileobj.read(1) )
        if method != 8:
            raise RuntimeError, 'Unknown compression method'
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

    def writelines(self,lines):
        self.write(string.join(lines))

    def read(self,size=None):
        if self.extrasize <= 0 and self.fileobj is None:
            return ''

        readsize = 1024
        if not size:        # get the whole thing
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
                pass
        
        chunk = self.extrabuf[:size]
        self.extrabuf = self.extrabuf[size:]
        self.extrasize = self.extrasize - size

        return chunk

    def _unread(self, buf):
        self.extrabuf = buf + self.extrabuf
        self.extrasize = len(buf) + self.extrasize

    def _read(self, size=1024):
        try:
            buf = self.fileobj.read(size)
        except AttributeError:
            raise EOFError, "Reached EOF"
        if buf == "":
            uncompress = self.decompress.flush()
            if uncompress == "":
                self._read_eof()
                self.fileobj = None
                raise EOFError, 'Reached EOF'
        else:
            uncompress = self.decompress.decompress(buf)
        self.crc = zlib.crc32(uncompress, self.crc)
        self.extrabuf = self.extrabuf + uncompress
        self.extrasize = self.extrasize + len(uncompress)
        self.size = self.size + len(uncompress)

    def _read_eof(self):
        # Andrew writes:
        ## We've read to the end of the file, so we have to rewind in order
        ## to reread the 8 bytes containing the CRC and the file size.  The
        ## decompressor is smart and knows when to stop, so feeding it
        ## extra data is harmless.  
        self.fileobj.seek(-8, 2)
        crc32 = read32(self.fileobj)
        isize = read32(self.fileobj)
        if crc32 != self.crc:
            self.error = "CRC check failed"
        elif isize != self.size:
            self.error = "Incorrect length of data produced"

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

    def flush(self):
        self.fileobj.flush()

    def seek(self):
        raise IOError, 'Random access not allowed in gzip files'

    def tell(self):
        raise IOError, 'I won\'t tell() you for gzip files'

    def isatty(self):
        return 0

    def readline(self):
        bufs = []
        readsize = 100
        while 1:
            c = self.read(readsize)
            i = string.find(c, '\n')
            if i >= 0 or c == '':
                bufs.append(c[:i])
                self._unread(c[i+1:])
                return string.join(bufs, '')
            bufs.append(c)
            readsize = readsize * 2

    def readlines(self):
        buf = self.read()
        return string.split(buf, '\n')

    def writelines(self, L):
        for line in L:
            self.write(line)


def _test():
    # Act like gzip; with -d, act like gunzip.
    # The input file is not deleted, however, nor are any other gzip
    # options or features supported.
    import sys
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
