# An FTP client class.  Based on RFC 959: File Transfer Protocol
# (FTP), by J. Postel and J. Reynolds


import os
import sys
import socket
import string


# Default port numbers used by the FTP protocol
FTP_PORT = 21
FTP_DATA_PORT = 20


# Exception raiseds when an error or invalid response is received
error_reply = 'nntp.error_reply'	# unexpected [123]xx reply
error_function = 'nntp.error_function'	# 4xx errors
error_form = 'nntp.error_form'		# 5xx errors
error_protocol = 'nntp.error_protocol'	# response does not begin with [1-5]


# Line terminators (we always output CRLF, but accept any of CRLF, CR, LF)
CRLF = '\r\n'


# Next port to be used by makeport(), with PORT_OFFSET added
nextport = 0
PORT_OFFSET = 40000
PORT_CYCLE = 1000
# XXX This is a nuisance: when using the program several times in a row,
# reusing the port doesn't work and you have to edit the first port
# assignment...


# The class itself
class FTP:

	# Initialize an instance.  Arguments:
	# - host: hostname to connect to
	# - port: port to connect to (default the standard FTP port)
	def init(self, host, *args):
		if len(args) > 1: raise TypeError, 'too many args'
		if args: port = args[0]
		else: port = FTP_PORT
		self.host = host
		self.port = port
		self.debugging = 0
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect(self.host, self.port)
		self.file = self.sock.makefile('r')
		self.welcome = self.getresp()
		return self

	# Get the welcome message from the server
	# (this is read and squirreled away by init())
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

	# Internal: get a response from the server, which may possibly
	# consist of multiple lines.  Return a single string with no
	# trailing CRLF.  If the response consists of multiple lines,
	# these are separated by '\n' characters in the string
	def getmultiline(self):
		line = self.getline()
		if line[3:4] == '-':
			code = line[:3]
			while 1:
				nextline = self.getline()
				line = line + ('\n' + nextline)
				if nextline[:3] == code and \
					nextline[3:4] <> '-':
					break
		return line

	# Internal: get a response from the server.
	# Raise various errors if the response indicates an error
	def getresp(self):
		resp = self.getmultiline()
		if self.debugging: print '*resp*', `resp`
		self.lastresp = resp[:3]
		c = resp[:1]
		if c == '4':
			raise error_function, resp
		if c == '5':
			raise error_form, resp
		if c not in '123':
			raise error_protocol, resp
		return resp

	# Send a command and return the response
	def sendcmd(self, cmd):
		self.putcmd(cmd)
		return self.getresp()

	# Send a PORT command with the current host and the given port number
	def sendport(self, port):
		hostname = socket.gethostname()
		hostaddr = socket.gethostbyname(hostname)
		hbytes = string.splitfields(hostaddr, '.')
		pbytes = [`port/256`, `port%256`]
		bytes = hbytes + pbytes
		cmd = 'PORT ' + string.joinfields(bytes, ',')
		resp = self.sendcmd(cmd)
		if resp[:3] <> '200':
			raise error_reply, resp

	# Create a new socket and send a PORT command for it
	def makeport(self):
		global nextport
		port = nextport + PORT_OFFSET
		nextport = (nextport + 1) % PORT_CYCLE
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		sock.bind('', port)
		sock.listen(0)
		resp = self.sendport(port)
		return sock

	# Retrieve data in binary mode.  (You must set the mode first.)
	# The argument is a RETR command.
	# The callback function is called for each block.
	# This creates a new port for you
	def retrbinary(self, cmd, callback, blocksize):
		sock = self.makeport()
		resp = self.sendcmd(cmd)
		if resp[0] <> '1':
			raise error_reply, resp
		conn, host = sock.accept()
		sock.close()
		while 1:
			data = conn.recv(blocksize)
			if not data:
				break
			callback(data)
		conn.close()
		resp = self.getresp()
		if resp[0] <> '2':
			raise error_reply, resp

	# Retrieve data in line mode.  (You must set the mode first.)
	# The argument is a RETR or LIST command.
	# The callback function is called for each line, with trailing
	# CRLF stripped.  This creates a new port for you
	def retrlines(self, cmd, callback):
		sock = self.makeport()
		resp = self.sendcmd(cmd)
		if resp[0] <> '1':
			raise error_reply, resp
		conn, host = sock.accept()
		sock.close()
		fp = conn.makefile('r')
		while 1:
			line = fp.readline()
			if not line:
				break
			if line[-2:] == CRLF:
				line = line[:-2]
			elif line[:-1] == '\n':
				line = line[:-1]
			callback(line)
		fp.close()
		conn.close()
		resp = self.getresp()
		if resp[0] <> '2':
			raise error_reply, resp

	# Login as user anonymous with given passwd (default user@thishost)
	def anonymouslogin(self, *args):
		resp = self.sendcmd('USER anonymous')
		if resp[0] == '3':
			if args:
				passwd = args[0]
			else:
				thishost = socket.gethostname()
				if os.environ.has_key('LOGNAME'):
					user = os.environ['LOGNAME']
				elif os.environ.has_key('USER'):
					user = os.environ['USER']
				else:
					user = 'anonymous'
				passwd = user + '@' + thishost
			resp = self.sendcmd('PASS ' + passwd)
		if resp[0] <> '2':
			raise error_reply, resp

	# Quit, and close the connection
	def quit(self):
		resp = self.sendcmd('QUIT')
		if resp[0] <> '2':
			raise error_reply, resp
		self.file.close()
		self.sock.close()


# Test program.
# Usage: ftp [-d] host [-l[dir]] [-d[dir]] [file] ...
def test():
	import marshal
	global nextport
	try:
		nextport = marshal.load(open('.@nextport', 'r'))
	except IOError:
		pass
	try:
		debugging = 0
		while sys.argv[1] == '-d':
			debugging = debugging+1
			del sys.argv[1]
		host = sys.argv[1]
		ftp = FTP().init(host)
		ftp.debug(debugging)
		ftp.anonymouslogin()
		def writeln(line): print line
		for file in sys.argv[2:]:
			if file[:2] == '-l':
				cmd = 'LIST'
				if file[2:]: cmd = cmd + ' ' + file[2:]
				ftp.retrlines(cmd, writeln)
			elif file[:2] == '-d':
				cmd = 'CWD'
				if file[2:]: cmd = cmd + ' ' + file[2:]
				resp = ftp.sendcmd(cmd)
			else:
				ftp.retrbinary('RETR ' + file, \
					       sys.stdout.write, 1024)
		ftp.quit()
	finally:
		marshal.dump(nextport, open('.@nextport', 'w'))
