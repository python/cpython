#
# socket.py for mac - Emulate socket module with mactcp and macdnr
#
# Jack Jansen, CWI, November 1994
#
import mactcp
import MACTCP
import macdnr
import sys

#
# Constants
#
_myerror = 'socket_wrapper.error'
error = (mactcp.error, macdnr.error, _myerror)

SOCK_DGRAM=1
SOCK_STREAM=2

AF_INET=1

#
# Internal constants
#
_BUFSIZE=4096		# Size of tcp/udp input buffer
_connectionClosing=-42 # XXXX

_myaddress=None
_myname=None
_myaddrstr=None

def _myipaddress():
	global _myaddress
	if _myaddress == None:
		_myaddress = mactcp.IPAddr()
	return _myaddress

def _ipaddress(str):
	if type(str) == type(1):
		return str			# Already numeric
	ptr = macdnr.StrToAddr(str)
	ptr.wait()
	return ptr.ip0
	
def gethostbyname(str):
	id = _ipaddress(str)
	return macdnr.AddrToStr(id)
	
def gethostname():
	global _myname
	if _myname == None:
		id = _myipaddress()
		ptr = macdnr.AddrToName(id)
		ptr.wait()
		_myname = ptr.cname
	return _myname

def _gethostaddress():
	global _myaddrstr
	if _myaddrstr == None:
		id = _myipaddress()
		_myaddrstr = macdnr.AddrToStr(id)
	return _myaddrstr
	
def socket(family, type, *which):
	if family <> AF_INET:
		raise my_error, 'Protocol family not supported'
	if type == SOCK_DGRAM:
		return _udpsocket()
	elif type == SOCK_STREAM:
		return _tcpsocket()
	raise my_error, 'Protocol type not supported'
	
def fromfd(*args):
	raise my_error, 'Operation not supported on a mac'
	
class _socket:
	def accept(self, *args):
		raise my_error, 'Operation not supported on this socket'
		
	bind = accept
	close = accept
	connect = accept
	fileno = accept
	getpeername = accept
	getsockname = accept
	getsockopt = accept
	listen = accept
	recv = accept
	recvfrom = accept
	send = accept
	sendto = accept
	setblocking = accept
	setsockopt = accept
	shutdown = accept
	 
class _tcpsocket(_socket):
	def __init__(self):
		self.stream = mactcp.TCPCreate(_BUFSIZE)
		##self.stream.asr = self.asr
		self.databuf = ''
		self.udatabuf = ''
		self.port = 0
		self.accepted = 0
		self.listening = 0

	def accept(self):
		if not self.listening:
			raise my_error, 'Not listening'
		self.listening = 0
		self.stream.wait()
		self.accepted = 1
		return self, self.getsockname()
		
	def bind(self, host, port):
		self.port = port
		
	def close(self):
		if self.accepted:
			self.accepted = 0
			return
		self.stream.Abort()
			
	def connect(self, host, port):
		self.stream.ActiveOpen(self.port, _ipaddress(host), port)
		
	def getsockname(self):
		st = self.stream.Status()
		host = macdnr.AddrToStr(st.localHost)
		return host, st.localPort
		
	def getpeername(self):
		st = self.stream.Status()
		host = macdnr.AddrToStr(st.remoteHost)
		return host, st.remotePort		
		
	def listen(self, backlog):
		self.stream.PassiveOpen(self.port)
		self.listening = 1
		
	def makefile(self, rw):
		return _socketfile(self)
		
	def recv(self, bufsize, flags=0):
		if flags:
			raise my_error, 'recv flags not yet supported on mac'
		if not self.databuf:
			try:
				self.databuf, urg, mark = self.stream.Rcv(0)
				if not self.databuf:
					print '** socket: no data!'
				print '** recv: got ', len(self.databuf)
			except mactcp.error, arg:
				if arg[0] != MACTCP.connectionClosing:
					raise mactcp.error, arg
		rv = self.databuf[:bufsize]
		self.databuf = self.databuf[bufsize:]
		return rv
		
	def send(self, buf):
		self.stream.Send(buf)
		return len(buf)
		
	def shutdown(self, how):
		if how == 0:
			return
		self.stream.Close()
		
	def bytes_readable(self):
		st = self.stream.Status()
		return st.amtUnreadData
		
	def bytes_writeable(self):
		st = self.stream.Status()
		return st.sendWindow - st.sendUnacked;
		
class _udpsocket(_socket):
	def __init__(self):
		pass
		
class _socketfile:
	def __init__(self, sock):
		self.sock = sock
		self.buf = ''
		
	def read(self, *arg):
		if arg:
			length = arg
		else:
			length = 0x7fffffff
		while len(self.buf) < length:
			new = self.sock.recv(0x7fffffff)
			if not new:
				break
			self.buf = self.buf + new
		rv = self.buf[:length]
		self.buf = self.buf[length:]
		return rv
		
	def readline(self):
		import string
		while not '\n' in self.buf:
			new = self.sock.recv(0x7fffffff)
			if not new:
				break
			self.buf = self.buf + new
		if not '\n' in self.buf:
			rv = self.buf
			self.buf = ''
		else:
			i = string.index(self.buf, '\n')
			rv = self.buf[:i+1]
			self.buf = self.buf[i+1:]
		print '** Readline:',self, `rv`
		return rv
			
	def write(self, buf):
		self.sock.send(buf)
		
	def close(self):
		self.sock.close()
		del self.sock
		
def __test_tcp():
	s = socket(AF_INET, SOCK_STREAM)
	s.connect('poseidon.cwi.nl', 13)
	rv = s.recv(1000)
	print 'Time/date:', rv
	rv = s.recv(1000)
	if rv:
		print 'Unexpected extra data:', rv
	s.close()
	
def __test_udp():
	s = socket(AF_INET, SOCK_DGRAM)
	print 'Sending data... (hello world)'
	s.sendto(('poseidon.cwi.nl', 7), 'hello world')
	rv, host = s.recvfrom(1000)
	print 'Got from ', host, ':', rv
	
