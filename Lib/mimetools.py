# Various tools used by MIME-reading or MIME-writing programs.


import string
import rfc822


# A derived class of rfc822.Message that knows about MIME headers and
# contains some hooks for decoding encoded and multipart messages.

class Message(rfc822.Message):

	def init(self, fp):
		self = rfc822.Message.init(self, fp)
		self.encodingheader = \
			self.getheader('content-transfer-encoding')
		self.typeheader = \
			self.getheader('content-type')
		self.parsetype()
		self.parseplist()
		return self

	def parsetype(self):
		str = self.typeheader
		if str == None:
			str = 'text/plain'
		if ';' in str:
			i = string.index(str, ';')
			self.plisttext = str[i:]
			str = str[:i]
		else:
			self.plisttext = ''
		fields = string.splitfields(str, '/')
		for i in range(len(fields)):
			fields[i] = string.lower(string.strip(fields[i]))
		self.type = string.joinfields(fields, '/')
		self.maintype = fields[0]
		self.subtype = string.joinfields(fields[1:], '/')

	def parseplist(self):
		str = self.plisttext
		self.plist = []
		while str[:1] == ';':
			str = str[1:]
			if ';' in str:
				# XXX Should parse quotes!
				end = string.index(str, ';')
			else:
				end = len(str)
			f = str[:end]
			if '=' in f:
				i = string.index(f, '=')
				f = string.lower(string.strip(f[:i])) + \
					'=' + string.strip(f[i+1:])
			self.plist.append(string.strip(f))

	def getplist(self):
		return self.plist

	def getparam(self, name):
		name = string.lower(name) + '='
		n = len(name)
		for p in self.plist:
			if p[:n] == name:
				return rfc822.unquote(p[n:])
		return None

	def getencoding(self):
		if self.encodingheader == None:
			return '7bit'
		return self.encodingheader

	def gettype(self):
		return self.type

	def getmaintype(self):
		return self.maintype

	def getsubtype(self):
		return self.subtype




# Utility functions
# -----------------


# Return a random string usable as a multipart boundary.
# The method used is so that it is *very* unlikely that the same
# string of characters will every occur again in the Universe,
# so the caller needn't check the data it is packing for the
# occurrence of the boundary.
#
# The boundary contains dots so you have to quote it in the header.

_prefix = None

def choose_boundary():
	global _generation, _prefix, _timestamp
	import time
	import rand
	if _prefix == None:
		import socket
		import os
		hostid = socket.gethostbyname(socket.gethostname())
		uid = `os.getuid()`
		pid = `os.getpid()`
		seed = `rand.rand()`
		_prefix = hostid + '.' + uid + '.' + pid
	timestamp = `time.time()`
	seed = `rand.rand()`
	return _prefix + '.' + timestamp + '.' + seed
