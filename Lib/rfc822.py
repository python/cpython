# RFC-822 message manipulation class.
#
# XXX This is only a very rough sketch of a full RFC-822 parser;
# in particular the tokenizing of addresses does not adhere to all the
# quoting rules.
#
# Directions for use:
#
# To create a Message object: first open a file, e.g.:
#   fp = open(file, 'r')
# (or use any other legal way of getting an open file object, e.g. use
# sys.stdin or call os.popen()).
# Then pass the open file object to the Message() constructor:
#   m = Message(fp)
#
# To get the text of a particular header there are several methods:
#   str = m.getheader(name)
#   str = m.getrawheader(name)
# where name is the name of the header, e.g. 'Subject'.
# The difference is that getheader() strips the leading and trailing
# whitespace, while getrawheader() doesn't.  Both functions retain
# embedded whitespace (including newlines) exactly as they are
# specified in the header, and leave the case of the text unchanged.
#
# For addresses and address lists there are functions
#   realname, mailaddress = m.getaddr(name) and
#   list = m.getaddrlist(name)
# where the latter returns a list of (realname, mailaddr) tuples.
#
# There is also a method
#   time = m.getdate(name)
# which parses a Date-like field and returns a time-compatible tuple,
# i.e. a tuple such as returned by time.localtime() or accepted by
# time.mktime().
#
# See the class definition for lower level access methods.
#
# There are also some utility functions here.


import regex
import string
import time


class Message:

	# Initialize the class instance and read the headers.
	
	def __init__(self, fp):
		self.fp = fp
		#
		try:
			self.startofheaders = self.fp.tell()
		except IOError:
			self.startofheaders = None
		#
		self.readheaders()
		#
		try:
			self.startofbody = self.fp.tell()
		except IOError:
			self.startofbody = None


	# Rewind the file to the start of the body (if seekable).

	def rewindbody(self):
		self.fp.seek(self.startofbody)


	# Read header lines up to the entirely blank line that
	# terminates them.  The (normally blank) line that ends the
	# headers is skipped, but not included in the returned list.
	# If a non-header line ends the headers, (which is an error),
	# an attempt is made to backspace over it; it is never
	# included in the returned list.
	#
	# The variable self.status is set to the empty string if all
	# went well, otherwise it is an error message.
	# The variable self.headers is a completely uninterpreted list
	# of lines contained in the header (so printing them will
	# reproduce the header exactly as it appears in the file).

	def readheaders(self):
		self.headers = list = []
		self.status = ''
		headerseen = 0
		firstline = 1
		while 1:
			line = self.fp.readline()
			if not line:
				self.status = 'EOF in headers'
				break
			# Skip unix From name time lines
			if firstline and line[:5] == 'From ':
			        continue
			firstline = 0
			if self.islast(line):
				break
			elif headerseen and line[0] in ' \t':
				# It's a continuation line.
				list.append(line)
			elif regex.match('^[!-9;-~]+:', line) >= 0:
				# It's a header line.
				list.append(line)
				headerseen = 1
			else:
				# It's not a header line; stop here.
				if not headerseen:
					self.status = 'No headers'
				else:
					self.status = 'Bad header'
				# Try to undo the read.
				try:
					self.fp.seek(-len(line), 1)
				except IOError:
					self.status = \
						self.status + '; bad seek'
				break


	# Method to determine whether a line is a legal end of
	# RFC-822 headers.  You may override this method if your
	# application wants to bend the rules, e.g. to strip trailing
	# whitespace, or to recognise MH template separators
	# ('--------').  For convenience (e.g. for code reading from
	# sockets) a line consisting of \r\n also matches.

	def islast(self, line):
		return line == '\n' or line == '\r\n'


	# Look through the list of headers and find all lines matching
	# a given header name (and their continuation lines).
	# A list of the lines is returned, without interpretation.
	# If the header does not occur, an empty list is returned.
	# If the header occurs multiple times, all occurrences are
	# returned.  Case is not important in the header name.

	def getallmatchingheaders(self, name):
		name = string.lower(name) + ':'
		n = len(name)
		list = []
		hit = 0
		for line in self.headers:
			if string.lower(line[:n]) == name:
				hit = 1
			elif line[:1] not in string.whitespace:
				hit = 0
			if hit:
				list.append(line)
		return list


	# Similar, but return only the first matching header (and its
	# continuation lines).

	def getfirstmatchingheader(self, name):
		name = string.lower(name) + ':'
		n = len(name)
		list = []
		hit = 0
		for line in self.headers:
			if hit:
				if line[:1] not in string.whitespace:
					break
			elif string.lower(line[:n]) == name:
				hit = 1
			if hit:
				list.append(line)
		return list


	# A higher-level interface to getfirstmatchingheader().
	# Return a string containing the literal text of the header
	# but with the keyword stripped.  All leading, trailing and
	# embedded whitespace is kept in the string, however.
	# Return None if the header does not occur.

	def getrawheader(self, name):
		list = self.getfirstmatchingheader(name)
		if not list:
			return None
		list[0] = list[0][len(name) + 1:]
		return string.joinfields(list, '')


	# Going one step further: also strip leading and trailing
	# whitespace.

	def getheader(self, name):
		text = self.getrawheader(name)
		if text == None:
			return None
		return string.strip(text)


	# Retrieve a single address from a header as a tuple, e.g.
	# ('Guido van Rossum', 'guido@cwi.nl').

	def getaddr(self, name):
		data = self.getheader(name)
		if not data:
			return None, None
		return parseaddr(data)

	# Retrieve a list of addresses from a header, where each
	# address is a tuple as returned by getaddr().

	def getaddrlist(self, name):
		# XXX This function is not really correct.  The split
		# on ',' might fail in the case of commas within
		# quoted strings.
		data = self.getheader(name)
		if not data:
			return []
		data = string.splitfields(data, ',')
		for i in range(len(data)):
			data[i] = parseaddr(data[i])
		return data

	# Retrieve a date field from a header as a tuple compatible
	# with time.mktime().

	def getdate(self, name):
		data = self.getheader(name)
		if not data:
			return None
		return parsedate(data)


	# Access as a dictionary (only finds first header of each type):

	def __len__(self):
		types = {}
		for line in self.headers:
			if line[0] in string.whitespace: continue
			i = string.find(line, ':')
			if i > 0:
				name = string.lower(line[:i])
				types[name] = None
		return len(types)

	def __getitem__(self, name):
		value = self.getheader(name)
		if value is None: raise KeyError, name
		return value

	def has_key(self, name):
		value = self.getheader(name)
		return value is not None

	def keys(self):
		types = {}
		for line in self.headers:
			if line[0] in string.whitespace: continue
			i = string.find(line, ':')
			if i > 0:
				name = line[:i]
				key = string.lower(name)
				types[key] = name
		return types.values()

	def values(self):
		values = []
		for name in self.keys():
			values.append(self[name])
		return values

	def items(self):
		items = []
		for name in self.keys():
			items.append(name, self[name])
		return items



# Utility functions
# -----------------

# XXX Should fix these to be really conformant.
# XXX The inverses of the parse functions may also be useful.


# Remove quotes from a string.

def unquote(str):
	if len(str) > 1:
		if str[0] == '"' and str[-1:] == '"':
			return str[1:-1]
		if str[0] == '<' and str[-1:] == '>':
			return str[1:-1]
	return str


# Parse an address into (name, address) tuple

def parseaddr(address):
	# This is probably not perfect
	address = string.strip(address)
	# Case 1: part of the address is in <xx@xx> form.
	pos = regex.search('<.*>', address)
	if pos >= 0:
		name = address[:pos]
		address = address[pos:]
		length = regex.match('<.*>', address)
		name = name + address[length:]
		address = address[:length]
	else:
		# Case 2: part of the address is in (comment) form
		pos = regex.search('(.*)', address)
		if pos >= 0:
			name = address[pos:]
			address = address[:pos]
			length = regex.match('(.*)', name)
			address = address + name[length:]
			name = name[:length]
		else:
			# Case 3: neither. Only an address
			name = ''
	name = string.strip(name)
	address = string.strip(address)
	if address and address[0] == '<' and address[-1] == '>':
		address = address[1:-1]
	if name and name[0] == '(' and name[-1] == ')':
		name = name[1:-1]
	return name, address


# Parse a date field

_monthnames = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul',
	  'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

def parsedate(data):
	# XXX This still mostly ignores timezone matters at the moment...
	data = string.split(data)
	if data[0][-1] == ',':
		# There's a dayname here. Skip it
		del data[0]
	if len(data) == 4:
		s = data[3]
		i = string.find(s, '+')
		if i > 0:
			data[3:] = [s[:i], s[i+1:]]
		else:
			data.append('') # Dummy tz
	if len(data) < 5:
		return None
	data = data[:5]
	[dd, mm, yy, tm, tz] = data
	if not mm in _monthnames:
		return None
	mm = _monthnames.index(mm)+1
	tm = string.splitfields(tm, ':')
	if len(tm) == 2:
		[thh, tmm] = tm
		tss = '0'
	else:
		[thh, tmm, tss] = tm
	try:
		yy = string.atoi(yy)
		dd = string.atoi(dd)
		thh = string.atoi(thh)
		tmm = string.atoi(tmm)
		tss = string.atoi(tss)
	except string.atoi_error:
		return None
	tuple = (yy, mm, dd, thh, tmm, tss, 0, 0, 0)
	return tuple


# When used as script, run a small test program.
# The first command line argument must be a filename containing one
# message in RFC-822 format.

if __name__ == '__main__':
	import sys
	file = '/ufs/guido/Mail/drafts/,1'
	if sys.argv[1:]: file = sys.argv[1]
	f = open(file, 'r')
	m = Message(f)
	print 'From:', m.getaddr('from')
	print 'To:', m.getaddrlist('to')
	print 'Subject:', m.getheader('subject')
	print 'Date:', m.getheader('date')
	date = m.getdate('date')
	if date:
		print 'ParsedDate:', time.asctime(date)
	else:
		print 'ParsedDate:', None
	m.rewindbody()
	n = 0
	while f.readline():
		n = n + 1
	print 'Lines:', n
	print '-'*70
	print 'len =', len(m)
	if m.has_key('Date'): print 'Date =', m['Date']
	if m.has_key('X-Nonsense'): pass
	print 'keys =', m.keys()
	print 'values =', m.values()
	print 'items =', m.items()
	
