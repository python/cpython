from test_support import TestFailed
import mimetools

import string,StringIO
start = string.letters + "=" + string.digits + "\n"
for enc in ['7bit','8bit','base64','quoted-printable']:
    print enc,
    i = StringIO.StringIO(start)
    o = StringIO.StringIO()
    mimetools.encode(i,o,enc)
    i = StringIO.StringIO(o.getvalue())
    o = StringIO.StringIO()
    mimetools.decode(i,o,enc)
    if o.getvalue()==start:
        print "PASS"
    else:
        print "FAIL"
        print o.getvalue()
