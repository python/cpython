# HTTP client class
#
# See the following document for a tentative protocol description:
#     Hypertext Transfer Protocol (HTTP)        Tim Berners-Lee, CERN
#     Internet Draft                                       5 Nov 1993
#     draft-ietf-iiir-http-00.txt                  Expires 5 May 1994
#
# Example:
#
# >>> from httplib import HTTP
# >>> h = HTTP('www.cwi.nl')
# >>> h.putreqest('GET', '/index.html')
# >>> h.putheader('Accept', 'text/html')
# >>> h.putheader('Accept', 'text/plain')
# >>> errcode, errmsg, headers = h.getreply()
# >>> if errcode == 200:
# ...     f = h.getfile()
# ...     print f.read() # Print the raw HTML
# ...
# <TITLE>Home Page of CWI, Amsterdam</TITLE>
# [...many more lines...]
# >>>
#
# Note that an HTTP object is used for a single request -- to issue a
# second request to the same server, you create a new HTTP object.
# (This is in accordance with the protocol, which uses a new TCP
# connection for each request.)


import os
import socket
import string
import regex
import regsub
import rfc822

HTTP_VERSION = 'HTTP/1.0'
HTTP_PORT = 80

replypat = regsub.gsub('\\.', '\\\\.', HTTP_VERSION) + \
	  '[ \t]+\([0-9][0-9][0-9]\)\(.*\)'
replyprog = regex.compile(replypat)

class HTTP:

	def __init__(self, *args):
		self.debuglevel = 0
		if args: apply(self.connect, args)

	def set_debuglevel(self, debuglevel):
		self.debuglevel = debuglevel

	def connect(self, host, *args):
		if args:
			if args[1:]: raise TypeError, 'too many args'
			port = args[0]
		else:
			i = string.find(host, ':')
			port = None
			if i >= 0:
				host, port = host[:i], host[i+1:]
				try: port = string.atoi(port)
				except string.atoi_error: pass
		if not port: port = HTTP_PORT
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		if self.debuglevel > 0: print 'connect:', (host, port)
		self.sock.connect(host, port)

	def send(self, str):
		if self.debuglevel > 0: print 'send:', `str`
		self.sock.send(str)

	def putrequest(self, request, selector):
		str = '%s %s %s\r\n' % (request, selector, HTTP_VERSION)
		self.send(str)

	def putheader(self, header, *args):
		str = '%s: %s\r\n' % (header, string.joinfields(args,'\r\n\t'))
		self.send(str)

	def endheaders(self):
		self.send('\r\n')

	def endrequest(self):
		if self.debuglevel > 0: print 'shutdown: 1'
		self.sock.shutdown(1)

	def getreply(self):
		self.endrequest()
		self.file = self.sock.makefile('r')
		line = self.file.readline()
		if self.debuglevel > 0: print 'reply:', `line`
		if replyprog.match(line) < 0:
			self.headers = None
			return -1, line, self.headers
		errcode, errmsg = replyprog.group(1, 2)
		errcode = string.atoi(errcode)
		errmsg = string.strip(errmsg)
		self.headers = rfc822.Message(self.file)
		return errcode, errmsg, self.headers

	def getfile(self):
		return self.file


def test():
	import sys
	import getopt
	opts, args = getopt.getopt(sys.argv[1:], 'd')
	dl = 0
	for o, a in opts:
		if o == '-d': dl = dl + 1
	host = 'www.cwi.nl:80'
	selector = '/index.html'
	if args[0:]: host = args[0]
	if args[1:]: selector = args[1]
	h = HTTP()
	h.set_debuglevel(dl)
	h.connect(host)
	h.putrequest('GET', selector)
	errcode, errmsg, headers = h.getreply()
	print 'errcode =', errcode
	print 'headers =', headers
	print 'errmsg  =', errmsg
	if headers:
		for header in headers.headers: print string.strip(header)
	print h.getfile().read()

if __name__ == '__main__':
	test()
