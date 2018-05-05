import httplib
import itertools
import array
import StringIO
import socket
import errno
import os
import tempfile

import unittest
TestCase = unittest.TestCase

from test import test_support

here = os.path.dirname(__file__)
# Self-signed cert file for 'localhost'
CERT_localhost = os.path.join(here, 'keycert.pem')
# Self-signed cert file for 'fakehostname'
CERT_fakehostname = os.path.join(here, 'keycert2.pem')
# Self-signed cert file for self-signed.pythontest.net
CERT_selfsigned_pythontestdotnet = os.path.join(here, 'selfsigned_pythontestdotnet.pem')

HOST = test_support.HOST

class FakeSocket:
    def __init__(self, text, fileclass=StringIO.StringIO, host=None, port=None):
        self.text = text
        self.fileclass = fileclass
        self.data = ''
        self.file_closed = False
        self.host = host
        self.port = port

    def sendall(self, data):
        self.data += ''.join(data)

    def makefile(self, mode, bufsize=None):
        if mode != 'r' and mode != 'rb':
            raise httplib.UnimplementedFileMode()
        # keep the file around so we can check how much was read from it
        self.file = self.fileclass(self.text)
        self.file.close = self.file_close #nerf close ()
        return self.file

    def file_close(self):
        self.file_closed = True

    def close(self):
        pass

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

    def test_content_length_0(self):

        class ContentLengthChecker(list):
            def __init__(self):
                list.__init__(self)
                self.content_length = None
            def append(self, item):
                kv = item.split(':', 1)
                if len(kv) > 1 and kv[0].lower() == 'content-length':
                    self.content_length = kv[1].strip()
                list.append(self, item)

        # Here, we're testing that methods expecting a body get a
        # content-length set to zero if the body is empty (either None or '')
        bodies = (None, '')
        methods_with_body = ('PUT', 'POST', 'PATCH')
        for method, body in itertools.product(methods_with_body, bodies):
            conn = httplib.HTTPConnection('example.com')
            conn.sock = FakeSocket(None)
            conn._buffer = ContentLengthChecker()
            conn.request(method, '/', body)
            self.assertEqual(
                conn._buffer.content_length, '0',
                'Header Content-Length incorrect on {}'.format(method)
            )

        # For these methods, we make sure that content-length is not set when
        # the body is None because it might cause unexpected behaviour on the
        # server.
        methods_without_body = (
             'GET', 'CONNECT', 'DELETE', 'HEAD', 'OPTIONS', 'TRACE',
        )
        for method in methods_without_body:
            conn = httplib.HTTPConnection('example.com')
            conn.sock = FakeSocket(None)
            conn._buffer = ContentLengthChecker()
            conn.request(method, '/', None)
            self.assertEqual(
                conn._buffer.content_length, None,
                'Header Content-Length set for empty body on {}'.format(method)
            )

        # If the body is set to '', that's considered to be "present but
        # empty" rather than "missing", so content length would be set, even
        # for methods that don't expect a body.
        for method in methods_without_body:
            conn = httplib.HTTPConnection('example.com')
            conn.sock = FakeSocket(None)
            conn._buffer = ContentLengthChecker()
            conn.request(method, '/', '')
            self.assertEqual(
                conn._buffer.content_length, '0',
                'Header Content-Length incorrect on {}'.format(method)
            )

        # If the body is set, make sure Content-Length is set.
        for method in itertools.chain(methods_without_body, methods_with_body):
            conn = httplib.HTTPConnection('example.com')
            conn.sock = FakeSocket(None)
            conn._buffer = ContentLengthChecker()
            conn.request(method, '/', ' ')
            self.assertEqual(
                conn._buffer.content_length, '1',
                'Header Content-Length incorrect on {}'.format(method)
            )

    def test_putheader(self):
        conn = httplib.HTTPConnection('example.com')
        conn.sock = FakeSocket(None)
        conn.putrequest('GET','/')
        conn.putheader('Content-length',42)
        self.assertIn('Content-length: 42', conn._buffer)

        conn.putheader('Foo', ' bar ')
        self.assertIn(b'Foo:  bar ', conn._buffer)
        conn.putheader('Bar', '\tbaz\t')
        self.assertIn(b'Bar: \tbaz\t', conn._buffer)
        conn.putheader('Authorization', 'Bearer mytoken')
        self.assertIn(b'Authorization: Bearer mytoken', conn._buffer)
        conn.putheader('IterHeader', 'IterA', 'IterB')
        self.assertIn(b'IterHeader: IterA\r\n\tIterB', conn._buffer)
        conn.putheader('LatinHeader', b'\xFF')
        self.assertIn(b'LatinHeader: \xFF', conn._buffer)
        conn.putheader('Utf8Header', b'\xc3\x80')
        self.assertIn(b'Utf8Header: \xc3\x80', conn._buffer)
        conn.putheader('C1-Control', b'next\x85line')
        self.assertIn(b'C1-Control: next\x85line', conn._buffer)
        conn.putheader('Embedded-Fold-Space', 'is\r\n allowed')
        self.assertIn(b'Embedded-Fold-Space: is\r\n allowed', conn._buffer)
        conn.putheader('Embedded-Fold-Tab', 'is\r\n\tallowed')
        self.assertIn(b'Embedded-Fold-Tab: is\r\n\tallowed', conn._buffer)
        conn.putheader('Key Space', 'value')
        self.assertIn(b'Key Space: value', conn._buffer)
        conn.putheader('KeySpace ', 'value')
        self.assertIn(b'KeySpace : value', conn._buffer)
        conn.putheader(b'Nonbreak\xa0Space', 'value')
        self.assertIn(b'Nonbreak\xa0Space: value', conn._buffer)
        conn.putheader(b'\xa0NonbreakSpace', 'value')
        self.assertIn(b'\xa0NonbreakSpace: value', conn._buffer)

    def test_ipv6host_header(self):
        # Default host header on IPv6 transaction should be wrapped by [] if
        # it is an IPv6 address
        expected = 'GET /foo HTTP/1.1\r\nHost: [2001::]:81\r\n' \
                   'Accept-Encoding: identity\r\n\r\n'
        conn = httplib.HTTPConnection('[2001::]:81')
        sock = FakeSocket('')
        conn.sock = sock
        conn.request('GET', '/foo')
        self.assertTrue(sock.data.startswith(expected))

        expected = 'GET /foo HTTP/1.1\r\nHost: [2001:102A::]\r\n' \
                   'Accept-Encoding: identity\r\n\r\n'
        conn = httplib.HTTPConnection('[2001:102A::]')
        sock = FakeSocket('')
        conn.sock = sock
        conn.request('GET', '/foo')
        self.assertTrue(sock.data.startswith(expected))

    def test_malformed_headers_coped_with(self):
        # Issue 19996
        body = "HTTP/1.1 200 OK\r\nFirst: val\r\n: nval\r\nSecond: val\r\n\r\n"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        resp.begin()

        self.assertEqual(resp.getheader('First'), 'val')
        self.assertEqual(resp.getheader('Second'), 'val')

    def test_malformed_truncation(self):
        # Other malformed header lines, especially without colons, used to
        # cause the rest of the header section to be truncated
        resp = (
            b'HTTP/1.1 200 OK\r\n'
            b'Public-Key-Pins: \n'
            b'pin-sha256="xxx=";\n'
            b'report-uri="https://..."\r\n'
            b'Transfer-Encoding: chunked\r\n'
            b'\r\n'
            b'4\r\nbody\r\n0\r\n\r\n'
        )
        resp = httplib.HTTPResponse(FakeSocket(resp))
        resp.begin()
        self.assertIsNotNone(resp.getheader('Public-Key-Pins'))
        self.assertEqual(resp.getheader('Transfer-Encoding'), 'chunked')
        self.assertEqual(resp.read(), b'body')

    def test_blank_line_forms(self):
        # Test that both CRLF and LF blank lines can terminate the header
        # section and start the body
        for blank in (b'\r\n', b'\n'):
            resp = b'HTTP/1.1 200 OK\r\n' b'Transfer-Encoding: chunked\r\n'
            resp += blank
            resp += b'4\r\nbody\r\n0\r\n\r\n'
            resp = httplib.HTTPResponse(FakeSocket(resp))
            resp.begin()
            self.assertEqual(resp.getheader('Transfer-Encoding'), 'chunked')
            self.assertEqual(resp.read(), b'body')

            resp = b'HTTP/1.0 200 OK\r\n' + blank + b'body'
            resp = httplib.HTTPResponse(FakeSocket(resp))
            resp.begin()
            self.assertEqual(resp.read(), b'body')

        # A blank line ending in CR is not treated as the end of the HTTP
        # header section, therefore header fields following it should be
        # parsed if possible
        resp = (
            b'HTTP/1.1 200 OK\r\n'
            b'\r'
            b'Name: value\r\n'
            b'Transfer-Encoding: chunked\r\n'
            b'\r\n'
            b'4\r\nbody\r\n0\r\n\r\n'
        )
        resp = httplib.HTTPResponse(FakeSocket(resp))
        resp.begin()
        self.assertEqual(resp.getheader('Transfer-Encoding'), 'chunked')
        self.assertEqual(resp.read(), b'body')

        # No header fields nor blank line
        resp = b'HTTP/1.0 200 OK\r\n'
        resp = httplib.HTTPResponse(FakeSocket(resp))
        resp.begin()
        self.assertEqual(resp.read(), b'')

    def test_from_line(self):
        # The parser handles "From" lines specially, so test this does not
        # affect parsing the rest of the header section
        resp = (
            b'HTTP/1.1 200 OK\r\n'
            b'From start\r\n'
            b' continued\r\n'
            b'Name: value\r\n'
            b'From middle\r\n'
            b' continued\r\n'
            b'Transfer-Encoding: chunked\r\n'
            b'From end\r\n'
            b'\r\n'
            b'4\r\nbody\r\n0\r\n\r\n'
        )
        resp = httplib.HTTPResponse(FakeSocket(resp))
        resp.begin()
        self.assertIsNotNone(resp.getheader('Name'))
        self.assertEqual(resp.getheader('Transfer-Encoding'), 'chunked')
        self.assertEqual(resp.read(), b'body')

        resp = (
            b'HTTP/1.0 200 OK\r\n'
            b'From alone\r\n'
            b'\r\n'
            b'body'
        )
        resp = httplib.HTTPResponse(FakeSocket(resp))
        resp.begin()
        self.assertEqual(resp.read(), b'body')

    def test_parse_all_octets(self):
        # Ensure no valid header field octet breaks the parser
        body = (
            b'HTTP/1.1 200 OK\r\n'
            b"!#$%&'*+-.^_`|~: value\r\n"  # Special token characters
            b'VCHAR: ' + bytearray(range(0x21, 0x7E + 1)) + b'\r\n'
            b'obs-text: ' + bytearray(range(0x80, 0xFF + 1)) + b'\r\n'
            b'obs-fold: text\r\n'
            b' folded with space\r\n'
            b'\tfolded with tab\r\n'
            b'Content-Length: 0\r\n'
            b'\r\n'
        )
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.getheader('Content-Length'), '0')
        self.assertEqual(resp.getheader("!#$%&'*+-.^_`|~"), 'value')
        vchar = ''.join(map(chr, range(0x21, 0x7E + 1)))
        self.assertEqual(resp.getheader('VCHAR'), vchar)
        self.assertIsNotNone(resp.getheader('obs-text'))
        folded = resp.getheader('obs-fold')
        self.assertTrue(folded.startswith('text'))
        self.assertIn(' folded with space', folded)
        self.assertTrue(folded.endswith('folded with tab'))

    def test_invalid_headers(self):
        conn = httplib.HTTPConnection('example.com')
        conn.sock = FakeSocket('')
        conn.putrequest('GET', '/')

        # http://tools.ietf.org/html/rfc7230#section-3.2.4, whitespace is no
        # longer allowed in header names
        cases = (
            (b'Invalid\r\nName', b'ValidValue'),
            (b'Invalid\rName', b'ValidValue'),
            (b'Invalid\nName', b'ValidValue'),
            (b'\r\nInvalidName', b'ValidValue'),
            (b'\rInvalidName', b'ValidValue'),
            (b'\nInvalidName', b'ValidValue'),
            (b' InvalidName', b'ValidValue'),
            (b'\tInvalidName', b'ValidValue'),
            (b'Invalid:Name', b'ValidValue'),
            (b':InvalidName', b'ValidValue'),
            (b'ValidName', b'Invalid\r\nValue'),
            (b'ValidName', b'Invalid\rValue'),
            (b'ValidName', b'Invalid\nValue'),
            (b'ValidName', b'InvalidValue\r\n'),
            (b'ValidName', b'InvalidValue\r'),
            (b'ValidName', b'InvalidValue\n'),
        )
        for name, value in cases:
            with self.assertRaisesRegexp(ValueError, 'Invalid header'):
                conn.putheader(name, value)


class BasicTest(TestCase):
    def test_status_lines(self):
        # Test HTTP status lines

        body = "HTTP/1.1 200 Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(0), '')  # Issue #20007
        self.assertFalse(resp.isclosed())
        self.assertEqual(resp.read(), 'Text')
        self.assertTrue(resp.isclosed())

        body = "HTTP/1.1 400.100 Not Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        self.assertRaises(httplib.BadStatusLine, resp.begin)

    def test_bad_status_repr(self):
        exc = httplib.BadStatusLine('')
        self.assertEqual(repr(exc), '''BadStatusLine("\'\'",)''')

    def test_partial_reads(self):
        # if we have a length, the system knows when to close itself
        # same behaviour than when we read the whole thing with read()
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 4\r\n\r\nText"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(2), 'Te')
        self.assertFalse(resp.isclosed())
        self.assertEqual(resp.read(2), 'xt')
        self.assertTrue(resp.isclosed())

    def test_partial_reads_no_content_length(self):
        # when no length is present, the socket should be gracefully closed when
        # all data was read
        body = "HTTP/1.1 200 Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(2), 'Te')
        self.assertFalse(resp.isclosed())
        self.assertEqual(resp.read(2), 'xt')
        self.assertEqual(resp.read(1), '')
        self.assertTrue(resp.isclosed())

    def test_partial_reads_incomplete_body(self):
        # if the server shuts down the connection before the whole
        # content-length is delivered, the socket is gracefully closed
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 10\r\n\r\nText"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(2), 'Te')
        self.assertFalse(resp.isclosed())
        self.assertEqual(resp.read(2), 'xt')
        self.assertEqual(resp.read(1), '')
        self.assertTrue(resp.isclosed())

    def test_host_port(self):
        # Check invalid host_port

        # Note that httplib does not accept user:password@ in the host-port.
        for hp in ("www.python.org:abc", "user:password@www.python.org"):
            self.assertRaises(httplib.InvalidURL, httplib.HTTP, hp)

        for hp, h, p in (("[fe80::207:e9ff:fe9b]:8000", "fe80::207:e9ff:fe9b",
                          8000),
                         ("www.python.org:80", "www.python.org", 80),
                         ("www.python.org", "www.python.org", 80),
                         ("www.python.org:", "www.python.org", 80),
                         ("[fe80::207:e9ff:fe9b]", "fe80::207:e9ff:fe9b", 80)):
            http = httplib.HTTP(hp)
            c = http._conn
            if h != c.host:
                self.fail("Host incorrectly parsed: %s != %s" % (h, c.host))
            if p != c.port:
                self.fail("Port incorrectly parsed: %s != %s" % (p, c.host))

    def test_response_headers(self):
        # test response with multiple message headers with the same field name.
        text = ('HTTP/1.1 200 OK\r\n'
                'Set-Cookie: Customer="WILE_E_COYOTE";'
                ' Version="1"; Path="/acme"\r\n'
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

    def test_too_many_headers(self):
        headers = '\r\n'.join('Header%d: foo' % i for i in xrange(200)) + '\r\n'
        text = ('HTTP/1.1 200 OK\r\n' + headers)
        s = FakeSocket(text)
        r = httplib.HTTPResponse(s)
        self.assertRaises(httplib.HTTPException, r.begin)

    def test_send_file(self):
        expected = 'GET /foo HTTP/1.1\r\nHost: example.com\r\n' \
                   'Accept-Encoding: identity\r\nContent-Length:'

        body = open(__file__, 'rb')
        conn = httplib.HTTPConnection('example.com')
        sock = FakeSocket(body)
        conn.sock = sock
        conn.request('GET', '/foo', body)
        self.assertTrue(sock.data.startswith(expected))
        self.assertIn('def test_send_file', sock.data)

    def test_send_tempfile(self):
        expected = ('GET /foo HTTP/1.1\r\nHost: example.com\r\n'
                    'Accept-Encoding: identity\r\nContent-Length: 9\r\n\r\n'
                    'fake\ndata')

        with tempfile.TemporaryFile() as body:
            body.write('fake\ndata')
            body.seek(0)

            conn = httplib.HTTPConnection('example.com')
            sock = FakeSocket(body)
            conn.sock = sock
            conn.request('GET', '/foo', body)
        self.assertEqual(sock.data, expected)

    def test_send(self):
        expected = 'this is a test this is only a test'
        conn = httplib.HTTPConnection('example.com')
        sock = FakeSocket(None)
        conn.sock = sock
        conn.send(expected)
        self.assertEqual(expected, sock.data)
        sock.data = ''
        conn.send(array.array('c', expected))
        self.assertEqual(expected, sock.data)
        sock.data = ''
        conn.send(StringIO.StringIO(expected))
        self.assertEqual(expected, sock.data)

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
        resp = httplib.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read(), 'hello world')
        resp.close()

        for x in ('', 'foo\r\n'):
            sock = FakeSocket(chunked_start + x)
            resp = httplib.HTTPResponse(sock, method="GET")
            resp.begin()
            try:
                resp.read()
            except httplib.IncompleteRead, i:
                self.assertEqual(i.partial, 'hello world')
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
        resp = httplib.HTTPResponse(sock, method="HEAD")
        resp.begin()
        self.assertEqual(resp.read(), '')
        self.assertEqual(resp.status, 200)
        self.assertEqual(resp.reason, 'OK')
        self.assertTrue(resp.isclosed())

    def test_negative_content_length(self):
        sock = FakeSocket('HTTP/1.1 200 OK\r\n'
                          'Content-Length: -1\r\n\r\nHello\r\n')
        resp = httplib.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read(), 'Hello\r\n')
        self.assertTrue(resp.isclosed())

    def test_incomplete_read(self):
        sock = FakeSocket('HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nHello\r\n')
        resp = httplib.HTTPResponse(sock, method="GET")
        resp.begin()
        try:
            resp.read()
        except httplib.IncompleteRead as i:
            self.assertEqual(i.partial, 'Hello\r\n')
            self.assertEqual(repr(i),
                             "IncompleteRead(7 bytes read, 3 more expected)")
            self.assertEqual(str(i),
                             "IncompleteRead(7 bytes read, 3 more expected)")
            self.assertTrue(resp.isclosed())
        else:
            self.fail('IncompleteRead expected')

    def test_epipe(self):
        sock = EPipeSocket(
            "HTTP/1.0 401 Authorization Required\r\n"
            "Content-type: text/html\r\n"
            "WWW-Authenticate: Basic realm=\"example\"\r\n",
            b"Content-Length")
        conn = httplib.HTTPConnection("example.com")
        conn.sock = sock
        self.assertRaises(socket.error,
                          lambda: conn.request("PUT", "/url", "body"))
        resp = conn.getresponse()
        self.assertEqual(401, resp.status)
        self.assertEqual("Basic realm=\"example\"",
                         resp.getheader("www-authenticate"))

    def test_filenoattr(self):
        # Just test the fileno attribute in the HTTPResponse Object.
        body = "HTTP/1.1 200 Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        self.assertTrue(hasattr(resp,'fileno'),
                'HTTPResponse should expose a fileno attribute')

    # Test lines overflowing the max line size (_MAXLINE in httplib)

    def test_overflowing_status_line(self):
        self.skipTest("disabled for HTTP 0.9 support")
        body = "HTTP/1.1 200 Ok" + "k" * 65536 + "\r\n"
        resp = httplib.HTTPResponse(FakeSocket(body))
        self.assertRaises((httplib.LineTooLong, httplib.BadStatusLine), resp.begin)

    def test_overflowing_header_line(self):
        body = (
            'HTTP/1.1 200 OK\r\n'
            'X-Foo: bar' + 'r' * 65536 + '\r\n\r\n'
        )
        resp = httplib.HTTPResponse(FakeSocket(body))
        self.assertRaises(httplib.LineTooLong, resp.begin)

    def test_overflowing_chunked_line(self):
        body = (
            'HTTP/1.1 200 OK\r\n'
            'Transfer-Encoding: chunked\r\n\r\n'
            + '0' * 65536 + 'a\r\n'
            'hello world\r\n'
            '0\r\n'
        )
        resp = httplib.HTTPResponse(FakeSocket(body))
        resp.begin()
        self.assertRaises(httplib.LineTooLong, resp.read)

    def test_early_eof(self):
        # Test httpresponse with no \r\n termination,
        body = "HTTP/1.1 200 Ok"
        sock = FakeSocket(body)
        resp = httplib.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(), '')
        self.assertTrue(resp.isclosed())

    def test_error_leak(self):
        # Test that the socket is not leaked if getresponse() fails
        conn = httplib.HTTPConnection('example.com')
        response = []
        class Response(httplib.HTTPResponse):
            def __init__(self, *pos, **kw):
                response.append(self)  # Avoid garbage collector closing the socket
                httplib.HTTPResponse.__init__(self, *pos, **kw)
        conn.response_class = Response
        conn.sock = FakeSocket('')  # Emulate server dropping connection
        conn.request('GET', '/')
        self.assertRaises(httplib.BadStatusLine, conn.getresponse)
        self.assertTrue(response)
        #self.assertTrue(response[0].closed)
        self.assertTrue(conn.sock.file_closed)

    def test_proxy_tunnel_without_status_line(self):
        # Issue 17849: If a proxy tunnel is created that does not return
        # a status code, fail.
        body = 'hello world'
        conn = httplib.HTTPConnection('example.com', strict=False)
        conn.set_tunnel('foo')
        conn.sock = FakeSocket(body)
        with self.assertRaisesRegexp(socket.error, "Invalid response"):
            conn._tunnel()

class OfflineTest(TestCase):
    def test_responses(self):
        self.assertEqual(httplib.responses[httplib.NOT_FOUND], "Not Found")


class TestServerMixin:
    """A limited socket server mixin.

    This is used by test cases for testing http connection end points.
    """
    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.port = test_support.bind_port(self.serv)
        self.source_port = test_support.find_unused_port()
        self.serv.listen(5)
        self.conn = None

    def tearDown(self):
        if self.conn:
            self.conn.close()
            self.conn = None
        self.serv.close()
        self.serv = None

class SourceAddressTest(TestServerMixin, TestCase):
    def testHTTPConnectionSourceAddress(self):
        self.conn = httplib.HTTPConnection(HOST, self.port,
                source_address=('', self.source_port))
        self.conn.connect()
        self.assertEqual(self.conn.sock.getsockname()[1], self.source_port)

    @unittest.skipIf(not hasattr(httplib, 'HTTPSConnection'),
                     'httplib.HTTPSConnection not defined')
    def testHTTPSConnectionSourceAddress(self):
        self.conn = httplib.HTTPSConnection(HOST, self.port,
                source_address=('', self.source_port))
        # We don't test anything here other than the constructor not barfing as
        # this code doesn't deal with setting up an active running SSL server
        # for an ssl_wrapped connect() to actually return from.


class HTTPTest(TestServerMixin, TestCase):
    def testHTTPConnection(self):
        self.conn = httplib.HTTP(host=HOST, port=self.port, strict=None)
        self.conn.connect()
        self.assertEqual(self.conn._conn.host, HOST)
        self.assertEqual(self.conn._conn.port, self.port)

    def testHTTPWithConnectHostPort(self):
        testhost = 'unreachable.test.domain'
        testport = '80'
        self.conn = httplib.HTTP(host=testhost, port=testport)
        self.conn.connect(host=HOST, port=self.port)
        self.assertNotEqual(self.conn._conn.host, testhost)
        self.assertNotEqual(self.conn._conn.port, testport)
        self.assertEqual(self.conn._conn.host, HOST)
        self.assertEqual(self.conn._conn.port, self.port)


class TimeoutTest(TestCase):
    PORT = None

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        TimeoutTest.PORT = test_support.bind_port(self.serv)
        self.serv.listen(5)

    def tearDown(self):
        self.serv.close()
        self.serv = None

    def testTimeoutAttribute(self):
        '''This will prove that the timeout gets through
        HTTPConnection and into the socket.
        '''
        # default -- use global socket timeout
        self.assertIsNone(socket.getdefaulttimeout())
        socket.setdefaulttimeout(30)
        try:
            httpConn = httplib.HTTPConnection(HOST, TimeoutTest.PORT)
            httpConn.connect()
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(httpConn.sock.gettimeout(), 30)
        httpConn.close()

        # no timeout -- do not use global socket default
        self.assertIsNone(socket.getdefaulttimeout())
        socket.setdefaulttimeout(30)
        try:
            httpConn = httplib.HTTPConnection(HOST, TimeoutTest.PORT,
                                              timeout=None)
            httpConn.connect()
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(httpConn.sock.gettimeout(), None)
        httpConn.close()

        # a value
        httpConn = httplib.HTTPConnection(HOST, TimeoutTest.PORT, timeout=30)
        httpConn.connect()
        self.assertEqual(httpConn.sock.gettimeout(), 30)
        httpConn.close()


class HTTPSTest(TestCase):

    def setUp(self):
        if not hasattr(httplib, 'HTTPSConnection'):
            self.skipTest('ssl support required')

    def make_server(self, certfile):
        from test.ssl_servers import make_https_server
        return make_https_server(self, certfile=certfile)

    def test_attributes(self):
        # simple test to check it's storing the timeout
        h = httplib.HTTPSConnection(HOST, TimeoutTest.PORT, timeout=30)
        self.assertEqual(h.timeout, 30)

    def test_networked(self):
        # Default settings: requires a valid cert from a trusted CA
        import ssl
        test_support.requires('network')
        with test_support.transient_internet('self-signed.pythontest.net'):
            h = httplib.HTTPSConnection('self-signed.pythontest.net', 443)
            with self.assertRaises(ssl.SSLError) as exc_info:
                h.request('GET', '/')
            self.assertEqual(exc_info.exception.reason, 'CERTIFICATE_VERIFY_FAILED')

    def test_networked_noverification(self):
        # Switch off cert verification
        import ssl
        test_support.requires('network')
        with test_support.transient_internet('self-signed.pythontest.net'):
            context = ssl._create_stdlib_context()
            h = httplib.HTTPSConnection('self-signed.pythontest.net', 443,
                                        context=context)
            h.request('GET', '/')
            resp = h.getresponse()
            self.assertIn('nginx', resp.getheader('server'))

    @test_support.system_must_validate_cert
    def test_networked_trusted_by_default_cert(self):
        # Default settings: requires a valid cert from a trusted CA
        test_support.requires('network')
        with test_support.transient_internet('www.python.org'):
            h = httplib.HTTPSConnection('www.python.org', 443)
            h.request('GET', '/')
            resp = h.getresponse()
            content_type = resp.getheader('content-type')
            self.assertIn('text/html', content_type)

    def test_networked_good_cert(self):
        # We feed the server's cert as a validating cert
        import ssl
        test_support.requires('network')
        with test_support.transient_internet('self-signed.pythontest.net'):
            context = ssl.SSLContext(ssl.PROTOCOL_TLS)
            context.verify_mode = ssl.CERT_REQUIRED
            context.load_verify_locations(CERT_selfsigned_pythontestdotnet)
            h = httplib.HTTPSConnection('self-signed.pythontest.net', 443, context=context)
            h.request('GET', '/')
            resp = h.getresponse()
            server_string = resp.getheader('server')
            self.assertIn('nginx', server_string)

    def test_networked_bad_cert(self):
        # We feed a "CA" cert that is unrelated to the server's cert
        import ssl
        test_support.requires('network')
        with test_support.transient_internet('self-signed.pythontest.net'):
            context = ssl.SSLContext(ssl.PROTOCOL_TLS)
            context.verify_mode = ssl.CERT_REQUIRED
            context.load_verify_locations(CERT_localhost)
            h = httplib.HTTPSConnection('self-signed.pythontest.net', 443, context=context)
            with self.assertRaises(ssl.SSLError) as exc_info:
                h.request('GET', '/')
            self.assertEqual(exc_info.exception.reason, 'CERTIFICATE_VERIFY_FAILED')

    def test_local_unknown_cert(self):
        # The custom cert isn't known to the default trust bundle
        import ssl
        server = self.make_server(CERT_localhost)
        h = httplib.HTTPSConnection('localhost', server.port)
        with self.assertRaises(ssl.SSLError) as exc_info:
            h.request('GET', '/')
        self.assertEqual(exc_info.exception.reason, 'CERTIFICATE_VERIFY_FAILED')

    def test_local_good_hostname(self):
        # The (valid) cert validates the HTTP hostname
        import ssl
        server = self.make_server(CERT_localhost)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS)
        context.verify_mode = ssl.CERT_REQUIRED
        context.load_verify_locations(CERT_localhost)
        h = httplib.HTTPSConnection('localhost', server.port, context=context)
        h.request('GET', '/nonexistent')
        resp = h.getresponse()
        self.assertEqual(resp.status, 404)

    def test_local_bad_hostname(self):
        # The (valid) cert doesn't validate the HTTP hostname
        import ssl
        server = self.make_server(CERT_fakehostname)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS)
        context.verify_mode = ssl.CERT_REQUIRED
        context.check_hostname = True
        context.load_verify_locations(CERT_fakehostname)
        h = httplib.HTTPSConnection('localhost', server.port, context=context)
        with self.assertRaises(ssl.CertificateError):
            h.request('GET', '/')
        h.close()
        # With context.check_hostname=False, the mismatching is ignored
        context.check_hostname = False
        h = httplib.HTTPSConnection('localhost', server.port, context=context)
        h.request('GET', '/nonexistent')
        resp = h.getresponse()
        self.assertEqual(resp.status, 404)

    def test_host_port(self):
        # Check invalid host_port

        for hp in ("www.python.org:abc", "user:password@www.python.org"):
            self.assertRaises(httplib.InvalidURL, httplib.HTTPSConnection, hp)

        for hp, h, p in (("[fe80::207:e9ff:fe9b]:8000",
                          "fe80::207:e9ff:fe9b", 8000),
                         ("www.python.org:443", "www.python.org", 443),
                         ("www.python.org:", "www.python.org", 443),
                         ("www.python.org", "www.python.org", 443),
                         ("[fe80::207:e9ff:fe9b]", "fe80::207:e9ff:fe9b", 443),
                         ("[fe80::207:e9ff:fe9b]:", "fe80::207:e9ff:fe9b",
                             443)):
            c = httplib.HTTPSConnection(hp)
            self.assertEqual(h, c.host)
            self.assertEqual(p, c.port)


class TunnelTests(TestCase):
    def test_connect(self):
        response_text = (
            'HTTP/1.0 200 OK\r\n\r\n'   # Reply to CONNECT
            'HTTP/1.1 200 OK\r\n'       # Reply to HEAD
            'Content-Length: 42\r\n\r\n'
        )

        def create_connection(address, timeout=None, source_address=None):
            return FakeSocket(response_text, host=address[0], port=address[1])

        conn = httplib.HTTPConnection('proxy.com')
        conn._create_connection = create_connection

        # Once connected, we should not be able to tunnel anymore
        conn.connect()
        self.assertRaises(RuntimeError, conn.set_tunnel, 'destination.com')

        # But if close the connection, we are good.
        conn.close()
        conn.set_tunnel('destination.com')
        conn.request('HEAD', '/', '')

        self.assertEqual(conn.sock.host, 'proxy.com')
        self.assertEqual(conn.sock.port, 80)
        self.assertIn('CONNECT destination.com', conn.sock.data)
        # issue22095
        self.assertNotIn('Host: destination.com:None', conn.sock.data)
        self.assertIn('Host: destination.com', conn.sock.data)

        self.assertNotIn('Host: proxy.com', conn.sock.data)

        conn.close()

        conn.request('PUT', '/', '')
        self.assertEqual(conn.sock.host, 'proxy.com')
        self.assertEqual(conn.sock.port, 80)
        self.assertTrue('CONNECT destination.com' in conn.sock.data)
        self.assertTrue('Host: destination.com' in conn.sock.data)


@test_support.reap_threads
def test_main(verbose=None):
    test_support.run_unittest(HeaderTests, OfflineTest, BasicTest, TimeoutTest,
                              HTTPTest, HTTPSTest, SourceAddressTest,
                              TunnelTests)

if __name__ == '__main__':
    test_main()
