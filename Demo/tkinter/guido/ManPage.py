# Widget to display a man page

import regex
from Tkinter import *
from ScrolledText import ScrolledText

# XXX These fonts may have to be changed to match your system
BOLDFONT = '*-Courier-Bold-R-Normal-*-120-*'
ITALICFONT = '*-Courier-Medium-O-Normal-*-120-*'

# XXX Recognizing footers is system dependent
# (This one works for IRIX 5.2 and Solaris 2.2)
footerprog = regex.compile(
	'^     Page [1-9][0-9]*[ \t]+\|^.*Last change:.*[1-9][0-9]*\n')
emptyprog = regex.compile('^[ \t]*\n')
ulprog = regex.compile('^[ \t]*[Xv!_][Xv!_ \t]*\n')

# Basic Man Page class -- does not disable editing
class EditableManPage(ScrolledText):

	def __init__(self, master=None, cnf={}):
		# Initialize base class
		ScrolledText.__init__(self, master, cnf)

		# Define tags for formatting styles
		self.tag_config('bold', {'font': BOLDFONT})
		self.tag_config('italic', {'font': ITALICFONT})
		self.tag_config('underline', {'underline': 1})

		# Create mapping from characters to tags
		self.tagmap = {
			'X': 'bold',
			'_': 'underline',
			'!': 'italic',
			}

	# Parse nroff output piped through ul -i and append it to the
	# text widget
	def parsefile(self, fp):
		save_cursor = self['cursor']
		self['cursor'] = 'watch'
		self.update()
		ok = 0
		empty = 0
		nextline = None
		while 1:
			if nextline:
				line = nextline
				nextline = None
			else:
				line = fp.readline()
				if not line:
					break
			if emptyprog.match(line) >= 0:
				empty = 1
				continue
			nextline = fp.readline()
			if nextline and ulprog.match(nextline) >= 0:
				propline = nextline
				nextline = None
			else:
				propline = ''
			if not ok:
				ok = 1
				empty = 0
				continue
			if footerprog.match(line) >= 0:
				ok = 0
				empty = 0
				continue
			if empty:
				self.insert_prop('\n')
				empty = 0
			p = ''
			j = 0
			for i in range(min(len(propline), len(line))):
				if propline[i] != p:
					if j < i:
						self.insert_prop(line[j:i], p)
						j = i
					p = propline[i]
			self.insert_prop(line[j:])
		self['cursor'] = save_cursor

	def insert_prop(self, str, prop = ' '):
		here = self.index(AtInsert())
		self.insert(AtInsert(), str)
		for tag in self.tagmap.values():
			self.tag_remove(tag, here, AtInsert())
		if self.tagmap.has_key(prop):
			self.tag_add(self.tagmap[prop], here, AtInsert())

# Readonly Man Page class -- disables editing, otherwise the same
class ReadonlyManPage(EditableManPage):

	def __init__(self, master=None, cnf={}):
		# Initialize base class
		EditableManPage.__init__(self, master, cnf)

		# Make the text readonly
		self.bind('<Any-KeyPress>', self.modify_cb)
		self.bind('<Return>', self.modify_cb)
		self.bind('<BackSpace>', self.modify_cb)
		self.bind('<Delete>', self.modify_cb)
		self.bind('<Control-h>', self.modify_cb)
		self.bind('<Control-d>', self.modify_cb)
		self.bind('<Control-v>', self.modify_cb)

	def modify_cb(self, e):
		pass

# Alias
ManPage = ReadonlyManPage

# Test program.
# usage: ManPage [manpage]; or ManPage [-f] file
# -f means that the file is nroff -man output run through ul -i
def test():
	import os
	import sys
	# XXX This directory may be different on your system
	MANDIR = '/usr/local/man/mann'
	DEFAULTPAGE = 'Tcl'
	formatted = 0
	if sys.argv[1:] and sys.argv[1] == '-f':
		formatted = 1
		del sys.argv[1]
	if sys.argv[1:]:
		name = sys.argv[1]
	else:
		name = DEFAULTPAGE
	if not formatted:
		if name[-2:-1] != '.':
			name = name + '.n'
		name = os.path.join(MANDIR, name)
	root = Tk()
	root.minsize(1, 1)
	manpage = ManPage(root, {'relief': 'sunken', 'bd': 2,
				 Pack: {'expand': 1, 'fill': 'both'}})
	if formatted:
		fp = open(name, 'r')
	else:
		fp = os.popen('nroff -man %s | ul -i' % name, 'r')
	manpage.parsefile(fp)
	root.mainloop()

# Run the test program when called as a script
if __name__ == '__main__':
	test()
