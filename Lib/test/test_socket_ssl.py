# Test just the SSL support in the socket module, in a moderately bogus way.

import test_support

# Optionally test SSL support.  This currently requires the 'network' resource
# as given on the regrtest command line.  If not available, nothing after this
# line will be executed.
test_support.requires('network')

import socket
import urllib

urllib.urlopen('https://sf.net')
