# Mount RPC client -- RFC 1094 (NFS), Appendix A

# This module demonstrates how to write your own RPC client in Python.
# Since there is no RPC compiler for Python (yet), you must first
# create classes derived from Packer and Unpacker to handle the data
# types for the server you want to interface to.  You then write the
# client class.  If you want to support both the TCP and the UDP
# version of a protocol, use multiple inheritance as shown below.


import rpc
from rpc import Packer, Unpacker, TCPClient, UDPClient


# Program number and version for the mount protocol
MOUNTPROG = 100005
MOUNTVERS = 1

# Size of the 'fhandle' opaque structure
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

	# This method is called by Client.__init__ to initialize
	# self.packer and self.unpacker
	def addpackers(self):
		self.packer = MountPacker()
		self.unpacker = MountUnpacker('')

	# This method is called by Client.__init__ to bind the socket
	# to a particular network interface and port.  We use the
	# default network interface, but if we're running as root,
	# we want to bind to a reserved port
	def bindsocket(self):
		import os
		try:
			uid = os.getuid()
		except AttributeError:
			uid = 1
		if uid == 0:
			port = rpc.bindresvport(self.sock, '')
			# 'port' is not used
		else:
			self.sock.bind(('', 0))

	# This function is called to cough up a suitable
	# authentication object for a call to procedure 'proc'.
	def mkcred(self):
		if self.cred == None:
			self.cred = rpc.AUTH_UNIX, rpc.make_auth_unix_default()
		return self.cred

	# The methods Mnt, Dump etc. each implement one Remote
	# Procedure Call.  This is done by calling self.make_call()
	# with as arguments:
	#
	# - the procedure number
	# - the arguments (or None)
	# - the "packer" function for the arguments (or None)
	# - the "unpacker" function for the return value (or None)
	#
	# The packer and unpacker function, if not None, *must* be
	# methods of self.packer and self.unpacker, respectively.
	# A value of None means that there are no arguments or is no
	# return value, respectively.
	#
	# The return value from make_call() is the return value from
	# the remote procedure call, as unpacked by the "unpacker"
	# function, or None if the unpacker function is None.
	#
	# (Even if you expect a result of None, you should still
	# return the return value from make_call(), since this may be
	# needed by a broadcasting version of the class.)
	#
	# If the call fails, make_call() raises an exception
	# (this includes time-outs and invalid results).
	#
	# Note that (at least with the UDP protocol) there is no
	# guarantee that a call is executed at most once.  When you do
	# get a reply, you know it has been executed at least once;
	# when you don't get a reply, you know nothing.

	def Mnt(self, directory):
		return self.make_call(1, directory, \
			self.packer.pack_string, \
			self.unpacker.unpack_fhstatus)

	def Dump(self):
		return self.make_call(2, None, \
			None, self.unpacker.unpack_mountlist)

	def Umnt(self, directory):
		return self.make_call(3, directory, \
			self.packer.pack_string, None)

	def Umntall(self):
		return self.make_call(4, None, None, None)

	def Export(self):
		return self.make_call(5, None, \
			None, self.unpacker.unpack_exportlist)


# We turn the partial Mount client into a full one for either protocol
# by use of multiple inheritance.  (In general, when class C has base
# classes B1...Bn, if x is an instance of class C, methods of x are
# searched first in C, then in B1, then in B2, ..., finally in Bn.)

class TCPMountClient(PartialMountClient, TCPClient):

	def __init__(self, host):
		TCPClient.__init__(self, host, MOUNTPROG, MOUNTVERS)


class UDPMountClient(PartialMountClient, UDPClient):

	def __init__(self, host):
		UDPClient.__init__(self, host, MOUNTPROG, MOUNTVERS)


# A little test program for the Mount client.  This takes a host as
# command line argument (default the local machine), prints its export
# list, and attempts to mount and unmount each exported files system.
# An optional first argument of -t or -u specifies the protocol to use
# (TCP or UDP), default is UDP.

def test():
	import sys
	if sys.argv[1:] and sys.argv[1] == '-t':
		C = TCPMountClient
		del sys.argv[1]
	elif sys.argv[1:] and sys.argv[1] == '-u':
		C = UDPMountClient
		del sys.argv[1]
	else:
		C = UDPMountClient
	if sys.argv[1:]: host = sys.argv[1]
	else: host = ''
	mcl = C(host)
	list = mcl.Export()
	for item in list:
		print item
		try:
			mcl.Mnt(item[0])
		except:
			print 'Sorry'
			continue
		mcl.Umnt(item[0])
