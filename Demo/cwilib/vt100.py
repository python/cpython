# VT100 terminal emulator.
# This is incomplete and slow, but will do for now...
# It shouldn't be difficult to extend it to be a more-or-less complete
# VT100 emulator.  And little bit of profiling could go a long way...

from array import array
import regex
import string

# Tunable parameters
DEBUGLEVEL = 1

# Symbolic constants
ESC = '\033'


# VT100 emulation class

class VT100:

	def __init__(self):
		self.debuglevel = DEBUGLEVEL
		# Unchangeable parameters (for now)
		self.width = 80
		self.height = 24
		self.blankline = array('c', ' '*self.width)
		self.blankattr = array('b', '\0'*self.width)
		# Set mutable display state
		self.reset()
		# Set parser state
		self.unfinished = ''
		# Set screen recognition state
		self.reset_recognizer()

	def msg(self, msg, *args):
		if self.debuglevel > 0:
			print 'VT100:', msg%args

	def set_debuglevel(self, debuglevel):
		self.debuglevel = debuglevel

	def reset(self):
		self.lines = []
		self.attrs = []
		self.fill_bottom()
		self.x = 0
		self.y = 0
		self.curattrs = []

	def show(self):
		lineno = 0
		for line in self.lines:
			lineno = lineno + 1
			i = len(line)
			while i > 0 and line[i-1] == ' ': i = i-1
			print line[:i]
			print 'CURSOR:', self.x, self.y

	def fill_bottom(self):
		while len(self.lines) < self.height:
			self.lines.append(self.blankline[:])
			self.attrs.append(self.blankattr[:])

	def fill_top(self):
		while len(self.lines) < self.height:
			self.lines.insert(0, self.blankline[:])
			self.attrs.insert(0, self.blankattr[:])

	def clear_all(self):
		self.lines = []
		self.attrs = []
		self.fill_bottom()

	def clear_below(self):
		del self.lines[self.y:]
		del self.attrs[self.y:]
		self.fill_bottom()

	def clear_above(self):
		del self.lines[:self.y]
		del self.attrs[:self.y]
		self.fill_top()

	def send(self, buffer):
		self.unfinished = self.unfinished + buffer
		i = 0
		n = len(self.unfinished)
		while i < n:
			c = self.unfinished[i]
			i = i+1
			if c != ESC:
				self.add_char(c)
				continue
			if i >= n:
				i = i-1
				break
			c = self.unfinished[i]
			i = i+1
			if c == 'c':
				self.reset()
				continue
			if c <> '[':
				self.msg('unrecognized: ESC %s', `c`)
				continue
			argstr = ''
			while i < n:
				c = self.unfinished[i]
				i = i+1
				if c not in '0123456789;':
					break
				argstr = argstr + c
			else:
				i = i - len(argstr)
				break
##			self.msg('found ESC [ %s %s' % (`argstr`, `c`))
			args = string.splitfields(argstr, ';')
			for j in range(len(args)):
				s = args[j]
				while s[:1] == '0': s = s[1:]
				if s: args[j] = eval(s)
				else: args[j] = 0
			p1 = p2 = 0
			if args: p1 = args[0]
			if args[1:]: p2 = args[1]
			if c in '@ABCDH':
				if not p1: p1 = 1
			if c in 'H':
				if not p2: p2 = 1
			if c == '@':
				for j in range(p1):
					self.add_char(' ')
			elif c == 'A':
				self.move_by(0, -p1)
			elif c == 'B':
				self.move_by(0, p1)
			elif c == 'C':
				self.move_by(p1, 0)
			elif c == 'D':
				self.move_by(-p1, 0)
			elif c == 'H':
				self.move_to(p2-1, p1-1)
			elif c == 'J':
				if p1 == 0: self.clear_above()
				elif p1 == 1: self.clear_below()
				elif p1 == 2: self.clear_all()
				else: self.msg('weird ESC [ %d J', p1)
			elif c == 'K':
				if p1 == 0: self.erase_right()
				elif p1 == 1: self.erase_left()
				elif p1 == 2: self.erase_line()
				else: self.msg('weird ESC [ %d K', p1)
			elif c == 'm':
				if p1 == 0:
					self.curattrs = []
				else:
					if p1 not in self.curattrs:
						self.curattrs.append(p1)
						self.curattrs.sort()
			else:
				self.msg('unrecognized: ESC [ %s', `argstr+c`)
		self.unfinished = self.unfinished[i:]

	def add_char(self, c):
		if c == '\r':
			self.move_to(0, self.y)
			return
		if c in '\n\f\v':
			self.move_to(self.x, self.y + 1)
			if self.y >= self.height:
				self.scroll_up(1)
				self.move_to(self.x, self.height - 1)
			return
		if c == '\b':
			self.move_by(-1, 0)
			return
		if c == '\a':
			self.msg('BELL')
			return
		if c == '\t':
			self.move_to((self.x+8)/8*8, self.y)
			return
		if c == '\0':
			return
		if c < ' ' or c > '~':
			self.msg('ignored control char: %s', `c`)
			return
		if self.x >= self.width:
			self.move_to(0, self.y + 1)
		if self.y >= self.height:
			self.scroll_up(1)
			self.move_to(self.x, self.height - 1)
		self.lines[self.y][self.x] = c
		if self.curattrs:
			self.attrs[self.y][self.x] = max(self.curattrs)
		else:
			self.attrs[self.y][self.x] = 0
		self.move_by(1, 0)

	def move_to(self, x, y):
		self.x = min(max(0, x), self.width)
		self.y = min(max(0, y), self.height)

	def move_by(self, dx, dy):
		self.move_to(self.x + dx, self.y + dy)

	def scroll_up(self, nlines):
		del self.lines[:max(0, nlines)]
		del self.attrs[:max(0, nlines)]
		self.fill_bottom()

	def scroll_down(self, nlines):
		del self.lines[-max(0, nlines):]
		del self.attrs[-max(0, nlines):]
		self.fill_top()

	def erase_left(self):
		x = min(self.width-1, x)
		y = min(self.height-1, y)
		self.lines[y][:x] = self.blankline[:x]
		self.attrs[y][:x] = self.blankattr[:x]

	def erase_right(self):
		x = min(self.width-1, x)
		y = min(self.height-1, y)
		self.lines[y][x:] = self.blankline[x:]
		self.attrs[y][x:] = self.blankattr[x:]

	def erase_line(self):
		self.lines[y][:] = self.blankline
		self.attrs[y][:] = self.blankattr

	# The following routines help automating the recognition of
	# standard screens.  A standard screen is characterized by
	# a number of fields.  A field is part of a line,
	# characterized by a (lineno, begin, end) tuple;
	# e.g. the first 10 characters of the second line are
	# specified by the tuple (1, 0, 10).  Fields can be:
	# - regex: desired contents given by a regular expression,
	# - extract: can be extracted,
	# - cursor: screen is only valid if cursor in field,
	# - copy: identical to another screen (position is ignored).
	# A screen is defined as a dictionary full of fields.  Screens
	# also have names and are placed in a dictionary.

	def reset_recognizer(self):
		self.screens = {}

	def define_screen(self, screenname, fields):
		fieldscopy = {}
		# Check if the fields make sense
		for fieldname in fields.keys():
			field = fields[fieldname]
			ftype, lineno, begin, end, extra = field
			if ftype in ('match', 'search'):
				extra = regex.compile(extra)
			elif ftype == 'extract':
				extra = None
			elif ftype == 'cursor':
				extra = None
			elif ftype == 'copy':
				if not self.screens.has_key(extra):
					raise ValueError, 'bad copy ref'
			else:
				raise ValueError, 'bad ftype: %s' % `ftype`
			fieldscopy[fieldname] = (
				  ftype, lineno, begin, end, extra)
		self.screens[screenname] = fieldscopy

	def which_screens(self):
		self.busy = []
		self.okay = []
		self.fail = []
		for name in self.screens.keys():
			ok = self.match_screen(name)
		return self.okay[:]

	def match_screen(self, name):
		if name in self.busy: raise RuntimeError, 'recursive match'
		if name in self.okay: return 1
		if name in self.fail: return 0
		self.busy.append(name)
		fields = self.screens[name]
		ok = 0
		for key in fields.keys():
			field = fields[key]
			ftype, lineno, begin, end, extra = field
			if ftype == 'copy':
				if not self.match_screen(extra): break
			elif ftype == 'search':
				text = self.lines[lineno][begin:end].tostring()
				if extra.search(text) < 0:
					break
			elif ftype == 'match':
				text = self.lines[lineno][begin:end].tostring()
				if extra.match(text) < 0:
					break
			elif ftype == 'cursor':
				if self.x != lineno or not \
					  begin <= self.y < end:
					break
		else:
			ok = 1
		if ok:
			self.okay.append(name)
		else:
			self.fail.append(name)
		self.busy.remove(name)
		return ok

	def extract_field(self, screenname, fieldname):
		ftype, lineno, begin, end, extra = \
			  self.screens[screenname][fieldname]
		return stripright(self.lines[lineno][begin:end].tostring())

	def extract_rect(self, left, top, right, bottom):
		lines = []
		for i in range(top, bottom):
			lines.append(stripright(self.lines[i][left:right])
				  .tostring())
		return lines


def stripright(line):
	i = len(line)
	while i > 0 and line[i-1] in string.whitespace: i = i-1
	return line[:i]
