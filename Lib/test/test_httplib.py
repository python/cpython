from test.test_support import verify,verbose
import httplib
import StringIO

class FakeSocket:
    def __init__(self, text):
        self.text = text

    def makefile(self, mode, bufsize=None):
        if mode != 'r' and mode != 'rb':
            raise httplib.UnimplementedFileMode()
        return StringIO.StringIO(self.text)

# Collect output to a buffer so that we don't have to cope with line-ending
# issues across platforms.  Specifically, the headers will have \r\n pairs
# and some platforms will strip them from the output file.

import sys

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

test()
