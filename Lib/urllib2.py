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
HTTP 301, 302, 303 and 307 redirect errors, and the HTTPDigestAuthHandler
deals with digest authentication.

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

proxy_support = urllib2.ProxyHandler({"http" : "http://ahad-haam:3128"})

# build a new opener that adds authentication and caching FTP handlers
opener = urllib2.build_opener(proxy_support, authinfo, urllib2.CacheFTPHandler)

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

import socket
import httplib
import inspect
import re
import base64
import types
import urlparse
import md5
import mimetypes
import mimetools
import rfc822
import ftplib
import sys
import time
import os
import stat
import gopherlib
import posixpath

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
from urllib import localhost, url2pathname

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

    def get_method(self):
        if self.has_data():
            return "POST"
        else:
            return "GET"

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
            if self.type is None:
                raise ValueError, "unknown url type: %s" % self.__original
        return self.type

    def get_host(self):
        if self.host is None:
            self.host, self.__r_host = splithost(self.__r_type)
            if self.host:
                self.host = unquote(self.host)
        return self.host

    def get_selector(self):
        return self.__r_host

    def set_proxy(self, host, type):
        self.host, self.type = host, type
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
        for meth in dir(handler):
            if meth[-5:] == '_open':
                protocol = meth[:-5]
                if self.handle_open.has_key(protocol):
                    self.handle_open[protocol].append(handler)
                else:
                    self.handle_open[protocol] = [handler]
                added = 1
                continue
            i = meth.find('_')
            j = meth[i+1:].find('_') + i + 1
            if j != -1 and meth[i+1:j] == 'error':
                proto = meth[:i]
                kind = meth[j+1:]
                try:
                    kind = int(kind)
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
        if isinstance(fullurl, (types.StringType, types.UnicodeType)):
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
        if proto in ['http', 'https']:
            # XXX http[s] protocols are special-cased
            dict = self.handle_error['http'] # https is not different than http
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
    if hasattr(httplib, 'HTTPS'):
        default_classes.append(HTTPSHandler)
    skip = []
    for klass in default_classes:
        for check in handlers:
            if inspect.isclass(check):
                if issubclass(check, klass):
                    skip.append(klass)
            elif isinstance(check, klass):
                skip.append(klass)
    for klass in skip:
        default_classes.remove(klass)

    for klass in default_classes:
        opener.add_handler(klass())

    for h in handlers:
        if inspect.isclass(h):
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
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        """Return a Request or None in response to a redirect.

        This is called by the http_error_30x methods when a redirection
        response is received.  If a redirection should take place, return a new
        Request to allow http_error_30x to perform the redirect.  Otherwise,
        raise HTTPError if no-one else should try to handle this url.  Return
        None if you can't but another Handler might.

        """
        m = req.get_method()
        if (code in (301, 302, 303, 307) and m in ("GET", "HEAD")
            or code in (301, 302, 303) and m == "POST"):
            # Strictly (according to RFC 2616), 301 or 302 in response
            # to a POST MUST NOT cause a redirection without confirmation
            # from the user (of urllib2, in this case).  In practice,
            # essentially all clients do redirect in this case, so we
            # do the same.
            return Request(newurl, headers=req.headers)
        else:
            raise HTTPError(req.get_full_url(), code, msg, headers, fp)

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
        newurl = urlparse.urljoin(req.get_full_url(), newurl)

        # XXX Probably want to forget about the state of the current
        # request, although that might interact poorly with other
        # handlers that also use handler-specific request attributes
        new = self.redirect_request(req, fp, code, msg, headers, newurl)
        if new is None:
            return

        # loop detection
        new.error_302_dict = {}
        if hasattr(req, 'error_302_dict'):
            if len(req.error_302_dict)>10 or \
               req.error_302_dict.has_key(newurl):
                raise HTTPError(req.get_full_url(), code,
                                self.inf_msg + msg, headers, fp)
            new.error_302_dict.update(req.error_302_dict)
        new.error_302_dict[newurl] = newurl

        # Don't close the fp until we are sure that we won't use it
        # with HTTPError.
        fp.read()
        fp.close()

        return self.parent.open(new)

    http_error_301 = http_error_303 = http_error_307 = http_error_302

    inf_msg = "The HTTP server returned a redirect error that would " \
              "lead to an infinite loop.\n" \
              "The last 30x error message was:\n"

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
        type, r_type = splittype(proxy)
        host, XXX = splithost(r_type)
        if '@' in host:
            user_pass, host = host.split('@', 1)
            if ':' in user_pass:
                user, password = user_pass.split(':', 1)
                user_pass = base64.encodestring('%s:%s' % (unquote(user),
                                                           unquote(password)))
                req.add_header('Proxy-Authorization', 'Basic ' + user_pass)
        host = unquote(host)
        req.set_proxy(host, type)
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
        if isinstance(uri, (types.StringType, types.UnicodeType)):
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
        common = posixpath.commonprefix((base[1], test[1]))
        if len(common) == len(base[1]):
            return 1
        return 0


class HTTPPasswordMgrWithDefaultRealm(HTTPPasswordMgr):

    def find_user_password(self, realm, authuri):
        user, password = HTTPPasswordMgr.find_user_password(self,realm,authuri)
        if user is not None:
            return user, password
        return HTTPPasswordMgr.find_user_password(self, None, authuri)


class AbstractBasicAuthHandler:

    rx = re.compile('[ \t]*([^ \t]+)[ \t]+realm="([^"]*)"')

    # XXX there can actually be multiple auth-schemes in a
    # www-authenticate header.  should probably be a lot more careful
    # in parsing them to extract multiple alternatives

    def __init__(self, password_mgr=None):
        if password_mgr is None:
            password_mgr = HTTPPasswordMgr()
        self.passwd = password_mgr
        self.add_password = self.passwd.add_password

    def http_error_auth_reqed(self, authreq, host, req, headers):
        # XXX could be multiple headers
        authreq = headers.get(authreq, None)
        if authreq:
            mo = AbstractBasicAuthHandler.rx.match(authreq)
            if mo:
                scheme, realm = mo.groups()
                if scheme.lower() == 'basic':
                    return self.retry_http_basic_auth(host, req, realm)

    def retry_http_basic_auth(self, host, req, realm):
        user,pw = self.passwd.find_user_password(realm, host)
        if pw:
            raw = "%s:%s" % (user, pw)
            auth = 'Basic %s' % base64.encodestring(raw).strip()
            if req.headers.get(self.auth_header, None) == auth:
                return None
            req.add_header(self.auth_header, auth)
            return self.parent.open(req)
        else:
            return None

class HTTPBasicAuthHandler(AbstractBasicAuthHandler, BaseHandler):

    auth_header = 'Authorization'

    def http_error_401(self, req, fp, code, msg, headers):
        host = urlparse.urlparse(req.get_full_url())[1]
        return self.http_error_auth_reqed('www-authenticate',
                                          host, req, headers)


class ProxyBasicAuthHandler(AbstractBasicAuthHandler, BaseHandler):

    auth_header = 'Proxy-Authorization'

    def http_error_407(self, req, fp, code, msg, headers):
        host = req.get_host()
        return self.http_error_auth_reqed('proxy-authenticate',
                                          host, req, headers)


class AbstractDigestAuthHandler:

    def __init__(self, passwd=None):
        if passwd is None:
            passwd = HTTPPasswordMgr()
        self.passwd = passwd
        self.add_password = self.passwd.add_password

    def http_error_auth_reqed(self, authreq, host, req, headers):
        authreq = headers.get(self.auth_header, None)
        if authreq:
            kind = authreq.split()[0]
            if kind == 'Digest':
                return self.retry_http_digest_auth(req, authreq)

    def retry_http_digest_auth(self, req, auth):
        token, challenge = auth.split(' ', 1)
        chal = parse_keqv_list(parse_http_list(challenge))
        auth = self.get_authorization(req, chal)
        if auth:
            auth_val = 'Digest %s' % auth
            if req.headers.get(self.auth_header, None) == auth_val:
                return None
            req.add_header(self.auth_header, auth_val)
            resp = self.parent.open(req)
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


class HTTPDigestAuthHandler(BaseHandler, AbstractDigestAuthHandler):
    """An authentication protocol defined by RFC 2069

    Digest authentication improves on basic authentication because it
    does not transmit passwords in the clear.
    """

    header = 'Authorization'

    def http_error_401(self, req, fp, code, msg, headers):
        host = urlparse.urlparse(req.get_full_url())[1]
        self.http_error_auth_reqed('www-authenticate', host, req, headers)


class ProxyDigestAuthHandler(BaseHandler, AbstractDigestAuthHandler):

    header = 'Proxy-Authorization'

    def http_error_407(self, req, fp, code, msg, headers):
        host = req.get_host()
        self.http_error_auth_reqed('proxy-authenticate', host, req, headers)


def encode_digest(digest):
    hexrep = []
    for c in digest:
        n = (ord(c) >> 4) & 0xf
        hexrep.append(hex(n)[-1])
        n = ord(c) & 0xf
        hexrep.append(hex(n)[-1])
    return ''.join(hexrep)


class AbstractHTTPHandler(BaseHandler):

    def do_open(self, http_class, req):
        host = req.get_host()
        if not host:
            raise URLError('no host given')

        h = http_class(host) # will parse host:port
        if req.has_data():
            data = req.get_data()
            h.putrequest('POST', req.get_selector())
            if not req.headers.has_key('Content-type'):
                h.putheader('Content-type',
                            'application/x-www-form-urlencoded')
            if not req.headers.has_key('Content-length'):
                h.putheader('Content-length', '%d' % len(data))
        else:
            h.putrequest('GET', req.get_selector())

        scheme, sel = splittype(req.get_selector())
        sel_host, sel_path = splithost(sel)
        h.putheader('Host', sel_host or host)
        for args in self.parent.addheaders:
            h.putheader(*args)
        for k, v in req.headers.items():
            h.putheader(k, v)
        # httplib will attempt to connect() here.  be prepared
        # to convert a socket error to a URLError.
        try:
            h.endheaders()
        except socket.error, err:
            raise URLError(err)
        if req.has_data():
            h.send(data)

        code, msg, hdrs = h.getreply()
        fp = h.getfile()
        if code == 200:
            return addinfourl(fp, hdrs, req.get_full_url())
        else:
            return self.parent.error('http', req, fp, code, msg, hdrs)


class HTTPHandler(AbstractHTTPHandler):

    def http_open(self, req):
        return self.do_open(httplib.HTTP, req)


if hasattr(httplib, 'HTTPS'):
    class HTTPSHandler(AbstractHTTPHandler):

        def https_open(self, req):
            return self.do_open(httplib.HTTPS, req)


class UnknownHandler(BaseHandler):
    def unknown_open(self, req):
        type = req.get_type()
        raise URLError('unknown url type: %s' % type)

def parse_keqv_list(l):
    """Parse list of key=value strings where keys are not duplicated."""
    parsed = {}
    for elt in l:
        k, v = elt.split('=', 1)
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
        c = cur.find(',')
        q = cur.find('"')
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
    return map(lambda x: x.strip(), list)

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
        host = req.get_host()
        file = req.get_selector()
        localfile = url2pathname(file)
        stats = os.stat(localfile)
        size = stats[stat.ST_SIZE]
        modified = rfc822.formatdate(stats[stat.ST_MTIME])
        mtype = mimetypes.guess_type(file)[0]
        stats = os.stat(localfile)
        headers = mimetools.Message(StringIO(
            'Content-Type: %s\nContent-Length: %d\nLast-modified: %s\n' %
            (mtype or 'text/plain', size, modified)))
        if host:
            host, port = splitport(host)
        if not host or \
           (not port and socket.gethostbyname(host) in self.get_names()):
            return addinfourl(open(localfile, 'rb'),
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
        dirs = path.split('/')
        dirs, file = dirs[:-1], dirs[-1]
        if dirs and not dirs[0]:
            dirs = dirs[1:]
        user = passwd = '' # XXX
        try:
            fw = self.connect_ftp(user, passwd, host, port, dirs)
            type = file and 'I' or 'D'
            for attr in attrs:
                attr, value = splitattr(attr)
                if attr.lower() == 'type' and \
                   value in ('a', 'A', 'i', 'I', 'd', 'D'):
                    type = value.upper()
            fp, retrlen = fw.retrfile(file, type)
            headers = ""
            mtype = mimetypes.guess_type(req.get_full_url())[0]
            if mtype:
                headers += "Content-Type: %s\n" % mtype
            if retrlen is not None and retrlen >= 0:
                headers += "Content-Length: %d\n" % retrlen
            sf = StringIO(headers)
            headers = mimetools.Message(sf)
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
        opener = OpenerDirector()
        for ph in self.proxy_handlers:
            if inspect.isclass(ph):
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
        'ftp://www.python.org/pub/python/misc/sousa.au',
        'ftp://www.python.org/pub/tmp/blat',
        'http://www.espn.com/', # redirect
        'http://www.python.org/Spanish/Inquistion/',
        ('http://www.python.org/cgi-bin/faqw.py',
         'query=pythonistas&querytype=simple&casefold=yes&req=search'),
        'http://www.python.org/',
        'ftp://gatekeeper.research.compaq.com/pub/DEC/SRC/research-reports/00README-Legal-Rules-Regs',
            ]

##    if localhost is not None:
##        urls = urls + [
##            'file://%s/etc/passwd' % localhost,
##            'http://%s/simple/' % localhost,
##            'http://%s/digest/' % localhost,
##            'http://%s/not/found.h' % localhost,
##            ]

##        bauth = HTTPBasicAuthHandler()
##        bauth.add_password('basic_test_realm', localhost, 'jhylton',
##                           'password')
##        dauth = HTTPDigestAuthHandler()
##        dauth.add_password('digest_test_realm', localhost, 'jhylton',
##                           'password')


    cfh = CacheFTPHandler()
    cfh.setTimeout(1)

##    # XXX try out some custom proxy objects too!
##    def at_cnri(req):
##        host = req.get_host()
##        print host
##        if host[-18:] == '.cnri.reston.va.us':
##            return 1
##    p = CustomProxy('http', at_cnri, 'proxy.cnri.reston.va.us')
##    ph = CustomProxyHandler(p)

##    install_opener(build_opener(dauth, bauth, cfh, GopherHandler, ph))
    install_opener(build_opener(cfh, GopherHandler))

    for url in urls:
        if isinstance(url, types.TupleType):
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
