"Socket wrapper for Windows, which does not support dup()."

# (And hence, fromfd() and makefile() are unimplemented in C....)

# XXX Living dangerously here -- close() is implemented by deleting a
# reference.  Thus we rely on the real _socket module to close on
# deallocation, and also hope that nobody keeps a reference to our _sock
# member.



try:
	from _socket import *
except ImportError:
	from socket import *

_realsocketcall = socket


def socket(family, type, proto=0):
	return _socketobject(_realsocketcall(family, type, proto))


class _socketobject:

	def __init__(self, sock):
		self._sock = sock

	def close(self):
		self._sock = 0

	def __del__(self):
		self.close()

	def accept(self):
		sock, addr = self._sock.accept()
		return _socketobject(sock), addr

	def dup(self):
		return _socketobject(self._sock)

	def makefile(self, mode='r', bufsize=-1):
		return _fileobject(self._sock, mode, bufsize)

	_s = "def %s(self, *args): return apply(self._sock.%s, args)\n\n"
	for _m in ('bind', 'connect', 'connect_ex', 'fileno', 'listen',
		   'getpeername', 'getsockname',
		   'getsockopt', 'setsockopt',
		   'recv', 'recvfrom', 'send', 'sendto',
		   'setblocking',
		   'shutdown'):
		exec _s % (_m, _m)


class _fileobject:

	def __init__(self, sock, mode, bufsize):
		self._sock = sock
		self._mode = mode
		if bufsize < 0:
			bufsize = 512
		self._rbufsize = max(1, bufsize)
		self._wbufsize = bufsize
		self._wbuf = self._rbuf = ""

	def close(self):
		try:
			if self._sock:
				self.flush()
		finally:
			self._sock = 0

	def __del__(self):
		self.close()

	def flush(self):
		if self._wbuf:
			self._sock.send(self._wbuf)
			self._wbuf = ""

	def fileno(self):
		return self._sock.fileno()

	def write(self, data):
		self._wbuf = self._wbuf + data
		if self._wbufsize == 1:
			if '\n' in data:
				self.flush()
		else:
			if len(self._wbuf) >= self._wbufsize:
				self.flush()

	def writelines(self, list):
		filter(self._sock.send, list)
		self.flush()

	def read(self, n=-1):
		if n >= 0:
			k = len(self._rbuf)
			if n <= k:
				data = self._rbuf[:n]
				self._rbuf = self._rbuf[n:]
				return data
			n = n - k
			l = [self._rbuf]
			self._rbuf = ""
			while n > 0:
				new = self._sock.recv(max(n, self._rbufsize))
				if not new: break
				k = len(new)
				if k > n:
					l.append(new[:n])
					self._rbuf = new[n:]
					break
				l.append(new)
				n = n - k
			return "".join(l)
		k = max(512, self._rbufsize)
		l = [self._rbuf]
		self._rbuf = ""
		while 1:
			new = self._sock.recv(k)
			if not new: break
			l.append(new)
			k = min(k*2, 1024**2)
		return "".join(l)

	def readline(self, limit=-1):
		data = ""
		i = self._rbuf.find('\n')
		while i < 0 and not (0 < limit <= len(self._rbuf)):
			new = self._sock.recv(self._rbufsize)
			if not new: break
			i = new.find('\n')
			if i >= 0: i = i + len(self._rbuf)
			self._rbuf = self._rbuf + new
		if i < 0: i = len(self._rbuf)
		else: i = i+1
		if 0 <= limit < len(self._rbuf): i = limit
		data, self._rbuf = self._rbuf[:i], self._rbuf[i:]
		return data

	def readlines(self):
		list = []
		while 1:
			line = self.readline()
			if not line: break
			list.append(line)
		return list


# WSA error codes
errorTab = {}
errorTab[10004] = "The operation was interrupted."
errorTab[10009] = "A bad file handle was passed."
errorTab[10013] = "Permission denied."
errorTab[10014] = "A fault occurred on the network??" # WSAEFAULT
errorTab[10022] = "An invalid operation was attempted."
errorTab[10035] = "The socket operation would block"
errorTab[10036] = "A blocking operation is already in progress."
errorTab[10048] = "The network address is in use."
errorTab[10054] = "The connection has been reset."
errorTab[10058] = "The network has been shut down."
errorTab[10060] = "The operation timed out."
errorTab[10061] = "Connection refused."
errorTab[10063] = "The name is too long."
errorTab[10064] = "The host is down."
errorTab[10065] = "The host is unreachable."
