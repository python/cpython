from test_support import verify,verbose
import httplib
import StringIO

class FakeSocket:
    def __init__(self, text):
        self.text = text

    def makefile(self, mode, bufsize=None):
        if mode != 'r' and mode != 'rb':
            raise httplib.UnimplementedFileMode()
        return StringIO.StringIO(self.text)

# Test HTTP status lines

body = "HTTP/1.1 200 Ok\n\nText"
sock = FakeSocket(body)
resp = httplib.HTTPResponse(sock, 1)
resp.begin()
print resp.read()
resp.close()

body = "HTTP/1.1 400.100 Not Ok\n\nText"
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
text = ('HTTP/1.1 200 OK\n'
        'Set-Cookie: Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"\n'
        'Set-Cookie: Part_Number="Rocket_Launcher_0001"; Version="1";'
        ' Path="/acme"\n'
        '\n'
        'No body\n')
hdr = ('Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"'
       ', '
       'Part_Number="Rocket_Launcher_0001"; Version="1"; Path="/acme"')
s = FakeSocket(text)
r = httplib.HTTPResponse(s, 1)
r.begin()
cookies = r.getheader("Set-Cookie")
if cookies != hdr:
    raise AssertionError, "multiple headers not combined properly"
