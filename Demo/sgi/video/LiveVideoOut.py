# Live video output (display video on the screen, presumably from the net)

import gl
from VFile import Displayer


# Video output (displayer) class.

class LiveVideoOut:

	# Call this to initialize things.  Arguments:
	# wid:    the window id where the video is to be displayed (centered)
	# vw, vh: size of the video image to be displayed

	def __init__(self, wid, vw, vh, type):
		##print 'Init', wid, xywh
		##print 'video', vw, vw
		self.vw = vw
		self.vh = vh
		self.disp = Displayer()
		if not type in ('rgb', 'rgb8', 'grey', 'mono', 'grey2', \
			  'grey4'):
			raise 'Incorrent live video output type', type
		if type == 'rgb':
			info = (type, vw, vh, 0, 32, 0, 0, 0, 0)
		else:
			info = (type, vw, vh, 1, 8, 0, 0, 0, 0)
		self.disp.setinfo(info)
		self.wid = wid
		oldwid = gl.winget()
		gl.winset(wid)
		self.disp.initcolormap()
		self.reshapewindow()
		gl.winset(oldwid)

	# Call this in response to every REDRAW event for the window
	# or if the window size has changed for other reasons.

	def reshapewindow(self):
		oldwid = gl.winget()
		gl.winset(self.wid)
		gl.reshapeviewport()
		w, h = gl.getsize()
		self.disp.xorigin = (w-self.vw)/2
		self.disp.yorigin = (h-self.vh)/2
		self.disp.clear()
		gl.winset(oldwid)

	# Call this to change the size of the video images being displayed.
	# Implies reshapewindow().

	def resizevideo(self, vw, vh):
		self.vw, self.vh = vw, vh
		self.disp.setsize(vw, vh)
		self.reshapewindow()

	# Return the number of bytes in one video line
	def linewidth(self):
		if self.disp.format == 'rgb':
			return self.vw*4
		if self.disp.format == 'mono':
			return (self.vw+7)/8
		elif self.disp.format == 'grey2':
			return (self.vw+3)/4
		elif self.disp.format == 'grey4':
			return (self.vw+1)/2
		else:
			return self.vw

	# Call this to display the next video packet.  Arguments:
	# pos:  line number where the packet begins
	# data: image data of the packet
	# (these correspond directly to the return values from
	# LiveVideoIn.getnextpacket()).

	def putnextpacket(self, pos, data):
		nline = len(data)/self.linewidth()
		if nline*self.linewidth() <> len(data):
			print 'Incorrect-sized video fragment ignored'
			return
		oldwid = gl.winget()
		gl.winset(self.wid)
		self.disp.showpartframe(data, None, (0, pos, self.vw, nline))
		gl.winset(oldwid)

	# Call this to close the window.

	def close(self):
		pass

	# Call this to set optional mirroring of video
	def setmirror(self, mirrored):
		f, w, h, pf, c0, c1, c2, o, cp = self.disp.getinfo()
		if type(pf) == type(()):
			xpf, ypf = pf
		else:
			xpf = ypf = pf
		xpf = abs(xpf)
		if mirrored:
			xpf = -xpf
		info = (f, w, h, (xpf, ypf), c0, c1, c2, o, cp)
		self.disp.setinfo(info)
		self.disp.initcolormap()
		self.disp.clear()

#
# This derived class has one difference with the base class: the video is
# not displayed until an entire image has been gotten
#
class LiveVideoOutSlow(LiveVideoOut):

	# Reshapewindow - Realloc buffer.
	# (is also called by __init__() indirectly)

	def reshapewindow(self):
		LiveVideoOut.reshapewindow(self)
		self.buffer = '\0'*self.linewidth()*self.vh
		self.isbuffered = []

	# putnextpacket - buffer incoming data until a complete
	# image has been received

	def putnextpacket(self, pos, data):
		if pos in self.isbuffered or pos == 0:
			LiveVideoOut.putnextpacket(self, 0, self.buffer)
			self.isbuffered = []
		self.isbuffered.append(pos)
		bpos = pos * self.linewidth()
		epos = bpos + len(data)
		self.buffer = self.buffer[:bpos] + data + self.buffer[epos:]
