# srcwin.py -- a source listing window

import stdwin
from stdwinevents import *
import basewin

WIDTH = 40
MAXHEIGHT = 24

class SourceWindow(basewin.BaseWindow):
	
	def init(self, filename):
		self.filename = filename
		#
		f = open(self.filename, 'r') # raise exception if not found
		self.contents = f.read()
		f.seek(0)
		self.linecount = len(f.readlines())
		f.close()
		#
		self.lineheight = lh = stdwin.lineheight()
		self.leftmargin = stdwin.textwidth('00000000')
		self.rightmargin = 30000 # Infinity
		self.bottom = lh * self.linecount
		#
		stdwin.setdefwinpos(0, 0)
		width = WIDTH*stdwin.textwidth('0')
		height = lh*min(MAXHEIGHT, self.linecount)
		stdwin.setdefwinsize(width, height)
		self = basewin.BaseWindow.init(self, filename)
		#
		self.win.setdocsize(0, self.bottom + lh)
		self.initeditor()
		return self
	
	def initeditor(self):
		r = (self.leftmargin, 0), (self.rightmargin, self.bottom)
		self.editor = self.win.textcreate(r)
		self.editor.settext(self.contents)
	
	def closeeditor(self):
		self.editor.close()
	
	def reopen(self):
		self.closeeditor()
		basewin.BaseWindow.reopen(self)
		self.initeditor()
	
	def close(self):
		self.closeeditor()
		basewin.BaseWindow.close(self)
	
	# Override this method to format line numbers differently
	def getmark(self, lineno):
		return `lineno`
	
	def dispatch(self, event):
		if event[0] == WE_NULL: return # Dummy tested by mainloop
		if event[0] == WE_DRAW or not self.editor.event(event):
			basewin.BaseWindow.dispatch(self, event)
	
	def draw(self, detail):
		dummy = self.editor.draw(detail)
		# Draw line numbers
		(left, top), (right, bottom) = detail
		topline = top/self.lineheight
		botline = bottom/self.lineheight + 1
		botline = min(self.linecount, botline)
		d = self.win.begindrawing()
		try:
			h, v = 0, self.lineheight * topline
			for lineno in range(topline+1, botline+1):
				d.text((h, v), self.getmark(lineno))
				v = v + self.lineheight
		finally:
			d.close()
	
	def changemark(self, lineno):
		left = 0
		top = (lineno-1) * self.lineheight
		right = self.leftmargin
		bottom = lineno * self.lineheight
		d = self.win.begindrawing()
		try:
			d.erase((left, top), (right, bottom))
			d.text((left, top), self.getmark(lineno))
		finally:
			d.close()

	def showline(self, lineno):
		left = 0
		top = (lineno-1) * self.lineheight
		right = self.leftmargin
		bottom = lineno * self.lineheight
		self.win.show((left, top), (right, bottom))


TESTFILE = 'srcwin.py'

def test():
	import mainloop
	sw = SourceWindow().init(TESTFILE)
	mainloop.mainloop()
