"""Various tools used by MIME-reading or MIME-writing programs."""


import os
import rfc822
import string
import tempfile


class Message(rfc822.Message):
	"""A derived class of rfc822.Message that knows about MIME headers and
	contains some hooks for decoding encoded and multipart messages."""

	def __init__(self, fp, seekable = 1):
		rfc822.Message.__init__(self, fp, seekable)
		self.encodingheader = \
			self.getheader('content-transfer-encoding')
		self.typeheader = \
			self.getheader('content-type')
		self.parsetype()
		self.parseplist()

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
			str = str[end:]

	def getplist(self):
		return self.plist

	def getparam(self, name):
		name = string.lower(name) + '='
		n = len(name)
		for p in self.plist:
			if p[:n] == name:
				return rfc822.unquote(p[n:])
		return None

	def getparamnames(self):
		result = []
		for p in self.plist:
			i = string.find(p, '=')
			if i >= 0:
				result.append(string.lower(p[:i]))
		return result

	def getencoding(self):
		if self.encodingheader == None:
			return '7bit'
		return string.lower(self.encodingheader)

	def gettype(self):
		return self.type

	def getmaintype(self):
		return self.maintype

	def getsubtype(self):
		return self.subtype




# Utility functions
# -----------------


_prefix = None

def choose_boundary():
	"""Return a random string usable as a multipart boundary.
	The method used is so that it is *very* unlikely that the same
	string of characters will every occur again in the Universe,
	so the caller needn't check the data it is packing for the
	occurrence of the boundary.

	The boundary contains dots so you have to quote it in the header."""

	global _prefix
	import time
	import random
	if _prefix == None:
		import socket
		import os
		hostid = socket.gethostbyname(socket.gethostname())
		try:
		    uid = `os.getuid()`
		except:
		    uid = '1'
		try:
		    pid = `os.getpid()`
		except:
		    pid = '1'
		_prefix = hostid + '.' + uid + '.' + pid
	timestamp = '%.3f' % time.time()
	seed = `random.randint(0, 32767)`
	return _prefix + '.' + timestamp + '.' + seed


# Subroutines for decoding some common content-transfer-types

def decode(input, output, encoding):
	"""Decode common content-transfer-encodings (base64, quopri, uuencode)."""
	if encoding == 'base64':
		import base64
		return base64.decode(input, output)
	if encoding == 'quoted-printable':
		import quopri
		return quopri.decode(input, output)
	if encoding in ('uuencode', 'x-uuencode', 'uue', 'x-uue'):
		import uu
		return uu.decode(input, output)
	if encoding in ('7bit', '8bit'):
		return output.write(input.read())
	if decodetab.has_key(encoding):
		pipethrough(input, decodetab[encoding], output)
	else:
		raise ValueError, \
		      'unknown Content-Transfer-Encoding: %s' % encoding

def encode(input, output, encoding):
	"""Encode common content-transfer-encodings (base64, quopri, uuencode)."""
	if encoding == 'base64':
		import base64
		return base64.encode(input, output)
	if encoding == 'quoted-printable':
		import quopri
		return quopri.encode(input, output, 0)
	if encoding in ('uuencode', 'x-uuencode', 'uue', 'x-uue'):
		import uu
		return uu.encode(input, output)
	if encoding in ('7bit', '8bit'):
		return output.write(input.read())
	if encodetab.has_key(encoding):
		pipethrough(input, encodetab[encoding], output)
	else:
		raise ValueError, \
		      'unknown Content-Transfer-Encoding: %s' % encoding

# The following is no longer used for standard encodings

# XXX This requires that uudecode and mmencode are in $PATH

uudecode_pipe = '''(
TEMP=/tmp/@uu.$$
sed "s%^begin [0-7][0-7]* .*%begin 600 $TEMP%" | uudecode
cat $TEMP
rm $TEMP
)'''

decodetab = {
	'uuencode':		uudecode_pipe,
	'x-uuencode':		uudecode_pipe,
	'uue':			uudecode_pipe,
	'x-uue':		uudecode_pipe,
	'quoted-printable':	'mmencode -u -q',
	'base64':		'mmencode -u -b',
}

encodetab = {
	'x-uuencode':		'uuencode tempfile',
	'uuencode':		'uuencode tempfile',
	'x-uue':		'uuencode tempfile',
	'uue':			'uuencode tempfile',
	'quoted-printable':	'mmencode -q',
	'base64':		'mmencode -b',
}

def pipeto(input, command):
	pipe = os.popen(command, 'w')
	copyliteral(input, pipe)
	pipe.close()

def pipethrough(input, command, output):
	tempname = tempfile.mktemp()
	try:
		temp = open(tempname, 'w')
	except IOError:
		print '*** Cannot create temp file', `tempname`
		return
	copyliteral(input, temp)
	temp.close()
	pipe = os.popen(command + ' <' + tempname, 'r')
	copybinary(pipe, output)
	pipe.close()
	os.unlink(tempname)

def copyliteral(input, output):
	while 1:
		line = input.readline()
		if not line: break
		output.write(line)

def copybinary(input, output):
	BUFSIZE = 8192
	while 1:
		line = input.read(BUFSIZE)
		if not line: break
		output.write(line)
