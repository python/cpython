# Live video output (display video on the screen, presumably from the net)

import gl
from VFile import Displayer


# Video output (displayer) class.

class LiveVideoOut:

	def init(self, wid, xywh, vw, vh):
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
		self.resize(xywh)
		gl.winset(oldwid)
		return self

	def resize(self, (x, y, w, h)):
		oldwid = gl.winget()
		gl.winset(self.wid)
		##print 'Resize', x, y, w, h
		gl.winposition(x, x+w-1, y, y+h-1)
		gl.reshapeviewport()
		if w < self.vw or h < self.vh:
			self.toosmall = 1
		else:
			self.disp.xorigin = (w-self.vw)/2
			self.disp.yorigin = (h-self.vh)/2
			self.toosmall = 0
			##print 'VIDEO OFFSET:', \
			##	self.disp.xorigin, self.disp.yorigin
		self.disp.clear()
		gl.winset(oldwid)

	def putnextpacket(self, pos, data):
		if self.toosmall:
			return
		oldwid = gl.winget()
		gl.winset(self.wid)
		nline = len(data)/self.vw
		if nline*self.vw <> len(data):
			print 'Incorrect-sized video fragment'
		self.disp.showpartframe(data, None, (0, pos, self.vw, nline))
		gl.winset(oldwid)

	def close(self):
		##print 'Done video out'
		pass
