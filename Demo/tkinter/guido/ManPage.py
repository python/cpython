# Widget to display a man page

import regex
from Tkinter import *
from Tkinter import _tkinter
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

	# Initialize instance
	def __init__(self, master=None, **cnf):
		# Initialize base class
		apply(ScrolledText.__init__, (self, master), cnf)

		# Define tags for formatting styles
		self.tag_config('X', underline=1)
		self.tag_config('!', font=BOLDFONT)
		self.tag_config('_', font=ITALICFONT)

		# Set state to idle
		self.fp = None
		self.lineno = 0

	# Test whether we are busy parsing a file
	def busy(self):
		return self.fp != None

	# Ensure we're not busy
	def kill(self):
		if self.busy():
			self._endparser()

	# Parse a file, in the background
	def asyncparsefile(self, fp):
		self._startparser(fp)
		self.tk.createfilehandler(fp, _tkinter.READABLE,
					  self._filehandler)

	parsefile = asyncparsefile	# Alias

	# I/O handler used by background parsing
	def _filehandler(self, fp, mask):
		nextline = self.fp.readline()
		if not nextline:
			self._endparser()
			return
		self._parseline(nextline)

	# Parse a file, now (cannot be aborted)
	def syncparsefile(self, fp):
		from select import select
		def avail(fp=fp, tout=0.0, select=select):
			return select([fp], [], [], tout)[0]
		height = self.getint(self['height'])
		self._startparser(fp)
		while 1:
			nextline = fp.readline()
			if not nextline:
				break
			self._parseline(nextline)
		self._endparser()

	# Initialize parsing from a particular file -- must not be busy
	def _startparser(self, fp):
		if self.busy():
			raise RuntimeError, 'startparser: still busy'
		fp.fileno()		# Test for file-ness
		self.fp = fp
		self.lineno = 0
		self.ok = 0
		self.empty = 0
		self.buffer = None
		savestate = self['state']
		self['state'] = NORMAL
		self.delete('1.0', END)
		self['state'] = savestate

	# End parsing -- must be busy, need not be at EOF
	def _endparser(self):
		if not self.busy():
			raise RuntimeError, 'endparser: not busy'
		if self.buffer:
			self._parseline('')
		try:
			self.tk.deletefilehandler(self.fp)
		except TclError, msg:
			pass
		self.fp.close()
		self.fp = None
		del self.ok, self.empty, self.buffer

	# Parse a single line
	def _parseline(self, nextline):
		if not self.buffer:
			# Save this line -- we need one line read-ahead
			self.buffer = nextline
			return
		if emptyprog.match(self.buffer) >= 0:
			# Buffered line was empty -- set a flag
			self.empty = 1
			self.buffer = nextline
			return
		textline = self.buffer
		if ulprog.match(nextline) >= 0:
			# Next line is properties for buffered line
			propline = nextline
			self.buffer = None
		else:
			# Next line is read-ahead
			propline = None
			self.buffer = nextline
		if not self.ok:
			# First non blank line after footer must be header
			# -- skip that too
			self.ok = 1
			self.empty = 0
			return
		if footerprog.match(textline) >= 0:
			# Footer -- start skipping until next non-blank line
			self.ok = 0
			self.empty = 0
			return
		savestate = self['state']
		self['state'] = NORMAL
		if TkVersion >= 4.0:
			self.mark_set('insert', 'end-1c')
		else:
			self.mark_set('insert', END)
		if self.empty:
			# One or more previous lines were empty
			# -- insert one blank line in the text
			self._insert_prop('\n')
			self.lineno = self.lineno + 1
			self.empty = 0
		if not propline:
			# No properties
			self._insert_prop(textline)
		else:
			# Search for properties
			p = ''
			j = 0
			for i in range(min(len(propline), len(textline))):
				if propline[i] != p:
					if j < i:
					    self._insert_prop(textline[j:i], p)
					    j = i
					p = propline[i]
			self._insert_prop(textline[j:])
		self.lineno = self.lineno + 1
		self['state'] = savestate

	# Insert a string at the end, with at most one property (tag)
	def _insert_prop(self, str, prop = ' '):
		here = self.index(AtInsert())
		self.insert(AtInsert(), str)
		if TkVersion <= 4.0:
			tags = self.tag_names(here)
			for tag in tags:
				self.tag_remove(tag, here, AtInsert())
		if prop != ' ':
			self.tag_add(prop, here, AtInsert())

# Readonly Man Page class -- disables editing, otherwise the same
class ReadonlyManPage(EditableManPage):

	# Initialize instance
	def __init__(self, master=None, **cnf):
		cnf['state'] = DISABLED
		apply(EditableManPage.__init__, (self, master), cnf)

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
	manpage = ManPage(root, relief=SUNKEN, borderwidth=2)
	manpage.pack(expand=1, fill=BOTH)
	if formatted:
		fp = open(name, 'r')
	else:
		fp = os.popen('nroff -man %s | ul -i' % name, 'r')
	manpage.parsefile(fp)
	root.mainloop()

# Run the test program when called as a script
if __name__ == '__main__':
	test()
