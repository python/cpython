# Mount RPC client -- RFC 1094 (NFS), Appendix A

# This module demonstrates how to write your own RPC client in Python.
# Since there is no RPC compiler for Python (yet), you must first
# create classes derived from Packer and Unpacker to handle the data
# types for the server you want to interface to.  You then write the
# client class.  If you want to support both the TCP and the UDP
# version of a protocol, use multiple inheritance as shown below.


import rpc
from rpc import Packer, Unpacker, TCPClient, UDPClient

MOUNTPROG = 100005
MOUNTVERS = 1

FHSIZE = 32


# Packer derived class for Mount protocol clients.
# The only thing we need to pack beyond basic types is an 'fhandle'

class MountPacker(Packer):

	def pack_fhandle(self, fhandle):
		self.pack_fopaque(FHSIZE, fhandle)


# Unpacker derived class for Mount protocol clients.
# The important types we need to unpack are fhandle, fhstatus,
# mountlist and exportlist; mountstruct, exportstruct and groups are
# used to unpack components of mountlist and exportlist and the
# corresponding functions are passed as function argument to the
# generic unpack_list function.

class MountUnpacker(Unpacker):

	def unpack_fhandle(self):
		return self.unpack_fopaque(FHSIZE)

	def unpack_fhstatus(self):
		status = self.unpack_uint()
		if status == 0:
			fh = self.unpack_fhandle()
		else:
			fh = None
		return status, fh

	def unpack_mountlist(self):
		return self.unpack_list(self.unpack_mountstruct)

	def unpack_mountstruct(self):
		hostname = self.unpack_string()
		directory = self.unpack_string()
		return (hostname, directory)

	def unpack_exportlist(self):
		return self.unpack_list(self.unpack_exportstruct)

	def unpack_exportstruct(self):
		filesys = self.unpack_string()
		groups = self.unpack_groups()
		return (filesys, groups)

	def unpack_groups(self):
		return self.unpack_list(self.unpack_string)


# These are the procedures specific to the Mount client class.
# Think of this as a derived class of either TCPClient or UDPClient.

class PartialMountClient:

	# This method is called by Client.init to initialize
	# self.packer and self.unpacker
	def addpackers(self):
		self.packer = MountPacker().init()
		self.unpacker = MountUnpacker().init('')

	# This function is called to gobble up a suitable
	# authentication object for a call to procedure 'proc'.
	# (Experiments suggest that for Mnt/Umnt, Unix authentication
	# is necessary, while the other calls require no
	# authentication.)
	def mkcred(self, proc):
		if proc not in (1, 3, 4): # not Mnt/Umnt/Umntall
			return rpc.AUTH_NULL, ''
		if self.cred == None:
			self.cred = rpc.AUTH_UNIX, rpc.make_auth_unix_default()
		return self.cred

	# The methods Mnt, Dump etc. each implement one Remote
	# Procedure Call.  Their general structure is
	#  self.start_call(<procedure-number>)
	#  <pack arguments using self.packer>
	#  self.do_call()	# This does the actual message exchange
	#  <unpack reply using self.unpacker>
	#  self.end_call()
	#  return <reply>
	# If the call fails, an exception is raised by do_call().
	# If the reply does not match what you unpack, an exception is
	# raised either during unpacking (if you overrun the buffer)
	# or by end_call() (if you leave values in the buffer).
	# Calling packer methods with invalid arguments (e.g. if
	# invalid arguments were passed from outside) will also result
	# in exceptions during packing.

	def Mnt(self, directory):
		self.start_call(1)
		self.packer.pack_string(directory)
		self.do_call()
		stat = self.unpacker.unpack_fhstatus()
		self.end_call()
		return stat

	def Dump(self):
		self.start_call(2)
		self.do_call()
		list = self.unpacker.unpack_mountlist()
		self.end_call()
		return list

	def Umnt(self, directory):
		self.start_call(3)
		self.packer.pack_string(directory)
		self.do_call()
		self.end_call()

	def Umntall(self):
		self.start_call(4)
		self.do_call()
		self.end_call()

	def Export(self):
		self.start_call(5)
		self.do_call()
		list = self.unpacker.unpack_exportlist()
		self.end_call()
		return list


# We turn the partial Mount client into a full one for either protocol
# by use of multiple inheritance.  (In general, when class C has base
# classes B1...Bn, if x is an instance of class C, methods of x are
# searched first in C, then in B1, then in B2, ..., finally in Bn.)

class TCPMountClient(PartialMountClient, TCPClient):

	def init(self, host):
		return TCPClient.init(self, host, MOUNTPROG, MOUNTVERS)


class UDPMountClient(PartialMountClient, UDPClient):

	def init(self, host):
		return UDPClient.init(self, host, MOUNTPROG, MOUNTVERS)


# A little test program for the Mount client.  This takes a host as
# command line argument (default the local machine), prints its export
# list, and attempt to mount and unmount each exported files system.

def test():
	import sys
	if sys.argv[1:]: host = sys.argv[1]
	else: host = ''
	mcl = UDPMountClient().init(host)
	list = mcl.Export()
	for item in list:
		print item
		try:
			mcl.Mnt(item[0])
		except:
			print 'Sorry'
			continue
		mcl.Umnt(item[0])
	return
