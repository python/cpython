import time
import string
import zlib
import StringIO

# implements a python function that reads and writes a gzipped file
# the user of the file doesn't have to worry about the compression,
# but sequential access is not allowed

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

written = []

_py_open = open

def open(filename, mode, compresslevel=9):
    return GzipFile(filename, mode, compresslevel)

class GzipFile:

    def __init__(self, filename, mode='r', compresslevel=9):
	if mode == 'r' or mode == 'rb':
	    self.mode = READ
	    self._init_read()
	    self.filename = filename
	    self.decompress = zlib.decompressobj(-zlib.MAX_WBITS)

	elif mode == 'w' or mode == 'wb':
	    self.mode = WRITE
	    self._init_write(filename)
	    self.compress = zlib.compressobj(compresslevel,
					     zlib.DEFLATED, 
					     -zlib.MAX_WBITS,
					     zlib.DEF_MEM_LEVEL,
					     0)
	else:
	    raise ValueError, "Mode " + mode + " not supported"

	self.fileobj = _py_open(self.filename,mode)

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
	self.fileobj.write(chr(FNAME))
	write32(self.fileobj, int(time.time()))
	self.fileobj.write('\002')
	self.fileobj.write('\377')
	self.fileobj.write(self.filename[:-3] + '\000')

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
		if s=='\000': break
	if flag & FCOMMENT:
	    # Read and discard a null-terminated string containing a comment
	    while (1):
		s=self.fileobj.read(1)
		if s=='\000': break
	if flag & FHCRC:
	    self.fileobj.read(2)     # Read & discard the 16-bit header CRC


    def write(self,data):
	if len(data) > 0:
	    self.size = self.size + len(data)
	    self.crc = zlib.crc32(data, self.crc)
	    self.fileobj.write( self.compress.compress(data) )

    def writelines(self,lines):
	self.write(string.join(lines))

    def read(self,size=None):
	if self.extrasize <= 0 and self.fileobj.closed:
	    return ''
	
	if not size:
	    # get the whole thing
	    try:
		while 1:
		    self._read()
	    except EOFError:
		size = self.extrasize
	else:
	    # just get some more of it
	    try:
		while size > self.extrasize:
		    self._read()
	    except EOFError:
		pass
	
	chunk = self.extrabuf[:size]
	self.extrabuf = self.extrabuf[size:]
	self.extrasize = self.extrasize - size

	return chunk

    def _read(self):
	buf = self.fileobj.read(1024)
	if buf == "":
	    uncompress = self.decompress.flush()
	    if uncompress == "":
		self._read_eof()
		self.fileobj.close()
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
	    self.fileobj.close()
	elif self.mode == READ:
	    self.fileobj.close()

    def flush(self):
	self.fileobj.flush()

    def seek(self):
	raise IOError, 'Random access not allowed in gzip files'

    def tell(self):
	raise IOError, 'I won\'t tell() you for gzip files'

    def isatty(self):
	return 0

    def readline(self):
	# should I bother with this
	raise RuntimeError, "not implemented"

    def readlines(self):
	# should I bother with this
	raise RuntimeError, "not implemented"

    
class StringIOgz(GzipFile):

    """A StringIO substitute that reads/writes gzipped buffers."""

    def __init__(self, buf=None, filename="StringIOgz"):
	"""Read/write mode depends on first argument.

	If __init__ is passed a buffer, it will treat that as the
	gzipped data and set up the StringIO for reading. Without the
	initial argument, it will assume a new file for writing.

	The filename argument is written in the header of buffers
	opened for writing. Not sure that this is useful, but the
	GzipFile code expects *some* filename."""

	if buf:
	    self.mode = READ
	    self._init_read()
	    self.filename = filename
	    self.decompress = zlib.decompressobj(-zlib.MAX_WBITS)
	    self.fileobj = StringIO.StringIO(buf)
	else:
	    self.mode = WRITE
	    self._init_write(filename)
	    self.compress = zlib.compressobj(compresslevel,
					     zlib.DEFLATED, 
					     -zlib.MAX_WBITS,
					     zlib.DEF_MEM_LEVEL,
					     0)
	    self.fileobj = StringIO.StringIO()

	if self.mode == WRITE:
	    self._write_gzip_header()
	elif self.mode == READ:
	    self._read_gzip_header()

