# Stuff to parse WAVE files.
#
# Usage.
#
# Reading WAVE files:
#	f = wave.open(file, 'r')
# where file is either the name of a file or an open file pointer.
# The open file pointer must have methods read(), seek(), and close().
# When the setpos() and rewind() methods are not used, the seek()
# method is not  necessary.
#
# This returns an instance of a class with the following public methods:
#	getnchannels()	-- returns number of audio channels (1 for
#			   mono, 2 for stereo)
#	getsampwidth()	-- returns sample width in bytes
#	getframerate()	-- returns sampling frequency
#	getnframes()	-- returns number of audio frames
#	getcomptype()	-- returns compression type ('NONE' for AIFF files)
#	getcompname()	-- returns human-readable version of
#			   compression type ('not compressed' for AIFF files)
#	getparams()	-- returns a tuple consisting of all of the
#			   above in the above order
#	getmarkers()	-- returns None (for compatibility with the
#			   aifc module)
#	getmark(id)	-- raises an error since the mark does not
#			   exist (for compatibility with the aifc module)
#	readframes(n)	-- returns at most n frames of audio
#	rewind()	-- rewind to the beginning of the audio stream
#	setpos(pos)	-- seek to the specified position
#	tell()		-- return the current position
#	close()		-- close the instance (make it unusable)
# The position returned by tell() and the position given to setpos()
# are compatible and have nothing to do with the actual postion in the
# file.
# The close() method is called automatically when the class instance
# is destroyed.
#
# Writing WAVE files:
#	f = wave.open(file, 'w')
# where file is either the name of a file or an open file pointer.
# The open file pointer must have methods write(), tell(), seek(), and
# close().
#
# This returns an instance of a class with the following public methods:
#	setnchannels(n)	-- set the number of channels
#	setsampwidth(n)	-- set the sample width
#	setframerate(n)	-- set the frame rate
#	setnframes(n)	-- set the number of frames
#	setcomptype(type, name)
#			-- set the compression type and the
#			   human-readable compression type
#	setparams(nchannels, sampwidth, framerate, nframes, comptype, compname)
#			-- set all parameters at once
#	tell()		-- return current position in output file
#	writeframesraw(data)
#			-- write audio frames without pathing up the
#			   file header
#	writeframes(data)
#			-- write audio frames and patch up the file header
#	close()		-- patch up the file header and close the
#			   output file
# You should set the parameters before the first writeframesraw or
# writeframes.  The total number of frames does not need to be set,
# but when it is set to the correct value, the header does not have to
# be patched up.
# It is best to first set all parameters, perhaps possibly the
# compression type, and then write audio frames using writeframesraw.
# When all frames have been written, either call writeframes('') or
# close() to patch up the sizes in the header.
# The close() method is called automatically when the class instance
# is destroyed.

import __builtin__

Error = 'wave.Error'

WAVE_FORMAT_PCM = 0x0001

_array_fmts = None, 'b', 'h', None, 'l'

def _read_long(file):
	x = 0L
	for i in range(4):
		byte = file.read(1)
		if byte == '':
			raise EOFError
		x = x + (ord(byte) << (8 * i))
	if x >= 0x80000000L:
		x = x - 0x100000000L
	return int(x)

def _read_ulong(file):
	x = 0L
	for i in range(4):
		byte = file.read(1)
		if byte == '':
			raise EOFError
		x = x + (ord(byte) << (8 * i))
	return x

def _read_short(file):
	x = 0
	for i in range(2):
		byte = file.read(1)
		if byte == '':
			raise EOFError
		x = x + (ord(byte) << (8 * i))
	if x >= 0x8000:
		x = x - 0x10000
	return x

def _write_short(f, x):
	d, m = divmod(x, 256)
	f.write(chr(m))
	f.write(chr(d))

def _write_long(f, x):
	if x < 0:
		x = x + 0x100000000L
	for i in range(4):
		d, m = divmod(x, 256)
		f.write(chr(int(m)))
		x = d

class Chunk:
	def __init__(self, file):
		self.file = file
		self.chunkname = self.file.read(4)
		if len(self.chunkname) < 4:
			raise EOFError
		self.chunksize = _read_long(self.file)
		self.size_read = 0
		self.offset = self.file.tell()

	def rewind(self):
		self.file.seek(self.offset, 0)
		self.size_read = 0

	def setpos(self, pos):
		if pos < 0 or pos > self.chunksize:
			raise RuntimeError
		self.file.seek(self.offset + pos, 0)
		self.size_read = pos
		
	def read(self, length):
		if self.size_read >= self.chunksize:
			return ''
		if length > self.chunksize - self.size_read:
 			length = self.chunksize - self.size_read
		data = self.file.read(length)
		self.size_read = self.size_read + len(data)
		return data

	def skip(self):
		try:
			self.file.seek(self.chunksize - self.size_read, 1)
		except RuntimeError:
			while self.size_read < self.chunksize:
				dummy = self.read(8192)
				if not dummy:
					raise EOFError

class Wave_read:
	# Variables used in this class:
	#
	# These variables are available to the user though appropriate
	# methods of this class:
	# _file -- the open file with methods read(), close(), and seek()
	#		set through the __init__() method
	# _nchannels -- the number of audio channels
	#		available through the getnchannels() method
	# _nframes -- the number of audio frames
	#		available through the getnframes() method
	# _sampwidth -- the number of bytes per audio sample
	#		available through the getsampwidth() method
	# _framerate -- the sampling frequency
	#		available through the getframerate() method
	# _comptype -- the AIFF-C compression type ('NONE' if AIFF)
	#		available through the getcomptype() method
	# _compname -- the human-readable AIFF-C compression type
	#		available through the getcomptype() method
	# _soundpos -- the position in the audio stream
	#		available through the tell() method, set through the
	#		setpos() method
	#
	# These variables are used internally only:
	# _fmt_chunk_read -- 1 iff the FMT chunk has been read
	# _data_seek_needed -- 1 iff positioned correctly in audio
	#		file for readframes()
	# _data_chunk -- instantiation of a chunk class for the DATA chunk
	# _framesize -- size of one frame in the file

	access _file, _nchannels, _nframes, _sampwidth, _framerate, \
		  _comptype, _compname, _soundpos, \
		  _fmt_chunk_read, _data_seek_needed, \
		  _data_chunk, _framesize: private

	def initfp(self, file):
		self._file = file
		self._convert = None
		self._soundpos = 0
		form = self._file.read(4)
		if form != 'RIFF':
			raise Error, 'file does not start with RIFF id'
		formlength = _read_long(self._file)
		if formlength <= 0:
			raise Error, 'invalid FORM chunk data size'
		formdata = self._file.read(4)
		formlength = formlength - 4
		if formdata != 'WAVE':
			raise Error, 'not a WAVE file'
		self._fmt_chunk_read = 0
		while formlength > 0:
			self._data_seek_needed = 1
			chunk = Chunk(self._file)
			if chunk.chunkname == 'fmt ':
				self._read_fmt_chunk(chunk)
				self._fmt_chunk_read = 1
			elif chunk.chunkname == 'data':
				if not self._fmt_chunk_read:
					raise Error, 'data chunk before fmt chunk'
				self._data_chunk = chunk
				self._nframes = chunk.chunksize / self._framesize
				self._data_seek_needed = 0
			formlength = formlength - 8 - chunk.chunksize
			if formlength > 0:
				chunk.skip()
		if not self._fmt_chunk_read or not self._data_chunk:
			raise Error, 'fmt chunk and/or data chunk missing'

	def __init__(self, f):
		if type(f) == type(''):
			f = __builtin__.open(f, 'r')
		# else, assume it is an open file object already
		self.initfp(f)

	def __del__(self):
		if self._file:
			self.close()

	#
	# User visible methods.
	#
	def getfp(self):
		return self._file

	def rewind(self):
		self._data_seek_needed = 1
		self._soundpos = 0

	def close(self):
		self._file = None

	def tell(self):
		return self._soundpos

	def getnchannels(self):
		return self._nchannels

	def getnframes(self):
		return self._nframes

	def getsampwidth(self):
		return self._sampwidth

	def getframerate(self):
		return self._framerate

	def getcomptype(self):
		return self._comptype

	def getcompname(self):
		return self._compname

	def getparams(self):
		return self.getnchannels(), self.getsampwidth(), \
			  self.getframerate(), self.getnframes(), \
			  self.getcomptype(), self.getcompname()

	def getmarkers(self):
		return None

	def getmark(self, id):
		raise Error, 'no marks'

	def setpos(self, pos):
		if pos < 0 or pos > self._nframes:
			raise Error, 'position not in range'
		self._soundpos = pos
		self._data_seek_needed = 1

	def readframes(self, nframes):
		if self._data_seek_needed:
			self._data_chunk.rewind()
			pos = self._soundpos * self._framesize
			if pos:
				self._data_chunk.setpos(pos)
			self._data_seek_needed = 0
		if nframes == 0:
			return ''
		if self._sampwidth > 1:
			# unfortunately the fromfile() method does not take
			# something that only looks like a file object, so
			# we have to reach into the innards of the chunk object
			import array
			data = array.array(_array_fmts[self._sampwidth])
			nitems = nframes * self._nchannels
			if nitems * self._sampwidth > self._data_chunk.chunksize - self._data_chunk.size_read:
				nitems = (self._data_chunk.chunksize - self._data_chunk.size_read) / self._sampwidth
			data.fromfile(self._data_chunk.file, nitems)
			self._data_chunk.size_read = self._data_chunk.size_read + nitems * self._sampwidth
			data.byteswap()
			data = data.tostring()
		else:
			data = self._data_chunk.read(nframes * self._framesize)
		if self._convert and data:
			data = self._convert(data)
		self._soundpos = self._soundpos + len(data) / (self._nchannels * self._sampwidth)
		return data

	#
	# Internal methods.
	#
	access *: private

	def _read_fmt_chunk(self, chunk):
		wFormatTag = _read_short(chunk)
		self._nchannels = _read_short(chunk)
		self._framerate = _read_long(chunk)
		dwAvgBytesPerSec = _read_long(chunk)
		wBlockAlign = _read_short(chunk)
		if wFormatTag == WAVE_FORMAT_PCM:
			self._sampwidth = (_read_short(chunk) + 7) / 8
		else:
			raise Error, 'unknown format'
		self._framesize = self._nchannels * self._sampwidth
		self._comptype = 'NONE'
		self._compname = 'not compressed'

class Wave_write:
	# Variables used in this class:
	#
	# These variables are user settable through appropriate methods
	# of this class:
	# _file -- the open file with methods write(), close(), tell(), seek()
	#		set through the __init__() method
	# _comptype -- the AIFF-C compression type ('NONE' in AIFF)
	#		set through the setcomptype() or setparams() method
	# _compname -- the human-readable AIFF-C compression type
	#		set through the setcomptype() or setparams() method
	# _nchannels -- the number of audio channels
	#		set through the setnchannels() or setparams() method
	# _sampwidth -- the number of bytes per audio sample
	#		set through the setsampwidth() or setparams() method
	# _framerate -- the sampling frequency
	#		set through the setframerate() or setparams() method
	# _nframes -- the number of audio frames written to the header
	#		set through the setnframes() or setparams() method
	#
	# These variables are used internally only:
	# _datalength -- the size of the audio samples written to the header
	# _nframeswritten -- the number of frames actually written
	# _datawritten -- the size of the audio samples actually written

	access _file, _comptype, _compname, _nchannels, _sampwidth, \
		  _framerate, _nframes, _nframeswritten, \
		  _datalength, _datawritten: private

	def __init__(self, f):
		if type(f) == type(''):
			f = __builtin__.open(f, 'w')
		self.initfp(f)

	def initfp(self, file):
		self._file = file
		self._convert = None
		self._nchannels = 0
		self._sampwidth = 0
		self._framerate = 0
		self._nframes = 0
		self._nframeswritten = 0
		self._datawritten = 0
		self._datalength = 0

	def __del__(self):
		if self._file:
			self.close()

	#
	# User visible methods.
	#
	def setnchannels(self, nchannels):
		if self._datawritten:
			raise Error, 'cannot change parameters after starting to write'
		if nchannels < 1:
			raise Error, 'bad # of channels'
		self._nchannels = nchannels

	def getnchannels(self):
		if not self._nchannels:
			raise Error, 'number of channels not set'
		return self._nchannels

	def setsampwidth(self, sampwidth):
		if self._datawritten:
			raise Error, 'cannot change parameters after starting to write'
		if sampwidth < 1 or sampwidth > 4:
			raise Error, 'bad sample width'
		self._sampwidth = sampwidth

	def getsampwidth(self):
		if not self._sampwidth:
			raise Error, 'sample width not set'
		return self._sampwidth

	def setframerate(self, framerate):
		if self._datawritten:
			raise Error, 'cannot change parameters after starting to write'
		if framerate <= 0:
			raise Error, 'bad frame rate'
		self._framerate = framerate

	def getframerate(self):
		if not self._framerate:
			raise Error, 'frame rate not set'
		return self._framerate

	def setnframes(self, nframes):
		if self._datawritten:
			raise Error, 'cannot change parameters after starting to write'
		self._nframes = nframes

	def getnframes(self):
		return self._nframeswritten

	def setcomptype(self, comptype, compname):
		if self._datawritten:
			raise Error, 'cannot change parameters after starting to write'
		if comptype not in ('NONE',):
			raise Error, 'unsupported compression type'
		self._comptype = comptype
		self._compname = compname

	def getcomptype(self):
		return self._comptype

	def getcompname(self):
		return self._compname

	def setparams(self, (nchannels, sampwidth, framerate, nframes, comptype, compname)):
		if self._datawritten:
			raise Error, 'cannot change parameters after starting to write'
		self.setnchannels(nchannels)
		self.setsampwidth(sampwidth)
		self.setframerate(framerate)
		self.setnframes(nframes)
		self.setcomptype(comptype, compname)

	def getparams(self):
		if not self._nchannels or not self._sampwidth or not self._framerate:
			raise Error, 'not all parameters set'
		return self._nchannels, self._sampwidth, self._framerate, \
			  self._nframes, self._comptype, self._compname

	def setmark(self, id, pos, name):
		raise Error, 'setmark() not supported'

	def getmark(self, id):
		raise Error, 'no marks'

	def getmarkers(self):
		return None
				
	def tell(self):
		return self._nframeswritten

	def writeframesraw(self, data):
		self._ensure_header_written(len(data))
		nframes = len(data) / (self._sampwidth * self._nchannels)
		if self._convert:
			data = self._convert(data)
		if self._sampwidth > 1:
			import array
			data = array.array(_array_fmts[self._sampwidth], data)
			data.byteswap()
			data.tofile(self._file)
			self._datawritten = self._datawritten + len(data) * self._sampwidth
		else:
			self._file.write(data)
			self._datawritten = self._datawritten + len(data)
		self._nframeswritten = self._nframeswritten + nframes

	def writeframes(self, data):
		self.writeframesraw(data)
		if self._datalength != self._datawritten:
			self._patchheader()

	def close(self):
		self._ensure_header_written(0)
		if self._datalength != self._datawritten:
			self._patchheader()
		self._file.flush()
		self._file = None

	#
	# Internal methods.
	#
	access *: private

	def _ensure_header_written(self, datasize):
		if not self._datawritten:
			if not self._nchannels:
				raise Error, '# channels not specified'
			if not self._sampwidth:
				raise Error, 'sample width not specified'
			if not self._framerate:
				raise Error, 'sampling rate not specified'
			self._write_header(datasize)

	def _write_header(self, initlength):
		self._file.write('RIFF')
		if not self._nframes:
			self._nframes = initlength / (self._nchannels * self._sampwidth)
		self._datalength = self._nframes * self._nchannels * self._sampwidth
		self._form_length_pos = self._file.tell()
		_write_long(self._file, 36 + self._datalength)
		self._file.write('WAVE')
		self._file.write('fmt ')
		_write_long(self._file, 16)
		_write_short(self._file, WAVE_FORMAT_PCM)
		_write_short(self._file, self._nchannels)
		_write_long(self._file, self._framerate)
		_write_long(self._file, self._nchannels * self._framerate * self._sampwidth)
		_write_short(self._file, self._nchannels * self._sampwidth)
		_write_short(self._file, self._sampwidth * 8)
		self._file.write('data')
		self._data_length_pos = self._file.tell()
		_write_long(self._file, self._datalength)

	def _patchheader(self):
		if self._datawritten == self._datalength:
			return
		curpos = self._file.tell()
		self._file.seek(self._form_length_pos, 0)
		_write_long(36 + self._datawritten)
		self._file.seek(self._data_length_pos, 0)
		_write_long(self._file, self._datawritten)
		self._file.seek(curpos, 0)
		self._datalength = self._datawritten

def open(f, mode):
	if mode == 'r':
		return Wave_read(f)
	elif mode == 'w':
		return Wave_write(f)
	else:
		raise Error, "mode must be 'r' or 'w'"

openfp = open # B/W compatibility
