# Classes to read and write CMIF video files.
# (For a description of the CMIF video format, see cmif-file.ms.)


# Layers of functionality:
#
# VideoParams: maintain essential parameters of a video file
# Displayer: display a frame in a window (with some extra parameters)
# BasicVinFile: read a CMIF video file
# BasicVoutFile: write a CMIF video file
# VinFile: BasicVinFile + Displayer
# VoutFile: BasicVoutFile + Displayer
#
# XXX Future extension:
# BasicVinoutFile: supports overwriting of individual frames


# Imported modules

import sys
try:
	import gl
	import GL
	import GET
	no_gl = 0
except ImportError:
	no_gl = 1
import colorsys
import imageop


# Exception raised for various occasions

Error = 'VFile.Error'			# file format errors
CallError = 'VFile.CallError'		# bad call
AssertError = 'VFile.AssertError'	# internal malfunction


# Max nr. of colormap entries to use

MAXMAP = 4096 - 256


# Parametrizations of colormap handling based on color system.
# (These functions are used via eval with a constructed argument!)

def conv_grey(l, x, y):
	return colorsys.yiq_to_rgb(l, 0, 0)

def conv_grey4(l, x, y):
	return colorsys.yiq_to_rgb(l*17, 0, 0)

def conv_mono(l, x, y):
	return colorsys.yiq_to_rgb(l*255, 0, 0)

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

def conv_jpeg(r, g, b):
	raise Error, 'Attempt to make RGB colormap (jpeg)'

conv_jpeggrey = conv_grey
conv_grey2 = conv_grey


# Choose one of the above based upon a color system name

def choose_conversion(format):
	try:
		return eval('conv_' + format)
	except:
		raise Error, 'Unknown color system: ' + `format`


# Inverses of the above

def inv_grey(r, g, b):
	y, i, q = colorsys.rgb_to_yiq(r, g, b)
	return y, 0, 0

def inv_yiq(r, g, b):
	y, i, q = colorsys.rgb_to_yiq(r, g, b)
	return y, i/1.2 + 0.5, q + 0.5

def inv_hls(r, g, b):
	h, l, s = colorsys.rgb_to_hls(r, g, b)
	return l, h, s

def inv_hsv(r, g, b):
	h, s, v = colorsys.rgb_to_hsv(r, g, b)
	return v, h, s

def inv_rgb(r, g, b):
	raise Error, 'Attempt to invert RGB colormap'

def inv_rgb8(r, g, b):
	r = int(r*7.0)
	g = int(g*7.0)
	b = int(b*7.0)
	rgb = ((r&7) << 5) | ((b&3) << 3) | (g&7)
	return rgb / 255.0, 0, 0

def inv_jpeg(r, g, b):
	raise Error, 'Attempt to invert RGB colormap (jpeg)'

inv_jpeggrey = inv_grey


# Choose one of the above based upon a color system name

def choose_inverse(format):
	try:
		return eval('inv_' + format)
	except:
		raise Error, 'Unknown color system: ' + `format`


# Predicate to see whether this is an entry level (non-XS) Indigo.
# If so we can lrectwrite 8-bit wide pixels into a window in RGB mode

def is_entry_indigo():
	# XXX hack, hack.  We should call gl.gversion() but that doesn't
	# exist in earlier Python versions.  Therefore we check the number
	# of bitplanes *and* the size of the monitor.
	xmax = gl.getgdesc(GL.GD_XPMAX)
	if xmax <> 1024: return 0
	ymax = gl.getgdesc(GL.GD_YPMAX)
	if ymax != 768: return 0
	r = gl.getgdesc(GL.GD_BITS_NORM_SNG_RED)
	g = gl.getgdesc(GL.GD_BITS_NORM_SNG_GREEN)
	b = gl.getgdesc(GL.GD_BITS_NORM_SNG_BLUE)
	return (r, g, b) == (3, 3, 2)


# Predicate to see whether this machine supports pixmode(PM_SIZE) with
# values 1 or 4.
#
# XXX Temporarily disabled, since it is unclear which machines support
# XXX which pixelsizes.
#
# XXX The XS appears to support 4 bit pixels, but (looking at osview) it
# XXX seems as if the conversion is done by the kernel (unpacking ourselves
# XXX is faster than using PM_SIZE=4)

def support_packed_pixels():
	return 0   # To be architecture-dependent



# Tables listing bits per pixel for some formats

bitsperpixel = { \
	  'rgb': 32, \
	  'rgb8': 8, \
	  'grey': 8, \
	  'grey4': 4, \
	  'grey2': 2, \
	  'mono': 1, \
	  'compress': 32, \
}

bppafterdecomp = {'jpeg': 32, 'jpeggrey': 8}


# Base class to manage video format parameters

class VideoParams:

	# Initialize an instance.
	# Set all parameters to something decent
	# (except width and height are set to zero)

	def __init__(self):
		# Essential parameters
		self.frozen = 0		# if set, can't change parameters
		self.format = 'grey'	# color system used
		# Choose from: grey, rgb, rgb8, hsv, yiq, hls, jpeg, jpeggrey,
		#              mono, grey2, grey4
		self.width = 0		# width of frame
		self.height = 0		# height of frame
		self.packfactor = 1, 1	# expansion using rectzoom
		# Colormap info
		self.c0bits = 8		# bits in first color dimension
		self.c1bits = 0		# bits in second color dimension
		self.c2bits = 0		# bits in third color dimension
		self.offset = 0		# colormap index offset (XXX ???)
		self.chrompack = 0	# set if separate chrominance data
		self.setderived()
		self.decompressor = None

	# Freeze the parameters (disallow changes)

	def freeze(self):
		self.frozen = 1

	# Unfreeze the parameters (allow changes)

	def unfreeze(self):
		self.frozen = 0

	# Set some values derived from the standard info values

	def setderived(self):
		if self.frozen: raise AssertError
		if bitsperpixel.has_key(self.format):
			self.bpp = bitsperpixel[self.format]
		else:
			self.bpp = 0
		xpf, ypf = self.packfactor
		self.xpf = abs(xpf)
		self.ypf = abs(ypf)
		self.mirror_image = (xpf < 0)
		self.upside_down = (ypf < 0)
		self.realwidth = self.width / self.xpf
		self.realheight = self.height / self.ypf

	# Set colormap info

	def setcmapinfo(self):
		stuff = 0, 0, 0, 0, 0
		if self.format in ('rgb8', 'grey'):
			stuff = 8, 0, 0, 0, 0
		if self.format == 'grey4':
			stuff = 4, 0, 0, 0, 0
		if self.format == 'grey2':
			stuff = 2, 0, 0, 0, 0
		if self.format == 'mono':
			stuff = 1, 0, 0, 0, 0
		self.c0bits, self.c1bits, self.c2bits, \
			  self.offset, self.chrompack = stuff

	# Set the frame width and height (e.g. from gl.getsize())

	def setsize(self, width, height):
		if self.frozen: raise CallError
		width = (width/self.xpf)*self.xpf
		height = (height/self.ypf)*self.ypf
		self.width, self.height = width, height
		self.setderived()

	# Retrieve the frame width and height (e.g. for gl.prefsize())

	def getsize(self):
		return (self.width, self.height)

	# Set the format

	def setformat(self, format):
		if self.frozen: raise CallError
		self.format = format
		self.setderived()
		self.setcmapinfo()

	# Get the format

	def getformat(self):
		return self.format

	# Set the packfactor

	def setpf(self, pf):
		if self.frozen: raise CallError
		if type(pf) == type(1):
			pf = (pf, pf)
		if type(pf) is not type(()) or len(pf) <> 2: raise CallError
		self.packfactor = pf
		self.setderived()

	# Get the packfactor

	def getpf(self):
		return self.packfactor

	# Set all parameters

	def setinfo(self, values):
		if self.frozen: raise CallError
		self.setformat(values[0])
		self.setpf(values[3])
		self.setsize(values[1], values[2])
		(self.c0bits, self.c1bits, self.c2bits, \
			  self.offset, self.chrompack) = values[4:9]
		if self.format == 'compress' and len(values) > 9:
			self.compressheader = values[9]
		self.setderived()

	# Retrieve all parameters in a format suitable for a subsequent
	# call to setinfo()

	def getinfo(self):
		return (self.format, self.width, self.height, self.packfactor,\
			self.c0bits, self.c1bits, self.c2bits, self.offset, \
			self.chrompack)

	def getcompressheader(self):
		return self.compressheader

	def setcompressheader(self, ch):
		self.compressheader = ch

	# Write the relevant bits to stdout

	def printinfo(self):
		print 'Format:  ', self.format
		print 'Size:    ', self.width, 'x', self.height
		print 'Pack:    ', self.packfactor, '; chrom:', self.chrompack
		print 'Bpp:     ', self.bpp
		print 'Bits:    ', self.c0bits, self.c1bits, self.c2bits
		print 'Offset:  ', self.offset

	# Calculate data size, if possible
	# (Not counting frame header or cdata size)

	def calcframesize(self):
		if not self.bpp: raise CallError
		size = self.width/self.xpf * self.height/self.ypf
		size = (size * self.bpp + 7) / 8
		return size

	# Decompress a possibly compressed frame. This method is here
	# since you sometimes want to use it on a VFile instance and sometimes
	# on a Displayer instance.
	#
	# XXXX This should also handle jpeg. Actually, the whole mechanism
	# should be much more of 'ihave/iwant' style, also allowing you to
	# read, say, greyscale images from a color movie.
	
	def decompress(self, data):
		if self.format <> 'compress':
			return data
		if not self.decompressor:
			import cl
			scheme = cl.QueryScheme(self.compressheader)
			self.decompressor = cl.OpenDecompressor(scheme)
			headersize = self.decompressor.ReadHeader(self.compressheader)
			width = self.decompressor.GetParam(cl.IMAGE_WIDTH)
			height = self.decompressor.GetParam(cl.IMAGE_HEIGHT)
			params = [cl.ORIGINAL_FORMAT, cl.RGBX, \
				  cl.ORIENTATION, cl.BOTTOM_UP, \
				  cl.FRAME_BUFFER_SIZE, width*height*cl.BytesPerPixel(cl.RGBX)]
			self.decompressor.SetParams(params)
		data = self.decompressor.Decompress(1, data)
		return data


# Class to display video frames in a window.
# It is the caller's responsibility to ensure that the correct window
# is current when using showframe(), initcolormap(), clear() and clearto()

class Displayer(VideoParams):

	# Initialize an instance.
	# This does not need a current window

	def __init__(self):
		if no_gl:
			raise RuntimeError, \
				  'no gl module available, so cannot display'
		VideoParams.__init__(self)
		# User-settable parameters
		self.magnify = 1.0	# frame magnification factor
		self.xorigin = 0	# x frame offset
		self.yorigin = 0	# y frame offset (from bottom)
		self.quiet = 0		# if set, don't print messages
		self.fallback = 1	# allow fallback to grey
		# Internal flags
		self.colormapinited = 0	# must initialize window
		self.skipchrom = 0	# don't skip chrominance data
		self.color0 = None	# magic, used by clearto()
		self.fixcolor0 = 0	# don't need to fix color0
		self.mustunpack = (not support_packed_pixels())

	# setinfo() must reset some internal flags

	def setinfo(self, values):
		VideoParams.setinfo(self, values)
		self.colormapinited = 0
		self.skipchrom = 0
		self.color0 = None
		self.fixcolor0 = 0

	# Show one frame, initializing the window if necessary

	def showframe(self, data, chromdata):
		self.showpartframe(data, chromdata, \
			  (0,0,self.width,self.height))

	def showpartframe(self, data, chromdata, (x,y,w,h)):
		pmsize = self.bpp
		xpf, ypf = self.xpf, self.ypf
		if self.upside_down:
			gl.pixmode(GL.PM_TTOB, 1)
		if self.mirror_image:
			gl.pixmode(GL.PM_RTOL, 1)
		if self.format in ('jpeg', 'jpeggrey'):
			import jpeg
			data, width, height, bytes = jpeg.decompress(data)
			pmsize = bytes*8
		elif self.format == 'compress':
			data = self.decompress(data)
			pmsize = 32
		elif self.format in ('mono', 'grey4'):
			if self.mustunpack:
				if self.format == 'mono':
					data = imageop.mono2grey(data, \
						  w/xpf, h/ypf, 0x20, 0xdf)
				elif self.format == 'grey4':
					data = imageop.grey42grey(data, \
						  w/xpf, h/ypf)
				pmsize = 8
		elif self.format == 'grey2':
			data = imageop.grey22grey(data, w/xpf, h/ypf)
			pmsize = 8
		if not self.colormapinited:
			self.initcolormap()
		if self.fixcolor0:
			gl.mapcolor(self.color0)
			self.fixcolor0 = 0
		xfactor = yfactor = self.magnify
		xfactor = xfactor * xpf
		yfactor = yfactor * ypf
		if chromdata and not self.skipchrom:
			cp = self.chrompack
			cx = int(x*xfactor*cp) + self.xorigin
			cy = int(y*yfactor*cp) + self.yorigin
			cw = (w+cp-1)/cp
			ch = (h+cp-1)/cp
			gl.rectzoom(xfactor*cp, yfactor*cp)
			gl.pixmode(GL.PM_SIZE, 16)
			gl.writemask(self.mask - ((1 << self.c0bits) - 1))
			gl.lrectwrite(cx, cy, cx + cw - 1, cy + ch - 1, \
				  chromdata)
		#
		if pmsize < 32:
			gl.writemask((1 << self.c0bits) - 1)
		gl.pixmode(GL.PM_SIZE, pmsize)
		w = w/xpf
		h = h/ypf
		x = x/xpf
		y = y/ypf
		gl.rectzoom(xfactor, yfactor)
		x = int(x*xfactor)+self.xorigin
		y = int(y*yfactor)+self.yorigin
		gl.lrectwrite(x, y, x + w - 1, y + h - 1, data)
		gl.gflush()

	# Initialize the window: set RGB or colormap mode as required,
	# fill in the colormap, and clear the window

	def initcolormap(self):
		self.colormapinited = 1
		self.color0 = None
		self.fixcolor0 = 0
		if self.format in ('rgb', 'jpeg', 'compress'):
			self.set_rgbmode()
			gl.RGBcolor(200, 200, 200) # XXX rather light grey
			gl.clear()
			return
		# This only works on an Entry-level Indigo from IRIX 4.0.5
		if self.format == 'rgb8' and is_entry_indigo() and \
			  gl.gversion() == 'GL4DLG-4.0.': # Note trailing '.'!
			self.set_rgbmode()
			gl.RGBcolor(200, 200, 200) # XXX rather light grey
			gl.clear()
			gl.pixmode(GL.PM_SIZE, 8)
			return
		self.set_cmode()
		self.skipchrom = 0
		if self.offset == 0:
			self.mask = 0x7ff
		else:
			self.mask = 0xfff
		if not self.quiet:
			sys.stderr.write('Initializing color map...')
		self._initcmap()
		gl.clear()
		if not self.quiet:
			sys.stderr.write(' Done.\n')

	# Set the window in RGB mode (may be overridden for Glx window)

	def set_rgbmode(self):
		gl.RGBmode()
		gl.gconfig()

	# Set the window in colormap mode (may be overridden for Glx window)

	def set_cmode(self):
		gl.cmode()
		gl.gconfig()

	# Clear the window to a default color

	def clear(self):
		if not self.colormapinited: raise CallError
		if gl.getdisplaymode() in (GET.DMRGB, GET.DMRGBDOUBLE):
			gl.RGBcolor(200, 200, 200) # XXX rather light grey
			gl.clear()
			return
		gl.writemask(0xffffffff)
		gl.clear()

	# Clear the window to a given RGB color.
	# This may steal the first color index used; the next call to
	# showframe() will restore the intended mapping for that index

	def clearto(self, r, g, b):
		if not self.colormapinited: raise CallError
		if gl.getdisplaymode() in (GET.DMRGB, GET.DMRGBDOUBLE):
			gl.RGBcolor(r, g, b)
			gl.clear()
			return
		index = self.color0[0]
		self.fixcolor0 = 1
		gl.mapcolor(index, r, g, b)
		gl.writemask(0xffffffff)
		gl.clear()
		gl.gflush()

	# Do the hard work for initializing the colormap (internal).
	# This also sets the current color to the first color index
	# used -- the caller should never change this since it is used
	# by clear() and clearto()

	def _initcmap(self):
		map = []
		if self.format in ('mono', 'grey4') and self.mustunpack:
			convcolor = conv_grey
		else:
			convcolor = choose_conversion(self.format)
		maxbits = gl.getgdesc(GL.GD_BITS_NORM_SNG_CMODE)
		if maxbits > 11:
			maxbits = 11
		c0bits = self.c0bits
		c1bits = self.c1bits
		c2bits = self.c2bits
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
		self.color0 = None
		self.fixcolor0 = 0
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
						map.append((index, r, g, b))
						if self.color0 == None:
							self.color0 = \
								index, r, g, b
		self.install_colormap(map)
		# Permanently make the first color index current
		gl.color(self.color0[0])

	# Install the colormap in the window (may be overridden for Glx window)

	def install_colormap(self, map):
		if not self.quiet:
			sys.stderr.write(' Installing ' + `len(map)` + \
				  ' entries...')
		for irgb in map:
			gl.mapcolor(irgb)
		gl.gflush() # send the colormap changes to the X server


# Read a CMIF video file header.
# Return (version, values) where version is 0.0, 1.0, 2.0 or 3.[01],
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
	elif line == 'CMIF video 3.1\n':
		version = 3.1
	else:
		# XXX Could be version 0.0 without identifying header
		raise Error, \
			filename + ': Unrecognized file header: ' + `line`[:20]
	compressheader = None
	#
	# Get color encoding info
	# (The format may change to 'rgb' later when packfactor == 0)
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
	elif version in (3.0, 3.1):
		line = fp.readline()
		try:
			format, rest = eval(line[:-1])
		except:
			raise Error, filename + ': Bad 3.[01] color info'
		if format in ('rgb', 'jpeg'):
			c0bits = c1bits = c2bits = 0
			chrompack = 0
			offset = 0
		elif format == 'compress':
			c0bits = c1bits = c2bits = 0
			chrompack = 0
			offset = 0
			compressheader = rest
		elif format in ('grey', 'jpeggrey', 'mono', 'grey2', 'grey4'):
			c0bits = rest
			c1bits = c2bits = 0
			chrompack = 0
			offset = 0
		else:
			# XXX ought to check that the format is valid
			try:
			    c0bits, c1bits, c2bits, chrompack, offset = rest
			except:
			    raise Error, filename + ': Bad 3.[01] color info'
	if format == 'xrgb8':
		format = 'rgb8' # rgb8 upside-down, for X
		upside_down = 1
	else:
		upside_down = 0
	#
	# Get frame geometry info
	#
	line = fp.readline()
	try:
		x = eval(line[:-1])
	except:
		raise Error, filename + ': Bad (w,h,pf) info'
	if type(x) <> type(()):
		raise Error, filename + ': Bad (w,h,pf) info'
	if len(x) == 3:
		width, height, packfactor = x
		if packfactor == 0 and version < 3.0:
			format = 'rgb'
			c0bits = 0
	elif len(x) == 2 and version <= 1.0:
		width, height = x
		packfactor = 2
	else:
		raise Error, filename + ': Bad (w,h,pf) info'
	if type(packfactor) is type(0):
		if packfactor == 0: packfactor = 1
		xpf = ypf = packfactor
	else:
		xpf, ypf = packfactor
	if upside_down:
		ypf = -ypf
	packfactor = (xpf, ypf)
	xpf = abs(xpf)
	ypf = abs(ypf)
	width = (width/xpf) * xpf
	height = (height/ypf) * ypf
	#
	# Return (version, values)
	#
	values = (format, width, height, packfactor, \
		  c0bits, c1bits, c2bits, offset, chrompack, compressheader)
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
		raise Error, 'Bad 3.[01] frame header'
	return x


# Write a CMIF video file header (always version 3.1)

def writefileheader(fp, values):
	(format, width, height, packfactor, \
		c0bits, c1bits, c2bits, offset, chrompack) = values
	#
	# Write identifying header
	#
	fp.write('CMIF video 3.1\n')
	#
	# Write color encoding info
	#
	if format in ('rgb', 'jpeg'):
		data = (format, 0)
	elif format in ('grey', 'jpeggrey', 'mono', 'grey2', 'grey4'):
		data = (format, c0bits)
	else:
		data = (format, (c0bits, c1bits, c2bits, chrompack, offset))
	fp.write(`data`+'\n')
	#
	# Write frame geometry info
	#
	data = (width, height, packfactor)
	fp.write(`data`+'\n')
	
def writecompressfileheader(fp, cheader, values):
	(format, width, height, packfactor, \
		c0bits, c1bits, c2bits, offset, chrompack) = values
	#
	# Write identifying header
	#
	fp.write('CMIF video 3.1\n')
	#
	# Write color encoding info
	#
	data = (format, cheader)
	fp.write(`data`+'\n')
	#
	# Write frame geometry info
	#
	data = (width, height, packfactor)
	fp.write(`data`+'\n')


# Basic class for reading CMIF video files

class BasicVinFile(VideoParams):

	def __init__(self, filename):
		if type(filename) != type(''):
			fp = filename
			filename = '???'
		elif filename == '-':
			fp = sys.stdin
		else:
			fp = open(filename, 'r')
		self.initfp(fp, filename)

	def initfp(self, fp, filename):
		VideoParams.__init__(self)
		self.fp = fp
		self.filename = filename
		self.version, values = readfileheader(fp, filename)
		self.setinfo(values)
		self.freeze()
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
		elif self.version in (3.0, 3.1):
			self._readframeheader = readv3frameheader
		else:
			raise Error, \
				filename + ': Bad version: ' + `self.version`
		self.framecount = 0
		self.atframeheader = 1
		self.eofseen = 0
		self.errorseen = 0
		try:
			self.startpos = self.fp.tell()
			self.canseek = 1
		except IOError:
			self.startpos = -1
			self.canseek = 0

	def _readv0frameheader(self, fp):
		t, ds, cs = readv0frameheader(fp)
		ds = self._datasize
		return (t, ds, cs)

	def close(self):
		self.fp.close()
		del self.fp
		del self._readframeheader

	def rewind(self):
		if not self.canseek:
			raise Error, self.filename + ': can\'t seek'
		self.fp.seek(self.startpos)
		self.framecount = 0
		self.atframeheader = 1
		self.eofseen = 0
		self.errorseen = 0

	def warmcache(self):
		print '[BasicVinFile.warmcache() not implemented]'

	def printinfo(self):
		print 'File:    ', self.filename
		print 'Size:    ', getfilesize(self.filename)
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
		if self.eofseen: raise EOFError
		if self.errorseen: raise CallError
		if not self.atframeheader: raise CallError
		self.atframeheader = 0
		try:
			return self._readframeheader(self.fp)
		except Error, msg:
			self.errorseen = 1
			# Patch up the error message
			raise Error, self.filename + ': ' + msg
		except EOFError:
			self.eofseen = 1
			raise EOFError

	def getnextframedata(self, ds, cs):
		if self.eofseen: raise EOFError
		if self.errorseen: raise CallError
		if self.atframeheader: raise CallError
		if ds:
			data = self.fp.read(ds)
			if len(data) < ds:
				self.eofseen = 1
				raise EOFError
		else:
			data = ''
		if cs:
			cdata = self.fp.read(cs)
			if len(cdata) < cs:
				self.eofseen = 1
				raise EOFError
		else:
			cdata = ''
		self.atframeheader = 1
		self.framecount = self.framecount + 1
		return (data, cdata)

	def skipnextframedata(self, ds, cs):
		if self.eofseen: raise EOFError
		if self.errorseen: raise CallError
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


# Subroutine to return a file's size in bytes

def getfilesize(filename):
	import os, stat
	try:
		st = os.stat(filename)
		return st[stat.ST_SIZE]
	except os.error:
		return 0


# Derived class implementing random access and index cached in the file

class RandomVinFile(BasicVinFile):

	def initfp(self, fp, filename):
		BasicVinFile.initfp(self, fp, filename)
		self.index = []

	def warmcache(self):
		if len(self.index) == 0:
			try:
				self.readcache()
			except Error:
				self.buildcache()
		else:
			print '[RandomVinFile.warmcache(): too late]'
			self.rewind()

	def buildcache(self):
		self.index = []
		self.rewind()
		while 1:
			try: dummy = self.skipnextframe()
			except EOFError: break
		self.rewind()

	def writecache(self):
		# Raises IOerror if the file is not seekable & writable!
		import marshal
		if len(self.index) == 0:
			self.buildcache()
			if len(self.index) == 0:
				raise Error, self.filename + ': No frames'
		self.fp.seek(0, 2)
		self.fp.write('\n/////CMIF/////\n')
		pos = self.fp.tell()
		data = `pos`
		data = '\n-*-*-CMIF-*-*-\n' + data + ' '*(15-len(data)) + '\n'
		try:
			marshal.dump(self.index, self.fp)
			self.fp.write(data)
			self.fp.flush()
		finally:
			self.rewind()

	def readcache(self):
		# Raises Error if there is no cache in the file
		import marshal
		if len(self.index) <> 0:
			raise CallError
		self.fp.seek(-32, 2)
		data = self.fp.read()
		if data[:16] <> '\n-*-*-CMIF-*-*-\n' or data[-1:] <> '\n':
			self.rewind()
			raise Error, self.filename + ': No cache'
		pos = eval(data[16:-1])
		self.fp.seek(pos)
		try:
			self.index = marshal.load(self.fp)
		except TypeError:
			self.rewind()
			raise Error, self.filename + ': Bad cache'
		self.rewind()

	def getnextframeheader(self):
		if self.framecount < len(self.index):
			return self._getindexframeheader(self.framecount)
		if self.framecount > len(self.index):
			raise AssertError, \
				'managed to bypass index?!?'
		rv = BasicVinFile.getnextframeheader(self)
		if self.canseek:
			pos = self.fp.tell()
			self.index.append((rv, pos))
		return rv

	def getrandomframe(self, i):
		t, ds, cs = self.getrandomframeheader(i)
		data, cdata = self.getnextframedata(ds, cs)
		return t, data, cdata

	def getrandomframeheader(self, i):
		if i < 0: raise ValueError, 'negative frame index'
		if not self.canseek:
			raise Error, self.filename + ': can\'t seek'
		if i < len(self.index):
			return self._getindexframeheader(i)
		if len(self.index) > 0:
			rv = self.getrandomframeheader(len(self.index)-1)
		else:
			self.rewind()
			rv = self.getnextframeheader()
		while i > self.framecount:
			self.skipnextframedata()
			rv = self.getnextframeheader()
		return rv

	def _getindexframeheader(self, i):
		(rv, pos) = self.index[i]
		self.fp.seek(pos)
		self.framecount = i
		self.atframeheader = 0
		self.eofseen = 0
		self.errorseen = 0
		return rv


# Basic class for writing CMIF video files

class BasicVoutFile(VideoParams):

	def __init__(self, filename):
		if type(filename) != type(''):
			fp = filename
			filename = '???'
		elif filename == '-':
			fp = sys.stdout
		else:
			fp = open(filename, 'w')
		self.initfp(fp, filename)

	def initfp(self, fp, filename):
		VideoParams.__init__(self)
		self.fp = fp
		self.filename = filename
		self.version = 3.1 # In case anyone inquries

	def flush(self):
		self.fp.flush()

	def close(self):
		self.fp.close()
		del self.fp

	def prealloc(self, nframes):
		if not self.frozen: raise CallError
		data = '\xff' * (self.calcframesize() + 64)
		pos = self.fp.tell()
		for i in range(nframes):
			self.fp.write(data)
		self.fp.seek(pos)

	def writeheader(self):
		if self.frozen: raise CallError
		if self.format == 'compress':
			writecompressfileheader(self.fp, self.compressheader, \
				  self.getinfo())
		else:
			writefileheader(self.fp, self.getinfo())
		self.freeze()
		self.atheader = 1
		self.framecount = 0

	def rewind(self):
		self.fp.seek(0)
		self.unfreeze()
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
		if not self.frozen: self.writeheader()
		if not self.atheader: raise CallError
		data = `(t, ds, cs)`
		n = len(data)
		if n < 63: data = data + ' '*(63-n)
		self.fp.write(data + '\n')
		self.atheader = 0

	def writeframedata(self, data, cdata):
		if not self.frozen or self.atheader: raise CallError
		if data: self.fp.write(data)
		if cdata: self.fp.write(cdata)
		self.atheader = 1
		self.framecount = self.framecount + 1


# Classes that combine files with displayers:

class VinFile(RandomVinFile, Displayer):

	def initfp(self, fp, filename):
		Displayer.__init__(self)
		RandomVinFile.initfp(self, fp, filename)

	def shownextframe(self):
		t, data, cdata = self.getnextframe()
		self.showframe(data, cdata)
		return t


class VoutFile(BasicVoutFile, Displayer):

	def initfp(self, fp, filename):
		Displayer.__init__(self)
##		Grabber.__init__(self) # XXX not needed
		BasicVoutFile.initfp(self, fp, filename)


# Simple test program (VinFile only)

def test():
	import time
	if sys.argv[1:]: filename = sys.argv[1]
	else: filename = 'film.video'
	vin = VinFile(filename)
	vin.printinfo()
	gl.foreground()
	gl.prefsize(vin.getsize())
	wid = gl.winopen(filename)
	vin.initcolormap()
	t0 = time.time()
	while 1:
		try: t, data, cdata = vin.getnextframe()
		except EOFError: break
		dt = t0 + t - time.time()
		if dt > 0: time.time(dt)
		vin.showframe(data, cdata)
	time.sleep(2)
