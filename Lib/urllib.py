"""Open an arbitrary URL.

See the following document for more info on URLs:
"Names and Addresses, URIs, URLs, URNs, URCs", at
http://www.w3.org/pub/WWW/Addressing/Overview.html

See also the HTTP spec (from which the error codes are derived):
"HTTP - Hypertext Transfer Protocol", at
http://www.w3.org/pub/WWW/Protocols/

Related standards and specs:
- RFC1808: the "relative URL" spec. (authoritative status)
- RFC1738 - the "URL standard". (authoritative status)
- RFC1630 - the "URI spec". (informational status)

The object returned by URLopener().open(file) will differ per
protocol.  All you know is that is has methods read(), readline(),
readlines(), fileno(), close() and info().  The read*(), fileno()
and close() methods work like those of open files.
The info() method returns a mimetools.Message object which can be
used to query various info about the object, if available.
(mimetools.Message objects are queried with the getheader() method.)
"""

import string
import socket
import os
import sys


__version__ = '1.13'    # XXX This version is not always updated :-(

MAXFTPCACHE = 10        # Trim the ftp cache beyond this size

# Helper for non-unix systems
if os.name == 'mac':
    from macurl2path import url2pathname, pathname2url
elif os.name == 'nt':
    from nturl2path import url2pathname, pathname2url
else:
    def url2pathname(pathname):
        return unquote(pathname)
    def pathname2url(pathname):
        return quote(pathname)

# This really consists of two pieces:
# (1) a class which handles opening of all sorts of URLs
#     (plus assorted utilities etc.)
# (2) a set of functions for parsing URLs
# XXX Should these be separated out into different modules?


# Shortcut for basic usage
_urlopener = None
def urlopen(url, data=None):
    """urlopen(url [, data]) -> open file-like object"""
    global _urlopener
    if not _urlopener:
        _urlopener = FancyURLopener()
    if data is None:
        return _urlopener.open(url)
    else:
        return _urlopener.open(url, data)
def urlretrieve(url, filename=None, reporthook=None, data=None):
    global _urlopener
    if not _urlopener:
        _urlopener = FancyURLopener()
    return _urlopener.retrieve(url, filename, reporthook, data)
def urlcleanup():
    if _urlopener:
        _urlopener.cleanup()


ftpcache = {}
class URLopener:
    """Class to open URLs.
    This is a class rather than just a subroutine because we may need
    more than one set of global protocol-specific options.
    Note -- this is a base class for those who don't want the
    automatic handling of errors type 302 (relocated) and 401
    (authorization needed)."""

    __tempfiles = None

    version = "Python-urllib/%s" % __version__

    # Constructor
    def __init__(self, proxies=None, **x509):
        if proxies is None:
            proxies = getproxies()
        assert hasattr(proxies, 'has_key'), "proxies must be a mapping"
        self.proxies = proxies
        self.key_file = x509.get('key_file')
        self.cert_file = x509.get('cert_file')
        self.addheaders = [('User-agent', self.version)]
        self.__tempfiles = []
        self.__unlink = os.unlink # See cleanup()
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
        # XXX This is not threadsafe.  Bah.

    def __del__(self):
        self.close()

    def close(self):
        self.cleanup()

    def cleanup(self):
        # This code sometimes runs when the rest of this module
        # has already been deleted, so it can't use any globals
        # or import anything.
        if self.__tempfiles:
            for file in self.__tempfiles:
                try:
                    self.__unlink(file)
                except:
                    pass
            del self.__tempfiles[:]
        if self.tempcache:
            self.tempcache.clear()

    def addheader(self, *args):
        """Add a header to be used by the HTTP interface only
        e.g. u.addheader('Accept', 'sound/basic')"""
        self.addheaders.append(args)

    # External interface
    def open(self, fullurl, data=None):
        """Use URLopener().open(file) instead of open(file, 'r')."""
        fullurl = unwrap(fullurl)
        if self.tempcache and self.tempcache.has_key(fullurl):
            filename, headers = self.tempcache[fullurl]
            fp = open(filename, 'rb')
            return addinfourl(fp, headers, fullurl)
        type, url = splittype(fullurl)
        if not type:
            type = 'file'
        if self.proxies.has_key(type):
            proxy = self.proxies[type]
            type, proxyhost = splittype(proxy)
            host, selector = splithost(proxyhost)
            url = (host, fullurl) # Signal special case to open_*()
        else:
            proxy = None
        name = 'open_' + type
        self.type = type
        if '-' in name:
            # replace - with _
            name = string.join(string.split(name, '-'), '_')
        if not hasattr(self, name):
            if proxy:
                return self.open_unknown_proxy(proxy, fullurl, data)
            else:
                return self.open_unknown(fullurl, data)
        try:
            if data is None:
                return getattr(self, name)(url)
            else:
                return getattr(self, name)(url, data)
        except socket.error, msg:
            raise IOError, ('socket error', msg), sys.exc_info()[2]

    def open_unknown(self, fullurl, data=None):
        """Overridable interface to open unknown URL type."""
        type, url = splittype(fullurl)
        raise IOError, ('url error', 'unknown url type', type)

    def open_unknown_proxy(self, proxy, fullurl, data=None):
        """Overridable interface to open unknown URL type."""
        type, url = splittype(fullurl)
        raise IOError, ('url error', 'invalid proxy for %s' % type, proxy)

    # External interface
    def retrieve(self, url, filename=None, reporthook=None, data=None):
        """retrieve(url) returns (filename, None) for a local object
        or (tempfilename, headers) for a remote object."""
        url = unwrap(url)
        if self.tempcache and self.tempcache.has_key(url):
            return self.tempcache[url]
        type, url1 = splittype(url)
        if not filename and (not type or type == 'file'):
            try:
                fp = self.open_local_file(url1)
                hdrs = fp.info()
                del fp
                return url2pathname(splithost(url1)[1]), hdrs
            except IOError, msg:
                pass
        fp = self.open(url, data)
        headers = fp.info()
        if not filename:
            import tempfile
            garbage, path = splittype(url)
            garbage, path = splithost(path or "")
            path, garbage = splitquery(path or "")
            path, garbage = splitattr(path or "")
            suffix = os.path.splitext(path)[1]
            filename = tempfile.mktemp(suffix)
            self.__tempfiles.append(filename)
        result = filename, headers
        if self.tempcache is not None:
            self.tempcache[url] = result
        tfp = open(filename, 'wb')
        bs = 1024*8
        size = -1
        blocknum = 1
        if reporthook:
            if headers.has_key("content-length"):
                size = int(headers["Content-Length"])
            reporthook(0, bs, size)
        block = fp.read(bs)
        if reporthook:
            reporthook(1, bs, size)
        while block:
            tfp.write(block)
            block = fp.read(bs)
            blocknum = blocknum + 1
            if reporthook:
                reporthook(blocknum, bs, size)
        fp.close()
        tfp.close()
        del fp
        del tfp
        return result

    # Each method named open_<type> knows how to open that type of URL

    def open_http(self, url, data=None):
        """Use HTTP protocol."""
        import httplib
        user_passwd = None
        if type(url) is type(""):
            host, selector = splithost(url)
            if host:
                user_passwd, host = splituser(host)
                host = unquote(host)
            realhost = host
        else:
            host, selector = url
            urltype, rest = splittype(selector)
            url = rest
            user_passwd = None
            if string.lower(urltype) != 'http':
                realhost = None
            else:
                realhost, rest = splithost(rest)
                if realhost:
                    user_passwd, realhost = splituser(realhost)
                if user_passwd:
                    selector = "%s://%s%s" % (urltype, realhost, rest)
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
            h.putheader('Content-type', 'application/x-www-form-urlencoded')
            h.putheader('Content-length', '%d' % len(data))
        else:
            h.putrequest('GET', selector)
        if auth: h.putheader('Authorization', 'Basic %s' % auth)
        if realhost: h.putheader('Host', realhost)
        for args in self.addheaders: apply(h.putheader, args)
        h.endheaders()
        if data is not None:
            h.send(data + '\r\n')
        errcode, errmsg, headers = h.getreply()
        fp = h.getfile()
        if errcode == 200:
            return addinfourl(fp, headers, "http:" + url)
        else:
            if data is None:
                return self.http_error(url, fp, errcode, errmsg, headers)
            else:
                return self.http_error(url, fp, errcode, errmsg, headers, data)

    def http_error(self, url, fp, errcode, errmsg, headers, data=None):
        """Handle http errors.
        Derived class can override this, or provide specific handlers
        named http_error_DDD where DDD is the 3-digit error code."""
        # First check if there's a specific handler for this error
        name = 'http_error_%d' % errcode
        if hasattr(self, name):
            method = getattr(self, name)
            if data is None:
                result = method(url, fp, errcode, errmsg, headers)
            else:
                result = method(url, fp, errcode, errmsg, headers, data)
            if result: return result
        return self.http_error_default(url, fp, errcode, errmsg, headers)

    def http_error_default(self, url, fp, errcode, errmsg, headers):
        """Default error handler: close the connection and raise IOError."""
        void = fp.read()
        fp.close()
        raise IOError, ('http error', errcode, errmsg, headers)

    if hasattr(socket, "ssl"):
        def open_https(self, url, data=None):
            """Use HTTPS protocol."""
            import httplib
            user_passwd = None
            if type(url) is type(""):
                host, selector = splithost(url)
                if host:
                    user_passwd, host = splituser(host)
                    host = unquote(host)
                realhost = host
            else:
                host, selector = url
                urltype, rest = splittype(selector)
                url = rest
                user_passwd = None
                if string.lower(urltype) != 'https':
                    realhost = None
                else:
                    realhost, rest = splithost(rest)
                    if realhost:
                        user_passwd, realhost = splituser(realhost)
                    if user_passwd:
                        selector = "%s://%s%s" % (urltype, realhost, rest)
                #print "proxy via https:", host, selector
            if not host: raise IOError, ('https error', 'no host given')
            if user_passwd:
                import base64
                auth = string.strip(base64.encodestring(user_passwd))
            else:
                auth = None
            h = httplib.HTTPS(host, 0,
                              key_file=self.key_file,
                              cert_file=self.cert_file)
            if data is not None:
                h.putrequest('POST', selector)
                h.putheader('Content-type',
                            'application/x-www-form-urlencoded')
                h.putheader('Content-length', '%d' % len(data))
            else:
                h.putrequest('GET', selector)
            if auth: h.putheader('Authorization: Basic %s' % auth)
            if realhost: h.putheader('Host', realhost)
            for args in self.addheaders: apply(h.putheader, args)
            h.endheaders()
            if data is not None:
                h.send(data + '\r\n')
            errcode, errmsg, headers = h.getreply()
            fp = h.getfile()
            if errcode == 200:
                return addinfourl(fp, headers, url)
            else:
                if data is None:
                    return self.http_error(url, fp, errcode, errmsg, headers)
                else:
                    return self.http_error(url, fp, errcode, errmsg, headers, data)

    def open_gopher(self, url):
        """Use Gopher protocol."""
        import gopherlib
        host, selector = splithost(url)
        if not host: raise IOError, ('gopher error', 'no host given')
        host = unquote(host)
        type, selector = splitgophertype(selector)
        selector, query = splitquery(selector)
        selector = unquote(selector)
        if query:
            query = unquote(query)
            fp = gopherlib.send_query(selector, query, host)
        else:
            fp = gopherlib.send_selector(selector, host)
        return addinfourl(fp, noheaders(), "gopher:" + url)

    def open_file(self, url):
        """Use local file or FTP depending on form of URL."""
        if url[:2] == '//' and url[2:3] != '/':
            return self.open_ftp(url)
        else:
            return self.open_local_file(url)

    def open_local_file(self, url):
        """Use local file."""
        import mimetypes, mimetools, StringIO
        mtype = mimetypes.guess_type(url)[0]
        headers = mimetools.Message(StringIO.StringIO(
            'Content-Type: %s\n' % (mtype or 'text/plain')))
        host, file = splithost(url)
        if not host:
            urlfile = file
            if file[:1] == '/':
                urlfile = 'file://' + file
            return addinfourl(open(url2pathname(file), 'rb'),
                              headers, urlfile)
        host, port = splitport(host)
        if not port \
           and socket.gethostbyname(host) in (localhost(), thishost()):
            urlfile = file
            if file[:1] == '/':
                urlfile = 'file://' + file
            return addinfourl(open(url2pathname(file), 'rb'),
                              headers, urlfile)
        raise IOError, ('local file error', 'not on local host')

    def open_ftp(self, url):
        """Use FTP protocol."""
        host, path = splithost(url)
        if not host: raise IOError, ('ftp error', 'no host given')
        host, port = splitport(host)
        user, host = splituser(host)
        if user: user, passwd = splitpasswd(user)
        else: passwd = None
        host = unquote(host)
        user = unquote(user or '')
        passwd = unquote(passwd or '')
        host = socket.gethostbyname(host)
        if not port:
            import ftplib
            port = ftplib.FTP_PORT
        else:
            port = int(port)
        path, attrs = splitattr(path)
        path = unquote(path)
        dirs = string.splitfields(path, '/')
        dirs, file = dirs[:-1], dirs[-1]
        if dirs and not dirs[0]: dirs = dirs[1:]
        if dirs and not dirs[0]: dirs[0] = '/'
        key = user, host, port, string.join(dirs, '/')
        # XXX thread unsafe!
        if len(self.ftpcache) > MAXFTPCACHE:
            # Prune the cache, rather arbitrarily
            for k in self.ftpcache.keys():
                if k != key:
                    v = self.ftpcache[k]
                    del self.ftpcache[k]
                    v.close()
        try:
            if not self.ftpcache.has_key(key):
                self.ftpcache[key] = \
                    ftpwrapper(user, passwd, host, port, dirs)
            if not file: type = 'D'
            else: type = 'I'
            for attr in attrs:
                attr, value = splitvalue(attr)
                if string.lower(attr) == 'type' and \
                   value in ('a', 'A', 'i', 'I', 'd', 'D'):
                    type = string.upper(value)
            (fp, retrlen) = self.ftpcache[key].retrfile(file, type)
            if retrlen is not None and retrlen >= 0:
                import mimetools, StringIO
                headers = mimetools.Message(StringIO.StringIO(
                    'Content-Length: %d\n' % retrlen))
            else:
                headers = noheaders()
            return addinfourl(fp, headers, "ftp:" + url)
        except ftperrors(), msg:
            raise IOError, ('ftp error', msg), sys.exc_info()[2]

    def open_data(self, url, data=None):
        """Use "data" URL."""
        # ignore POSTed data
        #
        # syntax of data URLs:
        # dataurl   := "data:" [ mediatype ] [ ";base64" ] "," data
        # mediatype := [ type "/" subtype ] *( ";" parameter )
        # data      := *urlchar
        # parameter := attribute "=" value
        import StringIO, mimetools, time
        try:
            [type, data] = string.split(url, ',', 1)
        except ValueError:
            raise IOError, ('data error', 'bad data URL')
        if not type:
            type = 'text/plain;charset=US-ASCII'
        semi = string.rfind(type, ';')
        if semi >= 0 and '=' not in type[semi:]:
            encoding = type[semi+1:]
            type = type[:semi]
        else:
            encoding = ''
        msg = []
        msg.append('Date: %s'%time.strftime('%a, %d %b %Y %T GMT',
                                            time.gmtime(time.time())))
        msg.append('Content-type: %s' % type)
        if encoding == 'base64':
            import base64
            data = base64.decodestring(data)
        else:
            data = unquote(data)
        msg.append('Content-length: %d' % len(data))
        msg.append('')
        msg.append(data)
        msg = string.join(msg, '\n')
        f = StringIO.StringIO(msg)
        headers = mimetools.Message(f, 0)
        f.fileno = None     # needed for addinfourl
        return addinfourl(f, headers, url)


class FancyURLopener(URLopener):
    """Derived class with handlers for errors we can handle (perhaps)."""

    def __init__(self, *args):
        apply(URLopener.__init__, (self,) + args)
        self.auth_cache = {}

    def http_error_default(self, url, fp, errcode, errmsg, headers):
        """Default error handling -- don't raise an exception."""
        return addinfourl(fp, headers, "http:" + url)

    def http_error_302(self, url, fp, errcode, errmsg, headers, data=None):
        """Error 302 -- relocated (temporarily)."""
        # XXX The server can force infinite recursion here!
        if headers.has_key('location'):
            newurl = headers['location']
        elif headers.has_key('uri'):
            newurl = headers['uri']
        else:
            return
        void = fp.read()
        fp.close()
        # In case the server sent a relative URL, join with original:
        newurl = basejoin("http:" + url, newurl)
        if data is None:
            return self.open(newurl)
        else:
            return self.open(newurl, data)

    def http_error_301(self, url, fp, errcode, errmsg, headers, data=None):
        """Error 301 -- also relocated (permanently)."""
        return self.http_error_302(url, fp, errcode, errmsg, headers, data)

    def http_error_401(self, url, fp, errcode, errmsg, headers, data=None):
        """Error 401 -- authentication required.
        See this URL for a description of the basic authentication scheme:
        http://www.ics.uci.edu/pub/ietf/http/draft-ietf-http-v10-spec-00.txt"""
        if headers.has_key('www-authenticate'):
            stuff = headers['www-authenticate']
            import re
            match = re.match('[ \t]*([^ \t]+)[ \t]+realm="([^"]*)"', stuff)
            if match:
                scheme, realm = match.groups()
                if string.lower(scheme) == 'basic':
                   name = 'retry_' + self.type + '_basic_auth'
                   if data is None:
                       return getattr(self,name)(url, realm)
                   else:
                       return getattr(self,name)(url, realm, data)

    def retry_http_basic_auth(self, url, realm, data=None):
        host, selector = splithost(url)
        i = string.find(host, '@') + 1
        host = host[i:]
        user, passwd = self.get_user_passwd(host, realm, i)
        if not (user or passwd): return None
        host = user + ':' + passwd + '@' + host
        newurl = 'http://' + host + selector
        if data is None:
            return self.open(newurl)
        else:
            return self.open(newurl, data)

    def retry_https_basic_auth(self, url, realm, data=None):
            host, selector = splithost(url)
            i = string.find(host, '@') + 1
            host = host[i:]
            user, passwd = self.get_user_passwd(host, realm, i)
            if not (user or passwd): return None
            host = user + ':' + passwd + '@' + host
            newurl = '//' + host + selector
            return self.open_https(newurl)

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
        """Override this in a GUI environment!"""
        import getpass
        try:
            user = raw_input("Enter username for %s at %s: " % (realm,
                                                                host))
            passwd = getpass.getpass("Enter password for %s in %s at %s: " %
                (user, realm, host))
            return user, passwd
        except KeyboardInterrupt:
            print
            return None, None


# Utility functions

_localhost = None
def localhost():
    """Return the IP address of the magic hostname 'localhost'."""
    global _localhost
    if not _localhost:
        _localhost = socket.gethostbyname('localhost')
    return _localhost

_thishost = None
def thishost():
    """Return the IP address of the current host."""
    global _thishost
    if not _thishost:
        _thishost = socket.gethostbyname(socket.gethostname())
    return _thishost

_ftperrors = None
def ftperrors():
    """Return the set of errors raised by the FTP class."""
    global _ftperrors
    if not _ftperrors:
        import ftplib
        _ftperrors = ftplib.all_errors
    return _ftperrors

_noheaders = None
def noheaders():
    """Return an empty mimetools.Message object."""
    global _noheaders
    if not _noheaders:
        import mimetools
        import StringIO
        _noheaders = mimetools.Message(StringIO.StringIO(), 0)
        _noheaders.fp.close()   # Recycle file descriptor
    return _noheaders


# Utility classes

class ftpwrapper:
    """Class used by open_ftp() for cache of open FTP connections."""

    def __init__(self, user, passwd, host, port, dirs):
        self.user = user
        self.passwd = passwd
        self.host = host
        self.port = port
        self.dirs = dirs
        self.init()

    def init(self):
        import ftplib
        self.busy = 0
        self.ftp = ftplib.FTP()
        self.ftp.connect(self.host, self.port)
        self.ftp.login(self.user, self.passwd)
        for dir in self.dirs:
            self.ftp.cwd(dir)

    def retrfile(self, file, type):
        import ftplib
        self.endtransfer()
        if type in ('d', 'D'): cmd = 'TYPE A'; isdir = 1
        else: cmd = 'TYPE ' + type; isdir = 0
        try:
            self.ftp.voidcmd(cmd)
        except ftplib.all_errors:
            self.init()
            self.ftp.voidcmd(cmd)
        conn = None
        if file and not isdir:
            # Use nlst to see if the file exists at all
            try:
                self.ftp.nlst(file)
            except ftplib.error_perm, reason:
                raise IOError, ('ftp error', reason), sys.exc_info()[2]
            # Restore the transfer mode!
            self.ftp.voidcmd(cmd)
            # Try to retrieve as a file
            try:
                cmd = 'RETR ' + file
                conn = self.ftp.ntransfercmd(cmd)
            except ftplib.error_perm, reason:
                if reason[:3] != '550':
                    raise IOError, ('ftp error', reason), sys.exc_info()[2]
        if not conn:
            # Set transfer mode to ASCII!
            self.ftp.voidcmd('TYPE A')
            # Try a directory listing
            if file: cmd = 'LIST ' + file
            else: cmd = 'LIST'
            conn = self.ftp.ntransfercmd(cmd)
        self.busy = 1
        # Pass back both a suitably decorated object and a retrieval length
        return (addclosehook(conn[0].makefile('rb'),
                             self.endtransfer), conn[1])
    def endtransfer(self):
        if not self.busy:
            return
        self.busy = 0
        try:
            self.ftp.voidresp()
        except ftperrors():
            pass

    def close(self):
        self.endtransfer()
        try:
            self.ftp.close()
        except ftperrors():
            pass

class addbase:
    """Base class for addinfo and addclosehook."""

    def __init__(self, fp):
        self.fp = fp
        self.read = self.fp.read
        self.readline = self.fp.readline
        if hasattr(self.fp, "readlines"): self.readlines = self.fp.readlines
        if hasattr(self.fp, "fileno"): self.fileno = self.fp.fileno

    def __repr__(self):
        return '<%s at %s whose fp = %s>' % (self.__class__.__name__,
                                             `id(self)`, `self.fp`)

    def close(self):
        self.read = None
        self.readline = None
        self.readlines = None
        self.fileno = None
        if self.fp: self.fp.close()
        self.fp = None

class addclosehook(addbase):
    """Class to add a close hook to an open file."""

    def __init__(self, fp, closehook, *hookargs):
        addbase.__init__(self, fp)
        self.closehook = closehook
        self.hookargs = hookargs

    def close(self):
        addbase.close(self)
        if self.closehook:
            apply(self.closehook, self.hookargs)
            self.closehook = None
            self.hookargs = None

class addinfo(addbase):
    """class to add an info() method to an open file."""

    def __init__(self, fp, headers):
        addbase.__init__(self, fp)
        self.headers = headers

    def info(self):
        return self.headers

class addinfourl(addbase):
    """class to add info() and geturl() methods to an open file."""

    def __init__(self, fp, headers, url):
        addbase.__init__(self, fp)
        self.headers = headers
        self.url = url

    def info(self):
        return self.headers

    def geturl(self):
        return self.url


def basejoin(base, url):
    """Utility to combine a URL with a base URL to form a new URL."""
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
    basepath, basetag = splittag(basepath) # remove extraneous cruft
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
            i = string.rfind(basepath[:-1], '/')
            if i > 0:
                basepath = basepath[:i+1]
            elif i == 0:
                basepath = '/'
                break
            else:
                basepath = ''

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
    """unwrap('<URL:type://host/path>') --> 'type://host/path'."""
    url = string.strip(url)
    if url[:1] == '<' and url[-1:] == '>':
        url = string.strip(url[1:-1])
    if url[:4] == 'URL:': url = string.strip(url[4:])
    return url

_typeprog = None
def splittype(url):
    """splittype('type:opaquestring') --> 'type', 'opaquestring'."""
    global _typeprog
    if _typeprog is None:
        import re
        _typeprog = re.compile('^([^/:]+):')

    match = _typeprog.match(url)
    if match:
        scheme = match.group(1)
        return scheme.lower(), url[len(scheme) + 1:]
    return None, url

_hostprog = None
def splithost(url):
    """splithost('//host[:port]/path') --> 'host[:port]', '/path'."""
    global _hostprog
    if _hostprog is None:
        import re
        _hostprog = re.compile('^//([^/]*)(.*)$')

    match = _hostprog.match(url)
    if match: return match.group(1, 2)
    return None, url

_userprog = None
def splituser(host):
    """splituser('user[:passwd]@host[:port]') --> 'user[:passwd]', 'host[:port]'."""
    global _userprog
    if _userprog is None:
        import re
        _userprog = re.compile('^([^@]*)@(.*)$')

    match = _userprog.match(host)
    if match: return map(unquote, match.group(1, 2))
    return None, host

_passwdprog = None
def splitpasswd(user):
    """splitpasswd('user:passwd') -> 'user', 'passwd'."""
    global _passwdprog
    if _passwdprog is None:
        import re
        _passwdprog = re.compile('^([^:]*):(.*)$')

    match = _passwdprog.match(user)
    if match: return match.group(1, 2)
    return user, None

# splittag('/path#tag') --> '/path', 'tag'
_portprog = None
def splitport(host):
    """splitport('host:port') --> 'host', 'port'."""
    global _portprog
    if _portprog is None:
        import re
        _portprog = re.compile('^(.*):([0-9]+)$')

    match = _portprog.match(host)
    if match: return match.group(1, 2)
    return host, None

_nportprog = None
def splitnport(host, defport=-1):
    """Split host and port, returning numeric port.
    Return given default port if no ':' found; defaults to -1.
    Return numerical port if a valid number are found after ':'.
    Return None if ':' but not a valid number."""
    global _nportprog
    if _nportprog is None:
        import re
        _nportprog = re.compile('^(.*):(.*)$')

    match = _nportprog.match(host)
    if match:
        host, port = match.group(1, 2)
        try:
            if not port: raise string.atoi_error, "no digits"
            nport = string.atoi(port)
        except string.atoi_error:
            nport = None
        return host, nport
    return host, defport

_queryprog = None
def splitquery(url):
    """splitquery('/path?query') --> '/path', 'query'."""
    global _queryprog
    if _queryprog is None:
        import re
        _queryprog = re.compile('^(.*)\?([^?]*)$')

    match = _queryprog.match(url)
    if match: return match.group(1, 2)
    return url, None

_tagprog = None
def splittag(url):
    """splittag('/path#tag') --> '/path', 'tag'."""
    global _tagprog
    if _tagprog is None:
        import re
        _tagprog = re.compile('^(.*)#([^#]*)$')

    match = _tagprog.match(url)
    if match: return match.group(1, 2)
    return url, None

def splitattr(url):
    """splitattr('/path;attr1=value1;attr2=value2;...') ->
        '/path', ['attr1=value1', 'attr2=value2', ...]."""
    words = string.splitfields(url, ';')
    return words[0], words[1:]

_valueprog = None
def splitvalue(attr):
    """splitvalue('attr=value') --> 'attr', 'value'."""
    global _valueprog
    if _valueprog is None:
        import re
        _valueprog = re.compile('^([^=]*)=(.*)$')

    match = _valueprog.match(attr)
    if match: return match.group(1, 2)
    return attr, None

def splitgophertype(selector):
    """splitgophertype('/Xselector') --> 'X', 'selector'."""
    if selector[:1] == '/' and selector[1:2]:
        return selector[1], selector[2:]
    return None, selector

def unquote(s):
    """unquote('abc%20def') -> 'abc def'."""
    mychr = chr
    myatoi = string.atoi
    list = string.split(s, '%')
    res = [list[0]]
    myappend = res.append
    del list[0]
    for item in list:
        if item[1:2]:
            try:
                myappend(mychr(myatoi(item[:2], 16))
                     + item[2:])
            except:
                myappend('%' + item)
        else:
            myappend('%' + item)
    return string.join(res, "")

def unquote_plus(s):
    """unquote('%7e/abc+def') -> '~/abc def'"""
    if '+' in s:
        # replace '+' with ' '
        s = string.join(string.split(s, '+'), ' ')
    return unquote(s)

always_safe = ('ABCDEFGHIJKLMNOPQRSTUVWXYZ'
               'abcdefghijklmnopqrstuvwxyz'
               '0123456789' '_.-')

_fast_safe_test = always_safe + '/'
_fast_safe = None

def _fast_quote(s):
    global _fast_safe
    if _fast_safe is None:
        _fast_safe = {}
        for c in _fast_safe_test:
            _fast_safe[c] = c
    res = list(s)
    for i in range(len(res)):
        c = res[i]
        if not _fast_safe.has_key(c):
            res[i] = '%%%02x' % ord(c)
    return string.join(res, '')

def quote(s, safe = '/'):
    """quote('abc def') -> 'abc%20def'

    Each part of a URL, e.g. the path info, the query, etc., has a
    different set of reserved characters that must be quoted.

    RFC 2396 Uniform Resource Identifiers (URI): Generic Syntax lists
    the following reserved characters.

    reserved    = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" |
                  "$" | ","

    Each of these characters is reserved in some component of a URL,
    but not necessarily in all of them.

    By default, the quote function is intended for quoting the path
    section of a URL.  Thus, it will not encode '/'.  This character
    is reserved, but in typical usage the quote function is being
    called on a path where the existing slash characters are used as
    reserved characters.
    """
    safe = always_safe + safe
    if _fast_safe_test == safe:
        return _fast_quote(s)
    res = list(s)
    for i in range(len(res)):
        c = res[i]
        if c not in safe:
            res[i] = '%%%02x' % ord(c)
    return string.join(res, '')

def quote_plus(s, safe = ''):
    """Quote the query fragment of a URL; replacing ' ' with '+'"""
    if ' ' in s:
        l = string.split(s, ' ')
        for i in range(len(l)):
            l[i] = quote(l[i], safe)
        return string.join(l, '+')
    else:
        return quote(s, safe)

def urlencode(dict):
    """Encode a dictionary of form entries into a URL query string."""
    l = []
    for k, v in dict.items():
        k = quote_plus(str(k))
        v = quote_plus(str(v))
        l.append(k + '=' + v)
    return string.join(l, '&')

# Proxy handling
def getproxies_environment():
    """Return a dictionary of scheme -> proxy server URL mappings.

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

if os.name == 'mac':
    def getproxies():
        """Return a dictionary of scheme -> proxy server URL mappings.

        By convention the mac uses Internet Config to store
        proxies.  An HTTP proxy, for instance, is stored under
        the HttpProxy key.

        """
        try:
            import ic
        except ImportError:
            return {}

        try:
            config = ic.IC()
        except ic.error:
            return {}
        proxies = {}
        # HTTP:
        if config.has_key('UseHTTPProxy') and config['UseHTTPProxy']:
            try:
                value = config['HTTPProxyHost']
            except ic.error:
                pass
            else:
                proxies['http'] = 'http://%s' % value
        # FTP: XXXX To be done.
        # Gopher: XXXX To be done.
        return proxies

elif os.name == 'nt':
    def getproxies_registry():
        """Return a dictionary of scheme -> proxy server URL mappings.

        Win32 uses the registry to store proxies.

        """
        proxies = {}
        try:
            import _winreg
        except ImportError:
            # Std module, so should be around - but you never know!
            return proxies
        try:
            internetSettings = _winreg.OpenKey(_winreg.HKEY_CURRENT_USER,
                r'Software\Microsoft\Windows\CurrentVersion\Internet Settings')
            proxyEnable = _winreg.QueryValueEx(internetSettings,
                                               'ProxyEnable')[0]
            if proxyEnable:
                # Returned as Unicode but problems if not converted to ASCII
                proxyServer = str(_winreg.QueryValueEx(internetSettings,
                                                       'ProxyServer')[0])
                if '=' in proxyServer:
                    # Per-protocol settings
                    for p in proxyServer.split(';'):
                        protocol, address = p.split('=', 1)
                        proxies[protocol] = '%s://%s' % (protocol, address)
                else:
                    # Use one setting for all protocols
                    if proxyServer[:5] == 'http:':
                        proxies['http'] = proxyServer
                    else:
                        proxies['http'] = 'http://%s' % proxyServer
                        proxies['ftp'] = 'ftp://%s' % proxyServer
            internetSettings.Close()
        except (WindowsError, ValueError, TypeError):
            # Either registry key not found etc, or the value in an
            # unexpected format.
            # proxies already set up to be empty so nothing to do
            pass
        return proxies

    def getproxies():
        """Return a dictionary of scheme -> proxy server URL mappings.

        Returns settings gathered from the environment, if specified,
        or the registry.

        """
        return getproxies_environment() or getproxies_registry()
else:
    # By default use environment variables
    getproxies = getproxies_environment


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


def reporthook(blocknum, blocksize, totalsize):
    # Report during remote transfers
    print "Block number: %d, Block size: %d, Total size: %d" % (blocknum, blocksize, totalsize)

# Test program
def test(args=[]):
    if not args:
        args = [
            '/etc/passwd',
            'file:/etc/passwd',
            'file://localhost/etc/passwd',
            'ftp://ftp.python.org/etc/passwd',
##          'gopher://gopher.micro.umn.edu/1/',
            'http://www.python.org/index.html',
            ]
        if hasattr(URLopener, "open_https"):
            args.append('https://synergy.as.cmu.edu/~geek/')
    try:
        for url in args:
            print '-'*10, url, '-'*10
            fn, h = urlretrieve(url, None, reporthook)
            print fn, h
            if h:
                print '======'
                for k in h.keys(): print k + ':', h[k]
                print '======'
            fp = open(fn, 'rb')
            data = fp.read()
            del fp
            if '\r' in data:
                table = string.maketrans("", "")
                data = string.translate(data, table, "\r")
            print data
            fn, h = None, None
        print '-'*40
    finally:
        urlcleanup()

def main():
    import getopt, sys
    try:
        opts, args = getopt.getopt(sys.argv[1:], "th")
    except getopt.error, msg:
        print msg
        print "Use -h for help"
        return
    t = 0
    for o, a in opts:
        if o == '-t':
            t = t + 1
        if o == '-h':
            print "Usage: python urllib.py [-t] [url ...]"
            print "-t runs self-test;",
            print "otherwise, contents of urls are printed"
            return
    if t:
        if t > 1:
            test1()
        test(args)
    else:
        if not args:
            print "Use -h for help"
        for url in args:
            print urlopen(url).read(),

# Run test program when run as a script
if __name__ == '__main__':
    main()
