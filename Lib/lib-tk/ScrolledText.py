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
		cnf[Pack] = {'side': 'left', 'fill': 'both', 'expand': 'yes'}
		self.text = Text(self.frame, cnf)
		self.text['yscrollcommand'] = (self.vbar, 'set')
		self.vbar['command'] = (self.text, 'yview')
		self.insert = self.text.insert
		# XXX should do all Text methods...
		self.pack = self.frame.pack
		self.bind = self.text.bind
		self.forget = self.frame.forget
		self.delete = self.text.delete
		self.insert = self.text.insert
		self.index = self.text.index
		self.get = self.text.get
		self.mark_set = self.text.mark_set
		self.tag_add = self.text.tag_add
		self.tag_delete = self.text.tag_delete
		self.tag_remove = self.text.tag_remove
		self.tag_config = self.text.tag_config
		self.yview = self.text.yview
		self.yview_pickplace = self.text.yview_pickplace
		self.tk = master.tk
	def __str__(self):
		return str(self.frame)
