import httplib
import StringIO
import sys

from unittest import TestCase

from test import test_support

class FakeSocket:
    def __init__(self, text, fileclass=StringIO.StringIO):
        self.text = text
        self.fileclass = fileclass

    def sendall(self, data):
        self.data = data

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

# Collect output to a buffer so that we don't have to cope with line-ending
# issues across platforms.  Specifically, the headers will have \r\n pairs
# and some platforms will strip them from the output file.

def test():
    buf = StringIO.StringIO()
    _stdout = sys.stdout
    try:
        sys.stdout = buf
        _test()
    finally:
        sys.stdout = _stdout

    # print individual lines with endings stripped
    s = buf.getvalue()
    for line in s.split("\n"):
        print line.strip()

def _test():
    # Test HTTP status lines

    body = "HTTP/1.1 200 Ok\r\n\r\nText"
    sock = FakeSocket(body)
    resp = httplib.HTTPResponse(sock, 1)
    resp.begin()
    print resp.read()
    resp.close()

    body = "HTTP/1.1 400.100 Not Ok\r\n\r\nText"
    sock = FakeSocket(body)
    resp = httplib.HTTPResponse(sock, 1)
    try:
        resp.begin()
    except httplib.BadStatusLine:
        print "BadStatusLine raised as expected"
    else:
        print "Expect BadStatusLine"

    # Check invalid host_port

    for hp in ("www.python.org:abc", "www.python.org:"):
        try:
            h = httplib.HTTP(hp)
        except httplib.InvalidURL:
            print "InvalidURL raised as expected"
        else:
            print "Expect InvalidURL"

    for hp,h,p in (("[fe80::207:e9ff:fe9b]:8000", "fe80::207:e9ff:fe9b", 8000),
                   ("www.python.org:80", "www.python.org", 80),
                   ("www.python.org", "www.python.org", 80),
                   ("[fe80::207:e9ff:fe9b]", "fe80::207:e9ff:fe9b", 80)):
        try:
            http = httplib.HTTP(hp)
        except httplib.InvalidURL:
            print "InvalidURL raised erroneously"
        c = http._conn
        if h != c.host: raise AssertionError, ("Host incorrectly parsed", h, c.host)
        if p != c.port: raise AssertionError, ("Port incorrectly parsed", p, c.host)

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
    r = httplib.HTTPResponse(s, 1)
    r.begin()
    cookies = r.getheader("Set-Cookie")
    if cookies != hdr:
        raise AssertionError, "multiple headers not combined properly"

    # Test that the library doesn't attempt to read any data
    # from a HEAD request.  (Tickles SF bug #622042.)
    sock = FakeSocket(
        'HTTP/1.1 200 OK\r\n'
        'Content-Length: 14432\r\n'
        '\r\n',
        NoEOFStringIO)
    resp = httplib.HTTPResponse(sock, 1, method="HEAD")
    resp.begin()
    if resp.read() != "":
        raise AssertionError, "Did not expect response from HEAD request"
    resp.close()


def test_main(verbose=None):
    tests = [HeaderTests,]
    test_support.run_unittest(*tests)

test()
