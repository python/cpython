# Classes to read and write CMIF video files.
# (For a description of the CMIF video format, see cmif-file.ms.)


# Layers of functionality:
#
# VideoParams: maintain essential parameters of a video file
# Displayer: display a frame in a window (with some extra parameters)
# Grabber: grab a frame from a window
# BasicVinFile: read a CMIF video file
# BasicVoutFile: write a CMIF video file
# VinFile: BasicVinFile + Displayer
# VoutFile: BasicVoutFile + Displayer + Grabber


# Imported modules

import sys
import gl
import GL
import colorsys


# Exception raised for various occasions

Error = 'VFile.Error'			# file format errors
CallError = 'VFile.CallError'		# bad call


# Constants returned by gl.getdisplaymode(), from <gl/get.h>

DMRGB = 0
DMSINGLE = 1
DMDOUBLE = 2
DMRGBDOUBLE = 5


# Max nr. of colormap entries to use

MAXMAP = 4096 - 256


# Parametrizations of colormap handling based on color system.
# (These functions are used via eval with a constructed argument!)

def conv_grey(l, x, y):
	return colorsys.yiq_to_rgb(l, 0, 0)

def conv_yiq(y, i, q):
	return colorsys.yiq_to_rgb(y, (i-0.5)*1.2, q-0.5)

def conv_hls(l, h, s):
	return colorsys.hls_to_rgb(h, l, s)

def conv_hsv(v, h, s):
	return colorsys.hsv_to_rgb(h, s, v)

def conv_rgb(r, g, b):
	raise Error, 'Attempt to make RGB colormap'

def conv_rgb8(rgb, d1, d2):
	rgb = int(rgb*255.0)
	r = (rgb >> 5) & 0x07
	g = (rgb     ) & 0x07
	b = (rgb >> 3) & 0x03
	return (r/7.0, g/7.0, b/3.0)


# Choose one of the above based upon a color system name

def choose_conversion(format):
	try:
		return eval('conv_' + format)
	except:
		raise Error, 'Unknown color system: ' + `format`


# Routines to grab data, per color system (only a few really supported).
# (These functions are used via eval with a constructed argument!)

def grab_rgb(w, h, pf):
	if gl.getdisplaymode() <> DMRGB:
		raise Error, 'Sorry, can only grab rgb in single-buf rgbmode'
	if pf <> 1 and pf <> 0:
		raise Error, 'Sorry, only grab rgb with packfactor 1'
	return gl.lrectread(0, 0, w-1, h-1), None

def grab_rgb8(w, h, pf):
	if gl.getdisplaymode() <> DMRGB:
		raise Error, 'Sorry, can only grab rgb8 in single-buf rgbmode'
	if pf <> 1 and pf <> 0:
		raise Error, 'Sorry, can only grab rgb8 with packfactor 1'
	r = gl.getgdesc(GL.GD_BITS_NORM_SNG_RED)
	g = gl.getgdesc(GL.GD_BITS_NORM_SNG_GREEN)
	b = gl.getgdesc(GL.GD_BITS_NORM_SNG_BLUE)
	if (r, g, b) <> (3, 3, 2):
		raise Error, 'Sorry, can only grab rgb8 on 8-bit Indigo'
	# XXX Dirty Dirty here.
	# XXX Set buffer to cmap mode, grab image and set it back.
	# XXX (Shouldn't be necessary???)
	gl.cmode()
	gl.gconfig()
	gl.pixmode(GL.PM_SIZE, 8)
	data = gl.lrectread(0, 0, w-1, h-1)
	data = data[:w*h]	# BUG FIX for python lrectread
	gl.RGBmode()
	gl.gconfig()
	gl.pixmode(GL.PM_SIZE, 32)
	return data, None

def grab_grey(w, h, pf):
	raise Error, 'Sorry, grabbing grey not implemented'

def grab_yiq(w, h, pf):
	raise Error, 'Sorry, grabbing yiq not implemented'

def grab_hls(w, h, pf):
	raise Error, 'Sorry, grabbing hls not implemented'

def grab_hsv(w, h, pf):
	raise Error, 'Sorry, grabbing hsv not implemented'


# Choose one of the above based upon a color system name

def choose_grabber(format):
	try:
		return eval('grab_' + format)
	except:
		raise Error, 'Unknown color system: ' + `format`


# Base class to manage video format parameters

class VideoParams:

	# Initialize an instance.
	# Set all parameters to something decent
	# (except width and height are set to zero)

	def init(self):
		# Essential parameters
		self.format = 'grey'	# color system used
		# Choose from: 'rgb', 'rgb8', 'hsv', 'yiq', 'hls'
		self.width = 0		# width of frame
		self.height = 0		# height of frame
		self.packfactor = 1	# expansion using rectzoom
		# if packfactor == 0, data is one 32-bit word/pixel;
		# otherwise, data is one byte/pixel
		self.c0bits = 8		# bits in first color dimension
		self.c1bits = 0		# bits in second color dimension
		self.c2bits = 0		# bits in third color dimension
		self.offset = 0		# colormap index offset (XXX ???)
		self.chrompack = 0	# set if separate chrominance data
		return self

	# Set the frame width and height (e.g. from gl.getsize())

	def setsize(self, size):
		self.width, self.height = size

	# Retrieve the frame width and height (e.g. for gl.prefsize())

	def getsize(self):
		return (self.width, self.height)

	# Set all parameters.
	# This does limited validity checking;
	# if the check fails no parameters are changed

	def setinfo(self, values):
		(self.format, self.width, self.height, self.packfactor,\
			self.c0bits, self.c1bits, self.c2bits, self.offset, \
			self.chrompack) = values

	# Retrieve all parameters in a format suitable for a subsequent
	# call to setinfo()

	def getinfo(self):
		return (self.format, self.width, self.height, self.packfactor,\
			self.c0bits, self.c1bits, self.c2bits, self.offset, \
			self.chrompack)

	# Write the relevant bits to stdout

	def printinfo(self):
		print 'Format:  ', self.format
		print 'Size:    ', self.width, 'x', self.height
		print 'Pack:    ', self.packfactor, '; chrom:', self.chrompack
		print 'Bits:    ', self.c0bits, self.c1bits, self.c2bits
		print 'Offset:  ', self.offset


# Class to display video frames in a window.
# It is the caller's responsibility to ensure that the correct window
# is current when using showframe(), initcolormap() and clear()

class Displayer(VideoParams):

	# Initialize an instance.
	# This does not need a current window

	def init(self):
		self = VideoParams.init(self)
		# User-settable parameters
		self.magnify = 1.0	# frame magnification factor
		self.xorigin = 0	# x frame offset
		self.yorigin = 0	# y frame offset (from bottom)
		self.quiet = 0		# if set, don't print messages
		self.fallback = 1	# allow fallback to grey
		# Internal flags
		self.colormapinited = 0	# must initialize window
		self.skipchrom = 0	# don't skip chrominance data
		return self

	# setinfo() must reset some internal flags

	def setinfo(self, values):
		VideoParams.setinfo(values)
		self.colormapinited = 0
		self.skipchrom = 0

	# Show one frame, initializing the window if necessary

	def showframe(self, data, chromdata):
		if not self.colormapinited:
			self.initcolormap()
		w, h, pf = self.width, self.height, self.packfactor
		factor = self.magnify
		if pf: factor = factor * pf
		if chromdata and not self.skipchrom:
			cp = self.chrompack
			cw = (w+cp-1)/cp
			ch = (h+cp-1)/cp
			gl.rectzoom(factor*cp, factor*cp)
			gl.pixmode(GL.PM_SIZE, 16)
			gl.writemask(self.mask - ((1 << self.c0bits) - 1))
			gl.lrectwrite(self.xorigin, self.yorigin, \
				self.xorigin + cw - 1, self.yorigin + ch - 1, \
				chromdata)
		#
		if pf:
			gl.writemask((1 << self.c0bits) - 1)
			gl.pixmode(GL.PM_SIZE, 8)
			w = w/pf
			h = h/pf
		gl.rectzoom(factor, factor)
		gl.lrectwrite(self.xorigin, self.yorigin, \
			self.xorigin + w - 1, self.yorigin + h - 1, data)

	# Initialize the window: set RGB or colormap mode as required,
	# fill in the colormap, and clear the window

	def initcolormap(self):
		if self.format == 'rgb':
			gl.RGBmode()
			gl.gconfig()
			self.colormapinited = 1
			gl.RGBcolor(200, 200, 200) # XXX rather light grey
			gl.clear()
			return
		gl.cmode()
		gl.gconfig()
		self.skipchrom = 0
		if self.offset == 0:
			self.mask = 0x7ff
		else:
			self.mask = 0xfff
		if not self.quiet:
			sys.stderr.write('Initializing color map...')
		self._initcmap()
		self.colormapinited = 1
		self.clear()
		if not self.quiet:
			sys.stderr.write(' Done.\n')

	# Clear the window

	def clear(self):
		if not self.colormapinited: raise CallError
		if self.offset == 0:
			gl.color(0x800)
			gl.clear()
		else:
			gl.clear()

	# Do the hard work for initializing the colormap

	def _initcmap(self):
		convcolor = choose_conversion(self.format)
		maxbits = gl.getgdesc(GL.GD_BITS_NORM_SNG_CMODE)
		if maxbits > 11:
			maxbits = 11
		c0bits, c1bits, c2bits = self.c0bits, self.c1bits, self.c2bits
		if c0bits+c1bits+c2bits > maxbits:
			if self.fallback and c0bits < maxbits:
				# Cannot display frames in this mode, use grey
				self.skipchrom = 1
				c1bits = c2bits = 0
				convcolor = choose_conversion('grey')
			else:
				raise Error, 'Sorry, '+`maxbits`+ \
				  ' bits max on this machine'
		maxc0 = 1 << c0bits
		maxc1 = 1 << c1bits
		maxc2 = 1 << c2bits
		if self.offset == 0 and maxbits == 11:
			offset = 2048
		else:
			offset = self.offset
		if maxbits <> 11:
			offset = offset & ((1<<maxbits)-1)
		# XXX why is this here?
		# for i in range(512, MAXMAP):
		#	gl.mapcolor(i, 0, 0, 0)
		# gl.gflush()
		for c0 in range(maxc0):
			c0v = c0/float(maxc0-1)
			for c1 in range(maxc1):
				if maxc1 == 1:
					c1v = 0
				else:
					c1v = c1/float(maxc1-1)
				for c2 in range(maxc2):
					if maxc2 == 1:
						c2v = 0
					else:
						c2v = c2/float(maxc2-1)
					index = offset + c0 + (c1<<c0bits) + \
						(c2 << (c0bits+c1bits))
					if index < MAXMAP:
						rv, gv, bv = \
						  convcolor(c0v, c1v, c2v)
						r, g, b = int(rv*255.0), \
							  int(gv*255.0), \
							  int(bv*255.0)
						gl.mapcolor(index, r, g, b)
		gl.gflush() # send the colormap changes to the X server


# Class to grab frames from a window.
# (This has fewer user-settable parameters than Displayer.)
# It is the caller's responsibility to initialize the window and to
# ensure that it is current when using grabframe()

class Grabber(VideoParams):

	# XXX The init() method of VideoParams is just fine, for now

	# Grab a frame.
	# Return (data, chromdata) just like getnextframe().

	def grabframe(self):
		grabber = choose_grabber(self.format)
		return grabber(self.width, self.height, self.packfactor)


# Read a CMIF video file header.
# Return (version, values) where version is 0.0, 1.0, 2.0 or 3.0,
# and values is ready for setinfo().
# Raise Error if there is an error in the info

def readfileheader(fp, filename):
	#
	# Get identifying header
	#
	line = fp.readline(20)
	if   line == 'CMIF video 0.0\n':
		version = 0.0
	elif line == 'CMIF video 1.0\n':
		version = 1.0
	elif line == 'CMIF video 2.0\n':
		version = 2.0
	elif line == 'CMIF video 3.0\n':
		version = 3.0
	else:
		# XXX Could be version 0.0 without identifying header
		raise Error, \
			filename + ': Unrecognized file header: ' + `line`[:20]
	#
	# Get color encoding info
	#
	if version <= 1.0:
		format = 'grey'
		c0bits, c1bits, c2bits = 8, 0, 0
		chrompack = 0
		offset = 0
	elif version == 2.0:
		line = fp.readline()
		try:
			c0bits, c1bits, c2bits, chrompack = eval(line[:-1])
		except:
			raise Error, filename + ': Bad 2.0 color info'
		if c1bits or c2bits:
			format = 'yiq'
		else:
			format = 'grey'
		offset = 0
	elif version == 3.0:
		line = fp.readline()
		try:
			format, rest = eval(line[:-1])
		except:
			raise Error, filename + ': Bad 3.0 color info'
		if format == 'rgb':
			c0bits = c1bits = c2bits = 0
			chrompack = 0
			offset = 0
		elif format == 'grey':
			c0bits = rest
			c1bits = c2bits = 0
			chrompack = 0
			offset = 0
		else:
			try:
			    c0bits, c1bits, c2bits, chrompack, offset = rest
			except:
			    raise Error, filename + ': Bad 3.0 color info'
	#
	# Get frame geometry info
	#
	line = fp.readline()
	try:
		x = eval(line[:-1])
	except:
		raise Error, filename + ': Bad (w,h,pf) info'
	if len(x) == 3:
		width, height, packfactor = x
		if packfactor == 0 and version < 3.0:
			format = 'rgb'
	elif len(x) == 2 and version <= 1.0:
		width, height = x
		packfactor = 2
	else:
		raise Error, filename + ': Bad (w,h,pf) info'
	#
	# Return (version, values)
	#
	values = (format, width, height, packfactor, \
		  c0bits, c1bits, c2bits, offset, chrompack)
	return (version, values)


# Read a *frame* header -- separate functions per version.
# Return (timecode, datasize, chromdatasize).
# Raise EOFError if end of data is reached.
# Raise Error if data is bad.

def readv0frameheader(fp):
	line = fp.readline()
	if not line or line == '\n': raise EOFError
	try:
		t = eval(line[:-1])
	except:
		raise Error, 'Bad 0.0 frame header'
	return (t, 0, 0)

def readv1frameheader(fp):
	line = fp.readline()
	if not line or line == '\n': raise EOFError
	try:
		t, datasize = eval(line[:-1])
	except:
		raise Error, 'Bad 1.0 frame header'
	return (t, datasize, 0)

def readv2frameheader(fp):
	line = fp.readline()
	if not line or line == '\n': raise EOFError
	try:
		t, datasize = eval(line[:-1])
	except:
		raise Error, 'Bad 2.0 frame header'
	return (t, datasize, 0)

def readv3frameheader(fp):
	line = fp.readline()
	if not line or line == '\n': raise EOFError
	try:
		t, datasize, chromdatasize = x = eval(line[:-1])
	except:
		raise Error, 'Bad 3.0 frame header'
	return x


# Write a CMIF video file header (always version 3.0)

def writefileheader(fp, values):
	(format, width, height, packfactor, \
		c0bits, c1bits, c2bits, offset, chrompack) = values
	#
	# Write identifying header
	#
	fp.write('CMIF video 3.0\n')
	#
	# Write color encoding info
	#
	if format == 'rgb':
		data = ('rgb', 0)
	elif format == 'grey':
		data = ('grey', c0bits)
	else:
		data = (format, (c0bits, c1bits, c2bits, chrompack, offset))
	fp.write(`data`+'\n')
	#
	# Write frame geometry info
	#
	if format == 'rgb':
		packfactor = 0
	elif packfactor == 0:
		packfactor = 1
	data = (width, height, packfactor)
	fp.write(`data`+'\n')


# Basic class for reading CMIF video files

class BasicVinFile(VideoParams):

	def init(self, filename):
		if filename == '-':
			fp = sys.stdin
		else:
			fp = open(filename, 'r')
		return self.initfp(fp, filename)

	def initfp(self, fp, filename):
		self = VideoParams.init(self)
		self.fp = fp
		self.filename = filename
		self.version, values = readfileheader(fp, filename)
		VideoParams.setinfo(self, values)
		if self.version == 0.0:
			w, h, pf = self.width, self.height, self.packfactor
			if pf == 0:
				self._datasize = w*h*4
			else:
				self._datasize = (w/pf) * (h/pf)
			self._readframeheader = self._readv0frameheader
		elif self.version == 1.0:
			self._readframeheader = readv1frameheader
		elif self.version == 2.0:
			self._readframeheader = readv2frameheader
		elif self.version == 3.0:
			self._readframeheader = readv3frameheader
		else:
			raise Error, \
				filename + ': Bad version: ' + `self.version`
		self.framecount = 0
		self.atframeheader = 1
		try:
			self.startpos = self.fp.tell()
			self.canseek = 1
		except IOError:
			self.startpos = -1
			self.canseek = 0
		return self

	def _readv0frameheader(self, fp):
		t, ds, cs = readv0frameheader(fp)
		ds = self._datasize
		return (t, ds, cs)

	def close(self):
		self.fp.close()
		del self.fp
		del self._readframeheader

	def setinfo(self, values):
		raise CallError # Can't change info of input file!

	def setsize(self, size):
		raise CallError # Can't change info of input file!

	def rewind(self):
		if not self.canseek:
			raise Error, self.filename + ': can\'t seek'
		self.fp.seek(self.startpos)
		self.framecount = 0
		self.atframeheader = 1

	def warmcache(self):
		pass

	def printinfo(self):
		print 'File:    ', self.filename
		print 'Version: ', self.version
		VideoParams.printinfo(self)

	def getnextframe(self):
		t, ds, cs = self.getnextframeheader()
		data, cdata = self.getnextframedata(ds, cs)
		return (t, data, cdata)

	def skipnextframe(self):
		t, ds, cs = self.getnextframeheader()
		self.skipnextframedata(ds, cs)
		return t

	def getnextframeheader(self):
		if not self.atframeheader: raise CallError
		self.atframeheader = 0
		try:
			return self._readframeheader(self.fp)
		except Error, msg:
			# Patch up the error message
			raise Error, self.filename + ': ' + msg

	def getnextframedata(self, ds, cs):
		if self.atframeheader: raise CallError
		if ds:
			data = self.fp.read(ds)
			if len(data) < ds: raise EOFError
		else:
			data = ''
		if cs:
			cdata = self.fp.read(cs)
			if len(cdata) < cs: raise EOFerror
		else:
			cdata = ''
		self.atframeheader = 1
		self.framecount = self.framecount + 1
		return (data, cdata)

	def skipnextframedata(self, ds, cs):
		if self.atframeheader: raise CallError
		# Note that this won't raise EOFError for a partial frame
		# since there is no easy way to tell whether a seek
		# ended up beyond the end of the file
		if self.canseek:
			self.fp.seek(ds + cs, 1) # Relative seek
		else:
			dummy = self.fp.read(ds + cs)
			del dummy
		self.atframeheader = 1
		self.framecount = self.framecount + 1


class BasicVoutFile(VideoParams):

	def init(self, filename):
		if filename == '-':
			fp = sys.stdout
		else:
			fp = open(filename, 'w')
		return self.initfp(fp, filename)

	def initfp(self, fp, filename):
		self = VideoParams.init(self)
		self.fp = fp
		self.filename = filename
		self.version = 3.0 # In case anyone inquires
		self.headerwritten = 0
		return self

	def flush(self):
		self.fp.flush()

	def close(self):
		self.fp.close()
		del self.fp

	def setinfo(self, values):
		if self.headerwritten: raise CallError
		VideoParams.setinfo(self, values)

	def writeheader(self):
		if self.headerwritten: raise CallError
		writefileheader(self.fp, self.getinfo())
		self.headerwritten = 1
		self.atheader = 1
		self.framecount = 0

	def rewind(self):
		self.fp.seek(0)
		self.headerwritten = 0
		self.atheader = 1
		self.framecount = 0

	def printinfo(self):
		print 'File:    ', self.filename
		VideoParams.printinfo(self)

	def writeframe(self, t, data, cdata):
		if data: ds = len(data)
		else: ds = 0
		if cdata: cs = len(cdata)
		else: cs = 0
		self.writeframeheader(t, ds, cs)
		self.writeframedata(data, cdata)

	def writeframeheader(self, t, ds, cs):
		if not self.headerwritten: self.writeheader()
		if not self.atheader: raise CallError
		self.fp.write(`(t, ds, cs)` + '\n')
		self.atheader = 0

	def writeframedata(self, data, cdata):
		if not self.headerwritten or self.atheader: raise CallError
		if data: self.fp.write(data)
		if cdata: self.fp.write(cdata)
		self.atheader = 1
		self.framecount = self.framecount + 1


# Classes that combine files with displayers and/or grabbers:

class VinFile(BasicVinFile, Displayer):

	def initfp(self, fp, filename):
		self = Displayer.init(self)
		return BasicVinFile.initfp(self, fp, filename)

	def shownextframe(self):
		t, data, cdata = self.getnextframe()
		self.showframe(data, cdata)
		return t


class VoutFile(BasicVoutFile, Displayer, Grabber):

	def initfp(self, fp, filename):
		self = Displayer.init(self)
##		self = Grabber.init(self) # XXX not needed
		return BasicVoutFile.initfp(self, fp, filename)


# Simple test program (VinFile only)

def test():
	import time
	if sys.argv[1:]: filename = sys.argv[1]
	else: filename = 'film.video'
	vin = VinFile().init(filename)
	vin.printinfo()
	gl.foreground()
	gl.prefsize(vin.getsize())
	wid = gl.winopen(filename)
	vin.initcolormap()
	t0 = time.millitimer()
	while 1:
		try: t = vin.shownextframe()
		except EOFError: break
		dt = t0 + t - time.millitimer()
		if dt > 0: time.millisleep(dt)
	time.sleep(2)
