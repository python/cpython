import unittest
from test import test_support

import os
import StringIO

import urllib2
from urllib2 import Request, OpenerDirector

# XXX
# Request
# CacheFTPHandler (hard to write)
# parse_keqv_list, parse_http_list (I'm leaving this for Anthony Baxter
#  and Greg Stein, since they're doing Digest Authentication)
# Authentication stuff (ditto)
# ProxyHandler, CustomProxy, CustomProxyHandler (I don't use a proxy)
# GopherHandler (haven't used gopher for a decade or so...)

class TrivialTests(unittest.TestCase):
    def test_trivial(self):
        # A couple trivial tests

        self.assertRaises(ValueError, urllib2.urlopen, 'bogus url')

        # XXX Name hacking to get this to work on Windows.
        fname = os.path.abspath(urllib2.__file__).replace('\\', '/')
        if fname[1:2] == ":":
            fname = fname[2:]
        # And more hacking to get it to work on MacOS. This assumes
        # urllib.pathname2url works, unfortunately...
        if os.name == 'mac':
            fname = '/' + fname.replace(':', '/')
        elif os.name == 'riscos':
            import string
            fname = os.expand(fname)
            fname = fname.translate(string.maketrans("/.", "./"))

        file_url = "file://%s" % fname
        f = urllib2.urlopen(file_url)

        buf = f.read()
        f.close()


class MockOpener:
    addheaders = []
    def open(self, req, data=None):
        self.req, self.data = req, data
    def error(self, proto, *args):
        self.proto, self.args = proto, args

class MockFile:
    def read(self, count=None): pass
    def readline(self, count=None): pass
    def close(self): pass

class MockResponse(StringIO.StringIO):
    def __init__(self, code, msg, headers, data, url=None):
        StringIO.StringIO.__init__(self, data)
        self.code, self.msg, self.headers, self.url = code, msg, headers, url
    def info(self):
        return self.headers
    def geturl(self):
        return self.url

class FakeMethod:
    def __init__(self, meth_name, action, handle):
        self.meth_name = meth_name
        self.handle = handle
        self.action = action
    def __call__(self, *args):
        return self.handle(self.meth_name, self.action, *args)

class MockHandler:
    def __init__(self, methods):
        self._define_methods(methods)
    def _define_methods(self, methods):
        for spec in methods:
            if len(spec) == 2: name, action = spec
            else: name, action = spec, None
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
            raise urllib2.URLError("blah")
        assert False
    def close(self): pass
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
        class MockHandlerSubclass(MockHandler): pass
        h = MockHandlerSubclass(meths)
        h.handler_order = count
        h.add_parent(opener)
        count = count + 1
        handlers.append(h)
        opener.add_handler(h)
    return handlers

class OpenerDirectorTests(unittest.TestCase):

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
        for meths, handler_order in [
            ([("http_open", "return self")], 500),
            (["http_open"], 0),
            ]:
            class MockHandlerSubclass(MockHandler): pass
            h = MockHandlerSubclass(meths)
            h.handler_order = handler_order
            handlers.append(h)
            o.add_handler(h)

        r = o.open("http://example.com/")
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
        self.assertRaises(urllib2.URLError, o.open, req)
        self.assertEqual(o.calls, [(handlers[0], "http_open", (req,), {})])

##     def test_error(self):
##         # XXX this doesn't actually seem to be used in standard library,
##         #  but should really be tested anyway...

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

        class Unknown:
            def __eq__(self, other): return True

        req = Request("http://example.com/")
        r = o.open(req)
        assert len(o.calls) == 2
        calls = [(handlers[0], "http_open", (req,)),
                 (handlers[2], "http_error_302",
                  (req, Unknown(), 302, "", {}))]
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
        r = o.open(req)
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
                self.assert_(isinstance(args[0], Request))
            else:
                # *_response
                self.assertEqual((handler, name), calls[i])
                self.assertEqual(len(args), 2)
                self.assert_(isinstance(args[0], Request))
                # response from opener.open is None, because there's no
                # handler that defines http_open to handle it
                self.assert_(args[1] is None or
                             isinstance(args[1], MockResponse))


class HandlerTests(unittest.TestCase):

    def test_ftp(self):
        class MockFTPWrapper:
            def __init__(self, data): self.data = data
            def retrfile(self, filename, filetype):
                self.filename, self.filetype = filename, filetype
                return StringIO.StringIO(self.data), len(self.data)

        class NullFTPHandler(urllib2.FTPHandler):
            def __init__(self, data): self.data = data
            def connect_ftp(self, user, passwd, host, port, dirs):
                self.user, self.passwd = user, passwd
                self.host, self.port = host, port
                self.dirs = dirs
                self.ftpwrapper = MockFTPWrapper(self.data)
                return self.ftpwrapper

        import ftplib, socket
        data = "rheum rhaponicum"
        h = NullFTPHandler(data)
        o = h.parent = MockOpener()

        for url, host, port, type_, dirs, filename, mimetype in [
            ("ftp://localhost/foo/bar/baz.html",
             "localhost", ftplib.FTP_PORT, "I",
             ["foo", "bar"], "baz.html", "text/html"),
# XXXX Bug: FTPHandler tries to gethostbyname "localhost:80", with the
#  port still there.
##             ("ftp://localhost:80/foo/bar/",
##              "localhost", 80, "D",
##              ["foo", "bar"], "", None),
# XXXX bug: second use of splitattr() in FTPHandler should be splitvalue()
##             ("ftp://localhost/baz.gif;type=a",
##              "localhost", ftplib.FTP_PORT, "A",
##              [], "baz.gif", "image/gif"),
            ]:
            r = h.ftp_open(Request(url))
            # ftp authentication not yet implemented by FTPHandler
            self.assert_(h.user == h.passwd == "")
            self.assertEqual(h.host, socket.gethostbyname(host))
            self.assertEqual(h.port, port)
            self.assertEqual(h.dirs, dirs)
            self.assertEqual(h.ftpwrapper.filename, filename)
            self.assertEqual(h.ftpwrapper.filetype, type_)
            headers = r.info()
            self.assertEqual(headers["Content-type"], mimetype)
            self.assertEqual(int(headers["Content-length"]), len(data))

    def test_file(self):
        import time, rfc822, socket
        h = urllib2.FileHandler()
        o = h.parent = MockOpener()

        #from test_support import TESTFN
        TESTFN = "test.txt"
        towrite = "hello, world\n"
        for url in [
            "file://localhost%s/%s" % (os.getcwd(), TESTFN),
            "file://%s/%s" % (os.getcwd(), TESTFN),
            "file://%s%s/%s" % (socket.gethostbyname('localhost'),
                                os.getcwd(), TESTFN),
            "file://%s%s/%s" % (socket.gethostbyname(socket.gethostname()),
                                os.getcwd(), TESTFN),
            # XXX Windows / Mac format(s), ... ?
            ]:
            f = open(TESTFN, "w")
            try:
                try:
                    f.write(towrite)
                finally:
                    f.close()

                r = h.file_open(Request(url))
                try:
                    data = r.read()
                    read_time = time.time()
                    headers = r.info()
                    newurl = r.geturl()
                finally:
                    r.close()
            finally:
                os.remove(TESTFN)
            self.assertEqual(data, towrite)
            self.assertEqual(headers["Content-type"], "text/plain")
            self.assertEqual(headers["Content-length"], "13")
            # Fudge Last-modified string comparison by one second to
            # prevent spurious failure on crossing a second boundary while
            # executing this test.
            unfudged = rfc822.formatdate(read_time)
            fudged = rfc822.formatdate(read_time-1)
            self.assert_(headers["Last-modified"] in [unfudged, fudged])

        for url in [
            "file://localhost:80%s/%s" % (os.getcwd(), TESTFN),
# XXXX bug: these fail with socket.gaierror, should be URLError
##             "file://%s:80%s/%s" % (socket.gethostbyname('localhost'),
##                                    os.getcwd(), TESTFN),
##             "file://somerandomhost.ontheinternet.com%s/%s" %
##             (os.getcwd(), TESTFN),
            ]:
            try:
                f = open(TESTFN, "w")
                try:
                    f.write(towrite)
                finally:
                    f.close()

                self.assertRaises(urllib2.URLError,
                                  h.file_open, Request(url))
            finally:
                os.remove(TESTFN)

        h = urllib2.FileHandler()
        o = h.parent = MockOpener()
        # XXXX why does // mean ftp (and /// mean not ftp!), and where
        #  is file: scheme specified?  I think this is really a bug, and
        #  what was intended was to distinguish between URLs like:
        # file:/blah.txt (a file)
        # file://localhost/blah.txt (a file)
        # file:///blah.txt (a file)
        # file://ftp.example.com/blah.txt (an ftp URL)
        for url, ftp in [
            ("file://ftp.example.com//foo.txt", True),
            ("file://ftp.example.com///foo.txt", False),
# XXXX bug: fails with OSError, should be URLError
            ("file://ftp.example.com/foo.txt", False),
            ]:
            req = Request(url)
            try:
                h.file_open(req)
            # XXXX remove OSError when bug fixed
            except (urllib2.URLError, OSError):
                self.assert_(not ftp)
            else:
                self.assert_(o.req is req)
                self.assertEqual(req.type, "ftp")

    def test_http(self):
        class MockHTTPResponse:
            def __init__(self, fp, msg, status, reason):
                self.fp = fp
                self.msg = msg
                self.status = status
                self.reason = reason
        class MockHTTPClass:
            def __init__(self):
                self.req_headers = []
                self.data = None
                self.raise_on_endheaders = False
            def __call__(self, host):
                self.host = host
                return self
            def set_debuglevel(self, level):
                self.level = level
            def request(self, method, url, body=None, headers={}):
                self.method = method
                self.selector = url
                self.req_headers += headers.items()
                if body:
                    self.data = body
                if self.raise_on_endheaders:
                    import socket
                    raise socket.error()
            def getresponse(self):
                return MockHTTPResponse(MockFile(), {}, 200, "OK")

        h = urllib2.AbstractHTTPHandler()
        o = h.parent = MockOpener()

        url = "http://example.com/"
        for method, data in [("GET", None), ("POST", "blah")]:
            req = Request(url, data, {"Foo": "bar"})
            req.add_unredirected_header("Spam", "eggs")
            http = MockHTTPClass()
            r = h.do_open(http, req)

            # result attributes
            r.read; r.readline  # wrapped MockFile methods
            r.info; r.geturl  # addinfourl methods
            r.code, r.msg == 200, "OK"  # added from MockHTTPClass.getreply()
            hdrs = r.info()
            hdrs.get; hdrs.has_key  # r.info() gives dict from .getreply()
            self.assertEqual(r.geturl(), url)

            self.assertEqual(http.host, "example.com")
            self.assertEqual(http.level, 0)
            self.assertEqual(http.method, method)
            self.assertEqual(http.selector, "/")
            self.assertEqual(http.req_headers,
                             [("Foo", "bar"), ("Spam", "eggs")])
            self.assertEqual(http.data, data)

        # check socket.error converted to URLError
        http.raise_on_endheaders = True
        self.assertRaises(urllib2.URLError, h.do_open, http, req)

        # check adding of standard headers
        o.addheaders = [("Spam", "eggs")]
        for data in "", None:  # POST, GET
            req = Request("http://example.com/", data)
            r = MockResponse(200, "OK", {}, "")
            newreq = h.do_request(req)
            if data is None:  # GET
                self.assert_("Content-length" not in req.unredirected_hdrs)
                self.assert_("Content-type" not in req.unredirected_hdrs)
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
            newreq = h.do_request(req)
            self.assertEqual(req.unredirected_hdrs["Content-length"], "foo")
            self.assertEqual(req.unredirected_hdrs["Content-type"], "bar")
            self.assertEqual(req.unredirected_hdrs["Host"], "baz")
            self.assertEqual(req.unredirected_hdrs["Spam"], "foo")

    def test_errors(self):
        h = urllib2.HTTPErrorProcessor()
        o = h.parent = MockOpener()

        url = "http://example.com/"
        req = Request(url)
        # 200 OK is passed through
        r = MockResponse(200, "OK", {}, "", url)
        newr = h.http_response(req, r)
        self.assert_(r is newr)
        self.assert_(not hasattr(o, "proto"))  # o.error not called
        # anything else calls o.error (and MockOpener returns None, here)
        r = MockResponse(201, "Created", {}, "", url)
        self.assert_(h.http_response(req, r) is None)
        self.assertEqual(o.proto, "http")  # o.error called
        self.assertEqual(o.args, (req, r, 201, "Created", {}))

    def test_redirect(self):
        from_url = "http://example.com/a.html"
        to_url = "http://example.com/b.html"
        h = urllib2.HTTPRedirectHandler()
        o = h.parent = MockOpener()

        # ordinary redirect behaviour
        for code in 301, 302, 303, 307:
            for data in None, "blah\nblah\n":
                method = getattr(h, "http_error_%s" % code)
                req = Request(from_url, data)
                req.add_header("Nonsense", "viking=withhold")
                req.add_unredirected_header("Spam", "spam")
                try:
                    method(req, MockFile(), code, "Blah", {"location": to_url})
                except urllib2.HTTPError:
                    # 307 in response to POST requires user OK
                    self.assert_(code == 307 and data is not None)
                self.assertEqual(o.req.get_full_url(), to_url)
                try:
                    self.assertEqual(o.req.get_method(), "GET")
                except AttributeError:
                    self.assert_(not o.req.has_data())
                self.assertEqual(o.req.headers["Nonsense"],
                                 "viking=withhold")
                self.assert_("Spam" not in o.req.headers)
                self.assert_("Spam" not in o.req.unredirected_hdrs)

        # loop detection
        req = Request(from_url)
        req.origin_req_host = "example.com"
        def redirect(h, req, code, url=to_url):
            method = getattr(h, "http_error_%s" % code)
            method(req, MockFile(), code, "Blah", {"location": url})
        # Note that the *original* request shares the same record of
        # redirections with the sub-requests caused by the redirections.
        # once
        redirect(h, req, 302)
        # twice: loop detected
        self.assertRaises(urllib2.HTTPError, redirect, h, req, 302)
        # and again
        self.assertRaises(urllib2.HTTPError, redirect, h, req, 302)
        # but this is a different redirect code, so OK...
        redirect(h, req, 301)
        self.assertRaises(urllib2.HTTPError, redirect, h, req, 301)
        # order doesn't matter
        redirect(h, req, 303)
        redirect(h, req, 307)
        self.assertRaises(urllib2.HTTPError, redirect, h, req, 303)

        # detect endless non-repeating chain of redirects
        req = Request(from_url)
        req.origin_req_host = "example.com"
        count = 0
        try:
            while 1:
                redirect(h, req, 302, "http://example.com/%d" % count)
                count = count + 1
        except urllib2.HTTPError:
            self.assertEqual(count,
                             urllib2.HTTPRedirectHandler.max_redirections)


class MiscTests(unittest.TestCase):

    def test_build_opener(self):
        class MyHTTPHandler(urllib2.HTTPHandler): pass
        class FooHandler(urllib2.BaseHandler):
            def foo_open(self): pass
        class BarHandler(urllib2.BaseHandler):
            def bar_open(self): pass

        build_opener = urllib2.build_opener

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
        self.opener_has_handler(o, urllib2.HTTPHandler)
        o = build_opener(urllib2.HTTPHandler)
        self.opener_has_handler(o, urllib2.HTTPHandler)
        o = build_opener(urllib2.HTTPHandler())
        self.opener_has_handler(o, urllib2.HTTPHandler)

    def opener_has_handler(self, opener, handler_class):
        for h in opener.handlers:
            if h.__class__ == handler_class:
                break
        else:
            self.assert_(False)


def test_main(verbose=None):
    from test import test_sets
    test_support.run_unittest(
        TrivialTests,
        OpenerDirectorTests,
        HandlerTests,
        MiscTests,
    )

if __name__ == "__main__":
    test_main(verbose=True)
