# srcwin.py -- a source listing window

import stdwin
from stdwinevents import *
import basewin

WIDTH = 40
MAXHEIGHT = 24


class TextWindow(basewin.BaseWindow):
	
	def __init__(self, title, contents):
		self.contents = contents
		self.linecount = countlines(self.contents)
		#
		self.lineheight = lh = stdwin.lineheight()
		self.leftmargin = self.getmargin()
		self.top = 0
		self.rightmargin = 30000 # Infinity
		self.bottom = lh * self.linecount
		#
		width = WIDTH*stdwin.textwidth('0')
		height = lh*min(MAXHEIGHT, self.linecount)
		stdwin.setdefwinsize(width, height)
		basewin.BaseWindow.__init__(self, title)
		#
		self.win.setdocsize(0, self.bottom)
		self.initeditor()
	
	def initeditor(self):
		r = (self.leftmargin, self.top), (self.rightmargin, self.bottom)
		self.editor = self.win.textcreate(r)
		self.editor.settext(self.contents)
	
	def closeeditor(self):
		self.editor.close()
	
#	def reopen(self):
#		self.closeeditor()
#		basewin.BaseWindow.reopen(self)
#		self.initeditor()
	
	# Override the following two methods to format line numbers differently
	
	def getmark(self, lineno):
		return `lineno`
	
	def getmargin(self):
		return stdwin.textwidth(`self.linecount + 1` + ' ')
	
	# Event dispatcher, called from mainloop.mainloop()
	
	def dispatch(self, event):
		if event[0] == WE_NULL: return # Dummy tested by mainloop
		if event[0] == WE_DRAW or not self.editor.event(event):
			basewin.BaseWindow.dispatch(self, event)
	
	# Event handlers
	
	def close(self):
		self.closeeditor()
		basewin.BaseWindow.close(self)
	
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
	
	# Calls from outside
	
	def changemark(self, lineno): # redraw the mark for a line
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

	def showline(self, lineno): # scroll to make a line visible
		left = 0
		top = (lineno-1) * self.lineheight
		right = self.leftmargin
		bottom = lineno * self.lineheight
		self.win.show((left, top), (right, bottom))


# Subroutine to count the number of lines in a string

def countlines(text):
	n = 0
	for c in text:
		if c == '\n': n = n+1
	if text and text[-1] != '\n': n = n+1 # Partial last line
	return n


class SourceWindow(TextWindow):

	def __init__(self, filename):
		self.filename = filename
		f = open(self.filename, 'r')
		contents = f.read()
		f.close()
		TextWindow.__init__(self, self.filename, contents)

# ------------------------------ testing ------------------------------

TESTFILE = 'srcwin.py'

def test():
	import mainloop
	sw = SourceWindow(TESTFILE)
	mainloop.mainloop()
