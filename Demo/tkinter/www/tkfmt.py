# Tk backend -- unfinished

debug = 0

from fmt import *

class TkFormatter:

	def __init__(self, text):
		self.text = text	# The text widget to draw in
		self.nospace = 1
		self.blanklines = 0
		self.font = ''

	# Methods called by htmllib.FormattingParser:

	def setfont(self, font):
		if 1 or debug: print "setfont(%s)" % `font`
		self.font = font

	def resetfont(self):
		if debug: print "resetfont()"
		self.font = ''

	def flush(self):
		if debug: print "flush()"
		self.needvspace(1)

	def setleftindent(self, n):
		if debug: print "setleftindent(%d)" % n

	def needvspace(self, n):
		if debug: print "needvspace(%d)" % n
		self.blanklines = max(n, self.blanklines)
		self.nospace = 1

	def addword(self, word, nspaces):
		if debug: print "addword(%s, %d)" % (`word`, nspaces)
		if self.nospace and not word:
			return
		if self.blanklines > 0:
			word = '\n'*self.blanklines + word
		self.blanklines = 0
		self.nospace = 0
		here = self.text.index('end')
		self.text.insert('end', word + nspaces*' ')
		if not self.font:
			self.tag_remo

	def setjust(self, c):
		if debug: print "setjust(%s)" % `c`

	def bgn_anchor(self):
		if debug: print "bgn_anchor()"

	def end_anchor(self):
		if debug: print "end_anchor()"

	def hrule(self):
		if debug: print "hrule()"
		self.flush()
		self.addword('_'*60, 0)
		self.flush()
