# A TELNET client class.  Based on RFC 854: TELNET Protocol
# Specification, by J. Postel and J. Reynolds


# Example:
#
# >>> from telnetlib import Telnet
# >>> tn = Telnet('voorn.cwi.nl', 79) # connect to finger port
# >>> tn.write('guido\r\n')
# >>> print tn.read_all()
# Login name: guido                       In real life: Guido van Rossum
# Office: M353,  x4127                    Home phone: 020-6225521
# Directory: /ufs/guido                   Shell: /usr/local/bin/esh
# On since Oct 28 11:02:16 on ttyq1   
# Project: Multimedia Kernel Systems
# No Plan.
# >>>
#
# Note that read() won't read until eof -- it just reads some data
# (but it guarantees to read at least one byte unless EOF is hit).
#
# It is possible to pass a Telnet object to select.select() in order
# to wait until more data is available.  Note that in this case,
# read_eager() may return '' even if there was data on the socket,
# because the protocol negotiation may have eaten the data.
# This is why EOFError is needed to distinguish between "no data"
# and "connection closed" (since the socket also appears ready for
# reading when it is closed).
#
# Bugs:
# - may hang when connection is slow in the middle of an IAC sequence
#
# To do:
# - option negotiation


# Imported modules
import socket
import select
import string
import regsub

# Tunable parameters
DEBUGLEVEL = 0

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
    def __init__(self, host, *args):
	if not args:
	    port = TELNET_PORT
	else:
	    if len(args) > 1: raise TypeError, 'too many args'
	    port = args[0]
	    if not port: port = TELNET_PORT
	self.debuglevel = DEBUGLEVEL
	self.host = host
	self.port = port
	self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	self.sock.connect((self.host, self.port))
	self.rawq = ''
	self.irawq = 0
	self.cookedq = ''
	self.eof = 0

    # Destructor
    def __del__(self):
	self.close()

    # Debug message
    def msg(self, msg, *args):
	if self.debuglevel > 0:
	    print 'Telnet(%s,%d):' % (self.host, self.port), msg % args

    # Set debug level
    def set_debuglevel(self, debuglevel):
	self.debuglevel = debuglevel

    # Explicit close
    def close(self):
	if self.sock:
	    self.sock.close()
	self.sock = None
	self.eof = 1

    # Return socket (e.g. for select)
    def get_socket(self):
	return self.sock

    # Return socket's fileno (e.g. for select)
    def fileno(self):
	return self.sock.fileno()

    # Write a string to the socket, doubling any IAC characters
    # Might block if the connection is blocked
    # May raise socket.error if the connection is closed
    def write(self, buffer):
	if IAC in buffer:
	    buffer = regsub.gsub(IAC, IAC+IAC, buffer)
	self.sock.send(buffer)

    # The following read_* methods exist:
    # Special case:
    # - read_until() reads until a string is encountered or a timeout is hit
    # These may block:
    # - read_all() reads all data until EOF
    # - read_some() reads at least one byte until EOF
    # These may do I/O but won't block doing it:
    # - read_very_eager() reads all data available on the socket
    # - read_eager() reads either data already queued or some data
    #                available on the socket
    # These don't do I/O:
    # - read_lazy() reads all data in the raw queue (processing it first)
    # - read_very_lazy() reads all data in the cooked queue

    # Read until a given string is encountered or until timeout
    # Raise EOFError if connection closed and no cooked data available
    # Return '' if no cooked data available otherwise
    def read_until(self, match, *args):
	if not args:
	    timeout = None
	else:
	    if len(args) > 1: raise TypeError, 'too many args'
	    timeout = args[0]
	n = len(match)
	self.process_rawq()
	i = string.find(self.cookedq, match)
	if i >= 0:
	    i = i+n
	    buf = self.cookedq[:i]
	    self.cookedq = self.cookedq[i:]
	    return buf
	s_reply = ([self], [], [])
	s_args = s_reply
	if timeout is not None:
	    s_args = s_args + (timeout,)
	while not self.eof and apply(select.select, s_args) == s_reply:
	    i = max(0, len(self.cookedq)-n)
	    self.fill_rawq()
	    self.process_rawq()
	    i = string.find(self.cookedq, match, i)
	    if i >= 0:
		i = i+n
		buf = self.cookedq[:i]
		self.cookedq = self.cookedq[i:]
		return buf
	return self.read_very_lazy()

    # Read all data until EOF
    # Block until connection closed
    def read_all(self):
	self.process_rawq()
	while not self.eof:
	    self.fill_rawq()
	    self.process_rawq()
	buf = self.cookedq
	self.cookedq = ''
	return buf

    # Read at least one byte of cooked data unless EOF is hit
    # Return '' if EOF is hit
    # Block if no data is immediately available
    def read_some(self):
	self.process_rawq()
	while not self.cookedq and not self.eof:
	    self.fill_rawq()
	    self.process_rawq()
	buf = self.cookedq
	self.cookedq = ''
	return buf

    # Read everything that's possible without blocking in I/O (eager)
    # Raise EOFError if connection closed and no cooked data available
    # Return '' if no cooked data available otherwise
    # Don't block unless in the midst of an IAC sequence
    def read_very_eager(self):
	self.process_rawq()
	while not self.eof and self.sock_avail():
	    self.fill_rawq()
	    self.process_rawq()
	return self.read_very_lazy()

    # Read readily available data
    # Raise EOFError if connection closed and no cooked data available
    # Return '' if no cooked data available otherwise
    # Don't block unless in the midst of an IAC sequence
    def read_eager(self):
	self.process_rawq()
	while not self.cookedq and not self.eof and self.sock_avail():
	    self.fill_rawq()
	    self.process_rawq()
	return self.read_very_lazy()

    # Process and return data that's already in the queues (lazy)
    # Raise EOFError if connection closed and no data available
    # Return '' if no cooked data available otherwise
    # Don't block unless in the midst of an IAC sequence
    def read_lazy(self):
	self.process_rawq()
	return self.read_very_lazy()

    # Return any data available in the cooked queue (very lazy)
    # Raise EOFError if connection closed and no data available
    # Return '' if no cooked data available otherwise
    # Don't block
    def read_very_lazy(self):
	buf = self.cookedq
	self.cookedq = ''
	if not buf and self.eof and not self.rawq:
	    raise EOFError, 'telnet connection closed'
	return buf

    # Transfer from raw queue to cooked queue
    # Set self.eof when connection is closed
    # Don't block unless in the midst of an IAC sequence
    def process_rawq(self):
	buf = ''
	try:
	    while self.rawq:
		c = self.rawq_getchar()
		if c != IAC:
		    buf = buf + c
		    continue
		c = self.rawq_getchar()
		if c == IAC:
		    buf = buf + c
		elif c in (DO, DONT):
		    opt = self.rawq_getchar()
		    self.msg('IAC %s %d', c == DO and 'DO' or 'DONT', ord(c))
		    self.sock.send(IAC + WONT + opt)
		elif c in (WILL, WONT):
		    opt = self.rawq_getchar()
		    self.msg('IAC %s %d',
			  c == WILL and 'WILL' or 'WONT', ord(c))
		else:
		    self.msg('IAC %s not recognized' % `c`)
	except EOFError: # raised by self.rawq_getchar()
	    pass
	self.cookedq = self.cookedq + buf

    # Get next char from raw queue
    # Block if no data is immediately available
    # Raise EOFError when connection is closed
    def rawq_getchar(self):
	if not self.rawq:
	    self.fill_rawq()
	    if self.eof:
		raise EOFError
	c = self.rawq[self.irawq]
	self.irawq = self.irawq + 1
	if self.irawq >= len(self.rawq):
	    self.rawq = ''
	    self.irawq = 0
	return c

    # Fill raw queue from exactly one recv() system call
    # Block if no data is immediately available
    # Set self.eof when connection is closed
    def fill_rawq(self):
	if self.irawq >= len(self.rawq):
	    self.rawq = ''
	    self.irawq = 0
	# The buffer size should be fairly small so as to avoid quadratic
	# behavior in process_rawq() above
	buf = self.sock.recv(50)
	self.eof = (not buf)
	self.rawq = self.rawq + buf

    # Test whether data is available on the socket
    def sock_avail(self):
	return select.select([self], [], [], 0) == ([self], [], [])


# Test program
# Usage: test [-d] ... [host [port]]
def test():
    import sys, string, socket, select
    debuglevel = 0
    while sys.argv[1:] and sys.argv[1] == '-d':
	debuglevel = debuglevel+1
	del sys.argv[1]
    host = 'localhost'
    if sys.argv[1:]:
	host = sys.argv[1]
    port = 0
    if sys.argv[2:]:
	portstr = sys.argv[2]
	try:
	    port = string.atoi(portstr)
	except string.atoi_error:
	    port = socket.getservbyname(portstr, 'tcp')
    tn = Telnet(host, port)
    tn.set_debuglevel(debuglevel)
    while 1:
	rfd, wfd, xfd = select.select([tn, sys.stdin], [], [])
	if sys.stdin in rfd:
	    line = sys.stdin.readline()
	    tn.write(line)
	if tn in rfd:
	    try:
		text = tn.read_eager()
	    except EOFError:
		print '*** Connection closed by remote host ***'
		break
	    if text:
		sys.stdout.write(text)
		sys.stdout.flush()
    tn.close()
