# VFile -- two classes for Video Files.
# VinFile -- for video input.
# VoutFile -- for video output.

import sys
import gl
import GL

Error = 'VFile.Error' # Exception


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
# ybits, ibits, qbits, chrompack
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
		else:
			raise Error, self.filename + ': bad video format'
		#
		if self.version < 2.0:
			self.ybits, self.ibits, self.qbits = 8, 0, 0
			self.chrompack = 0
		else:
			line = self.fp.readline()
			try:
				self.ybits, self.ibits, self.qbits, \
					self.chrompack = eval(line[:-1])
			except:
				raise Error, self.filename + ': bad color info'
		#
		line = self.fp.readline()
		try:
			x = eval(line[:-1])
			if self.version > 1.0 or len(x) = 3:
				self.width, self.height, self.packfactor = x
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
			self.fp.seek(size + chromsize, 1) # Relative seek
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
		factor = self.magnify
		if pf: factor = factor * pf
		if chromdata:
			cp = self.chrompack
			cw = (w+cp-1)/cp
			ch = (h+cp-1)/cp
			gl.rectzoom(factor*cp, factor*cp)
			gl.pixmode(GL.PM_SIZE, 16)
			gl.writemask(0x7ff - ((1 << self.ybits) - 1))
			gl.lrectwrite(self.xorigin, self.yorigin, \
				self.xorigin + cw - 1, self.yorigin + ch - 1, \
				chromdata)
		#
		if pf:
			if not self.colormapinited:
				self.initcolormap()
			gl.writemask((1 << self.ybits) - 1)
			gl.pixmode(GL.PM_SIZE, 8)
			gl.rectzoom(factor, factor)
			w = w/pf
			h = h/pf
		gl.lrectwrite(self.xorigin, self.yorigin, \
			self.xorigin + w - 1, self.yorigin + h - 1, data)

	def initcolormap(self):
		initcmap(self.ybits, self.ibits, self.qbits, self.chrompack)
		gl.color(0x800)
		gl.clear()
		self.colormapinited = 1


def initcmap(ybits, ibits, qbits, chrompack):
	if ybits+ibits+qbits > 11:
		raise Error, 'Sorry, 11 bits max'
	import colorsys
	maxy = 1 << ybits
	maxi = 1 << ibits
	maxq = 1 << qbits
	for i in range(2048, 4096-256):
		gl.mapcolor(i, 0, 255, 0)
	for y in range(maxy):
		yv = y/float(maxy-1)
		for i in range(maxi):
			if maxi = 1:
				iv = 0
			else:
				iv = (i/float(maxi-1))-0.5
			for q in range(maxq):
				if maxq = 1:
					qv = 0
				else:
					qv = (q/float(maxq-1))-0.5
				index = 2048 + y + \
					(i << ybits) + (q << (ybits+ibits))
				rv, gv, bv = colorsys.yiq_to_rgb(yv, iv, qv)
				r, g, b = \
				    int(rv*255.0), int(gv*255.0), int(bv*255.0)
				if index < 4096 - 256:
					gl.mapcolor(index, r, g, b)


def test():
	import sys
	filename = 'film.video'
	if sys.argv[1:]: filename = sys.argv[1]
	vin = VinFile().init(filename)
	gl.foreground()
	gl.prefsize(vin.width, vin.height)
	wid = gl.winopen('VFile.test: ' + filename)
	try:
		while 1:
			t = vin.shownextframe()
	except EOFError:
		pass
