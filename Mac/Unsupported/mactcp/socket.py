"""socket.py for mac - Emulate socket module with mactcp and macdnr

Currently only implements TCP sockets (AF_INET, SOCK_STREAM).
Esoteric things like socket options don't work,
but getpeername() and makefile() do work; everything used by ftplib works!
"""

# Jack Jansen, CWI, November 1994 (initial version)
# Guido van Rossum, CWI, March 1995 (bug fixes and lay-out)


import mactcp
import MACTCPconst
import macdnr


# Exported constants

_myerror = 'socket_wrapper.error'
error = (mactcp.error, macdnr.error, _myerror)

SOCK_DGRAM = 1
SOCK_STREAM = 2

AF_INET = 1


# Internal constants

_BUFSIZE = 15*1024			# Size of TCP/UDP input buffer

_myaddress = None
_myname = None
_myaddrstr = None


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


def gethostbyaddr(str):
	id = _ipaddress(str)
	ptr = macdnr.AddrToName(id)
	ptr.wait()
	name = ptr.cname
	if name[-1:] == '.': name = name[:-1]
	names, addresses = [], [str]
	return name, names, addresses

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
		raise _myerror, 'Protocol family %d not supported' % type
	if type == SOCK_DGRAM:
		return _udpsocket()
	elif type == SOCK_STREAM:
		return _tcpsocket()
	raise _myerror, 'Protocol type %d not supported' % type


def fromfd(*args):
	raise _myerror, 'Operation not supported on a mac'


class _socket:
	def unsupported(self, *args):
		raise _myerror, 'Operation not supported on this socket'
	
	accept = unsupported
	bind = unsupported
	close = unsupported
	connect = unsupported
	fileno = unsupported
	getpeername = unsupported
	getsockname = unsupported
	getsockopt = unsupported
	listen = unsupported
	recv = unsupported
	recvfrom = unsupported
	send = unsupported
	sendto = unsupported
	setblocking = unsupported
	setsockopt = unsupported
	shutdown = unsupported


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
			raise _myerror, 'Not listening'
		self.listening = 0
		self.stream.wait()
		self.accepted = 1
		return self, self.getsockname()
	
	# bind has two ways of calling: s.bind(host, port) or s.bind((host, port));
	# the latter is more proper but the former more common
	def bind(self, a1, a2=None):
		if a2 is None:
			host, port = a1
		else:
			host, port = a1, a2
		self.port = port
		
	def close(self):
		if self.accepted:
			self.accepted = 0
			return
		self.stream.Abort()
	
	# connect has the same problem as bind (see above)
	def connect(self, a1, a2=None):
		if a2 is None:
			host, port = a1
		else:
			host, port = a1, a2
		self.stream.ActiveOpen(self.port, _ipaddress(host), port)
		
	def getsockname(self):
		host, port = self.stream.GetSockName()
		host = macdnr.AddrToStr(host)
		return host, port
		
	def getpeername(self):
		st = self.stream.Status()
		host = macdnr.AddrToStr(st.remoteHost)
		return host, st.remotePort		
		
	def listen(self, backlog):
		self.stream.PassiveOpen(self.port)
		self.listening = 1
		
	def makefile(self, rw = 'r', bs = 512):
		return _socketfile(self, rw, bs)
		
	def recv(self, bufsize, flags=0):
		if flags:
			raise _myerror, 'recv flags not yet supported on mac'
		if not self.databuf:
			try:
				self.databuf, urg, mark = self.stream.Rcv(0)
			except mactcp.error, arg:
				if arg[0] != MACTCPconst.connectionClosing:
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
	
	def __init__(self, sock, rw, bs):
		if rw[1:] == 'b': rw = rw[:1]
		if rw not in ('r', 'w'): raise _myerror, "mode must be 'r' or 'w'"
		self.sock = sock
		self.rw = rw
		self.bs = bs
		self.buf = ''
		
	def read(self, length = -1):
		if length < 0:
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
		return rv
		
	def readlines(self):
		list = []
		line = self.readline()
		while line:
			list.append(line)
			line = self.readline()
		return list
		
	def write(self, buf):
		BS = self.bs
		if len(buf) >= BS:
			self.flush()
			self.sock.send(buf)
		elif len(buf) + len(self.buf) >= BS:
			self.flush()
			self.buf = buf
		else:
			self.buf = self.buf + buf
	
	def writelines(self, list):
		for line in list:
			self.write(line)
	
	def flush(self):
		if self.buf and self.rw == 'w':
			self.sock.send(self.buf)
			self.buf = ''
	
	def close(self):
		self.flush()
		##self.sock.close()
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
