from test_support import verify
import urllib2
import os

# A couple trivial tests

try:
    urllib2.urlopen('bogus url')
except ValueError:
    pass
else:
    verify(0)

# XXX Name hacking to get this to work on Windows.
fname = os.path.abspath(urllib2.__file__).replace('\\', '/')
if fname[1:2] == ":":
    fname = fname[2:]
file_url = "file://%s" % fname
f = urllib2.urlopen(file_url)

buf = f.read()
f.close()
