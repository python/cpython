# Live video output (display video on the screen, presumably from the net)

import gl
from VFile import Displayer


# Video output (displayer) class.

class LiveVideoOut:

	# Call this to initialize things.  Arguments:
	# wid:    the window id where the video is to be displayed (centered)
	# vw, vh: size of the video image to be displayed

	def init(self, wid, vw, vh):
		##print 'Init', wid, xywh
		##print 'video', vw, vw
		self.vw = vw
		self.vh = vh
		self.disp = Displayer().init()
		info = ('rgb8', vw, vh, 1, 8, 0, 0, 0, 0)
		self.disp.setinfo(info)
		self.wid = wid
		oldwid = gl.winget()
		gl.winset(wid)
		self.disp.initcolormap()
		self.reshapewindow()
		gl.winset(oldwid)
		return self

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

	# Call this to display the next video packet.  Arguments:
	# pos:  line number where the packet begins
	# data: image data of the packet
	# (these correspond directly to the return values from
	# LiveVideoIn.getnextpacket()).

	def putnextpacket(self, pos, data):
		nline = len(data)/self.vw
		if nline*self.vw <> len(data):
			print 'Incorrect-sized video fragment ignored'
			return
		oldwid = gl.winget()
		gl.winset(self.wid)
		self.disp.showpartframe(data, None, (0, pos, self.vw, nline))
		gl.winset(oldwid)

	# Call this to close the window.

	def close(self):
		pass
