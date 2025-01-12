import unittest
from test import support
from test.support import os_helper
from test.support import requires_subprocess
from test.support import warnings_helper
from test import test_urllib
from unittest import mock

import os
import io
import socket
import array
import sys
import tempfile
import subprocess

import urllib.request
# The proxy bypass method imported below has logic specific to the
# corresponding system but is testable on all platforms.
from urllib.request import (Request, OpenerDirector, HTTPBasicAuthHandler,
                            HTTPPasswordMgrWithPriorAuth, _parse_proxy,
                            _proxy_bypass_winreg_override,
                            _proxy_bypass_macosx_sysconf,
                            AbstractDigestAuthHandler)
from urllib.parse import urlparse
import urllib.error
import http.client

support.requires_working_socket(module=True)

# XXX
# Request
# CacheFTPHandler (hard to write)
# parse_keqv_list, parse_http_list, HTTPDigestAuthHandler


class TrivialTests(unittest.TestCase):

    def test___all__(self):
        # Verify which names are exposed
        for module in 'request', 'response', 'parse', 'error', 'robotparser':
            context = {}
            exec('from urllib.%s import *' % module, context)
            del context['__builtins__']
            if module == 'request' and os.name == 'nt':
                u, p = context.pop('url2pathname'), context.pop('pathname2url')
                self.assertEqual(u.__module__, 'nturl2path')
                self.assertEqual(p.__module__, 'nturl2path')
            for k, v in context.items():
                self.assertEqual(v.__module__, 'urllib.%s' % module,
                    "%r is exposed in 'urllib.%s' but defined in %r" %
                    (k, module, v.__module__))

    def test_trivial(self):
        # A couple trivial tests

        # clear _opener global variable
        self.addCleanup(urllib.request.urlcleanup)

        self.assertRaises(ValueError, urllib.request.urlopen, 'bogus url')

        # XXX Name hacking to get this to work on Windows.
        fname = os.path.abspath(urllib.request.__file__).replace(os.sep, '/')

        if os.name == 'nt':
            file_url = "file:///%s" % fname
        else:
            file_url = "file://%s" % fname

        with urllib.request.urlopen(file_url) as f:
            f.read()

    def test_parse_http_list(self):
        tests = [
            ('a,b,c', ['a', 'b', 'c']),
            ('path"o,l"og"i"cal, example', ['path"o,l"og"i"cal', 'example']),
            ('a, b, "c", "d", "e,f", g, h',
             ['a', 'b', '"c"', '"d"', '"e,f"', 'g', 'h']),
            ('a="b\\"c", d="e\\,f", g="h\\\\i"',
             ['a="b"c"', 'd="e,f"', 'g="h\\i"'])]
        for string, list in tests:
            self.assertEqual(urllib.request.parse_http_list(string), list)

    def test_URLError_reasonstr(self):
        err = urllib.error.URLError('reason')
        self.assertIn(err.reason, str(err))


class RequestHdrsTests(unittest.TestCase):

    def test_request_headers_dict(self):
        """
        The Request.headers dictionary is not a documented interface.  It
        should stay that way, because the complete set of headers are only
        accessible through the .get_header(), .has_header(), .header_items()
        interface.  However, .headers pre-dates those methods, and so real code
        will be using the dictionary.

        The introduction in 2.4 of those methods was a mistake for the same
        reason: code that previously saw all (urllib2 user)-provided headers in
        .headers now sees only a subset.

        """
        url = "http://example.com"
        self.assertEqual(Request(url,
                                 headers={"Spam-eggs": "blah"}
                                 ).headers["Spam-eggs"], "blah")
        self.assertEqual(Request(url,
                                 headers={"spam-EggS": "blah"}
                                 ).headers["Spam-eggs"], "blah")

    def test_request_headers_methods(self):
        """
        Note the case normalization of header names here, to
        .capitalize()-case.  This should be preserved for
        backwards-compatibility.  (In the HTTP case, normalization to
        .title()-case is done by urllib2 before sending headers to
        http.client).

        Note that e.g. r.has_header("spam-EggS") is currently False, and
        r.get_header("spam-EggS") returns None, but that could be changed in
        future.

        Method r.remove_header should remove items both from r.headers and
        r.unredirected_hdrs dictionaries
        """
        url = "http://example.com"
        req = Request(url, headers={"Spam-eggs": "blah"})
        self.assertTrue(req.has_header("Spam-eggs"))
        self.assertEqual(req.header_items(), [('Spam-eggs', 'blah')])

        req.add_header("Foo-Bar", "baz")
        self.assertEqual(sorted(req.header_items()),
                         [('Foo-bar', 'baz'), ('Spam-eggs', 'blah')])
        self.assertFalse(req.has_header("Not-there"))
        self.assertIsNone(req.get_header("Not-there"))
        self.assertEqual(req.get_header("Not-there", "default"), "default")

        req.remove_header("Spam-eggs")
        self.assertFalse(req.has_header("Spam-eggs"))

        req.add_unredirected_header("Unredirected-spam", "Eggs")
        self.assertTrue(req.has_header("Unredirected-spam"))

        req.remove_header("Unredirected-spam")
        self.assertFalse(req.has_header("Unredirected-spam"))

    def test_password_manager(self):
        mgr = urllib.request.HTTPPasswordMgr()
        add = mgr.add_password
        find_user_pass = mgr.find_user_password

        add("Some Realm", "http://example.com/", "joe", "password")
        add("Some Realm", "http://example.com/ni", "ni", "ni")
        add("Some Realm", "http://c.example.com:3128", "3", "c")
        add("Some Realm", "d.example.com", "4", "d")
        add("Some Realm", "e.example.com:3128", "5", "e")

        # For the same realm, password set the highest path is the winner.
        self.assertEqual(find_user_pass("Some Realm", "example.com"),
                         ('joe', 'password'))
        self.assertEqual(find_user_pass("Some Realm", "http://example.com/ni"),
                         ('joe', 'password'))
        self.assertEqual(find_user_pass("Some Realm", "http://example.com"),
                         ('joe', 'password'))
        self.assertEqual(find_user_pass("Some Realm", "http://example.com/"),
                         ('joe', 'password'))
        self.assertEqual(find_user_pass("Some Realm",
                                        "http://example.com/spam"),
                         ('joe', 'password'))
        self.assertEqual(find_user_pass("Some Realm",
                                        "http://example.com/spam/spam"),
                         ('joe', 'password'))

        # You can have different passwords for different paths.

        add("c", "http://example.com/foo", "foo", "ni")
        add("c", "http://example.com/bar", "bar", "nini")
        add("c", "http://example.com/foo/bar", "foobar", "nibar")

        self.assertEqual(find_user_pass("c", "http://example.com/foo"),
                         ('foo', 'ni'))
        self.assertEqual(find_user_pass("c", "http://example.com/bar"),
                         ('bar', 'nini'))
        self.assertEqual(find_user_pass("c", "http://example.com/foo/"),
                         ('foo', 'ni'))
        self.assertEqual(find_user_pass("c", "http://example.com/foo/bar"),
                         ('foo', 'ni'))
        self.assertEqual(find_user_pass("c", "http://example.com/foo/baz"),
                         ('foo', 'ni'))
        self.assertEqual(find_user_pass("c", "http://example.com/foobar"),
                         (None, None))

        add("c", "http://example.com/baz/", "baz", "ninini")

        self.assertEqual(find_user_pass("c", "http://example.com/baz"),
                         (None, None))
        self.assertEqual(find_user_pass("c", "http://example.com/baz/"),
                         ('baz', 'ninini'))
        self.assertEqual(find_user_pass("c", "http://example.com/baz/bar"),
                         ('baz', 'ninini'))

        # For the same path, newer password should be considered.

        add("b", "http://example.com/", "first", "blah")
        add("b", "http://example.com/", "second", "spam")

        self.assertEqual(find_user_pass("b", "http://example.com/"),
                         ('second', 'spam'))

        # No special relationship between a.example.com and example.com:

        add("a", "http://example.com", "1", "a")
        self.assertEqual(find_user_pass("a", "http://example.com/"),
                         ('1', 'a'))

        self.assertEqual(find_user_pass("a", "http://a.example.com/"),
                         (None, None))

        # Ports:

        self.assertEqual(find_user_pass("Some Realm", "c.example.com"),
                         (None, None))
        self.assertEqual(find_user_pass("Some Realm", "c.example.com:3128"),
                         ('3', 'c'))
        self.assertEqual(
            find_user_pass("Some Realm", "http://c.example.com:3128"),
            ('3', 'c'))
        self.assertEqual(find_user_pass("Some Realm", "d.example.com"),
                         ('4', 'd'))
        self.assertEqual(find_user_pass("Some Realm", "e.example.com:3128"),
                         ('5', 'e'))

    def test_password_manager_default_port(self):
        """
        The point to note here is that we can't guess the default port if
        there's no scheme.  This applies to both add_password and
        find_user_password.
        """
        mgr = urllib.request.HTTPPasswordMgr()
        add = mgr.add_password
        find_user_pass = mgr.find_user_password
        add("f", "http://g.example.com:80", "10", "j")
        add("g", "http://h.example.com", "11", "k")
        add("h", "i.example.com:80", "12", "l")
        add("i", "j.example.com", "13", "m")
        self.assertEqual(find_user_pass("f", "g.example.com:100"),
                         (None, None))
        self.assertEqual(find_user_pass("f", "g.example.com:80"),
                         ('10', 'j'))
        self.assertEqual(find_user_pass("f", "g.example.com"),
                         (None, None))
        self.assertEqual(find_user_pass("f", "http://g.example.com:100"),
                         (None, None))
        self.assertEqual(find_user_pass("f", "http://g.example.com:80"),
                         ('10', 'j'))
        self.assertEqual(find_user_pass("f", "http://g.example.com"),
                         ('10', 'j'))
        self.assertEqual(find_user_pass("g", "h.example.com"), ('11', 'k'))
        self.assertEqual(find_user_pass("g", "h.example.com:80"), ('11', 'k'))
        self.assertEqual(find_user_pass("g", "http://h.example.com:80"),
                         ('11', 'k'))
        self.assertEqual(find_user_pass("h", "i.example.com"), (None, None))
        self.assertEqual(find_user_pass("h", "i.example.com:80"), ('12', 'l'))
        self.assertEqual(find_user_pass("h", "http://i.example.com:80"),
                         ('12', 'l'))
        self.assertEqual(find_user_pass("i", "j.example.com"), ('13', 'm'))
        self.assertEqual(find_user_pass("i", "j.example.com:80"),
                         (None, None))
        self.assertEqual(find_user_pass("i", "http://j.example.com"),
                         ('13', 'm'))
        self.assertEqual(find_user_pass("i", "http://j.example.com:80"),
                         (None, None))


class MockOpener:
    addheaders = []

    def open(self, req, data=None, timeout=socket._GLOBAL_DEFAULT_TIMEOUT):
        self.req, self.data, self.timeout = req, data, timeout

    def error(self, proto, *args):
        self.proto, self.args = proto, args


class MockFile:
    def read(self, count=None):
        pass

    def readline(self, count=None):
        pass

    def close(self):
        pass


class MockHeaders(dict):
    def getheaders(self, name):
        return list(self.values())


class MockResponse(io.StringIO):
    def __init__(self, code, msg, headers, data, url=None):
        io.StringIO.__init__(self, data)
        self.code, self.msg, self.headers, self.url = code, msg, headers, url

    def info(self):
        return self.headers

    def geturl(self):
        return self.url


class MockCookieJar:
    def add_cookie_header(self, request):
        self.ach_req = request

    def extract_cookies(self, response, request):
        self.ec_req, self.ec_r = request, response


class FakeMethod:
    def __init__(self, meth_name, action, handle):
        self.meth_name = meth_name
        self.handle = handle
        self.action = action

    def __call__(self, *args):
        return self.handle(self.meth_name, self.action, *args)


class MockHTTPResponse(io.IOBase):
    def __init__(self, fp, msg, status, reason):
        self.fp = fp
        self.msg = msg
        self.status = status
        self.reason = reason
        self.code = 200

    def read(self):
        return ''

    def info(self):
        return {}

    def geturl(self):
        return self.url


class MockHTTPClass:
    def __init__(self):
        self.level = 0
        self.req_headers = []
        self.data = None
        self.raise_on_endheaders = False
        self.sock = None
        self._tunnel_headers = {}

    def __call__(self, host, timeout=socket._GLOBAL_DEFAULT_TIMEOUT):
        self.host = host
        self.timeout = timeout
        return self

    def set_debuglevel(self, level):
        self.level = level

    def set_tunnel(self, host, port=None, headers=None):
        self._tunnel_host = host
        self._tunnel_port = port
        if headers:
            self._tunnel_headers = headers
        else:
            self._tunnel_headers.clear()

    def request(self, method, url, body=None, headers=None, *,
                encode_chunked=False):
        self.method = method
        self.selector = url
        if headers is not None:
            self.req_headers += headers.items()
        self.req_headers.sort()
        if body:
            self.data = body
        self.encode_chunked = encode_chunked
        if self.raise_on_endheaders:
            raise OSError()

    def getresponse(self):
        return MockHTTPResponse(MockFile(), {}, 200, "OK")

    def close(self):
        pass


class MockHandler:
    # useful for testing handler machinery
    # see add_ordered_mock_handlers() docstring
    handler_order = 500

    def __init__(self, methods):
        self._define_methods(methods)

    def _define_methods(self, methods):
        for spec in methods:
            if len(spec) == 2:
                name, action = spec
            else:
                name, action = spec, None
            meth = FakeMethod(name, action, self.handle)
            setattr(self.__class__, name, meth)

    def handle(self, fn_name, action, *args, **kwds):
        self.parent.calls.append((self, fn_name, args, kwds))
        if action is None:
            return None
        elif action == "return self":
            return self
        elif action == "return response":
            res = MockResponse(200, "OK", {}, "")
            return res
        elif action == "return request":
            return Request("http://blah/")
        elif action.startswith("error"):
            code = action[action.rfind(" ")+1:]
            try:
                code = int(code)
            except ValueError:
                pass
            res = MockResponse(200, "OK", {}, "")
            return self.parent.error("http", args[0], res, code, "", {})
        elif action == "raise":
            raise urllib.error.URLError("blah")
        assert False

    def close(self):
        pass

    def add_parent(self, parent):
        self.parent = parent
        self.parent.calls = []

    def __lt__(self, other):
        if not hasattr(other, "handler_order"):
            # No handler_order, leave in original order.  Yuck.
            return True
        return self.handler_order < other.handler_order


def add_ordered_mock_handlers(opener, meth_spec):
    """Create MockHandlers and add them to an OpenerDirector.

    meth_spec: list of lists of tuples and strings defining methods to define
    on handlers.  eg:

    [["http_error", "ftp_open"], ["http_open"]]

    defines methods .http_error() and .ftp_open() on one handler, and
    .http_open() on another.  These methods just record their arguments and
    return None.  Using a tuple instead of a string causes the method to
    perform some action (see MockHandler.handle()), eg:

    [["http_error"], [("http_open", "return request")]]

    defines .http_error() on one handler (which simply returns None), and
    .http_open() on another handler, which returns a Request object.

    """
    handlers = []
    count = 0
    for meths in meth_spec:
        class MockHandlerSubclass(MockHandler):
            pass

        h = MockHandlerSubclass(meths)
        h.handler_order += count
        h.add_parent(opener)
        count = count + 1
        handlers.append(h)
        opener.add_handler(h)
    return handlers


def build_test_opener(*handler_instances):
    opener = OpenerDirector()
    for h in handler_instances:
        opener.add_handler(h)
    return opener


class MockHTTPHandler(urllib.request.HTTPHandler):
    # Very simple mock HTTP handler with no special behavior other than using a mock HTTP connection

    def __init__(self, debuglevel=None):
        super(MockHTTPHandler, self).__init__(debuglevel=debuglevel)
        self.httpconn = MockHTTPClass()

    def http_open(self, req):
        return self.do_open(self.httpconn, req)


class MockHTTPHandlerRedirect(urllib.request.BaseHandler):
    # useful for testing redirections and auth
    # sends supplied headers and code as first response
    # sends 200 OK as second response
    def __init__(self, code, headers):
        self.code = code
        self.headers = headers
        self.reset()

    def reset(self):
        self._count = 0
        self.requests = []

    def http_open(self, req):
        import email, copy
        self.requests.append(copy.deepcopy(req))
        if self._count == 0:
            self._count = self._count + 1
            name = http.client.responses[self.code]
            msg = email.message_from_string(self.headers)
            return self.parent.error(
                "http", req, MockFile(), self.code, name, msg)
        else:
            self.req = req
            msg = email.message_from_string("\r\n\r\n")
            return MockResponse(200, "OK", msg, "", req.get_full_url())


if hasattr(http.client, 'HTTPSConnection'):
    class MockHTTPSHandler(urllib.request.HTTPSHandler):
        # Useful for testing the Proxy-Authorization request by verifying the
        # properties of httpcon

        def __init__(self, debuglevel=None, context=None, check_hostname=None):
            super(MockHTTPSHandler, self).__init__(debuglevel, context, check_hostname)
            self.httpconn = MockHTTPClass()

        def https_open(self, req):
            return self.do_open(self.httpconn, req)


class MockHTTPHandlerCheckAuth(urllib.request.BaseHandler):
    # useful for testing auth
    # sends supplied code response
    # checks if auth header is specified in request
    def __init__(self, code):
        self.code = code
        self.has_auth_header = False

    def reset(self):
        self.has_auth_header = False

    def http_open(self, req):
        if req.has_header('Authorization'):
            self.has_auth_header = True
        name = http.client.responses[self.code]
        return MockResponse(self.code, name, MockFile(), "", req.get_full_url())



class MockPasswordManager:
    def add_password(self, realm, uri, user, password):
        self.realm = realm
        self.url = uri
        self.user = user
        self.password = password

    def find_user_password(self, realm, authuri):
        self.target_realm = realm
        self.target_url = authuri
        return self.user, self.password


class OpenerDirectorTests(unittest.TestCase):

    def test_add_non_handler(self):
        class NonHandler(object):
            pass
        self.assertRaises(TypeError,
                          OpenerDirector().add_handler, NonHandler())

    def test_badly_named_methods(self):
        # test work-around for three methods that accidentally follow the
        # naming conventions for handler methods
        # (*_open() / *_request() / *_response())

        # These used to call the accidentally-named methods, causing a
        # TypeError in real code; here, returning self from these mock
        # methods would either cause no exception, or AttributeError.

        from urllib.error import URLError

        o = OpenerDirector()
        meth_spec = [
            [("do_open", "return self"), ("proxy_open", "return self")],
            [("redirect_request", "return self")],
            ]
        add_ordered_mock_handlers(o, meth_spec)
        o.add_handler(urllib.request.UnknownHandler())
        for scheme in "do", "proxy", "redirect":
            self.assertRaises(URLError, o.open, scheme+"://example.com/")

    def test_handled(self):
        # handler returning non-None means no more handlers will be called
        o = OpenerDirector()
        meth_spec = [
            ["http_open", "ftp_open", "http_error_302"],
            ["ftp_open"],
            [("http_open", "return self")],
            [("http_open", "return self")],
            ]
        handlers = add_ordered_mock_handlers(o, meth_spec)

        req = Request("http://example.com/")
        r = o.open(req)
        # Second .http_open() gets called, third doesn't, since second returned
        # non-None.  Handlers without .http_open() never get any methods called
        # on them.
        # In fact, second mock handler defining .http_open() returns self
        # (instead of response), which becomes the OpenerDirector's return
        # value.
        self.assertEqual(r, handlers[2])
        calls = [(handlers[0], "http_open"), (handlers[2], "http_open")]
        for expected, got in zip(calls, o.calls):
            handler, name, args, kwds = got
            self.assertEqual((handler, name), expected)
            self.assertEqual(args, (req,))

    def test_handler_order(self):
        o = OpenerDirector()
        handlers = []
        for meths, handler_order in [([("http_open", "return self")], 500),
                                     (["http_open"], 0)]:
            class MockHandlerSubclass(MockHandler):
                pass

            h = MockHandlerSubclass(meths)
            h.handler_order = handler_order
            handlers.append(h)
            o.add_handler(h)

        o.open("http://example.com/")
        # handlers called in reverse order, thanks to their sort order
        self.assertEqual(o.calls[0][0], handlers[1])
        self.assertEqual(o.calls[1][0], handlers[0])

    def test_raise(self):
        # raising URLError stops processing of request
        o = OpenerDirector()
        meth_spec = [
            [("http_open", "raise")],
            [("http_open", "return self")],
            ]
        handlers = add_ordered_mock_handlers(o, meth_spec)

        req = Request("http://example.com/")
        self.assertRaises(urllib.error.URLError, o.open, req)
        self.assertEqual(o.calls, [(handlers[0], "http_open", (req,), {})])

    def test_http_error(self):
        # XXX http_error_default
        # http errors are a special case
        o = OpenerDirector()
        meth_spec = [
            [("http_open", "error 302")],
            [("http_error_400", "raise"), "http_open"],
            [("http_error_302", "return response"), "http_error_303",
             "http_error"],
            [("http_error_302")],
            ]
        handlers = add_ordered_mock_handlers(o, meth_spec)
        req = Request("http://example.com/")
        o.open(req)
        assert len(o.calls) == 2
        calls = [(handlers[0], "http_open", (req,)),
                 (handlers[2], "http_error_302",
                  (req, support.ALWAYS_EQ, 302, "", {}))]
        for expected, got in zip(calls, o.calls):
            handler, method_name, args = expected
            self.assertEqual((handler, method_name), got[:2])
            self.assertEqual(args, got[2])

    def test_processors(self):
        # *_request / *_response methods get called appropriately
        o = OpenerDirector()
        meth_spec = [
            [("http_request", "return request"),
             ("http_response", "return response")],
            [("http_request", "return request"),
             ("http_response", "return response")],
            ]
        handlers = add_ordered_mock_handlers(o, meth_spec)

        req = Request("http://example.com/")
        o.open(req)
        # processor methods are called on *all* handlers that define them,
        # not just the first handler that handles the request
        calls = [
            (handlers[0], "http_request"), (handlers[1], "http_request"),
            (handlers[0], "http_response"), (handlers[1], "http_response")]

        for i, (handler, name, args, kwds) in enumerate(o.calls):
            if i < 2:
                # *_request
                self.assertEqual((handler, name), calls[i])
                self.assertEqual(len(args), 1)
                self.assertIsInstance(args[0], Request)
            else:
                # *_response
                self.assertEqual((handler, name), calls[i])
                self.assertEqual(len(args), 2)
                self.assertIsInstance(args[0], Request)
                # response from opener.open is None, because there's no
                # handler that defines http_open to handle it
                if args[1] is not None:
                    self.assertIsInstance(args[1], MockResponse)


def sanepathname2url(path):
    urlpath = urllib.request.pathname2url(path)
    if os.name == "nt" and urlpath.startswith("///"):
        urlpath = urlpath[2:]
    # XXX don't ask me about the mac...
    return urlpath


class HandlerTests(unittest.TestCase):

    def test_ftp(self):
        class MockFTPWrapper:
            def __init__(self, data):
                self.data = data

            def retrfile(self, filename, filetype):
                self.filename, self.filetype = filename, filetype
                return io.StringIO(self.data), len(self.data)

            def close(self):
                pass

        class NullFTPHandler(urllib.request.FTPHandler):
            def __init__(self, data):
                self.data = data

            def connect_ftp(self, user, passwd, host, port, dirs,
                            timeout=socket._GLOBAL_DEFAULT_TIMEOUT):
                self.user, self.passwd = user, passwd
                self.host, self.port = host, port
                self.dirs = dirs
                self.ftpwrapper = MockFTPWrapper(self.data)
                return self.ftpwrapper

        import ftplib
        data = "rheum rhaponicum"
        h = NullFTPHandler(data)
        h.parent = MockOpener()

        for url, host, port, user, passwd, type_, dirs, filename, mimetype in [
            ("ftp://localhost/foo/bar/baz.html",
             "localhost", ftplib.FTP_PORT, "", "", "I",
             ["foo", "bar"], "baz.html", "text/html"),
            ("ftp://parrot@localhost/foo/bar/baz.html",
             "localhost", ftplib.FTP_PORT, "parrot", "", "I",
             ["foo", "bar"], "baz.html", "text/html"),
            ("ftp://%25parrot@localhost/foo/bar/baz.html",
             "localhost", ftplib.FTP_PORT, "%parrot", "", "I",
             ["foo", "bar"], "baz.html", "text/html"),
            ("ftp://%2542parrot@localhost/foo/bar/baz.html",
             "localhost", ftplib.FTP_PORT, "%42parrot", "", "I",
             ["foo", "bar"], "baz.html", "text/html"),
            ("ftp://localhost:80/foo/bar/",
             "localhost", 80, "", "", "D",
             ["foo", "bar"], "", None),
            ("ftp://localhost/baz.gif;type=a",
             "localhost", ftplib.FTP_PORT, "", "", "A",
             [], "baz.gif", "image/gif"),
            ]:
            req = Request(url)
            req.timeout = None
            r = h.ftp_open(req)
            # ftp authentication not yet implemented by FTPHandler
            self.assertEqual(h.user, user)
            self.assertEqual(h.passwd, passwd)
            self.assertEqual(h.host, socket.gethostbyname(host))
            self.assertEqual(h.port, port)
            self.assertEqual(h.dirs, dirs)
            self.assertEqual(h.ftpwrapper.filename, filename)
            self.assertEqual(h.ftpwrapper.filetype, type_)
            headers = r.info()
            self.assertEqual(headers.get("Content-type"), mimetype)
            self.assertEqual(int(headers["Content-length"]), len(data))
            r.close()

    def test_file(self):
        import email.utils
        h = urllib.request.FileHandler()
        o = h.parent = MockOpener()

        TESTFN = os_helper.TESTFN
        urlpath = sanepathname2url(os.path.abspath(TESTFN))
        towrite = b"hello, world\n"
        urls = [
            "file://localhost%s" % urlpath,
            "file://%s" % urlpath,
            "file://%s%s" % (socket.gethostbyname('localhost'), urlpath),
            ]
        try:
            localaddr = socket.gethostbyname(socket.gethostname())
        except socket.gaierror:
            localaddr = ''
        if localaddr:
            urls.append("file://%s%s" % (localaddr, urlpath))

        for url in urls:
            f = open(TESTFN, "wb")
            try:
                try:
                    f.write(towrite)
                finally:
                    f.close()

                r = h.file_open(Request(url))
                try:
                    data = r.read()
                    headers = r.info()
                    respurl = r.geturl()
                finally:
                    r.close()
                stats = os.stat(TESTFN)
                modified = email.utils.formatdate(stats.st_mtime, usegmt=True)
            finally:
                os.remove(TESTFN)
            self.assertEqual(data, towrite)
            self.assertEqual(headers["Content-type"], "text/plain")
            self.assertEqual(headers["Content-length"], "13")
            self.assertEqual(headers["Last-modified"], modified)
            self.assertEqual(respurl, url)

        for url in [
            "file://localhost:80%s" % urlpath,
            "file:///file_does_not_exist.txt",
            "file://not-a-local-host.com//dir/file.txt",
            "file://%s:80%s/%s" % (socket.gethostbyname('localhost'),
                                   os.getcwd(), TESTFN),
            "file://somerandomhost.ontheinternet.com%s/%s" %
            (os.getcwd(), TESTFN),
            ]:
            try:
                f = open(TESTFN, "wb")
                try:
                    f.write(towrite)
                finally:
                    f.close()

                self.assertRaises(urllib.error.URLError,
                                  h.file_open, Request(url))
            finally:
                os.remove(TESTFN)

        h = urllib.request.FileHandler()
        o = h.parent = MockOpener()
        # XXXX why does // mean ftp (and /// mean not ftp!), and where
        #  is file: scheme specified?  I think this is really a bug, and
        #  what was intended was to distinguish between URLs like:
        # file:/blah.txt (a file)
        # file://localhost/blah.txt (a file)
        # file:///blah.txt (a file)
        # file://ftp.example.com/blah.txt (an ftp URL)
        for url, ftp in [
            ("file://ftp.example.com//foo.txt", False),
            ("file://ftp.example.com///foo.txt", False),
            ("file://ftp.example.com/foo.txt", False),
            ("file://somehost//foo/something.txt", False),
            ("file://localhost//foo/something.txt", False),
            ]:
            req = Request(url)
            try:
                h.file_open(req)
            except urllib.error.URLError:
                self.assertFalse(ftp)
            else:
                self.assertIs(o.req, req)
                self.assertEqual(req.type, "ftp")
            self.assertEqual(req.type == "ftp", ftp)

    def test_http(self):

        h = urllib.request.AbstractHTTPHandler()
        o = h.parent = MockOpener()

        url = "http://example.com/"
        for method, data in [("GET", None), ("POST", b"blah")]:
            req = Request(url, data, {"Foo": "bar"})
            req.timeout = None
            req.add_unredirected_header("Spam", "eggs")
            http = MockHTTPClass()
            r = h.do_open(http, req)

            # result attributes
            r.read; r.readline  # wrapped MockFile methods
            r.info; r.geturl  # addinfourl methods
            r.code, r.msg == 200, "OK"  # added from MockHTTPClass.getreply()
            hdrs = r.info()
            hdrs.get; hdrs.__contains__  # r.info() gives dict from .getreply()
            self.assertEqual(r.geturl(), url)

            self.assertEqual(http.host, "example.com")
            self.assertEqual(http.level, 0)
            self.assertEqual(http.method, method)
            self.assertEqual(http.selector, "/")
            self.assertEqual(http.req_headers,
                             [("Connection", "close"),
                              ("Foo", "bar"), ("Spam", "eggs")])
            self.assertEqual(http.data, data)

        # check OSError converted to URLError
        http.raise_on_endheaders = True
        self.assertRaises(urllib.error.URLError, h.do_open, http, req)

        # Check for TypeError on POST data which is str.
        req = Request("http://example.com/","badpost")
        self.assertRaises(TypeError, h.do_request_, req)

        # check adding of standard headers
        o.addheaders = [("Spam", "eggs")]
        for data in b"", None:  # POST, GET
            req = Request("http://example.com/", data)
            r = MockResponse(200, "OK", {}, "")
            newreq = h.do_request_(req)
            if data is None:  # GET
                self.assertNotIn("Content-length", req.unredirected_hdrs)
                self.assertNotIn("Content-type", req.unredirected_hdrs)
            else:  # POST
                self.assertEqual(req.unredirected_hdrs["Content-length"], "0")
                self.assertEqual(req.unredirected_hdrs["Content-type"],
                             "application/x-www-form-urlencoded")
            # XXX the details of Host could be better tested
            self.assertEqual(req.unredirected_hdrs["Host"], "example.com")
            self.assertEqual(req.unredirected_hdrs["Spam"], "eggs")

            # don't clobber existing headers
            req.add_unredirected_header("Content-length", "foo")
            req.add_unredirected_header("Content-type", "bar")
            req.add_unredirected_header("Host", "baz")
            req.add_unredirected_header("Spam", "foo")
            newreq = h.do_request_(req)
            self.assertEqual(req.unredirected_hdrs["Content-length"], "foo")
            self.assertEqual(req.unredirected_hdrs["Content-type"], "bar")
            self.assertEqual(req.unredirected_hdrs["Host"], "baz")
            self.assertEqual(req.unredirected_hdrs["Spam"], "foo")

    def test_http_body_file(self):
        # A regular file - chunked encoding is used unless Content Length is
        # already set.

        h = urllib.request.AbstractHTTPHandler()
        o = h.parent = MockOpener()

        file_obj = tempfile.NamedTemporaryFile(mode='w+b', delete=False)
        file_path = file_obj.name
        file_obj.close()
        self.addCleanup(os.unlink, file_path)

        with open(file_path, "rb") as f:
            req = Request("http://example.com/", f, {})
            newreq = h.do_request_(req)
            te = newreq.get_header('Transfer-encoding')
            self.assertEqual(te, "chunked")
            self.assertFalse(newreq.has_header('Content-length'))

        with open(file_path, "rb") as f:
            req = Request("http://example.com/", f, {"Content-Length": 30})
            newreq = h.do_request_(req)
            self.assertEqual(int(newreq.get_header('Content-length')), 30)
            self.assertFalse(newreq.has_header("Transfer-encoding"))

    def test_http_body_fileobj(self):
        # A file object - chunked encoding is used
        # unless Content Length is already set.
        # (Note that there are some subtle differences to a regular
        # file, that is why we are testing both cases.)

        h = urllib.request.AbstractHTTPHandler()
        o = h.parent = MockOpener()
        file_obj = io.BytesIO()

        req = Request("http://example.com/", file_obj, {})
        newreq = h.do_request_(req)
        self.assertEqual(newreq.get_header('Transfer-encoding'), 'chunked')
        self.assertFalse(newreq.has_header('Content-length'))

        headers = {"Content-Length": 30}
        req = Request("http://example.com/", file_obj, headers)
        newreq = h.do_request_(req)
        self.assertEqual(int(newreq.get_header('Content-length')), 30)
        self.assertFalse(newreq.has_header("Transfer-encoding"))

        file_obj.close()

    @requires_subprocess()
    def test_http_body_pipe(self):
        # A file reading from a pipe.
        # A pipe cannot be seek'ed.  There is no way to determine the
        # content length up front.  Thus, do_request_() should fall
        # back to Transfer-encoding chunked.

        h = urllib.request.AbstractHTTPHandler()
        o = h.parent = MockOpener()

        cmd = [sys.executable, "-c", r"pass"]
        for headers in {}, {"Content-Length": 30}:
            with subprocess.Popen(cmd, stdout=subprocess.PIPE) as proc:
                req = Request("http://example.com/", proc.stdout, headers)
                newreq = h.do_request_(req)
                if not headers:
                    self.assertEqual(newreq.get_header('Content-length'), None)
                    self.assertEqual(newreq.get_header('Transfer-encoding'),
                                     'chunked')
                else:
                    self.assertEqual(int(newreq.get_header('Content-length')),
                                     30)

    def test_http_body_iterable(self):
        # Generic iterable.  There is no way to determine the content
        # length up front.  Fall back to Transfer-encoding chunked.

        h = urllib.request.AbstractHTTPHandler()
        o = h.parent = MockOpener()

        def iterable_body():
            yield b"one"

        for headers in {}, {"Content-Length": 11}:
            req = Request("http://example.com/", iterable_body(), headers)
            newreq = h.do_request_(req)
            if not headers:
                self.assertEqual(newreq.get_header('Content-length'), None)
                self.assertEqual(newreq.get_header('Transfer-encoding'),
                                 'chunked')
            else:
                self.assertEqual(int(newreq.get_header('Content-length')), 11)

    def test_http_body_empty_seq(self):
        # Zero-length iterable body should be treated like any other iterable
        h = urllib.request.AbstractHTTPHandler()
        h.parent = MockOpener()
        req = h.do_request_(Request("http://example.com/", ()))
        self.assertEqual(req.get_header("Transfer-encoding"), "chunked")
        self.assertFalse(req.has_header("Content-length"))

    def test_http_body_array(self):
        # array.array Iterable - Content Length is calculated

        h = urllib.request.AbstractHTTPHandler()
        o = h.parent = MockOpener()

        iterable_array = array.array("I",[1,2,3,4])

        for headers in {}, {"Content-Length": 16}:
            req = Request("http://example.com/", iterable_array, headers)
            newreq = h.do_request_(req)
            self.assertEqual(int(newreq.get_header('Content-length')),16)

    def test_http_handler_global_debuglevel(self):
        with mock.patch.object(http.client.HTTPConnection, 'debuglevel', 6):
            o = OpenerDirector()
            h = MockHTTPHandler()
            o.add_handler(h)
            o.open("http://www.example.com")
            self.assertEqual(h._debuglevel, 6)

    def test_http_handler_local_debuglevel(self):
        o = OpenerDirector()
        h = MockHTTPHandler(debuglevel=5)
        o.add_handler(h)
        o.open("http://www.example.com")
        self.assertEqual(h._debuglevel, 5)

    @unittest.skipUnless(hasattr(http.client, 'HTTPSConnection'), 'HTTPSConnection required for HTTPS tests.')
    def test_https_handler_global_debuglevel(self):
        with mock.patch.object(http.client.HTTPSConnection, 'debuglevel', 7):
            o = OpenerDirector()
            h = MockHTTPSHandler()
            o.add_handler(h)
            o.open("https://www.example.com")
            self.assertEqual(h._debuglevel, 7)

    @unittest.skipUnless(hasattr(http.client, 'HTTPSConnection'), 'HTTPSConnection required for HTTPS tests.')
    def test_https_handler_local_debuglevel(self):
        o = OpenerDirector()
        h = MockHTTPSHandler(debuglevel=4)
        o.add_handler(h)
        o.open("https://www.example.com")
        self.assertEqual(h._debuglevel, 4)

    def test_http_doubleslash(self):
        # Checks the presence of any unnecessary double slash in url does not
        # break anything. Previously, a double slash directly after the host
        # could cause incorrect parsing.
        h = urllib.request.AbstractHTTPHandler()
        h.parent = MockOpener()

        data = b""
        ds_urls = [
            "http://example.com/foo/bar/baz.html",
            "http://example.com//foo/bar/baz.html",
            "http://example.com/foo//bar/baz.html",
            "http://example.com/foo/bar//baz.html"
            ]

        for ds_url in ds_urls:
            ds_req = Request(ds_url, data)

            # Check whether host is determined correctly if there is no proxy
            np_ds_req = h.do_request_(ds_req)
            self.assertEqual(np_ds_req.unredirected_hdrs["Host"], "example.com")

            # Check whether host is determined correctly if there is a proxy
            ds_req.set_proxy("someproxy:3128", None)
            p_ds_req = h.do_request_(ds_req)
            self.assertEqual(p_ds_req.unredirected_hdrs["Host"], "example.com")

    def test_full_url_setter(self):
        # Checks to ensure that components are set correctly after setting the
        # full_url of a Request object

        urls = [
            'http://example.com?foo=bar#baz',
            'http://example.com?foo=bar&spam=eggs#bash',
            'http://example.com',
        ]

        # testing a reusable request instance, but the url parameter is
        # required, so just use a dummy one to instantiate
        r = Request('http://example.com')
        for url in urls:
            r.full_url = url
            parsed = urlparse(url)

            self.assertEqual(r.get_full_url(), url)
            # full_url setter uses splittag to split into components.
            # splittag sets the fragment as None while urlparse sets it to ''
            self.assertEqual(r.fragment or '', parsed.fragment)
            self.assertEqual(urlparse(r.get_full_url()).query, parsed.query)

    def test_full_url_deleter(self):
        r = Request('http://www.example.com')
        del r.full_url
        self.assertIsNone(r.full_url)
        self.assertIsNone(r.fragment)
        self.assertEqual(r.selector, '')

    def test_fixpath_in_weirdurls(self):
        # Issue4493: urllib2 to supply '/' when to urls where path does not
        # start with'/'

        h = urllib.request.AbstractHTTPHandler()
        h.parent = MockOpener()

        weird_url = 'http://www.python.org?getspam'
        req = Request(weird_url)
        newreq = h.do_request_(req)
        self.assertEqual(newreq.host, 'www.python.org')
        self.assertEqual(newreq.selector, '/?getspam')

        url_without_path = 'http://www.python.org'
        req = Request(url_without_path)
        newreq = h.do_request_(req)
        self.assertEqual(newreq.host, 'www.python.org')
        self.assertEqual(newreq.selector, '')

    def test_errors(self):
        h = urllib.request.HTTPErrorProcessor()
        o = h.parent = MockOpener()

        url = "http://example.com/"
        req = Request(url)
        # all 2xx are passed through
        r = MockResponse(200, "OK", {}, "", url)
        newr = h.http_response(req, r)
        self.assertIs(r, newr)
        self.assertFalse(hasattr(o, "proto"))  # o.error not called
        r = MockResponse(202, "Accepted", {}, "", url)
        newr = h.http_response(req, r)
        self.assertIs(r, newr)
        self.assertFalse(hasattr(o, "proto"))  # o.error not called
        r = MockResponse(206, "Partial content", {}, "", url)
        newr = h.http_response(req, r)
        self.assertIs(r, newr)
        self.assertFalse(hasattr(o, "proto"))  # o.error not called
        # anything else calls o.error (and MockOpener returns None, here)
        r = MockResponse(502, "Bad gateway", {}, "", url)
        self.assertIsNone(h.http_response(req, r))
        self.assertEqual(o.proto, "http")  # o.error called
        self.assertEqual(o.args, (req, r, 502, "Bad gateway", {}))

    def test_cookies(self):
        cj = MockCookieJar()
        h = urllib.request.HTTPCookieProcessor(cj)
        h.parent = MockOpener()

        req = Request("http://example.com/")
        r = MockResponse(200, "OK", {}, "")
        newreq = h.http_request(req)
        self.assertIs(cj.ach_req, req)
        self.assertIs(cj.ach_req, newreq)
        self.assertEqual(req.origin_req_host, "example.com")
        self.assertFalse(req.unverifiable)
        newr = h.http_response(req, r)
        self.assertIs(cj.ec_req, req)
        self.assertIs(cj.ec_r, r)
        self.assertIs(r, newr)

    def test_redirect(self):
        from_url = "http://example.com/a.html"
        to_url = "http://example.com/b.html"
        h = urllib.request.HTTPRedirectHandler()
        o = h.parent = MockOpener()

        # ordinary redirect behaviour
        for code in 301, 302, 303, 307, 308:
            for data in None, "blah\nblah\n":
                method = getattr(h, "http_error_%s" % code)
                req = Request(from_url, data)
                req.timeout = socket._GLOBAL_DEFAULT_TIMEOUT
                req.add_header("Nonsense", "viking=withhold")
                if data is not None:
                    req.add_header("Content-Length", str(len(data)))
                req.add_unredirected_header("Spam", "spam")
                try:
                    method(req, MockFile(), code, "Blah",
                           MockHeaders({"location": to_url}))
                except urllib.error.HTTPError as err:
                    # 307 and 308 in response to POST require user OK
                    self.assertIn(code, (307, 308))
                    self.assertIsNotNone(data)
                    err.close()
                self.assertEqual(o.req.get_full_url(), to_url)
                try:
                    self.assertEqual(o.req.get_method(), "GET")
                except AttributeError:
                    self.assertFalse(o.req.data)

                # now it's a GET, there should not be headers regarding content
                # (possibly dragged from before being a POST)
                headers = [x.lower() for x in o.req.headers]
                self.assertNotIn("content-length", headers)
                self.assertNotIn("content-type", headers)

                self.assertEqual(o.req.headers["Nonsense"],
                                 "viking=withhold")
                self.assertNotIn("Spam", o.req.headers)
                self.assertNotIn("Spam", o.req.unredirected_hdrs)

        # loop detection
        req = Request(from_url)
        req.timeout = socket._GLOBAL_DEFAULT_TIMEOUT

        def redirect(h, req, url=to_url):
            h.http_error_302(req, MockFile(), 302, "Blah",
                             MockHeaders({"location": url}))
        # Note that the *original* request shares the same record of
        # redirections with the sub-requests caused by the redirections.

        # detect infinite loop redirect of a URL to itself
        req = Request(from_url, origin_req_host="example.com")
        count = 0
        req.timeout = socket._GLOBAL_DEFAULT_TIMEOUT
        try:
            while 1:
                redirect(h, req, "http://example.com/")
                count = count + 1
        except urllib.error.HTTPError as err:
            # don't stop until max_repeats, because cookies may introduce state
            self.assertEqual(count, urllib.request.HTTPRedirectHandler.max_repeats)
            err.close()

        # detect endless non-repeating chain of redirects
        req = Request(from_url, origin_req_host="example.com")
        count = 0
        req.timeout = socket._GLOBAL_DEFAULT_TIMEOUT
        try:
            while 1:
                redirect(h, req, "http://example.com/%d" % count)
                count = count + 1
        except urllib.error.HTTPError as err:
            self.assertEqual(count,
                             urllib.request.HTTPRedirectHandler.max_redirections)
            err.close()

    def test_invalid_redirect(self):
        from_url = "http://example.com/a.html"
        valid_schemes = ['http','https','ftp']
        invalid_schemes = ['file','imap','ldap']
        schemeless_url = "example.com/b.html"
        h = urllib.request.HTTPRedirectHandler()
        o = h.parent = MockOpener()
        req = Request(from_url)
        req.timeout = socket._GLOBAL_DEFAULT_TIMEOUT

        for scheme in invalid_schemes:
            invalid_url = scheme + '://' + schemeless_url
            with self.assertRaises(urllib.error.HTTPError) as cm:
                h.http_error_302(
                    req, MockFile(), 302, "Security Loophole",
                    MockHeaders({"location": invalid_url}))
            cm.exception.close()

        for scheme in valid_schemes:
            valid_url = scheme + '://' + schemeless_url
            h.http_error_302(req, MockFile(), 302, "That's fine",
                MockHeaders({"location": valid_url}))
            self.assertEqual(o.req.get_full_url(), valid_url)

    def test_relative_redirect(self):
        from_url = "http://example.com/a.html"
        relative_url = "/b.html"
        h = urllib.request.HTTPRedirectHandler()
        o = h.parent = MockOpener()
        req = Request(from_url)
        req.timeout = socket._GLOBAL_DEFAULT_TIMEOUT

        valid_url = urllib.parse.urljoin(from_url,relative_url)
        h.http_error_302(req, MockFile(), 302, "That's fine",
            MockHeaders({"location": valid_url}))
        self.assertEqual(o.req.get_full_url(), valid_url)

    def test_cookie_redirect(self):
        # cookies shouldn't leak into redirected requests
        from http.cookiejar import CookieJar
        from test.test_http_cookiejar import interact_netscape

        cj = CookieJar()
        interact_netscape(cj, "http://www.example.com/", "spam=eggs")
        hh = MockHTTPHandlerRedirect(302, "Location: http://www.cracker.com/\r\n\r\n")
        hdeh = urllib.request.HTTPDefaultErrorHandler()
        hrh = urllib.request.HTTPRedirectHandler()
        cp = urllib.request.HTTPCookieProcessor(cj)
        o = build_test_opener(hh, hdeh, hrh, cp)
        o.open("http://www.example.com/")
        self.assertFalse(hh.req.has_header("Cookie"))

    def test_redirect_fragment(self):
        redirected_url = 'http://www.example.com/index.html#OK\r\n\r\n'
        hh = MockHTTPHandlerRedirect(302, 'Location: ' + redirected_url)
        hdeh = urllib.request.HTTPDefaultErrorHandler()
        hrh = urllib.request.HTTPRedirectHandler()
        o = build_test_opener(hh, hdeh, hrh)
        fp = o.open('http://www.example.com')
        self.assertEqual(fp.geturl(), redirected_url.strip())

    def test_redirect_no_path(self):
        # Issue 14132: Relative redirect strips original path

        # clear _opener global variable
        self.addCleanup(urllib.request.urlcleanup)

        real_class = http.client.HTTPConnection
        response1 = b"HTTP/1.1 302 Found\r\nLocation: ?query\r\n\r\n"
        http.client.HTTPConnection = test_urllib.fakehttp(response1)
        self.addCleanup(setattr, http.client, "HTTPConnection", real_class)
        urls = iter(("/path", "/path?query"))
        def request(conn, method, url, *pos, **kw):
            self.assertEqual(url, next(urls))
            real_class.request(conn, method, url, *pos, **kw)
            # Change response for subsequent connection
            conn.__class__.fakedata = b"HTTP/1.1 200 OK\r\n\r\nHello!"
        http.client.HTTPConnection.request = request
        fp = urllib.request.urlopen("http://python.org/path")
        self.assertEqual(fp.geturl(), "http://python.org/path?query")

    def test_redirect_encoding(self):
        # Some characters in the redirect target may need special handling,
        # but most ASCII characters should be treated as already encoded
        class Handler(urllib.request.HTTPHandler):
            def http_open(self, req):
                result = self.do_open(self.connection, req)
                self.last_buf = self.connection.buf
                # Set up a normal response for the next request
                self.connection = test_urllib.fakehttp(
                    b'HTTP/1.1 200 OK\r\n'
                    b'Content-Length: 3\r\n'
                    b'\r\n'
                    b'123'
                )
                return result
        handler = Handler()
        opener = urllib.request.build_opener(handler)
        tests = (
            (b'/p\xC3\xA5-dansk/', b'/p%C3%A5-dansk/'),
            (b'/spaced%20path/', b'/spaced%20path/'),
            (b'/spaced path/', b'/spaced%20path/'),
            (b'/?p\xC3\xA5-dansk', b'/?p%C3%A5-dansk'),
        )
        for [location, result] in tests:
            with self.subTest(repr(location)):
                handler.connection = test_urllib.fakehttp(
                    b'HTTP/1.1 302 Redirect\r\n'
                    b'Location: ' + location + b'\r\n'
                    b'\r\n'
                )
                response = opener.open('http://example.com/')
                expected = b'GET ' + result + b' '
                request = handler.last_buf
                self.assertTrue(request.startswith(expected), repr(request))

    def test_redirect_head_request(self):
        from_url = "http://example.com/a.html"
        to_url = "http://example.com/b.html"
        h = urllib.request.HTTPRedirectHandler()
        req = Request(from_url, method="HEAD")
        fp = MockFile()
        new_req = h.redirect_request(req, fp, 302, "Found", {}, to_url)
        self.assertEqual(new_req.get_method(), "HEAD")

    def test_proxy(self):
        u = "proxy.example.com:3128"
        for d in dict(http=u), dict(HTTP=u):
            o = OpenerDirector()
            ph = urllib.request.ProxyHandler(d)
            o.add_handler(ph)
            meth_spec = [
                [("http_open", "return response")]
                ]
            handlers = add_ordered_mock_handlers(o, meth_spec)

            req = Request("http://acme.example.com/")
            self.assertEqual(req.host, "acme.example.com")
            o.open(req)
            self.assertEqual(req.host, u)
            self.assertEqual([(handlers[0], "http_open")],
                             [tup[0:2] for tup in o.calls])

    def test_proxy_no_proxy(self):
        os.environ['no_proxy'] = 'python.org'
        o = OpenerDirector()
        ph = urllib.request.ProxyHandler(dict(http="proxy.example.com"))
        o.add_handler(ph)
        req = Request("http://www.perl.org/")
        self.assertEqual(req.host, "www.perl.org")
        o.open(req)
        self.assertEqual(req.host, "proxy.example.com")
        req = Request("http://www.python.org")
        self.assertEqual(req.host, "www.python.org")
        o.open(req)
        self.assertEqual(req.host, "www.python.org")
        del os.environ['no_proxy']

    def test_proxy_no_proxy_all(self):
        os.environ['no_proxy'] = '*'
        o = OpenerDirector()
        ph = urllib.request.ProxyHandler(dict(http="proxy.example.com"))
        o.add_handler(ph)
        req = Request("http://www.python.org")
        self.assertEqual(req.host, "www.python.org")
        o.open(req)
        self.assertEqual(req.host, "www.python.org")
        del os.environ['no_proxy']

    def test_proxy_https(self):
        o = OpenerDirector()
        ph = urllib.request.ProxyHandler(dict(https="proxy.example.com:3128"))
        o.add_handler(ph)
        meth_spec = [
            [("https_open", "return response")]
        ]
        handlers = add_ordered_mock_handlers(o, meth_spec)

        req = Request("https://www.example.com/")
        self.assertEqual(req.host, "www.example.com")
        o.open(req)
        self.assertEqual(req.host, "proxy.example.com:3128")
        self.assertEqual([(handlers[0], "https_open")],
                         [tup[0:2] for tup in o.calls])

    @unittest.skipUnless(hasattr(http.client, 'HTTPSConnection'), 'HTTPSConnection required for HTTPS tests.')
    def test_proxy_https_proxy_authorization(self):
        o = OpenerDirector()
        ph = urllib.request.ProxyHandler(dict(https='proxy.example.com:3128'))
        o.add_handler(ph)
        https_handler = MockHTTPSHandler()
        o.add_handler(https_handler)
        req = Request("https://www.example.com/")
        req.add_header("Proxy-Authorization", "FooBar")
        req.add_header("User-Agent", "Grail")
        self.assertEqual(req.host, "www.example.com")
        self.assertIsNone(req._tunnel_host)
        o.open(req)
        # Verify Proxy-Authorization gets tunneled to request.
        # httpsconn req_headers do not have the Proxy-Authorization header but
        # the req will have.
        self.assertNotIn(("Proxy-Authorization", "FooBar"),
                         https_handler.httpconn.req_headers)
        self.assertIn(("User-Agent", "Grail"),
                      https_handler.httpconn.req_headers)
        self.assertIsNotNone(req._tunnel_host)
        self.assertEqual(req.host, "proxy.example.com:3128")
        self.assertEqual(req.get_header("Proxy-authorization"), "FooBar")

    @unittest.skipUnless(os.name == "nt", "only relevant for Windows")
    def test_winreg_proxy_bypass(self):
        proxy_override = "www.example.com;*.example.net; 192.168.0.1"
        proxy_bypass = _proxy_bypass_winreg_override
        for host in ("www.example.com", "www.example.net", "192.168.0.1"):
            self.assertTrue(proxy_bypass(host, proxy_override),
                            "expected bypass of %s to be true" % host)

        for host in ("example.com", "www.example.org", "example.net",
                     "192.168.0.2"):
            self.assertFalse(proxy_bypass(host, proxy_override),
                             "expected bypass of %s to be False" % host)

        # check intranet address bypass
        proxy_override = "example.com; <local>"
        self.assertTrue(proxy_bypass("example.com", proxy_override),
                        "expected bypass of %s to be true" % host)
        self.assertFalse(proxy_bypass("example.net", proxy_override),
                         "expected bypass of %s to be False" % host)
        for host in ("test", "localhost"):
            self.assertTrue(proxy_bypass(host, proxy_override),
                            "expect <local> to bypass intranet address '%s'"
                            % host)

    @unittest.skipUnless(sys.platform == 'darwin', "only relevant for OSX")
    def test_osx_proxy_bypass(self):
        bypass = {
            'exclude_simple': False,
            'exceptions': ['foo.bar', '*.bar.com', '127.0.0.1', '10.10',
                           '10.0/16']
        }
        # Check hosts that should trigger the proxy bypass
        for host in ('foo.bar', 'www.bar.com', '127.0.0.1', '10.10.0.1',
                     '10.0.0.1'):
            self.assertTrue(_proxy_bypass_macosx_sysconf(host, bypass),
                            'expected bypass of %s to be True' % host)
        # Check hosts that should not trigger the proxy bypass
        for host in ('abc.foo.bar', 'bar.com', '127.0.0.2', '10.11.0.1',
                'notinbypass'):
            self.assertFalse(_proxy_bypass_macosx_sysconf(host, bypass),
                             'expected bypass of %s to be False' % host)

        # Check the exclude_simple flag
        bypass = {'exclude_simple': True, 'exceptions': []}
        self.assertTrue(_proxy_bypass_macosx_sysconf('test', bypass))

        # Check that invalid prefix lengths are ignored
        bypass = {
            'exclude_simple': False,
            'exceptions': [ '10.0.0.0/40', '172.19.10.0/24' ]
        }
        host = '172.19.10.5'
        self.assertTrue(_proxy_bypass_macosx_sysconf(host, bypass),
                        'expected bypass of %s to be True' % host)
        host = '10.0.1.5'
        self.assertFalse(_proxy_bypass_macosx_sysconf(host, bypass),
                        'expected bypass of %s to be False' % host)

    def check_basic_auth(self, headers, realm):
        with self.subTest(realm=realm, headers=headers):
            opener = OpenerDirector()
            password_manager = MockPasswordManager()
            auth_handler = urllib.request.HTTPBasicAuthHandler(password_manager)
            body = '\r\n'.join(headers) + '\r\n\r\n'
            http_handler = MockHTTPHandlerRedirect(401, body)
            opener.add_handler(auth_handler)
            opener.add_handler(http_handler)
            self._test_basic_auth(opener, auth_handler, "Authorization",
                                  realm, http_handler, password_manager,
                                  "http://acme.example.com/protected",
                                  "http://acme.example.com/protected")

    def test_basic_auth(self):
        realm = "realm2@example.com"
        realm2 = "realm2@example.com"
        basic = f'Basic realm="{realm}"'
        basic2 = f'Basic realm="{realm2}"'
        other_no_realm = 'Otherscheme xxx'
        digest = (f'Digest realm="{realm2}", '
                  f'qop="auth, auth-int", '
                  f'nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093", '
                  f'opaque="5ccc069c403ebaf9f0171e9517f40e41"')
        for realm_str in (
            # test "quote" and 'quote'
            f'Basic realm="{realm}"',
            f"Basic realm='{realm}'",

            # charset is ignored
            f'Basic realm="{realm}", charset="UTF-8"',

            # Multiple challenges per header
            f'{basic}, {basic2}',
            f'{basic}, {other_no_realm}',
            f'{other_no_realm}, {basic}',
            f'{basic}, {digest}',
            f'{digest}, {basic}',
        ):
            headers = [f'WWW-Authenticate: {realm_str}']
            self.check_basic_auth(headers, realm)

        # no quote: expect a warning
        with warnings_helper.check_warnings(("Basic Auth Realm was unquoted",
                                     UserWarning)):
            headers = [f'WWW-Authenticate: Basic realm={realm}']
            self.check_basic_auth(headers, realm)

        # Multiple headers: one challenge per header.
        # Use the first Basic realm.
        for challenges in (
            [basic,  basic2],
            [basic,  digest],
            [digest, basic],
        ):
            headers = [f'WWW-Authenticate: {challenge}'
                       for challenge in challenges]
            self.check_basic_auth(headers, realm)

    def test_proxy_basic_auth(self):
        opener = OpenerDirector()
        ph = urllib.request.ProxyHandler(dict(http="proxy.example.com:3128"))
        opener.add_handler(ph)
        password_manager = MockPasswordManager()
        auth_handler = urllib.request.ProxyBasicAuthHandler(password_manager)
        realm = "ACME Networks"
        http_handler = MockHTTPHandlerRedirect(
            407, 'Proxy-Authenticate: Basic realm="%s"\r\n\r\n' % realm)
        opener.add_handler(auth_handler)
        opener.add_handler(http_handler)
        self._test_basic_auth(opener, auth_handler, "Proxy-authorization",
                              realm, http_handler, password_manager,
                              "http://acme.example.com:3128/protected",
                              "proxy.example.com:3128",
                              )

    def test_basic_and_digest_auth_handlers(self):
        # HTTPDigestAuthHandler raised an exception if it couldn't handle a 40*
        # response (https://bugs.python.org/issue1479302), where it should instead
        # return None to allow another handler (especially
        # HTTPBasicAuthHandler) to handle the response.

        # Also (https://bugs.python.org/issue14797027, RFC 2617 section 1.2), we must
        # try digest first (since it's the strongest auth scheme), so we record
        # order of calls here to check digest comes first:
        class RecordingOpenerDirector(OpenerDirector):
            def __init__(self):
                OpenerDirector.__init__(self)
                self.recorded = []

            def record(self, info):
                self.recorded.append(info)

        class TestDigestAuthHandler(urllib.request.HTTPDigestAuthHandler):
            def http_error_401(self, *args, **kwds):
                self.parent.record("digest")
                urllib.request.HTTPDigestAuthHandler.http_error_401(self,
                                                             *args, **kwds)

        class TestBasicAuthHandler(urllib.request.HTTPBasicAuthHandler):
            def http_error_401(self, *args, **kwds):
                self.parent.record("basic")
                urllib.request.HTTPBasicAuthHandler.http_error_401(self,
                                                            *args, **kwds)

        opener = RecordingOpenerDirector()
        password_manager = MockPasswordManager()
        digest_handler = TestDigestAuthHandler(password_manager)
        basic_handler = TestBasicAuthHandler(password_manager)
        realm = "ACME Networks"
        http_handler = MockHTTPHandlerRedirect(
            401, 'WWW-Authenticate: Basic realm="%s"\r\n\r\n' % realm)
        opener.add_handler(basic_handler)
        opener.add_handler(digest_handler)
        opener.add_handler(http_handler)

        # check basic auth isn't blocked by digest handler failing
        self._test_basic_auth(opener, basic_handler, "Authorization",
                              realm, http_handler, password_manager,
                              "http://acme.example.com/protected",
                              "http://acme.example.com/protected",
                              )
        # check digest was tried before basic (twice, because
        # _test_basic_auth called .open() twice)
        self.assertEqual(opener.recorded, ["digest", "basic"]*2)

    def test_unsupported_auth_digest_handler(self):
        opener = OpenerDirector()
        # While using DigestAuthHandler
        digest_auth_handler = urllib.request.HTTPDigestAuthHandler(None)
        http_handler = MockHTTPHandlerRedirect(
            401, 'WWW-Authenticate: Kerberos\r\n\r\n')
        opener.add_handler(digest_auth_handler)
        opener.add_handler(http_handler)
        self.assertRaises(ValueError, opener.open, "http://www.example.com")

    def test_unsupported_auth_basic_handler(self):
        # While using BasicAuthHandler
        opener = OpenerDirector()
        basic_auth_handler = urllib.request.HTTPBasicAuthHandler(None)
        http_handler = MockHTTPHandlerRedirect(
            401, 'WWW-Authenticate: NTLM\r\n\r\n')
        opener.add_handler(basic_auth_handler)
        opener.add_handler(http_handler)
        self.assertRaises(ValueError, opener.open, "http://www.example.com")

    def _test_basic_auth(self, opener, auth_handler, auth_header,
                         realm, http_handler, password_manager,
                         request_url, protected_url):
        import base64
        user, password = "wile", "coyote"

        # .add_password() fed through to password manager
        auth_handler.add_password(realm, request_url, user, password)
        self.assertEqual(realm, password_manager.realm)
        self.assertEqual(request_url, password_manager.url)
        self.assertEqual(user, password_manager.user)
        self.assertEqual(password, password_manager.password)

        opener.open(request_url)

        # should have asked the password manager for the username/password
        self.assertEqual(password_manager.target_realm, realm)
        self.assertEqual(password_manager.target_url, protected_url)

        # expect one request without authorization, then one with
        self.assertEqual(len(http_handler.requests), 2)
        self.assertFalse(http_handler.requests[0].has_header(auth_header))
        userpass = bytes('%s:%s' % (user, password), "ascii")
        auth_hdr_value = ('Basic ' +
            base64.encodebytes(userpass).strip().decode())
        self.assertEqual(http_handler.requests[1].get_header(auth_header),
                         auth_hdr_value)
        self.assertEqual(http_handler.requests[1].unredirected_hdrs[auth_header],
                         auth_hdr_value)
        # if the password manager can't find a password, the handler won't
        # handle the HTTP auth error
        password_manager.user = password_manager.password = None
        http_handler.reset()
        opener.open(request_url)
        self.assertEqual(len(http_handler.requests), 1)
        self.assertFalse(http_handler.requests[0].has_header(auth_header))

    def test_basic_prior_auth_auto_send(self):
        # Assume already authenticated if is_authenticated=True
        # for APIs like Github that don't return 401

        user, password = "wile", "coyote"
        request_url = "http://acme.example.com/protected"

        http_handler = MockHTTPHandlerCheckAuth(200)

        pwd_manager = HTTPPasswordMgrWithPriorAuth()
        auth_prior_handler = HTTPBasicAuthHandler(pwd_manager)
        auth_prior_handler.add_password(
            None, request_url, user, password, is_authenticated=True)

        self.assertTrue(pwd_manager.is_authenticated(request_url))
        self.assertTrue(pwd_manager.is_authenticated(request_url + '/nested'))
        self.assertFalse(pwd_manager.is_authenticated(request_url + 'plain'))

        opener = OpenerDirector()
        opener.add_handler(auth_prior_handler)
        opener.add_handler(http_handler)

        opener.open(request_url)

        # expect request to be sent with auth header
        self.assertTrue(http_handler.has_auth_header)

    def test_basic_prior_auth_send_after_first_success(self):
        # Auto send auth header after authentication is successful once

        user, password = 'wile', 'coyote'
        request_url = 'http://acme.example.com/protected'
        realm = 'ACME'

        pwd_manager = HTTPPasswordMgrWithPriorAuth()
        auth_prior_handler = HTTPBasicAuthHandler(pwd_manager)
        auth_prior_handler.add_password(realm, request_url, user, password)

        is_auth = pwd_manager.is_authenticated(request_url)
        self.assertFalse(is_auth)

        opener = OpenerDirector()
        opener.add_handler(auth_prior_handler)

        http_handler = MockHTTPHandlerRedirect(
            401, 'WWW-Authenticate: Basic realm="%s"\r\n\r\n' % None)
        opener.add_handler(http_handler)

        opener.open(request_url)

        is_auth = pwd_manager.is_authenticated(request_url)
        self.assertTrue(is_auth)

        http_handler = MockHTTPHandlerCheckAuth(200)
        self.assertFalse(http_handler.has_auth_header)

        opener = OpenerDirector()
        opener.add_handler(auth_prior_handler)
        opener.add_handler(http_handler)

        # After getting 200 from MockHTTPHandler
        # Next request sends header in the first request
        opener.open(request_url)

        # expect request to be sent with auth header
        self.assertTrue(http_handler.has_auth_header)

    def test_http_closed(self):
        """Test the connection is cleaned up when the response is closed"""
        for (transfer, data) in (
            ("Connection: close", b"data"),
            ("Transfer-Encoding: chunked", b"4\r\ndata\r\n0\r\n\r\n"),
            ("Content-Length: 4", b"data"),
        ):
            header = "HTTP/1.1 200 OK\r\n{}\r\n\r\n".format(transfer)
            conn = test_urllib.fakehttp(header.encode() + data)
            handler = urllib.request.AbstractHTTPHandler()
            req = Request("http://dummy/")
            req.timeout = None
            with handler.do_open(conn, req) as resp:
                resp.read()
            self.assertTrue(conn.fakesock.closed,
                "Connection not closed with {!r}".format(transfer))

    def test_invalid_closed(self):
        """Test the connection is cleaned up after an invalid response"""
        conn = test_urllib.fakehttp(b"")
        handler = urllib.request.AbstractHTTPHandler()
        req = Request("http://dummy/")
        req.timeout = None
        with self.assertRaises(http.client.BadStatusLine):
            handler.do_open(conn, req)
        self.assertTrue(conn.fakesock.closed, "Connection not closed")


class MiscTests(unittest.TestCase):

    def opener_has_handler(self, opener, handler_class):
        self.assertTrue(any(h.__class__ == handler_class
                            for h in opener.handlers))

    def test_build_opener(self):
        class MyHTTPHandler(urllib.request.HTTPHandler):
            pass

        class FooHandler(urllib.request.BaseHandler):
            def foo_open(self):
                pass

        class BarHandler(urllib.request.BaseHandler):
            def bar_open(self):
                pass

        build_opener = urllib.request.build_opener

        o = build_opener(FooHandler, BarHandler)
        self.opener_has_handler(o, FooHandler)
        self.opener_has_handler(o, BarHandler)

        # can take a mix of classes and instances
        o = build_opener(FooHandler, BarHandler())
        self.opener_has_handler(o, FooHandler)
        self.opener_has_handler(o, BarHandler)

        # subclasses of default handlers override default handlers
        o = build_opener(MyHTTPHandler)
        self.opener_has_handler(o, MyHTTPHandler)

        # a particular case of overriding: default handlers can be passed
        # in explicitly
        o = build_opener()
        self.opener_has_handler(o, urllib.request.HTTPHandler)
        o = build_opener(urllib.request.HTTPHandler)
        self.opener_has_handler(o, urllib.request.HTTPHandler)
        o = build_opener(urllib.request.HTTPHandler())
        self.opener_has_handler(o, urllib.request.HTTPHandler)

        # Issue2670: multiple handlers sharing the same base class
        class MyOtherHTTPHandler(urllib.request.HTTPHandler):
            pass

        o = build_opener(MyHTTPHandler, MyOtherHTTPHandler)
        self.opener_has_handler(o, MyHTTPHandler)
        self.opener_has_handler(o, MyOtherHTTPHandler)

    def test_HTTPError_interface(self):
        """
        Issue 13211 reveals that HTTPError didn't implement the URLError
        interface even though HTTPError is a subclass of URLError.
        """
        msg = 'something bad happened'
        url = code = fp = None
        hdrs = 'Content-Length: 42'
        err = urllib.error.HTTPError(url, code, msg, hdrs, fp)
        self.assertTrue(hasattr(err, 'reason'))
        self.assertEqual(err.reason, 'something bad happened')
        self.assertTrue(hasattr(err, 'headers'))
        self.assertEqual(err.headers, 'Content-Length: 42')
        expected_errmsg = 'HTTP Error %s: %s' % (err.code, err.msg)
        self.assertEqual(str(err), expected_errmsg)
        expected_errmsg = '<HTTPError %s: %r>' % (err.code, err.msg)
        self.assertEqual(repr(err), expected_errmsg)
        err.close()

    def test_gh_98778(self):
        x = urllib.error.HTTPError("url", 405, "METHOD NOT ALLOWED", None, None)
        self.assertEqual(getattr(x, "__notes__", ()), ())
        self.assertIsInstance(x.fp.read(), bytes)
        x.close()

    def test_parse_proxy(self):
        parse_proxy_test_cases = [
            ('proxy.example.com',
             (None, None, None, 'proxy.example.com')),
            ('proxy.example.com:3128',
             (None, None, None, 'proxy.example.com:3128')),
            ('proxy.example.com', (None, None, None, 'proxy.example.com')),
            ('proxy.example.com:3128',
             (None, None, None, 'proxy.example.com:3128')),
            # The authority component may optionally include userinfo
            # (assumed to be # username:password):
            ('joe:password@proxy.example.com',
             (None, 'joe', 'password', 'proxy.example.com')),
            ('joe:password@proxy.example.com:3128',
             (None, 'joe', 'password', 'proxy.example.com:3128')),
            #Examples with URLS
            ('http://proxy.example.com/',
             ('http', None, None, 'proxy.example.com')),
            ('http://proxy.example.com:3128/',
             ('http', None, None, 'proxy.example.com:3128')),
            ('http://joe:password@proxy.example.com/',
             ('http', 'joe', 'password', 'proxy.example.com')),
            ('http://joe:password@proxy.example.com:3128',
             ('http', 'joe', 'password', 'proxy.example.com:3128')),
            # Everything after the authority is ignored
            ('ftp://joe:password@proxy.example.com/rubbish:3128',
             ('ftp', 'joe', 'password', 'proxy.example.com')),
            # Test for no trailing '/' case
            ('http://joe:password@proxy.example.com',
             ('http', 'joe', 'password', 'proxy.example.com')),
            # Testcases with '/' character in username, password
            ('http://user/name:password@localhost:22',
             ('http', 'user/name', 'password', 'localhost:22')),
            ('http://username:pass/word@localhost:22',
             ('http', 'username', 'pass/word', 'localhost:22')),
            ('http://user/name:pass/word@localhost:22',
             ('http', 'user/name', 'pass/word', 'localhost:22')),
        ]


        for tc, expected in parse_proxy_test_cases:
            self.assertEqual(_parse_proxy(tc), expected)

        self.assertRaises(ValueError, _parse_proxy, 'file:/ftp.example.com'),

    def test_unsupported_algorithm(self):
        handler = AbstractDigestAuthHandler()
        with self.assertRaises(ValueError) as exc:
            handler.get_algorithm_impls('invalid')
        self.assertEqual(
            str(exc.exception),
            "Unsupported digest authentication algorithm 'invalid'"
        )


class RequestTests(unittest.TestCase):
    class PutRequest(Request):
        method = 'PUT'

    def setUp(self):
        self.get = Request("http://www.python.org/~jeremy/")
        self.post = Request("http://www.python.org/~jeremy/",
                            "data",
                            headers={"X-Test": "test"})
        self.head = Request("http://www.python.org/~jeremy/", method='HEAD')
        self.put = self.PutRequest("http://www.python.org/~jeremy/")
        self.force_post = self.PutRequest("http://www.python.org/~jeremy/",
            method="POST")

    def test_method(self):
        self.assertEqual("POST", self.post.get_method())
        self.assertEqual("GET", self.get.get_method())
        self.assertEqual("HEAD", self.head.get_method())
        self.assertEqual("PUT", self.put.get_method())
        self.assertEqual("POST", self.force_post.get_method())

    def test_data(self):
        self.assertFalse(self.get.data)
        self.assertEqual("GET", self.get.get_method())
        self.get.data = "spam"
        self.assertTrue(self.get.data)
        self.assertEqual("POST", self.get.get_method())

    # issue 16464
    # if we change data we need to remove content-length header
    # (cause it's most probably calculated for previous value)
    def test_setting_data_should_remove_content_length(self):
        self.assertNotIn("Content-length", self.get.unredirected_hdrs)
        self.get.add_unredirected_header("Content-length", 42)
        self.assertEqual(42, self.get.unredirected_hdrs["Content-length"])
        self.get.data = "spam"
        self.assertNotIn("Content-length", self.get.unredirected_hdrs)

    # issue 17485 same for deleting data.
    def test_deleting_data_should_remove_content_length(self):
        self.assertNotIn("Content-length", self.get.unredirected_hdrs)
        self.get.data = 'foo'
        self.get.add_unredirected_header("Content-length", 3)
        self.assertEqual(3, self.get.unredirected_hdrs["Content-length"])
        del self.get.data
        self.assertNotIn("Content-length", self.get.unredirected_hdrs)

    def test_get_full_url(self):
        self.assertEqual("http://www.python.org/~jeremy/",
                         self.get.get_full_url())

    def test_selector(self):
        self.assertEqual("/~jeremy/", self.get.selector)
        req = Request("http://www.python.org/")
        self.assertEqual("/", req.selector)

    def test_get_type(self):
        self.assertEqual("http", self.get.type)

    def test_get_host(self):
        self.assertEqual("www.python.org", self.get.host)

    def test_get_host_unquote(self):
        req = Request("http://www.%70ython.org/")
        self.assertEqual("www.python.org", req.host)

    def test_proxy(self):
        self.assertFalse(self.get.has_proxy())
        self.get.set_proxy("www.perl.org", "http")
        self.assertTrue(self.get.has_proxy())
        self.assertEqual("www.python.org", self.get.origin_req_host)
        self.assertEqual("www.perl.org", self.get.host)

    def test_wrapped_url(self):
        req = Request("<URL:http://www.python.org>")
        self.assertEqual("www.python.org", req.host)

    def test_url_fragment(self):
        req = Request("http://www.python.org/?qs=query#fragment=true")
        self.assertEqual("/?qs=query", req.selector)
        req = Request("http://www.python.org/#fun=true")
        self.assertEqual("/", req.selector)

        # Issue 11703: geturl() omits fragment in the original URL.
        url = 'http://docs.python.org/library/urllib2.html#OK'
        req = Request(url)
        self.assertEqual(req.get_full_url(), url)

    def test_url_fullurl_get_full_url(self):
        urls = ['http://docs.python.org',
                'http://docs.python.org/library/urllib2.html#OK',
                'http://www.python.org/?qs=query#fragment=true']
        for url in urls:
            req = Request(url)
            self.assertEqual(req.get_full_url(), req.full_url)


if __name__ == "__main__":
    unittest.main()
