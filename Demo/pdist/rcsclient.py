"""Customize this file to change the default client etc.

(In general, it is probably be better to make local operation the
default and to require something like an RCSSERVER environment
variable to enable remote operation.)

"""

import string
import os

# These defaults don't belong here -- they should be taken from the
# environment or from a hidden file in the current directory

HOST = 'voorn.cwi.nl'
PORT = 4127
VERBOSE = 1
LOCAL = 0

import client


class RCSProxyClient(client.SecureClient):
	
	def __init__(self, address, verbose = client.VERBOSE):
		client.SecureClient.__init__(self, address, verbose)


def openrcsclient(opts = []):
	"open an RCSProxy client based on a list of options returned by getopt"
	import RCSProxy
	host = HOST
	port = PORT
	verbose = VERBOSE
	local = LOCAL
	directory = None
	for o, a in opts:
		if o == '-h':
			host = a
			if ':' in host:
				i = string.find(host, ':')
				host, p = host[:i], host[i+1:]
				if p:
					port = string.atoi(p)
		if o == '-p':
			port = string.atoi(a)
		if o == '-d':
			directory = a
		if o == '-v':
			verbose = verbose + 1
		if o == '-q':
			verbose = 0
		if o == '-L':
			local = 1
	if local:
		import RCSProxy
		x = RCSProxy.RCSProxyLocal()
	else:
		address = (host, port)
		x = RCSProxyClient(address, verbose)
	if not directory:
		try:
			directory = open(os.path.join("CVS", "Repository")).readline()
		except IOError:
			pass
		else:
			if directory[-1] == '\n':
				directory = directory[:-1]
	if directory:
		x.cd(directory)
	return x
