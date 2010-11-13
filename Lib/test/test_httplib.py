import errno
from http import client
import io
import os
import array
import socket

import unittest
TestCase = unittest.TestCase

from test import support

here = os.path.dirname(__file__)
# Self-signed cert file for 'localhost'
CERT_localhost = os.path.join(here, 'keycert.pem')
# Self-signed cert file for 'fakehostname'
CERT_fakehostname = os.path.join(here, 'keycert2.pem')
# Root cert file (CA) for svn.python.org's cert
CACERT_svn_python_org = os.path.join(here, 'https_svn_python_org_root.pem')

HOST = support.HOST

class FakeSocket:
    def __init__(self, text, fileclass=io.BytesIO):
        if isinstance(text, str):
            text = text.encode("ascii")
        self.text = text
        self.fileclass = fileclass
        self.data = b''

    def sendall(self, data):
        self.data += data

    def makefile(self, mode, bufsize=None):
        if mode != 'r' and mode != 'rb':
            raise client.UnimplementedFileMode()
        return self.fileclass(self.text)

class EPipeSocket(FakeSocket):

    def __init__(self, text, pipe_trigger):
        # When sendall() is called with pipe_trigger, raise EPIPE.
        FakeSocket.__init__(self, text)
        self.pipe_trigger = pipe_trigger

    def sendall(self, data):
        if self.pipe_trigger in data:
            raise socket.error(errno.EPIPE, "gotcha")
        self.data += data

    def close(self):
        pass

class NoEOFStringIO(io.BytesIO):
    """Like StringIO, but raises AssertionError on EOF.

    This is used below to test that http.client doesn't try to read
    more from the underlying file than it should.
    """
    def read(self, n=-1):
        data = io.BytesIO.read(self, n)
        if data == b'':
            raise AssertionError('caller tried to read past EOF')
        return data

    def readline(self, length=None):
        data = io.BytesIO.readline(self, length)
        if data == b'':
            raise AssertionError('caller tried to read past EOF')
        return data

class HeaderTests(TestCase):
    def test_auto_headers(self):
        # Some headers are added automatically, but should not be added by
        # .request() if they are explicitly set.

        class HeaderCountingBuffer(list):
            def __init__(self):
                self.count = {}
            def append(self, item):
                kv = item.split(b':')
                if len(kv) > 1:
                    # item is a 'Key: Value' header string
                    lcKey = kv[0].decode('ascii').lower()
                    self.count.setdefault(lcKey, 0)
                    self.count[lcKey] += 1
                list.append(self, item)

        for explicit_header in True, False:
            for header in 'Content-length', 'Host', 'Accept-encoding':
                conn = client.HTTPConnection('example.com')
                conn.sock = FakeSocket('blahblahblah')
                conn._buffer = HeaderCountingBuffer()

                body = 'spamspamspam'
                headers = {}
                if explicit_header:
                    headers[header] = str(len(body))
                conn.request('POST', '/', body, headers)
                self.assertEqual(conn._buffer.count[header.lower()], 1)

    def test_putheader(self):
        conn = client.HTTPConnection('example.com')
        conn.sock = FakeSocket(None)
        conn.putrequest('GET','/')
        conn.putheader('Content-length', 42)
        self.assertTrue(b'Content-length: 42' in conn._buffer)

    def test_ipv6host_header(self):
        # Default host header on IPv6 transaction should wrapped by [] if
        # its actual IPv6 address
        expected = b'GET /foo HTTP/1.1\r\nHost: [2001::]:81\r\n' \
                   b'Accept-Encoding: identity\r\n\r\n'
        conn = client.HTTPConnection('[2001::]:81')
        sock = FakeSocket('')
        conn.sock = sock
        conn.request('GET', '/foo')
        self.assertTrue(sock.data.startswith(expected))

        expected = b'GET /foo HTTP/1.1\r\nHost: [2001:102A::]\r\n' \
                   b'Accept-Encoding: identity\r\n\r\n'
        conn = client.HTTPConnection('[2001:102A::]')
        sock = FakeSocket('')
        conn.sock = sock
        conn.request('GET', '/foo')
        self.assertTrue(sock.data.startswith(expected))


class BasicTest(TestCase):
    def test_status_lines(self):
        # Test HTTP status lines

        body = "HTTP/1.1 200 Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(), b"Text")
        self.assertTrue(resp.isclosed())

        body = "HTTP/1.1 400.100 Not Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        self.assertRaises(client.BadStatusLine, resp.begin)

    def test_bad_status_repr(self):
        exc = client.BadStatusLine('')
        self.assertEquals(repr(exc), '''BadStatusLine("\'\'",)''')

    def test_partial_reads(self):
        # if we have a lenght, the system knows when to close itself
        # same behaviour than when we read the whole thing with read()
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 4\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(2), b'Te')
        self.assertFalse(resp.isclosed())
        self.assertEqual(resp.read(2), b'xt')
        self.assertTrue(resp.isclosed())

    def test_host_port(self):
        # Check invalid host_port

        for hp in ("www.python.org:abc", "www.python.org:"):
            self.assertRaises(client.InvalidURL, client.HTTPConnection, hp)

        for hp, h, p in (("[fe80::207:e9ff:fe9b]:8000",
                          "fe80::207:e9ff:fe9b", 8000),
                         ("www.python.org:80", "www.python.org", 80),
                         ("www.python.org", "www.python.org", 80),
                         ("[fe80::207:e9ff:fe9b]", "fe80::207:e9ff:fe9b", 80)):
            c = client.HTTPConnection(hp)
            self.assertEqual(h, c.host)
            self.assertEqual(p, c.port)

    def test_response_headers(self):
        # test response with multiple message headers with the same field name.
        text = ('HTTP/1.1 200 OK\r\n'
                'Set-Cookie: Customer="WILE_E_COYOTE"; '
                'Version="1"; Path="/acme"\r\n'
                'Set-Cookie: Part_Number="Rocket_Launcher_0001"; Version="1";'
                ' Path="/acme"\r\n'
                '\r\n'
                'No body\r\n')
        hdr = ('Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"'
               ', '
               'Part_Number="Rocket_Launcher_0001"; Version="1"; Path="/acme"')
        s = FakeSocket(text)
        r = client.HTTPResponse(s)
        r.begin()
        cookies = r.getheader("Set-Cookie")
        self.assertEqual(cookies, hdr)

    def test_read_head(self):
        # Test that the library doesn't attempt to read any data
        # from a HEAD request.  (Tickles SF bug #622042.)
        sock = FakeSocket(
            'HTTP/1.1 200 OK\r\n'
            'Content-Length: 14432\r\n'
            '\r\n',
            NoEOFStringIO)
        resp = client.HTTPResponse(sock, method="HEAD")
        resp.begin()
        if resp.read():
            self.fail("Did not expect response from HEAD request")

    def test_send_file(self):
        expected = (b'GET /foo HTTP/1.1\r\nHost: example.com\r\n'
                    b'Accept-Encoding: identity\r\nContent-Length:')

        with open(__file__, 'rb') as body:
            conn = client.HTTPConnection('example.com')
            sock = FakeSocket(body)
            conn.sock = sock
            conn.request('GET', '/foo', body)
            self.assertTrue(sock.data.startswith(expected), '%r != %r' %
                    (sock.data[:len(expected)], expected))

    def test_send(self):
        expected = b'this is a test this is only a test'
        conn = client.HTTPConnection('example.com')
        sock = FakeSocket(None)
        conn.sock = sock
        conn.send(expected)
        self.assertEquals(expected, sock.data)
        sock.data = b''
        conn.send(array.array('b', expected))
        self.assertEquals(expected, sock.data)
        sock.data = b''
        conn.send(io.BytesIO(expected))
        self.assertEquals(expected, sock.data)

    def test_chunked(self):
        chunked_start = (
            'HTTP/1.1 200 OK\r\n'
            'Transfer-Encoding: chunked\r\n\r\n'
            'a\r\n'
            'hello worl\r\n'
            '1\r\n'
            'd\r\n'
        )
        sock = FakeSocket(chunked_start + '0\r\n')
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEquals(resp.read(), b'hello world')
        resp.close()

        for x in ('', 'foo\r\n'):
            sock = FakeSocket(chunked_start + x)
            resp = client.HTTPResponse(sock, method="GET")
            resp.begin()
            try:
                resp.read()
            except client.IncompleteRead as i:
                self.assertEquals(i.partial, b'hello world')
                self.assertEqual(repr(i),'IncompleteRead(11 bytes read)')
                self.assertEqual(str(i),'IncompleteRead(11 bytes read)')
            else:
                self.fail('IncompleteRead expected')
            finally:
                resp.close()

    def test_chunked_head(self):
        chunked_start = (
            'HTTP/1.1 200 OK\r\n'
            'Transfer-Encoding: chunked\r\n\r\n'
            'a\r\n'
            'hello world\r\n'
            '1\r\n'
            'd\r\n'
        )
        sock = FakeSocket(chunked_start + '0\r\n')
        resp = client.HTTPResponse(sock, method="HEAD")
        resp.begin()
        self.assertEquals(resp.read(), b'')
        self.assertEquals(resp.status, 200)
        self.assertEquals(resp.reason, 'OK')
        self.assertTrue(resp.isclosed())

    def test_negative_content_length(self):
        sock = FakeSocket(
            'HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\nHello\r\n')
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEquals(resp.read(), b'Hello\r\n')
        resp.close()

    def test_incomplete_read(self):
        sock = FakeSocket('HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nHello\r\n')
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        try:
            resp.read()
        except client.IncompleteRead as i:
            self.assertEquals(i.partial, b'Hello\r\n')
            self.assertEqual(repr(i),
                             "IncompleteRead(7 bytes read, 3 more expected)")
            self.assertEqual(str(i),
                             "IncompleteRead(7 bytes read, 3 more expected)")
        else:
            self.fail('IncompleteRead expected')
        finally:
            resp.close()

    def test_epipe(self):
        sock = EPipeSocket(
            "HTTP/1.0 401 Authorization Required\r\n"
            "Content-type: text/html\r\n"
            "WWW-Authenticate: Basic realm=\"example\"\r\n",
            b"Content-Length")
        conn = client.HTTPConnection("example.com")
        conn.sock = sock
        self.assertRaises(socket.error,
                          lambda: conn.request("PUT", "/url", "body"))
        resp = conn.getresponse()
        self.assertEqual(401, resp.status)
        self.assertEqual("Basic realm=\"example\"",
                         resp.getheader("www-authenticate"))

class OfflineTest(TestCase):
    def test_responses(self):
        self.assertEquals(client.responses[client.NOT_FOUND], "Not Found")


class SourceAddressTest(TestCase):
    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.port = support.bind_port(self.serv)
        self.source_port = support.find_unused_port()
        self.serv.listen(5)
        self.conn = None

    def tearDown(self):
        if self.conn:
            self.conn.close()
            self.conn = None
        self.serv.close()
        self.serv = None

    def testHTTPConnectionSourceAddress(self):
        self.conn = client.HTTPConnection(HOST, self.port,
                source_address=('', self.source_port))
        self.conn.connect()
        self.assertEqual(self.conn.sock.getsockname()[1], self.source_port)

    @unittest.skipIf(not hasattr(client, 'HTTPSConnection'),
                     'http.client.HTTPSConnection not defined')
    def testHTTPSConnectionSourceAddress(self):
        self.conn = client.HTTPSConnection(HOST, self.port,
                source_address=('', self.source_port))
        # We don't test anything here other the constructor not barfing as
        # this code doesn't deal with setting up an active running SSL server
        # for an ssl_wrapped connect() to actually return from.


class TimeoutTest(TestCase):
    PORT = None

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        TimeoutTest.PORT = support.bind_port(self.serv)
        self.serv.listen(5)

    def tearDown(self):
        self.serv.close()
        self.serv = None

    def testTimeoutAttribute(self):
        # This will prove that the timeout gets through HTTPConnection
        # and into the socket.

        # default -- use global socket timeout
        self.assertTrue(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            httpConn = client.HTTPConnection(HOST, TimeoutTest.PORT)
            httpConn.connect()
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(httpConn.sock.gettimeout(), 30)
        httpConn.close()

        # no timeout -- do not use global socket default
        self.assertTrue(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            httpConn = client.HTTPConnection(HOST, TimeoutTest.PORT,
                                              timeout=None)
            httpConn.connect()
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(httpConn.sock.gettimeout(), None)
        httpConn.close()

        # a value
        httpConn = client.HTTPConnection(HOST, TimeoutTest.PORT, timeout=30)
        httpConn.connect()
        self.assertEqual(httpConn.sock.gettimeout(), 30)
        httpConn.close()


class HTTPSTest(TestCase):

    def setUp(self):
        if not hasattr(client, 'HTTPSConnection'):
            self.skipTest('ssl support required')

    def make_server(self, certfile):
        from test.ssl_servers import make_https_server
        return make_https_server(self, certfile)

    def test_attributes(self):
        # simple test to check it's storing the timeout
        h = client.HTTPSConnection(HOST, TimeoutTest.PORT, timeout=30)
        self.assertEqual(h.timeout, 30)

    def _check_svn_python_org(self, resp):
        # Just a simple check that everything went fine
        server_string = resp.getheader('server')
        self.assertIn('Apache', server_string)

    def test_networked(self):
        # Default settings: no cert verification is done
        support.requires('network')
        with support.transient_internet('svn.python.org'):
            h = client.HTTPSConnection('svn.python.org', 443)
            h.request('GET', '/')
            resp = h.getresponse()
            self._check_svn_python_org(resp)

    def test_networked_good_cert(self):
        # We feed a CA cert that validates the server's cert
        import ssl
        support.requires('network')
        with support.transient_internet('svn.python.org'):
            context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            context.verify_mode = ssl.CERT_REQUIRED
            context.load_verify_locations(CACERT_svn_python_org)
            h = client.HTTPSConnection('svn.python.org', 443, context=context)
            h.request('GET', '/')
            resp = h.getresponse()
            self._check_svn_python_org(resp)

    def test_networked_bad_cert(self):
        # We feed a "CA" cert that is unrelated to the server's cert
        import ssl
        support.requires('network')
        with support.transient_internet('svn.python.org'):
            context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            context.verify_mode = ssl.CERT_REQUIRED
            context.load_verify_locations(CERT_localhost)
            h = client.HTTPSConnection('svn.python.org', 443, context=context)
            with self.assertRaises(ssl.SSLError):
                h.request('GET', '/')

    def test_local_good_hostname(self):
        # The (valid) cert validates the HTTP hostname
        import ssl
        from test.ssl_servers import make_https_server
        server = make_https_server(self, CERT_localhost)
        context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        context.verify_mode = ssl.CERT_REQUIRED
        context.load_verify_locations(CERT_localhost)
        h = client.HTTPSConnection('localhost', server.port, context=context)
        h.request('GET', '/nonexistent')
        resp = h.getresponse()
        self.assertEqual(resp.status, 404)

    def test_local_bad_hostname(self):
        # The (valid) cert doesn't validate the HTTP hostname
        import ssl
        from test.ssl_servers import make_https_server
        server = make_https_server(self, CERT_fakehostname)
        context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        context.verify_mode = ssl.CERT_REQUIRED
        context.load_verify_locations(CERT_fakehostname)
        h = client.HTTPSConnection('localhost', server.port, context=context)
        with self.assertRaises(ssl.CertificateError):
            h.request('GET', '/')
        # Same with explicit check_hostname=True
        h = client.HTTPSConnection('localhost', server.port, context=context,
                                   check_hostname=True)
        with self.assertRaises(ssl.CertificateError):
            h.request('GET', '/')
        # With check_hostname=False, the mismatching is ignored
        h = client.HTTPSConnection('localhost', server.port, context=context,
                                   check_hostname=False)
        h.request('GET', '/nonexistent')
        resp = h.getresponse()
        self.assertEqual(resp.status, 404)


class RequestBodyTest(TestCase):
    """Test cases where a request includes a message body."""

    def setUp(self):
        self.conn = client.HTTPConnection('example.com')
        self.conn.sock = self.sock = FakeSocket("")
        self.conn.sock = self.sock

    def get_headers_and_fp(self):
        f = io.BytesIO(self.sock.data)
        f.readline()  # read the request line
        message = client.parse_headers(f)
        return message, f

    def test_manual_content_length(self):
        # Set an incorrect content-length so that we can verify that
        # it will not be over-ridden by the library.
        self.conn.request("PUT", "/url", "body",
                          {"Content-Length": "42"})
        message, f = self.get_headers_and_fp()
        self.assertEqual("42", message.get("content-length"))
        self.assertEqual(4, len(f.read()))

    def test_ascii_body(self):
        self.conn.request("PUT", "/url", "body")
        message, f = self.get_headers_and_fp()
        self.assertEqual("text/plain", message.get_content_type())
        self.assertEqual(None, message.get_charset())
        self.assertEqual("4", message.get("content-length"))
        self.assertEqual(b'body', f.read())

    def test_latin1_body(self):
        self.conn.request("PUT", "/url", "body\xc1")
        message, f = self.get_headers_and_fp()
        self.assertEqual("text/plain", message.get_content_type())
        self.assertEqual(None, message.get_charset())
        self.assertEqual("5", message.get("content-length"))
        self.assertEqual(b'body\xc1', f.read())

    def test_bytes_body(self):
        self.conn.request("PUT", "/url", b"body\xc1")
        message, f = self.get_headers_and_fp()
        self.assertEqual("text/plain", message.get_content_type())
        self.assertEqual(None, message.get_charset())
        self.assertEqual("5", message.get("content-length"))
        self.assertEqual(b'body\xc1', f.read())

    def test_file_body(self):
        with open(support.TESTFN, "w") as f:
            f.write("body")
        with open(support.TESTFN) as f:
            self.conn.request("PUT", "/url", f)
            message, f = self.get_headers_and_fp()
            self.assertEqual("text/plain", message.get_content_type())
            self.assertEqual(None, message.get_charset())
            self.assertEqual("4", message.get("content-length"))
            self.assertEqual(b'body', f.read())

    def test_binary_file_body(self):
        with open(support.TESTFN, "wb") as f:
            f.write(b"body\xc1")
        with open(support.TESTFN, "rb") as f:
            self.conn.request("PUT", "/url", f)
            message, f = self.get_headers_and_fp()
            self.assertEqual("text/plain", message.get_content_type())
            self.assertEqual(None, message.get_charset())
            self.assertEqual("5", message.get("content-length"))
            self.assertEqual(b'body\xc1', f.read())


class HTTPResponseTest(TestCase):

    def setUp(self):
        body = "HTTP/1.1 200 Ok\r\nMy-Header: first-value\r\nMy-Header: \
                second-value\r\n\r\nText"
        sock = FakeSocket(body)
        self.resp = client.HTTPResponse(sock)
        self.resp.begin()

    def test_getting_header(self):
        header = self.resp.getheader('My-Header')
        self.assertEqual(header, 'first-value, second-value')

        header = self.resp.getheader('My-Header', 'some default')
        self.assertEqual(header, 'first-value, second-value')

    def test_getting_nonexistent_header_with_string_default(self):
        header = self.resp.getheader('No-Such-Header', 'default-value')
        self.assertEqual(header, 'default-value')

    def test_getting_nonexistent_header_with_iterable_default(self):
        header = self.resp.getheader('No-Such-Header', ['default', 'values'])
        self.assertEqual(header, 'default, values')

        header = self.resp.getheader('No-Such-Header', ('default', 'values'))
        self.assertEqual(header, 'default, values')

    def test_getting_nonexistent_header_without_default(self):
        header = self.resp.getheader('No-Such-Header')
        self.assertEqual(header, None)

    def test_getting_header_defaultint(self):
        header = self.resp.getheader('No-Such-Header',default=42)
        self.assertEqual(header, 42)

def test_main(verbose=None):
    support.run_unittest(HeaderTests, OfflineTest, BasicTest, TimeoutTest,
                         HTTPSTest, RequestBodyTest, SourceAddressTest,
                         HTTPResponseTest)

if __name__ == '__main__':
    test_main()
