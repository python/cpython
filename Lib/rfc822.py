# RFC-822 message manipulation class.
#
# XXX This is only a very rough sketch of a full RFC-822 parser;
# additional methods are needed to parse addresses and dates, and to
# tokenize lines according to various other syntax rules.
#
# Directions for use:
#
# To create a Message object: first open a file, e.g.:
#   fp = open(file, 'r')
# (or use any other legal way of getting an open file object, e.g. use
# sys.stdin or call os.popen()).
# Then pass the open file object to the init() method of Message:
#   m = Message().init(fp)
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
# See the class definition for lower level access methods.
#
# There are also some utility functions here.


import regex
import string


class Message:

	# Initialize the class instance and read the headers.
	
	def init(self, fp):
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
		#
		return self


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
		while 1:
			line = self.fp.readline()
			if not line:
				self.status = 'EOF in headers'
				break
			if self.islast(line):
				break
			elif headerseen and line[0] in ' \t':
				# It's a continuation line.
				list.append(line)
			elif regex.match('^[!-9;-~]+:', line):
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
	# application wants to bend the rules, e.g. to accept lines
	# ending in '\r\n', to strip trailing whitespace, or to
	# recognise MH template separators ('--------'). 

	def islast(self, line):
		return line == '\n'


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
			if string.lower(line[:n]) == name:
				hit = 1
			elif line[:1] not in string.whitespace:
				if hit:
					break
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


	# XXX The next step would be to define self.getaddr(name)
	# and self.getaddrlist(name) which would parse a header
	# consisting of a single mail address and a number of mail
	# addresses, respectively.  Lower level functions would be
	# parseaddr(string) and parseaddrlist(string).

	# XXX Similar, there would be a function self.getdate(name) to
	# return a date in canonical form (perhaps a number compatible
	# to time.time()) and a function parsedate(string).

	# XXX The inverses of the parse functions may also be useful.




# Utility functions
# -----------------


# Remove quotes from a string.
# XXX Should fix this to be really conformant.

def unquote(str):
	if len(str) > 1:
		if str[0] == '"' and str[-1:] == '"':
			return str[1:-1]
		if str[0] == '<' and str[-1:] == '>':
			return str[1:-1]
	return str
