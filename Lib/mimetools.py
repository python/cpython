# Various tools used by MIME-reading or MIME-writing programs.


import os
import rfc822
import string
import tempfile


# A derived class of rfc822.Message that knows about MIME headers and
# contains some hooks for decoding encoded and multipart messages.

class Message(rfc822.Message):

	def __init__(self, fp):
		rfc822.Message.__init__(self, fp)
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
		return string.lower(self.encodingheader)

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
	timestamp = `int(time.time())`
	seed = `rand.rand()`
	return _prefix + '.' + timestamp + '.' + seed


# Subroutines for decoding some common content-transfer-types

# XXX This requires that uudecode and mmencode are in $PATH

def decode(input, output, encoding):
	if decodetab.has_key(encoding):
		pipethrough(input, decodetab[encoding], output)
	else:
		raise ValueError, \
		      'unknown Content-Transfer-Encoding: %s' % encoding

def encode(input, output, encoding):
	if encodetab.has_key(encoding):
		pipethrough(input, encodetab[encoding], output)
	else:
		raise ValueError, \
		      'unknown Content-Transfer-Encoding: %s' % encoding

uudecode_pipe = '''(
TEMP=/tmp/@uu.$$
sed "s%^begin [0-7][0-7]* .*%begin 600 $TEMP%" | uudecode
cat $TEMP
rm $TEMP
)'''

decodetab = {
	'uuencode':		uudecode_pipe,
	'x-uuencode':		uudecode_pipe,
	'quoted-printable':	'mmencode -u -q',
	'base64':		'mmencode -u -b',
}

encodetab = {
	'x-uuencode':		'uuencode tempfile',
	'uuencode':		'uuencode tempfile',
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
