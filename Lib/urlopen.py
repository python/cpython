# Open an arbitrary URL
#
# See the following document for a tentative description of URLs:
#     Uniform Resource Locators              Tim Berners-Lee
#     INTERNET DRAFT                                    CERN
#     IETF URL Working Group                    14 July 1993
#     draft-ietf-uri-url-01.txt
#
# The object returned by urlopen() will differ per protocol.
# All you know is that is has methods read(), fileno(), close() and info().
# The read(), fileno() and close() methods work like those of open files.
# The info() method returns an rfc822.Message object which can be
# used to query various info about the object, if available.
# (rfc822.Message objects are queried with the getheader() method.)

import socket
import regex
import regsub
import string
import rfc822
import ftplib


# External interface -- use urlopen(file) as if it were open(file, 'r')
def urlopen(url):
	url = string.strip(url)
	if url[:1] == '<' and url[-1:] == '>': url = string.strip(url[1:-1])
	if url[:4] == 'URL:': url = string.strip(url[4:])
	type, url = splittype(url)
	if not type: type = 'file'
	type = regsub.gsub('-', '_', type)
	try:
		func = eval('open_' + type)
	except NameError:
		raise IOError, ('url error', 'unknown url type', type)
	try:
		return func(url)
	except socket.error, msg:
		raise IOError, ('socket error', msg)


# Each routine of the form open_<type> knows how to open that type of URL

# Use HTTP protocol
def open_http(url):
	import httplib
	host, selector = splithost(url)
	h = httplib.HTTP(host)
	h.putrequest('GET', selector)
	errcode, errmsg, headers = h.getreply()
	if errcode == 200: return makefile(h.getfile(), headers)
	else: raise IOError, ('http error', errcode, errmsg, headers)

# Empty rfc822.Message object
noheaders = rfc822.Message(open('/dev/null', 'r'))
noheaders.fp.close()			# Recycle file descriptor

# Use Gopher protocol
def open_gopher(url):
	import gopherlib
	host, selector = splithost(url)
	type, selector = splitgophertype(selector)
	selector, query = splitquery(selector)
	if query: fp = gopherlib.send_query(selector, query, host)
	else: fp = gopherlib.send_selector(selector, host)
	return makefile(fp, noheaders)

# Use local file or FTP depending on form of URL
localhost = socket.gethostbyname('localhost')
thishost = socket.gethostbyname(socket.gethostname())
def open_file(url):
	host, file = splithost(url)
	if not host: return makefile(open(file, 'r'), noheaders)
	host, port = splitport(host)
	if not port and socket.gethostbyname(host) in (localhost, thishost):
		try: fp = open(file, 'r')
		except IOError: fp = None
		if fp: return makefile(fp, noheaders)
	return open_ftp(url)

# Use FTP protocol
ftpcache = {}
ftperrors = (ftplib.error_reply,
	     ftplib.error_temp,
	     ftplib.error_perm,
	     ftplib.error_proto)
def open_ftp(url):
	host, file = splithost(url)
	host, port = splitport(host)
	host = socket.gethostbyname(host)
	if not port: port = ftplib.FTP_PORT
	key = (host, port)
	try:
		if not ftpcache.has_key(key):
			ftpcache[key] = ftpwrapper(host, port)
		return makefile(ftpcache[key].retrfile(file), noheaders)
	except ftperrors, msg:
		raise IOError, ('ftp error', msg)


# Utility classes

# Class used to add an info() method to a file object
class makefile:
	def __init__(self, fp, headers):
		self.fp = fp
		self.headers = headers
		self.read = self.fp.read
		self.fileno = self.fp.fileno
		self.close = self.fp.close
	def info(self):
		return self.headers

# Class used by open_ftp() for cache of open FTP connections
class ftpwrapper:
	def __init__(self, host, port):
		self.host = host
		self.port = port
		self.init()
	def init(self):
		self.ftp = ftplib.FTP()
		self.ftp.connect(self.host, self.port)
		self.ftp.login()
	def retrfile(self, file):
		try:
			self.ftp.voidcmd('TYPE I')
		except ftplib.all_errors:
			self.init()
			self.ftp.voidcmd('TYPE I')
		conn = None
		if file:
			try:
				cmd = 'RETR ' + file
				conn = self.ftp.transfercmd(cmd)
			except ftplib.error_perm, reason:
				if reason[:3] != '550':
					raise IOError, ('ftp error', reason)
		if not conn:
			# Try a directory listing
			if file: cmd = 'NLST ' + file
			else: cmd = 'NLST'
			conn = self.ftp.transfercmd(cmd)
		return fakefile(self.ftp, conn)

# Class used by ftpwrapper to handle response when transfer is complete
class fakefile:
	def __init__(self, ftp, conn):
		self.ftp = ftp
		self.conn = conn
		self.fp = self.conn.makefile('r')
		self.read = self.fp.read
		self.fileno = self.fp.fileno
	def __del__(self):
		self.close()
	def close(self):
		self.conn = None
		self.fp = None
		self.read = None
		if self.ftp: self.ftp.voidresp()
		self.ftp = None


# Utilities to split url parts into components:
# splittype('type:opaquestring') --> 'type', 'opaquestring'
# splithost('//host[:port]/path') --> 'host[:port]', '/path'
# splitport('host:port') --> 'host', 'port'
# splitquery('/path?query') --> '/path', 'query'
# splittag('/path#tag') --> '/path', 'tag'
# splitgophertype('/Xselector') --> 'X', 'selector'

typeprog = regex.compile('^\([^/:]+\):\(.*\)$')
def splittype(url):
	if typeprog.match(url) >= 0: return typeprog.group(1, 2)
	return None, url

hostprog = regex.compile('^//\([^/]+\)\(.*\)$')
def splithost(url):
	if hostprog.match(url) >= 0: return hostprog.group(1, 2)
	return None, url

portprog = regex.compile('^\(.*\):\([0-9]+\)$')
def splitport(host):
	if portprog.match(host) >= 0: return portprog.group(1, 2)
	return host, None

queryprog = regex.compile('^\(.*\)\?\([^?]*\)$')
def splitquery(url):
	if queryprog.match(url) >= 0: return queryprog.group(1, 2)
	return url, None

tagprog = regex.compile('^\(.*\)#\([^#]*\)$')
def splittag(url):
	if tagprog.match(url) >= 0: return tagprog.group(1, 2)
	return url, None

def splitgophertype(selector):
	if selector[:1] == '/' and selector[1:2]:
		return selector[1], selector[2:]
	return None, selector


# Test program
def test():
	import sys
	args = sys.argv[1:]
	if not args:
		args = [
			'/etc/passwd',
			'file:/etc/passwd',
			'file://localhost/etc/passwd',
			'ftp://ftp.cwi.nl/etc/passwd',
			'gopher://gopher.cwi.nl/11/',
			'http://www.cwi.nl/index.html',
			]
	for arg in args:
		print '-'*10, arg, '-'*10
		print regsub.gsub('\r', '', urlopen(arg).read())
	print '-'*40

# Run test program when run as a script
if __name__ == '__main__':
	test()
