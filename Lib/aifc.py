# Stuff to parse AIFF-C and AIFF files.
#
# Unless explicitly stated otherwise, the description below is true
# both for AIFF-C files and AIFF files.
#
# An AIFF-C file has the following structure.
#
#	+-----------------+
#	| FORM            |
#	+-----------------+
#	| <size>          |
#	+----+------------+
#	|    | AIFC       |
#	|    +------------+
#	|    | <chunks>   |
#	|    |    .       |
#	|    |    .       |
#	|    |    .       |
#	+----+------------+
#
# An AIFF file has the string "AIFF" instead of "AIFC".
#
# A chunk consists of an identifier (4 bytes) followed by a size (4 bytes,
# big endian order), followed by the data.  The size field does not include
# the size of the 8 byte header.
#
# The following chunk types are recognized.
#
#	FVER
#		<version number of AIFF-C defining document> (AIFF-C only).
#	MARK
#		<# of markers> (2 bytes)
#		list of markers:
#			<marker ID> (2 bytes, must be > 0)
#			<position> (4 bytes)
#			<marker name> ("pstring")
#	COMM
#		<# of channels> (2 bytes)
#		<# of sound frames> (4 bytes)
#		<size of the samples> (2 bytes)
#		<sampling frequency> (10 bytes, IEEE 80-bit extended
#			floating point)
#		if AIFF-C files only:
#		<compression type> (4 bytes)
#		<human-readable version of compression type> ("pstring")
#	SSND
#		<offset> (4 bytes, not used by this program)
#		<blocksize> (4 bytes, not used by this program)
#		<sound data>
#
# A pstring consists of 1 byte length, a string of characters, and 0 or 1
# byte pad to make the total length even.
#
# Usage.
#
# Reading AIFF files:
#	f = aifc.open(file, 'r')
# or
#	f = aifc.openfp(filep, 'r')
# where file is the name of a file and filep is an open file pointer.
# The open file pointer must have methods  read(), seek(), and
# close().  In some types of audio files, if the setpos() method is
# not used, the seek() method is not necessary.
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
#	getmarkers()	-- get the list of marks in the audio file or None
#			   if there are no marks
#	getmark(id)	-- get mark with the specified id (raises an error
#			   if the mark does not exist)
#	readframes(n)	-- returns at most n frames of audio
#	rewind()	-- rewind to the beginning of the audio stream
#	setpos(pos)	-- seek to the specified position
#	tell()		-- return the current position
# The position returned by tell(), the position given to setpos() and
# the position of marks are all compatible and have nothing to do with
# the actual postion in the file.
#
# Writing AIFF files:
#	f = aifc.open(file, 'w')
# or
#	f = aifc.openfp(filep, 'w')
# where file is the name of a file and filep is an open file pointer.
# The open file pointer must have methods write(), tell(), seek(), and
# close().
#
# This returns an instance of a class with the following public methods:
#	aiff()		-- create an AIFF file (AIFF-C default)
#	aifc()		-- create an AIFF-C file
#	setnchannels(n)	-- set the number of channels
#	setsampwidth(n)	-- set the sample width
#	setframerate(n)	-- set the frame rate
#	setnframes(n)	-- set the number of frames
#	setcomptype(type, name)
#			-- set the compression type and the
#			   human-readable compression type
#	setparams(nchannels, sampwidth, framerate, nframes, comptype, compname)
#			-- set all parameters at once
#	setmark(id, pos, name)
#			-- add specified mark to the list of marks
#	tell()		-- return current position in output file (useful
#			   in combination with setmark())
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
# compression type, and the write audio frames using writeframesraw.
# When all frames have been written, either call writeframes('') or
# close() to patch up the sizes in the header.
# Marks can be added anytime.  If there are any marks, ypu must call
# close() after all frames have been written.
#
# When a file is opened with the extension '.aiff', an AIFF file is
# written, otherwise an AIFF-C file is written.  This default can be
# changed by calling aiff() or aifc() before the first writeframes or
# writeframesraw.

import builtin
import AL
try:
	import CL
except ImportError:
	pass

Error = 'aifc.Error'

_AIFC_version = 0xA2805140		# Version 1 of AIFF-C

_skiplist = 'COMT', 'INST', 'MIDI', 'AESD', \
	  'APPL', 'NAME', 'AUTH', '(c) ', 'ANNO'

_nchannelslist = [(1, AL.MONO), (2, AL.STEREO)]
_sampwidthlist = [(8, AL.SAMPLE_8), (16, AL.SAMPLE_16), (24, AL.SAMPLE_24)]
_frameratelist = [(48000, AL.RATE_48000), \
		  (44100, AL.RATE_44100), \
		  (32000, AL.RATE_32000), \
		  (22050, AL.RATE_22050), \
		  (16000, AL.RATE_16000), \
		  (11025, AL.RATE_11025), \
		  ( 8000,  AL.RATE_8000)]

def _convert1(value, list):
	for t in list:
		if value == t[0]:
			return t[1]
	raise Error, 'unknown parameter value'

def _convert2(value, list):
	for t in list:
		if value == t[1]:
			return t[0]
	raise Error, 'unknown parameter value'

def _read_long(file):
	x = 0L
	for i in range(4):
		byte = file.read(1)
		if byte == '':
			raise EOFError
		x = x*256 + ord(byte)
	if x >= 0x80000000L:
		x = x - 0x100000000L
	return int(x)

def _read_ulong(file):
	x = 0L
	for i in range(4):
		byte = file.read(1)
		if byte == '':
			raise EOFError
		x = x*256 + ord(byte)
	return x

def _read_short(file):
	x = 0
	for i in range(2):
		byte = file.read(1)
		if byte == '':
			raise EOFError
		x = x*256 + ord(byte)
	if x >= 0x8000:
		x = x - 0x10000
	return x

def _read_string(file):
	length = ord(file.read(1))
	data = file.read(length)
	if length & 1 == 0:
		dummy = file.read(1)
	return data

_HUGE_VAL = 1.79769313486231e+308 # See <limits.h>

def _read_float(f): # 10 bytes
	import math
	expon = _read_short(f) # 2 bytes
	sign = 1
	if expon < 0:
		sign = -1
		expon = expon + 0x8000
	himant = _read_ulong(f) # 4 bytes
	lomant = _read_ulong(f) # 4 bytes
	if expon == himant == lomant == 0:
		f = 0.0
	elif expon == 0x7FFF:
		f = _HUGE_VAL
	else:
		expon = expon - 16383
		f = (himant * 0x100000000L + lomant) * pow(2.0, expon - 63)
	return sign * f

def _write_short(f, x):
	d, m = divmod(x, 256)
	f.write(chr(d))
	f.write(chr(m))

def _write_long(f, x):
	if x < 0:
		x = x + 0x100000000L
	data = []
	for i in range(4):
		d, m = divmod(x, 256)
		data.insert(0, m)
		x = d
	for i in range(4):
		f.write(chr(int(data[i])))

def _write_string(f, s):
	f.write(chr(len(s)))
	f.write(s)
	if len(s) & 1 == 0:
		f.write(chr(0))

def _write_float(f, x):
	import math
	if x < 0:
		sign = 0x8000
		x = x * -1
	else:
		sign = 0
	if x == 0:
		expon = 0
		himant = 0
		lomant = 0
	else:
		fmant, expon = math.frexp(x)
		if expon > 16384 or fmant >= 1:		# Infinity or NaN
			expon = sign|0x7FFF
			himant = 0
			lomant = 0
		else:					# Finite
			expon = expon + 16382
			if expon < 0:			# denormalized
				fmant = math.ldexp(fmant, expon)
				expon = 0
			expon = expon | sign
			fmant = math.ldexp(fmant, 32)
			fsmant = math.floor(fmant)
			himant = long(fsmant)
			fmant = math.ldexp(fmant - fsmant, 32)
			fsmant = math.floor(fmant)
			lomant = long(fsmant)
	_write_short(f, expon)
	_write_long(f, himant)
	_write_long(f, lomant)

class Chunk():
	def init(self, file):
		self.file = file
		self.chunkname = self.file.read(4)
		if len(self.chunkname) < 4:
			raise EOFError
		self.chunksize = _read_long(self.file)
		self.size_read = 0
		self.offset = self.file.tell()
		return self

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
		if self.chunksize & 1:
			dummy = self.read(1)

class Aifc_read():
	# Variables used in this class:
	#
	# These variables are available to the user though appropriate
	# methods of this class:
	# _file -- the open file with methods read(), close(), and seek()
	#		set through the init() ir initfp() method
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
	# _markers -- the marks in the audio file
	#		available through the getmarkers() and getmark()
	#		methods
	# _soundpos -- the position in the audio stream
	#		available through the tell() method, set through the
	#		tell() method
	#
	# These variables are used internally only:
	# _version -- the AIFF-C version number
	# _decomp -- the decompressor from builtin module cl
	# _comm_chunk_read -- 1 iff the COMM chunk has been read
	# _aifc -- 1 iff reading an AIFF-C file
	# _ssnd_seek_needed -- 1 iff positioned correctly in audio
	#		file for readframes()
	# _ssnd_chunk -- instantiation of a chunk class for the SSND chunk
	def initfp(self, file):
		self._file = file
		self._version = 0
		self._decomp = None
		self._markers = []
		self._soundpos = 0
		form = self._file.read(4)
		if form != 'FORM':
			raise Error, 'file does not start with FORM id'
		formlength = _read_long(self._file)
		if formlength <= 0:
			raise Error, 'invalid FORM chunk data size'
		formdata = self._file.read(4)
		formlength = formlength - 4
		if formdata == 'AIFF':
			self._aifc = 0
		elif formdata == 'AIFC':
			self._aifc = 1
		else:
			raise Error, 'not an AIFF or AIFF-C file'
		self._comm_chunk_read = 0
		while formlength > 0:
			self._ssnd_seek_needed = 1
			#DEBUG: SGI's soundfiler has a bug.  There should
			# be no need to check for EOF here.
			try:
				chunk = Chunk().init(self._file)
			except EOFError:
				if formlength == 8:
					print 'Warning: FORM chunk size too large'
					formlength = 0
					break
				raise EOFError # different error, raise exception
			formlength = formlength - 8 - chunk.chunksize
			if chunk.chunksize & 1:
				formlength = formlength - 1
			if chunk.chunkname == 'COMM':
				self._read_comm_chunk(chunk)
				self._comm_chunk_read = 1
			elif chunk.chunkname == 'SSND':
				self._ssnd_chunk = chunk
				dummy = chunk.read(8)
				self._ssnd_seek_needed = 0
			elif chunk.chunkname == 'FVER':
				self._version = _read_long(chunk)
			elif chunk.chunkname == 'MARK':
				self._readmark(chunk)
			elif chunk.chunkname in _skiplist:
				pass
			else:
				raise Error, 'unrecognized chunk type '+chunk.chunkname
			if formlength > 0:
				chunk.skip()
		if not self._comm_chunk_read or not self._ssnd_chunk:
			raise Error, 'COMM chunk and/or SSND chunk missing'
		if self._aifc and self._decomp:
			params = [CL.ORIGINAL_FORMAT, 0, \
				  CL.BITS_PER_COMPONENT, 0, \
				  CL.FRAME_RATE, self._framerate]
			if self._nchannels == AL.MONO:
				params[1] = CL.MONO
			else:
				params[1] = CL.STEREO_INTERLEAVED
			if self._sampwidth == AL.SAMPLE_8:
				params[3] = 8
			elif self._sampwidth == AL.SAMPLE_16:
				params[3] = 16
			else:
				params[3] = 24
			self._decomp.SetParams(params)
		return self

	def init(self, filename):
		return self.initfp(builtin.open(filename, 'r'))

	#
	# User visible methods.
	#
	def getfp(self):
		return self._file

	def rewind(self):
		self._ssnd_seek_needed = 1
		self._soundpos = 0

	def close(self):
		if self._decomp:
			self._decomp.CloseDecompressor()
			self._decomp = None
		self._file.close()
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

##	def getversion(self):
##		return self._version

	def getparams(self):
		return self._nchannels, self._sampwidth, self._framerate, \
			  self._nframes, self._comptype, self._compname

	def getmarkers(self):
		if len(self._markers) == 0:
			return None
		return self._markers

	def getmark(self, id):
		for marker in self._markers:
			if id == marker[0]:
				return marker
		raise Error, 'marker ' + `id` + ' does not exist'

	def setpos(self, pos):
		if pos < 0 or pos > self._nframes:
			raise Error, 'position not in range'
		self._soundpos = pos
		self._ssnd_seek_needed = 1

	def readframes(self, nframes):
		if self._ssnd_seek_needed:
			self._ssnd_chunk.rewind()
			dummy = self._ssnd_chunk.read(8)
			pos = self._soundpos * self._nchannels * self._sampwidth
			if self._decomp:
				if self._comptype in ('ULAW', 'ALAW'):
					pos = pos / 2
			if pos:
				self._ssnd_chunk.setpos(pos + 8)
			self._ssnd_seek_needed = 0
		if nframes == 0:
			return ''
		size = nframes * self._nchannels * self._sampwidth
		if self._decomp:
			if self._comptype in ('ULAW', 'ALAW'):
				size = size / 2
		data = self._ssnd_chunk.read(size)
		if self._decomp and data:
			params = [CL.FRAME_BUFFER_SIZE, len(data) * 2, \
				  CL.COMPRESSED_BUFFER_SIZE, len(data)]
			self._decomp.SetParams(params)
			data = self._decomp.Decompress(len(data) / self._nchannels, data)
		self._soundpos = self._soundpos + len(data) / (self._nchannels * self._sampwidth)
		return data

	#
	# Internal methods.
	#
	def _read_comm_chunk(self, chunk):
		nchannels = _read_short(chunk)
		self._nchannels = _convert1(nchannels, _nchannelslist)
		self._nframes = _read_long(chunk)
		sampwidth = _read_short(chunk)
		self._sampwidth = _convert1(sampwidth, _sampwidthlist)
		framerate = _read_float(chunk)
		self._framerate = _convert1(framerate, _frameratelist)
		if self._aifc:
			#DEBUG: SGI's soundeditor produces a bad size :-(
			kludge = 0
			if chunk.chunksize == 18:
				kludge = 1
				print 'Warning: bad COMM chunk size'
				chunk.chunksize = 23
			#DEBUG end
			self._comptype = chunk.read(4)
			#DEBUG start
			if kludge:
				length = ord(chunk.file.read(1))
				if length & 1 == 0:
					length = length + 1
				chunk.chunksize = chunk.chunksize + length
				chunk.file.seek(-1, 1)
			#DEBUG end
			self._compname = _read_string(chunk)
			if self._comptype != 'NONE':
				try:
					import cl, CL
				except ImportError:
					raise Error, 'cannot read compressed AIFF-C files'
				if self._comptype == 'ULAW':
					scheme = CL.G711_ULAW
				elif self._comptype == 'ALAW':
					scheme = CL.G711_ALAW
				else:
					raise Error, 'unsupported compression type'
				self._decomp = cl.OpenDecompressor(scheme)
		else:
			self._comptype = 'NONE'
			self._compname = 'not compressed'

	def _readmark(self, chunk):
		nmarkers = _read_short(chunk)
		for i in range(nmarkers):
			id = _read_short(chunk)
			pos = _read_long(chunk)
			name = _read_string(chunk)
			self._markers.append((id, pos, name))

class Aifc_write():
	# Variables used in this class:
	#
	# These variables are user settable through appropriate methods
	# of this class:
	# _file -- the open file with methods write(), close(), tell(), seek()
	#		set through the init() or initfp() method
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
	# _aifc -- whether we're writing an AIFF-C file or an AIFF file
	#		set through the aifc() method, reset through the
	#		aiff() method
	#
	# These variables are used internally only:
	# _version -- the AIFF-C version number
	# _comp -- the compressor from builtin module cl
	# _nframeswritten -- the number of audio frames actually written
	# _datalength -- the size of the audio samples written to the header
	# _datawritten -- the size of the audio samples actually written

	def init(self, filename):
		self = self.initfp(builtin.open(filename, 'w'))
		if filename[-5:] == '.aiff':
			self._aifc = 0
		else:
			self._aifc = 1
		return self

	def initfp(self, file):
		self._file = file
		self._version = _AIFC_version
		self._comptype = 'NONE'
		self._compname = 'not compressed'
		self._comp = None
		self._nchannels = 0
		self._sampwidth = 0
		self._framerate = 0
		self._nframes = 0
		self._nframeswritten = 0
		self._datawritten = 0
		self._markers = []
		self._marklength = 0
		self._aifc = 1		# AIFF-C is default
		return self

	#
	# User visible methods.
	#
	def aiff(self):
		if self._nframeswritten:
			raise Error, 'cannot change parameters after starting to write'
		self._aifc = 0

	def aifc(self):
		if self._nframeswritten:
			raise Error, 'cannot change parameters after starting to write'
		self._aifc = 1

	def setnchannels(self, nchannels):
		if self._nframeswritten:
			raise Error, 'cannot change parameters after starting to write'
		self._nchannels = nchannels

	def getnchannels(self):
		if not self._nchannels:
			raise Error, 'number of channels not set'
		return self._nchannels

	def setsampwidth(self, sampwidth):
		if self._nframeswritten:
			raise Error, 'cannot change parameters after starting to write'
		self._sampwidth = sampwidth

	def getsampwidth(self):
		if not self._sampwidth:
			raise Error, 'sample width not set'
		return self._sampwidth

	def setframerate(self, framerate):
		if self._nframeswritten:
			raise Error, 'cannot change parameters after starting to write'
		self._framerate = framerate

	def getframerate(self):
		if not self._framerate:
			raise Error, 'frame rate not set'
		return self._framerate

	def setnframes(self, nframes):
		if self._nframeswritten:
			raise Error, 'cannot change parameters after starting to write'
		self._nframes = nframes

	def getnframes(self):
		return self._nframeswritten

	def setcomptype(self, comptype, compname):
		if self._nframeswritten:
			raise Error, 'cannot change parameters after starting to write'
		if comptype not in ('NONE', 'ULAW', 'ALAW'):
			raise Error, 'unsupported compression type'
		self._comptype = comptype
		self._compname = compname

	def getcomptype(self):
		return self._comptype

	def getcompname(self):
		return self._compname

##	def setversion(self, version):
##		if self._nframeswritten:
##			raise Error, 'cannot change parameters after starting to write'
##		self._version = version

	def setparams(self, (nchannels, sampwidth, framerate, nframes, comptype, compname)):
		if self._nframeswritten:
			raise Error, 'cannot change parameters after starting to write'
		if comptype not in ('NONE', 'ULAW', 'ALAW'):
			raise Error, 'unsupported compression type'
		self._nchannels = nchannels
		self._sampwidth = sampwidth
		self._framerate = framerate
		self._nframes = nframes
		self._comptype = comptype
		self._compname = compname

	def getparams(self):
		if not self._nchannels or not self._sampwidth or not self._framerate:
			raise Error, 'not all parameters set'
		return self._nchannels, self._sampwidth, self._framerate, \
			  self._nframes, self._comptype, self._compname

	def setmark(self, id, pos, name):
		if id <= 0:
			raise Error, 'marker ID must be > 0'
		if pos < 0:
			raise Error, 'marker position must be >= 0'
		if type(name) != type(''):
			raise Error, 'marker name must be a string'
		for i in range(len(self._markers)):
			if id == self._markers[i][0]:
				self._markers[i] = id, pos, name
				return
		self._markers.append((id, pos, name))

	def getmark(self, id):
		for marker in self._markers:
			if id == marker[0]:
				return marker
		raise Error, 'marker ' + `id` + ' does not exist'

	def getmarkers(self):
		if len(self._markers) == 0:
			return None
		return self._markers
				
	def writeframesraw(self, data):
		if not self._nframeswritten:
			if self._comptype in ('ULAW', 'ALAW'):
				if not self._sampwidth:
					self._sampwidth = AL.SAMPLE_16
				if self._sampwidth != AL.SAMPLE_16:
					raise Error, 'sample width must be 2 when compressing with ULAW or ALAW'
			if not self._nchannels:
				raise Error, '# channels not specified'
			if not self._sampwidth:
				raise Error, 'sample width not specified'
			if not self._framerate:
				raise Error, 'sampling rate not specified'
			self._write_header(len(data))
		nframes = len(data) / (self._sampwidth * self._nchannels)
		if self._comp:
			params = [CL.FRAME_BUFFER_SIZE, len(data), \
				  CL.COMPRESSED_BUFFER_SIZE, len(data)]
			self._comp.SetParams(params)
			data = self._comp.Compress(nframes, data)
		self._file.write(data)
		self._nframeswritten = self._nframeswritten + nframes
		self._datawritten = self._datawritten + len(data)

	def writeframes(self, data):
		self.writeframesraw(data)
		if self._nframeswritten != self._nframes or \
			  self._datalength != self._datawritten:
			self._patchheader()

	def close(self):
		if self._datawritten & 1:
			# quick pad to even size
			self._file.write(chr(0))
			self._datawritten = self._datawritten + 1
		self._writemarkers()
		if self._nframeswritten != self._nframes or \
			  self._datalength != self._datawritten or \
			  self._marklength:
			self._patchheader()
		if self._comp:
			self._comp.CloseCompressor()
			self._comp = None
		self._file.close()
		self._file = None

	#
	# Internal methods.
	#
	def _write_header(self, initlength):
		if self._aifc and self._comptype != 'NONE':
			try:
				import cl, CL
			except ImportError:
				raise Error, 'cannot write compressed AIFF-C files'
			if self._comptype == 'ULAW':
				scheme = CL.G711_ULAW
			elif self._comptype == 'ALAW':
				scheme = CL.G711_ALAW
			else:
				raise Error, 'unsupported compression type'
			self._comp = cl.OpenCompressor(scheme)
			params = [CL.ORIGINAL_FORMAT, 0, \
				  CL.BITS_PER_COMPONENT, 0, \
				  CL.FRAME_RATE, self._framerate]
			if self._nchannels == AL.MONO:
				params[1] = CL.MONO
			else:
				params[1] = CL.STEREO_INTERLEAVED
			if self._sampwidth == AL.SAMPLE_8:
				params[3] = 8
			elif self._sampwidth == AL.SAMPLE_16:
				params[3] = 16
			else:
				params[3] = 24
			self._comp.SetParams(params)
		self._file.write('FORM')
		if not self._nframes:
			self._nframes = initlength / (self._nchannels * self._sampwidth)
		self._datalength = self._nframes * self._nchannels * self._sampwidth
		if self._datalength & 1:
			self._datalength = self._datalength + 1
		if self._aifc and self._comptype in ('ULAW', 'ALAW'):
			self._datalength = self._datalength / 2
			if self._datalength & 1:
				self._datalength = self._datalength + 1
		self._form_length_pos = self._file.tell()
		commlength = self._write_form_length(self._datalength)
		if self._aifc:
			self._file.write('AIFC')
			self._file.write('FVER')
			_write_long(self._file, 4)
			_write_long(self._file, self._version)
		else:
			self._file.write('AIFF')
		self._file.write('COMM')
		_write_long(self._file, commlength)
		_write_short(self._file, self._nchannels)
		self._nframes_pos = self._file.tell()
		_write_long(self._file, self._nframes)
		_write_short(self._file, _convert2(self._sampwidth, _sampwidthlist))
		_write_float(self._file, _convert2(self._framerate, _frameratelist))
		if self._aifc:
			self._file.write(self._comptype)
			_write_string(self._file, self._compname)
		self._file.write('SSND')
		self._ssnd_length_pos = self._file.tell()
		_write_long(self._file, self._datalength + 8)
		_write_long(self._file, 0)
		_write_long(self._file, 0)

	def _write_form_length(self, datalength):
		if self._aifc:
			commlength = 18 + 5 + len(self._compname)
			if commlength & 1:
				commlength = commlength + 1
			verslength = 12
		else:
			commlength = 18
			verslength = 0
		_write_long(self._file, 4 + verslength + self._marklength + \
					8 + commlength + 16 + datalength)
		return commlength

	def _patchheader(self):
		curpos = self._file.tell()
		if self._datawritten & 1:
			datalength = self._datawritten + 1
			self._file.write(chr(0))
		else:
			datalength = self._datawritten
		if datalength == self._datalength and \
			  self._nframes == self._nframeswritten and \
			  self._marklength == 0:
			self._file.seek(curpos, 0)
			return
		self._file.seek(self._form_length_pos, 0)
		dummy = self._write_form_length(datalength)
		self._file.seek(self._nframes_pos, 0)
		_write_long(self._file, self._nframeswritten)
		self._file.seek(self._ssnd_length_pos, 0)
		_write_long(self._file, datalength + 8)
		self._file.seek(curpos, 0)
		self._nframes = self._nframeswritten
		self._datalength = datalength

	def _writemarkers(self):
		if len(self._markers) == 0:
			return
		self._file.write('MARK')
		length = 2
		for marker in self._markers:
			id, pos, name = marker
			length = length + len(name) + 1 + 6
			if len(name) & 1 == 0:
				length = length + 1
		_write_long(self._file, length)
		self._marklength = length + 8
		_write_short(self._file, len(self._markers))
		for marker in self._markers:
			id, pos, name = marker
			_write_short(self._file, id)
			_write_long(self._file, pos)
			_write_string(self._file, name)

def open(filename, mode):
	if mode == 'r':
		return Aifc_read().init(filename)
	elif mode == 'w':
		return Aifc_write().init(filename)
	else:
		raise Error, 'mode must be \'r\' or \'w\''

def openfp(filep, mode):
	if mode == 'r':
		return Aifc_read().initfp(filep)
	elif mode == 'w':
		return Aifc_write().initfp(filep)
	else:
		raise Error, 'mode must be \'r\' or \'w\''
