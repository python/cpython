# VFile -- two classes for Video Files.
# VinFile -- for video input.
# VoutFile -- for video output.

import sys
import gl
import GL
import colorsys

Error = 'VFile.Error' # Exception

def conv_grey(l,x,y): return colorsys.yiq_to_rgb(l,0,0)
def conv_yiq (y,i,q): return colorsys.yiq_to_rgb(y, (i-0.5)*1.2, q-0.5)
def conv_hls (l,h,s): return colorsys.hls_to_rgb(h,l,s)
def conv_hsv (v,h,s): return colorsys.hsv_to_rgb(h,s,v)
def conv_rgb (r,g,b):
    raise Error, 'Attempt to make RGB colormap'

# Class VinFile represents a video file used for input.
#
# It has the following methods:
# init(filename)
# initfp(fp, filename)
# rewind()
# getframe()
# skipframe()
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
# xorigin, yorigin

class VinFile():

	# init() and initfp() raise Error if the header is bad.
	# init() raises whatever open() raises if the file can't be opened.

	def init(self, filename):
		if filename = '-':
			return self.initfp(sys.stdin, filename)
		return self.initfp(open(filename, 'r'), filename)

	def initfp(self, (fp, filename)):
		self.colormapinited = 0
		self.magnify = 1.0
		self.xorigin = self.yorigin = 0
		self.fp = fp
		self.filename = filename
		#
		line = self.fp.readline()
		if line = 'CMIF video 1.0\n':
			self.version = 1.0
		elif line = 'CMIF video 2.0\n':
			self.version = 2.0
		elif line = 'CMIF video 3.0\n':
			self.version = 3.0
		else:
			raise Error, self.filename + ': bad video format'
		#
		if self.version < 2.0:
			self.c0bits, self.c1bits, self.c2bits = 8, 0, 0
			self.chrompack = 0
			self.offset = 0
			self.format = 'grey'
		elif self.version = 2.0:
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
				raise Error, self.filename + ': bad 2.0 color info'
		elif self.version = 3.0:
			line = self.fp.readline()
			try:
			    self.format, rest = eval(line[:-1])
			    if self.format = 'rgb':
				pass
			    elif self.format = 'grey':
				self.offset = 0
				self.c0bits = rest
				self.c1bits = self.c2bits = \
						self.chrompack = 0
			    else:
				self.c0bits,self.c1bits,self.c2bits,\
				    self.chrompack,self.offset = rest
			except:
			    raise Error, self.filename + ': bad 3.0 color info'

		try:
		    self.convcolor = eval('conv_'+self.format)
		except:
		    raise Error, self.filename + ': unknown colorsys ' + self.format
		#
		line = self.fp.readline()
		try:
			x = eval(line[:-1])
			if self.version > 1.0 or len(x) = 3:
				self.width, self.height, self.packfactor = x
				if self.packfactor = 0:
				    self.format = 'rgb'
			else:
				sef.width, self.height = x
				self.packfactor = 2
		except:
			raise Error, self.filename + ': bad (w,h,pf) info'
		#
		return self

	# rewind() raises Error if the header is bad (which can only
	# happen if the file was written to since opened).

	def rewind(self):
		self.fp.seek(0)
		x = self.initfp(self.fp, self.filename)

	# getnextframe() raises EOFError (built-in) if there is no next frame,
	# or if the next frame is broken.
	# So to getnextframeheader(), getnextframedata() and skipnextframe().

	def getnextframe(self):
		time, size, chromsize = self.getnextframeheader()
		data, chromdata = self.getnextframedata(size, chromsize)
		return time, data, chromdata

	def getnextframedata(self, (size, chromsize)):
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

	def skipnextframedata(self, (size, chromsize)):
		# Note that this won't raise EOFError for a partial frame.
		try:
			self.fp.seek(size + chromsize, 1) # Relatc1ve seek
		except:
			# Assume it's a pipe -- read the data to discard it
			dummy = self.fp.read(size)
			dummy = self.fp.read(chromsize)

	def getnextframeheader(self):
		line = self.fp.readline()
		if not line:
			raise EOFError
		#
		w, h, pf = self.width, self.height, self.packfactor
		try:
			x = eval(line[:-1])
			if type(x) in (type(0), type(0.0)):
				time = x
				if pf = 0:
					size = w * h * 4
				else:
					size = (w/pf) * (h/pf)
			elif len(x) = 2:
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
		return time, size, chromsize

	def shownextframe(self):
		time, data, chromdata = self.getnextframe()
		self.showframe(data, chromdata)
		return time

	def showframe(self, (data, chromdata)):
		w, h, pf = self.width, self.height, self.packfactor
		if not self.colormapinited:
			self.initcolormap()
		factor = self.magnify
		if pf: factor = factor * pf
		if chromdata:
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
		if self.format = 'rgb':
		    gl.RGBmode()
		    gl.gconfig()
		    return
		initcmap(self.convcolor, self.c0bits, self.c1bits, self.c2bits, self.chrompack, self.offset)
		if self.offset == 0:
		    gl.color(0x800)
		    self.mask = 0x7ff
		else:
		    self.mask = 0xfff
		gl.clear()


def initcmap(convcolor, c0bits, c1bits, c2bits, chrompack, offset):
	if c0bits+c1bits+c2bits > 11:
		raise Error, 'Sorry, 11 bits max'
	import colorsys
	maxc0 = 1 << c0bits
	maxc1 = 1 << c1bits
	maxc2 = 1 << c2bits
	if offset = 0:
	    offset = 2048
	rng = (offset, 4192-256)
	for i in range(rng):
		gl.mapcolor(i, 0, 255, 0)
	for c0 in range(maxc0):
		c0v = c0/float(maxc0-1)
		for c1 in range(maxc1):
			if maxc1 = 1:
				c1v = 0
			else:
				c1v = c1/float(maxc1-1)
			for c2 in range(maxc2):
				if maxc2 = 1:
					c2v = 0
				else:
					c2v = c2/float(maxc2-1)
				index = offset + c0 + \
					(c1 << c0bits) + (c2 << (c0bits+c1bits))
				rv, gv, bv = convcolor(c0v, c1v, c2v)
				r, g, b = \
				    int(rv*255.0), int(gv*255.0), int(bv*255.0)
				if index < 4096 - 256:
					gl.mapcolor(index, r, g, b)


def test():
	import sys
	filename = 'film.video'
	if sys.argv[1:]: filename = sys.argv[1]
	vin = VinFile().init(filename)
	print 'Version: ', vin.version
	print 'Size: ', vin.width, 'x', vin.height
	print 'Pack: ', vin.packfactor, vin.chrompack
	print 'Bits: ', vin.c0bits, vin.c1bits, vin.c2bits
	print 'Format: ', vin.format
	print 'Offset: ', vin.offset
	gl.foreground()
	gl.prefsize(vin.width, vin.height)
	wid = gl.winopen('VFile.test: ' + filename)
	try:
		while 1:
			t = vin.shownextframe()
	except EOFError:
		pass
	import time
	time.sleep(5)
