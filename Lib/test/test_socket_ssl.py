# Test just the SSL support in the socket module, in a moderately bogus way.

import test_support

# Optionally test SSL support.  This currently requires the 'network' resource
# as given on the regrtest command line.  If not available, nothing after this
# line will be executed.
test_support.requires('network')

import socket
if not hasattr(socket, "ssl"):
    raise test_support.TestSkipped("socket module has no ssl support")

import urllib

socket.RAND_status()
try:
    socket.RAND_egd(1)
except TypeError:
    pass
else:
    print "didn't raise TypeError"
socket.RAND_add("this is a random string", 75.0)

f = urllib.urlopen('https://sf.net')
buf = f.read()
f.close()
