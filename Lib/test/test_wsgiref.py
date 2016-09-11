from unittest import mock
from test import support
from test.test_httpservers import NoLogRequestHandler
from unittest import TestCase
from wsgiref.util import setup_testing_defaults
from wsgiref.headers import Headers
from wsgiref.handlers import BaseHandler, BaseCGIHandler, SimpleHandler
from wsgiref import util
from wsgiref.validate import validator
from wsgiref.simple_server import WSGIServer, WSGIRequestHandler
from wsgiref.simple_server import make_server
from http.client import HTTPConnection
from io import StringIO, BytesIO, BufferedReader
from socketserver import BaseServer
from platform import python_implementation

import os
import re
import signal
import sys
import unittest


class MockServer(WSGIServer):
    """Non-socket HTTP server"""

    def __init__(self, server_address, RequestHandlerClass):
        BaseServer.__init__(self, server_address, RequestHandlerClass)
        self.server_bind()

    def server_bind(self):
        host, port = self.server_address
        self.server_name = host
        self.server_port = port
        self.setup_environ()


class MockHandler(WSGIRequestHandler):
    """Non-socket HTTP handler"""
    def setup(self):
        self.connection = self.request
        self.rfile, self.wfile = self.connection

    def finish(self):
        pass


def hello_app(environ,start_response):
    start_response("200 OK", [
        ('Content-Type','text/plain'),
        ('Date','Mon, 05 Jun 2006 18:49:54 GMT')
    ])
    return [b"Hello, world!"]


def header_app(environ, start_response):
    start_response("200 OK", [
        ('Content-Type', 'text/plain'),
        ('Date', 'Mon, 05 Jun 2006 18:49:54 GMT')
    ])
    return [';'.join([
        environ['HTTP_X_TEST_HEADER'], environ['QUERY_STRING'],
        environ['PATH_INFO']
    ]).encode('iso-8859-1')]


def run_amock(app=hello_app, data=b"GET / HTTP/1.0\n\n"):
    server = make_server("", 80, app, MockServer, MockHandler)
    inp = BufferedReader(BytesIO(data))
    out = BytesIO()
    olderr = sys.stderr
    err = sys.stderr = StringIO()

    try:
        server.finish_request((inp, out), ("127.0.0.1",8888))
    finally:
        sys.stderr = olderr

    return out.getvalue(), err.getvalue()

def compare_generic_iter(make_it,match):
    """Utility to compare a generic 2.1/2.2+ iterator with an iterable

    If running under Python 2.2+, this tests the iterator using iter()/next(),
    as well as __getitem__.  'make_it' must be a function returning a fresh
    iterator to be tested (since this may test the iterator twice)."""

    it = make_it()
    n = 0
    for item in match:
        if not it[n]==item: raise AssertionError
        n+=1
    try:
        it[n]
    except IndexError:
        pass
    else:
        raise AssertionError("Too many items from __getitem__",it)

    try:
        iter, StopIteration
    except NameError:
        pass
    else:
        # Only test iter mode under 2.2+
        it = make_it()
        if not iter(it) is it: raise AssertionError
        for item in match:
            if not next(it) == item: raise AssertionError
        try:
            next(it)
        except StopIteration:
            pass
        else:
            raise AssertionError("Too many items from .__next__()", it)


class IntegrationTests(TestCase):

    def check_hello(self, out, has_length=True):
        pyver = (python_implementation() + "/" +
                sys.version.split()[0])
        self.assertEqual(out,
            ("HTTP/1.0 200 OK\r\n"
            "Server: WSGIServer/0.2 " + pyver +"\r\n"
            "Content-Type: text/plain\r\n"
            "Date: Mon, 05 Jun 2006 18:49:54 GMT\r\n" +
            (has_length and  "Content-Length: 13\r\n" or "") +
            "\r\n"
            "Hello, world!").encode("iso-8859-1")
        )

    def test_plain_hello(self):
        out, err = run_amock()
        self.check_hello(out)

    def test_environ(self):
        request = (
            b"GET /p%61th/?query=test HTTP/1.0\n"
            b"X-Test-Header: Python test \n"
            b"X-Test-Header: Python test 2\n"
            b"Content-Length: 0\n\n"
        )
        out, err = run_amock(header_app, request)
        self.assertEqual(
            out.splitlines()[-1],
            b"Python test,Python test 2;query=test;/path/"
        )

    def test_request_length(self):
        out, err = run_amock(data=b"GET " + (b"x" * 65537) + b" HTTP/1.0\n\n")
        self.assertEqual(out.splitlines()[0],
                         b"HTTP/1.0 414 Request-URI Too Long")

    def test_validated_hello(self):
        out, err = run_amock(validator(hello_app))
        # the middleware doesn't support len(), so content-length isn't there
        self.check_hello(out, has_length=False)

    def test_simple_validation_error(self):
        def bad_app(environ,start_response):
            start_response("200 OK", ('Content-Type','text/plain'))
            return ["Hello, world!"]
        out, err = run_amock(validator(bad_app))
        self.assertTrue(out.endswith(
            b"A server error occurred.  Please contact the administrator."
        ))
        self.assertEqual(
            err.splitlines()[-2],
            "AssertionError: Headers (('Content-Type', 'text/plain')) must"
            " be of type list: <class 'tuple'>"
        )

    def test_status_validation_errors(self):
        def create_bad_app(status):
            def bad_app(environ, start_response):
                start_response(status, [("Content-Type", "text/plain; charset=utf-8")])
                return [b"Hello, world!"]
            return bad_app

        tests = [
            ('200', 'AssertionError: Status must be at least 4 characters'),
            ('20X OK', 'AssertionError: Status message must begin w/3-digit code'),
            ('200OK', 'AssertionError: Status message must have a space after code'),
        ]

        for status, exc_message in tests:
            with self.subTest(status=status):
                out, err = run_amock(create_bad_app(status))
                self.assertTrue(out.endswith(
                    b"A server error occurred.  Please contact the administrator."
                ))
                self.assertEqual(err.splitlines()[-2], exc_message)

    def test_wsgi_input(self):
        def bad_app(e,s):
            e["wsgi.input"].read()
            s("200 OK", [("Content-Type", "text/plain; charset=utf-8")])
            return [b"data"]
        out, err = run_amock(validator(bad_app))
        self.assertTrue(out.endswith(
            b"A server error occurred.  Please contact the administrator."
        ))
        self.assertEqual(
            err.splitlines()[-2], "AssertionError"
        )

    def test_bytes_validation(self):
        def app(e, s):
            s("200 OK", [
                ("Content-Type", "text/plain; charset=utf-8"),
                ("Date", "Wed, 24 Dec 2008 13:29:32 GMT"),
                ])
            return [b"data"]
        out, err = run_amock(validator(app))
        self.assertTrue(err.endswith('"GET / HTTP/1.0" 200 4\n'))
        ver = sys.version.split()[0].encode('ascii')
        py  = python_implementation().encode('ascii')
        pyver = py + b"/" + ver
        self.assertEqual(
                b"HTTP/1.0 200 OK\r\n"
                b"Server: WSGIServer/0.2 "+ pyver + b"\r\n"
                b"Content-Type: text/plain; charset=utf-8\r\n"
                b"Date: Wed, 24 Dec 2008 13:29:32 GMT\r\n"
                b"\r\n"
                b"data",
                out)

    def test_cp1252_url(self):
        def app(e, s):
            s("200 OK", [
                ("Content-Type", "text/plain"),
                ("Date", "Wed, 24 Dec 2008 13:29:32 GMT"),
                ])
            # PEP3333 says environ variables are decoded as latin1.
            # Encode as latin1 to get original bytes
            return [e["PATH_INFO"].encode("latin1")]

        out, err = run_amock(
            validator(app), data=b"GET /\x80%80 HTTP/1.0")
        self.assertEqual(
            [
                b"HTTP/1.0 200 OK",
                mock.ANY,
                b"Content-Type: text/plain",
                b"Date: Wed, 24 Dec 2008 13:29:32 GMT",
                b"",
                b"/\x80\x80",
            ],
            out.splitlines())

    def test_interrupted_write(self):
        # BaseHandler._write() and _flush() have to write all data, even if
        # it takes multiple send() calls.  Test this by interrupting a send()
        # call with a Unix signal.
        threading = support.import_module("threading")
        pthread_kill = support.get_attribute(signal, "pthread_kill")

        def app(environ, start_response):
            start_response("200 OK", [])
            return [b'\0' * support.SOCK_MAX_SIZE]

        class WsgiHandler(NoLogRequestHandler, WSGIRequestHandler):
            pass

        server = make_server(support.HOST, 0, app, handler_class=WsgiHandler)
        self.addCleanup(server.server_close)
        interrupted = threading.Event()

        def signal_handler(signum, frame):
            interrupted.set()

        original = signal.signal(signal.SIGUSR1, signal_handler)
        self.addCleanup(signal.signal, signal.SIGUSR1, original)
        received = None
        main_thread = threading.get_ident()

        def run_client():
            http = HTTPConnection(*server.server_address)
            http.request("GET", "/")
            with http.getresponse() as response:
                response.read(100)
                # The main thread should now be blocking in a send() system
                # call.  But in theory, it could get interrupted by other
                # signals, and then retried.  So keep sending the signal in a
                # loop, in case an earlier signal happens to be delivered at
                # an inconvenient moment.
                while True:
                    pthread_kill(main_thread, signal.SIGUSR1)
                    if interrupted.wait(timeout=float(1)):
                        break
                nonlocal received
                received = len(response.read())
            http.close()

        background = threading.Thread(target=run_client)
        background.start()
        server.handle_request()
        background.join()
        self.assertEqual(received, support.SOCK_MAX_SIZE - 100)


class UtilityTests(TestCase):

    def checkShift(self,sn_in,pi_in,part,sn_out,pi_out):
        env = {'SCRIPT_NAME':sn_in,'PATH_INFO':pi_in}
        util.setup_testing_defaults(env)
        self.assertEqual(util.shift_path_info(env),part)
        self.assertEqual(env['PATH_INFO'],pi_out)
        self.assertEqual(env['SCRIPT_NAME'],sn_out)
        return env

    def checkDefault(self, key, value, alt=None):
        # Check defaulting when empty
        env = {}
        util.setup_testing_defaults(env)
        if isinstance(value, StringIO):
            self.assertIsInstance(env[key], StringIO)
        elif isinstance(value,BytesIO):
            self.assertIsInstance(env[key],BytesIO)
        else:
            self.assertEqual(env[key], value)

        # Check existing value
        env = {key:alt}
        util.setup_testing_defaults(env)
        self.assertIs(env[key], alt)

    def checkCrossDefault(self,key,value,**kw):
        util.setup_testing_defaults(kw)
        self.assertEqual(kw[key],value)

    def checkAppURI(self,uri,**kw):
        util.setup_testing_defaults(kw)
        self.assertEqual(util.application_uri(kw),uri)

    def checkReqURI(self,uri,query=1,**kw):
        util.setup_testing_defaults(kw)
        self.assertEqual(util.request_uri(kw,query),uri)

    def checkFW(self,text,size,match):

        def make_it(text=text,size=size):
            return util.FileWrapper(StringIO(text),size)

        compare_generic_iter(make_it,match)

        it = make_it()
        self.assertFalse(it.filelike.closed)

        for item in it:
            pass

        self.assertFalse(it.filelike.closed)

        it.close()
        self.assertTrue(it.filelike.closed)

    def testSimpleShifts(self):
        self.checkShift('','/', '', '/', '')
        self.checkShift('','/x', 'x', '/x', '')
        self.checkShift('/','', None, '/', '')
        self.checkShift('/a','/x/y', 'x', '/a/x', '/y')
        self.checkShift('/a','/x/',  'x', '/a/x', '/')

    def testNormalizedShifts(self):
        self.checkShift('/a/b', '/../y', '..', '/a', '/y')
        self.checkShift('', '/../y', '..', '', '/y')
        self.checkShift('/a/b', '//y', 'y', '/a/b/y', '')
        self.checkShift('/a/b', '//y/', 'y', '/a/b/y', '/')
        self.checkShift('/a/b', '/./y', 'y', '/a/b/y', '')
        self.checkShift('/a/b', '/./y/', 'y', '/a/b/y', '/')
        self.checkShift('/a/b', '///./..//y/.//', '..', '/a', '/y/')
        self.checkShift('/a/b', '///', '', '/a/b/', '')
        self.checkShift('/a/b', '/.//', '', '/a/b/', '')
        self.checkShift('/a/b', '/x//', 'x', '/a/b/x', '/')
        self.checkShift('/a/b', '/.', None, '/a/b', '')

    def testDefaults(self):
        for key, value in [
            ('SERVER_NAME','127.0.0.1'),
            ('SERVER_PORT', '80'),
            ('SERVER_PROTOCOL','HTTP/1.0'),
            ('HTTP_HOST','127.0.0.1'),
            ('REQUEST_METHOD','GET'),
            ('SCRIPT_NAME',''),
            ('PATH_INFO','/'),
            ('wsgi.version', (1,0)),
            ('wsgi.run_once', 0),
            ('wsgi.multithread', 0),
            ('wsgi.multiprocess', 0),
            ('wsgi.input', BytesIO()),
            ('wsgi.errors', StringIO()),
            ('wsgi.url_scheme','http'),
        ]:
            self.checkDefault(key,value)

    def testCrossDefaults(self):
        self.checkCrossDefault('HTTP_HOST',"foo.bar",SERVER_NAME="foo.bar")
        self.checkCrossDefault('wsgi.url_scheme',"https",HTTPS="on")
        self.checkCrossDefault('wsgi.url_scheme',"https",HTTPS="1")
        self.checkCrossDefault('wsgi.url_scheme',"https",HTTPS="yes")
        self.checkCrossDefault('wsgi.url_scheme',"http",HTTPS="foo")
        self.checkCrossDefault('SERVER_PORT',"80",HTTPS="foo")
        self.checkCrossDefault('SERVER_PORT',"443",HTTPS="on")

    def testGuessScheme(self):
        self.assertEqual(util.guess_scheme({}), "http")
        self.assertEqual(util.guess_scheme({'HTTPS':"foo"}), "http")
        self.assertEqual(util.guess_scheme({'HTTPS':"on"}), "https")
        self.assertEqual(util.guess_scheme({'HTTPS':"yes"}), "https")
        self.assertEqual(util.guess_scheme({'HTTPS':"1"}), "https")

    def testAppURIs(self):
        self.checkAppURI("http://127.0.0.1/")
        self.checkAppURI("http://127.0.0.1/spam", SCRIPT_NAME="/spam")
        self.checkAppURI("http://127.0.0.1/sp%E4m", SCRIPT_NAME="/sp\xe4m")
        self.checkAppURI("http://spam.example.com:2071/",
            HTTP_HOST="spam.example.com:2071", SERVER_PORT="2071")
        self.checkAppURI("http://spam.example.com/",
            SERVER_NAME="spam.example.com")
        self.checkAppURI("http://127.0.0.1/",
            HTTP_HOST="127.0.0.1", SERVER_NAME="spam.example.com")
        self.checkAppURI("https://127.0.0.1/", HTTPS="on")
        self.checkAppURI("http://127.0.0.1:8000/", SERVER_PORT="8000",
            HTTP_HOST=None)

    def testReqURIs(self):
        self.checkReqURI("http://127.0.0.1/")
        self.checkReqURI("http://127.0.0.1/spam", SCRIPT_NAME="/spam")
        self.checkReqURI("http://127.0.0.1/sp%E4m", SCRIPT_NAME="/sp\xe4m")
        self.checkReqURI("http://127.0.0.1/spammity/spam",
            SCRIPT_NAME="/spammity", PATH_INFO="/spam")
        self.checkReqURI("http://127.0.0.1/spammity/sp%E4m",
            SCRIPT_NAME="/spammity", PATH_INFO="/sp\xe4m")
        self.checkReqURI("http://127.0.0.1/spammity/spam;ham",
            SCRIPT_NAME="/spammity", PATH_INFO="/spam;ham")
        self.checkReqURI("http://127.0.0.1/spammity/spam;cookie=1234,5678",
            SCRIPT_NAME="/spammity", PATH_INFO="/spam;cookie=1234,5678")
        self.checkReqURI("http://127.0.0.1/spammity/spam?say=ni",
            SCRIPT_NAME="/spammity", PATH_INFO="/spam",QUERY_STRING="say=ni")
        self.checkReqURI("http://127.0.0.1/spammity/spam?s%E4y=ni",
            SCRIPT_NAME="/spammity", PATH_INFO="/spam",QUERY_STRING="s%E4y=ni")
        self.checkReqURI("http://127.0.0.1/spammity/spam", 0,
            SCRIPT_NAME="/spammity", PATH_INFO="/spam",QUERY_STRING="say=ni")

    def testFileWrapper(self):
        self.checkFW("xyz"*50, 120, ["xyz"*40,"xyz"*10])

    def testHopByHop(self):
        for hop in (
            "Connection Keep-Alive Proxy-Authenticate Proxy-Authorization "
            "TE Trailers Transfer-Encoding Upgrade"
        ).split():
            for alt in hop, hop.title(), hop.upper(), hop.lower():
                self.assertTrue(util.is_hop_by_hop(alt))

        # Not comprehensive, just a few random header names
        for hop in (
            "Accept Cache-Control Date Pragma Trailer Via Warning"
        ).split():
            for alt in hop, hop.title(), hop.upper(), hop.lower():
                self.assertFalse(util.is_hop_by_hop(alt))

class HeaderTests(TestCase):

    def testMappingInterface(self):
        test = [('x','y')]
        self.assertEqual(len(Headers()), 0)
        self.assertEqual(len(Headers([])),0)
        self.assertEqual(len(Headers(test[:])),1)
        self.assertEqual(Headers(test[:]).keys(), ['x'])
        self.assertEqual(Headers(test[:]).values(), ['y'])
        self.assertEqual(Headers(test[:]).items(), test)
        self.assertIsNot(Headers(test).items(), test)  # must be copy!

        h = Headers()
        del h['foo']   # should not raise an error

        h['Foo'] = 'bar'
        for m in h.__contains__, h.get, h.get_all, h.__getitem__:
            self.assertTrue(m('foo'))
            self.assertTrue(m('Foo'))
            self.assertTrue(m('FOO'))
            self.assertFalse(m('bar'))

        self.assertEqual(h['foo'],'bar')
        h['foo'] = 'baz'
        self.assertEqual(h['FOO'],'baz')
        self.assertEqual(h.get_all('foo'),['baz'])

        self.assertEqual(h.get("foo","whee"), "baz")
        self.assertEqual(h.get("zoo","whee"), "whee")
        self.assertEqual(h.setdefault("foo","whee"), "baz")
        self.assertEqual(h.setdefault("zoo","whee"), "whee")
        self.assertEqual(h["foo"],"baz")
        self.assertEqual(h["zoo"],"whee")

    def testRequireList(self):
        self.assertRaises(TypeError, Headers, "foo")

    def testExtras(self):
        h = Headers()
        self.assertEqual(str(h),'\r\n')

        h.add_header('foo','bar',baz="spam")
        self.assertEqual(h['foo'], 'bar; baz="spam"')
        self.assertEqual(str(h),'foo: bar; baz="spam"\r\n\r\n')

        h.add_header('Foo','bar',cheese=None)
        self.assertEqual(h.get_all('foo'),
            ['bar; baz="spam"', 'bar; cheese'])

        self.assertEqual(str(h),
            'foo: bar; baz="spam"\r\n'
            'Foo: bar; cheese\r\n'
            '\r\n'
        )

class ErrorHandler(BaseCGIHandler):
    """Simple handler subclass for testing BaseHandler"""

    # BaseHandler records the OS environment at import time, but envvars
    # might have been changed later by other tests, which trips up
    # HandlerTests.testEnviron().
    os_environ = dict(os.environ.items())

    def __init__(self,**kw):
        setup_testing_defaults(kw)
        BaseCGIHandler.__init__(
            self, BytesIO(), BytesIO(), StringIO(), kw,
            multithread=True, multiprocess=True
        )

class TestHandler(ErrorHandler):
    """Simple handler subclass for testing BaseHandler, w/error passthru"""

    def handle_error(self):
        raise   # for testing, we want to see what's happening


class HandlerTests(TestCase):

    def checkEnvironAttrs(self, handler):
        env = handler.environ
        for attr in [
            'version','multithread','multiprocess','run_once','file_wrapper'
        ]:
            if attr=='file_wrapper' and handler.wsgi_file_wrapper is None:
                continue
            self.assertEqual(getattr(handler,'wsgi_'+attr),env['wsgi.'+attr])

    def checkOSEnviron(self,handler):
        empty = {}; setup_testing_defaults(empty)
        env = handler.environ
        from os import environ
        for k,v in environ.items():
            if k not in empty:
                self.assertEqual(env[k],v)
        for k,v in empty.items():
            self.assertIn(k, env)

    def testEnviron(self):
        h = TestHandler(X="Y")
        h.setup_environ()
        self.checkEnvironAttrs(h)
        self.checkOSEnviron(h)
        self.assertEqual(h.environ["X"],"Y")

    def testCGIEnviron(self):
        h = BaseCGIHandler(None,None,None,{})
        h.setup_environ()
        for key in 'wsgi.url_scheme', 'wsgi.input', 'wsgi.errors':
            self.assertIn(key, h.environ)

    def testScheme(self):
        h=TestHandler(HTTPS="on"); h.setup_environ()
        self.assertEqual(h.environ['wsgi.url_scheme'],'https')
        h=TestHandler(); h.setup_environ()
        self.assertEqual(h.environ['wsgi.url_scheme'],'http')

    def testAbstractMethods(self):
        h = BaseHandler()
        for name in [
            '_flush','get_stdin','get_stderr','add_cgi_vars'
        ]:
            self.assertRaises(NotImplementedError, getattr(h,name))
        self.assertRaises(NotImplementedError, h._write, "test")

    def testContentLength(self):
        # Demo one reason iteration is better than write()...  ;)

        def trivial_app1(e,s):
            s('200 OK',[])
            return [e['wsgi.url_scheme'].encode('iso-8859-1')]

        def trivial_app2(e,s):
            s('200 OK',[])(e['wsgi.url_scheme'].encode('iso-8859-1'))
            return []

        def trivial_app3(e,s):
            s('200 OK',[])
            return ['\u0442\u0435\u0441\u0442'.encode("utf-8")]

        def trivial_app4(e,s):
            # Simulate a response to a HEAD request
            s('200 OK',[('Content-Length', '12345')])
            return []

        h = TestHandler()
        h.run(trivial_app1)
        self.assertEqual(h.stdout.getvalue(),
            ("Status: 200 OK\r\n"
            "Content-Length: 4\r\n"
            "\r\n"
            "http").encode("iso-8859-1"))

        h = TestHandler()
        h.run(trivial_app2)
        self.assertEqual(h.stdout.getvalue(),
            ("Status: 200 OK\r\n"
            "\r\n"
            "http").encode("iso-8859-1"))

        h = TestHandler()
        h.run(trivial_app3)
        self.assertEqual(h.stdout.getvalue(),
            b'Status: 200 OK\r\n'
            b'Content-Length: 8\r\n'
            b'\r\n'
            b'\xd1\x82\xd0\xb5\xd1\x81\xd1\x82')

        h = TestHandler()
        h.run(trivial_app4)
        self.assertEqual(h.stdout.getvalue(),
            b'Status: 200 OK\r\n'
            b'Content-Length: 12345\r\n'
            b'\r\n')

    def testBasicErrorOutput(self):

        def non_error_app(e,s):
            s('200 OK',[])
            return []

        def error_app(e,s):
            raise AssertionError("This should be caught by handler")

        h = ErrorHandler()
        h.run(non_error_app)
        self.assertEqual(h.stdout.getvalue(),
            ("Status: 200 OK\r\n"
            "Content-Length: 0\r\n"
            "\r\n").encode("iso-8859-1"))
        self.assertEqual(h.stderr.getvalue(),"")

        h = ErrorHandler()
        h.run(error_app)
        self.assertEqual(h.stdout.getvalue(),
            ("Status: %s\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "\r\n" % (h.error_status,len(h.error_body))).encode('iso-8859-1')
            + h.error_body)

        self.assertIn("AssertionError", h.stderr.getvalue())

    def testErrorAfterOutput(self):
        MSG = b"Some output has been sent"
        def error_app(e,s):
            s("200 OK",[])(MSG)
            raise AssertionError("This should be caught by handler")

        h = ErrorHandler()
        h.run(error_app)
        self.assertEqual(h.stdout.getvalue(),
            ("Status: 200 OK\r\n"
            "\r\n".encode("iso-8859-1")+MSG))
        self.assertIn("AssertionError", h.stderr.getvalue())

    def testHeaderFormats(self):

        def non_error_app(e,s):
            s('200 OK',[])
            return []

        stdpat = (
            r"HTTP/%s 200 OK\r\n"
            r"Date: \w{3}, [ 0123]\d \w{3} \d{4} \d\d:\d\d:\d\d GMT\r\n"
            r"%s" r"Content-Length: 0\r\n" r"\r\n"
        )
        shortpat = (
            "Status: 200 OK\r\n" "Content-Length: 0\r\n" "\r\n"
        ).encode("iso-8859-1")

        for ssw in "FooBar/1.0", None:
            sw = ssw and "Server: %s\r\n" % ssw or ""

            for version in "1.0", "1.1":
                for proto in "HTTP/0.9", "HTTP/1.0", "HTTP/1.1":

                    h = TestHandler(SERVER_PROTOCOL=proto)
                    h.origin_server = False
                    h.http_version = version
                    h.server_software = ssw
                    h.run(non_error_app)
                    self.assertEqual(shortpat,h.stdout.getvalue())

                    h = TestHandler(SERVER_PROTOCOL=proto)
                    h.origin_server = True
                    h.http_version = version
                    h.server_software = ssw
                    h.run(non_error_app)
                    if proto=="HTTP/0.9":
                        self.assertEqual(h.stdout.getvalue(),b"")
                    else:
                        self.assertTrue(
                            re.match((stdpat%(version,sw)).encode("iso-8859-1"),
                                h.stdout.getvalue()),
                            ((stdpat%(version,sw)).encode("iso-8859-1"),
                                h.stdout.getvalue())
                        )

    def testBytesData(self):
        def app(e, s):
            s("200 OK", [
                ("Content-Type", "text/plain; charset=utf-8"),
                ])
            return [b"data"]

        h = TestHandler()
        h.run(app)
        self.assertEqual(b"Status: 200 OK\r\n"
            b"Content-Type: text/plain; charset=utf-8\r\n"
            b"Content-Length: 4\r\n"
            b"\r\n"
            b"data",
            h.stdout.getvalue())

    def testCloseOnError(self):
        side_effects = {'close_called': False}
        MSG = b"Some output has been sent"
        def error_app(e,s):
            s("200 OK",[])(MSG)
            class CrashyIterable(object):
                def __iter__(self):
                    while True:
                        yield b'blah'
                        raise AssertionError("This should be caught by handler")
                def close(self):
                    side_effects['close_called'] = True
            return CrashyIterable()

        h = ErrorHandler()
        h.run(error_app)
        self.assertEqual(side_effects['close_called'], True)

    def testPartialWrite(self):
        written = bytearray()

        class PartialWriter:
            def write(self, b):
                partial = b[:7]
                written.extend(partial)
                return len(partial)

            def flush(self):
                pass

        environ = {"SERVER_PROTOCOL": "HTTP/1.0"}
        h = SimpleHandler(BytesIO(), PartialWriter(), sys.stderr, environ)
        msg = "should not do partial writes"
        with self.assertWarnsRegex(DeprecationWarning, msg):
            h.run(hello_app)
        self.assertEqual(b"HTTP/1.0 200 OK\r\n"
            b"Content-Type: text/plain\r\n"
            b"Date: Mon, 05 Jun 2006 18:49:54 GMT\r\n"
            b"Content-Length: 13\r\n"
            b"\r\n"
            b"Hello, world!",
            written)


if __name__ == "__main__":
    unittest.main()
