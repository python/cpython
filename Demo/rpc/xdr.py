# Implement (a subset of) Sun XDR -- RFC1014.


try:
	import struct
except ImportError:
	struct = None


Long = type(0L)


class Packer:

	def init(self):
		self.reset()
		return self

	def reset(self):
		self.buf = ''

	def get_buf(self):
		return self.buf

	def pack_uint(self, x):
		self.buf = self.buf + \
			(chr(int(x>>24 & 0xff)) + chr(int(x>>16 & 0xff)) + \
			 chr(int(x>>8 & 0xff)) + chr(int(x & 0xff)))
	if struct and struct.pack('l', 1) == '\0\0\0\1':
		def pack_uint(self, x):
			if type(x) == Long:
				x = int((x + 0x80000000L) % 0x100000000L \
					   - 0x80000000L)
			self.buf = self.buf + struct.pack('l', x)

	pack_int = pack_uint

	pack_enum = pack_int

	def pack_bool(self, x):
		if x: self.buf = self.buf + '\0\0\0\1'
		else: self.buf = self.buf + '\0\0\0\0'

	def pack_uhyper(self, x):
		self.pack_uint(int(x>>32 & 0xffffffff))
		self.pack_uint(int(x & 0xffffffff))

	pack_hyper = pack_uhyper

	def pack_fstring(self, n, s):
		if n < 0:
			raise ValueError, 'fstring size must be nonnegative'
		n = ((n+3)/4)*4
		data = s[:n]
		data = data + (n - len(data)) * '\0'
		self.buf = self.buf + data

	pack_fopaque = pack_fstring

	def pack_string(self, s):
		n = len(s)
		self.pack_uint(n)
		self.pack_fstring(n, s)

	pack_opaque = pack_string

	def pack_list(self, list, pack_item):
		for item in list:
			self.pack_uint(1)
			pack_item(list)
		self.pack_uint(0)


class Unpacker:

	def init(self, data):
		self.reset(data)
		return self

	def reset(self, data):
		self.buf = data
		self.pos = 0

	def done(self):
		if self.pos < len(self.buf):
			raise RuntimeError, 'unextracted data remains'

	def unpack_uint(self):
		i = self.pos
		self.pos = j = i+4
		data = self.buf[i:j]
		if len(data) < 4:
			raise EOFError
		x = long(ord(data[0]))<<24 | ord(data[1])<<16 | \
			ord(data[2])<<8 | ord(data[3])
		# Return a Python long only if the value is not representable
		# as a nonnegative Python int
		if x < 0x80000000L: x = int(x)
		return x
	if struct and struct.unpack('l', '\0\0\0\1') == 1:
		def unpack_uint(self):
			i = self.pos
			self.pos = j = i+4
			data = self.buf[i:j]
			if len(data) < 4:
				raise EOFError
			return struct.unpack('l', data)

	def unpack_int(self):
		x = self.unpack_uint()
		if x >= 0x80000000L: x = x - 0x100000000L
		return int(x)

	unpack_enum = unpack_int

	unpack_bool = unpack_int

	def unpack_uhyper(self):
		hi = self.unpack_uint()
		lo = self.unpack_uint()
		return long(hi)<<32 | lo

	def unpack_hyper(self):
		x = self.unpack_uhyper()
		if x >= 0x8000000000000000L: x = x - 0x10000000000000000L
		return x

	def unpack_fstring(self, n):
		if n < 0:
			raise ValueError, 'fstring size must be nonnegative'
		i = self.pos
		j = i + (n+3)/4*4
		if j > len(self.buf):
			raise EOFError
		self.pos = j
		return self.buf[i:i+n]

	unpack_fopaque = unpack_fstring

	def unpack_string(self):
		n = self.unpack_uint()
		return self.unpack_fstring(n)

	unpack_opaque = unpack_string

	def unpack_list(self, unpack_item):
		list = []
		while 1:
			x = self.unpack_uint()
			if not x: break
			if x <> 1:
				raise RuntimeError, \
					'0 or 1 expected, got ' + `x`
			list.append(unpack_item())
		return list
