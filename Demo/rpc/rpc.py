# Implement (a subset of) Sun RPC, version 2 -- RFC1057.

# XXX There should be separate exceptions for the various reasons why
# XXX an RPC can fail, rather than using RuntimeError for everything

# XXX The UDP version of the protocol resends requests when it does
# XXX not receive a timely reply -- use only for idempotent calls!

import xdr
import socket
import os

RPCVERSION = 2

CALL = 0
REPLY = 1

AUTH_NULL = 0
AUTH_UNIX = 1
AUTH_SHORT = 2
AUTH_DES = 3

MSG_ACCEPTED = 0
MSG_DENIED = 1

SUCCESS = 0				# RPC executed successfully
PROG_UNAVAIL  = 1			# remote hasn't exported program
PROG_MISMATCH = 2			# remote can't support version #
PROC_UNAVAIL  = 3			# program can't support procedure
GARBAGE_ARGS  = 4			# procedure can't decode params

RPC_MISMATCH = 0			# RPC version number != 2
AUTH_ERROR = 1				# remote can't authenticate caller

AUTH_BADCRED      = 1			# bad credentials (seal broken)
AUTH_REJECTEDCRED = 2			# client must begin new session
AUTH_BADVERF      = 3			# bad verifier (seal broken)
AUTH_REJECTEDVERF = 4			# verifier expired or replayed
AUTH_TOOWEAK      = 5			# rejected for security reasons


class Packer(xdr.Packer):

	def pack_auth(self, auth):
		flavor, stuff = auth
		self.pack_enum(flavor)
		self.pack_opaque(stuff)

	def pack_auth_unix(self, stamp, machinename, uid, gid, gids):
		self.pack_uint(stamp)
		self.pack_string(machinename)
		self.pack_uint(uid)
		self.pack_uint(gid)
		self.pack_uint(len(gids))
		for i in gids:
			self.pack_uint(i)

	def pack_callheader(self, xid, prog, vers, proc, cred, verf):
		self.pack_uint(xid)
		self.pack_enum(CALL)
		self.pack_uint(RPCVERSION)
		self.pack_uint(prog)
		self.pack_uint(vers)
		self.pack_uint(proc)
		self.pack_auth(cred)
		self.pack_auth(verf)
		# Caller must add procedure-specific part of call

	def pack_replyheader(self, xid, verf):
		self.pack_uint(xid)
		self.pack_enum(REPLY)
		self.pack_uint(MSG_ACCEPTED)
		self.pack_auth(verf)
		self.pack_enum(SUCCESS)
		# Caller must add procedure-specific part of reply


class Unpacker(xdr.Unpacker):

	def unpack_auth(self):
		flavor = self.unpack_enum()
		stuff = self.unpack_opaque()
		return (flavor, stuff)

	def unpack_replyheader(self):
		xid = self.unpack_uint()
		mtype = self.unpack_enum()
		if mtype <> REPLY:
			raise RuntimeError, 'no REPLY but ' + `mtype`
		stat = self.unpack_enum()
		if stat == MSG_DENIED:
			stat = self.unpack_enum()
			if stat == RPC_MISMATCH:
				low = self.unpack_uint()
				high = self.unpack_uint()
				raise RuntimeError, \
				  'MSG_DENIED: RPC_MISMATCH: ' + `low, high`
			if stat == AUTH_ERROR:
				stat = self.unpack_uint()
				raise RuntimeError, \
					'MSG_DENIED: AUTH_ERROR: ' + `stat`
			raise RuntimeError, 'MSG_DENIED: ' + `stat`
		if stat <> MSG_ACCEPTED:
			raise RuntimeError, \
			  'Neither MSG_DENIED nor MSG_ACCEPTED: ' + `stat`
		verf = self.unpack_auth()
		stat = self.unpack_enum()
		if stat == PROG_MISMATCH:
			low = self.unpack_uint()
			high = self.unpack_uint()
			raise RuntimeError, \
				'call failed: PROG_MISMATCH: ' + `low, high`
		if stat <> SUCCESS:
			raise RuntimeError, 'call failed: ' + `stat`
		return xid, verf
		# Caller must get procedure-specific part of reply


# Subroutines to create opaque authentication objects

def make_auth_null():
	return ''

def make_auth_unix(seed, host, uid, gid, groups):
	p = Packer().init()
	p.pack_auth_unix(seed, host, uid, gid, groups)
	return p.get_buf()

def make_auth_unix_default():
	try:
		from os import getuid, getgid
		uid = getuid()
		gid = getgid()
	except ImportError:
		uid = gid = 0
	return make_auth_unix(0, socket.gethostname(), uid, gid, [])


# Common base class for clients

class Client:

	def init(self, host, prog, vers, port, type):
		self.host = host
		self.prog = prog
		self.vers = vers
		self.port = port
		self.type = type
		self.sock = socket.socket(socket.AF_INET, type)
		self.sock.connect((host, port))
		self.lastxid = 0
		self.addpackers()
		self.cred = None
		self.verf = None
		return self

	def Null(self):			# Procedure 0 is always like this
		self.start_call(0)
		self.do_call(0)
		self.end_call()

	def close(self):
		self.sock.close()

	# Functions that may be overridden by specific derived classes

	def addpackers(self):
		self.packer = Packer().init()
		self.unpacker = Unpacker().init('')

	def mkcred(self, proc):
		if self.cred == None:
			self.cred = (AUTH_NULL, make_auth_null())
		return self.cred

	def mkverf(self, proc):
		if self.verf == None:
			self.verf = (AUTH_NULL, make_auth_null())
		return self.verf


# Record-Marking standard support

def sendfrag(sock, last, frag):
	x = len(frag)
	if last: x = x | 0x80000000L
	header = (chr(int(x>>24 & 0xff)) + chr(int(x>>16 & 0xff)) + \
		  chr(int(x>>8 & 0xff)) + chr(int(x & 0xff)))
	sock.send(header + frag)

def sendrecord(sock, record):
	sendfrag(sock, 1, record)

def recvfrag(sock):
	header = sock.recv(4)
	x = long(ord(header[0]))<<24 | ord(header[1])<<16 | \
	    ord(header[2])<<8 | ord(header[3])
	last = ((x & 0x80000000) != 0)
	n = int(x & 0x7fffffff)
	frag = ''
	while n > 0:
		buf = sock.recv(n)
		if not buf: raise EOFError
		n = n - len(buf)
		frag = frag + buf
	return last, frag

def recvrecord(sock):
	record = ''
	last = 0
	while not last:
		last, frag = recvfrag(sock)
		record = record + frag
	return record


# Raw TCP-based client

class RawTCPClient(Client):

	def init(self, host, prog, vers, port):
		return Client.init(self, host, prog, vers, port, \
			socket.SOCK_STREAM)

	def start_call(self, proc):
		self.lastxid = xid = self.lastxid + 1
		cred = self.mkcred(proc)
		verf = self.mkverf(proc)
		p = self.packer
		p.reset()
		p.pack_callheader(xid, self.prog, self.vers, proc, cred, verf)

	def do_call(self, *rest):
		# rest is used for UDP buffer size; ignored for TCP
		call = self.packer.get_buf()
		sendrecord(self.sock, call)
		reply = recvrecord(self.sock)
		u = self.unpacker
		u.reset(reply)
		xid, verf = u.unpack_replyheader()
		if xid <> self.lastxid:
			# Can't really happen since this is TCP...
			raise RuntimeError, 'wrong xid in reply ' + `xid` + \
				' instead of ' + `self.lastxid`

	def end_call(self):
		self.unpacker.done()


# Raw UDP-based client

class RawUDPClient(Client):

	def init(self, host, prog, vers, port):
		return Client.init(self, host, prog, vers, port, \
			socket.SOCK_DGRAM)

	def start_call(self, proc):
		self.lastxid = xid = self.lastxid + 1
		cred = self.mkcred(proc)
		verf = self.mkverf(proc)
		p = self.packer
		p.reset()
		p.pack_callheader(xid, self.prog, self.vers, proc, cred, verf)

	def do_call(self, *rest):
		try:
			from select import select
		except ImportError:
			print 'WARNING: select not found, RPC may hang'
			select = None
		if len(rest) == 0:
			bufsize = 8192
		elif len(rest) > 1:
			raise TypeError, 'too many args'
		else:
			bufsize = rest[0] + 512
		call = self.packer.get_buf()
		timeout = 1
		count = 5
		self.sock.send(call)
		while 1:
			r, w, x = [self.sock], [], []
			if select:
				r, w, x = select(r, w, x, timeout)
			if self.sock not in r:
				count = count - 1
				if count < 0: raise RuntimeError, 'timeout'
				if timeout < 25: timeout = timeout *2
				print 'RESEND', timeout, count
				self.sock.send(call)
				continue
			reply = self.sock.recv(bufsize)
			u = self.unpacker
			u.reset(reply)
			xid, verf = u.unpack_replyheader()
			if xid <> self.lastxid:
				print 'BAD xid'
				continue
			break

	def end_call(self):
		self.unpacker.done()


# Port mapper interface

PMAP_PORT = 111
PMAP_PROG = 100000
PMAP_VERS = 2
PMAPPROC_NULL = 0			# (void) -> void
PMAPPROC_SET = 1			# (mapping) -> bool
PMAPPROC_UNSET = 2			# (mapping) -> bool
PMAPPROC_GETPORT = 3			# (mapping) -> unsigned int
PMAPPROC_DUMP = 4			# (void) -> pmaplist
PMAPPROC_CALLIT = 5			# (call_args) -> call_result

# A mapping is (prog, vers, prot, port) and prot is one of:

IPPROTO_TCP = 6
IPPROTO_UDP = 17

# A pmaplist is a variable-length list of mappings, as follows:
# either (1, mapping, pmaplist) or (0).

# A call_args is (prog, vers, proc, args) where args is opaque;
# a call_result is (port, res) where res is opaque.


class PortMapperPacker(Packer):

	def pack_mapping(self, mapping):
		prog, vers, prot, port = mapping
		self.pack_uint(prog)
		self.pack_uint(vers)
		self.pack_uint(prot)
		self.pack_uint(port)

	def pack_pmaplist(self, list):
		self.pack_list(list, self.pack_mapping)


class PortMapperUnpacker(Unpacker):

	def unpack_mapping(self):
		prog = self.unpack_uint()
		vers = self.unpack_uint()
		prot = self.unpack_uint()
		port = self.unpack_uint()
		return prog, vers, prot, port

	def unpack_pmaplist(self):
		return self.unpack_list(self.unpack_mapping)


class PartialPortMapperClient:

	def addpackers(self):
		self.packer = PortMapperPacker().init()
		self.unpacker = PortMapperUnpacker().init('')

	def Getport(self, mapping):
		self.start_call(PMAPPROC_GETPORT)
		self.packer.pack_mapping(mapping)
		self.do_call(4)
		port = self.unpacker.unpack_uint()
		self.end_call()
		return port

	def Dump(self):
		self.start_call(PMAPPROC_DUMP)
		self.do_call(8192-512)
		list = self.unpacker.unpack_pmaplist()
		self.end_call()
		return list


class TCPPortMapperClient(PartialPortMapperClient, RawTCPClient):

	def init(self, host):
		return RawTCPClient.init(self, \
			host, PMAP_PROG, PMAP_VERS, PMAP_PORT)


class UDPPortMapperClient(PartialPortMapperClient, RawUDPClient):

	def init(self, host):
		return RawUDPClient.init(self, \
			host, PMAP_PROG, PMAP_VERS, PMAP_PORT)


class TCPClient(RawTCPClient):

	def init(self, host, prog, vers):
		pmap = TCPPortMapperClient().init(host)
		port = pmap.Getport((prog, vers, IPPROTO_TCP, 0))
		pmap.close()
		return RawTCPClient.init(self, host, prog, vers, port)


class UDPClient(RawUDPClient):

	def init(self, host, prog, vers):
		pmap = UDPPortMapperClient().init(host)
		port = pmap.Getport((prog, vers, IPPROTO_UDP, 0))
		pmap.close()
		return RawUDPClient.init(self, host, prog, vers, port)


def test():
	import T
	T.TSTART()
	pmap = UDPPortMapperClient().init('')
	T.TSTOP()
	pmap.Null()
	T.TSTOP()
	list = pmap.Dump()
	T.TSTOP()
	list.sort()
	for prog, vers, prot, port in list:
		print prog, vers,
		if prot == IPPROTO_TCP: print 'tcp',
		elif prot == IPPROTO_UDP: print 'udp',
		else: print prot,
		print port
