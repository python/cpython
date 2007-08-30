import httplib
import StringIO
import sys
import socket

from unittest import TestCase

from test import test_support

class FakeSocket:
    def __init__(self, text, fileclass=StringIO.StringIO):
        self.text = text
        self.fileclass = fileclass
        self.data = ''

    def sendall(self, data):
        self.data += data

    def makefile(self, mode, bufsize=None):
        if mode != 'r' and mode != 'rb':
            raise httplib.UnimplementedFileMode()
        return self.fileclass(self.text)

class NoEOFStringIO(StringIO.StringIO):
    """Like StringIO, but raises AssertionError on EOF.

    This is used below to test that httplib doesn't try to read
    more from the underlying file than it should.
    """
    def read(self, n=-1):
        data = StringIO.StringIO.read(self, n)
        if data == '':
            raise AssertionError('caller tried to read past EOF')
        return data

    def readline(self, length=None):
        data = StringIO.StringIO.readline(self, length)
        if data == '':
            raise AssertionError('caller tried to read past EOF')
        return data


class HeaderTests(TestCase):
    def test_auto_headers(self):
        # Some headers are added automatically, but should not be added by
        # .request() if they are explicitly set.

        import httplib

        class HeaderCountingBuffer(list):
            def __init__(self):
                self.count = {}
            def append(self, item):
                kv = item.split(':')
                if len(kv) > 1:
                    # item is a 'Key: Value' header string
                    lcKey = kv[0].lower()
                    self.count.setdefault(lcKey, 0)
                    self.count[lcKey] += 1
                list.append(self, item)

        for explicit_header in True, False:
            for header in 'Content-length', 'Host', 'Accept-encoding':
                conn = httplib.HTTPConnection('example.com')
                conn.sock = FakeSocket('blahblahblah')
                conn._buffer = HeaderCountingBuffer()

                body = 'spamspamspam'
                headers = {}
                if explicit_header:
                    headers[header] = str(len(body))
                conn.request('POST', '/', body, headers)
                self.assertEqual(conn._buffer.count[header.lower()], 1)

class BasicTest(TestCase):
    def test_status_lines(self):
        # Test HTTP status lines

        body = "HTTP/1.1 200 Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(), 'Text')
        resp.close()

        body = "HTTP/1.1 400.100 Not Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        self.assertRaises(httplib.BadStatusLine, resp.begin)

    def test_host_port(self):
        # Check invalid host_port

        for hp in ("www.python.org:abc", "www.python.org:"):
            self.assertRaises(httplib.InvalidURL, httplib.HTTP, hp)

        for hp, h, p in (("[fe80::207:e9ff:fe9b]:8000", "fe80::207:e9ff:fe9b", 8000),
                         ("www.python.org:80", "www.python.org", 80),
                         ("www.python.org", "www.python.org", 80),
                         ("[fe80::207:e9ff:fe9b]", "fe80::207:e9ff:fe9b", 80)):
            http = httplib.HTTP(hp)
            c = http._conn
            if h != c.host: self.fail("Host incorrectly parsed: %s != %s" % (h, c.host))
            if p != c.port: self.fail("Port incorrectly parsed: %s != %s" % (p, c.host))

    def test_response_headers(self):
        # test response with multiple message headers with the same field name.
        text = ('HTTP/1.1 200 OK\r\n'
                'Set-Cookie: Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"\r\n'
                'Set-Cookie: Part_Number="Rocket_Launcher_0001"; Version="1";'
                ' Path="/acme"\r\n'
                '\r\n'
                'No body\r\n')
        hdr = ('Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"'
               ', '
               'Part_Number="Rocket_Launcher_0001"; Version="1"; Path="/acme"')
        s = FakeSocket(text)
        r = httplib.HTTPResponse(s)
        r.begin()
        cookies = r.getheader("Set-Cookie")
        if cookies != hdr:
            self.fail("multiple headers not combined properly")

    def test_read_head(self):
        # Test that the library doesn't attempt to read any data
        # from a HEAD request.  (Tickles SF bug #622042.)
        sock = FakeSocket(
            'HTTP/1.1 200 OK\r\n'
            'Content-Length: 14432\r\n'
            '\r\n',
            NoEOFStringIO)
        resp = httplib.HTTPResponse(sock, method="HEAD")
        resp.begin()
        if resp.read() != "":
            self.fail("Did not expect response from HEAD request")
        resp.close()

    def test_send_file(self):
        expected = 'GET /foo HTTP/1.1\r\nHost: example.com\r\n' \
                   'Accept-Encoding: identity\r\nContent-Length:'

        body = open(__file__, 'rb')
        conn = httplib.HTTPConnection('example.com')
        sock = FakeSocket(body)
        conn.sock = sock
        conn.request('GET', '/foo', body)
        self.assertTrue(sock.data.startswith(expected))

class OfflineTest(TestCase):
    def test_responses(self):
        self.assertEquals(httplib.responses[httplib.NOT_FOUND], "Not Found")

PORT = 50003
HOST = "localhost"

class TimeoutTest(TestCase):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        global PORT
        PORT = test_support.bind_port(self.serv, HOST, PORT)
        self.serv.listen(5)

    def tearDown(self):
        self.serv.close()
        self.serv = None

    def testTimeoutAttribute(self):
        '''This will prove that the timeout gets through
        HTTPConnection and into the socket.
        '''
        # default
        httpConn = httplib.HTTPConnection(HOST, PORT)
        httpConn.connect()
        self.assertTrue(httpConn.sock.gettimeout() is None)
        httpConn.close()

        # a value
        httpConn = httplib.HTTPConnection(HOST, PORT, timeout=30)
        httpConn.connect()
        self.assertEqual(httpConn.sock.gettimeout(), 30)
        httpConn.close()

        # None, having other default
        previous = socket.getdefaulttimeout()
        socket.setdefaulttimeout(30)
        try:
            httpConn = httplib.HTTPConnection(HOST, PORT, timeout=None)
            httpConn.connect()
        finally:
            socket.setdefaulttimeout(previous)
        self.assertEqual(httpConn.sock.gettimeout(), 30)
        httpConn.close()


class HTTPSTimeoutTest(TestCase):
# XXX Here should be tests for HTTPS, there isn't any right now!

    def test_attributes(self):
        # simple test to check it's storing it
        if hasattr(httplib, 'HTTPSConnection'):
            h = httplib.HTTPSConnection(HOST, PORT, timeout=30)
            self.assertEqual(h.timeout, 30)

def test_main(verbose=None):
    test_support.run_unittest(HeaderTests, OfflineTest, BasicTest, TimeoutTest, HTTPSTimeoutTest)

if __name__ == '__main__':
    test_main()
