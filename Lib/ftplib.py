# An FTP client class.  Based on RFC 959: File Transfer Protocol
# (FTP), by J. Postel and J. Reynolds

# Changes and improvements suggested by Steve Majewski
# Modified by Jack to work on the mac.


# Example:
#
# >>> from ftplib import FTP
# >>> ftp = FTP('ftp.cwi.nl') # connect to host, default port
# >>> ftp.login() # default, i.e.: user anonymous, passwd user@hostname
# >>> ftp.retrlines('LIST') # list directory contents
# total 43
# d--x--x--x   2 root     root         512 Jul  1 16:50 bin
# d--x--x--x   2 root     root         512 Sep 16  1991 etc
# drwxr-xr-x   2 root     ftp        10752 Sep 16  1991 lost+found
# drwxr-srwt  15 root     ftp        10240 Nov  5 20:43 pub
# >>> ftp.quit()
#
# To download a file, use ftp.retrlines('RETR ' + filename),
# or ftp.retrbinary() with slightly different arguments.
# To upload a file, use ftp.storlines() or ftp.storbinary(), which have
# an open file as argument.
# The download/upload functions first issue appropriate TYPE and PORT
# commands.


import os
import sys
import string

# Import SOCKS module if it exists, else standard socket module socket
try:
    import SOCKS; socket = SOCKS
except ImportError:
    import socket


# Magic number from <socket.h>
MSG_OOB = 0x1				# Process data out of band


# The standard FTP server control port
FTP_PORT = 21


# Exception raised when an error or invalid response is received
error_reply = 'ftplib.error_reply'	# unexpected [123]xx reply
error_temp = 'ftplib.error_temp'	# 4xx errors
error_perm = 'ftplib.error_perm'	# 5xx errors
error_proto = 'ftplib.error_proto'	# response does not begin with [1-5]


# All exceptions (hopefully) that may be raised here and that aren't
# (always) programming errors on our side
all_errors = (error_reply, error_temp, error_perm, error_proto, \
	      socket.error, IOError)


# Line terminators (we always output CRLF, but accept any of CRLF, CR, LF)
CRLF = '\r\n'


# The class itself
class FTP:

	# New initialization method (called by class instantiation)
	# Initialize host to localhost, port to standard ftp port
	# Optional arguments are host (for connect()),
	# and user, passwd, acct (for login())
	def __init__(self, host = '', user = '', passwd = '', acct = ''):
		# Initialize the instance to something mostly harmless
		self.debugging = 0
		self.host = ''
		self.port = FTP_PORT
		self.sock = None
		self.file = None
		self.welcome = None
		if host:
			self.connect(host)
			if user: self.login(user, passwd, acct)

	# Connect to host.  Arguments:
	# - host: hostname to connect to (default previous host)
	# - port: port to connect to (default previous port)
	def connect(self, host = '', port = 0):
		if host: self.host = host
		if port: self.port = port
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect(self.host, self.port)
		self.file = self.sock.makefile('r')
		self.welcome = self.getresp()

	# Get the welcome message from the server
	# (this is read and squirreled away by connect())
	def getwelcome(self):
		if self.debugging:
			print '*welcome*', self.sanitize(self.welcome)
		return self.welcome

	# Set the debugging level.  Argument level means:
	# 0: no debugging output (default)
	# 1: print commands and responses but not body text etc.
	# 2: also print raw lines read and sent before stripping CR/LF
	def set_debuglevel(self, level):
		self.debugging = level
	debug = set_debuglevel

	# Internal: "sanitize" a string for printing
	def sanitize(self, s):
		if s[:5] == 'pass ' or s[:5] == 'PASS ':
			i = len(s)
			while i > 5 and s[i-1] in '\r\n':
				i = i-1
			s = s[:5] + '*'*(i-5) + s[i:]
		return `s`

	# Internal: send one line to the server, appending CRLF
	def putline(self, line):
		line = line + CRLF
		if self.debugging > 1: print '*put*', self.sanitize(line)
		self.sock.send(line)

	# Internal: send one command to the server (through putline())
	def putcmd(self, line):
		if self.debugging: print '*cmd*', self.sanitize(line)
		self.putline(line)

	# Internal: return one line from the server, stripping CRLF.
	# Raise EOFError if the connection is closed
	def getline(self):
		line = self.file.readline()
		if self.debugging > 1:
			print '*get*', self.sanitize(line)
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
		if self.debugging: print '*resp*', self.sanitize(resp)
		self.lastresp = resp[:3]
		c = resp[:1]
		if c == '4':
			raise error_temp, resp
		if c == '5':
			raise error_perm, resp
		if c not in '123':
			raise error_proto, resp
		return resp

	# Expect a response beginning with '2'
	def voidresp(self):
		resp = self.getresp()
		if resp[0] <> '2':
			raise error_reply, resp

	# Abort a file transfer.  Uses out-of-band data.
	# This does not follow the procedure from the RFC to send Telnet
	# IP and Synch; that doesn't seem to work with the servers I've
	# tried.  Instead, just send the ABOR command as OOB data.
	def abort(self):
		line = 'ABOR' + CRLF
		if self.debugging > 1: print '*put urgent*', self.sanitize(line)
		self.sock.send(line, MSG_OOB)
		resp = self.getmultiline()
		if resp[:3] not in ('426', '226'):
			raise error_proto, resp

	# Send a command and return the response
	def sendcmd(self, cmd):
		self.putcmd(cmd)
		return self.getresp()

	# Send a command and expect a response beginning with '2'
	def voidcmd(self, cmd):
		self.putcmd(cmd)
		self.voidresp()

	# Send a PORT command with the current host and the given port number
	def sendport(self, port):
		hostname = socket.gethostname()
		hostaddr = socket.gethostbyname(hostname)
		hbytes = string.splitfields(hostaddr, '.')
		pbytes = [`port/256`, `port%256`]
		bytes = hbytes + pbytes
		cmd = 'PORT ' + string.joinfields(bytes, ',')
		self.voidcmd(cmd)

	# Create a new socket and send a PORT command for it
	def makeport(self):
		global nextport
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		sock.bind(('', 0))
		sock.listen(1)
		host, port = sock.getsockname()
		resp = self.sendport(port)
		return sock

	# Send a port command and a transfer command, accept the connection
	# and return the socket for the connection
	def transfercmd(self, cmd):
		sock = self.makeport()
		resp = self.sendcmd(cmd)
		if resp[0] <> '1':
			raise error_reply, resp
		conn, sockaddr = sock.accept()
		return conn

	# Login, default anonymous
	def login(self, user = '', passwd = '', acct = ''):
		if not user: user = 'anonymous'
		if user == 'anonymous' and passwd in ('', '-'):
			thishost = socket.gethostname()
			# Make sure it is fully qualified
			if not '.' in thishost:
			    thisaddr = socket.gethostbyname(thishost)
			    firstname, names, unused = \
				       socket.gethostbyaddr(thisaddr)
			    names.insert(0, firstname)
			    for name in names:
				    if '.' in name:
					    thishost = name
					    break
			try:
				if os.environ.has_key('LOGNAME'):
					realuser = os.environ['LOGNAME']
				elif os.environ.has_key('USER'):
					realuser = os.environ['USER']
				else:
					realuser = 'anonymous'
			except AttributeError:
				# Not all systems have os.environ....
				realuser = 'anonymous'
			passwd = passwd + realuser + '@' + thishost
		resp = self.sendcmd('USER ' + user)
		if resp[0] == '3': resp = self.sendcmd('PASS ' + passwd)
		if resp[0] == '3': resp = self.sendcmd('ACCT ' + acct)
		if resp[0] <> '2':
			raise error_reply, resp

	# Retrieve data in binary mode.
	# The argument is a RETR command.
	# The callback function is called for each block.
	# This creates a new port for you
	def retrbinary(self, cmd, callback, blocksize):
		self.voidcmd('TYPE I')
		conn = self.transfercmd(cmd)
		while 1:
			data = conn.recv(blocksize)
			if not data:
				break
			callback(data)
		conn.close()
		self.voidresp()

	# Retrieve data in line mode.
	# The argument is a RETR or LIST command.
	# The callback function is called for each line, with trailing
	# CRLF stripped.  This creates a new port for you.
	# print_lines is the default callback 
	def retrlines(self, cmd, callback = None):
		if not callback: callback = print_line
		resp = self.sendcmd('TYPE A')
		conn = self.transfercmd(cmd)
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
		self.voidresp()

	# Store a file in binary mode
	def storbinary(self, cmd, fp, blocksize):
		self.voidcmd('TYPE I')
		conn = self.transfercmd(cmd)
		while 1:
			buf = fp.read(blocksize)
			if not buf: break
			conn.send(buf)
		conn.close()
		self.voidresp()

	# Store a file in line mode
	def storlines(self, cmd, fp):
		self.voidcmd('TYPE A')
		conn = self.transfercmd(cmd)
		while 1:
			buf = fp.readline()
			if not buf: break
			if buf[-2:] <> CRLF:
				if buf[-1] in CRLF: buf = buf[:-1]
				buf = buf + CRLF
			conn.send(buf)
		conn.close()
		self.voidresp()

	# Return a list of files in a given directory (default the current)
	def nlst(self, *args):
		cmd = 'NLST'
		for arg in args:
			cmd = cmd + (' ' + arg)
		files = []
		self.retrlines(cmd, files.append)
		return files

	# List a directory in long form.  By default list current directory
	# to stdout.  Optional last argument is callback function;
	# all non-empty arguments before it are concatenated to the
	# LIST command.  (This *should* only be used for a pathname.)
	def dir(self, *args):
		cmd = 'LIST' 
		func = None
		if args[-1:] and type(args[-1]) != type(''):
			args, func = args[:-1], args[-1]
		for arg in args:
			if arg:
				cmd = cmd + (' ' + arg) 
		self.retrlines(cmd, func)

	# Rename a file
	def rename(self, fromname, toname):
		resp = self.sendcmd('RNFR ' + fromname)
		if resp[0] <> '3':
			raise error_reply, resp
		self.voidcmd('RNTO ' + toname)

	# Change to a directory
	def cwd(self, dirname):
		if dirname == '..':
			try:
				self.voidcmd('CDUP')
				return
			except error_perm, msg:
				if msg[:3] != '500':
					raise error_perm, msg
		cmd = 'CWD ' + dirname
		self.voidcmd(cmd)

	# Retrieve the size of a file
	def size(self, filename):
		resp = self.sendcmd('SIZE ' + filename)
		if resp[:3] == '213':
			return string.atoi(string.strip(resp[3:]))

	# Make a directory, return its full pathname
	def mkd(self, dirname):
		resp = self.sendcmd('MKD ' + dirname)
		return parse257(resp)

	# Return current wording directory
	def pwd(self):
		resp = self.sendcmd('PWD')
		return parse257(resp)

	# Quit, and close the connection
	def quit(self):
		self.voidcmd('QUIT')
		self.close()

	# Close the connection without assuming anything about it
	def close(self):
		self.file.close()
		self.sock.close()
		del self.file, self.sock


# Parse a response type 257
def parse257(resp):
	if resp[:3] <> '257':
		raise error_reply, resp
	if resp[3:5] <> ' "':
		return '' # Not compliant to RFC 959, but UNIX ftpd does this
	dirname = ''
	i = 5
	n = len(resp)
	while i < n:
		c = resp[i]
		i = i+1
		if c == '"':
			if i >= n or resp[i] <> '"':
				break
			i = i+1
		dirname = dirname + c
	return dirname

# Default retrlines callback to print a line
def print_line(line):
	print line


# Test program.
# Usage: ftp [-d] host [-l[dir]] [-d[dir]] [file] ...
def test():
	import marshal
	debugging = 0
	while sys.argv[1] == '-d':
		debugging = debugging+1
		del sys.argv[1]
	host = sys.argv[1]
	ftp = FTP(host)
	ftp.set_debuglevel(debugging)
	ftp.login()
	for file in sys.argv[2:]:
		if file[:2] == '-l':
			ftp.dir(file[2:])
		elif file[:2] == '-d':
			cmd = 'CWD'
			if file[2:]: cmd = cmd + ' ' + file[2:]
			resp = ftp.sendcmd(cmd)
		else:
			ftp.retrbinary('RETR ' + file, \
				       sys.stdout.write, 1024)
	ftp.quit()
