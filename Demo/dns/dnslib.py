# Domain Name Server (DNS) interface
#
# See RFC 1035:
# ------------------------------------------------------------------------
# Network Working Group                                     P. Mockapetris
# Request for Comments: 1035                                           ISI
#                                                            November 1987
# Obsoletes: RFCs 882, 883, 973
# 
#             DOMAIN NAMES - IMPLEMENTATION AND SPECIFICATION
# ------------------------------------------------------------------------


import string

import dnstype
import dnsclass
import dnsopcode


# Low-level 16 and 32 bit integer packing and unpacking

def pack16bit(n):
	return chr((n>>8)&0xFF) + chr(n&0xFF)

def pack32bit(n):
	return chr((n>>24)&0xFF) + chr((n>>16)&0xFF) \
		  + chr((n>>8)&0xFF) + chr(n&0xFF)

def unpack16bit(s):
	return (ord(s[0])<<8) | ord(s[1])

def unpack32bit(s):
	return (ord(s[0])<<24) | (ord(s[1])<<16) \
		  | (ord(s[2])<<8) | ord(s[3])

def addr2bin(addr):
	if type(addr) == type(0):
		return addr
	bytes = string.splitfields(addr, '.')
	if len(bytes) != 4: raise ValueError, 'bad IP address'
	n = 0
	for byte in bytes: n = n<<8 | string.atoi(byte)
	return n

def bin2addr(n):
	return '%d.%d.%d.%d' % ((n>>24)&0xFF, (n>>16)&0xFF,
		  (n>>8)&0xFF, n&0xFF)


# Packing class

class Packer:
	def __init__(self):
		self.buf = ''
		self.index = {}
	def getbuf(self):
		return self.buf
	def addbyte(self, c):
		if len(c) != 1: raise TypeError, 'one character expected'
		self.buf = self.buf + c
	def addbytes(self, bytes):
		self.buf = self.buf + bytes
	def add16bit(self, n):
		self.buf = self.buf + pack16bit(n)
	def add32bit(self, n):
		self.buf = self.buf + pack32bit(n)
	def addaddr(self, addr):
		n = addr2bin(addr)
		self.buf = self.buf + pack32bit(n)
	def addstring(self, s):
		self.addbyte(chr(len(s)))
		self.addbytes(s)
	def addname(self, name):
		# Domain name packing (section 4.1.4)
		# Add a domain name to the buffer, possibly using pointers.
		# The case of the first occurrence of a name is preserved.
		# Redundant dots are ignored.
		list = []
		for label in string.splitfields(name, '.'):
			if label:
				if len(label) > 63:
					raise PackError, 'label too long'
				list.append(label)
		keys = []
		for i in range(len(list)):
			key = string.upper(string.joinfields(list[i:], '.'))
			keys.append(key)
			if self.index.has_key(key):
				pointer = self.index[key]
				break
		else:
			i = len(list)
			pointer = None
		# Do it into temporaries first so exceptions don't
		# mess up self.index and self.buf
		buf = ''
		offset = len(self.buf)
		index = []
		for j in range(i):
			label = list[j]
			n = len(label)
			if offset + len(buf) < 0x3FFF:
				index.append( (keys[j], offset + len(buf)) )
			else:
				print 'dnslib.Packer.addname:',
				print 'warning: pointer too big'
			buf = buf + (chr(n) + label)
		if pointer:
			buf = buf + pack16bit(pointer | 0xC000)
		else:
			buf = buf + '\0'
		self.buf = self.buf + buf
		for key, value in index:
			self.index[key] = value
	def dump(self):
		keys = self.index.keys()
		keys.sort()
		print '-'*40
		for key in keys:
			print '%20s %3d' % (key, self.index[key])
		print '-'*40
		space = 1
		for i in range(0, len(self.buf)+1, 2):
			if self.buf[i:i+2] == '**':
				if not space: print
				space = 1
				continue
			space = 0
			print '%4d' % i,
			for c in self.buf[i:i+2]:
				if ' ' < c < '\177':
					print ' %c' % c,
				else:
					print '%2d' % ord(c),
			print
		print '-'*40


# Unpacking class

UnpackError = 'dnslib.UnpackError'	# Exception

class Unpacker:
	def __init__(self, buf):
		self.buf = buf
		self.offset = 0
	def getbyte(self):
		c = self.buf[self.offset]
		self.offset = self.offset + 1
		return c
	def getbytes(self, n):
		s = self.buf[self.offset : self.offset + n]
		if len(s) != n: raise UnpackError, 'not enough data left'
		self.offset = self.offset + n
		return s
	def get16bit(self):
		return unpack16bit(self.getbytes(2))
	def get32bit(self):
		return unpack32bit(self.getbytes(4))
	def getaddr(self):
		return bin2addr(self.get32bit())
	def getstring(self):
		return self.getbytes(ord(self.getbyte()))
	def getname(self):
		# Domain name unpacking (section 4.1.4)
		c = self.getbyte()
		i = ord(c)
		if i & 0xC0 == 0xC0:
			d = self.getbyte()
			j = ord(d)
			pointer = ((i<<8) | j) & ~0xC000
			save_offset = self.offset
			try:
				self.offset = pointer
				domain = self.getname()
			finally:
				self.offset = save_offset
			return domain
		if i == 0:
			return ''
		domain = self.getbytes(i)
		remains = self.getname()
		if not remains:
			return domain
		else:
			return domain + '.' + remains


# Test program for packin/unpacking (section 4.1.4)

def testpacker():
	N = 25
	R = range(N)
	import timing
	# See section 4.1.4 of RFC 1035
	timing.start()
	for i in R:
		p = Packer()
		p.addbytes('*' * 20)
		p.addname('f.ISI.ARPA')
		p.addbytes('*' * 8)
		p.addname('Foo.F.isi.arpa')
		p.addbytes('*' * 18)
		p.addname('arpa')
		p.addbytes('*' * 26)
		p.addname('')
	timing.finish()
	print round(timing.milli() * 0.001 / N, 3), 'seconds per packing'
	p.dump()
	u = Unpacker(p.buf)
	u.getbytes(20)
	u.getname()
	u.getbytes(8)
	u.getname()
	u.getbytes(18)
	u.getname()
	u.getbytes(26)
	u.getname()
	timing.start()
	for i in R:
		u = Unpacker(p.buf)
		res = (u.getbytes(20),
		       u.getname(),
		       u.getbytes(8),
		       u.getname(),
		       u.getbytes(18),
		       u.getname(),
		       u.getbytes(26),
		       u.getname())
	timing.finish()
	print round(timing.milli() * 0.001 / N, 3), 'seconds per unpacking'
	for item in res: print item


# Pack/unpack RR toplevel format (section 3.2.1)

class RRpacker(Packer):
	def __init__(self):
		Packer.__init__(self)
		self.rdstart = None
	def addRRheader(self, name, type, klass, ttl, *rest):
		self.addname(name)
		self.add16bit(type)
		self.add16bit(klass)
		self.add32bit(ttl)
		if rest:
			if res[1:]: raise TypeError, 'too many args'
			rdlength = rest[0]
		else:
			rdlength = 0
		self.add16bit(rdlength)
		self.rdstart = len(self.buf)
	def patchrdlength(self):
		rdlength = unpack16bit(self.buf[self.rdstart-2:self.rdstart])
		if rdlength == len(self.buf) - self.rdstart:
			return
		rdata = self.buf[self.rdstart:]
		save_buf = self.buf
		ok = 0
		try:
			self.buf = self.buf[:self.rdstart-2]
			self.add16bit(len(rdata))
			self.buf = self.buf + rdata
			ok = 1
		finally:
			if not ok: self.buf = save_buf
	def endRR(self):
		if self.rdstart is not None:
			self.patchrdlength()
		self.rdstart = None
	def getbuf(self):
		if self.rdstart is not None: self.patchrdlenth()
		return Packer.getbuf(self)
	# Standard RRs (section 3.3)
	def addCNAME(self, name, klass, ttl, cname):
		self.addRRheader(name, dnstype.CNAME, klass, ttl)
		self.addname(cname)
		self.endRR()
	def addHINFO(self, name, klass, ttl, cpu, os):
		self.addRRheader(name, dnstype.HINFO, klass, ttl)
		self.addstring(cpu)
		self.addstring(os)
		self.endRR()
	def addMX(self, name, klass, ttl, preference, exchange):
		self.addRRheader(name, dnstype.MX, klass, ttl)
		self.add16bit(preference)
		self.addname(exchange)
		self.endRR()
	def addNS(self, name, klass, ttl, nsdname):
		self.addRRheader(name, dnstype.NS, klass, ttl)
		self.addname(nsdname)
		self.endRR()
	def addPTR(self, name, klass, ttl, ptrdname):
		self.addRRheader(name, dnstype.PTR, klass, ttl)
		self.addname(ptrdname)
		self.endRR()
	def addSOA(self, name, klass, ttl,
		  mname, rname, serial, refresh, retry, expire, minimum):
		self.addRRheader(name, dnstype.SOA, klass, ttl)
		self.addname(mname)
		self.addname(rname)
		self.add32bit(serial)
		self.add32bit(refresh)
		self.add32bit(retry)
		self.add32bit(expire)
		self.add32bit(minimum)
		self.endRR()
	def addTXT(self, name, klass, ttl, list):
		self.addRRheader(name, dnstype.TXT, klass, ttl)
		for txtdata in list:
			self.addstring(txtdata)
		self.endRR()
	# Internet specific RRs (section 3.4) -- class = IN
	def addA(self, name, ttl, address):
		self.addRRheader(name, dnstype.A, dnsclass.IN, ttl)
		self.addaddr(address)
		self.endRR()
	def addWKS(self, name, ttl, address, protocol, bitmap):
		self.addRRheader(name, dnstype.WKS, dnsclass.IN, ttl)
		self.addaddr(address)
		self.addbyte(chr(protocol))
		self.addbytes(bitmap)
		self.endRR()


class RRunpacker(Unpacker):
	def __init__(self, buf):
		Unpacker.__init__(self, buf)
		self.rdend = None
	def getRRheader(self):
		name = self.getname()
		type = self.get16bit()
		klass = self.get16bit()
		ttl = self.get32bit()
		rdlength = self.get16bit()
		self.rdend = self.offset + rdlength
		return (name, type, klass, ttl, rdlength)
	def endRR(self):
		if self.offset != self.rdend:
			raise UnpackError, 'end of RR not reached'
	def getCNAMEdata(self):
		return self.getname()
	def getHINFOdata(self):
		return self.getstring(), self.getstring()
	def getMXdata(self):
		return self.get16bit(), self.getname()
	def getNSdata(self):
		return self.getname()
	def getPTRdata(self):
		return self.getname()
	def getSOAdata(self):
		return self.getname(), \
		       self.getname(), \
		       self.get32bit(), \
		       self.get32bit(), \
		       self.get32bit(), \
		       self.get32bit(), \
		       self.get32bit()
	def getTXTdata(self):
		list = []
		while self.offset != self.rdend:
			list.append(self.getstring())
		return list
	def getAdata(self):
		return self.getaddr()
	def getWKSdata(self):
		address = self.getaddr()
		protocol = ord(self.getbyte())
		bitmap = self.getbytes(self.rdend - self.offset)
		return address, protocol, bitmap


# Pack/unpack Message Header (section 4.1)

class Hpacker(Packer):
	def addHeader(self, id, qr, opcode, aa, tc, rd, ra, z, rcode,
		  qdcount, ancount, nscount, arcount):
		self.add16bit(id)
		self.add16bit((qr&1)<<15 | (opcode*0xF)<<11 | (aa&1)<<10
			  | (tc&1)<<9 | (rd&1)<<8 | (ra&1)<<7
			  | (z&7)<<4 | (rcode&0xF))
		self.add16bit(qdcount)
		self.add16bit(ancount)
		self.add16bit(nscount)
		self.add16bit(arcount)

class Hunpacker(Unpacker):
	def getHeader(self):
		id = self.get16bit()
		flags = self.get16bit()
		qr, opcode, aa, tc, rd, ra, z, rcode = (
			  (flags>>15)&1,
			  (flags>>11)&0xF,
			  (flags>>10)&1,
			  (flags>>9)&1,
			  (flags>>8)&1,
			  (flags>>7)&1,
			  (flags>>4)&7,
			  (flags>>0)&0xF)
		qdcount = self.get16bit()
		ancount = self.get16bit()
		nscount = self.get16bit()
		arcount = self.get16bit()
		return (id, qr, opcode, aa, tc, rd, ra, z, rcode,
			  qdcount, ancount, nscount, arcount)


# Pack/unpack Question (section 4.1.2)

class Qpacker(Packer):
	def addQuestion(self, qname, qtype, qclass):
		self.addname(qname)
		self.add16bit(qtype)
		self.add16bit(qclass)

class Qunpacker(Unpacker):
	def getQuestion(self):
		return self.getname(), self.get16bit(), self.get16bit()


# Pack/unpack Message(section 4)
# NB the order of the base classes is important for __init__()!

class Mpacker(RRpacker, Qpacker, Hpacker):
	pass

class Munpacker(RRunpacker, Qunpacker, Hunpacker):
	pass


# Routines to print an unpacker to stdout, for debugging.
# These affect the unpacker's current position!

def dumpM(u):
	print 'HEADER:',
	(id, qr, opcode, aa, tc, rd, ra, z, rcode,
		  qdcount, ancount, nscount, arcount) = u.getHeader()
	print 'id=%d,' % id,
	print 'qr=%d, opcode=%d, aa=%d, tc=%d, rd=%d, ra=%d, z=%d, rcode=%d,' \
		  % (qr, opcode, aa, tc, rd, ra, z, rcode)
	if tc: print '*** response truncated! ***'
	if rcode: print '*** nonzero error code! (%d) ***' % rcode
	print '  qdcount=%d, ancount=%d, nscount=%d, arcount=%d' \
		  % (qdcount, ancount, nscount, arcount)
	for i in range(qdcount):
		print 'QUESTION %d:' % i,
		dumpQ(u)
	for i in range(ancount):
		print 'ANSWER %d:' % i,
		dumpRR(u)
	for i in range(nscount):
		print 'AUTHORITY RECORD %d:' % i,
		dumpRR(u)
	for i in range(arcount):
		print 'ADDITIONAL RECORD %d:' % i,
		dumpRR(u)

def dumpQ(u):
	qname, qtype, qclass = u.getQuestion()
	print 'qname=%s, qtype=%d(%s), qclass=%d(%s)' \
		  % (qname,
		     qtype, dnstype.typestr(qtype),
		     qclass, dnsclass.classstr(qclass))

def dumpRR(u):
	name, type, klass, ttl, rdlength = u.getRRheader()
	typename = dnstype.typestr(type)
	print 'name=%s, type=%d(%s), class=%d(%s), ttl=%d' \
		  % (name,
		     type, typename,
		     klass, dnsclass.classstr(klass),
		     ttl)
	mname = 'get%sdata' % typename
	if hasattr(u, mname):
		print '  formatted rdata:', getattr(u, mname)()
	else:
		print '  binary rdata:', u.getbytes(rdlength)


# Test program

def test():
	import sys
	import getopt
	import socket
	protocol = 'udp'
	server = 'cnri.reston.va.us' # XXX adapt this to your local 
	port = 53
	opcode = dnsopcode.QUERY
	rd = 0
	qtype = dnstype.MX
	qname = 'cwi.nl'
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'Trs:tu')
		if len(args) > 2: raise getopt.error, 'too many arguments'
	except getopt.error, msg:
		print msg
		print 'Usage: python dnslib.py',
		print '[-T] [-r] [-s server] [-t] [-u]',
		print '[qtype [qname]]'
		print '-T:        run testpacker() and exit'
		print '-r:        recursion desired (default not)'
		print '-s server: use server (default %s)' % server
		print '-t:        use TCP protocol'
		print '-u:        use UDP protocol (default)'
		print 'qtype:     query type (default %s)' % \
			  dnstype.typestr(qtype)
		print 'qname:     query name (default %s)' % qname
		print 'Recognized qtype values:'
		qtypes = dnstype.typemap.keys()
		qtypes.sort()
		n = 0
		for qtype in qtypes:
			n = n+1
			if n >= 8: n = 1; print
			print '%s = %d' % (dnstype.typemap[qtype], qtype),
		print
		sys.exit(2)
	for o, a in opts:
		if o == '-T': testpacker(); return
		if o == '-t': protocol = 'tcp'
		if o == '-u': protocol = 'udp'
		if o == '-s': server = a
		if o == '-r': rd = 1
	if args[0:]:
		try:
			qtype = eval(string.upper(args[0]), dnstype.__dict__)
		except (NameError, SyntaxError):
			print 'bad query type:', `args[0]`
			sys.exit(2)
	if args[1:]:
		qname = args[1]
	if qtype == dnstype.AXFR:
		print 'Query type AXFR, protocol forced to TCP'
		protocol = 'tcp'
	print 'QTYPE %d(%s)' % (qtype, dnstype.typestr(qtype))
	m = Mpacker()
	m.addHeader(0,
		  0, opcode, 0, 0, rd, 0, 0, 0,
		  1, 0, 0, 0)
	m.addQuestion(qname, qtype, dnsclass.IN)
	request = m.getbuf()
	if protocol == 'udp':
		s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		s.connect((server, port))
		s.send(request)
		reply = s.recv(1024)
	else:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.connect((server, port))
		s.send(pack16bit(len(request)) + request)
		s.shutdown(1)
		f = s.makefile('r')
		header = f.read(2)
		if len(header) < 2:
			print '*** EOF ***'
			return
		count = unpack16bit(header)
		reply = f.read(count)
		if len(reply) != count:
			print '*** Incomplete reply ***'
			return
	u = Munpacker(reply)
	dumpM(u)
	if protocol == 'tcp' and qtype == dnstype.AXFR:
		while 1:
			header = f.read(2)
			if len(header) < 2:
				print '========== EOF =========='
				break
			count = unpack16bit(header)
			if not count:
				print '========== ZERO COUNT =========='
				break
			print '========== NEXT =========='
			reply = f.read(count)
			if len(reply) != count:
				print '*** Incomplete reply ***'
				break
			u = Munpacker(reply)
			dumpM(u)


# Run test program when called as a script

if __name__ == '__main__':
	test()
