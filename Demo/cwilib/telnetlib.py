# Telnet client library

import socket
import select
import string
import regsub

# Tunable parameters
TIMEOUT = 30.0
DEBUGLEVEL = 1

# Telnet protocol defaults
TELNET_PORT = 23

# Telnet protocol characters (don't change)
IAC  = chr(255)	# "Interpret As Command"
DONT = chr(254)
DO   = chr(253)
WONT = chr(252)
WILL = chr(251)


# Telnet interface class

class Telnet:

	# Constructor
	def __init__(self, host, port):
		self.debuglevel = DEBUGLEVEL
		self.host = host
		if not port: port = TELNET_PORT
		self.port = port
		self.timeout = TIMEOUT
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect((self.host, self.port))
		self.rawq = ''
		self.irawq = 0
		self.cookedq = ''

	# Destructor
	def __del__(self):
		self.close()

	# Print debug message
	def msg(self, msg, *args):
		if self.debuglevel > 0:
			print 'TELNET:', msg%args

	# Set debug level
	def set_debuglevel(self, debuglevel):
		self.debuglevel = debuglevel

	# Set time-out on certain reads
	def set_timeout(self, timeout):
		self.timeout = float(timeout)

	# Explicit close
	def close(self):
		if self.sock:
			self.sock.close()
		self.sock = None

	# Return socket (e.g. for select)
	def get_socket(self):
		return self.sock

	# Return socket's fileno (e.g. for select)
	def fileno(self):
		return self.sock.fileno()

	# Write a string to the socket, doubling any IAC characters
	def write(self, buffer):
		if IAC in buffer:
			buffer = regsub.gsub(IAC, IAC+IAC, buffer)
		self.sock.send(buffer)

	# Read until a given string is encountered or until timeout
	def read_until(self, match):
##		self.msg('read_until(%s)' % `match`)
		n = len(match)
		self.process_rawq()
		i = string.find(self.cookedq, match)
		if i < 0:
			i = max(0, len(self.cookedq)-n)
			self.fill_cookedq()
			i = string.find(self.cookedq, match, i)
		if i >= 0:
			i = i+n
			buf = self.cookedq[:i]
			self.cookedq = self.cookedq[i:]
##			self.msg('read_until(%s) -> %s' % (`match`, `buf`))
			return buf
		while select.select([self], [], [], self.timeout) == \
			  ([self], [], []):
			i = max(0, len(self.cookedq)-n)
			self.fill_rawq()
			self.process_rawq()
			i = string.find(self.cookedq, match, i)
			if i >= 0:
				i = i+n
				buf = self.cookedq[:i]
				self.cookedq = self.cookedq[i:]
##				self.msg('read_until(%s) -> %s' %
##					  (`match`, `buf`))
				return buf
		buf = self.cookedq
		self.cookedq = ''
##		self.msg('read_until(%s) -> %s' % (`match`, `buf`))
		return buf

	# Read everything that's possible without really blocking
	def read_now(self):
		self.fill_cookedq()
		buf = self.cookedq
		self.cookedq = ''
##		self.msg('read_now() --> %s' % `buf`)
		return buf

	# Fill cooked queue without blocking
	def fill_cookedq(self):
		self.process_rawq()
		while select.select([self], [], [], 0) == ([self], [], []):
			self.fill_rawq()
			if not self.rawq:
				raise EOFError
			self.process_rawq()

	# Transfer from raw queue to cooked queue
	def process_rawq(self):
		# There is some silliness going on here in an attempt
		# to avoid quadratic behavior with large inputs...
		buf = ''
		while self.rawq:
			c = self.rawq_getchar()
			if c != IAC:
				buf = buf + c
				if len(buf) >= 44:
##					self.msg('transfer: %s' % `buf`)
					self.cookedq = self.cookedq + buf
					buf = ''
				continue
			c = self.rawq_getchar()
			if c == IAC:
				buf = buf + c
			elif c in (DO, DONT):
				opt = self.rawq_getchar()
				self.msg('IAC %s %d',
					  c == DO and 'DO' or 'DONT',
					  ord(c))
				self.sock.send(IAC + WONT + opt)
			elif c in (WILL, WONT):
				opt = self.rawq_getchar()
				self.msg('IAC %s %d',
					  c == WILL and 'WILL' or 'WONT',
					  ord(c))
			else:
				self.msg('IAC %s not recognized' % `c`)
##		self.msg('transfer: %s' % `buf`)
		self.cookedq = self.cookedq + buf

	# Get next char from raw queue, blocking if necessary
	def rawq_getchar(self):
		if not self.rawq:
			self.fill_rawq()
		if self.irawq >= len(self.rawq):
			raise EOFError
		c = self.rawq[self.irawq]
		self.irawq = self.irawq + 1
		if self.irawq >= len(self.rawq):
			self.rawq = ''
			self.irawq = 0
		return c

	# Fill raw queue
	def fill_rawq(self):
		if self.irawq >= len(self.rawq):
			self.rawq = ''
			self.irawq = 0
		buf = self.sock.recv(50)
##		self.msg('fill_rawq(): %s' % `buf`)
		self.rawq = self.rawq + buf
