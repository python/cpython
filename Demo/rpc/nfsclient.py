# NFS RPC client -- RFC 1094

# (See mountclient.py for some hints on how to write RPC clients in
# Python in general)

from rpc import UDPClient, TCPClient
from mountclient import FHSIZE, MountPacker, MountUnpacker

NFS_PROGRAM = 100003
NFS_VERSION = 2

# enum stat
NFS_OK = 0
# (...many error values...)

# enum ftype
NFNON = 0
NFREG = 1
NFDIR = 2
NFBLK = 3
NFCHR = 4
NFLNK = 5


class NFSPacker(MountPacker):

	def pack_sattrargs(self, sa):
		file, attributes = sa
		self.pack_fhandle(file)
		self.pack_sattr(attributes)

	def pack_sattr(self, sa):
		mode, uid, gid, size, atime, mtime = sa
		self.pack_uint(mode)
		self.pack_uint(uid)
		self.pack_uint(gid)
		self.pack_uint(size)
		self.pack_timeval(atime)
		self.pack_timeval(mtime)

	def pack_diropargs(self, da):
		dir, name = da
		self.pack_fhandle(dir)
		self.pack_string(name)

	def pack_readdirargs(self, ra):
		dir, cookie, count = ra
		self.pack_fhandle(dir)
		self.pack_uint(cookie)
		self.pack_uint(count)

	def pack_timeval(self, tv):
		secs, usecs = tv
		self.pack_uint(secs)
		self.pack_uint(usecs)


class NFSUnpacker(MountUnpacker):

	def unpack_readdirres(self):
		status = self.unpack_enum()
		if status == NFS_OK:
			entries = self.unpack_list(self.unpack_entry)
			eof = self.unpack_bool()
			rest = (entries, eof)
		else:
			rest = None
		return (status, rest)

	def unpack_entry(self):
		fileid = self.unpack_uint()
		name = self.unpack_string()
		cookie = self.unpack_uint()
		return (fileid, name, cookie)

	def unpack_diropres(self):
		status = self.unpack_enum()
		if status == NFS_OK:
			fh = self.unpack_fhandle()
			fa = self.unpack_fattr()
			rest = (fh, fa)
		else:
			rest = None
		return (status, rest)

	def unpack_attrstat(self):
		status = self.unpack_enum()
		if status == NFS_OK:
			attributes = self.unpack_fattr()
		else:
			attributes = None
		return status, attributes

	def unpack_fattr(self):
              type = self.unpack_enum()
              mode = self.unpack_uint()
              nlink = self.unpack_uint()
              uid = self.unpack_uint()
              gid = self.unpack_uint()
              size = self.unpack_uint()
              blocksize = self.unpack_uint()
              rdev = self.unpack_uint()
              blocks = self.unpack_uint()
              fsid = self.unpack_uint()
              fileid = self.unpack_uint()
              atime = self.unpack_timeval()
              mtime = self.unpack_timeval()
              ctime = self.unpack_timeval()
	      return (type, mode, nlink, uid, gid, size, blocksize, \
		      rdev, blocks, fsid, fileid, atime, mtime, ctime)

	def unpack_timeval(self):
		secs = self.unpack_uint()
		usecs = self.unpack_uint()
		return (secs, usecs)


class NFSClient(UDPClient):

	def init(self, host):
		return UDPClient.init(self, host, NFS_PROGRAM, NFS_VERSION)

	def addpackers(self):
		self.packer = NFSPacker().init()
		self.unpacker = NFSUnpacker().init('')

	def Getattr(self, fh):
		self.start_call(1)
		self.packer.pack_fhandle(fh)
		self.do_call()
		as = self.unpacker.unpack_attrstat()
		self.end_call()
		return as

	def Setattr(self, sa):
		self.start_call(2)
		self.packer.pack_sattrargs(sa)
		self.do_call()
		as = self.unpacker.unpack_attrstat()
		self.end_call()
		return as

	# Root() is obsolete

	def Lookup(self, da):
		self.start_call(4)
		self.packer.pack_diropargs(da)
		self.do_call()
		dr = self.unpacker.unpack_diropres()
		self.end_call()
		return dr

	# ...

	def Readdir(self, ra):
		self.start_call(16)
		self.packer.pack_readdirargs(ra)
		self.do_call()
		rr = self.unpacker.unpack_readdirres()
		self.end_call()
		return rr

	# Shorthand to get the entire contents of a directory
	def Listdir(self, dir):
		list = []
		ra = (dir, 0, 16)
		while 1:
			(status, rest) = self.Readdir(ra)
			if status <> NFS_OK:
				break
			entries, eof = rest
			last_cookie = None
			for fileid, name, cookie in entries:
				print (fileid, name, cookie) # XXX
				list.append(fileid, name)
				last_cookie = cookie
			if eof or not last_cookie:
				break
			ra = (ra[0], last_cookie, ra[2])
		return list

	
def test():
	import sys
	if sys.argv[1:]: host = sys.argv[1]
	else: host = ''
	if sys.argv[2:]: filesys = sys.argv[2]
	else: filesys = None
	from mountclient import UDPMountClient, TCPMountClient
	mcl = TCPMountClient().init(host)
	if filesys == None:
		list = mcl.Export()
		for item in list:
			print item
		return
	sf = mcl.Mnt(filesys)
	print sf
	fh = sf[1]
	if fh:
		ncl = NFSClient().init(host)
		as = ncl.Getattr(fh)
		print as
		list = ncl.Listdir(fh)
		for item in list: print item
		mcl.Unmnt(filesys)
	
test()
