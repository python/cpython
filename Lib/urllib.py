# Open an arbitrary URL
#
# See the following document for more info on URLs:
# "Names and Addresses, URIs, URLs, URNs, URCs", at
# http://www.w3.org/pub/WWW/Addressing/Overview.html
#
# See also the HTTP spec (from which the error codes are derived):
# "HTTP - Hypertext Transfer Protocol", at
# http://www.w3.org/pub/WWW/Protocols/
#
# Related standards and specs:
# - RFC1808: the "relative URL" spec. (authoritative status)
# - RFC1738 - the "URL standard". (authoritative status)
# - RFC1630 - the "URI spec". (informational status)
#
# The object returned by URLopener().open(file) will differ per
# protocol.  All you know is that is has methods read(), readline(),
# readlines(), fileno(), close() and info().  The read*(), fileno()
# and close() methods work like those of open files. 
# The info() method returns a mimetools.Message object which can be
# used to query various info about the object, if available.
# (mimetools.Message objects are queried with the getheader() method.)

import string
import socket
import regex
import os
import sys


__version__ = '1.6'

# Helper for non-unix systems
if os.name == 'mac':
	from macurl2path import url2pathname, pathname2url
elif os.name == 'nt':    
	from nturl2path import url2pathname, pathname2url 
else:
	def url2pathname(pathname):
		return pathname
	def pathname2url(pathname):
		return pathname

# This really consists of two pieces:
# (1) a class which handles opening of all sorts of URLs
#     (plus assorted utilities etc.)
# (2) a set of functions for parsing URLs
# XXX Should these be separated out into different modules?


# Shortcut for basic usage
_urlopener = None
def urlopen(url, data=None):
	global _urlopener
	if not _urlopener:
		_urlopener = FancyURLopener()
	if data is None:
		return _urlopener.open(url)
	else:
		return _urlopener.open(url, data)
def urlretrieve(url, filename=None):
	global _urlopener
	if not _urlopener:
		_urlopener = FancyURLopener()
	if filename:
	    return _urlopener.retrieve(url, filename)
	else:
	    return _urlopener.retrieve(url)
def urlcleanup():
	if _urlopener:
		_urlopener.cleanup()


# Class to open URLs.
# This is a class rather than just a subroutine because we may need
# more than one set of global protocol-specific options.
# Note -- this is a base class for those who don't want the
# automatic handling of errors type 302 (relocated) and 401
# (authorization needed).
ftpcache = {}
class URLopener:

	tempcache = None		# So close() in __del__() won't fail

	# Constructor
	def __init__(self, proxies=None):
		if proxies is None:
			proxies = getproxies()
		self.proxies = proxies
		server_version = "Python-urllib/%s" % __version__
		self.addheaders = [('User-agent', server_version)]
		self.tempcache = None
		# Undocumented feature: if you assign {} to tempcache,
		# it is used to cache files retrieved with
		# self.retrieve().  This is not enabled by default
		# since it does not work for changing documents (and I
		# haven't got the logic to check expiration headers
		# yet).
		self.ftpcache = ftpcache
		# Undocumented feature: you can use a different
		# ftp cache by assigning to the .ftpcache member;
		# in case you want logically independent URL openers

	def __del__(self):
		self.close()

	def close(self):
		self.cleanup()

	def cleanup(self):
		if self.tempcache:
			import os
			for url in self.tempcache.keys():
				try:
					os.unlink(self.tempcache[url][0])
				except os.error:
					pass
				del self.tempcache[url]

	# Add a header to be used by the HTTP interface only
	# e.g. u.addheader('Accept', 'sound/basic')
	def addheader(self, *args):
		self.addheaders.append(args)

	# External interface
	# Use URLopener().open(file) instead of open(file, 'r')
	def open(self, fullurl, data=None):
		fullurl = unwrap(fullurl)
		type, url = splittype(fullurl)
 		if not type: type = 'file'
		self.openedurl = '%s:%s' % (type, url)
		if self.proxies.has_key(type):
			proxy = self.proxies[type]
			type, proxy = splittype(proxy)
			host, selector = splithost(proxy)
			url = (host, fullurl) # Signal special case to open_*()
		name = 'open_' + type
		if '-' in name:
			import regsub
			name = regsub.gsub('-', '_', name)
		if not hasattr(self, name):
			if data is None:
				return self.open_unknown(fullurl)
			else:
				return self.open_unknown(fullurl, data)
		try:
			if data is None:
				return getattr(self, name)(url)
			else:
				return getattr(self, name)(url, data)
		except socket.error, msg:
			raise IOError, ('socket error', msg), sys.exc_traceback

	# Overridable interface to open unknown URL type
	def open_unknown(self, fullurl, data=None):
		type, url = splittype(fullurl)
		raise IOError, ('url error', 'unknown url type', type)

	# External interface
	# retrieve(url) returns (filename, None) for a local object
	# or (tempfilename, headers) for a remote object
	def retrieve(self, url, filename=None):
		if self.tempcache and self.tempcache.has_key(url):
			return self.tempcache[url]
		url1 = unwrap(url)
		self.openedurl = url1
		if self.tempcache and self.tempcache.has_key(url1):
			self.tempcache[url] = self.tempcache[url1]
			return self.tempcache[url1]
		type, url1 = splittype(url1)
		if not filename and (not type or type == 'file'):
			try:
				fp = self.open_local_file(url1)
				del fp
				return url2pathname(splithost(url1)[1]), None
			except IOError, msg:
				pass
		fp = self.open(url)
		headers = fp.info()
		if not filename:
		    import tempfile
		    filename = tempfile.mktemp()
		result = filename, headers
		if self.tempcache is not None:
			self.tempcache[url] = result
		tfp = open(filename, 'wb')
		bs = 1024*8
		block = fp.read(bs)
		while block:
			tfp.write(block)
			block = fp.read(bs)
		del fp
		del tfp
		return result

	# Each method named open_<type> knows how to open that type of URL

	# Use HTTP protocol
	def open_http(self, url, data=None):
		import httplib
		if type(url) is type(""):
			host, selector = splithost(url)
			user_passwd, host = splituser(host)
		else:
			host, selector = url
			urltype, rest = splittype(selector)
			user_passwd = None
			if string.lower(urltype) == 'http':
			    realhost, rest = splithost(rest)
			    user_passwd, realhost = splituser(realhost)
			    if user_passwd:
				selector = "%s://%s%s" % (urltype,
							  realhost, rest)
			#print "proxy via http:", host, selector
		if not host: raise IOError, ('http error', 'no host given')
		if user_passwd:
			import base64
			auth = string.strip(base64.encodestring(user_passwd))
		else:
			auth = None
		h = httplib.HTTP(host)
		if data is not None:
			h.putrequest('POST', selector)
			h.putheader('Content-type',
				    'application/x-www-form-urlencoded')
			h.putheader('Content-length', '%d' % len(data))
		else:
			h.putrequest('GET', selector)
		if auth: h.putheader('Authorization', 'Basic %s' % auth)
		for args in self.addheaders: apply(h.putheader, args)
		h.endheaders()
		if data is not None:
			h.send(data + '\r\n')
		errcode, errmsg, headers = h.getreply()
		fp = h.getfile()
		if errcode == 200:
			return addinfourl(fp, headers, self.openedurl)
		else:
			return self.http_error(url,
					       fp, errcode, errmsg, headers)

	# Handle http errors.
	# Derived class can override this, or provide specific handlers
	# named http_error_DDD where DDD is the 3-digit error code
	def http_error(self, url, fp, errcode, errmsg, headers):
		# First check if there's a specific handler for this error
		name = 'http_error_%d' % errcode
		if hasattr(self, name):
			method = getattr(self, name)
			result = method(url, fp, errcode, errmsg, headers)
			if result: return result
		return self.http_error_default(
			url, fp, errcode, errmsg, headers)

	# Default http error handler: close the connection and raises IOError
	def http_error_default(self, url, fp, errcode, errmsg, headers):
		void = fp.read()
		fp.close()
		raise IOError, ('http error', errcode, errmsg, headers)

	# Use Gopher protocol
	def open_gopher(self, url):
		import gopherlib
		host, selector = splithost(url)
		if not host: raise IOError, ('gopher error', 'no host given')
		type, selector = splitgophertype(selector)
		selector, query = splitquery(selector)
		selector = unquote(selector)
		if query:
			query = unquote(query)
			fp = gopherlib.send_query(selector, query, host)
		else:
			fp = gopherlib.send_selector(selector, host)
		return addinfourl(fp, noheaders(), self.openedurl)

	# Use local file or FTP depending on form of URL
	def open_file(self, url):
		if url[:2] == '//':
			return self.open_ftp(url)
		else:
			return self.open_local_file(url)

	# Use local file
	def open_local_file(self, url):
		host, file = splithost(url)
		if not host:
			return addinfourl(open(url2pathname(file), 'rb'), noheaders(), 'file:'+file)
		host, port = splitport(host)
		if not port and socket.gethostbyname(host) in (
			  localhost(), thishost()):
			file = unquote(file)
			return addinfourl(open(url2pathname(file), 'rb'), noheaders(), 'file:'+file)
		raise IOError, ('local file error', 'not on local host')

	# Use FTP protocol
	def open_ftp(self, url):
		host, path = splithost(url)
		if not host: raise IOError, ('ftp error', 'no host given')
		host, port = splitport(host)
		user, host = splituser(host)
		if user: user, passwd = splitpasswd(user)
		else: passwd = None
		host = socket.gethostbyname(host)
		if not port:
			import ftplib
			port = ftplib.FTP_PORT
		path, attrs = splitattr(path)
		dirs = string.splitfields(path, '/')
		dirs, file = dirs[:-1], dirs[-1]
		if dirs and not dirs[0]: dirs = dirs[1:]
		key = (user, host, port, string.joinfields(dirs, '/'))
		try:
			if not self.ftpcache.has_key(key):
				self.ftpcache[key] = \
						   ftpwrapper(user, passwd,
							      host, port, dirs)
			if not file: type = 'D'
			else: type = 'I'
			for attr in attrs:
				attr, value = splitvalue(attr)
				if string.lower(attr) == 'type' and \
				   value in ('a', 'A', 'i', 'I', 'd', 'D'):
					type = string.upper(value)
			return addinfourl(self.ftpcache[key].retrfile(file, type),
				  noheaders(), self.openedurl)
		except ftperrors(), msg:
			raise IOError, ('ftp error', msg), sys.exc_traceback


# Derived class with handlers for errors we can handle (perhaps)
class FancyURLopener(URLopener):

	def __init__(self, *args):
		apply(URLopener.__init__, (self,) + args)
		self.auth_cache = {}

	# Default error handling -- don't raise an exception
	def http_error_default(self, url, fp, errcode, errmsg, headers):
	    return addinfourl(fp, headers, self.openedurl)

	# Error 302 -- relocated (temporarily)
	def http_error_302(self, url, fp, errcode, errmsg, headers):
		# XXX The server can force infinite recursion here!
		if headers.has_key('location'):
			newurl = headers['location']
		elif headers.has_key('uri'):
			newurl = headers['uri']
		else:
			return
		void = fp.read()
		fp.close()
		return self.open(newurl)

	# Error 301 -- also relocated (permanently)
	http_error_301 = http_error_302

	# Error 401 -- authentication required
	# See this URL for a description of the basic authentication scheme:
	# http://www.ics.uci.edu/pub/ietf/http/draft-ietf-http-v10-spec-00.txt
	def http_error_401(self, url, fp, errcode, errmsg, headers):
		if headers.has_key('www-authenticate'):
			stuff = headers['www-authenticate']
			p = regex.compile(
				'[ \t]*\([^ \t]+\)[ \t]+realm="\([^"]*\)"')
			if p.match(stuff) >= 0:
				scheme, realm = p.group(1, 2)
				if string.lower(scheme) == 'basic':
					return self.retry_http_basic_auth(
						url, realm)

	def retry_http_basic_auth(self, url, realm):
		host, selector = splithost(url)
		i = string.find(host, '@') + 1
		host = host[i:]
		user, passwd = self.get_user_passwd(host, realm, i)
		if not (user or passwd): return None
		host = user + ':' + passwd + '@' + host
		newurl = '//' + host + selector
		return self.open_http(newurl)

	def get_user_passwd(self, host, realm, clear_cache = 0):
		key = realm + '@' + string.lower(host)
		if self.auth_cache.has_key(key):
			if clear_cache:
				del self.auth_cache[key]
			else:
				return self.auth_cache[key]
		user, passwd = self.prompt_user_passwd(host, realm)
		if user or passwd: self.auth_cache[key] = (user, passwd)
		return user, passwd

	def prompt_user_passwd(self, host, realm):
		# Override this in a GUI environment!
		try:
			user = raw_input("Enter username for %s at %s: " %
					 (realm, host))
			self.echo_off()
			try:
				passwd = raw_input(
				  "Enter password for %s in %s at %s: " %
				  (user, realm, host))
			finally:
				self.echo_on()
			return user, passwd
		except KeyboardInterrupt:
			return None, None

	def echo_off(self):
		import os
		os.system("stty -echo")

	def echo_on(self):
		import os
		print
		os.system("stty echo")


# Utility functions

# Return the IP address of the magic hostname 'localhost'
_localhost = None
def localhost():
	global _localhost
	if not _localhost:
		_localhost = socket.gethostbyname('localhost')
	return _localhost

# Return the IP address of the current host
_thishost = None
def thishost():
	global _thishost
	if not _thishost:
		_thishost = socket.gethostbyname(socket.gethostname())
	return _thishost

# Return the set of errors raised by the FTP class
_ftperrors = None
def ftperrors():
	global _ftperrors
	if not _ftperrors:
		import ftplib
		_ftperrors = (ftplib.error_reply,
			      ftplib.error_temp,
			      ftplib.error_perm,
			      ftplib.error_proto)
	return _ftperrors

# Return an empty mimetools.Message object
_noheaders = None
def noheaders():
	global _noheaders
	if not _noheaders:
		import mimetools
		import StringIO
		_noheaders = mimetools.Message(StringIO.StringIO(), 0)
		_noheaders.fp.close()	# Recycle file descriptor
	return _noheaders


# Utility classes

# Class used by open_ftp() for cache of open FTP connections
class ftpwrapper:
	def __init__(self, user, passwd, host, port, dirs):
		self.user = unquote(user or '')
		self.passwd = unquote(passwd or '')
		self.host = host
		self.port = port
		self.dirs = []
		for dir in dirs:
			self.dirs.append(unquote(dir))
		self.init()
	def init(self):
		import ftplib
		self.ftp = ftplib.FTP()
		self.ftp.connect(self.host, self.port)
		self.ftp.login(self.user, self.passwd)
		for dir in self.dirs:
			self.ftp.cwd(dir)
	def retrfile(self, file, type):
		import ftplib
		if type in ('d', 'D'): cmd = 'TYPE A'; isdir = 1
		else: cmd = 'TYPE ' + type; isdir = 0
		try:
			self.ftp.voidcmd(cmd)
		except ftplib.all_errors:
			self.init()
			self.ftp.voidcmd(cmd)
		conn = None
		if file and not isdir:
			try:
				cmd = 'RETR ' + file
				conn = self.ftp.transfercmd(cmd)
			except ftplib.error_perm, reason:
				if reason[:3] != '550':
					raise IOError, ('ftp error', reason), \
					      sys.exc_traceback
		if not conn:
			# Try a directory listing
			if file: cmd = 'LIST ' + file
			else: cmd = 'LIST'
			conn = self.ftp.transfercmd(cmd)
		return addclosehook(conn.makefile('rb'), self.ftp.voidresp)

# Base class for addinfo and addclosehook
class addbase:
	def __init__(self, fp):
		self.fp = fp
		self.read = self.fp.read
		self.readline = self.fp.readline
		self.readlines = self.fp.readlines
		self.fileno = self.fp.fileno
	def __repr__(self):
		return '<%s at %s whose fp = %s>' % (
			  self.__class__.__name__, `id(self)`, `self.fp`)
	def close(self):
		self.read = None
		self.readline = None
		self.readlines = None
		self.fileno = None
		if self.fp: self.fp.close()
		self.fp = None

# Class to add a close hook to an open file
class addclosehook(addbase):
	def __init__(self, fp, closehook, *hookargs):
		addbase.__init__(self, fp)
		self.closehook = closehook
		self.hookargs = hookargs
	def close(self):
		if self.closehook:
			apply(self.closehook, self.hookargs)
			self.closehook = None
			self.hookargs = None
		addbase.close(self)

# class to add an info() method to an open file
class addinfo(addbase):
	def __init__(self, fp, headers):
		addbase.__init__(self, fp)
		self.headers = headers
	def info(self):
		return self.headers

# class to add info() and geturl() methods to an open file
class addinfourl(addbase):
	def __init__(self, fp, headers, url):
		addbase.__init__(self, fp)
		self.headers = headers
		self.url = url
	def info(self):
		return self.headers
	def geturl(self):
		return self.url


# Utility to combine a URL with a base URL to form a new URL

def basejoin(base, url):
	type, path = splittype(url)
	if type:
		# if url is complete (i.e., it contains a type), return it
		return url
	host, path = splithost(path)
	type, basepath = splittype(base) # inherit type from base
	if host:
		# if url contains host, just inherit type
		if type: return type + '://' + host + path
		else:
			# no type inherited, so url must have started with //
			# just return it
			return url
	host, basepath = splithost(basepath) # inherit host
	basepath, basetag = splittag(basepath) # remove extraneuous cruft
	basepath, basequery = splitquery(basepath) # idem
	if path[:1] != '/':
		# non-absolute path name
		if path[:1] in ('#', '?'):
			# path is just a tag or query, attach to basepath
			i = len(basepath)
		else:
			# else replace last component
			i = string.rfind(basepath, '/')
		if i < 0:
			# basepath not absolute
			if host:
				# host present, make absolute
				basepath = '/'
			else:
				# else keep non-absolute
				basepath = ''
		else:
			# remove last file component
			basepath = basepath[:i+1]
		# Interpret ../ (important because of symlinks)
		while basepath and path[:3] == '../':
			path = path[3:]
			i = string.rfind(basepath, '/')
			if i > 0:
				basepath = basepath[:i-1]
			
		path = basepath + path
	if type and host: return type + '://' + host + path
	elif type: return type + ':' + path
	elif host: return '//' + host + path # don't know what this means
	else: return path


# Utilities to parse URLs (most of these return None for missing parts):
# unwrap('<URL:type://host/path>') --> 'type://host/path'
# splittype('type:opaquestring') --> 'type', 'opaquestring'
# splithost('//host[:port]/path') --> 'host[:port]', '/path'
# splituser('user[:passwd]@host[:port]') --> 'user[:passwd]', 'host[:port]'
# splitpasswd('user:passwd') -> 'user', 'passwd'
# splitport('host:port') --> 'host', 'port'
# splitquery('/path?query') --> '/path', 'query'
# splittag('/path#tag') --> '/path', 'tag'
# splitattr('/path;attr1=value1;attr2=value2;...') ->
#   '/path', ['attr1=value1', 'attr2=value2', ...]
# splitvalue('attr=value') --> 'attr', 'value'
# splitgophertype('/Xselector') --> 'X', 'selector'
# unquote('abc%20def') -> 'abc def'
# quote('abc def') -> 'abc%20def')

def unwrap(url):
	url = string.strip(url)
	if url[:1] == '<' and url[-1:] == '>':
		url = string.strip(url[1:-1])
	if url[:4] == 'URL:': url = string.strip(url[4:])
	return url

_typeprog = regex.compile('^\([^/:]+\):')
def splittype(url):
	if _typeprog.match(url) >= 0:
		scheme = _typeprog.group(1)
		return scheme, url[len(scheme) + 1:]
	return None, url

_hostprog = regex.compile('^//\([^/]+\)\(.*\)$')
def splithost(url):
	if _hostprog.match(url) >= 0: return _hostprog.group(1, 2)
	return None, url

_userprog = regex.compile('^\([^@]*\)@\(.*\)$')
def splituser(host):
	if _userprog.match(host) >= 0: return _userprog.group(1, 2)
	return None, host

_passwdprog = regex.compile('^\([^:]*\):\(.*\)$')
def splitpasswd(user):
	if _passwdprog.match(user) >= 0: return _passwdprog.group(1, 2)
	return user, None

_portprog = regex.compile('^\(.*\):\([0-9]+\)$')
def splitport(host):
	if _portprog.match(host) >= 0: return _portprog.group(1, 2)
	return host, None

# Split host and port, returning numeric port.
# Return given default port if no ':' found; defaults to -1.
# Return numerical port if a valid number are found after ':'.
# Return None if ':' but not a valid number.
_nportprog = regex.compile('^\(.*\):\(.*\)$')
def splitnport(host, defport=-1):
	if _nportprog.match(host) >= 0:
	    host, port = _nportprog.group(1, 2)
	    try:
		if not port: raise string.atoi_error, "no digits"
		nport = string.atoi(port)
	    except string.atoi_error:
		nport = None
	    return host, nport
	return host, defport

_queryprog = regex.compile('^\(.*\)\?\([^?]*\)$')
def splitquery(url):
	if _queryprog.match(url) >= 0: return _queryprog.group(1, 2)
	return url, None

_tagprog = regex.compile('^\(.*\)#\([^#]*\)$')
def splittag(url):
	if _tagprog.match(url) >= 0: return _tagprog.group(1, 2)
	return url, None

def splitattr(url):
	words = string.splitfields(url, ';')
	return words[0], words[1:]

_valueprog = regex.compile('^\([^=]*\)=\(.*\)$')
def splitvalue(attr):
	if _valueprog.match(attr) >= 0: return _valueprog.group(1, 2)
	return attr, None

def splitgophertype(selector):
	if selector[:1] == '/' and selector[1:2]:
		return selector[1], selector[2:]
	return None, selector

_quoteprog = regex.compile('%[0-9a-fA-F][0-9a-fA-F]')
def unquote(s):
	i = 0
	n = len(s)
	res = []
	while 0 <= i < n:
		j = _quoteprog.search(s, i)
		if j < 0:
			res.append(s[i:])
			break
		res.append(s[i:j] + chr(string.atoi(s[j+1:j+3], 16)))
		i = j+3
	return string.joinfields(res, '')

def unquote_plus(s):
    if '+' in s:
	import regsub
	s = regsub.gsub('+', ' ', s)
    return unquote(s)

always_safe = string.letters + string.digits + '_,.-'
def quote(s, safe = '/'):
	safe = always_safe + safe
	res = []
	for c in s:
		if c in safe:
			res.append(c)
		else:
			res.append('%%%02x' % ord(c))
	return string.joinfields(res, '')

def quote_plus(s, safe = '/'):
    if ' ' in s:
	import regsub
	s = regsub.gsub(' ', '+', s)
	return quote(s, safe + '+')
    else:
	return quote(s, safe)


# Proxy handling
def getproxies():
	"""Return a dictionary of protocol scheme -> proxy server URL mappings.

	Scan the environment for variables named <scheme>_proxy;
	this seems to be the standard convention.  If you need a
	different way, you can pass a proxies dictionary to the
	[Fancy]URLopener constructor.

	"""
	proxies = {}
	for name, value in os.environ.items():
		name = string.lower(name)
		if value and name[-6:] == '_proxy':
			proxies[name[:-6]] = value
	return proxies


# Test and time quote() and unquote()
def test1():
	import time
	s = ''
	for i in range(256): s = s + chr(i)
	s = s*4
	t0 = time.time()
	qs = quote(s)
	uqs = unquote(qs)
	t1 = time.time()
	if uqs != s:
		print 'Wrong!'
	print `s`
	print `qs`
	print `uqs`
	print round(t1 - t0, 3), 'sec'


# Test program
def test():
	import sys
	import regsub
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
	try:
		for url in args:
			print '-'*10, url, '-'*10
			fn, h = urlretrieve(url)
			print fn, h
			if h:
				print '======'
				for k in h.keys(): print k + ':', h[k]
				print '======'
			fp = open(fn, 'rb')
			data = fp.read()
			del fp
			print regsub.gsub('\r', '', data)
			fn, h = None, None
		print '-'*40
	finally:
		urlcleanup()

# Run test program when run as a script
if __name__ == '__main__':
##	test1()
	test()
