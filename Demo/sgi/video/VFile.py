# VFile -- two classes for Video Files.
# VinFile -- for video input.
# VoutFile -- for video output.

import sys
import gl
import GL
import colorsys

Error = 'VFile.Error' # Exception

# Missing from GL.py:
DMRGB = 0

MAXMAP = 4096 - 256

def conv_grey(l,x,y): return colorsys.yiq_to_rgb(l,0,0)
def conv_yiq (y,i,q): return colorsys.yiq_to_rgb(y, (i-0.5)*1.2, q-0.5)
def conv_hls (l,h,s): return colorsys.hls_to_rgb(h,l,s)
def conv_hsv (v,h,s): return colorsys.hsv_to_rgb(h,s,v)
def conv_rgb (r,g,b):
	raise Error, 'Attempt to make RGB colormap'
def conv_rgb8(rgb,d1,d2):
	rgb = int(rgb*255.0)
	r = (rgb >> 5) & 0x07
	g = (rgb     ) & 0x07
	b = (rgb >> 3) & 0x03
	return (r/7.0, g/7.0, b/3.0)

# Class VinFile represents a video file used for input.
#
# It has the following methods:
# init(filename)
# initfp(fp, filename)
# reopen()
# rewind()
# getnextframe()
# skipnextframe()
# (and many more)
#
# The following read-only data members provide public information:
# version
# filename
# width, height
# packfactor
# c0bits, c1bits, c2bits, chrompack
# offset
# format
#
# These writable data members provide additional parametrization:
# magnify
# xorigin, yorigin
# fallback



# XXX it's a total mess now -- VFile is a new base class
# XXX to support common functionality (e.g. showframe)

class VFile:

	#
	# getinfo returns all info pertaining to a film. The returned tuple
	# can be passed to VoutFile.setinfo()
	#
	def getinfo(self):
		return (self.format, self.width, self.height, self.packfactor,\
			  self.c0bits, self.c1bits, self.c2bits, self.offset, \
			  self.chrompack)

	# reopen() raises Error if the header is bad (which can only
	# happen if the file was written to since opened).

	def reopen(self):
		self.fp.seek(0)
		x = self.initfp(self.fp, self.filename)

	def setconvcolor(self):
		try:
			self.convcolor = eval('conv_'+self.format)
		except:
			raise Error, \
			  self.filename + ': unknown colorsys ' + self.format

	def showframe(self, data, chromdata):
		w, h, pf = self.width, self.height, self.packfactor
		if not self.colormapinited:
			self.initcolormap()
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
			gl.rectzoom(factor, factor)
			w = w/pf
			h = h/pf
		gl.lrectwrite(self.xorigin, self.yorigin, \
			self.xorigin + w - 1, self.yorigin + h - 1, data)

	def initcolormap(self):
		self.colormapinited = 1
		if self.format == 'rgb':
			gl.RGBmode()
			gl.gconfig()
			gl.RGBcolor(200, 200, 200)
			gl.clear()
			return
		gl.cmode()
		gl.gconfig()
		self.skipchrom = 0
		if not self.quiet:
			sys.stderr.write('Initializing color map...')
		self.initcmap()
		self.clear()
		if not self.quiet:
			sys.stderr.write(' Done.\n')

	def clear(self):
		if self.offset == 0:
			gl.color(0x800)
			gl.clear()
			self.mask = 0x7ff
		else:
			self.mask = 0xfff
			gl.clear()

	def initcmap(self):
		maxbits = gl.getgdesc(GL.GD_BITS_NORM_SNG_CMODE)
		if maxbits > 11:
			maxbits = 11
		c0bits, c1bits, c2bits = self.c0bits, self.c1bits, self.c2bits
		if c0bits+c1bits+c2bits > maxbits:
			if self.fallback and c0bits < maxbits:
				# Cannot display film in this mode, use mono
				self.skipchrom = 1
				c1bits = c2bits = 0
				self.convcolor = conv_grey
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
		#for i in range(512, MAXMAP):
		#	gl.mapcolor(i, 0, 0, 0)
		#void = gl.qtest()    # Should be gl.gflush()
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
					rv, gv, bv = self.convcolor( \
						  c0v, c1v, c2v)
					r, g, b = int(rv*255.0), \
						  int(gv*255.0), int(bv*255.0)
					if index < MAXMAP:
						gl.mapcolor(index, r, g, b)
		void = gl.gflush()

	

class VinFile(VFile):

	# init() and initfp() raise Error if the header is bad.
	# init() raises whatever open() raises if the file can't be opened.

	def init(self, filename):
		if filename == '-':
			return self.initfp(sys.stdin, filename)
		return self.initfp(open(filename, 'r'), filename)

	def initfp(self, fp, filename):
		self.colormapinited = 0
		self.magnify = 1.0
		self.xorigin = self.yorigin = 0
		self.fallback = 1
		self.skipchrom = 0
		self.fp = fp
		self.filename = filename
		self.quiet = 0
		#
		line = self.fp.readline()
		if line == 'CMIF video 1.0\n':
			self.version = 1.0
		elif line == 'CMIF video 2.0\n':
			self.version = 2.0
		elif line == 'CMIF video 3.0\n':
			self.version = 3.0
		else:
			raise Error, self.filename + ': bad video format'
		#
		if self.version < 2.0:
			self.c0bits, self.c1bits, self.c2bits = 8, 0, 0
			self.chrompack = 0
			self.offset = 0
			self.format = 'grey'
		elif self.version == 2.0:
			line = self.fp.readline()
			try:
				self.c0bits, self.c1bits, self.c2bits, \
					self.chrompack = eval(line[:-1])
				if self.c1bits or self.c2bits:
					self.format = 'yiq'
				else:
					self.format = 'grey'
				self.offset = 0
			except:
				raise Error, \
				  self.filename + ': bad 2.0 color info'
		elif self.version == 3.0:
			line = self.fp.readline()
			try:
				self.format, rest = eval(line[:-1])
				if self.format == 'rgb':
					self.offset = 0
					self.c0bits = 0
					self.c1bits = 0
					self.c2bits = 0
					self.chrompack = 0
				elif self.format == 'grey':
					self.offset = 0
					self.c0bits = rest
					self.c1bits = self.c2bits = \
						self.chrompack = 0
				else:
					self.c0bits,self.c1bits,self.c2bits,\
					  self.chrompack,self.offset = rest
			except:
				raise Error, \
					self.filename + ': bad 3.0 color info'

		self.setconvcolor()
		#
		line = self.fp.readline()
		try:
			x = eval(line[:-1])
			if self.version > 1.0 or len(x) == 3:
				self.width, self.height, self.packfactor = x
				if self.packfactor == 0:
					self.format = 'rgb'
			else:
				sef.width, self.height = x
				self.packfactor = 2
		except:
			raise Error, self.filename + ': bad (w,h,pf) info'
		self.frameno = 0
		self.framecache = []
		self.hascache = 0
		#
		return self

	def warmcache(self):
		if self.hascache: return
		n = 0
		try:
			while 1:
				void = self.skipnextframe()
				n = n + 1
		except EOFError:
			pass
		if not self.hascache:
			raise Error, 'Cannot warm cache'

	def close(self):
		self.fp.close()
		self.fp = None

	def rewind(self):
		if self.hascache:
			self.frameno = 0
		else:
			self.reopen()

	def position(self):
		if self.frameno >= len(self.framecache):
			raise EOFError
		self.fp.seek(self.framecache[self.frameno][0])

	# getnextframe() raises EOFError (built-in) if there is no next frame,
	# or if the next frame is broken.
	# So to getnextframeheader(), getnextframedata() and skipnextframe().

	def getnextframe(self):
		time, size, chromsize = self.getnextframeheader()
		data, chromdata = self.getnextframedata(size, chromsize)
		return time, data, chromdata

	def getnextframedata(self, size, chromsize):
		if self.hascache:
			self.position()
		self.frameno = self.frameno + 1
		data = self.fp.read(size)
		if len(data) <> size: raise EOFError
		if chromsize:
			chromdata = self.fp.read(chromsize)
			if len(chromdata) <> chromsize: raise EOFError
		else:
			chromdata = None
		#
		return data, chromdata

	def skipnextframe(self):
		time, size, chromsize = self.getnextframeheader()
		self.skipnextframedata(size, chromsize)
		return time

	def skipnextframedata(self, size, chromsize):
		if self.hascache:
			self.frameno = self.frameno + 1
			return
		# Note that this won't raise EOFError for a partial frame.
		try:
			self.fp.seek(size + chromsize, 1) # Relative seek
		except:
			# Assume it's a pipe -- read the data to discard it
			dummy = self.fp.read(size + chromsize)

	def getnextframeheader(self):
		if self.hascache:
			if self.frameno >= len(self.framecache):
				raise EOFError
			return self.framecache[self.frameno][1]
		line = self.fp.readline()
		if not line:
			self.hascache = 1
			raise EOFError
		#
		w, h, pf = self.width, self.height, self.packfactor
		try:
			x = eval(line[:-1])
			if type(x) in (type(0), type(0.0)):
				time = x
				if pf == 0:
					size = w * h * 4
				else:
					size = (w/pf) * (h/pf)
			elif len(x) == 2:
				time, size = x
				cp = self.chrompack
				if cp:
					cw = (w + cp - 1) / cp
					ch = (h + cp - 1) / cp
					chromsize = 2 * cw * ch
				else:
					chromsize = 0
			else:
				time, size, chromsize = x
		except:
			raise Error, self.filename + ': bad frame header'
		cdata = (self.fp.tell(), (time, size, chromsize))
		self.framecache.append(cdata)
		return time, size, chromsize

	def shownextframe(self):
		time, data, chromdata = self.getnextframe()
		self.showframe(data, chromdata)
		return time

#
# A set of routines to grab images from windows
#
def grab_rgb(w, h, pf):
	if gl.getdisplaymode() <> DMRGB:
		raise Error, 'Sorry, can only grab rgb in single-buf rgbmode'
	if pf <> 1 and pf <> 0:
		raise Error, 'Sorry, only grab with packfactor=1'
	return gl.lrectread(0, 0, w-1, h-1), None
	
def grab_rgb8(w, h, pf):
	if gl.getdisplaymode() <> DMRGB:
		raise Error, 'Sorry, can only grab rgb in single-buf rgbmode'
	if pf <> 1 and pf <> 0:
		raise Error, 'Sorry, can only grab with packfactor=1'
	r = gl.getgdesc(GL.GD_BITS_NORM_SNG_RED)
	g = gl.getgdesc(GL.GD_BITS_NORM_SNG_GREEN)
	b = gl.getgdesc(GL.GD_BITS_NORM_SNG_BLUE)
	if (r,g,b) <> (3,3,2):
		raise Error, 'Sorry, can only grab rgb8 on 8-bit Indigo'
	# Dirty Dirty here. Set buffer to cmap mode, grab image and set it back
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

#
# The class VoutFile is not as well-designed (and tested) as VinFile.
# Notably it will accept almost any garbage and write it to the video
# output file
#
class VoutFile(VFile):
	def init(self, filename):
		if filename == '-':
			return self.initfp(sys.stdout, filename)
		else:
			return self.initfp(open(filename,'w'), filename)
			
	def initfp(self, fp, filename):
		self.fp = fp
		self.format = 'grey'
		self.width = self.height = 0
		self.packfactor = 1
		self.c0bits = 8
		self.c1bits = self.c2bits = 0
		self.offset = 0
		self.chrompack = 0
		self.headerwritten = 0
		self.quiet = 0
		self.magnify = 1
		self.setconvcolor()
		self.xorigin = self.yorigin = 0
		return self

	def close(self):
		self.fp.close()
		x = self.initfp(None, None)

	def setinfo(self, values):
		self.format, self.width, self.height, self.packfactor,\
			  self.c0bits, self.c1bits, self.c2bits, self.offset, \
			  self.chrompack = values
		self.setconvcolor()

	def writeheader(self):
		self.headerwritten = 1
		if self.format == 'rgb':
			self.packfactor = 0
		elif self.packfactor == 0:
			self.packfactor = 1
		self.fp.write('CMIF video 3.0\n')
		if self.format == 'rgb':
			data = ('rgb', 0)
		elif self.format == 'grey':
			data = ('grey', self.c0bits)
		else:
			data = (self.format, (self.c0bits, self.c1bits, \
				  self.c2bits, self.chrompack, self.offset))
		self.fp.write(`data`+'\n')
		data = (self.width, self.height, self.packfactor)
		self.fp.write(`data`+'\n')
		try:
			self._grabber = eval('grab_' + self.format)
		except:
			raise Error, 'unknown colorsys: ' + self.format

	def writeframeheader(self, data):
		if not self.headerwritten:
			raise Error, 'Writing frame data before header'
		# XXXX Should we sanity check here?
		self.fp.write(`data`+'\n')

	def writeframedata(self, data, chromdata):
		# XXXX Check sizes here
		self.fp.write(data)
		if chromdata:
			self.fp.write(chromdata)

	def writeframe(self, time, data, chromdata):
		if chromdata:
			clen = len(chromdata)
		else:
			clen = 0
		self.writeframeheader((time, len(data), clen))
		self.writeframedata(data, chromdata)

	def grabframe(self):
		return self._grabber(self.width, self.height, self.packfactor)
		
def test():
	import sys, time
	filename = 'film.video'
	if sys.argv[1:]: filename = sys.argv[1]
	vin = VinFile().init(filename)
	print 'File:    ', filename
	print 'Version: ', vin.version
	print 'Size:    ', vin.width, 'x', vin.height
	print 'Pack:    ', vin.packfactor, '; chrom:', vin.chrompack
	print 'Bits:    ', vin.c0bits, vin.c1bits, vin.c2bits
	print 'Format:  ', vin.format
	print 'Offset:  ', vin.offset
	gl.foreground()
	gl.prefsize(vin.width, vin.height)
	wid = gl.winopen(filename)
	vin.initcolormap()
	t0 = time.millitimer()
	while 1:
		try:
			t, data, chromdata = vin.getnextframe()
		except EOFError:
			break
		dt = t + t0 - time.millitimer()
		if dt > 0:
			time.millisleep(dt)
		vin.showframe(data, chromdata)
	print 'Done.'
	time.sleep(2)

