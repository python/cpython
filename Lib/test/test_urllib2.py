from test_support import verify
import urllib2

# A couple trivial tests

try:
    urllib2.urlopen('bogus url')
except ValueError:
    pass
else:
    verify(0)

file_url = "file://%s" % urllib2.__file__
f = urllib2.urlopen(file_url)
buf = f.read()
f.close()
