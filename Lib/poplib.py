"""A POP3 client class.  Based on the J. Myers POP3 draft, Jan. 96

Author: David Ascher <david_ascher@brown.edu> [heavily stealing from
nntplib.py]

"""

__version__ = "0.01a - Feb 1, 1996 (with formatting changes by GvR)"

# Example (see the test function at the end of this file)

TESTSERVER = "localhost"
TESTACCOUNT = "test"
TESTPASSWORD = "_passwd_"

# Imports

from types import StringType
import regex
import socket
import string

# Exception raised when an error or invalid response is received:
error_proto = 'pop3.error_proto'	 # response does not begin with +

# Standard Port
POP3_PORT = 110

# Line terminators (we always output CRLF, but accept any of CRLF, CR, LF)
CRLF = '\r\n'

# This library supports both the minimal and optional command sets:
# Arguments can be strings or integers (where appropriate)
# (e.g.: retr(1) and retr('1') both work equally well.
#
# Minimal Command Set:
#	 USER name				user(name)
#	 PASS string				pass_(string)
#	 STAT					stat()
#	 LIST [msg]				list(msg = None)
#	 RETR msg				retr(msg)
#	 DELE msg				dele(msg)
#	 NOOP					noop()
#	 RSET					rset()
#	 QUIT					quit()
#
# Optional Commands (some servers support these)
#	 APOP name digest			apop(name, digest)
#	 TOP msg n				top(msg, n)
#	 UIDL [msg]				uidl(msg = None)
#


class POP3:
	def __init__(self, host, port = POP3_PORT):
		self.host = host
		self.port = port
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect(self.host, self.port)
		self.file = self.sock.makefile('rb')
		self._debugging = 0
		self.welcome = self._getresp()

	def _putline(self, line):
		line = line + CRLF
		if self._debugging > 1: print '*put*', `line`
		self.sock.send(line)

	# Internal: send one command to the server (through _putline())
	def _putcmd(self, line):
		if self._debugging: print '*cmd*', `line`
		self._putline(line)

	# Internal: return one line from the server, stripping CRLF.
	# Raise EOFError if the connection is closed
	def _getline(self):
		line = self.file.readline()
		if self._debugging > 1:
			print '*get*', `line`
		if not line: raise EOFError
		if line[-2:] == CRLF: line = line[:-2]
		elif line[-1:] in CRLF: line = line[:-1]
		return line

	# Internal: get a response from the server.
	# Raise various errors if the response indicates an error
	def _getresp(self):
		resp = self._getline()
		if self._debugging > 1: print '*resp*', `resp`
		c = resp[:1]
		if c != '+':
			raise error_proto, resp
		return resp

	# Internal: get a response plus following text from the server.
	# Raise various errors if the response indicates an error
	def _getlongresp(self):
		resp = self._getresp()
		list = []
		while 1:
			line = self._getline()
			if line == '.':
				break
			list.append(line)
		return resp, list

	# Internal: send a command and get the response
	def _shortcmd(self, line):
		self._putcmd(line)
		return self._getresp()

	# Internal: send a command and get the response plus following text
	def _longcmd(self, line):
		self._putcmd(line)
		return self._getlongresp()

	# These can be useful:

	def getwelcome(self):
		return self.welcome

	def set_debuglevel(self, level):
		self._debugging = level

	# Here are all the POP commands:

	def user(self, user):
		user = str(user)
		return self._shortcmd('USER ' + user)

	def pass_(self, pswd):
		pswd = str(pswd)
		return self._shortcmd('PASS ' + pswd)

	def stat(self):
		retval = self._shortcmd('STAT')
		rets = string.split(retval)
		numMessages = string.atoi(rets[1])
		sizeMessages = string.atoi(rets[2])
		return (numMessages, sizeMessages)

	def list(self, msg=None):
		if msg:
			msg = str(msg)
			return self._longcmd('LIST ' + msg)
		else:
			return self._longcmd('LIST')

	def retr(self, which):
		which = str(which)
		return self._longcmd('RETR ' + which)

	def dele(self, which):
		which = str(which)
		return self._shortcmd('DELE ' + which)

	def noop(self):
		return self._shortcmd('NOOP')

	def rset(self):
		return self._shortcmd('RSET')

	# optional commands:

	def apop(self, digest):
		digest = str(digest)
		return self._shortcmd('APOP ' + digest)

	def top(self, which, howmuch):
		which = str(which)
		howmuch = str(howmuch)
		return self._longcmd('TOP ' + which + ' ' + howmuch)

	def uidl(self, which = None):
		if which:
			which = str(which)
			return self._longcmd('UIDL ' + which)
		else:
			return self._longcmd('UIDL')

	def quit(self):
		resp = self._shortcmd('QUIT')
		self.file.close()
		self.sock.close()
		del self.file, self.sock
		return resp

if __name__ == "__main__":
	a = POP3(TESTSERVER)
	print a.getwelcome()
	a.user(TESTACCOUNT)
	a.pass_(TESTPASSWORD)
	a.list()
	(numMsgs, totalSize) = a.stat()
	for i in range(1, numMsgs + 1):
		(header, msg, octets) = a.retr(i)
		print "Message ", `i`, ':'
		for line in msg:
			print '   ' + line
		print '-----------------------'
	a.quit()
