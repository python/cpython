# Class to grab frames from a window.
# (This has fewer user-settable parameters than Displayer.)
# It is the caller's responsibility to initialize the window and to
# ensure that it is current when using grabframe()

import gl, GL
import VFile
import GET
from VFile import Error

class VGrabber(VFile.VideoParams):

	# XXX The constructor of VideoParams is just fine, for now

	# Grab a frame.
	# Return (data, chromdata) just like getnextframe().

	def grabframe(self):
		grabber = choose_grabber(self.format)
		return grabber(self.width, self.height, self.packfactor)


# Choose one of the grabber functions below based upon a color system name

def choose_grabber(format):
	try:
		return eval('grab_' + format)
	except:
		raise Error, 'Unknown color system: ' + `format`


# Routines to grab data, per color system (only a few really supported).
# (These functions are used via eval with a constructed argument!)

def grab_rgb(w, h, pf):
	if gl.getdisplaymode() <> GET.DMRGB:
		raise Error, 'Sorry, can only grab rgb in single-buf rgbmode'
	if pf <> (1, 1):
		raise Error, 'Sorry, only grab rgb with packfactor (1,1)'
	return gl.lrectread(0, 0, w-1, h-1), None

def grab_rgb8(w, h, pf):
	if gl.getdisplaymode() <> GET.DMRGB:
		raise Error, 'Sorry, can only grab rgb8 in single-buf rgbmode'
	if pf <> (1, 1):
		raise Error, 'Sorry, can only grab rgb8 with packfactor (1,1)'
	if not VFile.is_entry_indigo():
		raise Error, 'Sorry, can only grab rgb8 on entry level Indigo'
	# XXX Dirty Dirty here.
	# XXX Set buffer to cmap mode, grab image and set it back.
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

def grab_jpeg(w, h, pf):
	data, dummy = grab_rgb(w, h, pf)
	import jpeg
	data = jpeg.compress(data, w, h, 4)
	return data, None

def grab_jpeggrey(w, h, pf):
	raise Error, 'sorry, grabbing jpeggrey not implemented'
