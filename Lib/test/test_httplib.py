import enum
import errno
from http import client, HTTPStatus
import io
import itertools
import os
import array
import re
import socket
import threading

import unittest
from unittest import mock
TestCase = unittest.TestCase

from test import support
from test.support import os_helper
from test.support import socket_helper

support.requires_working_socket(module=True)

here = os.path.dirname(__file__)
# Self-signed cert file for 'localhost'
CERT_localhost = os.path.join(here, 'certdata', 'keycert.pem')
# Self-signed cert file for 'fakehostname'
CERT_fakehostname = os.path.join(here, 'certdata', 'keycert2.pem')
# Self-signed cert file for self-signed.pythontest.net
CERT_selfsigned_pythontestdotnet = os.path.join(
    here, 'certdata', 'selfsigned_pythontestdotnet.pem',
)

# constants for testing chunked encoding
chunked_start = (
    'HTTP/1.1 200 OK\r\n'
    'Transfer-Encoding: chunked\r\n\r\n'
    'a\r\n'
    'hello worl\r\n'
    '3\r\n'
    'd! \r\n'
    '8\r\n'
    'and now \r\n'
    '22\r\n'
    'for something completely different\r\n'
)
chunked_expected = b'hello world! and now for something completely different'
chunk_extension = ";foo=bar"
last_chunk = "0\r\n"
last_chunk_extended = "0" + chunk_extension + "\r\n"
trailers = "X-Dummy: foo\r\nX-Dumm2: bar\r\n"
chunked_end = "\r\n"

HOST = socket_helper.HOST

class FakeSocket:
    def __init__(self, text, fileclass=io.BytesIO, host=None, port=None):
        if isinstance(text, str):
            text = text.encode("ascii")
        self.text = text
        self.fileclass = fileclass
        self.data = b''
        self.sendall_calls = 0
        self.file_closed = False
        self.host = host
        self.port = port

    def sendall(self, data):
        self.sendall_calls += 1
        self.data += data

    def makefile(self, mode, bufsize=None):
        if mode != 'r' and mode != 'rb':
            raise client.UnimplementedFileMode()
        # keep the file around so we can check how much was read from it
        self.file = self.fileclass(self.text)
        self.file.close = self.file_close #nerf close ()
        return self.file

    def file_close(self):
        self.file_closed = True

    def close(self):
        pass

    def setsockopt(self, level, optname, value):
        pass

class EPipeSocket(FakeSocket):

    def __init__(self, text, pipe_trigger):
        # When sendall() is called with pipe_trigger, raise EPIPE.
        FakeSocket.__init__(self, text)
        self.pipe_trigger = pipe_trigger

    def sendall(self, data):
        if self.pipe_trigger in data:
            raise OSError(errno.EPIPE, "gotcha")
        self.data += data

    def close(self):
        pass

class NoEOFBytesIO(io.BytesIO):
    """Like BytesIO, but raises AssertionError on EOF.

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

class FakeSocketHTTPConnection(client.HTTPConnection):
    """HTTPConnection subclass using FakeSocket; counts connect() calls"""

    def __init__(self, *args):
        self.connections = 0
        super().__init__('example.com')
        self.fake_socket_args = args
        self._create_connection = self.create_connection

    def connect(self):
        """Count the number of times connect() is invoked"""
        self.connections += 1
        return super().connect()

    def create_connection(self, *pos, **kw):
        return FakeSocket(*self.fake_socket_args)

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

    def test_content_length_0(self):

        class ContentLengthChecker(list):
            def __init__(self):
                list.__init__(self)
                self.content_length = None
            def append(self, item):
                kv = item.split(b':', 1)
                if len(kv) > 1 and kv[0].lower() == b'content-length':
                    self.content_length = kv[1].strip()
                list.append(self, item)

        # Here, we're testing that methods expecting a body get a
        # content-length set to zero if the body is empty (either None or '')
        bodies = (None, '')
        methods_with_body = ('PUT', 'POST', 'PATCH')
        for method, body in itertools.product(methods_with_body, bodies):
            conn = client.HTTPConnection('example.com')
            conn.sock = FakeSocket(None)
            conn._buffer = ContentLengthChecker()
            conn.request(method, '/', body)
            self.assertEqual(
                conn._buffer.content_length, b'0',
                'Header Content-Length incorrect on {}'.format(method)
            )

        # For these methods, we make sure that content-length is not set when
        # the body is None because it might cause unexpected behaviour on the
        # server.
        methods_without_body = (
             'GET', 'CONNECT', 'DELETE', 'HEAD', 'OPTIONS', 'TRACE',
        )
        for method in methods_without_body:
            conn = client.HTTPConnection('example.com')
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
            conn = client.HTTPConnection('example.com')
            conn.sock = FakeSocket(None)
            conn._buffer = ContentLengthChecker()
            conn.request(method, '/', '')
            self.assertEqual(
                conn._buffer.content_length, b'0',
                'Header Content-Length incorrect on {}'.format(method)
            )

        # If the body is set, make sure Content-Length is set.
        for method in itertools.chain(methods_without_body, methods_with_body):
            conn = client.HTTPConnection('example.com')
            conn.sock = FakeSocket(None)
            conn._buffer = ContentLengthChecker()
            conn.request(method, '/', ' ')
            self.assertEqual(
                conn._buffer.content_length, b'1',
                'Header Content-Length incorrect on {}'.format(method)
            )

    def test_putheader(self):
        conn = client.HTTPConnection('example.com')
        conn.sock = FakeSocket(None)
        conn.putrequest('GET','/')
        conn.putheader('Content-length', 42)
        self.assertIn(b'Content-length: 42', conn._buffer)

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
        expected = b'GET /foo HTTP/1.1\r\nHost: [2001::]:81\r\n' \
                   b'Accept-Encoding: identity\r\n\r\n'
        conn = client.HTTPConnection('[2001::]:81')
        sock = FakeSocket('')
        conn.sock = sock
        conn.request('GET', '/foo')
        self.assertStartsWith(sock.data, expected)

        expected = b'GET /foo HTTP/1.1\r\nHost: [2001:102A::]\r\n' \
                   b'Accept-Encoding: identity\r\n\r\n'
        conn = client.HTTPConnection('[2001:102A::]')
        sock = FakeSocket('')
        conn.sock = sock
        conn.request('GET', '/foo')
        self.assertStartsWith(sock.data, expected)

        expected = b'GET /foo HTTP/1.1\r\nHost: [fe80::]\r\n' \
                   b'Accept-Encoding: identity\r\n\r\n'
        conn = client.HTTPConnection('[fe80::%2]')
        sock = FakeSocket('')
        conn.sock = sock
        conn.request('GET', '/foo')
        self.assertStartsWith(sock.data, expected)

        expected = b'GET /foo HTTP/1.1\r\nHost: [fe80::]:81\r\n' \
                   b'Accept-Encoding: identity\r\n\r\n'
        conn = client.HTTPConnection('[fe80::%2]:81')
        sock = FakeSocket('')
        conn.sock = sock
        conn.request('GET', '/foo')
        self.assertStartsWith(sock.data, expected)

    def test_malformed_headers_coped_with(self):
        # Issue 19996
        body = "HTTP/1.1 200 OK\r\nFirst: val\r\n: nval\r\nSecond: val\r\n\r\n"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()

        self.assertEqual(resp.getheader('First'), 'val')
        self.assertEqual(resp.getheader('Second'), 'val')

    def test_parse_all_octets(self):
        # Ensure no valid header field octet breaks the parser
        body = (
            b'HTTP/1.1 200 OK\r\n'
            b"!#$%&'*+-.^_`|~: value\r\n"  # Special token characters
            b'VCHAR: ' + bytes(range(0x21, 0x7E + 1)) + b'\r\n'
            b'obs-text: ' + bytes(range(0x80, 0xFF + 1)) + b'\r\n'
            b'obs-fold: text\r\n'
            b' folded with space\r\n'
            b'\tfolded with tab\r\n'
            b'Content-Length: 0\r\n'
            b'\r\n'
        )
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.getheader('Content-Length'), '0')
        self.assertEqual(resp.msg['Content-Length'], '0')
        self.assertEqual(resp.getheader("!#$%&'*+-.^_`|~"), 'value')
        self.assertEqual(resp.msg["!#$%&'*+-.^_`|~"], 'value')
        vchar = ''.join(map(chr, range(0x21, 0x7E + 1)))
        self.assertEqual(resp.getheader('VCHAR'), vchar)
        self.assertEqual(resp.msg['VCHAR'], vchar)
        self.assertIsNotNone(resp.getheader('obs-text'))
        self.assertIn('obs-text', resp.msg)
        for folded in (resp.getheader('obs-fold'), resp.msg['obs-fold']):
            self.assertStartsWith(folded, 'text')
            self.assertIn(' folded with space', folded)
            self.assertEndsWith(folded, 'folded with tab')

    def test_invalid_headers(self):
        conn = client.HTTPConnection('example.com')
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
            with self.subTest((name, value)):
                with self.assertRaisesRegex(ValueError, 'Invalid header'):
                    conn.putheader(name, value)

    def test_headers_debuglevel(self):
        body = (
            b'HTTP/1.1 200 OK\r\n'
            b'First: val\r\n'
            b'Second: val1\r\n'
            b'Second: val2\r\n'
        )
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock, debuglevel=1)
        with support.captured_stdout() as output:
            resp.begin()
        lines = output.getvalue().splitlines()
        self.assertEqual(lines[0], "reply: 'HTTP/1.1 200 OK\\r\\n'")
        self.assertEqual(lines[1], "header: First: val")
        self.assertEqual(lines[2], "header: Second: val1")
        self.assertEqual(lines[3], "header: Second: val2")


class HttpMethodTests(TestCase):
    def test_invalid_method_names(self):
        methods = (
            'GET\r',
            'POST\n',
            'PUT\n\r',
            'POST\nValue',
            'POST\nHOST:abc',
            'GET\nrHost:abc\n',
            'POST\rRemainder:\r',
            'GET\rHOST:\n',
            '\nPUT'
        )

        for method in methods:
            with self.assertRaisesRegex(
                    ValueError, "method can't contain control characters"):
                conn = client.HTTPConnection('example.com')
                conn.sock = FakeSocket(None)
                conn.request(method=method, url="/")


class TransferEncodingTest(TestCase):
    expected_body = b"It's just a flesh wound"

    def test_endheaders_chunked(self):
        conn = client.HTTPConnection('example.com')
        conn.sock = FakeSocket(b'')
        conn.putrequest('POST', '/')
        conn.endheaders(self._make_body(), encode_chunked=True)

        _, _, body = self._parse_request(conn.sock.data)
        body = self._parse_chunked(body)
        self.assertEqual(body, self.expected_body)

    def test_explicit_headers(self):
        # explicit chunked
        conn = client.HTTPConnection('example.com')
        conn.sock = FakeSocket(b'')
        # this shouldn't actually be automatically chunk-encoded because the
        # calling code has explicitly stated that it's taking care of it
        conn.request(
            'POST', '/', self._make_body(), {'Transfer-Encoding': 'chunked'})

        _, headers, body = self._parse_request(conn.sock.data)
        self.assertNotIn('content-length', [k.lower() for k in headers.keys()])
        self.assertEqual(headers['Transfer-Encoding'], 'chunked')
        self.assertEqual(body, self.expected_body)

        # explicit chunked, string body
        conn = client.HTTPConnection('example.com')
        conn.sock = FakeSocket(b'')
        conn.request(
            'POST', '/', self.expected_body.decode('latin-1'),
            {'Transfer-Encoding': 'chunked'})

        _, headers, body = self._parse_request(conn.sock.data)
        self.assertNotIn('content-length', [k.lower() for k in headers.keys()])
        self.assertEqual(headers['Transfer-Encoding'], 'chunked')
        self.assertEqual(body, self.expected_body)

        # User-specified TE, but request() does the chunk encoding
        conn = client.HTTPConnection('example.com')
        conn.sock = FakeSocket(b'')
        conn.request('POST', '/',
            headers={'Transfer-Encoding': 'gzip, chunked'},
            encode_chunked=True,
            body=self._make_body())
        _, headers, body = self._parse_request(conn.sock.data)
        self.assertNotIn('content-length', [k.lower() for k in headers])
        self.assertEqual(headers['Transfer-Encoding'], 'gzip, chunked')
        self.assertEqual(self._parse_chunked(body), self.expected_body)

    def test_request(self):
        for empty_lines in (False, True,):
            conn = client.HTTPConnection('example.com')
            conn.sock = FakeSocket(b'')
            conn.request(
                'POST', '/', self._make_body(empty_lines=empty_lines))

            _, headers, body = self._parse_request(conn.sock.data)
            body = self._parse_chunked(body)
            self.assertEqual(body, self.expected_body)
            self.assertEqual(headers['Transfer-Encoding'], 'chunked')

            # Content-Length and Transfer-Encoding SHOULD not be sent in the
            # same request
            self.assertNotIn('content-length', [k.lower() for k in headers])

    def test_empty_body(self):
        # Zero-length iterable should be treated like any other iterable
        conn = client.HTTPConnection('example.com')
        conn.sock = FakeSocket(b'')
        conn.request('POST', '/', ())
        _, headers, body = self._parse_request(conn.sock.data)
        self.assertEqual(headers['Transfer-Encoding'], 'chunked')
        self.assertNotIn('content-length', [k.lower() for k in headers])
        self.assertEqual(body, b"0\r\n\r\n")

    def _make_body(self, empty_lines=False):
        lines = self.expected_body.split(b' ')
        for idx, line in enumerate(lines):
            # for testing handling empty lines
            if empty_lines and idx % 2:
                yield b''
            if idx < len(lines) - 1:
                yield line + b' '
            else:
                yield line

    def _parse_request(self, data):
        lines = data.split(b'\r\n')
        request = lines[0]
        headers = {}
        n = 1
        while n < len(lines) and len(lines[n]) > 0:
            key, val = lines[n].split(b':')
            key = key.decode('latin-1').strip()
            headers[key] = val.decode('latin-1').strip()
            n += 1

        return request, headers, b'\r\n'.join(lines[n + 1:])

    def _parse_chunked(self, data):
        body = []
        trailers = {}
        n = 0
        lines = data.split(b'\r\n')
        # parse body
        while True:
            size, chunk = lines[n:n+2]
            size = int(size, 16)

            if size == 0:
                n += 1
                break

            self.assertEqual(size, len(chunk))
            body.append(chunk)

            n += 2
            # we /should/ hit the end chunk, but check against the size of
            # lines so we're not stuck in an infinite loop should we get
            # malformed data
            if n > len(lines):
                break

        return b''.join(body)


class BasicTest(TestCase):
    def test_dir_with_added_behavior_on_status(self):
        # see issue40084
        self.assertTrue({'description', 'name', 'phrase', 'value'} <= set(dir(HTTPStatus(404))))

    def test_simple_httpstatus(self):
        class CheckedHTTPStatus(enum.IntEnum):
            """HTTP status codes and reason phrases

            Status codes from the following RFCs are all observed:

                * RFC 7231: Hypertext Transfer Protocol (HTTP/1.1), obsoletes 2616
                * RFC 6585: Additional HTTP Status Codes
                * RFC 3229: Delta encoding in HTTP
                * RFC 4918: HTTP Extensions for WebDAV, obsoletes 2518
                * RFC 5842: Binding Extensions to WebDAV
                * RFC 7238: Permanent Redirect
                * RFC 2295: Transparent Content Negotiation in HTTP
                * RFC 2774: An HTTP Extension Framework
                * RFC 7725: An HTTP Status Code to Report Legal Obstacles
                * RFC 7540: Hypertext Transfer Protocol Version 2 (HTTP/2)
                * RFC 2324: Hyper Text Coffee Pot Control Protocol (HTCPCP/1.0)
                * RFC 8297: An HTTP Status Code for Indicating Hints
                * RFC 8470: Using Early Data in HTTP
            """
            def __new__(cls, value, phrase, description=''):
                obj = int.__new__(cls, value)
                obj._value_ = value

                obj.phrase = phrase
                obj.description = description
                return obj

            @property
            def is_informational(self):
                return 100 <= self <= 199

            @property
            def is_success(self):
                return 200 <= self <= 299

            @property
            def is_redirection(self):
                return 300 <= self <= 399

            @property
            def is_client_error(self):
                return 400 <= self <= 499

            @property
            def is_server_error(self):
                return 500 <= self <= 599

            # informational
            CONTINUE = 100, 'Continue', 'Request received, please continue'
            SWITCHING_PROTOCOLS = (101, 'Switching Protocols',
                    'Switching to new protocol; obey Upgrade header')
            PROCESSING = 102, 'Processing', 'Server is processing the request'
            EARLY_HINTS = (103, 'Early Hints',
                'Headers sent to prepare for the response')
            # success
            OK = 200, 'OK', 'Request fulfilled, document follows'
            CREATED = 201, 'Created', 'Document created, URL follows'
            ACCEPTED = (202, 'Accepted',
                'Request accepted, processing continues off-line')
            NON_AUTHORITATIVE_INFORMATION = (203,
                'Non-Authoritative Information', 'Request fulfilled from cache')
            NO_CONTENT = 204, 'No Content', 'Request fulfilled, nothing follows'
            RESET_CONTENT = 205, 'Reset Content', 'Clear input form for further input'
            PARTIAL_CONTENT = 206, 'Partial Content', 'Partial content follows'
            MULTI_STATUS = (207, 'Multi-Status',
                'Response contains multiple statuses in the body')
            ALREADY_REPORTED = (208, 'Already Reported',
                'Operation has already been reported')
            IM_USED = 226, 'IM Used', 'Request completed using instance manipulations'
            # redirection
            MULTIPLE_CHOICES = (300, 'Multiple Choices',
                'Object has several resources -- see URI list')
            MOVED_PERMANENTLY = (301, 'Moved Permanently',
                'Object moved permanently -- see URI list')
            FOUND = 302, 'Found', 'Object moved temporarily -- see URI list'
            SEE_OTHER = 303, 'See Other', 'Object moved -- see Method and URL list'
            NOT_MODIFIED = (304, 'Not Modified',
                'Document has not changed since given time')
            USE_PROXY = (305, 'Use Proxy',
                'You must use proxy specified in Location to access this resource')
            TEMPORARY_REDIRECT = (307, 'Temporary Redirect',
                'Object moved temporarily -- see URI list')
            PERMANENT_REDIRECT = (308, 'Permanent Redirect',
                'Object moved permanently -- see URI list')
            # client error
            BAD_REQUEST = (400, 'Bad Request',
                'Bad request syntax or unsupported method')
            UNAUTHORIZED = (401, 'Unauthorized',
                'No permission -- see authorization schemes')
            PAYMENT_REQUIRED = (402, 'Payment Required',
                'No payment -- see charging schemes')
            FORBIDDEN = (403, 'Forbidden',
                'Request forbidden -- authorization will not help')
            NOT_FOUND = (404, 'Not Found',
                'Nothing matches the given URI')
            METHOD_NOT_ALLOWED = (405, 'Method Not Allowed',
                'Specified method is invalid for this resource')
            NOT_ACCEPTABLE = (406, 'Not Acceptable',
                'URI not available in preferred format')
            PROXY_AUTHENTICATION_REQUIRED = (407,
                'Proxy Authentication Required',
                'You must authenticate with this proxy before proceeding')
            REQUEST_TIMEOUT = (408, 'Request Timeout',
                'Request timed out; try again later')
            CONFLICT = 409, 'Conflict', 'Request conflict'
            GONE = (410, 'Gone',
                'URI no longer exists and has been permanently removed')
            LENGTH_REQUIRED = (411, 'Length Required',
                'Client must specify Content-Length')
            PRECONDITION_FAILED = (412, 'Precondition Failed',
                'Precondition in headers is false')
            CONTENT_TOO_LARGE = (413, 'Content Too Large',
                'Content is too large')
            REQUEST_ENTITY_TOO_LARGE = CONTENT_TOO_LARGE
            URI_TOO_LONG = (414, 'URI Too Long', 'URI is too long')
            REQUEST_URI_TOO_LONG = URI_TOO_LONG
            UNSUPPORTED_MEDIA_TYPE = (415, 'Unsupported Media Type',
                'Entity body in unsupported format')
            RANGE_NOT_SATISFIABLE = (416,
                'Range Not Satisfiable',
                'Cannot satisfy request range')
            REQUESTED_RANGE_NOT_SATISFIABLE = RANGE_NOT_SATISFIABLE
            EXPECTATION_FAILED = (417, 'Expectation Failed',
                'Expect condition could not be satisfied')
            IM_A_TEAPOT = (418, 'I\'m a Teapot',
                'Server refuses to brew coffee because it is a teapot')
            MISDIRECTED_REQUEST = (421, 'Misdirected Request',
                'Server is not able to produce a response')
            UNPROCESSABLE_CONTENT = (422, 'Unprocessable Content',
                'Server is not able to process the contained instructions')
            UNPROCESSABLE_ENTITY = UNPROCESSABLE_CONTENT
            LOCKED = 423, 'Locked', 'Resource of a method is locked'
            FAILED_DEPENDENCY = (424, 'Failed Dependency',
                'Dependent action of the request failed')
            TOO_EARLY = (425, 'Too Early',
                'Server refuses to process a request that might be replayed')
            UPGRADE_REQUIRED = (426, 'Upgrade Required',
                'Server refuses to perform the request using the current protocol')
            PRECONDITION_REQUIRED = (428, 'Precondition Required',
                'The origin server requires the request to be conditional')
            TOO_MANY_REQUESTS = (429, 'Too Many Requests',
                'The user has sent too many requests in '
                'a given amount of time ("rate limiting")')
            REQUEST_HEADER_FIELDS_TOO_LARGE = (431,
                'Request Header Fields Too Large',
                'The server is unwilling to process the request because its header '
                'fields are too large')
            UNAVAILABLE_FOR_LEGAL_REASONS = (451,
                'Unavailable For Legal Reasons',
                'The server is denying access to the '
                'resource as a consequence of a legal demand')
            # server errors
            INTERNAL_SERVER_ERROR = (500, 'Internal Server Error',
                'Server got itself in trouble')
            NOT_IMPLEMENTED = (501, 'Not Implemented',
                'Server does not support this operation')
            BAD_GATEWAY = (502, 'Bad Gateway',
                'Invalid responses from another server/proxy')
            SERVICE_UNAVAILABLE = (503, 'Service Unavailable',
                'The server cannot process the request due to a high load')
            GATEWAY_TIMEOUT = (504, 'Gateway Timeout',
                'The gateway server did not receive a timely response')
            HTTP_VERSION_NOT_SUPPORTED = (505, 'HTTP Version Not Supported',
                'Cannot fulfill request')
            VARIANT_ALSO_NEGOTIATES = (506, 'Variant Also Negotiates',
                'Server has an internal configuration error')
            INSUFFICIENT_STORAGE = (507, 'Insufficient Storage',
                'Server is not able to store the representation')
            LOOP_DETECTED = (508, 'Loop Detected',
                'Server encountered an infinite loop while processing a request')
            NOT_EXTENDED = (510, 'Not Extended',
                'Request does not meet the resource access policy')
            NETWORK_AUTHENTICATION_REQUIRED = (511,
                'Network Authentication Required',
                'The client needs to authenticate to gain network access')
        enum._test_simple_enum(CheckedHTTPStatus, HTTPStatus)

    def test_httpstatus_range(self):
        """Checks that the statuses are in the 100-599 range"""

        for member in HTTPStatus.__members__.values():
            self.assertGreaterEqual(member, 100)
            self.assertLessEqual(member, 599)

    def test_httpstatus_category(self):
        """Checks that the statuses belong to the standard categories"""

        categories = (
            ((100, 199), "is_informational"),
            ((200, 299), "is_success"),
            ((300, 399), "is_redirection"),
            ((400, 499), "is_client_error"),
            ((500, 599), "is_server_error"),
        )
        for member in HTTPStatus.__members__.values():
            for (lower, upper), category in categories:
                category_indicator = getattr(member, category)
                if lower <= member <= upper:
                    self.assertTrue(category_indicator)
                else:
                    self.assertFalse(category_indicator)

    def test_status_lines(self):
        # Test HTTP status lines

        body = "HTTP/1.1 200 Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(0), b'')  # Issue #20007
        self.assertFalse(resp.isclosed())
        self.assertFalse(resp.closed)
        self.assertEqual(resp.read(), b"Text")
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

        body = "HTTP/1.1 400.100 Not Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        self.assertRaises(client.BadStatusLine, resp.begin)

    def test_bad_status_repr(self):
        exc = client.BadStatusLine('')
        self.assertEqual(repr(exc), '''BadStatusLine("''")''')

    def test_partial_reads(self):
        # if we have Content-Length, HTTPResponse knows when to close itself,
        # the same behaviour as when we read the whole thing with read()
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 4\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(2), b'Te')
        self.assertFalse(resp.isclosed())
        self.assertEqual(resp.read(2), b'xt')
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_mixed_reads(self):
        # readline() should update the remaining length, so that read() knows
        # how much data is left and does not raise IncompleteRead
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 13\r\n\r\nText\r\nAnother"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.readline(), b'Text\r\n')
        self.assertFalse(resp.isclosed())
        self.assertEqual(resp.read(), b'Another')
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_partial_readintos(self):
        # if we have Content-Length, HTTPResponse knows when to close itself,
        # the same behaviour as when we read the whole thing with read()
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 4\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        b = bytearray(2)
        n = resp.readinto(b)
        self.assertEqual(n, 2)
        self.assertEqual(bytes(b), b'Te')
        self.assertFalse(resp.isclosed())
        n = resp.readinto(b)
        self.assertEqual(n, 2)
        self.assertEqual(bytes(b), b'xt')
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_partial_reads_past_end(self):
        # if we have Content-Length, clip reads to the end
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 4\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(10), b'Text')
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_partial_readintos_past_end(self):
        # if we have Content-Length, clip readintos to the end
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 4\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        b = bytearray(10)
        n = resp.readinto(b)
        self.assertEqual(n, 4)
        self.assertEqual(bytes(b)[:4], b'Text')
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_partial_reads_no_content_length(self):
        # when no length is present, the socket should be gracefully closed when
        # all data was read
        body = "HTTP/1.1 200 Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(2), b'Te')
        self.assertFalse(resp.isclosed())
        self.assertEqual(resp.read(2), b'xt')
        self.assertEqual(resp.read(1), b'')
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_partial_readintos_no_content_length(self):
        # when no length is present, the socket should be gracefully closed when
        # all data was read
        body = "HTTP/1.1 200 Ok\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        b = bytearray(2)
        n = resp.readinto(b)
        self.assertEqual(n, 2)
        self.assertEqual(bytes(b), b'Te')
        self.assertFalse(resp.isclosed())
        n = resp.readinto(b)
        self.assertEqual(n, 2)
        self.assertEqual(bytes(b), b'xt')
        n = resp.readinto(b)
        self.assertEqual(n, 0)
        self.assertTrue(resp.isclosed())

    def test_partial_reads_incomplete_body(self):
        # if the server shuts down the connection before the whole
        # content-length is delivered, the socket is gracefully closed
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 10\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(2), b'Te')
        self.assertFalse(resp.isclosed())
        self.assertEqual(resp.read(2), b'xt')
        self.assertEqual(resp.read(1), b'')
        self.assertTrue(resp.isclosed())

    def test_partial_readintos_incomplete_body(self):
        # if the server shuts down the connection before the whole
        # content-length is delivered, the socket is gracefully closed
        body = "HTTP/1.1 200 Ok\r\nContent-Length: 10\r\n\r\nText"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        b = bytearray(2)
        n = resp.readinto(b)
        self.assertEqual(n, 2)
        self.assertEqual(bytes(b), b'Te')
        self.assertFalse(resp.isclosed())
        n = resp.readinto(b)
        self.assertEqual(n, 2)
        self.assertEqual(bytes(b), b'xt')
        n = resp.readinto(b)
        self.assertEqual(n, 0)
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_host_port(self):
        # Check invalid host_port

        for hp in ("www.python.org:abc", "user:password@www.python.org"):
            self.assertRaises(client.InvalidURL, client.HTTPConnection, hp)

        for hp, h, p in (("[fe80::207:e9ff:fe9b]:8000",
                          "fe80::207:e9ff:fe9b", 8000),
                         ("www.python.org:80", "www.python.org", 80),
                         ("www.python.org:", "www.python.org", 80),
                         ("www.python.org", "www.python.org", 80),
                         ("[fe80::207:e9ff:fe9b]", "fe80::207:e9ff:fe9b", 80),
                         ("[fe80::207:e9ff:fe9b]:", "fe80::207:e9ff:fe9b", 80)):
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
            NoEOFBytesIO)
        resp = client.HTTPResponse(sock, method="HEAD")
        resp.begin()
        if resp.read():
            self.fail("Did not expect response from HEAD request")

    def test_readinto_head(self):
        # Test that the library doesn't attempt to read any data
        # from a HEAD request.  (Tickles SF bug #622042.)
        sock = FakeSocket(
            'HTTP/1.1 200 OK\r\n'
            'Content-Length: 14432\r\n'
            '\r\n',
            NoEOFBytesIO)
        resp = client.HTTPResponse(sock, method="HEAD")
        resp.begin()
        b = bytearray(5)
        if resp.readinto(b) != 0:
            self.fail("Did not expect response from HEAD request")
        self.assertEqual(bytes(b), b'\x00'*5)

    def test_too_many_headers(self):
        headers = '\r\n'.join('Header%d: foo' % i
                              for i in range(client._MAXHEADERS + 1)) + '\r\n'
        text = ('HTTP/1.1 200 OK\r\n' + headers)
        s = FakeSocket(text)
        r = client.HTTPResponse(s)
        self.assertRaisesRegex(client.HTTPException,
                               r"got more than \d+ headers", r.begin)

    def test_send_file(self):
        expected = (b'GET /foo HTTP/1.1\r\nHost: example.com\r\n'
                    b'Accept-Encoding: identity\r\n'
                    b'Transfer-Encoding: chunked\r\n'
                    b'\r\n')

        with open(__file__, 'rb') as body:
            conn = client.HTTPConnection('example.com')
            sock = FakeSocket(body)
            conn.sock = sock
            conn.request('GET', '/foo', body)
            self.assertStartsWith(sock.data, expected)

    def test_send(self):
        expected = b'this is a test this is only a test'
        conn = client.HTTPConnection('example.com')
        sock = FakeSocket(None)
        conn.sock = sock
        conn.send(expected)
        self.assertEqual(expected, sock.data)
        sock.data = b''
        conn.send(array.array('b', expected))
        self.assertEqual(expected, sock.data)
        sock.data = b''
        conn.send(io.BytesIO(expected))
        self.assertEqual(expected, sock.data)

    def test_send_updating_file(self):
        def data():
            yield 'data'
            yield None
            yield 'data_two'

        class UpdatingFile(io.TextIOBase):
            mode = 'r'
            d = data()
            def read(self, blocksize=-1):
                return next(self.d)

        expected = b'data'

        conn = client.HTTPConnection('example.com')
        sock = FakeSocket("")
        conn.sock = sock
        conn.send(UpdatingFile())
        self.assertEqual(sock.data, expected)


    def test_send_iter(self):
        expected = b'GET /foo HTTP/1.1\r\nHost: example.com\r\n' \
                   b'Accept-Encoding: identity\r\nContent-Length: 11\r\n' \
                   b'\r\nonetwothree'

        def body():
            yield b"one"
            yield b"two"
            yield b"three"

        conn = client.HTTPConnection('example.com')
        sock = FakeSocket("")
        conn.sock = sock
        conn.request('GET', '/foo', body(), {'Content-Length': '11'})
        self.assertEqual(sock.data, expected)

    def test_blocksize_request(self):
        """Check that request() respects the configured block size."""
        blocksize = 8  # For easy debugging.
        conn = client.HTTPConnection('example.com', blocksize=blocksize)
        sock = FakeSocket(None)
        conn.sock = sock
        expected = b"a" * blocksize + b"b"
        conn.request("PUT", "/", io.BytesIO(expected), {"Content-Length": "9"})
        self.assertEqual(sock.sendall_calls, 3)
        body = sock.data.split(b"\r\n\r\n", 1)[1]
        self.assertEqual(body, expected)

    def test_blocksize_send(self):
        """Check that send() respects the configured block size."""
        blocksize = 8  # For easy debugging.
        conn = client.HTTPConnection('example.com', blocksize=blocksize)
        sock = FakeSocket(None)
        conn.sock = sock
        expected = b"a" * blocksize + b"b"
        conn.send(io.BytesIO(expected))
        self.assertEqual(sock.sendall_calls, 2)
        self.assertEqual(sock.data, expected)

    def test_send_type_error(self):
        # See: Issue #12676
        conn = client.HTTPConnection('example.com')
        conn.sock = FakeSocket('')
        with self.assertRaises(TypeError):
            conn.request('POST', 'test', conn)

    def test_chunked(self):
        expected = chunked_expected
        sock = FakeSocket(chunked_start + last_chunk + chunked_end)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read(), expected)
        resp.close()

        # Explicit full read
        for n in (-123, -1, None):
            with self.subTest('full read', n=n):
                sock = FakeSocket(chunked_start + last_chunk + chunked_end)
                resp = client.HTTPResponse(sock, method="GET")
                resp.begin()
                self.assertTrue(resp.chunked)
                self.assertEqual(resp.read(n), expected)
                resp.close()

        # Read first chunk
        with self.subTest('read1(-1)'):
            sock = FakeSocket(chunked_start + last_chunk + chunked_end)
            resp = client.HTTPResponse(sock, method="GET")
            resp.begin()
            self.assertTrue(resp.chunked)
            self.assertEqual(resp.read1(-1), b"hello worl")
            resp.close()

        # Various read sizes
        for n in range(1, 12):
            sock = FakeSocket(chunked_start + last_chunk + chunked_end)
            resp = client.HTTPResponse(sock, method="GET")
            resp.begin()
            self.assertEqual(resp.read(n) + resp.read(n) + resp.read(), expected)
            resp.close()

        for x in ('', 'foo\r\n'):
            sock = FakeSocket(chunked_start + x)
            resp = client.HTTPResponse(sock, method="GET")
            resp.begin()
            try:
                resp.read()
            except client.IncompleteRead as i:
                self.assertEqual(i.partial, expected)
                expected_message = 'IncompleteRead(%d bytes read)' % len(expected)
                self.assertEqual(repr(i), expected_message)
                self.assertEqual(str(i), expected_message)
            else:
                self.fail('IncompleteRead expected')
            finally:
                resp.close()

    def test_readinto_chunked(self):

        expected = chunked_expected
        nexpected = len(expected)
        b = bytearray(128)

        sock = FakeSocket(chunked_start + last_chunk + chunked_end)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        n = resp.readinto(b)
        self.assertEqual(b[:nexpected], expected)
        self.assertEqual(n, nexpected)
        resp.close()

        # Various read sizes
        for n in range(1, 12):
            sock = FakeSocket(chunked_start + last_chunk + chunked_end)
            resp = client.HTTPResponse(sock, method="GET")
            resp.begin()
            m = memoryview(b)
            i = resp.readinto(m[0:n])
            i += resp.readinto(m[i:n + i])
            i += resp.readinto(m[i:])
            self.assertEqual(b[:nexpected], expected)
            self.assertEqual(i, nexpected)
            resp.close()

        for x in ('', 'foo\r\n'):
            sock = FakeSocket(chunked_start + x)
            resp = client.HTTPResponse(sock, method="GET")
            resp.begin()
            try:
                n = resp.readinto(b)
            except client.IncompleteRead as i:
                self.assertEqual(i.partial, expected)
                expected_message = 'IncompleteRead(%d bytes read)' % len(expected)
                self.assertEqual(repr(i), expected_message)
                self.assertEqual(str(i), expected_message)
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
        sock = FakeSocket(chunked_start + last_chunk + chunked_end)
        resp = client.HTTPResponse(sock, method="HEAD")
        resp.begin()
        self.assertEqual(resp.read(), b'')
        self.assertEqual(resp.status, 200)
        self.assertEqual(resp.reason, 'OK')
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_readinto_chunked_head(self):
        chunked_start = (
            'HTTP/1.1 200 OK\r\n'
            'Transfer-Encoding: chunked\r\n\r\n'
            'a\r\n'
            'hello world\r\n'
            '1\r\n'
            'd\r\n'
        )
        sock = FakeSocket(chunked_start + last_chunk + chunked_end)
        resp = client.HTTPResponse(sock, method="HEAD")
        resp.begin()
        b = bytearray(5)
        n = resp.readinto(b)
        self.assertEqual(n, 0)
        self.assertEqual(bytes(b), b'\x00'*5)
        self.assertEqual(resp.status, 200)
        self.assertEqual(resp.reason, 'OK')
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_negative_content_length(self):
        sock = FakeSocket(
            'HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\nHello\r\n')
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read(), b'Hello\r\n')
        self.assertTrue(resp.isclosed())

    def test_incomplete_read(self):
        sock = FakeSocket('HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nHello\r\n')
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        try:
            resp.read()
        except client.IncompleteRead as i:
            self.assertEqual(i.partial, b'Hello\r\n')
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
        conn = client.HTTPConnection("example.com")
        conn.sock = sock
        self.assertRaises(OSError,
                          lambda: conn.request("PUT", "/url", "body"))
        resp = conn.getresponse()
        self.assertEqual(401, resp.status)
        self.assertEqual("Basic realm=\"example\"",
                         resp.getheader("www-authenticate"))

    # Test lines overflowing the max line size (_MAXLINE in http.client)

    def test_overflowing_status_line(self):
        body = "HTTP/1.1 200 Ok" + "k" * 65536 + "\r\n"
        resp = client.HTTPResponse(FakeSocket(body))
        self.assertRaises((client.LineTooLong, client.BadStatusLine), resp.begin)

    def test_overflowing_header_line(self):
        body = (
            'HTTP/1.1 200 OK\r\n'
            'X-Foo: bar' + 'r' * 65536 + '\r\n\r\n'
        )
        resp = client.HTTPResponse(FakeSocket(body))
        self.assertRaises(client.LineTooLong, resp.begin)

    def test_overflowing_header_limit_after_100(self):
        body = (
            'HTTP/1.1 100 OK\r\n'
            'r\n' * 32768
        )
        resp = client.HTTPResponse(FakeSocket(body))
        with self.assertRaises(client.HTTPException) as cm:
            resp.begin()
        # We must assert more because other reasonable errors that we
        # do not want can also be HTTPException derived.
        self.assertIn('got more than ', str(cm.exception))
        self.assertIn('headers', str(cm.exception))

    def test_overflowing_chunked_line(self):
        body = (
            'HTTP/1.1 200 OK\r\n'
            'Transfer-Encoding: chunked\r\n\r\n'
            + '0' * 65536 + 'a\r\n'
            'hello world\r\n'
            '0\r\n'
            '\r\n'
        )
        resp = client.HTTPResponse(FakeSocket(body))
        resp.begin()
        self.assertRaises(client.LineTooLong, resp.read)

    def test_early_eof(self):
        # Test httpresponse with no \r\n termination,
        body = "HTTP/1.1 200 Ok"
        sock = FakeSocket(body)
        resp = client.HTTPResponse(sock)
        resp.begin()
        self.assertEqual(resp.read(), b'')
        self.assertTrue(resp.isclosed())
        self.assertFalse(resp.closed)
        resp.close()
        self.assertTrue(resp.closed)

    def test_error_leak(self):
        # Test that the socket is not leaked if getresponse() fails
        conn = client.HTTPConnection('example.com')
        response = None
        class Response(client.HTTPResponse):
            def __init__(self, *pos, **kw):
                nonlocal response
                response = self  # Avoid garbage collector closing the socket
                client.HTTPResponse.__init__(self, *pos, **kw)
        conn.response_class = Response
        conn.sock = FakeSocket('Invalid status line')
        conn.request('GET', '/')
        self.assertRaises(client.BadStatusLine, conn.getresponse)
        self.assertTrue(response.closed)
        self.assertTrue(conn.sock.file_closed)

    def test_chunked_extension(self):
        extra = '3;foo=bar\r\n' + 'abc\r\n'
        expected = chunked_expected + b'abc'

        sock = FakeSocket(chunked_start + extra + last_chunk_extended + chunked_end)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read(), expected)
        resp.close()

    def test_chunked_missing_end(self):
        """some servers may serve up a short chunked encoding stream"""
        expected = chunked_expected
        sock = FakeSocket(chunked_start + last_chunk)  #no terminating crlf
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read(), expected)
        resp.close()

    def test_chunked_trailers(self):
        """See that trailers are read and ignored"""
        expected = chunked_expected
        sock = FakeSocket(chunked_start + last_chunk + trailers + chunked_end)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read(), expected)
        # we should have reached the end of the file
        self.assertEqual(sock.file.read(), b"") #we read to the end
        resp.close()

    def test_chunked_sync(self):
        """Check that we don't read past the end of the chunked-encoding stream"""
        expected = chunked_expected
        extradata = "extradata"
        sock = FakeSocket(chunked_start + last_chunk + trailers + chunked_end + extradata)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read(), expected)
        # the file should now have our extradata ready to be read
        self.assertEqual(sock.file.read(), extradata.encode("ascii")) #we read to the end
        resp.close()

    def test_content_length_sync(self):
        """Check that we don't read past the end of the Content-Length stream"""
        extradata = b"extradata"
        expected = b"Hello123\r\n"
        sock = FakeSocket(b'HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n' + expected + extradata)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read(), expected)
        # the file should now have our extradata ready to be read
        self.assertEqual(sock.file.read(), extradata) #we read to the end
        resp.close()

    def test_readlines_content_length(self):
        extradata = b"extradata"
        expected = b"Hello123\r\n"
        sock = FakeSocket(b'HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n' + expected + extradata)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.readlines(2000), [expected])
        # the file should now have our extradata ready to be read
        self.assertEqual(sock.file.read(), extradata) #we read to the end
        resp.close()

    def test_read1_content_length(self):
        extradata = b"extradata"
        expected = b"Hello123\r\n"
        sock = FakeSocket(b'HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n' + expected + extradata)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read1(2000), expected)
        # the file should now have our extradata ready to be read
        self.assertEqual(sock.file.read(), extradata) #we read to the end
        resp.close()

    def test_readline_bound_content_length(self):
        extradata = b"extradata"
        expected = b"Hello123\r\n"
        sock = FakeSocket(b'HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n' + expected + extradata)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.readline(10), expected)
        self.assertEqual(resp.readline(10), b"")
        # the file should now have our extradata ready to be read
        self.assertEqual(sock.file.read(), extradata) #we read to the end
        resp.close()

    def test_read1_bound_content_length(self):
        extradata = b"extradata"
        expected = b"Hello123\r\n"
        sock = FakeSocket(b'HTTP/1.1 200 OK\r\nContent-Length: 30\r\n\r\n' + expected*3 + extradata)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        self.assertEqual(resp.read1(20), expected*2)
        self.assertEqual(resp.read(), expected)
        # the file should now have our extradata ready to be read
        self.assertEqual(sock.file.read(), extradata) #we read to the end
        resp.close()

    def test_response_fileno(self):
        # Make sure fd returned by fileno is valid.
        serv = socket.create_server((HOST, 0))
        self.addCleanup(serv.close)

        result = None
        def run_server():
            [conn, address] = serv.accept()
            with conn, conn.makefile("rb") as reader:
                # Read the request header until a blank line
                while True:
                    line = reader.readline()
                    if not line.rstrip(b"\r\n"):
                        break
                conn.sendall(b"HTTP/1.1 200 Connection established\r\n\r\n")
                nonlocal result
                result = reader.read()

        thread = threading.Thread(target=run_server)
        thread.start()
        self.addCleanup(thread.join, float(1))
        conn = client.HTTPConnection(*serv.getsockname())
        conn.request("CONNECT", "dummy:1234")
        response = conn.getresponse()
        try:
            self.assertEqual(response.status, client.OK)
            s = socket.socket(fileno=response.fileno())
            try:
                s.sendall(b"proxied data\n")
            finally:
                s.detach()
        finally:
            response.close()
            conn.close()
        thread.join()
        self.assertEqual(result, b"proxied data\n")

    def test_putrequest_override_domain_validation(self):
        """
        It should be possible to override the default validation
        behavior in putrequest (bpo-38216).
        """
        class UnsafeHTTPConnection(client.HTTPConnection):
            def _validate_path(self, url):
                pass

        conn = UnsafeHTTPConnection('example.com')
        conn.sock = FakeSocket('')
        conn.putrequest('GET', '/\x00')

    def test_putrequest_override_host_validation(self):
        class UnsafeHTTPConnection(client.HTTPConnection):
            def _validate_host(self, url):
                pass

        conn = UnsafeHTTPConnection('example.com\r\n')
        conn.sock = FakeSocket('')
        # set skip_host so a ValueError is not raised upon adding the
        # invalid URL as the value of the "Host:" header
        conn.putrequest('GET', '/', skip_host=1)

    def test_putrequest_override_encoding(self):
        """
        It should be possible to override the default encoding
        to transmit bytes in another encoding even if invalid
        (bpo-36274).
        """
        class UnsafeHTTPConnection(client.HTTPConnection):
            def _encode_request(self, str_url):
                return str_url.encode('utf-8')

        conn = UnsafeHTTPConnection('example.com')
        conn.sock = FakeSocket('')
        conn.putrequest('GET', '/☃')


class ExtendedReadTest(TestCase):
    """
    Test peek(), read1(), readline()
    """
    lines = (
        'HTTP/1.1 200 OK\r\n'
        '\r\n'
        'hello world!\n'
        'and now \n'
        'for something completely different\n'
        'foo'
        )
    lines_expected = lines[lines.find('hello'):].encode("ascii")
    lines_chunked = (
        'HTTP/1.1 200 OK\r\n'
        'Transfer-Encoding: chunked\r\n\r\n'
        'a\r\n'
        'hello worl\r\n'
        '3\r\n'
        'd!\n\r\n'
        '9\r\n'
        'and now \n\r\n'
        '23\r\n'
        'for something completely different\n\r\n'
        '3\r\n'
        'foo\r\n'
        '0\r\n' # terminating chunk
        '\r\n'  # end of trailers
    )

    def setUp(self):
        sock = FakeSocket(self.lines)
        resp = client.HTTPResponse(sock, method="GET")
        resp.begin()
        resp.fp = io.BufferedReader(resp.fp)
        self.resp = resp



    def test_peek(self):
        resp = self.resp
        # patch up the buffered peek so that it returns not too much stuff
        oldpeek = resp.fp.peek
        def mypeek(n=-1):
            p = oldpeek(n)
            if n >= 0:
                return p[:n]
            return p[:10]
        resp.fp.peek = mypeek

        all = []
        while True:
            # try a short peek
            p = resp.peek(3)
            if p:
                self.assertGreater(len(p), 0)
                # then unbounded peek
                p2 = resp.peek()
                self.assertGreaterEqual(len(p2), len(p))
                self.assertStartsWith(p2, p)
                next = resp.read(len(p2))
                self.assertEqual(next, p2)
            else:
                next = resp.read()
                self.assertFalse(next)
            all.append(next)
            if not next:
                break
        self.assertEqual(b"".join(all), self.lines_expected)

    def test_readline(self):
        resp = self.resp
        self._verify_readline(self.resp.readline, self.lines_expected)

    def test_readline_without_limit(self):
        self._verify_readline(self.resp.readline, self.lines_expected, limit=-1)

    def _verify_readline(self, readline, expected, limit=5):
        all = []
        while True:
            # short readlines
            line = readline(limit)
            if line and line != b"foo":
                if len(line) < 5:
                    self.assertEndsWith(line, b"\n")
            all.append(line)
            if not line:
                break
        self.assertEqual(b"".join(all), expected)
        self.assertTrue(self.resp.isclosed())

    def test_read1(self):
        resp = self.resp
        def r():
            res = resp.read1(4)
            self.assertLessEqual(len(res), 4)
            return res
        readliner = Readliner(r)
        self._verify_readline(readliner.readline, self.lines_expected)

    def test_read1_unbounded(self):
        resp = self.resp
        all = []
        while True:
            data = resp.read1()
            if not data:
                break
            all.append(data)
        self.assertEqual(b"".join(all), self.lines_expected)
        self.assertTrue(resp.isclosed())

    def test_read1_bounded(self):
        resp = self.resp
        all = []
        while True:
            data = resp.read1(10)
            if not data:
                break
            self.assertLessEqual(len(data), 10)
            all.append(data)
        self.assertEqual(b"".join(all), self.lines_expected)
        self.assertTrue(resp.isclosed())

    def test_read1_0(self):
        self.assertEqual(self.resp.read1(0), b"")
        self.assertFalse(self.resp.isclosed())

    def test_peek_0(self):
        p = self.resp.peek(0)
        self.assertLessEqual(0, len(p))


class ExtendedReadTestContentLengthKnown(ExtendedReadTest):
    _header, _body = ExtendedReadTest.lines.split('\r\n\r\n', 1)
    lines = _header + f'\r\nContent-Length: {len(_body)}\r\n\r\n' + _body


class ExtendedReadTestChunked(ExtendedReadTest):
    """
    Test peek(), read1(), readline() in chunked mode
    """
    lines = (
        'HTTP/1.1 200 OK\r\n'
        'Transfer-Encoding: chunked\r\n\r\n'
        'a\r\n'
        'hello worl\r\n'
        '3\r\n'
        'd!\n\r\n'
        '9\r\n'
        'and now \n\r\n'
        '23\r\n'
        'for something completely different\n\r\n'
        '3\r\n'
        'foo\r\n'
        '0\r\n' # terminating chunk
        '\r\n'  # end of trailers
    )


class Readliner:
    """
    a simple readline class that uses an arbitrary read function and buffering
    """
    def __init__(self, readfunc):
        self.readfunc = readfunc
        self.remainder = b""

    def readline(self, limit):
        data = []
        datalen = 0
        read = self.remainder
        try:
            while True:
                idx = read.find(b'\n')
                if idx != -1:
                    break
                if datalen + len(read) >= limit:
                    idx = limit - datalen - 1
                # read more data
                data.append(read)
                read = self.readfunc()
                if not read:
                    idx = 0 #eof condition
                    break
            idx += 1
            data.append(read[:idx])
            self.remainder = read[idx:]
            return b"".join(data)
        except:
            self.remainder = b"".join(data)
            raise


class OfflineTest(TestCase):
    def test_all(self):
        # Documented objects defined in the module should be in __all__
        expected = {"responses"}  # Allowlist documented dict() object
        # HTTPMessage, parse_headers(), and the HTTP status code constants are
        # intentionally omitted for simplicity
        denylist = {"HTTPMessage", "parse_headers"}
        for name in dir(client):
            if name.startswith("_") or name in denylist:
                continue
            module_object = getattr(client, name)
            if getattr(module_object, "__module__", None) == "http.client":
                expected.add(name)
        self.assertCountEqual(client.__all__, expected)

    def test_responses(self):
        self.assertEqual(client.responses[client.NOT_FOUND], "Not Found")

    def test_client_constants(self):
        # Make sure we don't break backward compatibility with 3.4
        expected = [
            'CONTINUE',
            'SWITCHING_PROTOCOLS',
            'PROCESSING',
            'OK',
            'CREATED',
            'ACCEPTED',
            'NON_AUTHORITATIVE_INFORMATION',
            'NO_CONTENT',
            'RESET_CONTENT',
            'PARTIAL_CONTENT',
            'MULTI_STATUS',
            'IM_USED',
            'MULTIPLE_CHOICES',
            'MOVED_PERMANENTLY',
            'FOUND',
            'SEE_OTHER',
            'NOT_MODIFIED',
            'USE_PROXY',
            'TEMPORARY_REDIRECT',
            'BAD_REQUEST',
            'UNAUTHORIZED',
            'PAYMENT_REQUIRED',
            'FORBIDDEN',
            'NOT_FOUND',
            'METHOD_NOT_ALLOWED',
            'NOT_ACCEPTABLE',
            'PROXY_AUTHENTICATION_REQUIRED',
            'REQUEST_TIMEOUT',
            'CONFLICT',
            'GONE',
            'LENGTH_REQUIRED',
            'PRECONDITION_FAILED',
            'CONTENT_TOO_LARGE',
            'REQUEST_ENTITY_TOO_LARGE',
            'URI_TOO_LONG',
            'REQUEST_URI_TOO_LONG',
            'UNSUPPORTED_MEDIA_TYPE',
            'RANGE_NOT_SATISFIABLE',
            'REQUESTED_RANGE_NOT_SATISFIABLE',
            'EXPECTATION_FAILED',
            'IM_A_TEAPOT',
            'MISDIRECTED_REQUEST',
            'UNPROCESSABLE_CONTENT',
            'UNPROCESSABLE_ENTITY',
            'LOCKED',
            'FAILED_DEPENDENCY',
            'UPGRADE_REQUIRED',
            'PRECONDITION_REQUIRED',
            'TOO_MANY_REQUESTS',
            'REQUEST_HEADER_FIELDS_TOO_LARGE',
            'UNAVAILABLE_FOR_LEGAL_REASONS',
            'INTERNAL_SERVER_ERROR',
            'NOT_IMPLEMENTED',
            'BAD_GATEWAY',
            'SERVICE_UNAVAILABLE',
            'GATEWAY_TIMEOUT',
            'HTTP_VERSION_NOT_SUPPORTED',
            'INSUFFICIENT_STORAGE',
            'NOT_EXTENDED',
            'NETWORK_AUTHENTICATION_REQUIRED',
            'EARLY_HINTS',
            'TOO_EARLY'
        ]
        for const in expected:
            with self.subTest(constant=const):
                self.assertHasAttr(client, const)


class SourceAddressTest(TestCase):
    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.port = socket_helper.bind_port(self.serv)
        self.source_port = socket_helper.find_unused_port()
        self.serv.listen()
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
        # We don't test anything here other than the constructor not barfing as
        # this code doesn't deal with setting up an active running SSL server
        # for an ssl_wrapped connect() to actually return from.


class TimeoutTest(TestCase):
    PORT = None

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        TimeoutTest.PORT = socket_helper.bind_port(self.serv)
        self.serv.listen()

    def tearDown(self):
        self.serv.close()
        self.serv = None

    def testTimeoutAttribute(self):
        # This will prove that the timeout gets through HTTPConnection
        # and into the socket.

        # default -- use global socket timeout
        self.assertIsNone(socket.getdefaulttimeout())
        socket.setdefaulttimeout(30)
        try:
            httpConn = client.HTTPConnection(HOST, TimeoutTest.PORT)
            httpConn.connect()
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(httpConn.sock.gettimeout(), 30)
        httpConn.close()

        # no timeout -- do not use global socket default
        self.assertIsNone(socket.getdefaulttimeout())
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


class PersistenceTest(TestCase):

    def test_reuse_reconnect(self):
        # Should reuse or reconnect depending on header from server
        tests = (
            ('1.0', '', False),
            ('1.0', 'Connection: keep-alive\r\n', True),
            ('1.1', '', True),
            ('1.1', 'Connection: close\r\n', False),
            ('1.0', 'Connection: keep-ALIVE\r\n', True),
            ('1.1', 'Connection: cloSE\r\n', False),
        )
        for version, header, reuse in tests:
            with self.subTest(version=version, header=header):
                msg = (
                    'HTTP/{} 200 OK\r\n'
                    '{}'
                    'Content-Length: 12\r\n'
                    '\r\n'
                    'Dummy body\r\n'
                ).format(version, header)
                conn = FakeSocketHTTPConnection(msg)
                self.assertIsNone(conn.sock)
                conn.request('GET', '/open-connection')
                with conn.getresponse() as response:
                    self.assertEqual(conn.sock is None, not reuse)
                    response.read()
                self.assertEqual(conn.sock is None, not reuse)
                self.assertEqual(conn.connections, 1)
                conn.request('GET', '/subsequent-request')
                self.assertEqual(conn.connections, 1 if reuse else 2)

    def test_disconnected(self):

        def make_reset_reader(text):
            """Return BufferedReader that raises ECONNRESET at EOF"""
            stream = io.BytesIO(text)
            def readinto(buffer):
                size = io.BytesIO.readinto(stream, buffer)
                if size == 0:
                    raise ConnectionResetError()
                return size
            stream.readinto = readinto
            return io.BufferedReader(stream)

        tests = (
            (io.BytesIO, client.RemoteDisconnected),
            (make_reset_reader, ConnectionResetError),
        )
        for stream_factory, exception in tests:
            with self.subTest(exception=exception):
                conn = FakeSocketHTTPConnection(b'', stream_factory)
                conn.request('GET', '/eof-response')
                self.assertRaises(exception, conn.getresponse)
                self.assertIsNone(conn.sock)
                # HTTPConnection.connect() should be automatically invoked
                conn.request('GET', '/reconnect')
                self.assertEqual(conn.connections, 2)

    def test_100_close(self):
        conn = FakeSocketHTTPConnection(
            b'HTTP/1.1 100 Continue\r\n'
            b'\r\n'
            # Missing final response
        )
        conn.request('GET', '/', headers={'Expect': '100-continue'})
        self.assertRaises(client.RemoteDisconnected, conn.getresponse)
        self.assertIsNone(conn.sock)
        conn.request('GET', '/reconnect')
        self.assertEqual(conn.connections, 2)


class HTTPSTest(TestCase):

    def setUp(self):
        if not hasattr(client, 'HTTPSConnection'):
            self.skipTest('ssl support required')

    def make_server(self, certfile):
        from test.ssl_servers import make_https_server
        return make_https_server(self, certfile=certfile)

    def test_attributes(self):
        # simple test to check it's storing the timeout
        h = client.HTTPSConnection(HOST, TimeoutTest.PORT, timeout=30)
        self.assertEqual(h.timeout, 30)

    def test_networked(self):
        # Default settings: requires a valid cert from a trusted CA
        import ssl
        support.requires('network')
        with socket_helper.transient_internet('self-signed.pythontest.net'):
            h = client.HTTPSConnection('self-signed.pythontest.net', 443)
            with self.assertRaises(ssl.SSLError) as exc_info:
                h.request('GET', '/')
            self.assertEqual(exc_info.exception.reason, 'CERTIFICATE_VERIFY_FAILED')

    def test_networked_noverification(self):
        # Switch off cert verification
        import ssl
        support.requires('network')
        with socket_helper.transient_internet('self-signed.pythontest.net'):
            context = ssl._create_unverified_context()
            h = client.HTTPSConnection('self-signed.pythontest.net', 443,
                                       context=context)
            h.request('GET', '/')
            resp = h.getresponse()
            h.close()
            self.assertIn('nginx', resp.getheader('server'))
            resp.close()

    @support.system_must_validate_cert
    def test_networked_trusted_by_default_cert(self):
        # Default settings: requires a valid cert from a trusted CA
        support.requires('network')
        with socket_helper.transient_internet('www.python.org'):
            h = client.HTTPSConnection('www.python.org', 443)
            h.request('GET', '/')
            resp = h.getresponse()
            content_type = resp.getheader('content-type')
            resp.close()
            h.close()
            self.assertIn('text/html', content_type)

    def test_networked_good_cert(self):
        # We feed the server's cert as a validating cert
        import ssl
        support.requires('network')
        selfsigned_pythontestdotnet = 'self-signed.pythontest.net'
        with socket_helper.transient_internet(selfsigned_pythontestdotnet):
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
            self.assertEqual(context.verify_mode, ssl.CERT_REQUIRED)
            self.assertEqual(context.check_hostname, True)
            context.load_verify_locations(CERT_selfsigned_pythontestdotnet)
            try:
                h = client.HTTPSConnection(selfsigned_pythontestdotnet, 443,
                                           context=context)
                h.request('GET', '/')
                resp = h.getresponse()
            except ssl.SSLError as ssl_err:
                ssl_err_str = str(ssl_err)
                # In the error message of [SSL: CERTIFICATE_VERIFY_FAILED] on
                # modern Linux distros (Debian Buster, etc) default OpenSSL
                # configurations it'll fail saying "key too weak" until we
                # address https://bugs.python.org/issue36816 to use a proper
                # key size on self-signed.pythontest.net.
                if re.search(r'(?i)key.too.weak', ssl_err_str):
                    raise unittest.SkipTest(
                        f'Got {ssl_err_str} trying to connect '
                        f'to {selfsigned_pythontestdotnet}. '
                        'See https://bugs.python.org/issue36816.')
                raise
            server_string = resp.getheader('server')
            resp.close()
            h.close()
            self.assertIn('nginx', server_string)

    @support.requires_resource('walltime')
    def test_networked_bad_cert(self):
        # We feed a "CA" cert that is unrelated to the server's cert
        import ssl
        support.requires('network')
        with socket_helper.transient_internet('self-signed.pythontest.net'):
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
            context.load_verify_locations(CERT_localhost)
            h = client.HTTPSConnection('self-signed.pythontest.net', 443, context=context)
            with self.assertRaises(ssl.SSLError) as exc_info:
                h.request('GET', '/')
            self.assertEqual(exc_info.exception.reason, 'CERTIFICATE_VERIFY_FAILED')

    def test_local_unknown_cert(self):
        # The custom cert isn't known to the default trust bundle
        import ssl
        server = self.make_server(CERT_localhost)
        h = client.HTTPSConnection('localhost', server.port)
        with self.assertRaises(ssl.SSLError) as exc_info:
            h.request('GET', '/')
        self.assertEqual(exc_info.exception.reason, 'CERTIFICATE_VERIFY_FAILED')

    def test_local_good_hostname(self):
        # The (valid) cert validates the HTTPS hostname
        import ssl
        server = self.make_server(CERT_localhost)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.load_verify_locations(CERT_localhost)
        h = client.HTTPSConnection('localhost', server.port, context=context)
        self.addCleanup(h.close)
        h.request('GET', '/nonexistent')
        resp = h.getresponse()
        self.addCleanup(resp.close)
        self.assertEqual(resp.status, 404)

    def test_local_bad_hostname(self):
        # The (valid) cert doesn't validate the HTTPS hostname
        import ssl
        server = self.make_server(CERT_fakehostname)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.load_verify_locations(CERT_fakehostname)
        h = client.HTTPSConnection('localhost', server.port, context=context)
        with self.assertRaises(ssl.CertificateError):
            h.request('GET', '/')

        # Same with explicit context.check_hostname=True
        context.check_hostname = True
        h = client.HTTPSConnection('localhost', server.port, context=context)
        with self.assertRaises(ssl.CertificateError):
            h.request('GET', '/')

        # With context.check_hostname=False, the mismatching is ignored
        context.check_hostname = False
        h = client.HTTPSConnection('localhost', server.port, context=context)
        h.request('GET', '/nonexistent')
        resp = h.getresponse()
        resp.close()
        h.close()
        self.assertEqual(resp.status, 404)

    @unittest.skipIf(not hasattr(client, 'HTTPSConnection'),
                     'http.client.HTTPSConnection not available')
    def test_host_port(self):
        # Check invalid host_port

        for hp in ("www.python.org:abc", "user:password@www.python.org"):
            self.assertRaises(client.InvalidURL, client.HTTPSConnection, hp)

        for hp, h, p in (("[fe80::207:e9ff:fe9b]:8000",
                          "fe80::207:e9ff:fe9b", 8000),
                         ("www.python.org:443", "www.python.org", 443),
                         ("www.python.org:", "www.python.org", 443),
                         ("www.python.org", "www.python.org", 443),
                         ("[fe80::207:e9ff:fe9b]", "fe80::207:e9ff:fe9b", 443),
                         ("[fe80::207:e9ff:fe9b]:", "fe80::207:e9ff:fe9b",
                             443)):
            c = client.HTTPSConnection(hp)
            self.assertEqual(h, c.host)
            self.assertEqual(p, c.port)

    def test_tls13_pha(self):
        import ssl
        if not ssl.HAS_TLSv1_3 or not ssl.HAS_PHA:
            self.skipTest('TLS 1.3 PHA support required')
        # just check status of PHA flag
        h = client.HTTPSConnection('localhost', 443)
        self.assertTrue(h._context.post_handshake_auth)

        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertFalse(context.post_handshake_auth)
        h = client.HTTPSConnection('localhost', 443, context=context)
        self.assertIs(h._context, context)
        self.assertFalse(h._context.post_handshake_auth)

        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT, cert_file=CERT_localhost)
        context.post_handshake_auth = True
        h = client.HTTPSConnection('localhost', 443, context=context)
        self.assertTrue(h._context.post_handshake_auth)


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

    def test_list_body(self):
        # Note that no content-length is automatically calculated for
        # an iterable.  The request will fall back to send chunked
        # transfer encoding.
        cases = (
            ([b'foo', b'bar'], b'3\r\nfoo\r\n3\r\nbar\r\n0\r\n\r\n'),
            ((b'foo', b'bar'), b'3\r\nfoo\r\n3\r\nbar\r\n0\r\n\r\n'),
        )
        for body, expected in cases:
            with self.subTest(body):
                self.conn = client.HTTPConnection('example.com')
                self.conn.sock = self.sock = FakeSocket('')

                self.conn.request('PUT', '/url', body)
                msg, f = self.get_headers_and_fp()
                self.assertNotIn('Content-Type', msg)
                self.assertNotIn('Content-Length', msg)
                self.assertEqual(msg.get('Transfer-Encoding'), 'chunked')
                self.assertEqual(expected, f.read())

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
        self.assertIsNone(message.get_charset())
        self.assertEqual("4", message.get("content-length"))
        self.assertEqual(b'body', f.read())

    def test_latin1_body(self):
        self.conn.request("PUT", "/url", "body\xc1")
        message, f = self.get_headers_and_fp()
        self.assertEqual("text/plain", message.get_content_type())
        self.assertIsNone(message.get_charset())
        self.assertEqual("5", message.get("content-length"))
        self.assertEqual(b'body\xc1', f.read())

    def test_bytes_body(self):
        self.conn.request("PUT", "/url", b"body\xc1")
        message, f = self.get_headers_and_fp()
        self.assertEqual("text/plain", message.get_content_type())
        self.assertIsNone(message.get_charset())
        self.assertEqual("5", message.get("content-length"))
        self.assertEqual(b'body\xc1', f.read())

    def test_text_file_body(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            f.write("body")
        with open(os_helper.TESTFN, encoding="utf-8") as f:
            self.conn.request("PUT", "/url", f)
            message, f = self.get_headers_and_fp()
            self.assertEqual("text/plain", message.get_content_type())
            self.assertIsNone(message.get_charset())
            # No content-length will be determined for files; the body
            # will be sent using chunked transfer encoding instead.
            self.assertIsNone(message.get("content-length"))
            self.assertEqual("chunked", message.get("transfer-encoding"))
            self.assertEqual(b'4\r\nbody\r\n0\r\n\r\n', f.read())

    def test_binary_file_body(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with open(os_helper.TESTFN, "wb") as f:
            f.write(b"body\xc1")
        with open(os_helper.TESTFN, "rb") as f:
            self.conn.request("PUT", "/url", f)
            message, f = self.get_headers_and_fp()
            self.assertEqual("text/plain", message.get_content_type())
            self.assertIsNone(message.get_charset())
            self.assertEqual("chunked", message.get("Transfer-Encoding"))
            self.assertNotIn("Content-Length", message)
            self.assertEqual(b'5\r\nbody\xc1\r\n0\r\n\r\n', f.read())


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

class TunnelTests(TestCase):
    def setUp(self):
        response_text = (
            'HTTP/1.1 200 OK\r\n\r\n' # Reply to CONNECT
            'HTTP/1.1 200 OK\r\n' # Reply to HEAD
            'Content-Length: 42\r\n\r\n'
        )
        self.host = 'proxy.com'
        self.port = client.HTTP_PORT
        self.conn = client.HTTPConnection(self.host)
        self.conn._create_connection = self._create_connection(response_text)

    def tearDown(self):
        self.conn.close()

    def _create_connection(self, response_text):
        def create_connection(address, timeout=None, source_address=None):
            return FakeSocket(response_text, host=address[0], port=address[1])
        return create_connection

    def test_set_tunnel_host_port_headers_add_host_missing(self):
        tunnel_host = 'destination.com'
        tunnel_port = 8888
        tunnel_headers = {'User-Agent': 'Mozilla/5.0 (compatible, MSIE 11)'}
        tunnel_headers_after = tunnel_headers.copy()
        tunnel_headers_after['Host'] = '%s:%d' % (tunnel_host, tunnel_port)
        self.conn.set_tunnel(tunnel_host, port=tunnel_port,
                             headers=tunnel_headers)
        self.conn.request('HEAD', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, self.port)
        self.assertEqual(self.conn._tunnel_host, tunnel_host)
        self.assertEqual(self.conn._tunnel_port, tunnel_port)
        self.assertEqual(self.conn._tunnel_headers, tunnel_headers_after)

    def test_set_tunnel_host_port_headers_set_host_identical(self):
        tunnel_host = 'destination.com'
        tunnel_port = 8888
        tunnel_headers = {'User-Agent': 'Mozilla/5.0 (compatible, MSIE 11)',
                          'Host': '%s:%d' % (tunnel_host, tunnel_port)}
        self.conn.set_tunnel(tunnel_host, port=tunnel_port,
                             headers=tunnel_headers)
        self.conn.request('HEAD', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, self.port)
        self.assertEqual(self.conn._tunnel_host, tunnel_host)
        self.assertEqual(self.conn._tunnel_port, tunnel_port)
        self.assertEqual(self.conn._tunnel_headers, tunnel_headers)

    def test_set_tunnel_host_port_headers_set_host_different(self):
        tunnel_host = 'destination.com'
        tunnel_port = 8888
        tunnel_headers = {'User-Agent': 'Mozilla/5.0 (compatible, MSIE 11)',
                          'Host': '%s:%d' % ('example.com', 4200)}
        self.conn.set_tunnel(tunnel_host, port=tunnel_port,
                             headers=tunnel_headers)
        self.conn.request('HEAD', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, self.port)
        self.assertEqual(self.conn._tunnel_host, tunnel_host)
        self.assertEqual(self.conn._tunnel_port, tunnel_port)
        self.assertEqual(self.conn._tunnel_headers, tunnel_headers)

    def test_disallow_set_tunnel_after_connect(self):
        # Once connected, we shouldn't be able to tunnel anymore
        self.conn.connect()
        self.assertRaises(RuntimeError, self.conn.set_tunnel,
                          'destination.com')

    def test_connect_with_tunnel(self):
        d = {
            b'host': b'destination.com',
            b'port': client.HTTP_PORT,
        }
        self.conn.set_tunnel(d[b'host'].decode('ascii'))
        self.conn.request('HEAD', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, self.port)
        self.assertIn(b'CONNECT %(host)s:%(port)d HTTP/1.1\r\n'
                      b'Host: %(host)s:%(port)d\r\n\r\n' % d,
                      self.conn.sock.data)
        self.assertIn(b'HEAD / HTTP/1.1\r\nHost: %(host)s\r\n' % d,
                      self.conn.sock.data)

    def test_connect_with_tunnel_with_default_port(self):
        d = {
            b'host': b'destination.com',
            b'port': client.HTTP_PORT,
        }
        self.conn.set_tunnel(d[b'host'].decode('ascii'), port=d[b'port'])
        self.conn.request('HEAD', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, self.port)
        self.assertIn(b'CONNECT %(host)s:%(port)d HTTP/1.1\r\n'
                      b'Host: %(host)s:%(port)d\r\n\r\n' % d,
                      self.conn.sock.data)
        self.assertIn(b'HEAD / HTTP/1.1\r\nHost: %(host)s\r\n' % d,
                      self.conn.sock.data)

    def test_connect_with_tunnel_with_nonstandard_port(self):
        d = {
            b'host': b'destination.com',
            b'port': 8888,
        }
        self.conn.set_tunnel(d[b'host'].decode('ascii'), port=d[b'port'])
        self.conn.request('HEAD', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, self.port)
        self.assertIn(b'CONNECT %(host)s:%(port)d HTTP/1.1\r\n'
                      b'Host: %(host)s:%(port)d\r\n\r\n' % d,
                      self.conn.sock.data)
        self.assertIn(b'HEAD / HTTP/1.1\r\nHost: %(host)s:%(port)d\r\n' % d,
                      self.conn.sock.data)

    # This request is not RFC-valid, but it's been possible with the library
    # for years, so don't break it unexpectedly... This also tests
    # case-insensitivity when injecting Host: headers if they're missing.
    def test_connect_with_tunnel_with_different_host_header(self):
        d = {
            b'host': b'destination.com',
            b'tunnel_host_header': b'example.com:9876',
            b'port': client.HTTP_PORT,
        }
        self.conn.set_tunnel(
            d[b'host'].decode('ascii'),
            headers={'HOST': d[b'tunnel_host_header'].decode('ascii')})
        self.conn.request('HEAD', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, self.port)
        self.assertIn(b'CONNECT %(host)s:%(port)d HTTP/1.1\r\n'
                      b'HOST: %(tunnel_host_header)s\r\n\r\n' % d,
                      self.conn.sock.data)
        self.assertIn(b'HEAD / HTTP/1.1\r\nHost: %(host)s\r\n' % d,
                      self.conn.sock.data)

    def test_connect_with_tunnel_different_host(self):
        d = {
            b'host': b'destination.com',
            b'port': client.HTTP_PORT,
        }
        self.conn.set_tunnel(d[b'host'].decode('ascii'))
        self.conn.request('HEAD', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, self.port)
        self.assertIn(b'CONNECT %(host)s:%(port)d HTTP/1.1\r\n'
                      b'Host: %(host)s:%(port)d\r\n\r\n' % d,
                      self.conn.sock.data)
        self.assertIn(b'HEAD / HTTP/1.1\r\nHost: %(host)s\r\n' % d,
                      self.conn.sock.data)

    def test_connect_with_tunnel_idna(self):
        dest = '\u03b4\u03c0\u03b8.gr'
        dest_port = b'%s:%d' % (dest.encode('idna'), client.HTTP_PORT)
        expected = b'CONNECT %s HTTP/1.1\r\nHost: %s\r\n\r\n' % (
            dest_port, dest_port)
        self.conn.set_tunnel(dest)
        self.conn.request('HEAD', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, client.HTTP_PORT)
        self.assertIn(expected, self.conn.sock.data)

    def test_tunnel_connect_single_send_connection_setup(self):
        """Regresstion test for https://bugs.python.org/issue43332."""
        with mock.patch.object(self.conn, 'send') as mock_send:
            self.conn.set_tunnel('destination.com')
            self.conn.connect()
            self.conn.request('GET', '/')
        mock_send.assert_called()
        # Likely 2, but this test only cares about the first.
        self.assertGreater(
                len(mock_send.mock_calls), 1,
                msg=f'unexpected number of send calls: {mock_send.mock_calls}')
        proxy_setup_data_sent = mock_send.mock_calls[0][1][0]
        self.assertIn(b'CONNECT destination.com', proxy_setup_data_sent)
        self.assertEndsWith(proxy_setup_data_sent, b'\r\n\r\n',
                msg=f'unexpected proxy data sent {proxy_setup_data_sent!r}')

    def test_connect_put_request(self):
        d = {
            b'host': b'destination.com',
            b'port': client.HTTP_PORT,
        }
        self.conn.set_tunnel(d[b'host'].decode('ascii'))
        self.conn.request('PUT', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, self.port)
        self.assertIn(b'CONNECT %(host)s:%(port)d HTTP/1.1\r\n'
                      b'Host: %(host)s:%(port)d\r\n\r\n' % d,
                      self.conn.sock.data)
        self.assertIn(b'PUT / HTTP/1.1\r\nHost: %(host)s\r\n' % d,
                      self.conn.sock.data)

    def test_connect_put_request_ipv6(self):
        self.conn.set_tunnel('[1:2:3::4]', 1234)
        self.conn.request('PUT', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, client.HTTP_PORT)
        self.assertIn(b'CONNECT [1:2:3::4]:1234', self.conn.sock.data)
        self.assertIn(b'Host: [1:2:3::4]:1234', self.conn.sock.data)

    def test_connect_put_request_ipv6_port(self):
        self.conn.set_tunnel('[1:2:3::4]:1234')
        self.conn.request('PUT', '/', '')
        self.assertEqual(self.conn.sock.host, self.host)
        self.assertEqual(self.conn.sock.port, client.HTTP_PORT)
        self.assertIn(b'CONNECT [1:2:3::4]:1234', self.conn.sock.data)
        self.assertIn(b'Host: [1:2:3::4]:1234', self.conn.sock.data)

    def test_tunnel_debuglog(self):
        expected_header = 'X-Dummy: 1'
        response_text = 'HTTP/1.0 200 OK\r\n{}\r\n\r\n'.format(expected_header)

        self.conn.set_debuglevel(1)
        self.conn._create_connection = self._create_connection(response_text)
        self.conn.set_tunnel('destination.com')

        with support.captured_stdout() as output:
            self.conn.request('PUT', '/', '')
        lines = output.getvalue().splitlines()
        self.assertIn('header: {}'.format(expected_header), lines)

    def test_proxy_response_headers(self):
        expected_header = ('X-Dummy', '1')
        response_text = (
            'HTTP/1.0 200 OK\r\n'
            '{0}\r\n\r\n'.format(':'.join(expected_header))
        )

        self.conn._create_connection = self._create_connection(response_text)
        self.conn.set_tunnel('destination.com')

        self.conn.request('PUT', '/', '')
        headers = self.conn.get_proxy_response_headers()
        self.assertIn(expected_header, headers.items())

    def test_no_proxy_response_headers(self):
        expected_header = ('X-Dummy', '1')
        response_text = (
            'HTTP/1.0 200 OK\r\n'
            '{0}\r\n\r\n'.format(':'.join(expected_header))
        )

        self.conn._create_connection = self._create_connection(response_text)

        self.conn.request('PUT', '/', '')
        headers = self.conn.get_proxy_response_headers()
        self.assertIsNone(headers)

    def test_tunnel_leak(self):
        sock = None

        def _create_connection(address, timeout=None, source_address=None):
            nonlocal sock
            sock = FakeSocket(
                'HTTP/1.1 404 NOT FOUND\r\n\r\n',
                host=address[0],
                port=address[1],
            )
            return sock

        self.conn._create_connection = _create_connection
        self.conn.set_tunnel('destination.com')
        exc = None
        try:
            self.conn.request('HEAD', '/', '')
        except OSError as e:
            # keeping a reference to exc keeps response alive in the traceback
            exc = e
        self.assertIsNotNone(exc)
        self.assertTrue(sock.file_closed)


if __name__ == '__main__':
    unittest.main(verbosity=2)
