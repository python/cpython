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
# And more hacking to get it to work on MacOS. This assumes
# urllib.pathname2url works, unfortunately...
if os.name == 'mac':
    fname = '/' + fname.replace(':', '/')
elif os.name == 'riscos':
    import string
    fname = os.expand(fname)
    fname = fname.translate(string.maketrans("/.", "./"))

file_url = "file://%s" % fname
f = urllib2.urlopen(file_url)

buf = f.read()
f.close()
