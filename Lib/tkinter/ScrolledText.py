# A ScrolledText widget feels like a text widget but also has a
# vertical scroll bar on its right.  (Later, options may be added to
# add a horizontal bar as well, to make the bars disappear
# automatically when not needed, to move them to the other side of the
# window, etc.)
#
# Configuration options are passed to the Text widget.
# A Frame widget is inserted between the master and the text, to hold
# the Scrollbar widget.
# Most methods calls are passed to the Text widget; the pack command
# is redirected to the Frame widget however.

from Tkinter import *

class ScrolledText(Pack, Place):
	def __init__(self, master=None, cnf={}):
		fcnf = {}
		self.frame = Frame(master, {})
		if cnf.has_key(Pack):
			self.frame.pack(cnf[Pack])
			del cnf[Pack]
		self.vbar = Scrollbar(self.frame, {})
		self.vbar.pack({'side': 'right', 'fill': 'y'})
		cnf[Pack] = {'side': 'left', 'fill': 'both',
			     'expand': 'yes'}
		self.text = Text(self.frame, cnf)
		self.text['yscrollcommand'] = (self.vbar, 'set')
		self.vbar['command'] = (self.text, 'yview')
		self.insert = self.text.insert
		# XXX should do all Text methods...
		self.pack = self.frame.pack
		self.forget = self.frame.forget
		self.tk = master.tk
	def __str__(self):
		return str(self.frame)
