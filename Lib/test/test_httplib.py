import httplib
import StringIO
import sys

from test.test_support import verify,verbose

class FakeSocket:
    def __init__(self, text, fileclass=StringIO.StringIO):
        self.text = text
        self.fileclass = fileclass

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

test()
