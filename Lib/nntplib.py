# An NNTP client class.  Based on RFC 977: Network News Transfer
# Protocol, by Brian Kantor and Phil Lapsley.


# Example:
#
# >>> from nntp import NNTP
# >>> s = NNTP().init('charon')
# >>> resp, count, first, last, name = s.group('nlnet.misc')
# >>> print 'Group', name, 'has', count, 'articles, range', first, 'to', last
# Group nlnet.misc has 525 articles, range 6960 to 7485
# >>> resp, subs = s.xhdr('subject', first + '-' + last)
# >>> resp = s.quit()
# >>>
#
# Here 'resp' is the server response line.
# Error responses are turned into exceptions.
#
# To post an article from a file:
# >>> f = open(filename, 'r') # file containing article, including header
# >>> resp = s.post(f)
# >>>
#
# For descriptions of all methods, read the comments in the code below.
# Note that all arguments and return values representing article numbers
# are strings, not numbers, since they are rarely used for calculations.


# Imports
import regex
import socket
import string


# Exception raiseds when an error or invalid response is received

error_reply = 'nntp.error_reply'	# unexpected [123]xx reply
error_function = 'nntp.error_function'	# 4xx errors
error_form = 'nntp.error_form'		# 5xx errors
error_protocol = 'nntp.error_protocol'	# response does not begin with [1-5]


# Standard port used by NNTP servers
NNTP_PORT = 119


# Response numbers that are followed by additional text (e.g. article)
LONGRESP = ['100', '215', '220', '221', '222', '230', '231']


# Line terminators (we always output CRLF, but accept any of CRLF, CR, LF)
CRLF = '\r\n'


# The class itself

class NNTP:

	# Initialize an instance.  Arguments:
	# - host: hostname to connect to
	# - port: port to connect to (default the standard NNTP port)

	def init(self, host, *args):
		if len(args) > 1: raise TypeError, 'too many args'
		if args: port = args[0]
		else: port = NNTP_PORT
		self.host = host
		self.port = port
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect(self.host, self.port)
		self.file = self.sock.makefile('r')
		self.debugging = 0
		self.welcome = self.getresp()
		return self

	# Get the welcome message from the server
	# (this is read and squirreled away by init()).
	# If the response code is 200, posting is allowed;
	# if it 201, posting is not allowed

	def getwelcome(self):
		if self.debugging: print '*welcome*', `self.welcome`
		return self.welcome

	# Set the debugging level.  Argument level means:
	# 0: no debugging output (default)
	# 1: print commands and responses but not body text etc.
	# 2: also print raw lines read and sent before stripping CR/LF

	def debug(self, level):
		self.debugging = level

	# Internal: send one line to the server, appending CRLF
	def putline(self, line):
		line = line + CRLF
		if self.debugging > 1: print '*put*', `line`
		self.sock.send(line)

	# Internal: send one command to the server (through putline())
	def putcmd(self, line):
		if self.debugging: print '*cmd*', `line`
		self.putline(line)

	# Internal: return one line from the server, stripping CRLF.
	# Raise EOFError if the connection is closed
	def getline(self):
		line = self.file.readline()
		if self.debugging > 1:
			print '*get*', `line`
		if not line: raise EOFError
		if line[-2:] == CRLF: line = line[:-2]
		elif line[-1:] in CRLF: line = line[:-1]
		return line

	# Internal: get a response from the server.
	# Raise various errors if the response indicates an error
	def getresp(self):
		resp = self.getline()
		if self.debugging: print '*resp*', `resp`
		c = resp[:1]
		if c == '4':
			raise error_function, resp
		if c == '5':
			raise error_form, resp
		if c not in '123':
			raise error_protocol, resp
		return resp

	# Internal: get a response plus following text from the server.
	# Raise various errors if the response indicates an error
	def getlongresp(self):
		resp = self.getresp()
		if resp[:3] not in LONGRESP:
			raise error_reply, resp
		list = []
		while 1:
			line = self.getline()
			if line == '.':
				break
			list.append(line)
		return resp, list

	# Internal: send a command and get the response
	def shortcmd(self, line):
		self.putcmd(line)
		return self.getresp()

	# Internal: send a command and get the response plus following text
	def longcmd(self, line):
		self.putcmd(line)
		return self.getlongresp()

	# Process a NEWGROUPS command.  Arguments:
	# - date: string 'yymmdd' indicating the date
	# - time: string 'hhmmss' indicating the time
	# Return:
	# - resp: server response if succesful
	# - list: list of newsgroup names

	def newgroups(self, date, time):
		return self.longcmd('NEWGROUPS ' + date + ' ' + time)

	# Process a NEWNEWS command.  Arguments:
	# - group: group name or '*'
	# - date: string 'yymmdd' indicating the date
	# - time: string 'hhmmss' indicating the time
	# Return:
	# - resp: server response if succesful
	# - list: list of article ids

	def newnews(self, group, date, time):
		cmd = 'NEWNEWS ' + group + ' ' + date + ' ' + time
		return self.longcmd(cmd)

	# Process a LIST command.  Return:
	# - resp: server response if succesful
	# - list: list of (group, first, last, flag) (strings)

	def list(self):
		resp, list = self.longcmd('LIST')
		for i in range(len(list)):
			# Parse lines into "group first last flag"
			list[i] = string.split(list[i])
		return resp, list

	# Process a GROUP command.  Argument:
	# - group: the group name
	# Returns:
	# - resp: server response if succesful
	# - count: number of articles (string)
	# - first: first article number (string)
	# - last: last article number (string)
	# - name: the group name

	def group(self, name):
		resp = self.shortcmd('GROUP ' + name)
		if resp[:3] <> '211':
			raise error_reply, resp
		words = string.split(resp)
		count = first = last = 0
		n = len(words)
		if n > 1:
			count = words[1]
			if n > 2:
				first = words[2]
				if n > 3:
					last = words[3]
					if n > 4:
						name = string.lower(words[4])
		return resp, count, first, last, name

	# Process a HELP command.  Returns:
	# - resp: server response if succesful
	# - list: list of strings

	def help(self):
		return self.longcmd('HELP')

	# Internal: parse the response of a STAT, NEXT or LAST command
	def statparse(self, resp):
		if resp[:2] <> '22':
			raise error_reply, resp
		words = string.split(resp)
		nr = 0
		id = ''
		n = len(words)
		if n > 1:
			nr = words[1]
			if n > 2:
				id = string.lower(words[2])
		return resp, nr, id

	# Internal: process a STAT, NEXT or LAST command
	def statcmd(self, line):
		resp = self.shortcmd(line)
		return self.statparse(resp)

	# Process a STAT command.  Argument:
	# - id: article number or message id
	# Returns:
	# - resp: server response if succesful
	# - nr:   the article number
	# - id:   the article id

	def stat(self, id):
		return self.statcmd('STAT ' + id)

	# Process a NEXT command.  No arguments.  Return as for STAT

	def next(self):
		return self.statcmd('NEXT')

	# Process a LAST command.  No arguments.  Return as for STAT

	def last(self):
		return self.statcmd('LAST')

	# Internal: process a HEAD, BODY or ARTICLE command
	def artcmd(self, line):
		resp, list = self.longcmd(line)
		resp, nr, id = self.statparse(resp)
		return resp, nr, id, list

	# Process a HEAD command.  Argument:
	# - id: article number or message id
	# Returns:
	# - resp: server response if succesful
	# - list: the lines of the article's header

	def head(self, id):
		return self.artcmd('HEAD ' + id)

	# Process a BODY command.  Argument:
	# - id: article number or message id
	# Returns:
	# - resp: server response if succesful
	# - list: the lines of the article's body

	def body(self, id):
		return self.artcmd('BODY ' + id)

	# Process an ARTICLE command.  Argument:
	# - id: article number or message id
	# Returns:
	# - resp: server response if succesful
	# - list: the lines of the article

	def article(self, id):
		return self.artcmd('ARTICLE ' + id)

	# Process a SLAVE command.  Returns:
	# - resp: server response if succesful

	def slave(self):
		return self.shortcmd('SLAVE')

	# Process an XHDR command (optional server extension).  Arguments:
	# - hdr: the header type (e.g. 'subject')
	# - str: an article nr, a message id, or a range nr1-nr2
	# Returns:
	# - resp: server response if succesful
	# - list: list of (nr, value) strings

	def xhdr(self, hdr, str):
		resp, lines = self.longcmd('XHDR ' + hdr + ' ' + str)
		for i in range(len(lines)):
			line = lines[i]
			n = regex.match('^[0-9]+', line)
			nr = line[:n]
			if n < len(line) and line[n] == ' ': n = n+1
			lines[i] = (nr, line[n:])
		return resp, lines

	# Process a POST command.  Arguments:
	# - f: file containing the article
	# Returns:
	# - resp: server response if succesful

	def post(self, f):
		resp = self.shortcmd('POST')
		# Raises error_??? if posting is not allowed
		if resp[0] <> '3':
			raise error_reply, resp
		while 1:
			line = f.readline()
			if not line:
				break
			if line[-1] == '\n':
				line = line[:-1]
			if line == '.':
				line = '..'
			self.putline(line)
		self.putline('.')
		return self.getresp()

	# Process an IHAVE command.  Arguments:
	# - id: message-id of the article
	# - f:  file containing the article
	# Returns:
	# - resp: server response if succesful
	# Note that if the server refuses the article an exception is raised

	def ihave(self, id, f):
		resp = self.shortcmd('IHAVE ' + id)
		# Raises error_function if the server already has it
		if resp[0] <> '3':
			raise error_reply, resp
		while 1:
			line = f.readline()
			if not line:
				break
			if line[-1] == '\n':
				line = line[:-1]
			if line == '.':
				line = '..'
			self.putline(line)
		self.putline('.')
		return self.getresp()

	 # Process a QUIT command and close the socket.  Returns:
	 # - resp: server response if succesful

	def quit(self):
		resp = self.shortcmd('QUIT')
		self.file.close()
		self.sock.close()
		del self.file, self.sock
		return resp
