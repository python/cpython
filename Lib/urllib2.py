"""An extensible library for opening URLs using a variety of protocols

The simplest way to use this module is to call the urlopen function,
which accepts a string containing a URL or a Request object (described 
below).  It opens the URL and returns the results as file-like
object; the returned object has some extra methods described below.

The OpenerDirectory manages a collection of Handler objects that do
all the actual work.  Each Handler implements a particular protocol or 
option.  The OpenerDirector is a composite object that invokes the
Handlers needed to open the requested URL.  For example, the
HTTPHandler performs HTTP GET and POST requests and deals with
non-error returns.  The HTTPRedirectHandler automatically deals with
HTTP 301 & 302 redirect errors, and the HTTPDigestAuthHandler deals
with digest authentication.

urlopen(url, data=None) -- basic usage is that same as original
urllib.  pass the url and optionally data to post to an HTTP URL, and
get a file-like object back.  One difference is that you can also pass 
a Request instance instead of URL.  Raises a URLError (subclass of
IOError); for HTTP errors, raises an HTTPError, which can also be
treated as a valid response.

build_opener -- function that creates a new OpenerDirector instance.
will install the default handlers.  accepts one or more Handlers as
arguments, either instances or Handler classes that it will
instantiate.  if one of the argument is a subclass of the default
handler, the argument will be installed instead of the default.

install_opener -- installs a new opener as the default opener.

objects of interest:
OpenerDirector --

Request -- an object that encapsulates the state of a request.  the
state can be a simple as the URL.  it can also include extra HTTP
headers, e.g. a User-Agent.

BaseHandler --

exceptions:
URLError-- a subclass of IOError, individual protocols have their own
specific subclass

HTTPError-- also a valid HTTP response, so you can treat an HTTP error 
as an exceptional event or valid response

internals:
BaseHandler and parent
_call_chain conventions

Example usage:

import urllib2

# set up authentication info
authinfo = urllib2.HTTPBasicAuthHandler()
authinfo.add_password('realm', 'host', 'username', 'password')

# build a new opener that adds authentication and caching FTP handlers 
opener = urllib2.build_opener(authinfo, urllib2.CacheFTPHandler)

# install it
urllib2.install_opener(opener)

f = urllib2.urlopen('http://www.python.org/')


"""

# XXX issues:
# If an authentication error handler that tries to perform
 # authentication for some reason but fails, how should the error be
 # signalled?  The client needs to know the HTTP error code.  But if
 # the handler knows that the problem was, e.g., that it didn't know
 # that hash algo that requested in the challenge, it would be good to 
 # pass that information along to the client, too.

# XXX to do:
# name!
# documentation (getting there)
# complex proxies
# abstract factory for opener
# ftp errors aren't handled cleanly
# gopher can return a socket.error
# check digest against correct (i.e. non-apache) implementation

import string
import socket
import UserDict
import httplib
import re
import base64
import types
import urlparse
import os
import md5
import mimetypes
import mimetools
import ftplib
import sys
import time
import gopherlib

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

try:
    import sha
except ImportError:
    # need 1.5.2 final
    sha = None

# not sure how many of these need to be gotten rid of
from urllib import unwrap, unquote, splittype, splithost, \
     addinfourl, splitport, splitgophertype, splitquery, \
     splitattr, ftpwrapper, noheaders

# support for proxies via environment variables
from urllib import getproxies

# support for FileHandler
from urllib import localhost, thishost, url2pathname, pathname2url

# support for GopherHandler
from urllib import splitgophertype, splitquery

__version__ = "2.0a1"

_opener = None
def urlopen(url, data=None):
    global _opener
    if _opener is None:
        _opener = build_opener()
    return _opener.open(url, data)

def install_opener(opener):
    global _opener
    _opener = opener

# do these error classes make sense?
# make sure all of the IOError stuff is overridden.  we just want to be 
 # subtypes.

class URLError(IOError):
    # URLError is a sub-type of IOError, but it doesn't share any of
    # the implementation.  need to override __init__ and __str__
    def __init__(self, reason):
        self.reason = reason

    def __str__(self):
        return '<urlopen error %s>' % self.reason

class HTTPError(URLError, addinfourl):
    """Raised when HTTP error occurs, but also acts like non-error return"""
    __super_init = addinfourl.__init__

    def __init__(self, url, code, msg, hdrs, fp):
        self.__super_init(fp, hdrs, url)
        self.code = code
        self.msg = msg
        self.hdrs = hdrs
        self.fp = fp
        # XXX
        self.filename = url
        
    def __str__(self):
        return 'HTTP Error %s: %s' % (self.code, self.msg)

    def __del__(self):
        # XXX is this safe? what if user catches exception, then
        # extracts fp and discards exception?
        if self.fp:
            self.fp.close()

class GopherError(URLError):
    pass

class Request:
    def __init__(self, url, data=None, headers={}):
        # unwrap('<URL:type://host/path>') --> 'type://host/path'
        self.__original = unwrap(url)
        self.type = None
        # self.__r_type is what's left after doing the splittype
        self.host = None
        self.port = None
        self.data = data
        self.headers = {}
        self.headers.update(headers)

    def __getattr__(self, attr):
        # XXX this is a fallback mechanism to guard against these
        # methods getting called in a non-standard order.  this may be 
        # too complicated and/or unnecessary.
        # XXX should the __r_XXX attributes be public?
        if attr[:12] == '_Request__r_':
            name = attr[12:]
            if hasattr(Request, 'get_' + name):
                getattr(self, 'get_' + name)()
                return getattr(self, attr)
        raise AttributeError, attr

    def add_data(self, data):
        self.data = data

    def has_data(self):
        return self.data is not None

    def get_data(self):
        return self.data

    def get_full_url(self):
        return self.__original

    def get_type(self):
        if self.type is None:
            self.type, self.__r_type = splittype(self.__original)
            assert self.type is not None, self.__original
        return self.type

    def get_host(self):
        if self.host is None:
            self.host, self.__r_host = splithost(self.__r_type)
            if self.host:
                self.host = unquote(self.host)
        return self.host

    def get_selector(self):
        return self.__r_host

    def set_proxy(self, proxy):
        self.__proxy = proxy
        # XXX this code is based on urllib, but it doesn't seem
        # correct.  specifically, if the proxy has a port number then
        # splittype will return the hostname as the type and the port
        # will be include with everything else
        self.type, self.__r_type = splittype(self.__proxy)
        self.host, XXX = splithost(self.__r_type)
        self.host = unquote(self.host)
        self.__r_host = self.__original

    def add_header(self, key, val):
        # useful for something like authentication
        self.headers[key] = val

class OpenerDirector:
    def __init__(self):
        server_version = "Python-urllib/%s" % __version__
        self.addheaders = [('User-agent', server_version)]
        # manage the individual handlers
        self.handlers = []
        self.handle_open = {}
        self.handle_error = {}

    def add_handler(self, handler):
        added = 0
        for meth in get_methods(handler):
            if meth[-5:] == '_open':
                protocol = meth[:-5]
                if self.handle_open.has_key(protocol): 
                    self.handle_open[protocol].append(handler)
                else:
                    self.handle_open[protocol] = [handler]
                added = 1
                continue
            i = string.find(meth, '_')
            j = string.find(meth[i+1:], '_') + i + 1
            if j != -1 and meth[i+1:j] == 'error':
                proto = meth[:i]
                kind = meth[j+1:]
                try:
                    kind = string.atoi(kind)
                except ValueError:
                    pass
                dict = self.handle_error.get(proto, {})
                if dict.has_key(kind):
                    dict[kind].append(handler)
                else:
                    dict[kind] = [handler]
                self.handle_error[proto] = dict
                added = 1
                continue
        if added:
            self.handlers.append(handler)
            handler.add_parent(self)
        
    def __del__(self):
        self.close()

    def close(self):
        for handler in self.handlers:
            handler.close()
        self.handlers = []

    def _call_chain(self, chain, kind, meth_name, *args):
        # XXX raise an exception if no one else should try to handle
        # this url.  return None if you can't but someone else could.
        handlers = chain.get(kind, ())
        for handler in handlers:
            func = getattr(handler, meth_name)

            result = func(*args)
            if result is not None:
                return result

    def open(self, fullurl, data=None):
        # accept a URL or a Request object
        if type(fullurl) == types.StringType:
            req = Request(fullurl, data)
        else:
            req = fullurl
            if data is not None:
                req.add_data(data)
        assert isinstance(req, Request) # really only care about interface
        
        result = self._call_chain(self.handle_open, 'default',
                                  'default_open', req)  
        if result:
            return result

        type_ = req.get_type()
        result = self._call_chain(self.handle_open, type_, type_ + \
                                  '_open', req)
        if result:
            return result

        return self._call_chain(self.handle_open, 'unknown',
                                'unknown_open', req)

    def error(self, proto, *args):
        if proto == 'http':
            # XXX http protocol is special cased
            dict = self.handle_error[proto]
            proto = args[2]  # YUCK!
            meth_name = 'http_error_%d' % proto
            http_err = 1
            orig_args = args
        else:
            dict = self.handle_error
            meth_name = proto + '_error'
            http_err = 0
        args = (dict, proto, meth_name) + args
        result = self._call_chain(*args)
        if result:
            return result

        if http_err:
            args = (dict, 'default', 'http_error_default') + orig_args
            return self._call_chain(*args)

def is_callable(obj):
    # not quite like builtin callable (which I didn't know existed),
    # not entirely sure it needs to be different
    if type(obj) in (types.BuiltinFunctionType,
                     types.BuiltinMethodType,  types.LambdaType,
                     types.MethodType):
        return 1
    if type(obj) == types.InstanceType:
        return hasattr(obj, '__call__')
    return 0

def get_methods(inst):
    methods = {}
    classes = []
    classes.append(inst.__class__)
    while classes:
        klass = classes[0]
        del classes[0]
        classes = classes + list(klass.__bases__)
        for name in dir(klass):
            attr = getattr(klass, name)
            if type(attr) == types.UnboundMethodType:
                methods[name] = 1
    for name in dir(inst):
        if is_callable(getattr(inst, name)):
            methods[name] = 1
    return methods.keys()

# XXX probably also want an abstract factory that knows things like
 # the fact that a ProxyHandler needs to get inserted first.
# would also know when it makes sense to skip a superclass in favor of
 # a subclass and when it might make sense to include both 

def build_opener(*handlers):
    """Create an opener object from a list of handlers.

    The opener will use several default handlers, including support
    for HTTP and FTP.  If there is a ProxyHandler, it must be at the
    front of the list of handlers.  (Yuck.)

    If any of the handlers passed as arguments are subclasses of the
    default handlers, the default handlers will not be used.
    """
    
    opener = OpenerDirector()
    default_classes = [ProxyHandler, UnknownHandler, HTTPHandler,
                       HTTPDefaultErrorHandler, HTTPRedirectHandler,
                       FTPHandler, FileHandler]
    skip = []
    for klass in default_classes:
        for check in handlers:
            if type(check) == types.ClassType:
                if issubclass(check, klass):
                    skip.append(klass)
            elif type(check) == types.InstanceType:
                if isinstance(check, klass):
                    skip.append(klass)
    for klass in skip:
        default_classes.remove(klass)

    for klass in default_classes:
        opener.add_handler(klass())

    for h in handlers:
        if type(h) == types.ClassType:
            h = h()
        opener.add_handler(h)
    return opener

class BaseHandler:
    def add_parent(self, parent):
        self.parent = parent
    def close(self):
        self.parent = None

class HTTPDefaultErrorHandler(BaseHandler):
    def http_error_default(self, req, fp, code, msg, hdrs):
        raise HTTPError(req.get_full_url(), code, msg, hdrs, fp)

class HTTPRedirectHandler(BaseHandler):
    # Implementation note: To avoid the server sending us into an
    # infinite loop, the request object needs to track what URLs we
    # have already seen.  Do this by adding a handler-specific
    # attribute to the Request object.
    def http_error_302(self, req, fp, code, msg, headers):
        if headers.has_key('location'):
            newurl = headers['location']
        elif headers.has_key('uri'):
            newurl = headers['uri']
        else:
            return
        nil = fp.read()
        fp.close()

        newurl = urlparse.urljoin(req.get_full_url(), newurl)

        # XXX Probably want to forget about the state of the current
        # request, although that might interact poorly with other
        # handlers that also use handler-specific request attributes
        new = Request(newurl, req.get_data())
        new.error_302_dict = {}
        if hasattr(req, 'error_302_dict'):
            if req.error_302_dict.has_key(newurl):
                raise HTTPError(req.get_full_url(), code,
                                self.inf_msg + msg, headers)
            new.error_302_dict.update(req.error_302_dict)
        new.error_302_dict[newurl] = newurl
        return self.parent.open(new)

    http_error_301 = http_error_302

    inf_msg = "The HTTP server returned a redirect error that would" \
              "lead to an infinite loop.\n" \
              "The last 302 error message was:\n"

class ProxyHandler(BaseHandler):
    def __init__(self, proxies=None):
        if proxies is None:
            proxies = getproxies()
        assert hasattr(proxies, 'has_key'), "proxies must be a mapping"
        self.proxies = proxies
        for type, url in proxies.items():
            setattr(self, '%s_open' % type, 
                    lambda r, proxy=url, type=type, meth=self.proxy_open: \
                    meth(r, proxy, type))

    def proxy_open(self, req, proxy, type):
        orig_type = req.get_type()
        req.set_proxy(proxy)
        if orig_type == type:
            # let other handlers take care of it
            # XXX this only makes sense if the proxy is before the
            # other handlers
            return None
        else:
            # need to start over, because the other handlers don't
            # grok the proxy's URL type
            return self.parent.open(req)

# feature suggested by Duncan Booth
# XXX custom is not a good name
class CustomProxy:
    # either pass a function to the constructor or override handle
    def __init__(self, proto, func=None, proxy_addr=None):
        self.proto = proto
        self.func = func
        self.addr = proxy_addr

    def handle(self, req):
        if self.func and self.func(req):
            return 1

    def get_proxy(self):
        return self.addr

class CustomProxyHandler(BaseHandler):
    def __init__(self, *proxies):
        self.proxies = {}

    def proxy_open(self, req):
        proto = req.get_type()
        try:
            proxies = self.proxies[proto]
        except KeyError:
            return None
        for p in proxies:
            if p.handle(req):
                req.set_proxy(p.get_proxy())
                return self.parent.open(req)
        return None

    def do_proxy(self, p, req):
        p
        return self.parent.open(req)

    def add_proxy(self, cpo):
        if self.proxies.has_key(cpo.proto):
            self.proxies[cpo.proto].append(cpo)
        else:
            self.proxies[cpo.proto] = [cpo]

class HTTPPasswordMgr:
    def __init__(self):
        self.passwd = {}

    def add_password(self, realm, uri, user, passwd):
        # uri could be a single URI or a sequence
        if type(uri) == types.StringType:
            uri = [uri]
        uri = tuple(map(self.reduce_uri, uri))
        if not self.passwd.has_key(realm):
            self.passwd[realm] = {}
        self.passwd[realm][uri] = (user, passwd)

    def find_user_password(self, realm, authuri):
        domains = self.passwd.get(realm, {})
        authuri = self.reduce_uri(authuri)
        for uris, authinfo in domains.items():
            for uri in uris:
                if self.is_suburi(uri, authuri):
                    return authinfo
        return None, None

    def reduce_uri(self, uri):
        """Accept netloc or URI and extract only the netloc and path"""
        parts = urlparse.urlparse(uri)
        if parts[1]:
            return parts[1], parts[2] or '/'
        else:
            return parts[2], '/'

    def is_suburi(self, base, test):
        """Check if test is below base in a URI tree

        Both args must be URIs in reduced form.
        """
        if base == test:
            return 1
        if base[0] != test[0]:
            return 0
        common = os.path.commonprefix((base[1], test[1]))
        if len(common) == len(base[1]):
            return 1
        return 0
        

class HTTPBasicAuthHandler(BaseHandler):
    rx = re.compile('[ \t]*([^ \t]+)[ \t]+realm="([^"]*)"')

    # XXX there can actually be multiple auth-schemes in a
    # www-authenticate header.  should probably be a lot more careful
    # in parsing them to extract multiple alternatives

    def __init__(self):
        self.passwd = HTTPPasswordMgr()
        self.add_password = self.passwd.add_password
        self.__current_realm = None
        # if __current_realm is not None, then the server must have
        # refused our name/password and is asking for authorization
        # again.  must be careful to set it to None on successful
        # return. 
    
    def http_error_401(self, req, fp, code, msg, headers):
        # XXX could be mult. headers
        authreq = headers.get('www-authenticate', None)
        if authreq:
            mo = HTTPBasicAuthHandler.rx.match(authreq)
            if mo:
                scheme, realm = mo.groups()
                if string.lower(scheme) == 'basic':
                    return self.retry_http_basic_auth(req, realm)

    def retry_http_basic_auth(self, req, realm):
        if self.__current_realm is None:
            self.__current_realm = realm
        else:
            self.__current_realm = realm
            return None
        # XXX host isn't really the correct URI?
        host = req.get_host()
        user,pw = self.passwd.find_user_password(realm, host)
        if pw:
            raw = "%s:%s" % (user, pw)
            auth = string.strip(base64.encodestring(raw))
            req.add_header('Authorization', 'Basic %s' % auth)
            resp = self.parent.open(req)
            self.__current_realm = None
            return resp
        else:
            self.__current_realm = None
            return None

class HTTPDigestAuthHandler(BaseHandler):
    """An authentication protocol defined by RFC 2069

    Digest authentication improves on basic authentication because it
    does not transmit passwords in the clear.
    """

    def __init__(self):
        self.passwd = HTTPPasswordMgr()
        self.add_password = self.passwd.add_password
        self.__current_realm = None

    def http_error_401(self, req, fp, code, msg, headers):
        # XXX could be mult. headers
        authreq = headers.get('www-authenticate', None)
        if authreq:
            kind = string.split(authreq)[0]
            if kind == 'Digest':
                return self.retry_http_digest_auth(req, authreq)

    def retry_http_digest_auth(self, req, auth):
        token, challenge = string.split(auth, ' ', 1)
        chal = parse_keqv_list(parse_http_list(challenge))
        auth = self.get_authorization(req, chal)
        if auth:
            req.add_header('Authorization', 'Digest %s' % auth)
            resp = self.parent.open(req)
            self.__current_realm = None
            return resp

    def get_authorization(self, req, chal):
        try:
            realm = chal['realm']
            nonce = chal['nonce']
            algorithm = chal.get('algorithm', 'MD5')
            # mod_digest doesn't send an opaque, even though it isn't
            # supposed to be optional
            opaque = chal.get('opaque', None)
        except KeyError:
            return None

        if self.__current_realm is None:
            self.__current_realm = realm
        else:
            self.__current_realm = realm
            return None

        H, KD = self.get_algorithm_impls(algorithm)
        if H is None:
            return None

        user, pw = self.passwd.find_user_password(realm,
                                                  req.get_full_url()) 
        if user is None:
            return None

        # XXX not implemented yet
        if req.has_data():
            entdig = self.get_entity_digest(req.get_data(), chal)
        else:
            entdig = None

        A1 = "%s:%s:%s" % (user, realm, pw)
        A2 = "%s:%s" % (req.has_data() and 'POST' or 'GET',
                        # XXX selector: what about proxies and full urls
                        req.get_selector())
        respdig = KD(H(A1), "%s:%s" % (nonce, H(A2)))
        # XXX should the partial digests be encoded too?

        base = 'username="%s", realm="%s", nonce="%s", uri="%s", ' \
               'response="%s"' % (user, realm, nonce, req.get_selector(),
                                  respdig)
        if opaque:
            base = base + ', opaque="%s"' % opaque
        if entdig:
            base = base + ', digest="%s"' % entdig
        if algorithm != 'MD5':
            base = base + ', algorithm="%s"' % algorithm
        return base

    def get_algorithm_impls(self, algorithm):
        # lambdas assume digest modules are imported at the top level
        if algorithm == 'MD5':
            H = lambda x, e=encode_digest:e(md5.new(x).digest())
        elif algorithm == 'SHA':
            H = lambda x, e=encode_digest:e(sha.new(x).digest())
        # XXX MD5-sess
        KD = lambda s, d, H=H: H("%s:%s" % (s, d))
        return H, KD

    def get_entity_digest(self, data, chal):
        # XXX not implemented yet
        return None

def encode_digest(digest):
    hexrep = []
    for c in digest:
        n = (ord(c) >> 4) & 0xf
        hexrep.append(hex(n)[-1])
        n = ord(c) & 0xf
        hexrep.append(hex(n)[-1])
    return string.join(hexrep, '')
        
        
class HTTPHandler(BaseHandler):
    def http_open(self, req):
        # XXX devise a new mechanism to specify user/password
        host = req.get_host()
        if not host:
            raise URLError('no host given')

        try:
            h = httplib.HTTP(host) # will parse host:port
            if req.has_data():
                data = req.get_data()
                h.putrequest('POST', req.get_selector())
                h.putheader('Content-type',
                            'application/x-www-form-urlencoded')
                h.putheader('Content-length', '%d' % len(data))
            else:
                h.putrequest('GET', req.get_selector())
        except socket.error, err:
            raise URLError(err)
            
        # XXX proxies would have different host here
        h.putheader('Host', host)
        for args in self.parent.addheaders:
            h.putheader(*args)
        for k, v in req.headers.items():
            h.putheader(k, v)
        h.endheaders()
        if req.has_data():
            h.send(data + '\r\n')

        code, msg, hdrs = h.getreply()
        fp = h.getfile()
        if code == 200:
            return addinfourl(fp, hdrs, req.get_full_url())
        else:
            return self.parent.error('http', req, fp, code, msg, hdrs)

class UnknownHandler(BaseHandler):
    def unknown_open(self, req):
        type = req.get_type()
        raise URLError('unknown url type: %s' % type)

def parse_keqv_list(l):
    """Parse list of key=value strings where keys are not duplicated."""
    parsed = {}
    for elt in l:
        k, v = string.split(elt, '=', 1)
        if v[0] == '"' and v[-1] == '"':
            v = v[1:-1]
        parsed[k] = v
    return parsed

def parse_http_list(s):
    """Parse lists as described by RFC 2068 Section 2.

    In particular, parse comman-separated lists where the elements of
    the list may include quoted-strings.  A quoted-string could
    contain a comma.
    """
    # XXX this function could probably use more testing

    list = []
    end = len(s)
    i = 0
    inquote = 0
    start = 0
    while i < end:
        cur = s[i:]
        c = string.find(cur, ',')
        q = string.find(cur, '"')
        if c == -1:
            list.append(s[start:])
            break
        if q == -1:
            if inquote:
                raise ValueError, "unbalanced quotes"
            else:
                list.append(s[start:i+c])
                i = i + c + 1
                continue
        if inquote:
            if q < c:
                list.append(s[start:i+c])
                i = i + c + 1
                start = i
                inquote = 0
            else:
                i = i + q 
        else:
            if c < q:
                list.append(s[start:i+c])
                i = i + c + 1
                start = i
            else:
                inquote = 1
                i = i + q + 1
    return map(string.strip, list)

class FileHandler(BaseHandler):
    # Use local file or FTP depending on form of URL
    def file_open(self, req):
        url = req.get_selector()
        if url[:2] == '//' and url[2:3] != '/':
            req.type = 'ftp'
            return self.parent.open(req)
        else:
            return self.open_local_file(req)

    # names for the localhost
    names = None
    def get_names(self):
        if FileHandler.names is None:
            FileHandler.names = (socket.gethostbyname('localhost'), 
                                 socket.gethostbyname(socket.gethostname()))
        return FileHandler.names

    # not entirely sure what the rules are here
    def open_local_file(self, req):
        mtype = mimetypes.guess_type(req.get_selector())[0]
        headers = mimetools.Message(StringIO('Content-Type: %s\n' \
                                             % (mtype or 'text/plain')))
        host = req.get_host()
        file = req.get_selector()
        if host:
            host, port = splitport(host)
        if not host or \
           (not port and socket.gethostbyname(host) in self.get_names()):
            return addinfourl(open(url2pathname(file), 'rb'),
                              headers, 'file:'+file)
        raise URLError('file not on local host')

class FTPHandler(BaseHandler):
    def ftp_open(self, req):
        host = req.get_host()
        if not host:
            raise IOError, ('ftp error', 'no host given')
        # XXX handle custom username & password
        try:
            host = socket.gethostbyname(host)
        except socket.error, msg:
            raise URLError(msg)
        host, port = splitport(host)
        if port is None:
            port = ftplib.FTP_PORT
        path, attrs = splitattr(req.get_selector())
        path = unquote(path)
        dirs = string.splitfields(path, '/')
        dirs, file = dirs[:-1], dirs[-1]
        if dirs and not dirs[0]:
            dirs = dirs[1:]
        user = passwd = '' # XXX
        try:
            fw = self.connect_ftp(user, passwd, host, port, dirs)
            type = file and 'I' or 'D'
            for attr in attrs:
                attr, value = splitattr(attr)
                if string.lower(attr) == 'type' and \
                   value in ('a', 'A', 'i', 'I', 'd', 'D'):
                    type = string.upper(value)
            fp, retrlen = fw.retrfile(file, type)
            if retrlen is not None and retrlen >= 0:
                sf = StringIO('Content-Length: %d\n' % retrlen)
                headers = mimetools.Message(sf)
            else:
                headers = noheaders()
            return addinfourl(fp, headers, req.get_full_url())
        except ftplib.all_errors, msg:
            raise IOError, ('ftp error', msg), sys.exc_info()[2]

    def connect_ftp(self, user, passwd, host, port, dirs):
        fw = ftpwrapper(user, passwd, host, port, dirs)
##        fw.ftp.set_debuglevel(1)
        return fw

class CacheFTPHandler(FTPHandler):
    # XXX would be nice to have pluggable cache strategies
    # XXX this stuff is definitely not thread safe
    def __init__(self):
        self.cache = {}
        self.timeout = {}
        self.soonest = 0
        self.delay = 60
        self.max_conns = 16

    def setTimeout(self, t):
        self.delay = t

    def setMaxConns(self, m):
        self.max_conns = m

    def connect_ftp(self, user, passwd, host, port, dirs):
        key = user, passwd, host, port
        if self.cache.has_key(key):
            self.timeout[key] = time.time() + self.delay
        else:
            self.cache[key] = ftpwrapper(user, passwd, host, port, dirs)
            self.timeout[key] = time.time() + self.delay
        self.check_cache()
        return self.cache[key]

    def check_cache(self):
        # first check for old ones
        t = time.time()
        if self.soonest <= t:
            for k, v in self.timeout.items():
                if v < t:
                    self.cache[k].close()
                    del self.cache[k]
                    del self.timeout[k]
        self.soonest = min(self.timeout.values())

        # then check the size
        if len(self.cache) == self.max_conns:
            for k, v in self.timeout.items():
                if v == self.soonest:
                    del self.cache[k]
                    del self.timeout[k]
                    break
            self.soonest = min(self.timeout.values())

class GopherHandler(BaseHandler):
    def gopher_open(self, req):
        host = req.get_host()
        if not host:
            raise GopherError('no host given')
        host = unquote(host)
        selector = req.get_selector()
        type, selector = splitgophertype(selector)
        selector, query = splitquery(selector)
        selector = unquote(selector)
        if query:
            query = unquote(query)
            fp = gopherlib.send_query(selector, query, host)
        else:
            fp = gopherlib.send_selector(selector, host)
        return addinfourl(fp, noheaders(), req.get_full_url())

#bleck! don't use this yet
class OpenerFactory:

    default_handlers = [UnknownHandler, HTTPHandler,
                        HTTPDefaultErrorHandler, HTTPRedirectHandler, 
                        FTPHandler, FileHandler]
    proxy_handlers = [ProxyHandler]
    handlers = []
    replacement_handlers = []

    def add_proxy_handler(self, ph):
        self.proxy_handlers = self.proxy_handlers + [ph]

    def add_handler(self, h):
        self.handlers = self.handlers + [h]

    def replace_handler(self, h):
        pass

    def build_opener(self):
        opener = OpenerDirectory()
        for ph in self.proxy_handlers:
            if type(ph) == types.ClassType:
                ph = ph()
            opener.add_handler(ph)

if __name__ == "__main__":
    # XXX some of the test code depends on machine configurations that 
    # are internal to CNRI.   Need to set up a public server with the
    # right authentication configuration for test purposes.
    if socket.gethostname() == 'bitdiddle':
        localhost = 'bitdiddle.cnri.reston.va.us'
    elif socket.gethostname() == 'bitdiddle.concentric.net':
        localhost = 'localhost'
    else:
        localhost = None
    urls = [
        # Thanks to Fred for finding these!
        'gopher://gopher.lib.ncsu.edu/11/library/stacks/Alex',
        'gopher://gopher.vt.edu:10010/10/33',

        'file:/etc/passwd',
        'file://nonsensename/etc/passwd',
        'ftp://www.python.org/pub/tmp/httplib.py',
        'ftp://www.python.org/pub/tmp/imageop.c',
        'ftp://www.python.org/pub/tmp/blat',
        'http://www.espn.com/', # redirect
        'http://www.python.org/Spanish/Inquistion/',
        ('http://grail.cnri.reston.va.us/cgi-bin/faqw.py',
         'query=pythonistas&querytype=simple&casefold=yes&req=search'),
        'http://www.python.org/',
        'ftp://prep.ai.mit.edu/welcome.msg',
        'ftp://www.python.org/pub/tmp/figure.prn',
        'ftp://www.python.org/pub/tmp/interp.pl',
        'http://checkproxy.cnri.reston.va.us/test/test.html',
            ]

    if localhost is not None:
        urls = urls + [
            'file://%s/etc/passwd' % localhost,
            'http://%s/simple/' % localhost,
            'http://%s/digest/' % localhost,
            'http://%s/not/found.h' % localhost,
            ]

        bauth = HTTPBasicAuthHandler()
        bauth.add_password('basic_test_realm', localhost, 'jhylton',
                           'password') 
        dauth = HTTPDigestAuthHandler()
        dauth.add_password('digest_test_realm', localhost, 'jhylton', 
                           'password')
        

    cfh = CacheFTPHandler()
    cfh.setTimeout(1)

    # XXX try out some custom proxy objects too!
    def at_cnri(req):
        host = req.get_host()
        print host
        if host[-18:] == '.cnri.reston.va.us':
            return 1
    p = CustomProxy('http', at_cnri, 'proxy.cnri.reston.va.us')
    ph = CustomProxyHandler(p)

    install_opener(build_opener(dauth, bauth, cfh, GopherHandler, ph))

    for url in urls:
        if type(url) == types.TupleType:
            url, req = url
        else:
            req = None
        print url
        try:
            f = urlopen(url, req)
        except IOError, err:
            print "IOError:", err
        except socket.error, err:
            print "socket.error:", err
        else:
            buf = f.read()
            f.close()
            print "read %d bytes" % len(buf)
        print
        time.sleep(0.1)
